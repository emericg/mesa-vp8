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

typedef enum
{
    PRED = 0,
    DEST = 1
} BLOCKSET;

static void setup_block(BLOCKD *b,
                        int mv_stride,
                        unsigned char **base,
                        int stride,
                        int offset,
                        BLOCKSET bs)
{
    if (bs == DEST)
    {
        b->dst_stride = stride;
        b->dst = offset;
        b->base_dst = base;
    }
    else
    {
        b->pre_stride = stride;
        b->pre = offset;
        b->base_pre = base;
    }
}

static void setup_macroblock(MACROBLOCKD *mb, BLOCKSET bs)
{
    int block;
    unsigned char **y, **u, **v;

    if (bs == DEST)
    {
        y = &mb->dst.y_buffer;
        u = &mb->dst.u_buffer;
        v = &mb->dst.v_buffer;
    }
    else
    {
        y = &mb->pre.y_buffer;
        u = &mb->pre.u_buffer;
        v = &mb->pre.v_buffer;
    }

    for (block = 0; block < 16; block++) /* Y blocks */
    {
        setup_block(&mb->block[block], mb->dst.y_stride, y, mb->dst.y_stride,
                    (block >> 2) * 4 * mb->dst.y_stride + (block & 3) * 4, bs);
    }

    for (block = 16; block < 20; block++) /* U and V blocks */
    {
        setup_block(&mb->block[block], mb->dst.uv_stride, u, mb->dst.uv_stride,
                    ((block - 16) >> 1) * 4 * mb->dst.uv_stride + (block & 1) * 4, bs);

        setup_block(&mb->block[block+4], mb->dst.uv_stride, v, mb->dst.uv_stride,
                    ((block - 16) >> 1) * 4 * mb->dst.uv_stride + (block & 1) * 4, bs);
    }
}

void vp8_setup_block_dptrs(MACROBLOCKD *mb)
{
    int r, c;

    for (r = 0; r < 4; r++)
    {
        for (c = 0; c < 4; c++)
        {
            mb->block[r*4+c].diff      = &mb->diff[r * 4 * 16 + c * 4];
            mb->block[r*4+c].predictor = mb->predictor + r * 4 * 16 + c * 4;
        }
    }

    for (r = 0; r < 2; r++)
    {
        for (c = 0; c < 2; c++)
        {
            mb->block[16+r*2+c].diff      = &mb->diff[256 + r * 4 * 8 + c * 4];
            mb->block[16+r*2+c].predictor = mb->predictor + 256 + r * 4 * 8 + c * 4;
        }
    }

    for (r = 0; r < 2; r++)
    {
        for (c = 0; c < 2; c++)
        {
            mb->block[20+r*2+c].diff      = &mb->diff[320+ r * 4 * 8 + c * 4];
            mb->block[20+r*2+c].predictor = mb->predictor + 320 + r * 4 * 8 + c * 4;
        }
    }

    mb->block[24].diff = &mb->diff[384];

    for (r = 0; r < 25; r++)
    {
        mb->block[r].qcoeff  = mb->qcoeff  + r * 16;
        mb->block[r].dqcoeff = mb->dqcoeff + r * 16;
    }
}

void vp8_setup_block_doffsets(MACROBLOCKD *mb)
{
    /* Handle the destination pitch features */
    setup_macroblock(mb, DEST);
    setup_macroblock(mb, PRED);
}

void update_blockd_bmi(MACROBLOCKD *mb)
{
    int i;

    /* If the block size is 4x4. */
    if (mb->mode_info_context->mbmi.mode == SPLITMV ||
        mb->mode_info_context->mbmi.mode == B_PRED)
    {
        for (i = 0; i < 16; i++)
        {
            mb->block[i].bmi = mb->mode_info_context->bmi[i];
        }
    }
}
