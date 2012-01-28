/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef FINDNEARMV_H
#define FINDNEARMV_H

#include "mv.h"
#include "blockd.h"
#include "entropymode.h"
#include "treereader_common.h"

#define LEFT_TOP_MARGIN (16 << 3)
#define RIGHT_BOTTOM_MARGIN (16 << 3)

static void vp8_clamp_mv2(int_mv *mv, const MACROBLOCKD *mb)
{
    if (mv->as_mv.col < (mb->mb_to_left_edge - LEFT_TOP_MARGIN))
        mv->as_mv.col = mb->mb_to_left_edge - LEFT_TOP_MARGIN;
    else if (mv->as_mv.col > mb->mb_to_right_edge + RIGHT_BOTTOM_MARGIN)
        mv->as_mv.col = mb->mb_to_right_edge + RIGHT_BOTTOM_MARGIN;

    if (mv->as_mv.row < (mb->mb_to_top_edge - LEFT_TOP_MARGIN))
        mv->as_mv.row = mb->mb_to_top_edge - LEFT_TOP_MARGIN;
    else if (mv->as_mv.row > mb->mb_to_bottom_edge + RIGHT_BOTTOM_MARGIN)
        mv->as_mv.row = mb->mb_to_bottom_edge + RIGHT_BOTTOM_MARGIN;
}

static void vp8_clamp_mv(int_mv *mv,
                         int mb_to_left_edge, int mb_to_right_edge,
                         int mb_to_top_edge, int mb_to_bottom_edge)
{
    mv->as_mv.col = (mv->as_mv.col < mb_to_left_edge) ?
        mb_to_left_edge : mv->as_mv.col;
    mv->as_mv.col = (mv->as_mv.col > mb_to_right_edge) ?
        mb_to_right_edge : mv->as_mv.col;
    mv->as_mv.row = (mv->as_mv.row < mb_to_top_edge) ?
        mb_to_top_edge : mv->as_mv.row;
    mv->as_mv.row = (mv->as_mv.row > mb_to_bottom_edge) ?
        mb_to_bottom_edge : mv->as_mv.row;
}

void vp8_find_near_mvs(MACROBLOCKD *mb, const MODE_INFO *here,
                       int_mv *nearest, int_mv *nearby, int_mv *best,
                       int near_mv_ref_cts[4],
                       int ref_frame, int *ref_frame_sign_bias);

vp8_prob *vp8_mv_ref_probs(vp8_prob p[VP8_MVREFS - 1],
                           const int near_mv_ref_ct[4]);

#endif /* FINDNEARMV_H */
