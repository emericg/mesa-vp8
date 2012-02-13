/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include <stdio.h>
#include <assert.h>

#include "vp8_mem.h"
#include "vp8_decoder.h"

#include "alloccommon.h"
#include "dequantize_common.h"
#include "detokenize.h"

static int get_free_fb(VP8_COMMON *common)
{
    int i;
    for (i = 0; i < NUM_YV12_BUFFERS; i++)
        if (common->fb_idx_ref_cnt[i] == 0)
            break;

    assert(i < NUM_YV12_BUFFERS);
    common->fb_idx_ref_cnt[i] = 1;

    return i;
}

static void ref_cnt_fb(int *buf, int *idx, int new_idx)
{
    if (buf[*idx] > 0)
        buf[*idx]--;

    *idx = new_idx;

    buf[new_idx]++;
}

/**
 * If any buffer copy / swapping is signalled it should be done here.
 */
static int swap_frame_buffers(VP8_COMMON *common)
{
    int err = 0;

    /* The alternate reference frame or golden frame can be updated using the
     * new, last, or golden/alt ref frame. If it is updated using the newly
     * decoded frame it is a refresh. An update using the last or golden/alt
     * ref frame is a copy. */
    if (common->copy_buffer_to_alternate)
    {
        int new_fb = 0;

        if (common->copy_buffer_to_alternate == 1)
            new_fb = common->lst_fb_idx;
        else if (common->copy_buffer_to_alternate == 2)
            new_fb = common->gld_fb_idx;
        else
            err = -1;

        ref_cnt_fb(common->fb_idx_ref_cnt, &common->alt_fb_idx, new_fb);
    }

    if (common->copy_buffer_to_golden)
    {
        int new_fb = 0;

        if (common->copy_buffer_to_golden == 1)
            new_fb = common->lst_fb_idx;
        else if (common->copy_buffer_to_golden == 2)
            new_fb = common->alt_fb_idx;
        else
            err = -1;

        ref_cnt_fb(common->fb_idx_ref_cnt, &common->gld_fb_idx, new_fb);
    }

    if (common->refresh_golden_frame)
        ref_cnt_fb(common->fb_idx_ref_cnt, &common->gld_fb_idx, common->new_fb_idx);

    if (common->refresh_alternate_frame)
        ref_cnt_fb(common->fb_idx_ref_cnt, &common->alt_fb_idx, common->new_fb_idx);

    if (common->refresh_last_frame)
    {
        ref_cnt_fb(common->fb_idx_ref_cnt, &common->lst_fb_idx, common->new_fb_idx);

        common->frame_to_show = &common->yv12_fb[common->lst_fb_idx];
    }
    else
        common->frame_to_show = &common->yv12_fb[common->new_fb_idx];

    common->fb_idx_ref_cnt[common->new_fb_idx]--;

    return err;
}

/**
 * Create a VP8 decoder instance.
 */
VP8_COMMON *vp8_decoder_create()
{
    VP8_COMMON *common = vpx_memalign(32, sizeof(VP8_COMMON));

    if (!common)
        return NULL;

    memset(common, 0, sizeof(VP8_COMMON));

    if (setjmp(common->error.jmp))
    {
        common->error.setjmp = 0;
        vp8_decoder_remove(common);
        return 0;
    }

    common->error.setjmp = 1;
    common->show_frame = 0;

    vp8_initialize_common(common);
    vp8_initialize_dequantizer(common);
    //vp8_initialize_loopfilter(common);

    common->error.setjmp = 0;

    return common;
}

/**
 * Decode one VP8 frame.
 */
int vp8_decoder_start(VP8_COMMON *common,
                      struct pipe_vp8_picture_desc *frame_header,
                      const unsigned char *data, unsigned data_size)
{
    int retcode = 0;

    common->error.error_code = VPX_CODEC_OK;

    {
        common->data = data;
        common->data_size = data_size;

        common->new_fb_idx = get_free_fb(common);

        if (setjmp(common->error.jmp))
        {
            common->error.setjmp = 0;

           /* We do not know if the missing frame(s) was supposed to update
            * any of the reference buffers, but we act conservative and
            * mark only the last buffer as corrupted. */
            common->yv12_fb[common->lst_fb_idx].corrupted = 1;

            if (common->fb_idx_ref_cnt[common->new_fb_idx] > 0)
                common->fb_idx_ref_cnt[common->new_fb_idx]--;

            return -1;
        }

        common->error.setjmp = 1;
    }

    retcode = vp8_frame_decode(common, frame_header);

    if (retcode < 0)
    {
        common->error.error_code = VPX_CODEC_ERROR;
        common->error.setjmp = 0;
        if (common->fb_idx_ref_cnt[common->new_fb_idx] > 0)
            common->fb_idx_ref_cnt[common->new_fb_idx]--;

        return retcode;
    }

    {
        if (swap_frame_buffers(common))
        {
            common->error.error_code = VPX_CODEC_ERROR;
            common->error.setjmp = 0;

            return -1;
        }
/*
        if (cm->filter_level)
        {
            // Apply the loop filter if appropriate.
            vp8_loop_filter_frame(common, &mb);
        }
*/
        vp8_yv12_extend_frame_borders(common->frame_to_show);
    }

    /* from libvpx : vp8_print_modes_and_motion_vectors(cm->mi, cm->mb_rows, cm->mb_cols, current_video_frame); */

    common->data_size = 0;
    common->error.setjmp = 0;

    return retcode;
}

/**
 * Return a decoded VP8 frame in a YV12 framebuffer.
 */
int vp8_decoder_get_frame_decoded(VP8_COMMON *common, YV12_BUFFER_CONFIG *sd)
{
    int ret = -1;

    /* ie no raw frame to show!!! */
    if (common->show_frame == 0)
        return ret;

    if (common->frame_to_show)
    {
        *sd = *common->frame_to_show;
        sd->clrtype = common->color_space;
        sd->y_width = common->width;
        sd->y_height = common->height;
        sd->uv_height = common->height / 2;

        ret = 0;
    }

    return ret;
}

/**
 * Destroy a VP8 decoder instance.
 */
void vp8_decoder_remove(VP8_COMMON *common)
{
    if (!common)
        return;

    vp8_dealloc_frame_buffers(common);
    vpx_free(common->mbd);
    vpx_free(common);
}
