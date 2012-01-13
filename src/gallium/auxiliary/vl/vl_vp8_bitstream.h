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

#ifndef vl_vp8_bitstream_h
#define vl_vp8_bitstream_h

#include "vl_defines.h"

struct vl_vp8_bs
{
   struct pipe_video_decoder *decoder;

   struct pipe_vp8_picture_desc desc;
};

void
vl_vp8_bs_init(struct vl_vp8_bs *bs, struct pipe_video_decoder *decoder);

void
vl_vp8_bs_set_picture_desc(struct vl_vp8_bs *bs, struct pipe_vp8_picture_desc *picture);

void
vl_vp8_bs_decode(struct vl_vp8_bs *bs, unsigned num_buffers,
                 const void * const *buffers, const unsigned *sizes);

#endif /* vl_vp8_bitstream_h */
