/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

#include "vp8_decoder.h"

vpx_codec_err_t vp8_init(vpx_codec_alg_priv_t *ctx)
{
    printf("[VP8] vp8_init()\n");

    return VPX_CODEC_OK;
}

vpx_codec_err_t vp8_destroy(vpx_codec_alg_priv_t *ctx)
{
    printf("[VP8] vp8_destroy()\n");

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

vpx_codec_err_t vp8_decode(vpx_codec_alg_priv_t *ctx,
                           const uint8_t        *data,
                           unsigned int          data_sz,
                           long                  deadline)
{
    vpx_codec_err_t res = VPX_CODEC_OK;
    ctx->img_avail = 0;

    /* printf("[VP8] vp8_decode()\n"); */

    // Determine the stream parameters
    if (!ctx->si.h)
        res = vp8_peek_si(data, data_sz, &ctx->si);

    // Initialize the decoder instance on the first frame
    if (!ctx->decoder_init)
    {
        VP8D_PTR optr;

        vp8dx_initialize();

        optr = vp8dx_create_decompressor(ctx->si.w, ctx->si.h, 0);

        if (!optr)
            res = VPX_CODEC_ERROR;
        else
            ctx->pbi = optr;

        ctx->decoder_init = 1;
    }

    if (!res && ctx->pbi)
    {
        int64_t time_stamp = 0, time_end_stamp = 0;

        if (vp8dx_receive_compressed_data(ctx->pbi, data_sz, data, deadline))
        {
            res = ((VP8D_COMP *)ctx->pbi)->common.error.error_code;
        }

        if (!res && 0 == vp8dx_get_raw_frame(ctx->pbi, &ctx->img_yv12, &time_stamp, &time_end_stamp))
        {
            ctx->img_avail = 1;
        }
    }

    return res;
}

vpx_codec_err_t vp8_dump_frame(vpx_codec_alg_priv_t *ctx)
{
    vpx_codec_err_t res = VPX_CODEC_ERROR;

    if (ctx->img_avail)
    {
        // Write the frame on disk (for debugging purpose only)
        FILE *outfile = fopen("output_img.yv12", "wb");
        if (outfile)
        {
           unsigned y;

           for (y = 0; y < ctx->img_yv12.y_height; y++)
           {
              fwrite(ctx->img_yv12.y_buffer, 1, ctx->img_yv12.y_width, outfile);
           }

           for (y = 0; y < ctx->img_yv12.uv_height; y++)
           {
              fwrite(ctx->img_yv12.u_buffer, 1, ctx->img_yv12.uv_width, outfile);
           }

           for (y = 0; y < ctx->img_yv12.uv_height; y++)
           {
              fwrite(ctx->img_yv12.v_buffer, 1, ctx->img_yv12.uv_width, outfile);
           }

           fclose(outfile);
           res = VPX_CODEC_OK;
           printf("[G3DVL] Image written on disk !\n");
        }
    }

    return res;
}

YV12_BUFFER_CONFIG *vp8_get_frame(vpx_codec_alg_priv_t *ctx)
{
    YV12_BUFFER_CONFIG *yv12 = NULL;

    if (ctx->img_avail)
    {
        yv12 = &ctx->img_yv12;
    }

    return yv12;
}
