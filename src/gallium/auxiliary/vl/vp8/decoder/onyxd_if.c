/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "../common/onyxc_int.h"
#include "onyxd_int.h"
#include "../vpx_mem.h"
#include "../common/onyxd.h"
#include "../common/alloccommon.h"
#include "../common/loopfilter.h"
#include "../common/yv12utils.h"

#include <stdio.h>
#include <assert.h>

#include "../common/quant_common.h"
#include "detokenize.h"

extern void vp8cx_init_de_quantizer(VP8D_COMP *pbi);
static int get_free_fb (VP8_COMMON *cm);
static void ref_cnt_fb (int *buf, int *idx, int new_idx);


void vp8dx_initialize()
{
    static int init_done = 0;

    if (!init_done)
    {
        vp8_initialize_common();
        init_done = 1;
    }
}


VP8D_PTR vp8dx_create_decompressor(VP8D_CONFIG *oxcf)
{
    VP8D_COMP *pbi = vpx_memalign(32, sizeof(VP8D_COMP));

    printf("[VP8] vp8dx_create_decompressor()\n");

    if (!pbi)
        return NULL;

    memset(pbi, 0, sizeof(VP8D_COMP));

    if (setjmp(pbi->common.error.jmp))
    {
        pbi->common.error.setjmp = 0;
        vp8dx_remove_decompressor(pbi);
        return 0;
    }

    pbi->common.error.setjmp = 1;
    vp8dx_initialize();

    vp8_create_common(&pbi->common);

    pbi->common.current_video_frame = 0;
    pbi->ready_for_new_data = 1;

    /* vp8cx_init_de_quantizer() is first called here. Add check in
     * frame_init_dequantizer() to avoid unnecessary calling of
     * vp8cx_init_de_quantizer() for every frame. */
    vp8cx_init_de_quantizer(pbi);

    // vp8_loop_filter_init(&pbi->common);

    pbi->common.error.setjmp = 0;
    pbi->input_partition = oxcf->input_partition;

    return (VP8D_PTR) pbi;
}


void vp8dx_remove_decompressor(VP8D_PTR ptr)
{
    VP8D_COMP *pbi = (VP8D_COMP *) ptr;

    printf("[VP8] vp8dx_remove_decompressor()\n");

    if (!pbi)
        return;

    vp8_remove_common(&pbi->common);
    vpx_free(pbi->mbc);
    vpx_free(pbi);
}


vpx_codec_err_t vp8dx_get_reference(VP8D_PTR ptr,
                                    VP8_REF_FRAME ref_frame_flag,
                                    YV12_BUFFER_CONFIG *sd)
{
    VP8D_COMP *pbi = (VP8D_COMP *) ptr;
    VP8_COMMON *cm = &pbi->common;
    int ref_fb_idx;

    if (ref_frame_flag == VP8_LAST_FRAME)
        ref_fb_idx = cm->lst_fb_idx;
    else if (ref_frame_flag == VP8_GOLD_FRAME)
        ref_fb_idx = cm->gld_fb_idx;
    else if (ref_frame_flag == VP8_ALTR_FRAME)
        ref_fb_idx = cm->alt_fb_idx;
    else {
        vpx_internal_error(&pbi->common.error, VPX_CODEC_ERROR, "Invalid reference frame");
        return pbi->common.error.error_code;
    }

    if (cm->yv12_fb[ref_fb_idx].y_height != sd->y_height ||
        cm->yv12_fb[ref_fb_idx].y_width != sd->y_width ||
        cm->yv12_fb[ref_fb_idx].uv_height != sd->uv_height ||
        cm->yv12_fb[ref_fb_idx].uv_width != sd->uv_width){
        vpx_internal_error(&pbi->common.error, VPX_CODEC_ERROR, "Incorrect buffer dimensions");
    }
    else
        vp8_yv12_copy_frame(&cm->yv12_fb[ref_fb_idx], sd);

    return pbi->common.error.error_code;
}


vpx_codec_err_t vp8dx_set_reference(VP8D_PTR ptr,
                                    VP8_REF_FRAME ref_frame_flag,
                                    YV12_BUFFER_CONFIG *sd)
{
    VP8D_COMP *pbi = (VP8D_COMP *) ptr;
    VP8_COMMON *cm = &pbi->common;
    int *ref_fb_ptr = NULL;
    int free_fb;

    if (ref_frame_flag == VP8_LAST_FRAME)
        ref_fb_ptr = &cm->lst_fb_idx;
    else if (ref_frame_flag == VP8_GOLD_FRAME)
        ref_fb_ptr = &cm->gld_fb_idx;
    else if (ref_frame_flag == VP8_ALTR_FRAME)
        ref_fb_ptr = &cm->alt_fb_idx;
    else {
        vpx_internal_error(&pbi->common.error, VPX_CODEC_ERROR,
            "Invalid reference frame");
        return pbi->common.error.error_code;
    }

    if (cm->yv12_fb[*ref_fb_ptr].y_height != sd->y_height ||
        cm->yv12_fb[*ref_fb_ptr].y_width != sd->y_width ||
        cm->yv12_fb[*ref_fb_ptr].uv_height != sd->uv_height ||
        cm->yv12_fb[*ref_fb_ptr].uv_width != sd->uv_width){
        vpx_internal_error(&pbi->common.error, VPX_CODEC_ERROR,
            "Incorrect buffer dimensions");
    } else {
        /* Find an empty frame buffer. */
        free_fb = get_free_fb(cm);
        /* Decrease fb_idx_ref_cnt since it will be increased again in
         * ref_cnt_fb() below. */
        cm->fb_idx_ref_cnt[free_fb]--;

        /* Manage the reference counters and copy image. */
        ref_cnt_fb (cm->fb_idx_ref_cnt, ref_fb_ptr, free_fb);
        vp8_yv12_copy_frame(sd, &cm->yv12_fb[*ref_fb_ptr]);
    }

   return pbi->common.error.error_code;
}

static int get_free_fb (VP8_COMMON *cm)
{
    int i;
    for (i = 0; i < NUM_YV12_BUFFERS; i++)
        if (cm->fb_idx_ref_cnt[i] == 0)
            break;

    assert(i < NUM_YV12_BUFFERS);
    cm->fb_idx_ref_cnt[i] = 1;
    return i;
}

static void ref_cnt_fb (int *buf, int *idx, int new_idx)
{
    if (buf[*idx] > 0)
        buf[*idx]--;

    *idx = new_idx;

    buf[new_idx]++;
}

/* If any buffer copy / swapping is signalled it should be done here. */
static int swap_frame_buffers (VP8_COMMON *cm)
{
    int err = 0;

    /* The alternate reference frame or golden frame can be updated
     *  using the new, last, or golden/alt ref frame.  If it
     *  is updated using the newly decoded frame it is a refresh.
     *  An update using the last or golden/alt ref frame is a copy.
     */
    if (cm->copy_buffer_to_arf)
    {
        int new_fb = 0;

        if (cm->copy_buffer_to_arf == 1)
            new_fb = cm->lst_fb_idx;
        else if (cm->copy_buffer_to_arf == 2)
            new_fb = cm->gld_fb_idx;
        else
            err = -1;

        ref_cnt_fb (cm->fb_idx_ref_cnt, &cm->alt_fb_idx, new_fb);
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

        ref_cnt_fb (cm->fb_idx_ref_cnt, &cm->gld_fb_idx, new_fb);
    }

    if (cm->refresh_golden_frame)
        ref_cnt_fb (cm->fb_idx_ref_cnt, &cm->gld_fb_idx, cm->new_fb_idx);

    if (cm->refresh_alt_ref_frame)
        ref_cnt_fb (cm->fb_idx_ref_cnt, &cm->alt_fb_idx, cm->new_fb_idx);

    if (cm->refresh_last_frame)
    {
        ref_cnt_fb (cm->fb_idx_ref_cnt, &cm->lst_fb_idx, cm->new_fb_idx);

        cm->frame_to_show = &cm->yv12_fb[cm->lst_fb_idx];
    }
    else
        cm->frame_to_show = &cm->yv12_fb[cm->new_fb_idx];

    cm->fb_idx_ref_cnt[cm->new_fb_idx]--;

    return err;
}

int vp8dx_receive_compressed_data(VP8D_PTR ptr, unsigned long size, const unsigned char *source, int64_t time_stamp)
{
    VP8D_COMP *pbi = (VP8D_COMP *) ptr;
    VP8_COMMON *cm = &pbi->common;
    int retcode = 0;

    /*if(pbi->ready_for_new_data == 0)
        return -1;*/

    if (ptr == 0)
    {
        return -1;
    }

    pbi->common.error.error_code = VPX_CODEC_OK;

    if (pbi->input_partition && !(source == NULL && size == 0))
    {
        /* Store a pointer to this partition and return. We haven't
         * received the complete frame yet, so we will wait with decoding.
         */
        pbi->partitions[pbi->num_partitions] = source;
        pbi->partition_sizes[pbi->num_partitions] = size;
        pbi->source_sz += size;
        pbi->num_partitions++;
        if (pbi->num_partitions > (1<<pbi->common.multi_token_partition) + 1)
            pbi->common.multi_token_partition++;
        if (pbi->common.multi_token_partition > EIGHT_PARTITION)
        {
            pbi->common.error.error_code = VPX_CODEC_UNSUP_BITSTREAM;
            pbi->common.error.setjmp = 0;
            return -1;
        }

        return 0;
    }
    else
    {
        if (!pbi->input_partition)
        {
            pbi->Source = source;
            pbi->source_sz = size;
        }

        if (pbi->source_sz == 0)
        {
           /* This is used to signal that we are missing frames.
            * We do not know if the missing frame(s) was supposed to update
            * any of the reference buffers, but we act conservative and
            * mark only the last buffer as corrupted.
            */
            cm->yv12_fb[cm->lst_fb_idx].corrupted = 1;

            /* If error concealment is disabled we won't signal missing frames to
             * the decoder.
             */
            {
                /* Signal that we have no frame to show. */
                cm->show_frame = 0;
                return 0;
            }
        }

        cm->new_fb_idx = get_free_fb(cm);

        if (setjmp(pbi->common.error.jmp))
        {
            pbi->common.error.setjmp = 0;

           /* We do not know if the missing frame(s) was supposed to update
            * any of the reference buffers, but we act conservative and
            * mark only the last buffer as corrupted.
            */
            cm->yv12_fb[cm->lst_fb_idx].corrupted = 1;

            if (cm->fb_idx_ref_cnt[cm->new_fb_idx] > 0)
                cm->fb_idx_ref_cnt[cm->new_fb_idx]--;

            return -1;
        }

        pbi->common.error.setjmp = 1;
    }

    retcode = vp8_decode_frame(pbi);

    if (retcode < 0)
    {
        pbi->common.error.error_code = VPX_CODEC_ERROR;
        pbi->common.error.setjmp = 0;
        if (cm->fb_idx_ref_cnt[cm->new_fb_idx] > 0)
            cm->fb_idx_ref_cnt[cm->new_fb_idx]--;

        return retcode;
    }

    {
        if (swap_frame_buffers (cm))
        {
            pbi->common.error.error_code = VPX_CODEC_ERROR;
            pbi->common.error.setjmp = 0;

            return -1;
        }

        if (cm->filter_level)
        {
            /* Apply the loop filter if appropriate. */
            // vp8_loop_filter_frame(cm, &pbi->mb);
        }

        vp8_yv12_extend_frame_borders(cm->frame_to_show);
    }

    /* from libvpx : vp8_print_modes_and_motion_vectors(cm->mi, cm->mb_rows, cm->mb_cols, cm->current_video_frame); */

    if (cm->show_frame)
        cm->current_video_frame++;

    pbi->ready_for_new_data = 0;
    pbi->last_time_stamp = time_stamp;
    pbi->num_partitions = 0;
    if (pbi->input_partition)
        pbi->common.multi_token_partition = 0;
    pbi->source_sz = 0;
    pbi->common.error.setjmp = 0;

    return retcode;
}

int vp8dx_get_raw_frame(VP8D_PTR ptr, YV12_BUFFER_CONFIG *sd, int64_t *time_stamp, int64_t *time_end_stamp)
{
    int ret = -1;
    VP8D_COMP *pbi = (VP8D_COMP *) ptr;

    if (pbi->ready_for_new_data == 1)
        return ret;

    /* ie no raw frame to show!!! */
    if (pbi->common.show_frame == 0)
        return ret;

    pbi->ready_for_new_data = 1;
    *time_stamp = pbi->last_time_stamp;
    *time_end_stamp = 0;

    sd->clrtype = pbi->common.clr_type;

    if (pbi->common.frame_to_show)
    {
        *sd = *pbi->common.frame_to_show;
        sd->y_width = pbi->common.Width;
        sd->y_height = pbi->common.Height;
        sd->uv_height = pbi->common.Height / 2;
        ret = 0;
    }
    else
    {
        ret = -1;
    }

    return ret;
}
