/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef RECONINTRA_H
#define RECONINTRA_H

#include "yv12utils.h"

extern void vp8_setup_intra_recon(YV12_BUFFER_CONFIG *ybf);

void vp8_recon_intra_mbuv(const vp8_recon_rtcd_vtable_t *rtcd,
                          MACROBLOCKD *x);

#endif /* RECONINTRA_H */
