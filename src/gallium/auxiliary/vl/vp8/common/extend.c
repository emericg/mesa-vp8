/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "../vpx_mem.h"
#include "extend.h"

/* note the extension is only for the last row, for intra prediction purpose */
void vp8_extend_mb_row(YV12_BUFFER_CONFIG *ybf, unsigned char *YPtr, unsigned char *UPtr, unsigned char *VPtr)
{
    int i;

    YPtr += ybf->y_stride * 14;
    UPtr += ybf->uv_stride * 6;
    VPtr += ybf->uv_stride * 6;

    for (i = 0; i < 4; i++)
    {
        YPtr[i] = YPtr[-1];
        UPtr[i] = UPtr[-1];
        VPtr[i] = VPtr[-1];
    }

    YPtr += ybf->y_stride;
    UPtr += ybf->uv_stride;
    VPtr += ybf->uv_stride;

    for (i = 0; i < 4; i++)
    {
        YPtr[i] = YPtr[-1];
        UPtr[i] = UPtr[-1];
        VPtr[i] = VPtr[-1];
    }
}
