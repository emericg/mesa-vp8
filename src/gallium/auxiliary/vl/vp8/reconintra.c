/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "reconintra.h"
#include "vp8_mem.h"

/**
 * Set up frame new frame for intra coded blocks.
 */
void vp8_setup_intra_recon(YV12_BUFFER_CONFIG *ybf)
{
    int i;

    memset(ybf->y_buffer - 1 - ybf->y_stride, 127, ybf->y_width + 5);
    for (i = 0; i < ybf->y_height; i++)
        ybf->y_buffer[ybf->y_stride*i - 1] = (unsigned char)129;

    memset(ybf->u_buffer - 1 - ybf->uv_stride, 127, ybf->uv_width + 5);
    for (i = 0; i < ybf->uv_height; i++)
        ybf->u_buffer[ybf->uv_stride*i - 1] = (unsigned char)129;

    memset(ybf->v_buffer - 1 - ybf->uv_stride, 127, ybf->uv_width + 5);
    for (i = 0; i < ybf->uv_height; i++)
        ybf->v_buffer[ybf->uv_stride*i - 1] = (unsigned char)129;
}

/**
 * For skip_recon_mb(), add vp8_build_intra_predictors_mby_s(MACROBLOCKD *mb)
 * and vp8_build_intra_predictors_mbuv_s(MACROBLOCKD *mb).
 */
void vp8_recon_intra_mbuv(const vp8_recon_rtcd_vtable_t *rtcd, MACROBLOCKD *mb)
{
    int i;
    for (i = 16; i < 24; i += 2)
    {
        BLOCKD *b = &mb->block[i];
        RECON_INVOKE(rtcd, recon2)(b->predictor, b->diff, *(b->base_dst) + b->dst, b->dst_stride);
    }
}

void vp8_build_intra_predictors_mby(MACROBLOCKD *mb)
{
    int r, c, i;
    unsigned char *yabove_row = mb->dst.y_buffer - mb->dst.y_stride;
    unsigned char yleft_col[16];
    unsigned char ytop_left = yabove_row[-1];
    unsigned char *ypred_ptr = mb->predictor;

    for (i = 0; i < 16; i++)
    {
        yleft_col[i] = mb->dst.y_buffer [i* mb->dst.y_stride -1];
    }

    /* for Y */
    switch (mb->mode_info_context->mbmi.mode)
    {
    case DC_PRED:
    {
        int expected_dc;
        int i;
        int shift;
        int average = 0;

        if (mb->up_available || mb->left_available)
        {
            if (mb->up_available)
            {
                for (i = 0; i < 16; i++)
                {
                    average += yabove_row[i];
                }
            }

            if (mb->left_available)
            {

                for (i = 0; i < 16; i++)
                {
                    average += yleft_col[i];
                }
            }

            shift = 3 + mb->up_available + mb->left_available;
            expected_dc = (average + (1 << (shift - 1))) >> shift;
        }
        else
        {
            expected_dc = 128;
        }

        memset(ypred_ptr, expected_dc, 256);
    }
    break;

    case V_PRED:
    {
        for (r = 0; r < 16; r++)
        {
            ((int *)ypred_ptr)[0] = ((int *)yabove_row)[0];
            ((int *)ypred_ptr)[1] = ((int *)yabove_row)[1];
            ((int *)ypred_ptr)[2] = ((int *)yabove_row)[2];
            ((int *)ypred_ptr)[3] = ((int *)yabove_row)[3];
            ypred_ptr += 16;
        }
    }
    break;

    case H_PRED:
    {
        for (r = 0; r < 16; r++)
        {
            memset(ypred_ptr, yleft_col[r], 16);
            ypred_ptr += 16;
        }
    }
    break;

    case TM_PRED:
    {
        for (r = 0; r < 16; r++)
        {
            for (c = 0; c < 16; c++)
            {
                int pred =  yleft_col[r] + yabove_row[c] - ytop_left;

                if (pred < 0)
                    pred = 0;

                if (pred > 255)
                    pred = 255;

                ypred_ptr[c] = pred;
            }

            ypred_ptr += 16;
        }
    }
    break;

    case B_PRED:
    case NEARESTMV:
    case NEARMV:
    case ZEROMV:
    case NEWMV:
    case SPLITMV:
    case MB_MODE_COUNT:
        break;
    }
}

void vp8_build_intra_predictors_mby_s(MACROBLOCKD *mb)
{
    unsigned char *yabove_row = mb->dst.y_buffer - mb->dst.y_stride;
    unsigned char yleft_col[16];
    unsigned char ytop_left = yabove_row[-1];
    unsigned char *ypred_ptr = mb->predictor;
    int r, c, i;

    int y_stride = mb->dst.y_stride;
    ypred_ptr = mb->dst.y_buffer; /*mb->predictor;*/

    for (i = 0; i < 16; i++)
    {
        yleft_col[i] = mb->dst.y_buffer [i* mb->dst.y_stride -1];
    }

    /* for Y */
    switch (mb->mode_info_context->mbmi.mode)
    {
    case DC_PRED:
    {
        int expected_dc;
        int i;
        int shift;
        int average = 0;

        if (mb->up_available || mb->left_available)
        {
            if (mb->up_available)
            {
                for (i = 0; i < 16; i++)
                {
                    average += yabove_row[i];
                }
            }

            if (mb->left_available)
            {

                for (i = 0; i < 16; i++)
                {
                    average += yleft_col[i];
                }
            }

            shift = 3 + mb->up_available + mb->left_available;
            expected_dc = (average + (1 << (shift - 1))) >> shift;
        }
        else
        {
            expected_dc = 128;
        }

        /*memset(ypred_ptr, expected_dc, 256);*/
        for (r = 0; r < 16; r++)
        {
            memset(ypred_ptr, expected_dc, 16);
            ypred_ptr += y_stride; /*16;*/
        }
    }
    break;

    case V_PRED:
    {
        for (r = 0; r < 16; r++)
        {
            ((int *)ypred_ptr)[0] = ((int *)yabove_row)[0];
            ((int *)ypred_ptr)[1] = ((int *)yabove_row)[1];
            ((int *)ypred_ptr)[2] = ((int *)yabove_row)[2];
            ((int *)ypred_ptr)[3] = ((int *)yabove_row)[3];
            ypred_ptr += y_stride; /*16;*/
        }
    }
    break;

    case H_PRED:
    {
        for (r = 0; r < 16; r++)
        {
            memset(ypred_ptr, yleft_col[r], 16);
            ypred_ptr += y_stride;  /*16;*/
        }
    }
    break;

    case TM_PRED:
    {
        for (r = 0; r < 16; r++)
        {
            for (c = 0; c < 16; c++)
            {
                int pred =  yleft_col[r] + yabove_row[c] - ytop_left;

                if (pred < 0)
                    pred = 0;

                if (pred > 255)
                    pred = 255;

                ypred_ptr[c] = pred;
            }

            ypred_ptr += y_stride;  /*16;*/
        }
    }
    break;

    case B_PRED:
    case NEARESTMV:
    case NEARMV:
    case ZEROMV:
    case NEWMV:
    case SPLITMV:
    case MB_MODE_COUNT:
        break;
    }
}

void vp8_build_intra_predictors_mbuv(MACROBLOCKD *mb)
{
    unsigned char *uabove_row = mb->dst.u_buffer - mb->dst.uv_stride;
    unsigned char uleft_col[16];
    unsigned char utop_left = uabove_row[-1];
    unsigned char *vabove_row = mb->dst.v_buffer - mb->dst.uv_stride;
    unsigned char vleft_col[20];
    unsigned char vtop_left = vabove_row[-1];
    unsigned char *upred_ptr = &mb->predictor[256];
    unsigned char *vpred_ptr = &mb->predictor[320];
    int i, j;

    for (i = 0; i < 8; i++)
    {
        uleft_col[i] = mb->dst.u_buffer [i*mb->dst.uv_stride - 1];
        vleft_col[i] = mb->dst.v_buffer [i*mb->dst.uv_stride - 1];
    }

    switch (mb->mode_info_context->mbmi.uv_mode)
    {
    case DC_PRED:
    {
        int expected_udc;
        int expected_vdc;
        int i;
        int shift;
        int Uaverage = 0;
        int Vaverage = 0;

        if (mb->up_available)
        {
            for (i = 0; i < 8; i++)
            {
                Uaverage += uabove_row[i];
                Vaverage += vabove_row[i];
            }
        }

        if (mb->left_available)
        {
            for (i = 0; i < 8; i++)
            {
                Uaverage += uleft_col[i];
                Vaverage += vleft_col[i];
            }
        }

        if (!mb->up_available && !mb->left_available)
        {
            expected_udc = 128;
            expected_vdc = 128;
        }
        else
        {
            shift = 2 + mb->up_available + mb->left_available;
            expected_udc = (Uaverage + (1 << (shift - 1))) >> shift;
            expected_vdc = (Vaverage + (1 << (shift - 1))) >> shift;
        }

        memset(upred_ptr, expected_udc, 64);
        memset(vpred_ptr, expected_vdc, 64);
    }
    break;

    case V_PRED:
    {
        int i;

        for (i = 0; i < 8; i++)
        {
            memcpy(upred_ptr, uabove_row, 8);
            memcpy(vpred_ptr, vabove_row, 8);
            upred_ptr += 8;
            vpred_ptr += 8;
        }
    }
    break;

    case H_PRED:
    {
        int i;
        for (i = 0; i < 8; i++)
        {
            memset(upred_ptr, uleft_col[i], 8);
            memset(vpred_ptr, vleft_col[i], 8);
            upred_ptr += 8;
            vpred_ptr += 8;
        }
    }
    break;

    case TM_PRED:
    {
        int i;
        for (i = 0; i < 8; i++)
        {
            for (j = 0; j < 8; j++)
            {
                int predu = uleft_col[i] + uabove_row[j] - utop_left;
                int predv = vleft_col[i] + vabove_row[j] - vtop_left;

                if (predu < 0)
                    predu = 0;

                if (predu > 255)
                    predu = 255;

                if (predv < 0)
                    predv = 0;

                if (predv > 255)
                    predv = 255;

                upred_ptr[j] = predu;
                vpred_ptr[j] = predv;
            }

            upred_ptr += 8;
            vpred_ptr += 8;
        }
    }
    break;

    case B_PRED:
    case NEARESTMV:
    case NEARMV:
    case ZEROMV:
    case NEWMV:
    case SPLITMV:
    case MB_MODE_COUNT:
        break;
    }
}

void vp8_build_intra_predictors_mbuv_s(MACROBLOCKD *mb)
{
    unsigned char *uabove_row = mb->dst.u_buffer - mb->dst.uv_stride;
    unsigned char uleft_col[16];
    unsigned char utop_left = uabove_row[-1];
    unsigned char *vabove_row = mb->dst.v_buffer - mb->dst.uv_stride;
    unsigned char vleft_col[20];
    unsigned char vtop_left = vabove_row[-1];
    unsigned char *upred_ptr = mb->dst.u_buffer; /*&mb->predictor[256];*/
    unsigned char *vpred_ptr = mb->dst.v_buffer; /*&mb->predictor[320];*/
    int uv_stride = mb->dst.uv_stride;

    int i, j;

    for (i = 0; i < 8; i++)
    {
        uleft_col[i] = mb->dst.u_buffer [i * mb->dst.uv_stride - 1];
        vleft_col[i] = mb->dst.v_buffer [i * mb->dst.uv_stride - 1];
    }

    switch (mb->mode_info_context->mbmi.uv_mode)
    {
    case DC_PRED:
    {
        int expected_udc;
        int expected_vdc;
        int i;
        int shift;
        int Uaverage = 0;
        int Vaverage = 0;

        if (mb->up_available)
        {
            for (i = 0; i < 8; i++)
            {
                Uaverage += uabove_row[i];
                Vaverage += vabove_row[i];
            }
        }

        if (mb->left_available)
        {
            for (i = 0; i < 8; i++)
            {
                Uaverage += uleft_col[i];
                Vaverage += vleft_col[i];
            }
        }

        if (!mb->up_available && !mb->left_available)
        {
            expected_udc = 128;
            expected_vdc = 128;
        }
        else
        {
            shift = 2 + mb->up_available + mb->left_available;
            expected_udc = (Uaverage + (1 << (shift - 1))) >> shift;
            expected_vdc = (Vaverage + (1 << (shift - 1))) >> shift;
        }

        /*memset(upred_ptr, expected_udc, 64);*/
        /*memset(vpred_ptr, expected_vdc, 64);*/
        for (i = 0; i < 8; i++)
        {
            memset(upred_ptr, expected_udc, 8);
            memset(vpred_ptr, expected_vdc, 8);
            upred_ptr += uv_stride; /*8;*/
            vpred_ptr += uv_stride; /*8;*/
        }
    }
    break;

    case V_PRED:
    {
        int i;
        for (i = 0; i < 8; i++)
        {
            memcpy(upred_ptr, uabove_row, 8);
            memcpy(vpred_ptr, vabove_row, 8);
            upred_ptr += uv_stride; /*8;*/
            vpred_ptr += uv_stride; /*8;*/
        }
    }
    break;

    case H_PRED:
    {
        int i;

        for (i = 0; i < 8; i++)
        {
            memset(upred_ptr, uleft_col[i], 8);
            memset(vpred_ptr, vleft_col[i], 8);
            upred_ptr += uv_stride; /*8;*/
            vpred_ptr += uv_stride; /*8;*/
        }
    }
    break;

    case TM_PRED:
    {
        int i;
        for (i = 0; i < 8; i++)
        {
            for (j = 0; j < 8; j++)
            {
                int predu = uleft_col[i] + uabove_row[j] - utop_left;
                int predv = vleft_col[i] + vabove_row[j] - vtop_left;

                if (predu < 0)
                    predu = 0;

                if (predu > 255)
                    predu = 255;

                if (predv < 0)
                    predv = 0;

                if (predv > 255)
                    predv = 255;

                upred_ptr[j] = predu;
                vpred_ptr[j] = predv;
            }

            upred_ptr += uv_stride; /*8;*/
            vpred_ptr += uv_stride; /*8;*/
        }
    }
    break;

    case B_PRED:
    case NEARESTMV:
    case NEARMV:
    case ZEROMV:
    case NEWMV:
    case SPLITMV:
    case MB_MODE_COUNT:
        break;
    }
}
