/**************************************************************************
 *
 * Copyright 2010 Thomas Balling Sørensen.
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

#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_debug.h"

#include "vdpau_private.h"

VdpStatus
vlVdpDecoderCreate(VdpDevice device,
                   VdpDecoderProfile profile,
                   uint32_t width, uint32_t height,
                   uint32_t max_references,
                   VdpDecoder *decoder)
{
   enum pipe_video_profile p_profile;
   struct pipe_context *pipe;
   vlVdpDevice *dev;
   vlVdpDecoder *vldecoder;
   VdpStatus ret;
   unsigned i;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Creating decoder\n");

   if (!decoder)
      return VDP_STATUS_INVALID_POINTER;

   if (!(width && height))
      return VDP_STATUS_INVALID_VALUE;

   p_profile = ProfileToPipe(profile);
   if (p_profile == PIPE_VIDEO_PROFILE_UNKNOWN)
      return VDP_STATUS_INVALID_DECODER_PROFILE;

   dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;

   pipe = dev->context->pipe;

   vldecoder = CALLOC(1,sizeof(vlVdpDecoder));
   if (!vldecoder)
      return VDP_STATUS_RESOURCES;

   vldecoder->device = dev;

   vldecoder->decoder = pipe->create_video_decoder(pipe, p_profile,
                                                   PIPE_VIDEO_ENTRYPOINT_BITSTREAM,
                                                   PIPE_VIDEO_CHROMA_FORMAT_420,
                                                   width, height, max_references);

   if (!vldecoder->decoder) {
      ret = VDP_STATUS_ERROR;
      goto error_decoder;
   }

   vldecoder->num_buffers = pipe->screen->get_video_param
   (
      pipe->screen, p_profile,
      PIPE_VIDEO_CAP_NUM_BUFFERS_DESIRED
   );
   vldecoder->cur_buffer = 0;

   vldecoder->buffers = CALLOC(vldecoder->num_buffers, sizeof(void*));
   if (!vldecoder->buffers)
         goto error_alloc_buffers;

   for (i = 0; i < vldecoder->num_buffers; ++i) {
      vldecoder->buffers[i] = vldecoder->decoder->create_buffer(vldecoder->decoder);
      if (!vldecoder->buffers[i]) {
         ret = VDP_STATUS_ERROR;
         goto error_create_buffers;
      }
   }

   *decoder = vlAddDataHTAB(vldecoder);
   if (*decoder == 0) {
      ret = VDP_STATUS_ERROR;
      goto error_handle;
   }

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Decoder created succesfully\n");

   return VDP_STATUS_OK;

error_handle:
error_create_buffers:

   for (i = 0; i < vldecoder->num_buffers; ++i)
      if (vldecoder->buffers[i])
         vldecoder->decoder->destroy_buffer(vldecoder->decoder, vldecoder->buffers[i]);

   FREE(vldecoder->buffers);

error_alloc_buffers:

   vldecoder->decoder->destroy(vldecoder->decoder);

error_decoder:
   FREE(vldecoder);
   return ret;
}

VdpStatus
vlVdpDecoderDestroy(VdpDecoder decoder)
{
   vlVdpDecoder *vldecoder;
   unsigned i;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Destroying decoder\n");

   vldecoder = (vlVdpDecoder *)vlGetDataHTAB(decoder);
   if (!vldecoder)
      return VDP_STATUS_INVALID_HANDLE;

   for (i = 0; i < vldecoder->num_buffers; ++i)
      if (vldecoder->buffers[i])
         vldecoder->decoder->destroy_buffer(vldecoder->decoder, vldecoder->buffers[i]);

   FREE(vldecoder->buffers);

   vldecoder->decoder->destroy(vldecoder->decoder);

   FREE(vldecoder);

   return VDP_STATUS_OK;
}

VdpStatus
vlVdpDecoderGetParameters(VdpDecoder decoder,
                          VdpDecoderProfile *profile,
                          uint32_t *width,
                          uint32_t *height)
{
   vlVdpDecoder *vldecoder;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] decoder get parameters called\n");

   vldecoder = (vlVdpDecoder *)vlGetDataHTAB(decoder);
   if (!vldecoder)
      return VDP_STATUS_INVALID_HANDLE;

   *profile = PipeToProfile(vldecoder->decoder->profile);
   *width = vldecoder->decoder->width;
   *height = vldecoder->decoder->height;

   return VDP_STATUS_OK;
}

static VdpStatus
vlVdpDecoderRenderMpeg12(struct pipe_video_decoder *decoder,
                         VdpPictureInfoMPEG1Or2 *picture_info,
                         uint32_t bitstream_buffer_count,
                         VdpBitstreamBuffer const *bitstream_buffers)
{
   struct pipe_mpeg12_picture_desc picture;
   struct pipe_mpeg12_quant_matrix quant;
   struct pipe_video_buffer *ref_frames[2];
   unsigned i;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Decoding MPEG2\n");

   i = 0;

   /* Get back reference pictures
      If reference surfaces equals VDP_INVALID_HANDLE, they are not used */
   if (picture_info->forward_reference !=  VDP_INVALID_HANDLE) {
      ref_frames[i] = ((vlVdpSurface *)vlGetDataHTAB(picture_info->forward_reference))->video_buffer;
      if (!ref_frames[i])
         return VDP_STATUS_INVALID_HANDLE;
      ++i;
   }

   if (picture_info->backward_reference !=  VDP_INVALID_HANDLE) {
      ref_frames[i] = ((vlVdpSurface *)vlGetDataHTAB(picture_info->backward_reference))->video_buffer;
      if (!ref_frames[i])
         return VDP_STATUS_INVALID_HANDLE;
      ++i;
   }

   decoder->set_reference_frames(decoder, ref_frames, i);

   /* Get back picture parameters */
   memset(&picture, 0, sizeof(picture));
   picture.base.profile = decoder->profile;
   picture.picture_coding_type = picture_info->picture_coding_type;
   picture.picture_structure = picture_info->picture_structure;
   picture.frame_pred_frame_dct = picture_info->frame_pred_frame_dct;
   picture.q_scale_type = picture_info->q_scale_type;
   picture.alternate_scan = picture_info->alternate_scan;
   picture.intra_vlc_format = picture_info->intra_vlc_format;
   picture.concealment_motion_vectors = picture_info->concealment_motion_vectors;
   picture.intra_dc_precision = picture_info->intra_dc_precision;
   picture.f_code[0][0] = picture_info->f_code[0][0] - 1;
   picture.f_code[0][1] = picture_info->f_code[0][1] - 1;
   picture.f_code[1][0] = picture_info->f_code[1][0] - 1;
   picture.f_code[1][1] = picture_info->f_code[1][1] - 1;

   decoder->set_picture_parameters(decoder, &picture.base);

   memset(&quant, 0, sizeof(quant));
   quant.base.codec = PIPE_VIDEO_CODEC_MPEG12;
   quant.intra_matrix = picture_info->intra_quantizer_matrix;
   quant.non_intra_matrix = picture_info->non_intra_quantizer_matrix;

   decoder->set_quant_matrix(decoder, &quant.base);

   decoder->begin_frame(decoder);

   for (i = 0; i < bitstream_buffer_count; ++i)
      decoder->decode_bitstream(decoder, bitstream_buffers[i].bitstream_bytes,
                                bitstream_buffers[i].bitstream);

   decoder->end_frame(decoder);

   return VDP_STATUS_OK;
}

static VdpStatus
vlVdpDecoderRenderVP8(struct pipe_video_decoder *decoder,
                      VdpPictureInfoVP8 *picture_info,
                      uint32_t bitstream_buffer_count,
                      VdpBitstreamBuffer const *bitstream_buffers)
{
   struct pipe_vp8_picture_desc picture;
   struct pipe_video_buffer *ref_frames[3];
   unsigned i, j, k, l;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Decoding VP8\n");

   /* Get back reference pictures
      If reference surfaces equals VDP_INVALID_HANDLE, they are not used */
   ref_frames[0] = NULL;
   ref_frames[1] = NULL;
   ref_frames[2] = NULL;

   if (picture_info->golden_frame != VDP_INVALID_HANDLE) {
      ref_frames[0] = ((vlVdpSurface *)vlGetDataHTAB(picture_info->golden_frame))->video_buffer;
      if (!ref_frames[0])
         return VDP_STATUS_INVALID_HANDLE;
   }
   if (picture_info->altref_frame != VDP_INVALID_HANDLE) {
      ref_frames[1] = ((vlVdpSurface *)vlGetDataHTAB(picture_info->altref_frame))->video_buffer;
      if (!ref_frames[1])
         return VDP_STATUS_INVALID_HANDLE;
   }
   if (picture_info->previous_frame != VDP_INVALID_HANDLE) {
      ref_frames[2] = ((vlVdpSurface *)vlGetDataHTAB(picture_info->previous_frame))->video_buffer;
      if (!ref_frames[2])
         return VDP_STATUS_INVALID_HANDLE;
   }

   decoder->set_reference_frames(decoder, ref_frames, i);

   /* Get back picture parameters */
   memset(&picture, 0, sizeof(picture));
   picture.base.profile = picture_info->version;
   picture.key_frame = picture_info->key_frame;
   picture.show_frame = picture_info->show_frame;
   picture.first_partition_size = picture_info->first_partition_size;
   picture.horizontal_scale = picture_info->horizontal_scale;
   picture.width = picture_info->width;
   picture.vertical_scale = picture_info->vertical_scale;
   picture.height = picture_info->height;
   picture.color_space = picture_info->color_space;
   picture.clamping_type = picture_info->clamping_type;
   picture.segmentation_enable = picture_info->segmentation_enable;
   picture.segment_map_update = picture_info->segment_map_update;
   picture.segment_data_update = picture_info->segment_data_update;
   picture.segment_data_mode = picture_info->segment_data_mode;
   for (i = 0; i < 4; i++) {
      picture.segment_base_quant[i] = picture_info->segment_base_quant[i];
      picture.segment_filter_level[i] = picture_info->segment_filter_level[i];
   }
   for (i = 0; i < 3; i++) {
      picture.segment_id[i] = picture_info->segment_id[i];
   }
   picture.filter_type = picture_info->filter_type;
   picture.filter_level = picture_info->filter_level;
   picture.filter_sharpness_level = picture_info->filter_sharpness_level;
   picture.filter_update = picture_info->filter_update;
   for (i = 0; i < 4; i++) {
      picture.filter_update_value[i] = picture_info->filter_update_value[i];
      picture.filter_update_sign[i] = picture_info->filter_update_sign[i];
   }
   picture.num_coeff_partitions = picture_info->num_coeff_partitions;
   for (i = 0; i < 4; i++) {
       for (j = 0; j < 2; j++) {
           picture.dquant[i].luma_qi[j] = picture_info->dquant[i].luma_qi[j];
           picture.dquant[i].luma_dc_qi[j] = picture_info->dquant[i].luma_dc_qi[j];
           picture.dquant[i].chroma_qi[j] = picture_info->dquant[i].chroma_qi[j];
       }
   }
   picture.refresh_golden = picture_info->refresh_golden;
   picture.refresh_altref = picture_info->refresh_altref;
   picture.sign_bias_flag[0] = picture_info->sign_bias_flag[0];
   picture.sign_bias_flag[1] = picture_info->sign_bias_flag[1];
   picture.refresh_probabilities = picture_info->refresh_probabilities;
   picture.refresh_last = picture_info->refresh_last;
   for (i = 0; i < 4; i++)
      for (j = 0; j < 8; j++)
         for (k = 0; k < 3; k++)
            for (l = 0; l < 11; l++)
               picture.token_prob[i][j][k][l] = picture_info->token_prob[i][j][k][l];
   picture.mb_no_coeff_skip = picture_info->mb_no_coeff_skip;
   picture.prob_skip_false = picture_info->prob_skip_false;
   picture.prob_intra = picture_info->prob_intra;
   picture.prob_last = picture_info->prob_last;
   picture.prob_golden = picture_info->prob_golden;
   picture.intra_16x16_prob[4];
   picture.intra_chroma_prob[3];
   for (i = 0; i < 2; i++)
      for (j = 0; j < 19; j++)
         picture.mv_prob[i][j] = picture_info->mv_prob[i][j];

   decoder->set_picture_parameters(decoder, &picture.base);
/*
   memset(&quant, 0, sizeof(quant));
   quant.base.codec = PIPE_VIDEO_CODEC_VP8;
   quant.intra_matrix = picture_info->intra_quantizer_matrix;
   quant.non_intra_matrix = picture_info->non_intra_quantizer_matrix;

   decoder->set_quant_matrix(decoder, &quant.base);
*/
   decoder->begin_frame(decoder);

   for (i = 0; i < bitstream_buffer_count; ++i)
      decoder->decode_bitstream(decoder, bitstream_buffers[i].bitstream_bytes,
                                bitstream_buffers[i].bitstream);

   decoder->end_frame(decoder);

   return VDP_STATUS_OK;
}

VdpStatus
vlVdpDecoderRender(VdpDecoder decoder,
                   VdpVideoSurface target,
                   VdpPictureInfo const *picture_info,
                   uint32_t bitstream_buffer_count,
                   VdpBitstreamBuffer const *bitstream_buffers)
{
   vlVdpDecoder *vldecoder;
   vlVdpSurface *vlsurf;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Decoding\n");

   if (!(picture_info && bitstream_buffers))
      return VDP_STATUS_INVALID_POINTER;

   vldecoder = (vlVdpDecoder *)vlGetDataHTAB(decoder);
   if (!vldecoder)
      return VDP_STATUS_INVALID_HANDLE;

   vlsurf = (vlVdpSurface *)vlGetDataHTAB(target);
   if (!vlsurf)
      return VDP_STATUS_INVALID_HANDLE;

   if (vlsurf->device != vldecoder->device)
      return VDP_STATUS_HANDLE_DEVICE_MISMATCH;

   if (vlsurf->video_buffer->chroma_format != vldecoder->decoder->chroma_format)
      // TODO: Recreate decoder with correct chroma
      return VDP_STATUS_INVALID_CHROMA_TYPE;

   // TODO: Support more video formats !
   switch (vldecoder->decoder->profile) {
   case PIPE_VIDEO_PROFILE_MPEG1:
   case PIPE_VIDEO_PROFILE_MPEG2_SIMPLE:
   case PIPE_VIDEO_PROFILE_MPEG2_MAIN:
      ++vldecoder->cur_buffer;
      vldecoder->cur_buffer %= vldecoder->num_buffers;

      vldecoder->decoder->set_decode_buffer(vldecoder->decoder, vldecoder->buffers[vldecoder->cur_buffer]);
      vldecoder->decoder->set_decode_target(vldecoder->decoder, vlsurf->video_buffer);

      return vlVdpDecoderRenderMpeg12(vldecoder->decoder, (VdpPictureInfoMPEG1Or2 *)picture_info,
                                      bitstream_buffer_count, bitstream_buffers);
      break;
   case PIPE_VIDEO_PROFILE_VP8_V0:
   case PIPE_VIDEO_PROFILE_VP8_V1:
   case PIPE_VIDEO_PROFILE_VP8_V2:
   case PIPE_VIDEO_PROFILE_VP8_V3:
      ++vldecoder->cur_buffer;
      vldecoder->cur_buffer %= vldecoder->num_buffers;

      vldecoder->decoder->set_decode_buffer(vldecoder->decoder, vldecoder->buffers[vldecoder->cur_buffer]);
      vldecoder->decoder->set_decode_target(vldecoder->decoder, vlsurf->video_buffer);

      return vlVdpDecoderRenderVP8(vldecoder->decoder, (VdpPictureInfoVP8 *)picture_info,
                                   bitstream_buffer_count, bitstream_buffers);
      break;

   default:
      return VDP_STATUS_INVALID_DECODER_PROFILE;
   }
}
