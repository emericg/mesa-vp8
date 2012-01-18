/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef FILTER_DISPATCH_H
#define FILTER_DISPATCH_H

#define prototype_filter_predict(sym) \
    void sym(unsigned char *src, int src_pitch, int xofst, int yofst, \
             unsigned char *dst, int dst_pitch)

#ifndef vp8_filter_sixtap16x16
#define vp8_filter_sixtap16x16 vp8_sixtap_predict16x16_c
#endif
extern prototype_filter_predict(vp8_filter_sixtap16x16);

#ifndef vp8_filter_sixtap8x8
#define vp8_filter_sixtap8x8 vp8_sixtap_predict8x8_c
#endif
extern prototype_filter_predict(vp8_filter_sixtap8x8);

#ifndef vp8_filter_sixtap8x4
#define vp8_filter_sixtap8x4 vp8_sixtap_predict8x4_c
#endif
extern prototype_filter_predict(vp8_filter_sixtap8x4);

#ifndef vp8_filter_sixtap4x4
#define vp8_filter_sixtap4x4 vp8_sixtap_predict4x4_c
#endif
extern prototype_filter_predict(vp8_filter_sixtap4x4);

#ifndef vp8_filter_bilinear16x16
#define vp8_filter_bilinear16x16 vp8_bilinear_predict16x16_c
#endif
extern prototype_filter_predict(vp8_filter_bilinear16x16);

#ifndef vp8_filter_bilinear8x8
#define vp8_filter_bilinear8x8 vp8_bilinear_predict8x8_c
#endif
extern prototype_filter_predict(vp8_filter_bilinear8x8);

#ifndef vp8_filter_bilinear8x4
#define vp8_filter_bilinear8x4 vp8_bilinear_predict8x4_c
#endif
extern prototype_filter_predict(vp8_filter_bilinear8x4);

#ifndef vp8_filter_bilinear4x4
#define vp8_filter_bilinear4x4 vp8_bilinear_predict4x4_c
#endif
extern prototype_filter_predict(vp8_filter_bilinear4x4);

typedef prototype_filter_predict((*vp8_filter_fn_t));

typedef struct
{
    vp8_filter_fn_t sixtap16x16;
    vp8_filter_fn_t sixtap8x8;
    vp8_filter_fn_t sixtap8x4;
    vp8_filter_fn_t sixtap4x4;
    vp8_filter_fn_t bilinear16x16;
    vp8_filter_fn_t bilinear8x8;
    vp8_filter_fn_t bilinear8x4;
    vp8_filter_fn_t bilinear4x4;
} vp8_filter_rtcd_vtable_t;

#define FILTER_INVOKE(ctx,fn) vp8_filter_##fn

#endif /* FILTER_DISPATCH_H */
