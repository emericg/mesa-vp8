/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef QUANT_COMMON_H
#define QUANT_COMMON_H

#include "blockd.h"
#include "onyxc_int.h"

extern int vp8_ac_yquant(int QIndex);
extern int vp8_dc_quant(int QIndex, int Delta);
extern int vp8_dc2quant(int QIndex, int Delta);
extern int vp8_ac2quant(int QIndex, int Delta);
extern int vp8_dc_uv_quant(int QIndex, int Delta);
extern int vp8_ac_uv_quant(int QIndex, int Delta);

extern void vp8_initialize_dequantizer(VP8_COMMON *pc);

#endif /* QUANT_COMMON_H */
