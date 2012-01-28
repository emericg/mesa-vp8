/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "blockd.h"
#include "alloccommon.h"
#include "findnearmv.h"
#include "entropymode.h"

#include "vp8_decoder.h"
#include "vp8_mem.h"

static void update_mode_info_border(MODE_INFO *mi, int rows, int cols)
{
    int i;
    memset(mi - cols - 2, 0, sizeof(MODE_INFO) * (cols + 1));

    for (i = 0; i < rows; i++)
    {
        /* TODO(holmer): Bug? This updates the last element of each row
         * rather than the border element!
         */
        memset(&mi[i*cols - 1], 0, sizeof(MODE_INFO));
    }
}

void vp8_dealloc_frame_buffers(VP8_COMMON *common)
{
    int i;

    for (i = 0; i < NUM_YV12_BUFFERS; i++)
        vp8_yv12_de_alloc_frame_buffer(&common->yv12_fb[i]);

    vpx_free(common->above_context);
    vpx_free(common->mip);

    common->above_context = 0;
    common->mip = 0;
}

int vp8_alloc_frame_buffers(VP8_COMMON *common, int width, int height)
{
    int i;

    vp8_dealloc_frame_buffers(common);

    /* our internal buffers are always multiples of 16 */
    if ((width & 0xf) != 0)
        width += 16 - (width & 0xf);

    if ((height & 0xf) != 0)
        height += 16 - (height & 0xf);

    for (i = 0; i < NUM_YV12_BUFFERS; i++)
    {
        common->fb_idx_ref_cnt[i] = 0;
        common->yv12_fb[i].flags = 0;
        if (vp8_yv12_alloc_frame_buffer(&common->yv12_fb[i], width, height, VP8BORDERINPIXELS) < 0)
        {
            vp8_dealloc_frame_buffers(common);
            return 1;
        }
    }

    common->new_fb_idx = 0;
    common->lst_fb_idx = 1;
    common->gld_fb_idx = 2;
    common->alt_fb_idx = 3;

    common->fb_idx_ref_cnt[0] = 1;
    common->fb_idx_ref_cnt[1] = 1;
    common->fb_idx_ref_cnt[2] = 1;
    common->fb_idx_ref_cnt[3] = 1;

    common->mb_rows = height >> 4;
    common->mb_cols = width >> 4;
    common->MBs = common->mb_rows * common->mb_cols;
    common->mode_info_stride = common->mb_cols + 1;
    common->mip = vpx_calloc((common->mb_cols + 1) * (common->mb_rows + 1), sizeof(MODE_INFO));

    if (!common->mip)
    {
        vp8_dealloc_frame_buffers(common);
        return 1;
    }

    common->mi = common->mip + common->mode_info_stride + 1;

    common->above_context = vpx_calloc(sizeof(ENTROPY_CONTEXT_PLANES) * common->mb_cols, 1);

    if (!common->above_context)
    {
        vp8_dealloc_frame_buffers(common);
        return 1;
    }

    update_mode_info_border(common->mi, common->mb_rows, common->mb_cols);

    return 0;
}

/**
 * Initialize version specific parameters.
 * The VP8 "version" field is the equivalent of the mpeg "profile".
 */
void vp8_setup_version(VP8_COMMON *common)
{
    switch (common->version)
    {
    case 0:
        common->no_lpf = 0;
        common->filter_type = NORMAL_LOOPFILTER;
        common->use_bilinear_mc_filter = 0;
        common->full_pixel = 0;
        break;
    case 1:
        common->no_lpf = 0;
        common->filter_type = SIMPLE_LOOPFILTER;
        common->use_bilinear_mc_filter = 1;
        common->full_pixel = 0;
        break;
    case 2:
        common->no_lpf = 1;
        common->filter_type = NORMAL_LOOPFILTER;
        common->use_bilinear_mc_filter = 1;
        common->full_pixel = 0;
        break;
    case 3:
        common->no_lpf = 1;
        common->filter_type = SIMPLE_LOOPFILTER;
        common->use_bilinear_mc_filter = 1;
        common->full_pixel = 1;
        break;
    default:
        /* 4,5,6,7 are reserved for future use */
        common->no_lpf = 0;
        common->filter_type = NORMAL_LOOPFILTER;
        common->use_bilinear_mc_filter = 0;
        common->full_pixel = 0;
        break;
    }
}

void vp8_initialize_common(VP8_COMMON *common)
{
    vp8_default_coef_probs(common);
    vp8_init_mbmode_probs(common);
    vp8_default_bmode_probs(common->fc.bmode_prob);

    common->mb_no_coeff_skip = 1;
    common->no_lpf = 0;
    common->filter_type = NORMAL_LOOPFILTER;
    common->use_bilinear_mc_filter = 0;
    common->full_pixel = 0;
    common->multi_token_partition = ONE_PARTITION;
    common->clr_type = REG_YUV;
    common->clamp_type = RECON_CLAMP_REQUIRED;

    /* Initialise reference frame sign bias structure to defaults */
    memset(common->ref_frame_sign_bias, 0, sizeof(common->ref_frame_sign_bias));

    /* Default disable buffer to buffer copying */
    common->copy_buffer_to_gf = 0;
    common->copy_buffer_to_arf = 0;

    /*  */
    vp8_coef_tree_initialize();
    vp8_entropy_mode_init();
    vp8_init_scan_order_mask();
}
