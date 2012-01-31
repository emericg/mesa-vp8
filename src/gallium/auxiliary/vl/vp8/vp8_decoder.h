/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef VP8_DECODER_H
#define VP8_DECODER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#include "pipe/p_video_decoder.h"

#include "vp8_debug.h"
#include "entropymv.h"
#include "entropy.h"
#include "yv12utils.h"
#include "treereader.h"
#include "dequantize.h"
#include "recon_dispatch.h"
#include "idct_dispatch.h"

#define MINQ 0
#define MAXQ 127
#define QINDEX_RANGE (MAXQ + 1)

#define NUM_YV12_BUFFERS 4

typedef struct
{
    vp8_prob bmode_prob[VP8_BINTRAMODES - 1];
    vp8_prob ymode_prob[VP8_YMODES - 1];     /**< interframe intra mode probs */
    vp8_prob uv_mode_prob[VP8_UV_MODES - 1];
    vp8_prob sub_mv_ref_prob[VP8_SUBMVREFS - 1];
    vp8_prob coef_probs[BLOCK_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS][ENTROPY_NODES];
    MV_CONTEXT mvc[2];
} FRAME_CONTEXT;

typedef struct
{
    int16_t min_val;
    int16_t length;
    uint8_t probs[12];
} TOKEN_EXTRABITS;

typedef enum
{
    ONE_PARTITION   = 0,
    TWO_PARTITION   = 1,
    FOUR_PARTITION  = 2,
    EIGHT_PARTITION = 3
} TOKEN_PARTITION;

typedef enum
{
    RECON_CLAMP_REQUIRED    = 0,
    RECON_CLAMP_NOTREQUIRED = 1
} CLAMP_TYPE;

typedef enum
{
    SIXTAP   = 0,
    BILINEAR = 1
} INTERPOLATIONFILTER_TYPE;

typedef enum
{
    NORMAL_LOOPFILTER = 0,
    SIMPLE_LOOPFILTER = 1
} LOOPFILTER_TYPE;

typedef struct VP8Common
{
    struct vpx_internal_error_info error;

    /* Bitstream buffer */
    const unsigned char *data;
    unsigned int         data_size;

    /* Boolean decoders */
    BOOL_DECODER *mbd;
    BOOL_DECODER bd, bd2;

    /* Frame header content */
    FRAME_TYPE frame_type;
    int version;
    int show_frame;

    int horizontal_scale;
    int width;
    int vertical_scale;
    int height;

    YUV_TYPE color_space;
    CLAMP_TYPE clamping_type;

    INTERPOLATIONFILTER_TYPE mcomp_filter_type;  /**< Motion compensation filter type. Not from the bitstream. */
    LOOPFILTER_TYPE filter_type;                 /**< Loop filter type */
    int filter_level;
    int sharpness_level;

    int refresh_last_frame;       /**< Two state 0 = NO, 1 = YES */
    int refresh_golden_frame;     /**< Two state 0 = NO, 1 = YES */
    int refresh_alternate_frame;  /**< Two state 0 = NO, 1 = YES */
    int refresh_entropy_probs;    /**< Two state 0 = NO, 1 = YES */

    int copy_buffer_to_golden;    /**< 0 = none, 1 = LAST to GOLDEN, 2 = ALTERNATE to GOLDEN */
    int copy_buffer_to_alternate; /**< 0 = none, 1 = LAST to ALTERNATE, 2 = GOLDEN to ALTERNATE */

    int mb_no_skip_coeff;

    /* Profile/Level settings */
    int no_lpf;
    int use_bilinear_mc_filter;
    int full_pixel;

    /* Quantization parameters */
    DECLARE_ALIGNED(16, short, Y1dequant[QINDEX_RANGE][16]);
    DECLARE_ALIGNED(16, short, Y2dequant[QINDEX_RANGE][16]);
    DECLARE_ALIGNED(16, short, UVdequant[QINDEX_RANGE][16]);

    int base_qindex;
    int last_kf_gf_q;  /**< Q used on the last GF or KF */

    int y1dc_delta_q;
    int y2dc_delta_q;
    int y2ac_delta_q;
    int uvdc_delta_q;
    int uvac_delta_q;

    /* Macroblock */
    DECLARE_ALIGNED(16, MACROBLOCKD, mb);

    int MBs;
    int mb_rows;
    int mb_cols;
    int mode_info_stride;

    /* Frame buffers */
    YV12_BUFFER_CONFIG *frame_to_show;
    YV12_BUFFER_CONFIG yv12_fb[NUM_YV12_BUFFERS];
    int fb_idx_ref_cnt[NUM_YV12_BUFFERS];
    int new_fb_idx, lst_fb_idx, gld_fb_idx, alt_fb_idx;

    /* We allocate a MODE_INFO struct for each macroblock, together with
       an extra row on top and column on the left to simplify prediction. */

    MODE_INFO *mip; /**< Base of allocated array */
    MODE_INFO *mi;  /**< Corresponds to upper left visible macroblock */


    int ref_frame_sign_bias[MAX_REF_FRAMES]; /**< Two state 0, 1 */

    /* keyframe block modes are predicted by their above, left neighbors */

    ENTROPY_CONTEXT_PLANES *above_context; /**< Row of context for each plane */
    ENTROPY_CONTEXT_PLANES left_context;   /**< (up to) 4 contexts */

    vp8_prob prob_intra;
    vp8_prob prob_last;
    vp8_prob prob_gf;
    vp8_prob prob_skip_false;

    vp8_prob kf_bmode_prob[VP8_BINTRAMODES][VP8_BINTRAMODES][VP8_BINTRAMODES - 1];
    vp8_prob kf_ymode_prob[VP8_YMODES - 1];  /**< keyframe */
    vp8_prob kf_uv_mode_prob[VP8_UV_MODES - 1];

    FRAME_CONTEXT lfc;  /**< Last frame entropy */
    FRAME_CONTEXT fc;   /**< Current frame entropy */

    TOKEN_PARTITION multi_token_partition;

} VP8_COMMON;

/* ************************************************************************** */

VP8_COMMON *vp8_decoder_create();

int vp8_decoder_start(VP8_COMMON *common,
                      struct pipe_vp8_picture_desc *frame_header,
                      const unsigned char *data, unsigned data_size);

int vp8_decoder_getframe(VP8_COMMON *common, YV12_BUFFER_CONFIG *sd);

void vp8_decoder_remove(VP8_COMMON *common);

int vp8_frame_decode(VP8_COMMON *common,
                     struct pipe_vp8_picture_desc *frame_header);

#ifdef __cplusplus
}
#endif

#endif /* VP8_DECODER_H */
