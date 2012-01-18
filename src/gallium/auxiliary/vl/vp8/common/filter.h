/*
 *  Copyright (c) 2011 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef FILTER_H
#define FILTER_H

void vp8_sixtap_predict4x4_c(unsigned char *src_ptr,
                             int src_pixels_per_line,
                             int xoffset,
                             int yoffset,
                             unsigned char *dst_ptr,
                             int dst_pitch);

void vp8_sixtap_predict8x8_c(unsigned char *src_ptr,
                             int src_pixels_per_line,
                             int xoffset,
                             int yoffset,
                             unsigned char *dst_ptr,
                             int dst_pitch);

void vp8_sixtap_predict8x4_c(unsigned char *src_ptr,
                             int src_pixels_per_line,
                             int xoffset,
                             int yoffset,
                             unsigned char *dst_ptr,
                             int dst_pitch);

void vp8_sixtap_predict16x16_c(unsigned char *src_ptr,
                               int src_pixels_per_line,
                               int xoffset,
                               int yoffset,
                               unsigned char *dst_ptr,
                               int dst_pitch);

void vp8_bilinear_predict4x4_c(unsigned char *src_ptr,
                               int src_pixels_per_line,
                               int xoffset,
                               int yoffset,
                               unsigned char *dst_ptr,
                               int dst_pitch);

void vp8_bilinear_predict8x8_c(unsigned char *src_ptr,
                               int src_pixels_per_line,
                               int xoffset,
                               int yoffset,
                               unsigned char *dst_ptr,
                               int dst_pitch);

void vp8_bilinear_predict8x4_c(unsigned char *src_ptr,
                               int src_pixels_per_line,
                               int xoffset,
                               int yoffset,
                               unsigned char *dst_ptr,
                               int dst_pitch);

void vp8_bilinear_predict16x16_c(unsigned char *src_ptr,
                                 int src_pixels_per_line,
                                 int xoffset,
                                 int yoffset,
                                 unsigned char *dst_ptr,
                                 int dst_pitch);

#endif /* FILTER_H */
