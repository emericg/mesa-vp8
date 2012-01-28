/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef RECONINTER_H
#define RECONINTER_H

extern void vp8_build_inter_predictors_mb(MACROBLOCKD *mb);

extern void vp8_build_inter16x16_predictors_mb(MACROBLOCKD *mb,
                                               unsigned char *dst_y,
                                               unsigned char *dst_u,
                                               unsigned char *dst_v,
                                               int dst_ystride,
                                               int dst_uvstride);

extern void vp8_build_inter16x16_predictors_mby(MACROBLOCKD *mb);

extern void vp8_build_uvmvs(MACROBLOCKD *mb, int fullpixel);

extern void vp8_build_inter_predictors_b(BLOCKD *d, int pitch, vp8_filter_fn_t sppf);

extern void vp8_build_inter_predictors_mbuv(MACROBLOCKD *mb);

#endif /* RECONINTER_H */
