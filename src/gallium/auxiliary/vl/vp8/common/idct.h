/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef IDCT_H
#define IDCT_H

#define prototype_second_order(sym) \
    void sym(short *input, short *output)

#define prototype_idct(sym) \
    void sym(short *input, short *output, int pitch)

#define prototype_idct_scalar_add(sym) \
    void sym(short input, \
             unsigned char *pred, unsigned char *output, \
             int pitch, int stride)

#ifndef vp8_idct_idct1
#define vp8_idct_idct1 vp8_short_idct4x4llm_1_c
#endif
extern prototype_idct(vp8_idct_idct1);

#ifndef vp8_idct_idct16
#define vp8_idct_idct16 vp8_short_idct4x4llm_c
#endif
extern prototype_idct(vp8_idct_idct16);

#ifndef vp8_idct_idct1_scalar_add
#define vp8_idct_idct1_scalar_add vp8_dc_only_idct_add_c
#endif
extern prototype_idct_scalar_add(vp8_idct_idct1_scalar_add);


#ifndef vp8_idct_iwalsh1
#define vp8_idct_iwalsh1 vp8_short_inv_walsh4x4_1_c
#endif
extern prototype_second_order(vp8_idct_iwalsh1);

#ifndef vp8_idct_iwalsh16
#define vp8_idct_iwalsh16 vp8_short_inv_walsh4x4_c
#endif
extern prototype_second_order(vp8_idct_iwalsh16);

typedef prototype_idct((*vp8_idct_fn_t));
typedef prototype_idct_scalar_add((*vp8_idct_scalar_add_fn_t));
typedef prototype_second_order((*vp8_second_order_fn_t));

typedef struct
{
    vp8_idct_fn_t            idct1;
    vp8_idct_fn_t            idct16;
    vp8_idct_scalar_add_fn_t idct1_scalar_add;

    vp8_second_order_fn_t iwalsh1;
    vp8_second_order_fn_t iwalsh16;
} vp8_idct_rtcd_vtable_t;

#define IDCT_INVOKE(ctx,fn) vp8_idct_##fn

#endif /* IDCT_H */
