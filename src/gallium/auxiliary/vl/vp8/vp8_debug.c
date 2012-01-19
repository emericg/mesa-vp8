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
        size_t sz = sizeof(info->detail);

        info->has_detail = 1;
        va_start(ap, fmt);
        vsnprintf(info->detail, sz - 1, fmt, ap);
        va_end(ap);
        info->detail[sz - 1] = '\0';
    }

    if (info->setjmp)
        longjmp(info->jmp, info->error_code);
}
