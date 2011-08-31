/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "postproc.h"
#include "yv12utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define RGB_TO_YUV(t)                                                                       \
    ( (0.257*(float)(t>>16)) + (0.504*(float)(t>>8&0xff)) + (0.098*(float)(t&0xff)) + 16),  \
    (-(0.148*(float)(t>>16)) - (0.291*(float)(t>>8&0xff)) + (0.439*(float)(t&0xff)) + 128), \
    ( (0.439*(float)(t>>16)) - (0.368*(float)(t>>8&0xff)) - (0.071*(float)(t&0xff)) + 128)

/* global constants */

static const short kernel5[] =
{
    1, 1, 4, 1, 1
};

const short vp8_rv[] =
{
    8, 5, 2, 2, 8, 12, 4, 9, 8, 3,
    0, 3, 9, 0, 0, 0, 8, 3, 14, 4,
    10, 1, 11, 14, 1, 14, 9, 6, 12, 11,
    8, 6, 10, 0, 0, 8, 9, 0, 3, 14,
    8, 11, 13, 4, 2, 9, 0, 3, 9, 6,
    1, 2, 3, 14, 13, 1, 8, 2, 9, 7,
    3, 3, 1, 13, 13, 6, 6, 5, 2, 7,
    11, 9, 11, 8, 7, 3, 2, 0, 13, 13,
    14, 4, 12, 5, 12, 10, 8, 10, 13, 10,
    4, 14, 4, 10, 0, 8, 11, 1, 13, 7,
    7, 14, 6, 14, 13, 2, 13, 5, 4, 4,
    0, 10, 0, 5, 13, 2, 12, 7, 11, 13,
    8, 0, 4, 10, 7, 2, 7, 2, 2, 5,
    3, 4, 7, 3, 3, 14, 14, 5, 9, 13,
    3, 14, 3, 6, 3, 0, 11, 8, 13, 1,
    13, 1, 12, 0, 10, 9, 7, 6, 2, 8,
    5, 2, 13, 7, 1, 13, 14, 7, 6, 7,
    9, 6, 10, 11, 7, 8, 7, 5, 14, 8,
    4, 4, 0, 8, 7, 10, 0, 8, 14, 11,
    3, 12, 5, 7, 14, 3, 14, 5, 2, 6,
    11, 12, 12, 8, 0, 11, 13, 1, 2, 0,
    5, 10, 14, 7, 8, 0, 4, 11, 0, 8,
    0, 3, 10, 5, 8, 0, 11, 6, 7, 8,
    10, 7, 13, 9, 2, 5, 1, 5, 10, 2,
    4, 3, 5, 6, 10, 8, 9, 4, 11, 14,
    0, 10, 0, 5, 13, 2, 12, 7, 11, 13,
    8, 0, 4, 10, 7, 2, 7, 2, 2, 5,
    3, 4, 7, 3, 3, 14, 14, 5, 9, 13,
    3, 14, 3, 6, 3, 0, 11, 8, 13, 1,
    13, 1, 12, 0, 10, 9, 7, 6, 2, 8,
    5, 2, 13, 7, 1, 13, 14, 7, 6, 7,
    9, 6, 10, 11, 7, 8, 7, 5, 14, 8,
    4, 4, 0, 8, 7, 10, 0, 8, 14, 11,
    3, 12, 5, 7, 14, 3, 14, 5, 2, 6,
    11, 12, 12, 8, 0, 11, 13, 1, 2, 0,
    5, 10, 14, 7, 8, 0, 4, 11, 0, 8,
    0, 3, 10, 5, 8, 0, 11, 6, 7, 8,
    10, 7, 13, 9, 2, 5, 1, 5, 10, 2,
    4, 3, 5, 6, 10, 8, 9, 4, 11, 14,
    3, 8, 3, 7, 8, 5, 11, 4, 12, 3,
    11, 9, 14, 8, 14, 13, 4, 3, 1, 2,
    14, 6, 5, 4, 4, 11, 4, 6, 2, 1,
    5, 8, 8, 12, 13, 5, 14, 10, 12, 13,
    0, 9, 5, 5, 11, 10, 13, 9, 10, 13,
};

/* ************************************************************************** */

void vp8_post_proc_down_and_across_c(unsigned char *src_ptr,
                                     unsigned char *dst_ptr,
                                     int src_pixels_per_line,
                                     int dst_pixels_per_line,
                                     int rows,
                                     int cols,
                                     int flimit)
{
    unsigned char *p_src, *p_dst;
    int row;
    int col;
    int i;
    int v;
    int pitch = src_pixels_per_line;
    unsigned char d[8];
    (void)dst_pixels_per_line;

    for (row = 0; row < rows; row++)
    {
        /* post_proc_down for one row */
        p_src = src_ptr;
        p_dst = dst_ptr;

        for (col = 0; col < cols; col++)
        {

            int kernel = 4;
            int v = p_src[col];

            for (i = -2; i <= 2; i++)
            {
                if (abs(v - p_src[col+i*pitch]) > flimit)
                    goto down_skip_convolve;

                kernel += kernel5[2+i] * p_src[col+i*pitch];
            }

            v = (kernel >> 3);
        down_skip_convolve:
            p_dst[col] = v;
        }

        /* now post_proc_across */
        p_src = dst_ptr;
        p_dst = dst_ptr;

        for (i = 0; i < 8; i++)
            d[i] = p_src[i];

        for (col = 0; col < cols; col++)
        {
            int kernel = 4;
            v = p_src[col];

            d[col&7] = v;

            for (i = -2; i <= 2; i++)
            {
                if (abs(v - p_src[col+i]) > flimit)
                    goto across_skip_convolve;

                kernel += kernel5[2+i] * p_src[col+i];
            }

            d[col&7] = (kernel >> 3);
        across_skip_convolve:

            if (col >= 2)
                p_dst[col-2] = d[(col-2)&7];
        }

        /* handle the last two pixels */
        p_dst[col-2] = d[(col-2)&7];
        p_dst[col-1] = d[(col-1)&7];


        /* next row */
        src_ptr += pitch;
        dst_ptr += pitch;
    }
}

static int q2mbl(int x)
{
    if (x < 20) x = 20;

    x = 50 + (x - 50) * 10 / 8;
    return x * x / 3;
}

void vp8_mbpost_proc_across_ip_c(unsigned char *src, int pitch, int rows, int cols, int flimit)
{
    int r, c, i;

    unsigned char *s = src;
    unsigned char d[16];


    for (r = 0; r < rows; r++)
    {
        int sumsq = 0;
        int sum   = 0;

        for (i = -8; i <= 6; i++)
        {
            sumsq += s[i] * s[i];
            sum   += s[i];
            d[i+8] = 0;
        }

        for (c = 0; c < cols + 8; c++)
        {
            int x = s[c+7] - s[c-8];
            int y = s[c+7] + s[c-8];

            sum  += x;
            sumsq += x * y;

            d[c&15] = s[c];

            if (sumsq * 15 - sum * sum < flimit)
            {
                d[c&15] = (8 + sum + s[c]) >> 4;
            }

            s[c-8] = d[(c-8)&15];
        }

        s += pitch;
    }
}

void vp8_mbpost_proc_down_c(unsigned char *dst,
                            int pitch,
                            int rows,
                            int cols,
                            int flimit)
{
    int r, c, i;
    const short *rv3 = &vp8_rv[63&rand()];

    for (c = 0; c < cols; c++)
    {
        unsigned char *s = &dst[c];
        int sumsq = 0;
        int sum   = 0;
        unsigned char d[16];
        const short *rv2 = rv3 + ((c * 17) & 127);

        for (i = -8; i <= 6; i++)
        {
            sumsq += s[i*pitch] * s[i*pitch];
            sum   += s[i*pitch];
        }

        for (r = 0; r < rows + 8; r++)
        {
            sumsq += s[7*pitch] * s[ 7*pitch] - s[-8*pitch] * s[-8*pitch];
            sum  += s[7*pitch] - s[-8*pitch];
            d[r&15] = s[0];

            if (sumsq * 15 - sum * sum < flimit)
            {
                d[r&15] = (rv2[r&127] + sum + s[0]) >> 4;
            }

            s[-8*pitch] = d[(r-8)&15];
            s += pitch;
        }
    }
}

static void vp8_deblock_and_de_macro_block(YV12_BUFFER_CONFIG         *source,
                                           YV12_BUFFER_CONFIG         *post,
                                           int                         q,
                                           int                         low_var_thresh,
                                           int                         flag,
                                           vp8_postproc_rtcd_vtable_t *rtcd)
{
    double level = 6.0e-05 * q * q * q - .0067 * q * q + .306 * q + .0065;
    int ppl = (int)(level + .5);
    (void) low_var_thresh;
    (void) flag;

    POSTPROC_INVOKE(rtcd, downacross)(source->y_buffer, post->y_buffer, source->y_stride,  post->y_stride, source->y_height, source->y_width,  ppl);
    POSTPROC_INVOKE(rtcd, across)(post->y_buffer, post->y_stride, post->y_height, post->y_width, q2mbl(q));
    POSTPROC_INVOKE(rtcd, down)(post->y_buffer, post->y_stride, post->y_height, post->y_width, q2mbl(q));

    POSTPROC_INVOKE(rtcd, downacross)(source->u_buffer, post->u_buffer, source->uv_stride, post->uv_stride, source->uv_height, source->uv_width, ppl);
    POSTPROC_INVOKE(rtcd, downacross)(source->v_buffer, post->v_buffer, source->uv_stride, post->uv_stride, source->uv_height, source->uv_width, ppl);
}

void vp8_deblock(YV12_BUFFER_CONFIG         *source,
                 YV12_BUFFER_CONFIG         *post,
                 int                         q,
                 int                         low_var_thresh,
                 int                         flag,
                 vp8_postproc_rtcd_vtable_t *rtcd)
{
    double level = 6.0e-05 * q * q * q - .0067 * q * q + .306 * q + .0065;
    int ppl = (int)(level + .5);
    (void) low_var_thresh;
    (void) flag;

    POSTPROC_INVOKE(rtcd, downacross)(source->y_buffer, post->y_buffer, source->y_stride,  post->y_stride, source->y_height, source->y_width,   ppl);
    POSTPROC_INVOKE(rtcd, downacross)(source->u_buffer, post->u_buffer, source->uv_stride, post->uv_stride,  source->uv_height, source->uv_width, ppl);
    POSTPROC_INVOKE(rtcd, downacross)(source->v_buffer, post->v_buffer, source->uv_stride, post->uv_stride, source->uv_height, source->uv_width, ppl);
}

void vp8_de_noise(YV12_BUFFER_CONFIG         *source,
                  YV12_BUFFER_CONFIG         *post,
                  int                         q,
                  int                         low_var_thresh,
                  int                         flag,
                  vp8_postproc_rtcd_vtable_t *rtcd)
{
    double level = 6.0e-05 * q * q * q - .0067 * q * q + .306 * q + .0065;
    int ppl = (int)(level + .5);
    (void) post;
    (void) low_var_thresh;
    (void) flag;

    POSTPROC_INVOKE(rtcd, downacross)(
        source->y_buffer + 2 * source->y_stride + 2,
        source->y_buffer + 2 * source->y_stride + 2,
        source->y_stride,
        source->y_stride,
        source->y_height - 4,
        source->y_width - 4,
        ppl);
    POSTPROC_INVOKE(rtcd, downacross)(
        source->u_buffer + 2 * source->uv_stride + 2,
        source->u_buffer + 2 * source->uv_stride + 2,
        source->uv_stride,
        source->uv_stride,
        source->uv_height - 4,
        source->uv_width - 4, ppl);
    POSTPROC_INVOKE(rtcd, downacross)(
        source->v_buffer + 2 * source->uv_stride + 2,
        source->v_buffer + 2 * source->uv_stride + 2,
        source->uv_stride,
        source->uv_stride,
        source->uv_height - 4,
        source->uv_width - 4, ppl);
}

double vp8_gaussian(double sigma, double mu, double x)
{
    return 1 / (sigma * sqrt(2.0 * 3.14159265)) * (exp(-(x - mu) * (x - mu) / (2 * sigma * sigma)));
}

static void fillrd(struct postproc_state *state, int q, int a)
{
    char char_dist[300];

    double sigma;
    int ai = a, qi = q, i;

    sigma = ai + .5 + .6 * (63 - qi) / 63.0;

    /* set up a lookup table of 256 entries that matches
     * a gaussian distribution with sigma determined by q.
     */
    {
        double i;
        int next, j;

        next = 0;

        for (i = -32; i < 32; i++)
        {
            int a = (int)(.5 + 256 * vp8_gaussian(sigma, 0, i));

            if (a)
            {
                for (j = 0; j < a; j++)
                {
                    char_dist[next+j] = (char) i;
                }

                next = next + j;
            }

        }

        for (next = next; next < 256; next++)
            char_dist[next] = 0;

    }

    for (i = 0; i < 3072; i++)
    {
        state->noise[i] = char_dist[rand() & 0xff];
    }

    for (i = 0; i < 16; i++)
    {
        state->blackclamp[i] = -char_dist[0];
        state->whiteclamp[i] = -char_dist[0];
        state->bothclamp[i] = -2 * char_dist[0];
    }

    state->last_q = q;
    state->last_noise = a;
}

/****************************************************************************
 *
 *  ROUTINE       : plane_add_noise_c
 *
 *  INPUTS        : unsigned char *Start    starting address of buffer to add gaussian
 *                                  noise to
 *                  unsigned int Width    width of plane
 *                  unsigned int Height   height of plane
 *                  int  Pitch    distance between subsequent lines of frame
 *                  int  q        quantizer used to determine amount of noise
 *                                  to add
 *
 *  OUTPUTS       : None.
 *
 *  RETURNS       : void.
 *
 *  FUNCTION      : adds gaussian noise to a plane of pixels
 *
 *  SPECIAL NOTES : None.
 *
 ****************************************************************************/
void vp8_plane_add_noise_c(unsigned char *Start, char *noise,
                           char blackclamp[16],
                           char whiteclamp[16],
                           char bothclamp[16],
                           unsigned int Width, unsigned int Height, int Pitch)
{
    unsigned int i, j;

    for (i = 0; i < Height; i++)
    {
        unsigned char *Pos = Start + i * Pitch;
        char  *Ref = (char *)(noise + (rand() & 0xff));

        for (j = 0; j < Width; j++)
        {
            if (Pos[j] < blackclamp[0])
                Pos[j] = blackclamp[0];

            if (Pos[j] > 255 + whiteclamp[0])
                Pos[j] = 255 + whiteclamp[0];

            Pos[j] += Ref[j];
        }
    }
}

/* Blend the macro block with a solid colored square.  Leave the
 * edges unblended to give distinction to macro blocks in areas
 * filled with the same color block.
 */
void vp8_blend_mb_inner_c(unsigned char *y, unsigned char *u, unsigned char *v,
                          int y1, int u1, int v1, int alpha, int stride)
{
    int i, j;
    int y1_const = y1*((1<<16)-alpha);
    int u1_const = u1*((1<<16)-alpha);
    int v1_const = v1*((1<<16)-alpha);

    y += 2*stride + 2;
    for (i = 0; i < 12; i++)
    {
        for (j = 0; j < 12; j++)
        {
            y[j] = (y[j]*alpha + y1_const)>>16;
        }
        y += stride;
    }

    stride >>= 1;

    u += stride + 1;
    v += stride + 1;

    for (i = 0; i < 6; i++)
    {
        for (j = 0; j < 6; j++)
        {
            u[j] = (u[j]*alpha + u1_const)>>16;
            v[j] = (v[j]*alpha + v1_const)>>16;
        }
        u += stride;
        v += stride;
    }
}

/* Blend only the edge of the macro block.  Leave center
 * unblended to allow for other visualizations to be layered.
 */
void vp8_blend_mb_outer_c(unsigned char *y, unsigned char *u, unsigned char *v,
                          int y1, int u1, int v1, int alpha, int stride)
{
    int i, j;
    int y1_const = y1*((1<<16)-alpha);
    int u1_const = u1*((1<<16)-alpha);
    int v1_const = v1*((1<<16)-alpha);

    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 16; j++)
        {
            y[j] = (y[j]*alpha + y1_const)>>16;
        }
        y += stride;
    }

    for (i = 0; i < 12; i++)
    {
        y[0]  = (y[0]*alpha  + y1_const)>>16;
        y[1]  = (y[1]*alpha  + y1_const)>>16;
        y[14] = (y[14]*alpha + y1_const)>>16;
        y[15] = (y[15]*alpha + y1_const)>>16;
        y += stride;
    }

    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 16; j++)
        {
            y[j] = (y[j]*alpha + y1_const)>>16;
        }
        y += stride;
    }

    stride >>= 1;

    for (j = 0; j < 8; j++)
    {
        u[j] = (u[j]*alpha + u1_const)>>16;
        v[j] = (v[j]*alpha + v1_const)>>16;
    }
    u += stride;
    v += stride;

    for (i = 0; i < 6; i++)
    {
        u[0] = (u[0]*alpha + u1_const)>>16;
        v[0] = (v[0]*alpha + v1_const)>>16;

        u[7] = (u[7]*alpha + u1_const)>>16;
        v[7] = (v[7]*alpha + v1_const)>>16;

        u += stride;
        v += stride;
    }

    for (j = 0; j < 8; j++)
    {
        u[j] = (u[j]*alpha + u1_const)>>16;
        v[j] = (v[j]*alpha + v1_const)>>16;
    }
}

void vp8_blend_b_c (unsigned char *y, unsigned char *u, unsigned char *v,
                        int y1, int u1, int v1, int alpha, int stride)
{
    int i, j;
    int y1_const = y1*((1<<16)-alpha);
    int u1_const = u1*((1<<16)-alpha);
    int v1_const = v1*((1<<16)-alpha);

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            y[j] = (y[j]*alpha + y1_const)>>16;
        }
        y += stride;
    }

    stride >>= 1;

    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 2; j++)
        {
            u[j] = (u[j]*alpha + u1_const)>>16;
            v[j] = (v[j]*alpha + v1_const)>>16;
        }
        u += stride;
        v += stride;
    }
}

static void constrain_line (int x0, int *x1, int y0, int *y1, int width, int height)
{
    int dx;
    int dy;

    if (*x1 > width)
    {
        dx = *x1 - x0;
        dy = *y1 - y0;

        *x1 = width;
        if (dx)
            *y1 = ((width-x0)*dy)/dx + y0;
    }
    if (*x1 < 0)
    {
        dx = *x1 - x0;
        dy = *y1 - y0;

        *x1 = 0;
        if (dx)
            *y1 = ((0-x0)*dy)/dx + y0;
    }
    if (*y1 > height)
    {
        dx = *x1 - x0;
        dy = *y1 - y0;

        *y1 = height;
        if (dy)
            *x1 = ((height-y0)*dx)/dy + x0;
    }
    if (*y1 < 0)
    {
        dx = *x1 - x0;
        dy = *y1 - y0;

        *y1 = 0;
        if (dy)
            *x1 = ((0-y0)*dx)/dy + x0;
    }
}

#define RTCD_VTABLE(oci) NULL

int vp8_post_proc_frame(VP8_COMMON *oci, YV12_BUFFER_CONFIG *dest, vp8_ppflags_t *ppflags)
{
    int q = oci->filter_level * 10 / 6;
    int flags = ppflags->post_proc_flag;
    int deblock_level = ppflags->deblocking_level;
    int noise_level = ppflags->noise_level;

    if (!oci->frame_to_show)
        return -1;

    if (q > 63)
        q = 63;

    if (!flags)
    {
        *dest = *oci->frame_to_show;

        /* handle problem with extending borders */
        dest->y_width = oci->Width;
        dest->y_height = oci->Height;
        dest->uv_height = dest->y_height / 2;

        return 0;
    }

    if (flags & VP8D_DEMACROBLOCK)
    {
        vp8_deblock_and_de_macro_block(oci->frame_to_show, &oci->post_proc_buffer,
                                       q + (deblock_level - 5) * 10, 1, 0, RTCD_VTABLE(oci));
    }
    else if (flags & VP8D_DEBLOCK)
    {
        vp8_deblock(oci->frame_to_show, &oci->post_proc_buffer,
                    q, 1, 0, RTCD_VTABLE(oci));
    }
    else
    {
        vp8_yv12_copy_frame(oci->frame_to_show, &oci->post_proc_buffer);
    }

    if (flags & VP8D_ADDNOISE)
    {
        if (oci->postproc_state.last_q != q
            || oci->postproc_state.last_noise != noise_level)
        {
            fillrd(&oci->postproc_state, 63 - q, noise_level);
        }

        POSTPROC_INVOKE(RTCD_VTABLE(oci), addnoise)
        (oci->post_proc_buffer.y_buffer,
         oci->postproc_state.noise,
         oci->postproc_state.blackclamp,
         oci->postproc_state.whiteclamp,
         oci->postproc_state.bothclamp,
         oci->post_proc_buffer.y_width, oci->post_proc_buffer.y_height,
         oci->post_proc_buffer.y_stride);
    }

    *dest = oci->post_proc_buffer;

    /* handle problem with extending borders */
    dest->y_width = oci->Width;
    dest->y_height = oci->Height;
    dest->uv_height = dest->y_height / 2;
    return 0;
}
