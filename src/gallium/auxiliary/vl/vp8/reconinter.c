/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "recon_dispatch.h"
#include "filter_dispatch.h"
#include "reconinter.h"
#include "vp8_mem.h"

static const int bbb[4] = {0, 2, 8, 10};

void vp8_copy_mem16x16_c(unsigned char *src, int src_stride,
                         unsigned char *dst, int dst_stride)
{
    int r;

    for (r = 0; r < 16; r++)
    {
#ifdef MUST_BE_ALIGNED
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        dst[4] = src[4];
        dst[5] = src[5];
        dst[6] = src[6];
        dst[7] = src[7];
        dst[8] = src[8];
        dst[9] = src[9];
        dst[10] = src[10];
        dst[11] = src[11];
        dst[12] = src[12];
        dst[13] = src[13];
        dst[14] = src[14];
        dst[15] = src[15];
#else
        ((int *)dst)[0] = ((int *)src)[0];
        ((int *)dst)[1] = ((int *)src)[1];
        ((int *)dst)[2] = ((int *)src)[2];
        ((int *)dst)[3] = ((int *)src)[3];
#endif /* MUST_BE_ALIGNED */
        src += src_stride;
        dst += dst_stride;
    }
}

void vp8_copy_mem8x8_c(unsigned char *src, int src_stride,
                       unsigned char *dst, int dst_stride)
{
    int r;

    for (r = 0; r < 8; r++)
    {
#ifdef MUST_BE_ALIGNED
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        dst[4] = src[4];
        dst[5] = src[5];
        dst[6] = src[6];
        dst[7] = src[7];
#else
        ((int *)dst)[0] = ((int *)src)[0];
        ((int *)dst)[1] = ((int *)src)[1];
#endif /* MUST_BE_ALIGNED */
        src += src_stride;
        dst += dst_stride;
    }
}

void vp8_copy_mem8x4_c(unsigned char *src, int src_stride,
                       unsigned char *dst, int dst_stride)
{
    int r;

    for (r = 0; r < 4; r++)
    {
#ifdef MUST_BE_ALIGNED
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        dst[4] = src[4];
        dst[5] = src[5];
        dst[6] = src[6];
        dst[7] = src[7];
#else
        ((int *)dst)[0] = ((int *)src)[0];
        ((int *)dst)[1] = ((int *)src)[1];
#endif /* MUST_BE_ALIGNED */
        src += src_stride;
        dst += dst_stride;
    }
}

void vp8_build_inter_predictors_b(BLOCKD *d, int pitch, vp8_filter_fn_t sppf)
{
    int r;
    unsigned char *ptr_base = *(d->base_pre);
    unsigned char *ptr;
    unsigned char *pred_ptr = d->predictor;

    if (d->bmi.mv.as_mv.row & 7 || d->bmi.mv.as_mv.col & 7)
    {
        ptr = ptr_base + d->pre + (d->bmi.mv.as_mv.row >> 3) * d->pre_stride + (d->bmi.mv.as_mv.col >> 3);
        sppf(ptr, d->pre_stride, d->bmi.mv.as_mv.col & 7, d->bmi.mv.as_mv.row & 7, pred_ptr, pitch);
    }
    else
    {
        ptr_base += d->pre + (d->bmi.mv.as_mv.row >> 3) * d->pre_stride + (d->bmi.mv.as_mv.col >> 3);
        ptr = ptr_base;

        for (r = 0; r < 4; r++)
        {
#ifdef MUST_BE_ALIGNED
            pred_ptr[0] = ptr[0];
            pred_ptr[1] = ptr[1];
            pred_ptr[2] = ptr[2];
            pred_ptr[3] = ptr[3];
#else
            *(int *)pred_ptr = *(int *)ptr;
#endif /* MUST_BE_ALIGNED */
            pred_ptr += pitch;
            ptr      += d->pre_stride;
        }
    }
}

static void build_inter_predictors4b(MACROBLOCKD *mb, BLOCKD *d, int pitch)
{
    unsigned char *ptr_base = *(d->base_pre);
    unsigned char *ptr = ptr_base + d->pre + (d->bmi.mv.as_mv.row >> 3) * d->pre_stride + (d->bmi.mv.as_mv.col >> 3);
    unsigned char *pred_ptr = d->predictor;

    if (d->bmi.mv.as_mv.row & 7 || d->bmi.mv.as_mv.col & 7)
    {
        mb->filter_predict8x8(ptr, d->pre_stride, d->bmi.mv.as_mv.col & 7, d->bmi.mv.as_mv.row & 7, pred_ptr, pitch);
    }
    else
    {
        RECON_INVOKE(&mb->rtcd->recon, copy8x8)(ptr, d->pre_stride, pred_ptr, pitch);
    }
}

static void build_inter_predictors2b(MACROBLOCKD *mb, BLOCKD *d, int pitch)
{
    unsigned char *ptr_base = *(d->base_pre);
    unsigned char *ptr = ptr_base + d->pre + (d->bmi.mv.as_mv.row >> 3) * d->pre_stride + (d->bmi.mv.as_mv.col >> 3);
    unsigned char *pred_ptr = d->predictor;

    if (d->bmi.mv.as_mv.row & 7 || d->bmi.mv.as_mv.col & 7)
    {
        mb->filter_predict8x4(ptr, d->pre_stride, d->bmi.mv.as_mv.col & 7, d->bmi.mv.as_mv.row & 7, pred_ptr, pitch);
    }
    else
    {
        RECON_INVOKE(&mb->rtcd->recon, copy8x4)(ptr, d->pre_stride, pred_ptr, pitch);
    }
}

void vp8_build_inter16x16_predictors_mb(MACROBLOCKD *mb,
                                        unsigned char *dst_y,
                                        unsigned char *dst_u,
                                        unsigned char *dst_v,
                                        int dst_ystride,
                                        int dst_uvstride)
{
    int offset;

    int mv_row = mb->mode_info_context->mbmi.mv.as_mv.row;
    int mv_col = mb->mode_info_context->mbmi.mv.as_mv.col;

    unsigned char *ptr_base = mb->pre.y_buffer;
    int pre_stride = mb->block[0].pre_stride;

    unsigned char *ptr = ptr_base + (mv_row >> 3) * pre_stride + (mv_col >> 3);
    unsigned char *uptr, *vptr;

    if ((mv_row | mv_col) & 7)
    {
        mb->filter_predict16x16(ptr, pre_stride, mv_col & 7, mv_row & 7, dst_y, dst_ystride);
    }
    else
    {
        RECON_INVOKE(&mb->rtcd->recon, copy16x16)(ptr, pre_stride, dst_y, dst_ystride);
    }

    mv_row = mb->block[16].bmi.mv.as_mv.row;
    mv_col = mb->block[16].bmi.mv.as_mv.col;
    pre_stride >>= 1;
    offset = (mv_row >> 3) * pre_stride + (mv_col >> 3);
    uptr = mb->pre.u_buffer + offset;
    vptr = mb->pre.v_buffer + offset;

    if ((mv_row | mv_col) & 7)
    {
        mb->filter_predict8x8(uptr, pre_stride, mv_col & 7, mv_row & 7, dst_u, dst_uvstride);
        mb->filter_predict8x8(vptr, pre_stride, mv_col & 7, mv_row & 7, dst_v, dst_uvstride);
    }
    else
    {
        RECON_INVOKE(&mb->rtcd->recon, copy8x8)(uptr, pre_stride, dst_u, dst_uvstride);
        RECON_INVOKE(&mb->rtcd->recon, copy8x8)(vptr, pre_stride, dst_v, dst_uvstride);
    }
}

void vp8_build_inter4x4_predictors_mb(MACROBLOCKD *mb)
{
    int i;

    if (mb->mode_info_context->mbmi.partitioning < 3)
    {
        for (i = 0; i < 4; i++)
        {
            BLOCKD *d = &mb->block[bbb[i]];
            build_inter_predictors4b(mb, d, 16);
        }
    }
    else
    {
        for (i = 0; i < 16; i += 2)
        {
            BLOCKD *d0 = &mb->block[i];
            BLOCKD *d1 = &mb->block[i+1];

            if (d0->bmi.mv.as_int == d1->bmi.mv.as_int)
                build_inter_predictors2b(mb, d0, 16);
            else
            {
                vp8_build_inter_predictors_b(d0, 16, mb->filter_predict4x4);
                vp8_build_inter_predictors_b(d1, 16, mb->filter_predict4x4);
            }
        }
    }

    for (i = 16; i < 24; i += 2)
    {
        BLOCKD *d0 = &mb->block[i];
        BLOCKD *d1 = &mb->block[i+1];

        if (d0->bmi.mv.as_int == d1->bmi.mv.as_int)
            build_inter_predictors2b(mb, d0, 8);
        else
        {
            vp8_build_inter_predictors_b(d0, 8, mb->filter_predict4x4);
            vp8_build_inter_predictors_b(d1, 8, mb->filter_predict4x4);
        }
    }
}

void vp8_build_inter_predictors_mb(MACROBLOCKD *mb)
{
    if (mb->mode_info_context->mbmi.mode != SPLITMV)
    {
        vp8_build_inter16x16_predictors_mb(mb, mb->predictor, &mb->predictor[256],
                                           &mb->predictor[320], 16, 8);
    }
    else
    {
        vp8_build_inter4x4_predictors_mb(mb);
    }
}

void vp8_build_uvmvs(MACROBLOCKD *mb, int fullpixel)
{
    int i, j;

    if (mb->mode_info_context->mbmi.mode == SPLITMV)
    {
        for (i = 0; i < 2; i++)
        {
            for (j = 0; j < 2; j++)
            {
                int yoffset = i * 8 + j * 2;
                int uoffset = 16 + i * 2 + j;
                int voffset = 20 + i * 2 + j;

                int temp = mb->block[yoffset].bmi.mv.as_mv.row
                           + mb->block[yoffset+1].bmi.mv.as_mv.row
                           + mb->block[yoffset+4].bmi.mv.as_mv.row
                           + mb->block[yoffset+5].bmi.mv.as_mv.row;

                if (temp < 0)
                    temp -= 4;
                else
                    temp += 4;

                mb->block[uoffset].bmi.mv.as_mv.row = temp / 8;

                if (fullpixel)
                    mb->block[uoffset].bmi.mv.as_mv.row = (temp / 8) & 0xfffffff8;

                temp = mb->block[yoffset].bmi.mv.as_mv.col
                       + mb->block[yoffset+1].bmi.mv.as_mv.col
                       + mb->block[yoffset+4].bmi.mv.as_mv.col
                       + mb->block[yoffset+5].bmi.mv.as_mv.col;

                if (temp < 0)
                    temp -= 4;
                else
                    temp += 4;

                mb->block[uoffset].bmi.mv.as_mv.col = temp / 8;

                if (fullpixel)
                    mb->block[uoffset].bmi.mv.as_mv.col = (temp / 8) & 0xfffffff8;

                mb->block[voffset].bmi.mv.as_mv.row = mb->block[uoffset].bmi.mv.as_mv.row;
                mb->block[voffset].bmi.mv.as_mv.col = mb->block[uoffset].bmi.mv.as_mv.col;
            }
        }
    }
    else
    {
        int mvrow = mb->mode_info_context->mbmi.mv.as_mv.row;
        int mvcol = mb->mode_info_context->mbmi.mv.as_mv.col;

        if (mvrow < 0)
            mvrow -= 1;
        else
            mvrow += 1;

        if (mvcol < 0)
            mvcol -= 1;
        else
            mvcol += 1;

        mvrow /= 2;
        mvcol /= 2;

        for (i = 0; i < 8; i++)
        {
            mb->block[16 + i].bmi.mv.as_mv.row = mvrow;
            mb->block[16 + i].bmi.mv.as_mv.col = mvcol;

            if (fullpixel)
            {
                mb->block[16 + i].bmi.mv.as_mv.row = mvrow & 0xfffffff8;
                mb->block[16 + i].bmi.mv.as_mv.col = mvcol & 0xfffffff8;
            }
        }
    }
}
