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

#include <math.h>
#include <assert.h>

#include "util/u_memory.h"
#include "util/u_rect.h"
#include "util/u_sampler.h"
#include "util/u_video.h"

#include "vl_vp8_decoder.h"
#include "vl_defines.h"

#define SCALE_FACTOR_SNORM (32768.0f / 256.0f)
#define SCALE_FACTOR_SSCALED (1.0f / 256.0f)

struct format_config {
   enum pipe_format loopfilter_source_format;
};

static const struct format_config bitstream_format_config[] = {
//   { PIPE_FORMAT_R16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, PIPE_FORMAT_R16G16B16A16_FLOAT, 1.0f, SCALE_FACTOR_SSCALED },
//   { PIPE_FORMAT_R16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, 1.0f, SCALE_FACTOR_SSCALED },
   { PIPE_FORMAT_R16_SNORM, PIPE_FORMAT_R16G16B16A16_SNORM, PIPE_FORMAT_R16G16B16A16_FLOAT, 1.0f, SCALE_FACTOR_SNORM },
   { PIPE_FORMAT_R16_SNORM, PIPE_FORMAT_R16G16B16A16_SNORM, PIPE_FORMAT_R16G16B16A16_SNORM, 1.0f, SCALE_FACTOR_SNORM }
};

static const unsigned num_bitstream_format_configs =
   sizeof(bitstream_format_config) / sizeof(struct format_config);

static const struct format_config loopfilter_format_config[] = {
//   { PIPE_FORMAT_R16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, PIPE_FORMAT_R16G16B16A16_FLOAT, 1.0f, SCALE_FACTOR_SSCALED },
//   { PIPE_FORMAT_R16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, 1.0f, SCALE_FACTOR_SSCALED },
   { PIPE_FORMAT_R16_SNORM, PIPE_FORMAT_R16G16B16A16_SNORM, PIPE_FORMAT_R16G16B16A16_FLOAT, 1.0f, SCALE_FACTOR_SNORM },
   { PIPE_FORMAT_R16_SNORM, PIPE_FORMAT_R16G16B16A16_SNORM, PIPE_FORMAT_R16G16B16A16_SNORM, 1.0f, SCALE_FACTOR_SNORM }
};

static const unsigned num_loopfilter_format_configs =
   sizeof(loopfilter_format_config) / sizeof(struct format_config);

static void
vl_vp8_destroy(struct pipe_video_decoder *decoder)
{
   struct vl_vp8_decoder *dec = (struct vl_vp8_decoder *)decoder;
   assert(dec);

   printf("[G3DVL] vl_vp8_destroy()\n");

   /* Asserted in softpipe_delete_fs_state() for some reason */
   dec->base.context->bind_vs_state(dec->base.context, NULL);
   dec->base.context->bind_fs_state(dec->base.context, NULL);

   dec->base.context->delete_depth_stencil_alpha_state(dec->base.context, dec->dsa);
   dec->base.context->delete_sampler_state(dec->base.context, dec->sampler_ycbcr);

   vp8_decoder_remove(dec->vp8_dec);
   FREE(dec);
}

static void *
vl_vp8_create_buffer(struct pipe_video_decoder *decoder)
{
   struct vl_vp8_decoder *dec = (struct vl_vp8_decoder *)decoder;
   struct vl_vp8_buffer *buffer;

   assert(dec);

   buffer = CALLOC_STRUCT(vl_vp8_buffer);
   if (buffer == NULL)
      return NULL;

   if (dec->base.entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM)
      vl_vp8_bs_init(&buffer->bs, decoder);

   return buffer;
}

static void
vl_vp8_destroy_buffer(struct pipe_video_decoder *decoder, void *buffer)
{
   struct vl_vp8_decoder *dec = (struct vl_vp8_decoder *)decoder;
   struct vl_vp8_buffer *buf = buffer;

   assert(dec && buf);

   FREE(buf);
}

static void
vl_vp8_set_decode_buffer(struct pipe_video_decoder *decoder, void *buffer)
{
   struct vl_vp8_decoder *dec = (struct vl_vp8_decoder *)decoder;

   assert(dec && buffer);

   dec->current_buffer = buffer;
}

static void
vl_vp8_set_picture_parameters(struct pipe_video_decoder *decoder,
                              struct pipe_picture_desc *picture)
{
   struct vl_vp8_decoder *dec = (struct vl_vp8_decoder *)decoder;
   struct pipe_vp8_picture_desc *pic = (struct pipe_vp8_picture_desc *)picture;

   assert(dec && pic);

   dec->picture_desc = *pic;
}

static void
vl_vp8_set_quant_matrix(struct pipe_video_decoder *decoder,
                        const struct pipe_quant_matrix *matrix)
{
   struct vl_vp8_decoder *dec = (struct vl_vp8_decoder *)decoder;
   assert(dec);
}

static void
vl_vp8_set_decode_target(struct pipe_video_decoder *decoder,
                         struct pipe_video_buffer *target)
{
   struct vl_vp8_decoder *dec = (struct vl_vp8_decoder *)decoder;
   struct pipe_surface **surfaces;
   unsigned i;

   assert(dec);

   dec->target = target;

   surfaces = target->get_surfaces(target);
   for (i = 0; i < VL_MAX_PLANES; ++i)
      pipe_surface_reference(&dec->target_surfaces[i], surfaces[i]);
}

static void
vl_vp8_set_reference_frames(struct pipe_video_decoder *decoder,
                            struct pipe_video_buffer **ref_frames,
                            unsigned num_ref_frames)
{
   struct vl_vp8_decoder *dec = (struct vl_vp8_decoder *)decoder;
   struct pipe_sampler_view **sv;
   unsigned i,j;

   assert(dec);
   assert(num_ref_frames <= VL_MAX_REF_FRAMES);

   for (i = 0; i < num_ref_frames; ++i) {
      sv = ref_frames[i]->get_sampler_view_planes(ref_frames[i]);
      for (j = 0; j < VL_MAX_PLANES; ++j)
         pipe_sampler_view_reference(&dec->ref_frames[i][j], sv[j]);
   }

   for (; i < VL_MAX_REF_FRAMES; ++i)
      for (j = 0; j < VL_MAX_PLANES; ++j)
         pipe_sampler_view_reference(&dec->ref_frames[i][j], NULL);
}

static void
vl_vp8_begin_frame(struct pipe_video_decoder *decoder)
{
   struct vl_vp8_decoder *dec = (struct vl_vp8_decoder *)decoder;
   struct vl_vp8_buffer *buf;

   assert(dec);

   buf = dec->current_buffer;
   assert(buf);

   dec->startcode = 0;
   dec->img_ready = 0;
}

static void
vl_vp8_decode_macroblock(struct pipe_video_decoder *decoder,
                         const struct pipe_macroblock *macroblocks,
                         unsigned num_macroblocks)
{
   struct vl_vp8_decoder *dec = (struct vl_vp8_decoder *)decoder;
   const struct pipe_vp8_macroblock *mb = (const struct pipe_vp8_macroblock *)macroblocks;
   struct vl_vp8_buffer *buf;

   assert(dec && dec->current_buffer);
   assert(macroblocks && macroblocks->codec == PIPE_VIDEO_CODEC_VP8);

   buf = dec->current_buffer;
   assert(buf);
/*
   for (; num_macroblocks > 0; --num_macroblocks) {
      // STUB
   }
*/
}

static void
vl_vp8_decode_bitstream(struct pipe_video_decoder *decoder,
                        unsigned num_bytes, const void *data)
{
   struct vl_vp8_decoder *dec;
   struct vl_vp8_buffer *buf;

   dec = (struct vl_vp8_decoder *)decoder;
   assert(dec && dec->current_buffer);

   buf = dec->current_buffer;
   assert(buf);

   assert(data);

   //vl_vp8_bs_decode(&buf->bs, num_bytes, data);

   if (num_bytes == 0)
   {
      printf("[G3DVL] Error : no data !\n");
      //((VP8_COMMON *)((VP8D_COMP *)(dec->vp8_dec))->common)->show_frame = 0;
      return;
   }

   if (dec->startcode)
   {
       //printf("[0]frame_type = %u \n", dec->picture_desc.key_frame);
       //printf("[0]version = %u \n", dec->picture_desc.base.profile);
       //printf("[0]show_frame = %u \n", dec->picture_desc.show_frame);
       //printf("[0]first_partition_size = %u \n \n", dec->picture_desc.first_partition_size);

       if (vp8_decoder_start(dec->vp8_dec, &(dec->picture_desc), data, num_bytes, 0))
       {
          printf("[G3DVL] Error : not a valid VP8 VDPAU frame !\n");
          dec->img_ready = 0;
       }
       else
       {
          dec->img_ready = 1;
       }
   }
   else
   {
      const uint8_t *datab = (const uint8_t *)data;

      if (datab[0] == 0x9D &&
          datab[1] == 0x01 &&
          datab[2] == 0x2A)
      {
         //printf("[G3DVL] buffer start_code [9D 01 2A] is present\n");
         dec->startcode = 1;

         //TODO handle the startcode presence on the same buffer than the data
      }
   }
}

static void
vl_vp8_end_frame(struct pipe_video_decoder *decoder)
{
   struct vl_vp8_decoder *dec;
   struct vl_vp8_buffer *buf;

   struct pipe_context *pipe;
   struct pipe_sampler_view **sampler_views;

   int64_t timestamp = 0, timestamp_end = 0;

   unsigned i;

   dec = (struct vl_vp8_decoder*)decoder;
   assert(dec);

   buf = dec->current_buffer;
   assert(buf);

   pipe = buf->bs.decoder->context;
   if (!pipe) {
      printf("[end_frame] no pipe\n");
      return;
   }

   sampler_views = dec->target->get_sampler_view_planes(dec->target);
   if (!sampler_views) {
      printf("[end_frame] no sampler_views\n");
      return;
   }

   // Get the decoded frame
   if (vp8_decoder_getframe(dec->vp8_dec, &dec->img_yv12, &timestamp, &timestamp_end)) {
       printf("[end_frame] No image to output !\n");
       return;
   }

   // Load YCbCr planes into a GPU texture
   for (i = 0; i < 3; ++i)
   {
      struct pipe_sampler_view *sv = sampler_views[i ? i ^ 3 : 0];
      struct pipe_box dst_box = { 0, 0, 0, sv->texture->width0, sv->texture->height0, 1 };

      struct pipe_transfer *transfer;
      void *map;

      transfer = pipe->get_transfer(pipe, sv->texture, 0, PIPE_TRANSFER_WRITE, &dst_box);
      if (!transfer)
      {
         printf("[end_frame] no transfer\n");
         return;
      }

      ubyte *dst = dec->img_yv12.y_buffer;

      if (i == 1)
          dst = dec->img_yv12.v_buffer;
      else if (i == 2)
          dst = dec->img_yv12.u_buffer;

      map = pipe->transfer_map(pipe, transfer);
      if (map)
      {
         util_copy_rect(map, sv->texture->format, transfer->stride, 0, 0,
                        dst_box.width, dst_box.height, dst,
                        (i ? dec->img_yv12.uv_stride : dec->img_yv12.y_stride),
                        0, 0);

         pipe->transfer_unmap(pipe, transfer);
      }

      pipe->transfer_destroy(pipe, transfer);
   }
}

static void
vl_vp8_flush(struct pipe_video_decoder *decoder)
{
   assert(decoder);

   //Noop, for shaders it is much faster to flush everything in end_frame
}

static bool
init_pipe_state(struct vl_vp8_decoder *dec)
{
   struct pipe_depth_stencil_alpha_state dsa;
   struct pipe_sampler_state sampler;
   unsigned i;

   assert(dec);

   memset(&dsa, 0, sizeof dsa);
   dsa.depth.enabled = 0;
   dsa.depth.writemask = 0;
   dsa.depth.func = PIPE_FUNC_ALWAYS;
   for (i = 0; i < 2; ++i) {
      dsa.stencil[i].enabled = 0;
      dsa.stencil[i].func = PIPE_FUNC_ALWAYS;
      dsa.stencil[i].fail_op = PIPE_STENCIL_OP_KEEP;
      dsa.stencil[i].zpass_op = PIPE_STENCIL_OP_KEEP;
      dsa.stencil[i].zfail_op = PIPE_STENCIL_OP_KEEP;
      dsa.stencil[i].valuemask = 0;
      dsa.stencil[i].writemask = 0;
   }
   dsa.alpha.enabled = 0;
   dsa.alpha.func = PIPE_FUNC_ALWAYS;
   dsa.alpha.ref_value = 0;
   dec->dsa = dec->base.context->create_depth_stencil_alpha_state(dec->base.context, &dsa);
   dec->base.context->bind_depth_stencil_alpha_state(dec->base.context, dec->dsa);

   memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_BORDER;
   sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
   sampler.compare_func = PIPE_FUNC_ALWAYS;
   sampler.normalized_coords = 1;
   dec->sampler_ycbcr = dec->base.context->create_sampler_state(dec->base.context, &sampler);
   if (!dec->sampler_ycbcr)
      return false;

   return true;
}

static const struct format_config*
find_format_config(struct vl_vp8_decoder *dec,
                   const struct format_config configs[],
                   unsigned num_configs)
{
   struct pipe_screen *screen;
   unsigned i;

   assert(dec);

   screen = dec->base.context->screen;

   for (i = 0; i < num_configs; ++i) {
      if (!screen->is_format_supported(screen,
                                       configs[i].loopfilter_source_format,
                                       PIPE_TEXTURE_2D,
                                       1, PIPE_BIND_SAMPLER_VIEW))
         continue;

      return &configs[i];
   }

   return NULL;
}

/** Called by the VDPAU state tracker */
struct pipe_video_decoder *
vl_create_vp8_decoder(struct pipe_context *context,
                      enum pipe_video_profile profile,
                      enum pipe_video_entrypoint entrypoint,
                      enum pipe_video_chroma_format chroma_format,
                      unsigned width, unsigned height, unsigned max_references)
{
   const unsigned block_size_pixels = VL_MICROBLOCK_WIDTH * VL_MICROBLOCK_HEIGHT;
   const struct format_config *format_config;
   struct vl_vp8_decoder *dec;

   printf("[G3DVL] vl_create_vp8_decoder()\n");

   assert(u_reduce_video_profile(profile) == PIPE_VIDEO_CODEC_VP8);

   dec = CALLOC_STRUCT(vl_vp8_decoder);

   if (!dec)
      return NULL;

   dec->base.context = context;
   dec->base.profile = profile;
   dec->base.entrypoint = entrypoint;
   dec->base.chroma_format = chroma_format;
   dec->base.width = width;
   dec->base.height = height;
   dec->base.max_references = max_references;

   dec->base.destroy = vl_vp8_destroy;
   dec->base.create_buffer = vl_vp8_create_buffer;
   dec->base.destroy_buffer = vl_vp8_destroy_buffer;
   dec->base.set_decode_buffer = vl_vp8_set_decode_buffer;
   dec->base.set_picture_parameters = vl_vp8_set_picture_parameters;
   dec->base.set_quant_matrix = vl_vp8_set_quant_matrix;
   dec->base.set_decode_target = vl_vp8_set_decode_target;
   dec->base.set_reference_frames = vl_vp8_set_reference_frames;
   dec->base.begin_frame = vl_vp8_begin_frame;
   dec->base.decode_macroblock = vl_vp8_decode_macroblock;
   dec->base.decode_bitstream = vl_vp8_decode_bitstream;
   dec->base.end_frame = vl_vp8_end_frame;
   dec->base.flush = vl_vp8_flush;

   dec->blocks_per_line = MAX2(util_next_power_of_two(dec->base.width) / block_size_pixels, 4);
   dec->num_blocks = (dec->base.width * dec->base.height) / block_size_pixels;

   assert(dec->base.chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420);
   dec->chroma_width = dec->base.width / 2;
   dec->chroma_height = dec->base.height / 2;
   dec->width_in_macroblocks = align(dec->base.width, MACROBLOCK_WIDTH) / MACROBLOCK_WIDTH;

   switch (entrypoint) {
   case PIPE_VIDEO_ENTRYPOINT_BITSTREAM:
      format_config = find_format_config(dec, bitstream_format_config, num_bitstream_format_configs);
      break;
   case PIPE_VIDEO_ENTRYPOINT_LOOPFILTER:
      format_config = find_format_config(dec, loopfilter_format_config, num_loopfilter_format_configs);
      break;
   default:
      assert(0);
      return NULL;
   }

   if (!format_config)
      return NULL;

   if (!init_pipe_state(dec))
      goto error_pipe_state;

   // Initialize the vp8 decoder instance
   dec->vp8_dec = vp8_decoder_create();

   if (!dec->vp8_dec)
      goto error_vp8_dec;

   return &dec->base;

error_pipe_state:
error_vp8_dec:
   FREE(dec);

error_buffer:
   return NULL;
}
