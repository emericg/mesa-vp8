/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "onyxd_int.h"
#include "../common/reconintra.h"
#include "../common/reconintra4x4.h"
#include "../common/recon.h"
#include "../common/reconinter.h"
#include "dequantize.h"
#include "detokenize.h"
#include "../common/invtrans.h"
#include "../common/alloccommon.h"
#include "../common/entropymode.h"
#include "../common/quant_common.h"
#include "../common/yv12utils.h"
#include "../common/setupintrarecon.h"

#include "decodemv.h"
#include "../vp8_mem.h"
#include "../common/idct.h"
#include "dboolhuff.h"

#include <assert.h>
#include <stdio.h>

void mb_init_dequantizer(VP8D_COMP *pbi, MACROBLOCKD *xd)
{
    int i;
    int QIndex;
    MB_MODE_INFO *mbmi = &xd->mode_info_context->mbmi;
    VP8_COMMON *const pc = &pbi->common;

    /* Decide whether to use the default or alternate baseline Q value. */
    if (xd->segmentation_enabled)
    {
        if (xd->mb_segement_abs_delta == SEGMENT_ABSDATA)
        {
            /* Abs Value */
            QIndex = xd->segment_feature_data[MB_LVL_ALT_Q][mbmi->segment_id];
        }
        else
        {
            /* Delta Value */
            QIndex = pc->base_qindex + xd->segment_feature_data[MB_LVL_ALT_Q][mbmi->segment_id];
            QIndex = (QIndex >= 0) ? ((QIndex <= MAXQ) ? QIndex : MAXQ) : 0; /* Clamp to valid range */
        }
    }
    else
    {
        QIndex = pc->base_qindex;
    }

    /* Set up the block level dequant pointers */
    for (i = 0; i < 16; i++)
    {
        xd->block[i].dequant = pc->Y1dequant[QIndex];
    }

    for (i = 16; i < 24; i++)
    {
        xd->block[i].dequant = pc->UVdequant[QIndex];
    }

    xd->block[24].dequant = pc->Y2dequant[QIndex];
}

#define RTCD_VTABLE(x) NULL

/* skip_recon_mb() is Modified: Instead of writing the result to predictor buffer and then copying it
 * to dst buffer, we can write the result directly to dst buffer. This eliminates unnecessary copy. */
static void skip_recon_mb(VP8D_COMP *pbi, MACROBLOCKD *xd)
{
    if (xd->mode_info_context->mbmi.ref_frame == INTRA_FRAME)
    {
        RECON_INVOKE(&pbi->common.rtcd.recon, build_intra_predictors_mbuv_s)(xd);
        RECON_INVOKE(&pbi->common.rtcd.recon, build_intra_predictors_mby_s)(xd);
    }
    else
    {
        vp8_build_inter16x16_predictors_mb(xd, xd->dst.y_buffer,
                                           xd->dst.u_buffer, xd->dst.v_buffer,
                                           xd->dst.y_stride, xd->dst.uv_stride);
    }
}

static void clamp_mv_to_umv_border(MV *mv, const MACROBLOCKD *xd)
{
    /* If the MV points so far into the UMV border that no visible pixels
     * are used for reconstruction, the subpel part of the MV can be
     * discarded and the MV limited to 16 pixels with equivalent results.
     *
     * This limit kicks in at 19 pixels for the top and left edges, for
     * the 16 pixels plus 3 taps right of the central pixel when subpel
     * filtering. The bottom and right edges use 16 pixels plus 2 pixels
     * left of the central pixel when filtering.
     */
    if (mv->col < (xd->mb_to_left_edge - (19 << 3)))
        mv->col = xd->mb_to_left_edge - (16 << 3);
    else if (mv->col > xd->mb_to_right_edge + (18 << 3))
        mv->col = xd->mb_to_right_edge + (16 << 3);

    if (mv->row < (xd->mb_to_top_edge - (19 << 3)))
        mv->row = xd->mb_to_top_edge - (16 << 3);
    else if (mv->row > xd->mb_to_bottom_edge + (18 << 3))
        mv->row = xd->mb_to_bottom_edge + (16 << 3);
}

/* A version of the above function for chroma block MVs.*/
static void clamp_uvmv_to_umv_border(MV *mv, const MACROBLOCKD *xd)
{
    mv->col = (2*mv->col < (xd->mb_to_left_edge - (19 << 3))) ? (xd->mb_to_left_edge - (16 << 3)) >> 1 : mv->col;
    mv->col = (2*mv->col > xd->mb_to_right_edge + (18 << 3)) ? (xd->mb_to_right_edge + (16 << 3)) >> 1 : mv->col;

    mv->row = (2*mv->row < (xd->mb_to_top_edge - (19 << 3))) ? (xd->mb_to_top_edge - (16 << 3)) >> 1 : mv->row;
    mv->row = (2*mv->row > xd->mb_to_bottom_edge + (18 << 3)) ? (xd->mb_to_bottom_edge + (16 << 3)) >> 1 : mv->row;
}

void clamp_mvs(MACROBLOCKD *xd)
{
    if (xd->mode_info_context->mbmi.mode == SPLITMV)
    {
        int i;

        for (i = 0; i < 16; i++)
            clamp_mv_to_umv_border(&xd->block[i].bmi.mv.as_mv, xd);
        for (i = 16; i < 24; i++)
            clamp_uvmv_to_umv_border(&xd->block[i].bmi.mv.as_mv, xd);
    }
    else
    {
        clamp_mv_to_umv_border(&xd->mode_info_context->mbmi.mv.as_mv, xd);
        clamp_uvmv_to_umv_border(&xd->block[16].bmi.mv.as_mv, xd);
    }
}

static int get_delta_q(BOOL_DECODER *bd, int prev, int *q_update)
{
    int ret_val = 0;

    if (vp8_read_bit(bd))
    {
        ret_val = vp8_read_literal(bd, 4);

        if (vp8_read_bit(bd))
            ret_val = -ret_val;
    }

    /* Trigger a quantizer update if the delta-q value has changed */
    if (ret_val != prev)
        *q_update = 1;

    return ret_val;
}

/* note the extension is only for the last row, for intra prediction purpose */
static void vp8_extend_mb_row(YV12_BUFFER_CONFIG *ybf,
                              unsigned char *YPtr,
                              unsigned char *UPtr,
                              unsigned char *VPtr)
{
    int i;

    YPtr += ybf->y_stride * 14;
    UPtr += ybf->uv_stride * 6;
    VPtr += ybf->uv_stride * 6;

    for (i = 0; i < 4; i++)
    {
        YPtr[i] = YPtr[-1];
        UPtr[i] = UPtr[-1];
        VPtr[i] = VPtr[-1];
    }

    YPtr += ybf->y_stride;
    UPtr += ybf->uv_stride;
    VPtr += ybf->uv_stride;

    for (i = 0; i < 4; i++)
    {
        YPtr[i] = YPtr[-1];
        UPtr[i] = UPtr[-1];
        VPtr[i] = VPtr[-1];
    }
}

static void decode_macroblock(VP8D_COMP *pbi, MACROBLOCKD *xd, unsigned int mb_idx)
{
    int eobtotal = 0;
    MB_PREDICTION_MODE mode;
    int i;

    if (xd->mode_info_context->mbmi.mb_skip_coeff)
    {
        vp8_reset_mb_tokens_context(xd);
    }
    else
    {
        eobtotal = vp8_decode_mb_tokens(pbi, xd);
    }

    /* Perform temporary clamping of the MV to be used for prediction */
    if (xd->mode_info_context->mbmi.need_to_clamp_mvs)
    {
        clamp_mvs(xd);
    }

    mode = xd->mode_info_context->mbmi.mode;

    if (eobtotal == 0 && mode != B_PRED && mode != SPLITMV)
    {
        /* Special case: Force the loopfilter to skip when eobtotal and
         * mb_skip_coeff are zero. */
        xd->mode_info_context->mbmi.mb_skip_coeff = 1;

        skip_recon_mb(pbi, xd);
        return;
    }

    if (xd->segmentation_enabled)
        mb_init_dequantizer(pbi, xd);

    /* do prediction */
    if (xd->mode_info_context->mbmi.ref_frame == INTRA_FRAME)
    {
        RECON_INVOKE(&pbi->common.rtcd.recon, build_intra_predictors_mbuv)(xd);

        if (mode != B_PRED)
        {
            RECON_INVOKE(&pbi->common.rtcd.recon, build_intra_predictors_mby)(xd);
        }
        else
        {
            vp8_intra_prediction_down_copy(xd);
        }
    }
    else
    {
        vp8_build_inter_predictors_mb(xd);
    }

    /* dequantization and idct */
    if (mode == B_PRED)
    {
        for (i = 0; i < 16; i++)
        {
            BLOCKD *b = &xd->block[i];
            RECON_INVOKE(RTCD_VTABLE(recon), intra4x4_predict)
                          (b, b->bmi.as_mode, b->predictor);

            if (xd->eobs[i] > 1)
            {
                DEQUANT_INVOKE(&pbi->dequant, idct_add)
                    (b->qcoeff, b->dequant, b->predictor,
                    *(b->base_dst) + b->dst, 16, b->dst_stride);
            }
            else
            {
                IDCT_INVOKE(RTCD_VTABLE(idct), idct1_scalar_add)
                    (b->qcoeff[0] * b->dequant[0], b->predictor,
                    *(b->base_dst) + b->dst, 16, b->dst_stride);
                ((int *)b->qcoeff)[0] = 0;
            }
        }

    }
    else if (mode == SPLITMV)
    {
        DEQUANT_INVOKE (&pbi->dequant, idct_add_y_block)
                        (xd->qcoeff, xd->block[0].dequant,
                         xd->predictor, xd->dst.y_buffer,
                         xd->dst.y_stride, xd->eobs);
    }
    else
    {
        BLOCKD *b = &xd->block[24];

        DEQUANT_INVOKE(&pbi->dequant, block)(b);

        /* do 2nd order transform on the dc block */
        if (xd->eobs[24] > 1)
        {
            IDCT_INVOKE(RTCD_VTABLE(idct), iwalsh16)(&b->dqcoeff[0], b->diff);
            ((int *)b->qcoeff)[0] = 0;
            ((int *)b->qcoeff)[1] = 0;
            ((int *)b->qcoeff)[2] = 0;
            ((int *)b->qcoeff)[3] = 0;
            ((int *)b->qcoeff)[4] = 0;
            ((int *)b->qcoeff)[5] = 0;
            ((int *)b->qcoeff)[6] = 0;
            ((int *)b->qcoeff)[7] = 0;
        }
        else
        {
            IDCT_INVOKE(RTCD_VTABLE(idct), iwalsh1)(&b->dqcoeff[0], b->diff);
            ((int *)b->qcoeff)[0] = 0;
        }

        DEQUANT_INVOKE (&pbi->dequant, dc_idct_add_y_block)
                        (xd->qcoeff, xd->block[0].dequant,
                         xd->predictor, xd->dst.y_buffer,
                         xd->dst.y_stride, xd->eobs, xd->block[24].diff);
    }

    DEQUANT_INVOKE (&pbi->dequant, idct_add_uv_block)
                    (xd->qcoeff+16*16, xd->block[16].dequant,
                     xd->predictor+16*16, xd->dst.u_buffer, xd->dst.v_buffer,
                     xd->dst.uv_stride, xd->eobs+16);
}

static void
decode_macroblock_row(VP8D_COMP *pbi, VP8_COMMON *pc, int mb_row, MACROBLOCKD *xd)
{
    int recon_yoffset, recon_uvoffset;
    int mb_col;
    int ref_fb_idx = pc->lst_fb_idx;
    int dst_fb_idx = pc->new_fb_idx;
    int recon_y_stride = pc->yv12_fb[ref_fb_idx].y_stride;
    int recon_uv_stride = pc->yv12_fb[ref_fb_idx].uv_stride;

    memset(&pc->left_context, 0, sizeof(pc->left_context));
    recon_yoffset = mb_row * recon_y_stride * 16;
    recon_uvoffset = mb_row * recon_uv_stride * 8;

    /* Reset above block coeffs */
    xd->above_context = pc->above_context;
    xd->up_available = (mb_row != 0);

    xd->mb_to_top_edge = -((mb_row * 16)) << 3;
    xd->mb_to_bottom_edge = ((pc->mb_rows - 1 - mb_row) * 16) << 3;

    for (mb_col = 0; mb_col < pc->mb_cols; mb_col++)
    {
        /* Distance of Mb to the various image edges.
         * These are specified to 8th pel as they are always compared to values
         * that are in 1/8th pel units. */
        xd->mb_to_left_edge = -((mb_col * 16) << 3);
        xd->mb_to_right_edge = ((pc->mb_cols - 1 - mb_col) * 16) << 3;

        update_blockd_bmi(xd);

        xd->dst.y_buffer = pc->yv12_fb[dst_fb_idx].y_buffer + recon_yoffset;
        xd->dst.u_buffer = pc->yv12_fb[dst_fb_idx].u_buffer + recon_uvoffset;
        xd->dst.v_buffer = pc->yv12_fb[dst_fb_idx].v_buffer + recon_uvoffset;

        xd->left_available = (mb_col != 0);

        /* Select the appropriate reference frame for this MB */
        if (xd->mode_info_context->mbmi.ref_frame == LAST_FRAME)
            ref_fb_idx = pc->lst_fb_idx;
        else if (xd->mode_info_context->mbmi.ref_frame == GOLDEN_FRAME)
            ref_fb_idx = pc->gld_fb_idx;
        else
            ref_fb_idx = pc->alt_fb_idx;

        xd->pre.y_buffer = pc->yv12_fb[ref_fb_idx].y_buffer + recon_yoffset;
        xd->pre.u_buffer = pc->yv12_fb[ref_fb_idx].u_buffer + recon_uvoffset;
        xd->pre.v_buffer = pc->yv12_fb[ref_fb_idx].v_buffer + recon_uvoffset;

        if (xd->mode_info_context->mbmi.ref_frame != INTRA_FRAME)
        {
            /* Propagate errors from reference frames */
            xd->corrupted |= pc->yv12_fb[ref_fb_idx].corrupted;
        }

        vp8_build_uvmvs(xd, pc->full_pixel);

        decode_macroblock(pbi, xd, mb_row * pc->mb_cols + mb_col);

        /* Check if the boolean decoder has suffered an error */
        xd->corrupted |= vp8dx_bool_error(xd->current_bd);

        recon_yoffset += 16;
        recon_uvoffset += 8;

        ++xd->mode_info_context; /* next mb */

        xd->above_context++;
    }

    /* Adjust to the next row of mbs */
    vp8_extend_mb_row(&pc->yv12_fb[dst_fb_idx],
                      xd->dst.y_buffer + 16,
                      xd->dst.u_buffer + 8,
                      xd->dst.v_buffer + 8);

    /* Skip prediction column */
    ++xd->mode_info_context;
}

static unsigned int token_decoder_readpartitionsize(const unsigned char *cx_size)
{
    const unsigned int size = cx_size[0] + (cx_size[1] << 8) + (cx_size[2] << 16);
    return size;
}

static void token_decoder_setup(VP8D_COMP *pbi,
                                const unsigned char *cx_data)
{
    int num_part;
    int i;
    VP8_COMMON          *pc = &pbi->common;
    const unsigned char *user_data_end = pbi->data + pbi->data_size;

    /* Set up pointers to the first partition */
    BOOL_DECODER        *bool_decoder = &pbi->bd2;
    const unsigned char *partition = cx_data;

    /* Parse number of token partitions to use */
    const TOKEN_PARTITION multi_token_partition = (TOKEN_PARTITION)vp8_read_literal(&pbi->bd, 2);

    /* Only update the multi_token_partition field if we are sure the value is correct. */
    if (!vp8dx_bool_error(&pbi->bd))
        pc->multi_token_partition = multi_token_partition;

    num_part = 1 << pc->multi_token_partition;

    if (num_part > 1)
    {
        CHECK_MEM_ERROR(pbi->mbd, vpx_malloc(num_part * sizeof(BOOL_DECODER)));
        bool_decoder = pbi->mbd;
        partition += 3 * (num_part - 1);
    }

    for (i = 0; i < num_part; i++)
    {
        const unsigned char *partition_size_ptr = cx_data + i * 3;
        ptrdiff_t            partition_size;

        /* Calculate the length of this partition. The last partition
         * size is implicit. */
        if (i < num_part - 1)
        {
            partition_size = token_decoder_readpartitionsize(partition_size_ptr);
        }
        else
        {
            partition_size = user_data_end - partition;
        }

        if (partition + partition_size > user_data_end
            || partition + partition_size < partition)
        {
            vpx_internal_error(&pc->error, VPX_CODEC_CORRUPT_FRAME,
                               "Truncated packet or corrupt partition "
                               "%d length", i + 1);
        }

        if (vp8dx_start_decode(bool_decoder, partition, partition_size))
        {
            vpx_internal_error(&pc->error, VPX_CODEC_MEM_ERROR,
                               "Failed to allocate bool decoder %d", i + 1);
        }

        /* Advance to the next partition */
        partition += partition_size;
        bool_decoder++;
    }
}

static void token_decoder_stop(VP8D_COMP *pbi)
{
    VP8_COMMON *pc = &pbi->common;

    if (pc->multi_token_partition != ONE_PARTITION)
    {
        vpx_free(pbi->mbd);
        pbi->mbd = NULL;
    }
}

static void vp8_frame_init(VP8D_COMP *pbi)
{
    VP8_COMMON *const pc = &pbi->common;
    MACROBLOCKD *const xd = &pbi->mb;

    if (pc->frame_type == KEY_FRAME)
    {
        /* Various keyframe initializations. */
        memcpy(pc->fc.mvc, vp8_default_mv_context, sizeof(vp8_default_mv_context));

        vp8_init_mbmode_probs(pc);

        vp8_default_coef_probs(pc);
        vp8_kf_default_bmode_probs(pc->kf_bmode_prob);

        /* Reset the segment feature data to 0 with delta coding (Default state). */
        memset(xd->segment_feature_data, 0, sizeof(xd->segment_feature_data));
        xd->mb_segement_abs_delta = SEGMENT_DELTADATA;

        /* Reset the mode ref deltasa for loop filter. */
        memset(xd->ref_lf_deltas, 0, sizeof(xd->ref_lf_deltas));
        memset(xd->mode_lf_deltas, 0, sizeof(xd->mode_lf_deltas));

        /* All buffers are implicitly updated on key frames. */
        pc->refresh_golden_frame = 1;
        pc->refresh_alt_ref_frame = 1;
        pc->copy_buffer_to_gf = 0;
        pc->copy_buffer_to_arf = 0;

        /* Note that Golden and Altref modes cannot be used on a key frame so
         * ref_frame_sign_bias[] is undefined and meaningless. */
        pc->ref_frame_sign_bias[GOLDEN_FRAME] = 0;
        pc->ref_frame_sign_bias[ALTREF_FRAME] = 0;
    }
    else
    {
        if (pc->use_bilinear_mc_filter)
        {
            pc->mcomp_filter_type     = BILINEAR;
            xd->subpixel_predict      = SUBPIX_INVOKE(RTCD_VTABLE(subpix), bilinear4x4);
            xd->subpixel_predict8x4   = SUBPIX_INVOKE(RTCD_VTABLE(subpix), bilinear8x4);
            xd->subpixel_predict8x8   = SUBPIX_INVOKE(RTCD_VTABLE(subpix), bilinear8x8);
            xd->subpixel_predict16x16 = SUBPIX_INVOKE(RTCD_VTABLE(subpix), bilinear16x16);
        }
        else
        {
            pc->mcomp_filter_type     = SIXTAP;
            xd->subpixel_predict      = SUBPIX_INVOKE(RTCD_VTABLE(subpix), sixtap4x4);
            xd->subpixel_predict8x4   = SUBPIX_INVOKE(RTCD_VTABLE(subpix), sixtap8x4);
            xd->subpixel_predict8x8   = SUBPIX_INVOKE(RTCD_VTABLE(subpix), sixtap8x8);
            xd->subpixel_predict16x16 = SUBPIX_INVOKE(RTCD_VTABLE(subpix), sixtap16x16);
        }
    }

    xd->left_context = &pc->left_context;
    xd->mode_info_context = pc->mi;
    xd->frame_type = pc->frame_type;
    xd->mode_info_context->mbmi.mode = DC_PRED;
    xd->mode_info_stride = pc->mode_info_stride;
    xd->corrupted = 0; /* init without corruption */
}

int vp8_frame_decode(VP8D_COMP *pbi, struct pipe_vp8_picture_desc *frame_header)
{
    BOOL_DECODER *const bd = &pbi->bd;
    VP8_COMMON *const pc = &pbi->common;
    MACROBLOCKD *const xd = &pbi->mb;
    const unsigned char *data = pbi->data;
    const unsigned char *data_end = data + pbi->data_size;
    ptrdiff_t first_partition_length_in_bytes = (ptrdiff_t)frame_header->first_partition_size;

    int mb_row;
    int i, j, k, l;

    /* Start with no corruption of current frame */
    xd->corrupted = 0;
    pc->yv12_fb[pc->new_fb_idx].corrupted = 0;

    pc->frame_type = (FRAME_TYPE)frame_header->key_frame;
    pc->version = (int)frame_header->base.profile;
    pc->show_frame = (int)frame_header->show_frame;

    vp8_setup_version(pc);

    if (pc->frame_type == KEY_FRAME)
    {
        if (pc->Width != frame_header->width ||
            pc->Height != frame_header->height)
        {
            if (vp8_alloc_frame_buffers(pc, frame_header->width, frame_header->height))
            {
                vpx_internal_error(&pc->error, VPX_CODEC_MEM_ERROR,
                                   "Failed to allocate frame buffers");
            }
        }

        pc->Width = frame_header->width;
        pc->horiz_scale = frame_header->horizontal_scale;
        pc->Height = frame_header->height;
        pc->vert_scale = frame_header->vertical_scale;
    }

    vp8_frame_init(pbi);

    if (vp8dx_start_decode(bd, data, data_end - data))
    {
        vpx_internal_error(&pc->error, VPX_CODEC_MEM_ERROR,
                           "Failed to allocate bool decoder 0");
    }

    if (pc->frame_type == KEY_FRAME)
    {
        pc->clr_type   = (YUV_TYPE)vp8_read_bit(bd);
        pc->clamp_type = (CLAMP_TYPE)vp8_read_bit(bd);
    }

    /* Is segmentation enabled */
    xd->segmentation_enabled = (unsigned char)vp8_read_bit(bd);

    if (xd->segmentation_enabled)
    {
        /* Signal whether or not the segmentation map is being explicitly updated this frame. */
        xd->update_mb_segmentation_map = (unsigned char)vp8_read_bit(bd);
        xd->update_mb_segmentation_data = (unsigned char)vp8_read_bit(bd);

        if (xd->update_mb_segmentation_data)
        {
            xd->mb_segement_abs_delta = (unsigned char)vp8_read_bit(bd);

            memset(xd->segment_feature_data, 0, sizeof(xd->segment_feature_data));

            /* For each segmentation feature (Quant and loop filter level) */
            for (i = 0; i < MB_LVL_MAX; i++)
            {
                for (j = 0; j < MAX_MB_SEGMENTS; j++)
                {
                    /* Frame level data */
                    if (vp8_read_bit(bd))
                    {
                        xd->segment_feature_data[i][j] = (signed char)vp8_read_literal(bd, vp8_mb_feature_data_bits[i]);

                        if (vp8_read_bit(bd))
                            xd->segment_feature_data[i][j] = -xd->segment_feature_data[i][j];
                    }
                    else
                        xd->segment_feature_data[i][j] = 0;
                }
            }
        }

        if (xd->update_mb_segmentation_map)
        {
            /* Which macro block level features are enabled */
            memset(xd->mb_segment_tree_probs, 255, sizeof(xd->mb_segment_tree_probs));

            /* Read the probs used to decode the segment id for each macro block. */
            for (i = 0; i < MB_FEATURE_TREE_PROBS; i++)
            {
                /* If not explicitly set value is defaulted to 255 by memset above. */
                if (vp8_read_bit(bd))
                    xd->mb_segment_tree_probs[i] = (vp8_prob)vp8_read_literal(bd, 8);
            }
        }
    }

    /* Read the loop filter level and type */
    pc->filter_type = (LOOPFILTER_TYPE)vp8_read_bit(bd);
    pc->filter_level = vp8_read_literal(bd, 6);
    pc->sharpness_level = vp8_read_literal(bd, 3);

    /* Read in loop filter deltas applied at the MB level based on mode or ref frame. */
    xd->mode_ref_lf_delta_update = 0;
    xd->mode_ref_lf_delta_enabled = (unsigned char)vp8_read_bit(bd);

    if (xd->mode_ref_lf_delta_enabled)
    {
        /* Do the deltas need to be updated */
        xd->mode_ref_lf_delta_update = (unsigned char)vp8_read_bit(bd);

        if (xd->mode_ref_lf_delta_update)
        {
            /* Send update */
            for (i = 0; i < MAX_REF_LF_DELTAS; i++)
            {
                if (vp8_read_bit(bd))
                {
                    xd->ref_lf_deltas[i] = (signed char)vp8_read_literal(bd, 6);

                    /* Apply sign */
                    if (vp8_read_bit(bd))
                        xd->ref_lf_deltas[i] = xd->ref_lf_deltas[i] * -1;
                }
            }

            /* Send update */
            for (i = 0; i < MAX_MODE_LF_DELTAS; i++)
            {
                if (vp8_read_bit(bd))
                {
                    xd->mode_lf_deltas[i] = (signed char)vp8_read_literal(bd, 6);

                    /* Apply sign */
                    if (vp8_read_bit(bd))
                        xd->mode_lf_deltas[i] = xd->mode_lf_deltas[i] * -1;
                }
            }
        }
    }

    token_decoder_setup(pbi, data + first_partition_length_in_bytes);
    xd->current_bd = &pbi->bd2;

    /* Read the default quantizers. */
    {
        int Q, q_update;

        Q = vp8_read_literal(bd, 7); /* AC 1st order Q = default */
        pc->base_qindex = Q;
        q_update = 0;
        pc->y1dc_delta_q = get_delta_q(bd, pc->y1dc_delta_q, &q_update);
        pc->y2dc_delta_q = get_delta_q(bd, pc->y2dc_delta_q, &q_update);
        pc->y2ac_delta_q = get_delta_q(bd, pc->y2ac_delta_q, &q_update);
        pc->uvdc_delta_q = get_delta_q(bd, pc->uvdc_delta_q, &q_update);
        pc->uvac_delta_q = get_delta_q(bd, pc->uvac_delta_q, &q_update);

        if (q_update)
            vp8_initialize_dequantizer(&pbi->common);

        /* MB level dequantizer setup */
        mb_init_dequantizer(pbi, &pbi->mb);
    }

    /* Determine if the golden frame or ARF buffer should be updated and how.
     * For all non key frames the GF and ARF refresh flags and sign bias
     * flags must be set explicitly. */
    if (pc->frame_type != KEY_FRAME)
    {
        /* Should the GF or ARF be updated from the current frame. */
        pc->refresh_golden_frame = vp8_read_bit(bd);

        pc->refresh_alt_ref_frame = vp8_read_bit(bd);

        /* Buffer to buffer copy flags. */
        pc->copy_buffer_to_gf = 0;

        if (!pc->refresh_golden_frame)
            pc->copy_buffer_to_gf = vp8_read_literal(bd, 2);

        pc->copy_buffer_to_arf = 0;

        if (!pc->refresh_alt_ref_frame)
            pc->copy_buffer_to_arf = vp8_read_literal(bd, 2);

        pc->ref_frame_sign_bias[GOLDEN_FRAME] = vp8_read_bit(bd);
        pc->ref_frame_sign_bias[ALTREF_FRAME] = vp8_read_bit(bd);
    }

    pc->refresh_entropy_probs = vp8_read_bit(bd);
    if (pc->refresh_entropy_probs == 0)
    {
        memcpy(&pc->lfc, &pc->fc, sizeof(pc->fc));
    }

    pc->refresh_last_frame = (pc->frame_type == KEY_FRAME || vp8_read_bit(bd));

    /* Read coef probability tree */
    for (i = 0; i < BLOCK_TYPES; i++)
        for (j = 0; j < COEF_BANDS; j++)
            for (k = 0; k < PREV_COEF_CONTEXTS; k++)
                for (l = 0; l < ENTROPY_NODES; l++)
                {
                    vp8_prob *const p = pc->fc.coef_probs[i][j][k] + l;

                    if (vp8_read(bd, vp8_coef_update_probs[i][j][k][l]))
                    {
                        *p = (vp8_prob)vp8_read_literal(bd, 8);
                    }
                }

    memcpy(&xd->pre, &pc->yv12_fb[pc->lst_fb_idx], sizeof(YV12_BUFFER_CONFIG));
    memcpy(&xd->dst, &pc->yv12_fb[pc->new_fb_idx], sizeof(YV12_BUFFER_CONFIG));

    /* set up frame new frame for intra coded blocks */
    vp8_setup_intra_recon(&pc->yv12_fb[pc->new_fb_idx]);

    vp8_setup_block_dptrs(xd);

    vp8_build_block_doffsets(xd);

    /* clear out the coeff buffer */
    memset(xd->qcoeff, 0, sizeof(xd->qcoeff));

    /* Read the mb_no_coeff_skip flag */
    pc->mb_no_coeff_skip = vp8_read_bit(bd);

    vp8_decode_mode_mvs(pbi);

    memset(pc->above_context, 0, sizeof(ENTROPY_CONTEXT_PLANES) * pc->mb_cols);

    {
        int ibc = 0;
        int num_part = 1 << pc->multi_token_partition;

        /* Decode the individual macro block */
        for (mb_row = 0; mb_row < pc->mb_rows; mb_row++)
        {
            if (num_part > 1)
            {
                xd->current_bd = &pbi->mbd[ibc];
                ibc++;

                if (ibc == num_part)
                    ibc = 0;
            }

            decode_macroblock_row(pbi, pc, mb_row, xd);
        }
    }

    token_decoder_stop(pbi);

    /* Collect information about decoder corruption. */
    /* 1. Check first boolean decoder for errors. */
    pc->yv12_fb[pc->new_fb_idx].corrupted = vp8dx_bool_error(bd);

    /* 2. Check the macroblock information */
    pc->yv12_fb[pc->new_fb_idx].corrupted |= xd->corrupted;

    /* printf("Decoder: Frame Decoded, Size Roughly:%d bytes \n", bc->pos+pbi->bc2.pos); */

    /* If this was a kf or Gf note the Q used */
    if (pc->frame_type == KEY_FRAME
        || pc->refresh_golden_frame
        || pc->refresh_alt_ref_frame)
    {
        pc->last_kf_gf_q = pc->base_qindex;
    }

    if (pc->refresh_entropy_probs == 0)
    {
        memcpy(&pc->fc, &pc->lfc, sizeof(pc->fc));
    }

    return 0;
}