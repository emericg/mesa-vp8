/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef RECON_H
#define RECON_H

#include "blockd.h"
#include "reconinter.h"
#include "reconintra.h"
#include "reconintra4x4.h"
#include "recon_dispatch.h"

void vp8_recon_b_c(unsigned char *pred_ptr,
                   short *diff_ptr,
                   unsigned char *dst_ptr,
                   int stride);

void vp8_recon4b_c(unsigned char *pred_ptr,
                   short *diff_ptr,
                   unsigned char *dst_ptr,
                   int stride);

void vp8_recon2b_c(unsigned char *pred_ptr,
                   short *diff_ptr,
                   unsigned char *dst_ptr,
                   int stride);

void vp8_recon_mby_c(const vp8_recon_rtcd_vtable_t *rtcd, MACROBLOCKD *mb);

void vp8_recon_mb_c(const vp8_recon_rtcd_vtable_t *rtcd, MACROBLOCKD *mb);

#endif /* RECON_H */
