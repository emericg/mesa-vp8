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
    enum pipe_format mc_source_format;
    enum pipe_format loopfilter_source_format;

    float mc_scale;
};

static const struct format_config bitstream_format_config[] = {
//   { PIPE_FORMAT_R16G16B16A16_FLOAT, PIPE_FORMAT_R16G16B16A16_FLOAT, SCALE_FACTOR_SSCALED },
//   { PIPE_FORMAT_R16G16B16A16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, SCALE_FACTOR_SSCALED },
   { PIPE_FORMAT_R16G16B16A16_FLOAT, PIPE_FORMAT_R16G16B16A16_FLOAT, SCALE_FACTOR_SNORM },
   { PIPE_FORMAT_R16G16B16A16_SNORM, PIPE_FORMAT_R16G16B16A16_SNORM, SCALE_FACTOR_SNORM }
};

static const unsigned num_bitstream_format_configs =
   sizeof(bitstream_format_config) / sizeof(struct format_config);

static const struct format_config mc_format_config[] = {
   //{ PIPE_FORMAT_R16_SSCALED, PIPE_FORMAT_R16_SSCALED, SCALE_FACTOR_SSCALED },
   { PIPE_FORMAT_R16_SNORM, PIPE_FORMAT_R16_SNORM, SCALE_FACTOR_SNORM }
};

static const unsigned num_mc_format_configs =
   sizeof(mc_format_config) / sizeof(struct format_config);

static const struct format_config loopfilter_format_config[] = {
    //{ PIPE_FORMAT_R16G16B16A16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, SCALE_FACTOR_SSCALED },
    { PIPE_FORMAT_R16G16B16A16_SNORM, PIPE_FORMAT_R16G16B16A16_SNORM, SCALE_FACTOR_SNORM }
};

static const unsigned num_loopfilter_format_configs =
   sizeof(loopfilter_format_config) / sizeof(struct format_config);

static void
vl_vp8_destroy_buffer(void *buffer)
{
   struct vl_vp8_buffer *buf = buffer;

   assert(buf);

   vl_vb_cleanup(&buf->vertex_stream);

   FREE(buf);
}

static void
vl_vp8_destroy(struct pipe_video_decoder *decoder)
{
   struct vl_vp8_decoder *dec = (struct vl_vp8_decoder *)decoder;
   unsigned i;

   assert(dec);

   printf("[G3DVL] vl_vp8_destroy()\n");

   /* Asserted in softpipe_delete_fs_state() for some reason */
   dec->base.context->bind_vs_state(dec->base.context, NULL);
   dec->base.context->bind_fs_state(dec->base.context, NULL);

   dec->base.context->delete_depth_stencil_alpha_state(dec->base.context, dec->dsa);
   dec->base.context->delete_sampler_state(dec->base.context, dec->sampler_ycbcr);

   for (i = 0; i < 4; ++i)
      if (dec->dec_buffers[i])
          vl_vp8_destroy_buffer(dec->dec_buffers[i]);

   // Destroy the VP8 software decoder
   vp8_decoder_remove(dec->vp8_dec);

   FREE(dec);
}

static struct vl_vp8_buffer *
vl_vp8_get_decode_buffer(struct vl_vp8_decoder *dec, struct pipe_video_buffer *target)
{
   struct vl_vp8_buffer *buffer;

   assert(dec);

   buffer = vl_video_buffer_get_associated_data(target, &dec->base);

   if (buffer)
      return buffer;

   buffer = dec->dec_buffers[dec->current_buffer];
   if (buffer)
      return buffer;

   buffer = CALLOC_STRUCT(vl_vp8_buffer);
   if (buffer == NULL)
      return NULL;

   if (!vl_vb_init(&buffer->vertex_stream, dec->base.context,
                   dec->base.width / MACROBLOCK_WIDTH,
                   dec->base.height / MACROBLOCK_HEIGHT))
      goto error_vertex_buffer;

   if (dec->base.entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM)
      vl_vp8_bs_init(&buffer->bs, &dec->base);

   if (dec->expect_chunked_decode)
      vl_video_buffer_set_associated_data(target, &dec->base,
                                          buffer, vl_vp8_destroy_buffer);
   else
      dec->dec_buffers[dec->current_buffer] = buffer;

   return buffer;

error_vertex_buffer:
   FREE(buffer);
   return NULL;
}

static void
vl_vp8_begin_frame(struct pipe_video_decoder *decoder,
                   struct pipe_video_buffer *target,
                   struct pipe_picture_desc *picture)
{
   struct vl_vp8_decoder *dec = (struct vl_vp8_decoder *)decoder;
   struct pipe_vp8_picture_desc *desc = (struct pipe_vp8_picture_desc *)picture;
   struct vl_vp8_buffer *buf;

   assert(dec && target && picture);

   buf = vl_vp8_get_decode_buffer(dec, target);
   assert(buf);

   vl_vb_map(&buf->vertex_stream, dec->base.context);

   if (dec->base.entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM) {
      //
   }

   dec->current_buffer = 0;
   dec->img_ready = 0;
}

static void
vl_vp8_decode_macroblock(struct pipe_video_decoder *decoder,
                         struct pipe_video_buffer *target,
                         struct pipe_picture_desc *picture,
                         const struct pipe_macroblock *macroblocks,
                         unsigned num_macroblocks)
{
   struct vl_vp8_decoder *dec = (struct vl_vp8_decoder *)decoder;
   struct pipe_vp8_picture_desc *desc = (struct pipe_vp8_picture_desc *)picture;
   const struct pipe_vp8_macroblock *mb = (const struct pipe_vp8_macroblock *)macroblocks;
   struct vl_vp8_buffer *buf;

   assert(dec && target && picture);
   assert(macroblocks && macroblocks->codec == PIPE_VIDEO_CODEC_VP8);

   buf = vl_vp8_get_decode_buffer(dec, target);
   assert(buf);
}

static void
vl_vp8_decode_bitstream(struct pipe_video_decoder *decoder,
                        struct pipe_video_buffer *target,
                        struct pipe_picture_desc *picture,
                        unsigned num_buffers,
                        const void * const *buffers,
                        const unsigned *sizes)
{
   struct vl_vp8_decoder *dec = (struct vl_vp8_decoder *)decoder;
   struct pipe_vp8_picture_desc *desc = (struct pipe_vp8_picture_desc *)picture;
   struct vl_vp8_buffer *buf;

   assert(dec && target && picture);

   buf = vl_vp8_get_decode_buffer(dec, target);
   assert(buf);

   // Try and detect start_code from the first data buffer
   const uint8_t *datab = (const uint8_t *)buffers[dec->current_buffer];

   if (datab[0] == 0x9D &&
       datab[1] == 0x01 &&
       datab[2] == 0x2A)
   {
      if (sizes[dec->current_buffer] == 3)
      {
         // The start_code [0x9D012A] is present in a dedicated buffer
         ++dec->current_buffer;
         dec->current_buffer %= 4;
      }
      else if (num_buffers == 1)
      {
         // The start_code [0x9D012A] is present in the same buffer as the bistream data
         //buffers[dec->current_buffer] += 3;
      }

      // Start bitstream decoding
      //vl_vp8_bs_decode(&buf->bs, target, desc, num_buffers, buffers, sizes);

      if (vp8_decoder_start(dec->vp8_dec, desc, (const uint8_t *)buffers[dec->current_buffer], sizes[dec->current_buffer]))
      {
         printf("[G3DVL] Error : decoding error, not a valid VP8 VDPAU frame !\n");
         dec->img_ready = 0;
      }
      else
      {
         dec->img_ready = 1;
      }
   }
   else
   {
      printf("[G3DVL] Error : the first data buffer does not contain the mandatory start_code [0x9D012A] !\n");
   }
}

static void
vl_vp8_end_frame(struct pipe_video_decoder *decoder,
                 struct pipe_video_buffer *target,
                 struct pipe_picture_desc *picture)
{
   struct vl_vp8_decoder *dec = (struct vl_vp8_decoder*)decoder;
   struct vl_vp8_buffer *buf;
   struct pipe_vp8_picture_desc *desc = (struct pipe_vp8_picture_desc *)picture;
   struct pipe_sampler_view **ref_frames[VL_MAX_REF_FRAMES];
   struct pipe_sampler_view **mc_source_sv;
   struct pipe_surface **target_surfaces;
   struct pipe_vertex_buffer vb[3];

   const unsigned *plane_order;
   unsigned i, j, component;
   unsigned nr_components;

   struct pipe_sampler_view **sampler_views;
   struct pipe_context *pipe;

   assert(dec && target && picture);

   buf = vl_vp8_get_decode_buffer(dec, target);
   assert(buf);

   vl_vb_unmap(&buf->vertex_stream, dec->base.context);

   vb[0] = dec->quads;
   vb[1] = dec->pos;

   target_surfaces = target->get_surfaces(target);

   for (i = 0; i < VL_MAX_REF_FRAMES; ++i) {
      if (desc->ref[i])
         ref_frames[i] = desc->ref[i]->get_sampler_view_planes(desc->ref[i]);
      else
         ref_frames[i] = NULL;
   }

   // VP8 bypass transfert frame
   pipe = buf->bs.decoder->context;
   if (!pipe) {
      printf("[end_frame] No pipe\n");
      return;
   }
/*
   if (target->buffer_format != PIPE_FORMAT_YV12) {
      printf("[end_frame] bad format ? current is %i\n", target->buffer_format);

      // destroy the old one
      if (target)
         target->destroy(target);

      // adjust the template parameters
      struct pipe_video_buffer templat;
      memset(&templat, 0, sizeof(templat));
      templat.chroma_format = PIPE_VIDEO_CHROMA_FORMAT_420;
      templat.width = target->width;
      templat.height = target->height;
      templat.buffer_format = PIPE_FORMAT_YV12;
      templat.interlaced = false;

      // and try to create the video buffer with the new format
      target = pipe->create_video_buffer(pipe, &templat);

      // stil no luck? ok forget it we don't support it
      if (!target)
         return;

      if (target->buffer_format != PIPE_FORMAT_YV12) {
         printf("[end_frame] bad format ? current is %i\n", target->buffer_format);
      } else {
          printf("[end_frame] bad format ? nope, allright than\n");
      }
   }
*/
   sampler_views = target->get_sampler_view_planes(target);
   if (!sampler_views) {
      printf("[end_frame] No sampler_views\n");
      return;
   }

   // Get the decoded frame
   if (vp8_decoder_getframe(dec->vp8_dec, &dec->img_yv12)) {
      printf("[end_frame] No image to output !\n");
      return;
   }

   // Load YCbCr planes into a GPU texture
   for (i = 0; i < 3; ++i)
   {
      struct pipe_sampler_view *sv = sampler_views[i];
      struct pipe_box dst_box = { 0, 0, 0, sv->texture->width0, sv->texture->height0, 1 };
      struct pipe_transfer *dst_transfer = pipe->get_transfer(pipe, sv->texture, 0, PIPE_TRANSFER_WRITE, &dst_box);;

      if (!dst_transfer) {
         printf("[end_frame] No transfer\n");
         return;
      }

      void *dst = pipe->transfer_map(pipe, dst_transfer);
      ubyte *src = dec->img_yv12.y_buffer;

      if (i == 1)
         src = dec->img_yv12.v_buffer;
      else if (i == 2)
         src = dec->img_yv12.u_buffer;

      if (dst) {
         util_copy_rect(dst,
                        sv->texture->format,
                        dst_transfer->stride,
                        dst_box.x, dst_box.y,
                        dst_box.width, dst_box.height,
                        src,
                        (i ? dec->img_yv12.uv_stride : dec->img_yv12.y_stride),
                        0, 0);

         pipe->transfer_unmap(pipe, dst_transfer);
      }

      pipe->transfer_destroy(pipe, dst_transfer);
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
find_format_config(struct vl_vp8_decoder *dec, const struct format_config configs[], unsigned num_configs)
{
   struct pipe_screen *screen;
   unsigned i;

   assert(dec);

   screen = dec->base.context->screen;

   for (i = 0; i < num_configs; ++i) {
      if (!screen->is_format_supported(screen, configs[i].mc_source_format, PIPE_TEXTURE_2D,
                                       1, PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET))
         continue;

      if (!screen->is_format_supported(screen,
                                       configs[i].loopfilter_source_format, PIPE_TEXTURE_2D,
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
                      unsigned width, unsigned height, unsigned max_references,
                      bool expect_chunked_decode)
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
   dec->base.begin_frame = vl_vp8_begin_frame;
   dec->base.decode_macroblock = vl_vp8_decode_macroblock;
   dec->base.decode_bitstream = vl_vp8_decode_bitstream;
   dec->base.end_frame = vl_vp8_end_frame;
   dec->base.flush = vl_vp8_flush;

   dec->blocks_per_line = MAX2(util_next_power_of_two(dec->base.width) / block_size_pixels, 4);
   dec->num_blocks = ((dec->base.width * dec->base.height) / block_size_pixels) * 2;
   dec->width_in_macroblocks = align(dec->base.width, MACROBLOCK_WIDTH) / MACROBLOCK_WIDTH;
   dec->expect_chunked_decode = expect_chunked_decode;

   // VP8 only support 4:2:0 chroma subsampling
   assert(dec->base.chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420);
   dec->chroma_width = dec->base.width / 2;
   dec->chroma_height = dec->base.height / 2;

   dec->quads = vl_vb_upload_quads(dec->base.context);
   dec->pos = vl_vb_upload_pos(dec->base.context,
                               dec->base.width / MACROBLOCK_WIDTH,
                               dec->base.height / MACROBLOCK_HEIGHT);

   dec->ves_ycbcr = vl_vb_get_ves_ycbcr(dec->base.context);
   dec->ves_mv = vl_vb_get_ves_mv(dec->base.context);

   switch (entrypoint) {
   case PIPE_VIDEO_ENTRYPOINT_BITSTREAM:
      format_config = find_format_config(dec, bitstream_format_config, num_bitstream_format_configs);
      break;

   case PIPE_VIDEO_ENTRYPOINT_MC:
      format_config = find_format_config(dec, mc_format_config, num_mc_format_configs);
      break;

   case PIPE_VIDEO_ENTRYPOINT_LOOPFILTER:
      format_config = find_format_config(dec, loopfilter_format_config, num_loopfilter_format_configs);
      break;

   default:
      assert(0);
      FREE(dec);
      return NULL;
   }

   if (!format_config) {
      FREE(dec);
      return NULL;
   }

   if (!init_pipe_state(dec))
      goto error_pipe_state;

   // Initialize the VP8 software decoder
   dec->vp8_dec = vp8_decoder_create();

   if (!dec->vp8_dec)
      goto error_vp8_dec;

   return &dec->base;

error_pipe_state:
error_vp8_dec:
   FREE(dec);
   return NULL;
}
