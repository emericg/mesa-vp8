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
#include "vl_video_buffer.h"

#include "vp8/vp8_decoder.h"

struct pipe_screen;
struct pipe_context;

struct vl_vp8_decoder
{
   struct pipe_video_decoder base;

   unsigned chroma_width, chroma_height;

   unsigned blocks_per_line;
   unsigned num_blocks;
   unsigned width_in_macroblocks;

   void *sampler_ycbcr;

   void *dsa;

   struct vl_vp8_buffer *current_buffer;
   struct pipe_vp8_picture_desc picture_desc;
   struct pipe_sampler_view *ref_frames[VL_MAX_REF_FRAMES][VL_MAX_PLANES];
   struct pipe_video_buffer *target;
   struct pipe_surface *target_surfaces[VL_MAX_PLANES];

   // vp8 decoder context
   vpx_codec_priv_t *priv; // lvl 1
   vpx_codec_alg_priv_t *alg_priv; // lvl 2
};

struct vl_vp8_buffer
{
   struct vl_vp8_bs bs;
};

/**
 * Creates a shader based VP8 decoder.
 */
struct pipe_video_decoder *
vl_create_vp8_decoder(struct pipe_context *pipe,
                      enum pipe_video_profile profile,
                      enum pipe_video_entrypoint entrypoint,
                      enum pipe_video_chroma_format chroma_format,
                      unsigned width, unsigned height, unsigned max_references);

#endif /* vl_vp8_decoder_h */
