/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vp8_decoder.h"

static const mem_req_t vp8_mem_req_segs[] =
{
    {VP8_SEG_ALG_PRIV, 0, 8, 1},
    {VP8_SEG_MAX, 0, 0, 0}
};

struct vpx_codec_alg_priv
{
    vpx_codec_priv_t        base;
    vpx_codec_mmap_t        mmaps[NELEMENTS(vp8_mem_req_segs)-1];
    vp8_stream_info_t       si;
    int                     defer_alloc;
    int                     decoder_init;
    VP8D_PTR                pbi;
    vpx_image_t             img;
    int                     img_setup;
    int                     img_avail;
};

static void vp8_mmap_dtor(vpx_codec_mmap_t *mmap)
{
    free(mmap->priv);
}

static vpx_codec_err_t vp8_mmap_alloc(vpx_codec_mmap_t *mmap)
{
    vpx_codec_err_t res;
    unsigned int align;

    align = mmap->align ? mmap->align - 1 : 0;

    if (mmap->flags == 1)
        mmap->priv = calloc(1, mmap->sz + align);
    else
        mmap->priv = malloc(mmap->sz + align);

    res = (mmap->priv) ? VPX_CODEC_OK : VPX_CODEC_MEM_ERROR;
    mmap->base = (void *)((((uintptr_t)mmap->priv) + align) & ~(uintptr_t)align);
    mmap->dtor = vp8_mmap_dtor;

    return res;
}

static vpx_codec_err_t vp8_validate_mmaps(const vp8_stream_info_t *si,
                                          const vpx_codec_mmap_t  *mmaps,
                                          vpx_codec_flags_t        init_flags)
{
    int i;
    vpx_codec_err_t res = VPX_CODEC_OK;

    for (i = 0; i < NELEMENTS(vp8_mem_req_segs) - 1; i++)
    {
        /* Ensure the segment has been allocated */
        if (!mmaps[i].base)
        {
            res = VPX_CODEC_MEM_ERROR;
            break;
        }

        /* Verify variable size segment is big enough for the current si. */
        {
            vpx_codec_dec_cfg_t cfg;

            cfg.w = si->w;
            cfg.h = si->h;

            if (mmaps[i].sz < sizeof(si))
            {
                res = VPX_CODEC_MEM_ERROR;
                break;
            }
        }
    }

    return res;
}

vpx_codec_err_t vp8_init(vpx_codec_ctx_t *ctx)
{
    vpx_codec_err_t res = VPX_CODEC_OK;

    printf("[VP8] vp8_init()\n");

    /* This function only allocates space for the vpx_codec_alg_priv_t
     * structure. More memory may be required at the time the stream
     * information becomes known.
     */
    if (!ctx->priv)
    {
        vpx_codec_mmap_t mmap;

        mmap.id = vp8_mem_req_segs[0].id;
        mmap.sz = sizeof(vpx_codec_alg_priv_t);
        mmap.align = vp8_mem_req_segs[0].align;
        mmap.flags = vp8_mem_req_segs[0].flags;

        res = vp8_mmap_alloc(&mmap);

        if (!res)
        {
            int i;

            ctx->priv = mmap.base;
            ctx->priv->sz = sizeof(*ctx->priv);
            ctx->priv->alg_priv = mmap.base;

            for (i = 0; i < NELEMENTS(ctx->priv->alg_priv->mmaps); i++)
                ctx->priv->alg_priv->mmaps[i].id = vp8_mem_req_segs[i].id;

            ctx->priv->alg_priv->mmaps[0] = mmap;
            ctx->priv->alg_priv->si.sz = sizeof(ctx->priv->alg_priv->si);
            ctx->priv->alg_priv->defer_alloc = 1;
        }
    }

    return res;
}

vpx_codec_err_t vp8_destroy(vpx_codec_alg_priv_t *ctx)
{
    int i;

    printf("[VP8] vp8_destroy()\n");

    vp8dx_remove_decompressor(ctx->pbi);

    for (i = NELEMENTS(ctx->mmaps) - 1; i >= 0; i--)
    {
        if (ctx->mmaps[i].dtor)
            ctx->mmaps[i].dtor(&ctx->mmaps[i]);
    }

    return VPX_CODEC_OK;
}

static vpx_codec_err_t vp8_peek_si(const uint8_t     *data,
                                   unsigned int       data_size,
                                   vp8_stream_info_t *stream_infos)
{
    vpx_codec_err_t res = VPX_CODEC_OK;

    printf("[VP8] vp8_peek_si()\n");

    if (data + data_size <= data)
    {
        res = VPX_CODEC_INVALID_PARAM;
    }
    else
    {
        /* Parse uncompresssed part of key frame header.
         * 3 bytes:- including version, frame type and an offset
         * 3 bytes:- sync code (0x9d, 0x01, 0x2a)
         * 4 bytes:- including image width and height in the lowest 14 bits
         *           of each 2-byte value.
         */
        if (data_size >= 10 && !(data[0] & 0x01))  /* I-Frame */
        {
            stream_infos->is_kf = 1;
            const uint8_t *c = data + 3;

            // Check start_code
            if (c[0] != 0x9d || c[1] != 0x01 || c[2] != 0x2a)
                res = VPX_CODEC_UNSUP_BITSTREAM;

            stream_infos->w = (c[3] | (c[4] << 8)) & 0x3fff;
            stream_infos->h = (c[5] | (c[6] << 8)) & 0x3fff;

            // printf("w=%d, h=%d\n", si->w, si->h);
            if (!(stream_infos->h | stream_infos->w))
                res = VPX_CODEC_UNSUP_BITSTREAM;
        }
        else
        {
           stream_infos->is_kf = 0;
           res = VPX_CODEC_UNSUP_BITSTREAM;
        }
    }

    return res;
}

static vpx_codec_err_t
update_error_state(vpx_codec_alg_priv_t                 *ctx,
                   const struct vpx_internal_error_info *error)
{
    vpx_codec_err_t res = error->error_code;

    if (res)
        ctx->base.err_detail = error->has_detail
                               ? error->detail
                               : NULL;

    return res;
}

vpx_codec_err_t vp8_decode(vpx_codec_alg_priv_t *ctx,
                           const uint8_t        *data,
                           unsigned int          data_sz,
                           void                 *user_priv,
                           long                  deadline)
{
    vpx_codec_err_t res = VPX_CODEC_OK;
    ctx->img_avail = 0;

    /* printf("[VP8] vp8_decode()\n"); */

    /* Determine the stream parameters. Note that we rely on peek_si to
     * validate that we have a buffer that does not wrap around the top
     * of the heap.
     */
    if (!ctx->si.h)
        res = vp8_peek_si(data, data_sz, &ctx->si);

    /* Perform deferred allocations, if required */
    if (!res && ctx->defer_alloc)
    {
        int i;

        for (i = 1; !res && i < NELEMENTS(ctx->mmaps); i++)
        {
            vpx_codec_dec_cfg_t cfg;

            cfg.w = ctx->si.w;
            cfg.h = ctx->si.h;
            ctx->mmaps[i].id = vp8_mem_req_segs[i].id;
            ctx->mmaps[i].sz = vp8_mem_req_segs[i].sz;
            ctx->mmaps[i].align = vp8_mem_req_segs[i].align;
            ctx->mmaps[i].flags = vp8_mem_req_segs[i].flags;

            if (!ctx->mmaps[i].sz)
            {
                (void)&cfg;
                ctx->mmaps[i].sz = sizeof(vpx_codec_alg_priv_t);
            }

            res = vp8_mmap_alloc(&ctx->mmaps[i]);
        }

        ctx->defer_alloc = 0;
    }

    /* Initialize the decoder instance on the first frame*/
    if (!res && !ctx->decoder_init)
    {
        res = vp8_validate_mmaps(&ctx->si, ctx->mmaps, ctx->base.init_flags);

        if (!res)
        {
            VP8D_CONFIG oxcf;
            VP8D_PTR optr;

            vp8dx_initialize();

            oxcf.Width = ctx->si.w;
            oxcf.Height = ctx->si.h;
            oxcf.input_partition = 0;

            optr = vp8dx_create_decompressor(&oxcf);

            if (!optr)
                res = VPX_CODEC_ERROR;
            else
                ctx->pbi = optr;
        }

        ctx->decoder_init = 1;
    }

    if (!res && ctx->pbi)
    {
        YV12_BUFFER_CONFIG sd;
        int64_t time_stamp = 0, time_end_stamp = 0;

        if (vp8dx_receive_compressed_data(ctx->pbi, data_sz, data, deadline))
        {
            VP8D_COMP *pbi = (VP8D_COMP *)ctx->pbi;
            res = update_error_state(ctx, &pbi->common.error);
        }

        if (!res && 0 == vp8dx_get_raw_frame(ctx->pbi, &sd, &time_stamp, &time_end_stamp))
        {
            yuvconfig2image(&ctx->img, &sd, user_priv);
            ctx->img_avail = 1;
        }
    }

    return res;
}

vpx_image_t *vp8_get_frame(vpx_codec_alg_priv_t *ctx,
                           vpx_codec_iter_t     *iter)
{
    vpx_image_t *img = NULL;

    /* printf("[VP8] vp8_get_frame()\n"); */

    if (ctx->img_avail)
    {
        /* iter acts as a flip flop, so an image is only returned on the first
         * call to get_frame.
         */
        if (!(*iter))
        {
            img = &ctx->img;
            *iter = img;
        }
    }

    return img;
}
