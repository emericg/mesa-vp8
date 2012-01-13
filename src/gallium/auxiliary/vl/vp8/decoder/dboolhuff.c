/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "dboolhuff.h"

int vp8dx_start_decode(BOOL_DECODER *bd,
                       const unsigned char *data,
                       unsigned int data_size)
{
    bd->user_buffer_end = data + data_size;
    bd->user_buffer     = data;
    bd->value = 0;
    bd->count = -8;
    bd->range = 255;

    if (data_size && !data)
        return 1;

    /* Populate the buffer */
    vp8dx_bool_decoder_fill(bd);

    return 0;
}

void vp8dx_bool_decoder_fill(BOOL_DECODER *bd)
{
    const unsigned char *bufptr;
    const unsigned char *bufend;
    VP8_BD_VALUE         value;
    int                  count;
    bufend = bd->user_buffer_end;
    bufptr = bd->user_buffer;
    value = bd->value;
    count = bd->count;

    VP8DX_BOOL_DECODER_FILL(count, value, bufptr, bufend);

    bd->user_buffer = bufptr;
    bd->value = value;
    bd->count = count;
}

/**
 * Check if we have reached the end of the buffer.
 *
 * Variable 'count' stores the number of bits in the 'value' buffer, minus
 * 8. The top byte is part of the algorithm, and the remainder is buffered
 * to be shifted into it. So if count == 8, the top 16 bits of 'value' are
 * occupied, 8 for the algorithm and 8 in the buffer.
 *
 * When reading a byte from the user's buffer, count is filled with 8 and
 * one byte is filled into the value buffer. When we reach the end of the
 * data, count is additionally filled with VP8_LOTS_OF_BITS. So when
 * count == VP8_LOTS_OF_BITS - 1, the user's data has been exhausted.
 */
int vp8dx_bool_error(BOOL_DECODER *bd)
{
    if ((bd->count > VP8_BD_VALUE_SIZE) && (bd->count < VP8_LOTS_OF_BITS))
    {
        /* We have tried to decode bits after the end of stream was encountered. */
        return 1;
    }

    return 0;
}
