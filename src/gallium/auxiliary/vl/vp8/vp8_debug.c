/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdio.h>
#include <stdarg.h>

#include "vp8_debug.h"

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
    case VPX_CODEC_UNSUP_BITSTREAM:
        return "Bitstream not supported by this decoder";
    case VPX_CODEC_CORRUPT_FRAME:
        return "Corrupt frame detected";
    case  VPX_CODEC_INVALID_PARAM:
        return "Invalid parameter";
    }

    return "Unrecognized error code";
}

void vpx_internal_error(struct vpx_internal_error_info *info,
                        vpx_codec_err_t                 error,
                        const char                     *fmt,
                        ...)
{
    va_list ap;

    info->error_code = error;
    info->has_detail = 0;

    if (fmt)
    {
        size_t  sz = sizeof(info->detail);

        info->has_detail = 1;
        va_start(ap, fmt);
        vsnprintf(info->detail, sz - 1, fmt, ap);
        va_end(ap);
        info->detail[sz-1] = '\0';
    }

    if (info->setjmp)
        longjmp(info->jmp, info->error_code);
}
