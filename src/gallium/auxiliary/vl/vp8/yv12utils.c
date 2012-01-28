/*
 *  Copyright (c) 2011 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vp8_mem.h"
#include "yv12utils.h"

/**
 * \note buffer_alloc isn't accessed by most functions. Rather y_buffer,
 *       u_buffer and v_buffer point to buffer_alloc and are used. Clear out
 *       all of this so that a freed pointer isn't inadvertently used
 */
int vp8_yv12_de_alloc_frame_buffer(YV12_BUFFER_CONFIG *ybf)
{
    if (ybf)
    {
        vpx_free(ybf->buffer_alloc);
        memset (ybf, 0, sizeof(YV12_BUFFER_CONFIG));
    }
    else
    {
        return -1;
    }

    return 0;
}

/**
 * There is currently a bunch of code which assumes uv_stride == y_stride/2,
 * so enforce this here.
 *
 * Only support allocating buffers that have a height and width that are
 * multiples of 16, and a border that's a multiple of 32.
 * The border restriction is required to get 16-byte alignment of the start of
 * the chroma rows without intoducing an arbitrary gap between planes.
 */
int vp8_yv12_alloc_frame_buffer(YV12_BUFFER_CONFIG *ybf,
                                int width, int height, int border)
{
    if (ybf)
    {
        int y_stride = ((width + 2 * border) + 31) & ~31;
        int y_size = (height + 2 * border) * y_stride;

        int uv_width = width >> 1;
        int uv_height = height >> 1;
        int uv_stride = y_stride >> 1;
        int uv_size = (uv_height + border) * uv_stride;

        vp8_yv12_de_alloc_frame_buffer(ybf);

        if ((width & 0xf) | (height & 0xf) | (border & 0x1f))
            return -3;

        ybf->y_width  = width;
        ybf->y_height = height;
        ybf->y_stride = y_stride;

        ybf->uv_width = uv_width;
        ybf->uv_height = uv_height;
        ybf->uv_stride = uv_stride;

        ybf->border = border;
        ybf->frame_size = y_size + 2 * uv_size;

        ybf->buffer_alloc = (unsigned char *)vpx_memalign(32, ybf->frame_size);

        if (ybf->buffer_alloc == NULL)
        {
            return -1;
        }

        ybf->y_buffer = ybf->buffer_alloc + (border * y_stride) + border;
        ybf->u_buffer = ybf->buffer_alloc + y_size + (border / 2  * uv_stride) + border / 2;
        ybf->v_buffer = ybf->buffer_alloc + y_size + uv_size + (border / 2  * uv_stride) + border / 2;

        ybf->corrupted = 0; /* Assume not currupted by errors */
    }
    else
    {
        return -2;
    }

    return 0;
}

void vp8_yv12_extend_frame_borders(YV12_BUFFER_CONFIG *ybf)
{
    int i;
    unsigned char *src_ptr1, *src_ptr2;
    unsigned char *dest_ptr1, *dest_ptr2;

    unsigned int border;
    int plane_stride;
    int plane_height;
    int plane_width;

    /***********/
    /* Y Plane */
    /***********/

    border = ybf->border;
    plane_stride = ybf->y_stride;
    plane_height = ybf->y_height;
    plane_width = ybf->y_width;

    /* Copy the left and right most columns out */
    src_ptr1 = ybf->y_buffer;
    src_ptr2 = src_ptr1 + plane_width - 1;
    dest_ptr1 = src_ptr1 - border;
    dest_ptr2 = src_ptr2 + 1;

    for (i = 0; i < plane_height; i++)
    {
        memset(dest_ptr1, src_ptr1[0], border);
        memset(dest_ptr2, src_ptr2[0], border);
        src_ptr1  += plane_stride;
        src_ptr2  += plane_stride;
        dest_ptr1 += plane_stride;
        dest_ptr2 += plane_stride;
    }

    /* Now copy the top and bottom source lines into each line of the respective borders */
    src_ptr1 = ybf->y_buffer - border;
    src_ptr2 = src_ptr1 + (plane_height * plane_stride) - plane_stride;
    dest_ptr1 = src_ptr1 - (border * plane_stride);
    dest_ptr2 = src_ptr2 + plane_stride;

    for (i = 0; i < (int)border; i++)
    {
        memcpy(dest_ptr1, src_ptr1, plane_stride);
        memcpy(dest_ptr2, src_ptr2, plane_stride);
        dest_ptr1 += plane_stride;
        dest_ptr2 += plane_stride;
    }

    /***********/
    /* U Plane */
    /***********/

    plane_stride = ybf->uv_stride;
    plane_height = ybf->uv_height;
    plane_width = ybf->uv_width;
    border /= 2;

    /* Copy the left and right most columns out */
    src_ptr1 = ybf->u_buffer;
    src_ptr2 = src_ptr1 + plane_width - 1;
    dest_ptr1 = src_ptr1 - border;
    dest_ptr2 = src_ptr2 + 1;

    for (i = 0; i < plane_height; i++)
    {
        memset(dest_ptr1, src_ptr1[0], border);
        memset(dest_ptr2, src_ptr2[0], border);
        src_ptr1  += plane_stride;
        src_ptr2  += plane_stride;
        dest_ptr1 += plane_stride;
        dest_ptr2 += plane_stride;
    }

    /* Now copy the top and bottom source lines into each line of the respective borders */
    src_ptr1 = ybf->u_buffer - border;
    src_ptr2 = src_ptr1 + (plane_height * plane_stride) - plane_stride;
    dest_ptr1 = src_ptr1 - (border * plane_stride);
    dest_ptr2 = src_ptr2 + plane_stride;

    for (i = 0; i < (int)(border); i++)
    {
        memcpy(dest_ptr1, src_ptr1, plane_stride);
        memcpy(dest_ptr2, src_ptr2, plane_stride);
        dest_ptr1 += plane_stride;
        dest_ptr2 += plane_stride;
    }

    /***********/
    /* V Plane */
    /***********/

    /* Copy the left and right most columns out */
    src_ptr1 = ybf->v_buffer;
    src_ptr2 = src_ptr1 + plane_width - 1;
    dest_ptr1 = src_ptr1 - border;
    dest_ptr2 = src_ptr2 + 1;

    for (i = 0; i < plane_height; i++)
    {
        memset(dest_ptr1, src_ptr1[0], border);
        memset(dest_ptr2, src_ptr2[0], border);
        src_ptr1  += plane_stride;
        src_ptr2  += plane_stride;
        dest_ptr1 += plane_stride;
        dest_ptr2 += plane_stride;
    }

    /* Now copy the top and bottom source lines into each line of the respective borders */
    src_ptr1 = ybf->v_buffer - border;
    src_ptr2 = src_ptr1 + (plane_height * plane_stride) - plane_stride;
    dest_ptr1 = src_ptr1 - (border * plane_stride);
    dest_ptr2 = src_ptr2 + plane_stride;

    for (i = 0; i < (int)(border); i++)
    {
        memcpy(dest_ptr1, src_ptr1, plane_stride);
        memcpy(dest_ptr2, src_ptr2, plane_stride);
        dest_ptr1 += plane_stride;
        dest_ptr2 += plane_stride;
    }
}
