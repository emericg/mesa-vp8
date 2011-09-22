/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef __INC_VP8D_INT_H
#define __INC_VP8D_INT_H

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
    int64_t time_stamp;
    int size;
} DATARATE;

typedef struct
{
    int16_t min_val;
    int16_t Length;
    uint8_t Probs[12];
} TOKENEXTRABITS;

typedef struct
{
    int const *scan;
    uint8_t const *ptr_block2leftabove;
    vp8_tree_index const *vp8_coef_tree_ptr;
    TOKENEXTRABITS const *teb_base_ptr;
    unsigned char *norm_ptr;
    uint8_t *ptr_coef_bands_x;

    ENTROPY_CONTEXT_PLANES *A;
    ENTROPY_CONTEXT_PLANES *L;

    int16_t *qcoeff_start_ptr;
    BOOL_DECODER *current_bc;

    vp8_prob const *coef_probs[4];

    uint8_t eob[25];

} DETOK;

typedef struct VP8Decompressor
{
    DECLARE_ALIGNED(16, MACROBLOCKD, mb);

    DECLARE_ALIGNED(16, VP8_COMMON, common);

    vp8_reader bc, bc2;

    VP8D_CONFIG oxcf;

    const unsigned char *Source;
    unsigned int         source_sz;
    const unsigned char *partitions[MAX_PARTITIONS];
    unsigned int         partition_sizes[MAX_PARTITIONS];
    unsigned int         num_partitions;

    vp8_reader *mbc;
    int64_t     last_time_stamp;
    int         ready_for_new_data;

    DATARATE dr[16];

    DETOK detoken;

    vp8_prob prob_intra;
    vp8_prob prob_last;
    vp8_prob prob_gf;
    vp8_prob prob_skip_false;

    int input_partition;

} VP8D_COMP;

int vp8_decode_frame(VP8D_COMP *cpi);

#if CONFIG_DEBUG
#define CHECK_MEM_ERROR(lval,expr) do {\
        lval = (expr); \
        if(!lval) \
            vpx_internal_error(&pbi->common.error, VPX_CODEC_MEM_ERROR,\
                               "Failed to allocate "#lval" at %s:%d", \
                               __FILE__,__LINE__);\
    } while(0)
#else
#define CHECK_MEM_ERROR(lval,expr) do {\
        lval = (expr); \
        if(!lval) \
            vpx_internal_error(&pbi->common.error, VPX_CODEC_MEM_ERROR,\
                               "Failed to allocate "#lval);\
    } while(0)
#endif /* CONFIG_DEBUG */

#endif /* __INC_VP8D_INT_H */
