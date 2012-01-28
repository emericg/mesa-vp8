/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "treereader.h"
#include "entropymv.h"
#include "entropymode.h"
#include "findnearmv.h"

#if ENABLE_DEBUG
#include <assert.h>
#endif

static int left_block_mv(const MODE_INFO *cur_mb, int b)
{
    if (!(b & 3))
    {
        /* On L edge, get from MB to left of us */
        --cur_mb;

        if (cur_mb->mbmi.mode != SPLITMV)
            return cur_mb->mbmi.mv.as_int;

        b += 4;
    }

    return (cur_mb->bmi + b - 1)->mv.as_int;
}

static int above_block_mv(const MODE_INFO *cur_mb, int b, int mi_stride)
{
    if (!(b >> 2))
    {
        /* On top edge, get from MB above us */
        cur_mb -= mi_stride;

        if (cur_mb->mbmi.mode != SPLITMV)
            return cur_mb->mbmi.mv.as_int;

        b += 16;
    }

    return (cur_mb->bmi + b - 4)->mv.as_int;
}

static B_PREDICTION_MODE left_block_mode(const MODE_INFO *cur_mb, int b)
{
    if (!(b & 3))
    {
        /* On L edge, get from MB to left of us */
        --cur_mb;

        switch (cur_mb->mbmi.mode)
        {
            case B_PRED:
                return (cur_mb->bmi + b + 3)->as_mode;
            case DC_PRED:
                return B_DC_PRED;
            case V_PRED:
                return B_VE_PRED;
            case H_PRED:
                return B_HE_PRED;
            case TM_PRED:
                return B_TM_PRED;
            default:
                return B_DC_PRED;
        }
    }

    return (cur_mb->bmi + b - 1)->as_mode;
}

static B_PREDICTION_MODE above_block_mode(const MODE_INFO *cur_mb, int b, int mi_stride)
{
    if (!(b >> 2))
    {
        /* On top edge, get from MB above us */
        cur_mb -= mi_stride;

        switch (cur_mb->mbmi.mode)
        {
            case B_PRED:
                return (cur_mb->bmi + b + 12)->as_mode;
            case DC_PRED:
                return B_DC_PRED;
            case V_PRED:
                return B_VE_PRED;
            case H_PRED:
                return B_HE_PRED;
            case TM_PRED:
                return B_TM_PRED;
            default:
                return B_DC_PRED;
        }
    }

    return (cur_mb->bmi + b - 4)->as_mode;
}

static int vp8_read_bmode(BOOL_DECODER *bd, const vp8_prob *p)
{
    const int i = vp8_treed_read(bd, vp8_bmode_tree, p);

    return i;
}

static int vp8_read_ymode(BOOL_DECODER *bd, const vp8_prob *p)
{
    const int i = vp8_treed_read(bd, vp8_ymode_tree, p);

    return i;
}

static int vp8_kfread_ymode(BOOL_DECODER *bd, const vp8_prob *p)
{
    const int i = vp8_treed_read(bd, vp8_kf_ymode_tree, p);

    return i;
}

static int vp8_read_uv_mode(BOOL_DECODER *bd, const vp8_prob *p)
{
    const int i = vp8_treed_read(bd, vp8_uv_mode_tree, p);

    return i;
}

static void vp8_read_mb_features(BOOL_DECODER *bd, MB_MODE_INFO *mi, MACROBLOCKD *mb)
{
    /* Is segmentation enabled. */
    if (mb->segmentation_enabled && mb->update_mb_segmentation_map)
    {
        /* If so then read the segment id. */
        if (vp8_read(bd, mb->mb_segment_tree_probs[0]))
            mi->segment_id = (unsigned char)(2 + vp8_read(bd, mb->mb_segment_tree_probs[2]));
        else
            mi->segment_id = (unsigned char)(vp8_read(bd, mb->mb_segment_tree_probs[1]));
    }
}

static void vp8_kfread_modes(VP8_COMMON *common, MODE_INFO *mi, int mb_row, int mb_col)
{
    BOOL_DECODER *const bd = &common->bd;
    MB_PREDICTION_MODE y_mode;
    const int mis = common->mode_info_stride;

    /* Read the Macroblock segmentation map if it is being updated explicitly this frame (reset to 0 above by default)
     * By default on a key frame reset all MBs to segment 0 */
    mi->mbmi.segment_id = 0;

    if (common->mb.update_mb_segmentation_map)
        vp8_read_mb_features(bd, &mi->mbmi, &common->mb);

    /* Read the macroblock coeff skip flag if this feature is in use, else default to 0 */
    if (common->mb_no_coeff_skip)
        mi->mbmi.mb_skip_coeff = vp8_read(bd, common->prob_skip_false);
    else
        mi->mbmi.mb_skip_coeff = 0;

    y_mode = (MB_PREDICTION_MODE)vp8_kfread_ymode(bd, common->kf_ymode_prob);

    mi->mbmi.ref_frame = INTRA_FRAME;

    if ((mi->mbmi.mode = y_mode) == B_PRED)
    {
        int i = 0;

        do {
            const B_PREDICTION_MODE A = above_block_mode(mi, i, mis);
            const B_PREDICTION_MODE L = left_block_mode(mi, i);

            mi->bmi[i].as_mode = (B_PREDICTION_MODE)vp8_read_bmode(bd, common->kf_bmode_prob[A][L]);
        }
        while (++i < 16);
    }

    mi->mbmi.uv_mode = (MB_PREDICTION_MODE)vp8_read_uv_mode(bd, common->kf_uv_mode_prob);
}

static unsigned int vp8_check_mv_bounds(int_mv *mv, int mb_to_left_edge,
                                        int mb_to_right_edge, int mb_to_top_edge,
                                        int mb_to_bottom_edge)
{
    unsigned int need_to_clamp;
    need_to_clamp = (mv->as_mv.col < mb_to_left_edge) ? 1 : 0;
    need_to_clamp |= (mv->as_mv.col > mb_to_right_edge) ? 1 : 0;
    need_to_clamp |= (mv->as_mv.row < mb_to_top_edge) ? 1 : 0;
    need_to_clamp |= (mv->as_mv.row > mb_to_bottom_edge) ? 1 : 0;

    return need_to_clamp;
}

static int read_mvcomponent(BOOL_DECODER *bd, const MV_CONTEXT *mvc)
{
    const vp8_prob *const p = (const vp8_prob *)mvc;
    int x = 0;
    int i = 0;

    if (vp8_read(bd, p[mvpis_short])) /* Large */
    {
        do {
            x += vp8_read(bd, p[MVPbits + i]) << i;
        }
        while (++i < 3);

        i = mvlong_width - 1; /* Skip bit 3, which is sometimes implicit */

        do {
            x += vp8_read(bd, p[MVPbits + i]) << i;
        }
        while (--i > 3);

        if (!(x & 0xFFF0) || vp8_read(bd, p[MVPbits + 3]))
        {
            x += 8;
        }
    }
    else /* Small */
    {
        x = vp8_treed_read(bd, vp8_small_mvtree, p + MVPshort);
    }

    if (x && vp8_read(bd, p[MVPsign]))
    {
        x = -x;
    }

    return x;
}

static void read_mv(BOOL_DECODER *bd, MV *mv, const MV_CONTEXT *mvc)
{
    mv->row = (short)(read_mvcomponent(bd,   mvc) << 1);
    mv->col = (short)(read_mvcomponent(bd, ++mvc) << 1);
}

static void read_mvcontexts(BOOL_DECODER *bd, MV_CONTEXT *mvc)
{
    int i = 0;

    do {
        const vp8_prob *up = vp8_mv_update_probs[i].prob;
        vp8_prob *p = (vp8_prob *)(mvc + i);
        vp8_prob *const pstop = p + MVPcount;

        do {
            if (vp8_read(bd, *up++))
            {
                const vp8_prob x = (vp8_prob)vp8_read_literal(bd, 7);
                *p = x ? x << 1 : 1;
            }
        }
        while (++p < pstop);
    }
    while (++i < 2);
}

static MB_PREDICTION_MODE read_mv_ref(BOOL_DECODER *bd, const vp8_prob *p)
{
    const int i = vp8_treed_read(bd, vp8_mv_ref_tree, p);

    return (MB_PREDICTION_MODE)i;
}

static B_PREDICTION_MODE sub_mv_ref(BOOL_DECODER *bd, const vp8_prob *p)
{
    const int i = vp8_treed_read(bd, vp8_sub_mv_ref_tree, p);

    return (B_PREDICTION_MODE)i;
}

static const unsigned char mbsplit_fill_count[4] = {8, 8, 4, 1};

static const unsigned char mbsplit_fill_offset[4][16] =
{
    { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15},
    { 0,  1,  4,  5,  8,  9, 12, 13,  2,  3,  6,  7, 10, 11, 14, 15},
    { 0,  1,  4,  5,  2,  3,  6,  7,  8,  9, 12, 13, 10, 11, 14, 15},
    { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15}
};

static const unsigned char mbsplit_offset[4][16] =
{
    { 0,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
    { 0,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
    { 0,  2,  8, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
    { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15}
};

static void mb_mode_mv_init(VP8_COMMON *common)
{
    BOOL_DECODER *const bd = &common->bd;
    MV_CONTEXT *const mvc = common->fc.mvc;

    common->prob_skip_false = 0;
    if (common->mb_no_coeff_skip)
        common->prob_skip_false = (vp8_prob)vp8_read_literal(bd, 8);

    if (common->frame_type != KEY_FRAME)
    {
        common->prob_intra = (vp8_prob)vp8_read_literal(bd, 8);
        common->prob_last  = (vp8_prob)vp8_read_literal(bd, 8);
        common->prob_gf    = (vp8_prob)vp8_read_literal(bd, 8);

        if (vp8_read_bit(bd))
        {
            int i = 0;
            do {
                common->fc.ymode_prob[i] = (vp8_prob)vp8_read_literal(bd, 8);
            }
            while (++i < 4);
        }

        if (vp8_read_bit(bd))
        {
            int i = 0;
            do {
                common->fc.uv_mode_prob[i] = (vp8_prob)vp8_read_literal(bd, 8);
            }
            while (++i < 3);
        }

        read_mvcontexts(bd, mvc);
    }
}

static void read_mb_modes_mv(VP8_COMMON *common,
                             MODE_INFO *mi, MB_MODE_INFO *mbmi,
                             int mb_row, int mb_col)
{
    BOOL_DECODER *const bd = &common->bd;
    MV_CONTEXT *const mvc = common->fc.mvc;
    const int mis = common->mode_info_stride;

    int_mv *const mv = &mbmi->mv;
    int mb_to_left_edge;
    int mb_to_right_edge;
    int mb_to_top_edge;
    int mb_to_bottom_edge;

    mb_to_top_edge     = common->mb.mb_to_top_edge;
    mb_to_bottom_edge  = common->mb.mb_to_bottom_edge;
    mb_to_top_edge    -= LEFT_TOP_MARGIN;
    mb_to_bottom_edge += RIGHT_BOTTOM_MARGIN;

    mbmi->need_to_clamp_mvs = 0;

    /* Distance of Mb to the various image edges.
     * These specified to 8th pel as they are always compared to MV values that
     * are in 1/8th pel units */
    common->mb.mb_to_left_edge =
    mb_to_left_edge  = -((mb_col * 16) << 3);
    mb_to_left_edge -= LEFT_TOP_MARGIN;

    common->mb.mb_to_right_edge =
    mb_to_right_edge  = ((common->mb_cols - 1 - mb_col) * 16) << 3;
    mb_to_right_edge += RIGHT_BOTTOM_MARGIN;

    /* If required read in new segmentation data for this MB */
    if (common->mb.update_mb_segmentation_map)
        vp8_read_mb_features(bd, mbmi, &common->mb);

    /* Read the macroblock coeff skip flag if this feature is in use, else default to 0 */
    if (common->mb_no_coeff_skip)
        mbmi->mb_skip_coeff = vp8_read(bd, common->prob_skip_false);
    else
        mbmi->mb_skip_coeff = 0;

    if ((mbmi->ref_frame = (MV_REFERENCE_FRAME)vp8_read(bd, common->prob_intra))) /* Inter MB */
    {
        int rct[4];
        vp8_prob mv_ref_p[VP8_MVREFS - 1];
        int_mv nearest, nearby, best_mv;

        if (vp8_read(bd, common->prob_last))
        {
            mbmi->ref_frame = mbmi->ref_frame + (MV_REFERENCE_FRAME)vp8_read(bd, common->prob_gf) + 1;
        }

        vp8_find_near_mvs(&common->mb, mi, &nearest, &nearby, &best_mv, rct, mbmi->ref_frame, common->ref_frame_sign_bias);

        vp8_clamp_mv2(&nearest, &common->mb);
        vp8_clamp_mv2(&nearby, &common->mb);
        vp8_clamp_mv2(&best_mv, &common->mb);

        vp8_mv_ref_probs(mv_ref_p, rct);

        mbmi->uv_mode = DC_PRED;
        mbmi->mode = read_mv_ref(bd, mv_ref_p);
        switch (mbmi->mode)
        {
        case SPLITMV:
        {
            const int s = mbmi->partitioning = vp8_treed_read(bd, vp8_mbsplit_tree, vp8_mbsplit_probs);
            const int num_p = vp8_mbsplit_count[s];
            int j = 0;

            do /* For each subset j */
            {
                int_mv leftmv, abovemv;
                int_mv blockmv;
                int k = mbsplit_offset[s][j]; /* First block in subset j */
                int mv_contz;

                leftmv.as_int = left_block_mv(mi, k);
                abovemv.as_int = above_block_mv(mi, k, mis);
                mv_contz = vp8_mv_cont(&leftmv, &abovemv);

                switch (sub_mv_ref(bd, vp8_sub_mv_ref_prob2[mv_contz])) /*pc->fc.sub_mv_ref_prob))*/
                {
                case NEW4X4:
                    read_mv(bd, &blockmv.as_mv, (const MV_CONTEXT *)mvc);
                    blockmv.as_mv.row += best_mv.as_mv.row;
                    blockmv.as_mv.col += best_mv.as_mv.col;
                    break;
                case LEFT4X4:
                    blockmv.as_int = leftmv.as_int;
                    break;
                case ABOVE4X4:
                    blockmv.as_int = abovemv.as_int;
                    break;
                case ZERO4X4:
                    blockmv.as_int = 0;
                    break;
                default:
                    break;
                }

                mbmi->need_to_clamp_mvs = vp8_check_mv_bounds(&blockmv,
                                                              mb_to_left_edge,
                                                              mb_to_right_edge,
                                                              mb_to_top_edge,
                                                              mb_to_bottom_edge);

                {
                    /* Fill (uniform) modes, mvs of jth subset.
                     * Must do it here because ensuing subsets can
                     * refer back to us via "left" or "above". */
                    const unsigned char *fill_offset = &mbsplit_fill_offset[s][(unsigned char)j * mbsplit_fill_count[s]];
                    unsigned int fill_count = mbsplit_fill_count[s];

                    do {
                        mi->bmi[*fill_offset].mv.as_int = blockmv.as_int;
                        fill_offset++;
                    } while (--fill_count);
                }
            }
            while (++j < num_p);
        }
        mv->as_int = mi->bmi[15].mv.as_int;
        break; /* Done with SPLITMV */

        case NEARMV:
            mv->as_int = nearby.as_int;
            /* Clip "next_nearest" so that it does not extend to far out of image */
            vp8_clamp_mv(mv, mb_to_left_edge, mb_to_right_edge,
                         mb_to_top_edge, mb_to_bottom_edge);
            break;

        case NEARESTMV:
            mv->as_int = nearest.as_int;
            /* Clip "next_nearest" so that it does not extend to far out of image */
            vp8_clamp_mv(mv, mb_to_left_edge, mb_to_right_edge,
                         mb_to_top_edge, mb_to_bottom_edge);
            break;

        case ZEROMV:
            mv->as_int = 0;
            break;

        case NEWMV:
            read_mv(bd, &mv->as_mv, (const MV_CONTEXT *)mvc);
            mv->as_mv.row += best_mv.as_mv.row;
            mv->as_mv.col += best_mv.as_mv.col;

            /* Don't need to check this on NEARMV and NEARESTMV modes
             * since those modes clamp the MV. The NEWMV mode does not,
             * so signal to the prediction stage whether special
             * handling may be required. */
            mbmi->need_to_clamp_mvs = vp8_check_mv_bounds(mv,
                                                          mb_to_left_edge,
                                                          mb_to_right_edge,
                                                          mb_to_top_edge,
                                                          mb_to_bottom_edge);
            break;

#if ENABLE_DEBUG
        default:
            assert(0);
#else
        default:
            break;
#endif
        }
    }
    else
    {
        /* Required for left and above block mv */
        mbmi->mv.as_int = 0;
        mbmi->mode = (MB_PREDICTION_MODE)vp8_read_ymode(bd, common->fc.ymode_prob);

        /* MB is intra coded */
        if (mbmi->mode == B_PRED)
        {
            int j = 0;
            do {
                mi->bmi[j].as_mode = (B_PREDICTION_MODE)vp8_read_bmode(bd, common->fc.bmode_prob);
            }
            while (++j < 16);
        }

        mbmi->uv_mode = (MB_PREDICTION_MODE)vp8_read_uv_mode(bd, common->fc.uv_mode_prob);
    }
}

/**
 *
 */
void vp8_decode_mode_mvs(VP8_COMMON *common)
{
    MODE_INFO *mi = common->mi;
    int mb_row = -1;

    mb_mode_mv_init(common);

    while (++mb_row < common->mb_rows)
    {
        int mb_col = -1;
        int mb_to_top_edge;
        int mb_to_bottom_edge;

        common->mb.mb_to_top_edge =
        mb_to_top_edge  = -((mb_row * 16)) << 3;
        mb_to_top_edge -= LEFT_TOP_MARGIN;

        common->mb.mb_to_bottom_edge =
        mb_to_bottom_edge  = ((common->mb_rows - 1 - mb_row) * 16) << 3;
        mb_to_bottom_edge += RIGHT_BOTTOM_MARGIN;

        while (++mb_col < common->mb_cols)
        {
            /*read_mb_modes_mv(common, mb->mode_info_context, &mb->mode_info_context->mbmi, mb_row, mb_col);*/
            if (common->frame_type == KEY_FRAME)
                vp8_kfread_modes(common, mi, mb_row, mb_col);
            else
                read_mb_modes_mv(common, mi, &mi->mbmi, mb_row, mb_col);

            mi++; /* Next macroblock */
        }

        mi++; /* Skip left predictor each row */
    }
}
