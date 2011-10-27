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

vpx_codec_err_t vp8_decode(vpx_codec_alg_priv_t *ctx,
                           const uint8_t        *data,
                           unsigned              data_size,
                           int64_t               deadline)
{
    vpx_codec_err_t res = VPX_CODEC_OK;
    ctx->img_avail = 0;

    /* printf("[VP8] vp8_decode()\n"); */

    // Initialize the decoder instance on the first frame
    if (!ctx->decoder_init)
    {
        VP8D_PTR optr;

        vp8dx_initialize();

        optr = vp8dx_create_decompressor(0);

        if (!optr)
            res = VPX_CODEC_ERROR;
        else
            ctx->pbi = optr;

        ctx->decoder_init = 1;
    }

    if (!res && ctx->pbi)
    {
        int64_t time_stamp = 0, time_end_stamp = 0;

        if (vp8dx_receive_compressed_data(ctx->pbi, data, data_size, deadline))
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
