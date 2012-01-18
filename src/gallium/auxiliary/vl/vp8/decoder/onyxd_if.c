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

#include "../vp8_mem.h"

#include "../common/onyxc_int.h"
#include "onyxd_int.h"
#include "../common/onyxd.h"
#include "../common/alloccommon.h"
#include "../common/yv12utils.h"

#include "../common/quant_common.h"
#include "detokenize.h"

static int get_free_fb(VP8_COMMON *cm)
{
    int i;
    for (i = 0; i < NUM_YV12_BUFFERS; i++)
        if (cm->fb_idx_ref_cnt[i] == 0)
            break;

    assert(i < NUM_YV12_BUFFERS);
    cm->fb_idx_ref_cnt[i] = 1;

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
static int swap_frame_buffers(VP8_COMMON *cm)
{
    int err = 0;

    /* The alternate reference frame or golden frame can be updated
     *  using the new, last, or golden/alt ref frame.  If it
     *  is updated using the newly decoded frame it is a refresh.
     *  An update using the last or golden/alt ref frame is a copy. */
    if (cm->copy_buffer_to_arf)
    {
        int new_fb = 0;

        if (cm->copy_buffer_to_arf == 1)
            new_fb = cm->lst_fb_idx;
        else if (cm->copy_buffer_to_arf == 2)
            new_fb = cm->gld_fb_idx;
        else
            err = -1;

        ref_cnt_fb(cm->fb_idx_ref_cnt, &cm->alt_fb_idx, new_fb);
    }

    if (cm->copy_buffer_to_gf)
    {
        int new_fb = 0;

        if (cm->copy_buffer_to_gf == 1)
            new_fb = cm->lst_fb_idx;
        else if (cm->copy_buffer_to_gf == 2)
            new_fb = cm->alt_fb_idx;
        else
            err = -1;

        ref_cnt_fb(cm->fb_idx_ref_cnt, &cm->gld_fb_idx, new_fb);
    }

    if (cm->refresh_golden_frame)
        ref_cnt_fb(cm->fb_idx_ref_cnt, &cm->gld_fb_idx, cm->new_fb_idx);

    if (cm->refresh_alt_ref_frame)
        ref_cnt_fb(cm->fb_idx_ref_cnt, &cm->alt_fb_idx, cm->new_fb_idx);

    if (cm->refresh_last_frame)
    {
        ref_cnt_fb(cm->fb_idx_ref_cnt, &cm->lst_fb_idx, cm->new_fb_idx);

        cm->frame_to_show = &cm->yv12_fb[cm->lst_fb_idx];
    }
    else
        cm->frame_to_show = &cm->yv12_fb[cm->new_fb_idx];

    cm->fb_idx_ref_cnt[cm->new_fb_idx]--;

    return err;
}

/**
 * Create a VP8 decoder instance.
 */
VP8D_PTR vp8_decoder_create()
{
    VP8D_COMP *pbi = vpx_memalign(32, sizeof(VP8D_COMP));

    if (!pbi)
        return NULL;

    memset(pbi, 0, sizeof(VP8D_COMP));

    if (setjmp(pbi->common.error.jmp))
    {
        pbi->common.error.setjmp = 0;
        vp8_decoder_remove(pbi);
        return 0;
    }

    pbi->common.error.setjmp = 1;
    vp8_initialize_common();

    vp8_create_common(&pbi->common);

    pbi->common.current_video_frame = 0;
    pbi->common.show_frame = 0;
    pbi->ready_for_new_data = 1;

    vp8_initialize_dequantizer(&pbi->common);

    // vp8_loop_filter_init(&pbi->common);

    pbi->common.error.setjmp = 0;

    return (VP8D_PTR)pbi;
}

/**
 * Decode one VP8 frame.
 */
int vp8_decoder_start(VP8D_PTR ptr, struct pipe_vp8_picture_desc *frame_header,
                      const unsigned char *data, unsigned data_size,
                      int64_t timestamp_deadline)
{
    VP8D_COMP *pbi = (VP8D_COMP *)ptr;
    VP8_COMMON *cm = &pbi->common;
    int retcode = 0;

    pbi->common.error.error_code = VPX_CODEC_OK;

    {
        pbi->data = data;
        pbi->data_size = data_size;

        cm->new_fb_idx = get_free_fb(cm);

        if (setjmp(pbi->common.error.jmp))
        {
            pbi->common.error.setjmp = 0;

           /* We do not know if the missing frame(s) was supposed to update
            * any of the reference buffers, but we act conservative and
            * mark only the last buffer as corrupted. */
            cm->yv12_fb[cm->lst_fb_idx].corrupted = 1;

            if (cm->fb_idx_ref_cnt[cm->new_fb_idx] > 0)
                cm->fb_idx_ref_cnt[cm->new_fb_idx]--;

            return -1;
        }

        pbi->common.error.setjmp = 1;
    }

    retcode = vp8_frame_decode(pbi, frame_header);

    if (retcode < 0)
    {
        pbi->common.error.error_code = VPX_CODEC_ERROR;
        pbi->common.error.setjmp = 0;
        if (cm->fb_idx_ref_cnt[cm->new_fb_idx] > 0)
            cm->fb_idx_ref_cnt[cm->new_fb_idx]--;

        return retcode;
    }

    {
        if (swap_frame_buffers(cm))
        {
            pbi->common.error.error_code = VPX_CODEC_ERROR;
            pbi->common.error.setjmp = 0;

            return -1;
        }
/*
        if (cm->filter_level)
        {
            // Apply the loop filter if appropriate.
            vp8_loop_filter_frame(cm, &pbi->mb);
        }
*/
        vp8_yv12_extend_frame_borders(cm->frame_to_show);
    }

    /* from libvpx : vp8_print_modes_and_motion_vectors(cm->mi, cm->mb_rows, cm->mb_cols, cm->current_video_frame); */

    if (cm->show_frame)
        cm->current_video_frame++;

    pbi->ready_for_new_data = 0;
    pbi->last_time_stamp = timestamp_deadline;
    pbi->data_size = 0;
    pbi->common.error.setjmp = 0;

    return retcode;
}

/**
 * Return a decoded VP8 frame in a YV12 framebuffer.
 */
int vp8_decoder_getframe(VP8D_PTR ptr,
                         YV12_BUFFER_CONFIG *sd,
                         int64_t *timestamp,
                         int64_t *timestamp_end)
{
    int ret = -1;
    VP8D_COMP *pbi = (VP8D_COMP *)ptr;

    if (pbi->ready_for_new_data == 1)
        return ret;

    /* ie no raw frame to show!!! */
    if (pbi->common.show_frame == 0)
        return ret;

    pbi->ready_for_new_data = 1;
    *timestamp = pbi->last_time_stamp;
    *timestamp_end = 0;

    if (pbi->common.frame_to_show)
    {
        *sd = *pbi->common.frame_to_show;
        sd->clrtype = pbi->common.clr_type;
        sd->y_width = pbi->common.width;
        sd->y_height = pbi->common.height;
        sd->uv_height = pbi->common.height / 2;

        ret = 0;
    }

    return ret;
}

/**
 * Destroy a VP8 decoder instance.
 */
void vp8_decoder_remove(VP8D_PTR ptr)
{
    VP8D_COMP *pbi = (VP8D_COMP *)ptr;

    if (!pbi)
        return;

    vp8_remove_common(&pbi->common);
    vpx_free(pbi->mbd);
    vpx_free(pbi);
}
