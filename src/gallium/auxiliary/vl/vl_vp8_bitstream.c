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

#include <stdint.h>

#include <pipe/p_video_state.h>

#include "vl_vlc.h"
#include "vl_vp8_bitstream.h"

static INLINE bool
decode_frame(struct vl_vp8_bs *bs)
{
   // STUB

   return false;
}

void
vl_vp8_bs_init(struct vl_vp8_bs *bs, struct pipe_video_decoder *decoder)
{
   assert(bs);

   memset(bs, 0, sizeof(struct vl_vp8_bs));

   bs->decoder = decoder;
}

void
vl_vp8_bs_set_picture_desc(struct vl_vp8_bs *bs, struct pipe_vp8_picture_desc *picture)
{
   bs->desc = *picture;
}

void
vl_vp8_bs_decode(struct vl_vp8_bs *bs, unsigned num_bytes, const uint8_t *buffer)
{
   assert(bs);
   assert(buffer && num_bytes);

#if 0
   {
      printf("[G3DVL] vl_vp8_bs_decode()\n");

      // Print some parameters from mplayer/ffmpeg
      printf("[G3DVL] picture_info->version       %i\n", picture->base.profile);
      printf("[G3DVL] picture_info->key_frame     %i\n", picture->key_frame);
      printf("[G3DVL] picture_info->show_frame    %i\n", picture->show_frame);

      // Print the begining of the buffer, for debugging purpose
      printf("[G3DVL] buffer [%02X %02X %02X %02X ", buffer[0], buffer[1], buffer[2], buffer[3]);
      printf("%02X %02X %02X %02X]\n", buffer[4], buffer[5], buffer[6], buffer[7]);
   }
#endif

   if (buffer[0] == 0x9D &&
       buffer[1] == 0x01 &&
       buffer[2] == 0x2A)
   {
      printf("[G3DVL] buffer start_code [%02X %02X %02X]\n", buffer[0], buffer[1], buffer[2]);
   }
   else
   {
      if (decode_frame(bs) == false)
      {
         printf("[G3DVL] Error while decoding VP8 frame !\n");
      }
   }
}
