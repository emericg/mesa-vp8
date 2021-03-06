/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef RECONINTRA4x4_H
#define RECONINTRA4x4_H

#include "blockd.h"

extern void vp8_intra4x4_predict(BLOCKD *x, int b_mode, unsigned char *predictor);

extern void vp8_intra_prediction_down_copy(MACROBLOCKD *mb);

#endif /* RECONINTRA4x4_H */
