/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include <stdlib.h>

#include "filter.h"
#include "vp8_mem.h"

#define VP8_FILTER_WEIGHT 128
#define VP8_FILTER_SHIFT  7

DECLARE_ALIGNED(16, const short, vp8_filters_bilinear[8][2]) =
{
    { 128,   0 },
    { 112,  16 },
    {  96,  32 },
    {  80,  48 },
    {  64,  64 },
    {  48,  80 },
    {  32,  96 },
    {  16, 112 }
};

DECLARE_ALIGNED(16, const short, vp8_filters_sixtap[8][6]) =
{

    { 0,  0,  128,    0,   0,  0 },   /* note that 1/8 pel positions are just as per alpha -0.5 bicubic */
    { 0, -6,  123,   12,  -1,  0 },
    { 2, -11, 108,   36,  -8,  1 },   /* New 1/4 pel 6 tap filter */
    { 0, -9,   93,   50,  -6,  0 },
    { 3, -16,  77,   77, -16,  3 },   /* New 1/2 pel 6 tap filter */
    { 0, -6,   50,   93,  -9,  0 },
    { 1, -8,   36,  108, -11,  2 },   /* New 1/4 pel 6 tap filter */
    { 0, -1,   12,  123,  -6,  0 },
};

static void filter_sixtap_block2d_first_pass(unsigned char *src_ptr,
                                             int *output_ptr,
                                             unsigned int src_pixels_per_line,
                                             unsigned int pixel_step,
                                             unsigned int output_height,
                                             unsigned int output_width,
                                             const short *vp8_filter)
{
    unsigned int i, j;
    int temp;

    for (i = 0; i < output_height; i++)
    {
        for (j = 0; j < output_width; j++)
        {
            temp = ((int)src_ptr[-2 * (int)pixel_step] * vp8_filter[0]) +
                   ((int)src_ptr[-1 * (int)pixel_step] * vp8_filter[1]) +
                   ((int)src_ptr[0]                    * vp8_filter[2]) +
                   ((int)src_ptr[pixel_step]           * vp8_filter[3]) +
                   ((int)src_ptr[2*pixel_step]         * vp8_filter[4]) +
                   ((int)src_ptr[3*pixel_step]         * vp8_filter[5]) +
                   (VP8_FILTER_WEIGHT >> 1);   /* Rounding */

            /* Normalize back to 0-255 */
            temp >>= VP8_FILTER_SHIFT;

            if (temp < 0)
                temp = 0;
            else if (temp > 255)
                temp = 255;

            output_ptr[j] = temp;
            src_ptr++;
        }

        /* Next row... */
        src_ptr    += src_pixels_per_line - output_width;
        output_ptr += output_width;
    }
}

static void filter_sixtap_block2d_second_pass(int *src_ptr,
                                              unsigned char *output_ptr,
                                              int output_pitch,
                                              unsigned int src_pixels_per_line,
                                              unsigned int pixel_step,
                                              unsigned int output_height,
                                              unsigned int output_width,
                                              const short *vp8_filter)
{
    unsigned int i, j;
    int temp;

    for (i = 0; i < output_height; i++)
    {
        for (j = 0; j < output_width; j++)
        {
            /* Apply filter */
            temp = ((int)src_ptr[-2 * (int)pixel_step] * vp8_filter[0]) +
                   ((int)src_ptr[-1 * (int)pixel_step] * vp8_filter[1]) +
                   ((int)src_ptr[0]                    * vp8_filter[2]) +
                   ((int)src_ptr[pixel_step]           * vp8_filter[3]) +
                   ((int)src_ptr[2*pixel_step]         * vp8_filter[4]) +
                   ((int)src_ptr[3*pixel_step]         * vp8_filter[5]) +
                   (VP8_FILTER_WEIGHT >> 1);   /* Rounding */

            /* Normalize back to 0-255 */
            temp >>= VP8_FILTER_SHIFT;

            if (temp < 0)
                temp = 0;
            else if (temp > 255)
                temp = 255;

            output_ptr[j] = (unsigned char)temp;
            src_ptr++;
        }

        /* Start next row */
        src_ptr    += src_pixels_per_line - output_width;
        output_ptr += output_pitch;
    }
}

static void filter_sixtap_block2d(unsigned char *src_ptr,
                                  unsigned char *output_ptr,
                                  unsigned int src_pixels_per_line,
                                  int output_pitch,
                                  const short *HFilter,
                                  const short *VFilter)
{
    int FData[9*4]; /* Temp data buffer used in filtering */

    /* First filter 1-D horizontally... */
    filter_sixtap_block2d_first_pass(src_ptr - (2 * src_pixels_per_line), FData, src_pixels_per_line, 1, 9, 4, HFilter);

    /* Then filter verticaly... */
    filter_sixtap_block2d_second_pass(FData + 8, output_ptr, output_pitch, 4, 4, 4, 4, VFilter);
}


void vp8_sixtap_predict4x4_c(unsigned char *src_ptr,
                             int src_pixels_per_line,
                             int xoffset,
                             int yoffset,
                             unsigned char *dst_ptr,
                             int dst_pitch)
{
    const short *HFilter = vp8_filters_sixtap[xoffset];
    const short *VFilter = vp8_filters_sixtap[yoffset];

    filter_sixtap_block2d(src_ptr, dst_ptr, src_pixels_per_line, dst_pitch, HFilter, VFilter);
}

void vp8_sixtap_predict8x8_c(unsigned char *src_ptr,
                             int src_pixels_per_line,
                             int xoffset,
                             int yoffset,
                             unsigned char *dst_ptr,
                             int dst_pitch)
{
    const short *HFilter = vp8_filters_sixtap[xoffset];
    const short *VFilter = vp8_filters_sixtap[yoffset];
    int FData[13*16];   /* Temp data buffer used in filtering */

    /* First filter 1-D horizontally... */
    filter_sixtap_block2d_first_pass(src_ptr - (2 * src_pixels_per_line), FData, src_pixels_per_line, 1, 13, 8, HFilter);

    /* Then filter verticaly... */
    filter_sixtap_block2d_second_pass(FData + 16, dst_ptr, dst_pitch, 8, 8, 8, 8, VFilter);
}

void vp8_sixtap_predict8x4_c(unsigned char *src_ptr,
                             int src_pixels_per_line,
                             int xoffset,
                             int yoffset,
                             unsigned char *dst_ptr,
                             int dst_pitch)
{
    const short *HFilter = vp8_filters_sixtap[xoffset];
    const short *VFilter = vp8_filters_sixtap[yoffset];
    int FData[13*16];   /* Temp data buffer used in filtering */

    /* First filter 1-D horizontally... */
    filter_sixtap_block2d_first_pass(src_ptr - (2 * src_pixels_per_line), FData, src_pixels_per_line, 1, 9, 8, HFilter);

    /* Then filter verticaly... */
    filter_sixtap_block2d_second_pass(FData + 16, dst_ptr, dst_pitch, 8, 8, 4, 8, VFilter);
}

void vp8_sixtap_predict16x16_c(unsigned char *src_ptr,
                               int src_pixels_per_line,
                               int xoffset,
                               int yoffset,
                               unsigned char *dst_ptr,
                               int dst_pitch)
{
    const short *HFilter = vp8_filters_sixtap[xoffset];
    const short *VFilter = vp8_filters_sixtap[yoffset];
    int FData[21*24];   /* Temp data buffer used in filtering */

    /* First filter 1-D horizontally... */
    filter_sixtap_block2d_first_pass(src_ptr - (2 * src_pixels_per_line), FData, src_pixels_per_line, 1, 21, 16, HFilter);

    /* Then filter verticaly... */
    filter_sixtap_block2d_second_pass(FData + 32, dst_ptr, dst_pitch, 16, 16, 16, 16, VFilter);
}

/* ************************************************************************** */

/**
 * \param[in] *src_ptr Pointer to source block.
 * \param[out] *dst_ptr Pointer to destination (filtered) block.
 * \param[in] src_stride Stride of source block.
 * \param[in] width Block width.
 * \param[in] height Block height.
 * \param[in] *vp8_filter Array of 2 bilinear filter taps.
 *
 * \note Produces INT32 output to retain precision for next pass. Two filter
 *       taps should sum to VP8_FILTER_WEIGHT.
 *
 * Applies a 1-D 2-tap bilinear filter to the source block in the horizontal
 * direction to produce the filtered output block. Used to implement first-pass
 * of 2-D separable filter.
 */
static void filter_bilinear_block2d_first_pass(unsigned char  *src_ptr,
                                               unsigned short *dst_ptr,
                                               unsigned int    src_stride,
                                               unsigned int    width,
                                               unsigned int    height,
                                               const short    *vp8_filter)
{
    unsigned int i, j;

    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
        {
            /* Apply bilinear filter */
            dst_ptr[j] = (((int)src_ptr[0] * vp8_filter[0]) +
                          ((int)src_ptr[1] * vp8_filter[1]) +
                          (VP8_FILTER_WEIGHT / 2)) >> VP8_FILTER_SHIFT;
            src_ptr++;
        }

        /* Next row... */
        src_ptr += src_stride - width;
        dst_ptr += width;
    }
}

/**
 * \param[in] *src_ptr Pointer to source block.
 * \param[out] *dst_ptr Pointer to destination (filtered) block.
 * \param[in] dst_pitch Destination block pitch.
 * \param[in] width Block width.
 * \param[in] height Block height.
 * \param[in] *vp8_filter Array of 2 bilinear filter taps.
 *
 * \note Requires 32-bit input as produced by filter_block2d_bil_first_pass.
 *       Two filter taps should sum to VP8_FILTER_WEIGHT.
 *
 * Applies a 1-D 2-tap bilinear filter to the source block in the vertical
 * direction to produce the filtered output block. Used to implement second-pass
 * of 2-D separable filter.
 */
static void filter_bilinear_block2d_second_pass(unsigned short *src_ptr,
                                                unsigned char  *dst_ptr,
                                                int             dst_pitch,
                                                unsigned int    width,
                                                unsigned int    height,
                                                const short    *vp8_filter)
{
    unsigned int i, j;
    int temp;

    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
        {
            /* Apply filter */
            temp = ((int)src_ptr[0]     * vp8_filter[0]) +
                   ((int)src_ptr[width] * vp8_filter[1]) +
                   (VP8_FILTER_WEIGHT / 2);
            dst_ptr[j] = (unsigned int)(temp >> VP8_FILTER_SHIFT);
            src_ptr++;
        }

        /* Next row... */
        dst_ptr += dst_pitch;
    }
}

/**
 * \param[in] *src_ptr Pointer to source block.
 * \param[out] *dst_ptr Pointer to destination (filtered) block.
 * \param[in] src_pitch Stride of source block.
 * \param[in] dst_pitch Stride of destination block.
 * \param[in] *HFilter Array of 2 horizontal filter taps.
 * \param[in] *VFilter Array of 2 vertical filter taps.
 * \param[in] width Block width.
 * \param[in] height Block height.
 *
 * \note The largest block size can be handled here is 16x16.
 *
 * 2-D filters an input block by applying a 2-tap bilinear filter horizontally
 * followed by a 2-tap bilinear filter vertically on the result.
 */
static void filter_bilinear_block2d(unsigned char *src_ptr,
                                    unsigned char *dst_ptr,
                                    unsigned int   src_pitch,
                                    unsigned int   dst_pitch,
                                    const short   *HFilter,
                                    const short   *VFilter,
                                    int            width,
                                    int            height)
{
    unsigned short FData[17*16];    /* Temp data buffer used in filtering */

    /* First filter 1-D horizontally... */
    filter_bilinear_block2d_first_pass(src_ptr, FData, src_pitch, width, height + 1, HFilter);

    /* then 1-D vertically... */
    filter_bilinear_block2d_second_pass(FData, dst_ptr, dst_pitch, width, height, VFilter);
}

void vp8_bilinear_predict4x4_c(unsigned char *src_ptr,
                               int src_pixels_per_line,
                               int xoffset,
                               int yoffset,
                               unsigned char *dst_ptr,
                               int dst_pitch)
{
    const short *HFilter = vp8_filters_bilinear[xoffset];
    const short *VFilter = vp8_filters_bilinear[yoffset];

    filter_bilinear_block2d(src_ptr, dst_ptr, src_pixels_per_line, dst_pitch, HFilter, VFilter, 4, 4);
}

void vp8_bilinear_predict8x8_c(unsigned char *src_ptr,
                               int src_pixels_per_line,
                               int xoffset,
                               int yoffset,
                               unsigned char *dst_ptr,
                               int dst_pitch)
{
    const short *HFilter = vp8_filters_bilinear[xoffset];
    const short *VFilter = vp8_filters_bilinear[yoffset];

    filter_bilinear_block2d(src_ptr, dst_ptr, src_pixels_per_line, dst_pitch, HFilter, VFilter, 8, 8);
}

void vp8_bilinear_predict8x4_c(unsigned char *src_ptr,
                               int src_pixels_per_line,
                               int xoffset,
                               int yoffset,
                               unsigned char *dst_ptr,
                               int dst_pitch)
{
    const short *HFilter = vp8_filters_bilinear[xoffset];
    const short *VFilter = vp8_filters_bilinear[yoffset];

    filter_bilinear_block2d(src_ptr, dst_ptr, src_pixels_per_line, dst_pitch, HFilter, VFilter, 8, 4);
}

void vp8_bilinear_predict16x16_c(unsigned char *src_ptr,
                                 int src_pixels_per_line,
                                 int xoffset,
                                 int yoffset,
                                 unsigned char *dst_ptr,
                                 int dst_pitch)
{
    const short *HFilter = vp8_filters_bilinear[xoffset];
    const short *VFilter = vp8_filters_bilinear[yoffset];

    filter_bilinear_block2d(src_ptr, dst_ptr, src_pixels_per_line, dst_pitch, HFilter, VFilter, 16, 16);
}
