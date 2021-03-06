/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "idct.h"
#include "dequantize.h"

/****************************************************************************
 * Notes:
 *
 * This implementation makes use of 16 bit fixed point verio of two multiply
 * constants:
 *         1.   sqrt(2) * cos (pi/8)
 *         2.   sqrt(2) * sin (pi/8)
 * Becuase the first constant is bigger than 1, to maintain the same 16 bit
 * fixed point precision as the second one, we use a trick of
 *         x * a = x + x*(a-1)
 * so
 *         x * sqrt(2) * cos (pi/8) = x + x * (sqrt(2) *cos(pi/8)-1).
 **************************************************************************/

static const int cospi8sqrt2minus1 = 20091;
static const int sinpi8sqrt2       = 35468;
static const int rounding          = 0;

void vp8_short_idct4x4llm_c(short *input, short *output, int pitch)
{
    int i;
    int a1, b1, c1, d1;

    short *ip = input;
    short *op = output;
    int temp1, temp2;
    int shortpitch = pitch >> 1;

    for (i = 0; i < 4; i++)
    {
        a1 = ip[0] + ip[8];
        b1 = ip[0] - ip[8];

        temp1 = (ip[4] * sinpi8sqrt2 + rounding) >> 16;
        temp2 = ip[12] + ((ip[12] * cospi8sqrt2minus1 + rounding) >> 16);
        c1 = temp1 - temp2;

        temp1 = ip[4] + ((ip[4] * cospi8sqrt2minus1 + rounding) >> 16);
        temp2 = (ip[12] * sinpi8sqrt2 + rounding) >> 16;
        d1 = temp1 + temp2;

        op[shortpitch*0] = a1 + d1;
        op[shortpitch*3] = a1 - d1;

        op[shortpitch*1] = b1 + c1;
        op[shortpitch*2] = b1 - c1;

        ip++;
        op++;
    }

    ip = output;
    op = output;

    for (i = 0; i < 4; i++)
    {
        a1 = ip[0] + ip[2];
        b1 = ip[0] - ip[2];

        temp1 = (ip[1] * sinpi8sqrt2 + rounding) >> 16;
        temp2 = ip[3] + ((ip[3] * cospi8sqrt2minus1 + rounding) >> 16);
        c1 = temp1 - temp2;

        temp1 = ip[1] + ((ip[1] * cospi8sqrt2minus1 + rounding) >> 16);
        temp2 = (ip[3] * sinpi8sqrt2 + rounding) >> 16;
        d1 = temp1 + temp2;

        op[0] = (a1 + d1 + 4) >> 3;
        op[3] = (a1 - d1 + 4) >> 3;

        op[1] = (b1 + c1 + 4) >> 3;
        op[2] = (b1 - c1 + 4) >> 3;

        ip += shortpitch;
        op += shortpitch;
    }
}

void vp8_short_idct4x4llm_1_c(short *input, short *output, int pitch)
{
    int i;
    int a1 = ((input[0] + 4) >> 3);
    short *op = output;
    int shortpitch = pitch >> 1;

    for (i = 0; i < 4; i++)
    {
        op[0] = a1;
        op[1] = a1;
        op[2] = a1;
        op[3] = a1;
        op += shortpitch;
    }
}

void vp8_dc_only_idct_add_c(short input_dc, unsigned char *pred_ptr,
                            unsigned char *dst_ptr, int pitch, int stride)
{
    int a1 = ((input_dc + 4) >> 3);
    int r, c;

    for (r = 0; r < 4; r++)
    {
        for (c = 0; c < 4; c++)
        {
            int a = a1 + pred_ptr[c];

            if (a < 0)
                a = 0;

            if (a > 255)
                a = 255;

            dst_ptr[c] = (unsigned char)a;
        }

        dst_ptr += stride;
        pred_ptr += pitch;
    }
}

void vp8_short_inv_walsh4x4_c(short *input, short *output)
{
    int i;
    int a1, b1, c1, d1;
    int a2, b2, c2, d2;
    short *ip = input;
    short *op = output;

    for (i = 0; i < 4; i++)
    {
        a1 = ip[0] + ip[12];
        b1 = ip[4] + ip[8];
        c1 = ip[4] - ip[8];
        d1 = ip[0] - ip[12];

        op[0] = a1 + b1;
        op[4] = c1 + d1;
        op[8] = a1 - b1;
        op[12] = d1 - c1;
        ip++;
        op++;
    }

    ip = output;
    op = output;

    for (i = 0; i < 4; i++)
    {
        a1 = ip[0] + ip[3];
        b1 = ip[1] + ip[2];
        c1 = ip[1] - ip[2];
        d1 = ip[0] - ip[3];

        a2 = a1 + b1;
        b2 = c1 + d1;
        c2 = a1 - b1;
        d2 = d1 - c1;

        op[0] = (a2 + 3) >> 3;
        op[1] = (b2 + 3) >> 3;
        op[2] = (c2 + 3) >> 3;
        op[3] = (d2 + 3) >> 3;

        ip += 4;
        op += 4;
    }
}

void vp8_short_inv_walsh4x4_1_c(short *input, short *output)
{
    int i;
    int a1 = ((input[0] + 3) >> 3);
    short *op = output;

    for (i = 0; i < 4; i++)
    {
        op[0] = a1;
        op[1] = a1;
        op[2] = a1;
        op[3] = a1;
        op += 4;
    }
}

/* ************************************************************************** */

void vp8_dequant_dc_idct_add_y_block_c(short *q, short *dq, unsigned char *pre,
                                       unsigned char *dst, int stride,
                                       char *eobs, short *dc)
{
    int i, j;

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            if (*eobs++ > 1)
                vp8_dequant_dc_idct_add_c(q, dq, pre, dst, 16, stride, dc[0]);
            else
                vp8_dc_only_idct_add_c(dc[0], pre, dst, 16, stride);

            q   += 16;
            pre += 4;
            dst += 4;
            dc  ++;
        }

        pre += 64 - 16;
        dst += 4*stride - 16;
    }
}

void vp8_dequant_idct_add_y_block_c(short *q, short *dq, unsigned char *pre,
                                    unsigned char *dst, int stride, char *eobs)
{
    int i, j;

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            if (*eobs++ > 1)
                vp8_dequant_idct_add_c(q, dq, pre, dst, 16, stride);
            else
            {
                vp8_dc_only_idct_add_c(q[0]*dq[0], pre, dst, 16, stride);
                ((int *)q)[0] = 0;
            }

            q   += 16;
            pre += 4;
            dst += 4;
        }

        pre += 64 - 16;
        dst += 4*stride - 16;
    }
}

void vp8_dequant_idct_add_uv_block_c(short *q, short *dq, unsigned char *pre,
                                     unsigned char *dstu, unsigned char *dstv,
                                     int stride, char *eobs)
{
    int i, j;

    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 2; j++)
        {
            if (*eobs++ > 1)
                vp8_dequant_idct_add_c(q, dq, pre, dstu, 8, stride);
            else
            {
                vp8_dc_only_idct_add_c(q[0]*dq[0], pre, dstu, 8, stride);
                ((int *)q)[0] = 0;
            }

            q    += 16;
            pre  += 4;
            dstu += 4;
        }

        pre  += 32 - 8;
        dstu += 4*stride - 8;
    }

    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 2; j++)
        {
            if (*eobs++ > 1)
                vp8_dequant_idct_add_c(q, dq, pre, dstv, 8, stride);
            else
            {
                vp8_dc_only_idct_add_c(q[0]*dq[0], pre, dstv, 8, stride);
                ((int *)q)[0] = 0;
            }

            q    += 16;
            pre  += 4;
            dstv += 4;
        }

        pre  += 32 - 8;
        dstv += 4*stride - 8;
    }
}
