/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "treereader.h"

#include <stddef.h>
#include <limits.h>

/**
 * Initialize the treereader.
 */
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
    VP8_BD_VALUE value;
    int count;
    bufend = bd->user_buffer_end;
    bufptr = bd->user_buffer;
    value = bd->value;
    count = bd->count;

    VP8DX_BOOL_DECODER_FILL(count, value, bufptr, bufend);

    bd->user_buffer = bufptr;
    bd->value = value;
    bd->count = count;
}

int vp8dx_decode_bool(BOOL_DECODER *bd, int probability)
{
    unsigned int bit = 0;
    VP8_BD_VALUE value;
    unsigned int split;
    VP8_BD_VALUE bigsplit;
    int count;
    unsigned int range;

    split = 1 + (((bd->range - 1) * probability) >> 8);

    if (bd->count < 0)
        vp8dx_bool_decoder_fill(bd);

    value = bd->value;
    count = bd->count;

    bigsplit = (VP8_BD_VALUE)split << (VP8_BD_VALUE_SIZE - 8);

    range = split;

    if (value >= bigsplit)
    {
        range = bd->range - split;
        value = value - bigsplit;
        bit = 1;
    }

    {
        register unsigned int shift = vp8_norm[range];
        range <<= shift;
        value <<= shift;
        count -= shift;
    }

    bd->value = value;
    bd->count = count;
    bd->range = range;

    return bit;
}

int vp8_decode_value(BOOL_DECODER *bd, int bits)
{
    int z = 0;
    int bit;

    for (bit = bits - 1; bit >= 0; bit--)
    {
        z |= (vp8dx_decode_bool(bd, 0x80) << bit);
    }

    return z;
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

/** Must return a 0 or 1 !!! */
int vp8_treed_read(BOOL_DECODER *const bd, vp8_tree t, const vp8_prob *const p)
{
    register vp8_tree_index i = 0;

    while ((i = t[i + vp8_read(bd, p[i >> 1])]) > 0);

    return -i;
}
