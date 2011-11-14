/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef VP8D_INT_H
#define VP8D_INT_H

#include "../common/onyxd.h"
#include "treereader.h"
#include "../common/onyxc_int.h"
#include "dequantize.h"

typedef struct
{
    MACROBLOCKD mbd;
    int mb_row;
    int current_mb_col;
    short *coef_ptr;
} MB_ROW_DEC;

typedef struct
{
    int16_t min_val;
    int16_t Length;
    uint8_t Probs[12];
} TOKENEXTRABITS;

typedef struct
{
    DECLARE_ALIGNED(16, MACROBLOCKD, mb);
    DECLARE_ALIGNED(16, VP8_COMMON, common);

    vp8_reader bc, bc2;

    const unsigned char *data;
    unsigned int         data_size;

    vp8_reader *mbc;
    int64_t     last_time_stamp;
    int         ready_for_new_data;

    vp8_prob prob_intra;
    vp8_prob prob_last;
    vp8_prob prob_gf;
    vp8_prob prob_skip_false;

} VP8D_COMP;

int vp8_frame_decode(VP8D_COMP *cpi, struct pipe_vp8_picture_desc *frame_header);

#endif /* VP8D_INT_H */
