/**************************************************************************
 *
 * Copyright 2011 Emeric Grange.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef vl_vp8_decoder_h
#define vl_vp8_decoder_h

#include <stdio.h>

#include "pipe/p_video_decoder.h"

#include "vl_vp8_bitstream.h"
#include "vl_vertex_buffers.h"
#include "vl_video_buffer.h"

#include "vp8/vp8_decoder.h"
#include "vp8/yv12utils.h"

struct pipe_screen;
struct pipe_context;

struct vl_vp8_decoder
{
   struct pipe_video_decoder base;

   unsigned chroma_width, chroma_height;

   unsigned blocks_per_line;
   unsigned num_blocks;
   unsigned width_in_macroblocks;
   bool expect_chunked_decode;

   struct pipe_vertex_buffer quads;
   struct pipe_vertex_buffer pos;

   void *ves_ycbcr;
   void *ves_mv;

   void *sampler_ycbcr;
/*
   struct pipe_sampler_view *zscan_linear;
   struct pipe_sampler_view *zscan_normal;
   struct pipe_sampler_view *zscan_alternate;

   struct pipe_video_buffer *idct_source;
   struct pipe_video_buffer *mc_source;
   struct pipe_video_buffer *lf_source;

   struct vl_zscan zscan_y, zscan_c;
   struct vl_idct idct_y, idct_c;
   struct vl_mc mc_y, mc_c;
*/
   void *dsa;

   unsigned current_buffer;
   struct vl_vp8_buffer *dec_buffers[4];

   // VP8 decoder context
   VP8_COMMON *vp8_dec;
   YV12_BUFFER_CONFIG img_yv12;
};

struct vl_vp8_buffer
{
   struct vl_vertex_buffer vertex_stream;

   unsigned block_num;
   unsigned num_ycbcr_blocks[3];

   struct pipe_sampler_view *zscan_source;

   struct vl_vp8_bs bs;

   struct pipe_transfer *tex_transfer;
   short *texels;

   struct vl_ycbcr_block *ycbcr_stream[VL_NUM_COMPONENTS];
   struct vl_motionvector *mv_stream[VL_MAX_REF_FRAMES];
};

/**
 * Creates a shader based VP8 decoder.
 */
struct pipe_video_decoder *
vl_create_vp8_decoder(struct pipe_context *pipe,
                      enum pipe_video_profile profile,
                      enum pipe_video_entrypoint entrypoint,
                      enum pipe_video_chroma_format chroma_format,
                      unsigned width, unsigned height, unsigned max_references,
                      bool expect_chunked_decode);

#endif /* vl_vp8_decoder_h */
