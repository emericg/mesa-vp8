/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


/*!\file
 * \brief Provides the high level interface to wrap decoder algorithms.
 */

#include "vpx_codec.h"

const char *vpx_codec_err_to_string(vpx_codec_err_t err)
{
    switch (err)
    {
    case VPX_CODEC_OK:
        return "Success";
    case VPX_CODEC_ERROR:
        return "Unspecified internal error";
    case VPX_CODEC_MEM_ERROR:
        return "Memory allocation error";
    case VPX_CODEC_ABI_MISMATCH:
        return "ABI version mismatch";
    case VPX_CODEC_INCAPABLE:
        return "Codec does not implement requested capability";
    case VPX_CODEC_UNSUP_BITSTREAM:
        return "Bitstream not supported by this decoder";
    case VPX_CODEC_UNSUP_FEATURE:
        return "Bitstream required feature not supported by this decoder";
    case VPX_CODEC_CORRUPT_FRAME:
        return "Corrupt frame detected";
    case  VPX_CODEC_INVALID_PARAM:
        return "Invalid parameter";
    case VPX_CODEC_LIST_END:
        return "End of iterated list";
    }

    return "Unrecognized error code";
}

const char *vpx_codec_error(vpx_codec_ctx_t *ctx)
{
    return (ctx) ? vpx_codec_err_to_string(ctx->err)
           : vpx_codec_err_to_string(VPX_CODEC_INVALID_PARAM);
}

const char *vpx_codec_error_detail(vpx_codec_ctx_t *ctx)
{
    if (ctx && ctx->err)
        return ctx->priv ? ctx->priv->err_detail : ctx->err_detail;

    return NULL;
}
