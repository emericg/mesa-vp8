/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef TREEREADER_H
#define TREEREADER_H

#include "treereader_common.h"
#include "vp8_mem.h"

typedef size_t VP8_BD_VALUE;

typedef struct
{
    const unsigned char *user_buffer_end;
    const unsigned char *user_buffer;
    VP8_BD_VALUE         value;
    int                  count;
    unsigned int         range;
} BOOL_DECODER;

/**
 * This is meant to be a large, positive constant that can still be efficiently
 * loaded as an immediate (on platforms like ARM, for example).
 * Even relatively modest values like 100 would work fine.
 */
#define VP8_LOTS_OF_BITS (0x40000000)

#define VP8_BD_VALUE_SIZE ((int)sizeof(VP8_BD_VALUE)*CHAR_BIT)

#define vp8_read vp8dx_decode_bool
#define vp8_read_literal vp8_decode_value
#define vp8_read_bit( R) vp8_read( R, vp8_prob_half)

DECLARE_ALIGNED(16, extern const unsigned char, vp8_norm[256]);

int vp8dx_start_decode(BOOL_DECODER *bd, const unsigned char *data,
                       const unsigned int data_size);

void vp8dx_bool_decoder_fill(BOOL_DECODER *bd);

int vp8dx_bool_error(BOOL_DECODER *bd);

int vp8dx_decode_bool(BOOL_DECODER *bd, int probability);

int vp8_decode_value(BOOL_DECODER *bd, int bits);

int vp8_treed_read(BOOL_DECODER *const bd, vp8_tree t, const vp8_prob *const p);

/**
 * The refill loop is used in several places, so define it in a macro to make
 * sure they're all consistent.
 * An inline function would be cleaner, but has a significant penalty, because
 * multiple BOOL_DECODER fields must be modified, and the compiler is not smart
 * enough to eliminate the stores to those fields and the subsequent reloads
 * from them when inlining the function.
 */
#define VP8DX_BOOL_DECODER_FILL(_count, _value, _bufptr, _bufend) \
    do \
    { \
        size_t bits_left = ((_bufend) - (_bufptr))*CHAR_BIT; \
        int shift = VP8_BD_VALUE_SIZE - 8 - ((_count) + 8); \
        int loop_end = 0; \
        int x = shift + CHAR_BIT - bits_left; \
        \
        if (x >= 0) \
        { \
            (_count) += VP8_LOTS_OF_BITS; \
            loop_end = x; \
            if (!bits_left) break; \
        } \
        while (shift >= loop_end) \
        { \
            (_count) += CHAR_BIT; \
            (_value) |= (VP8_BD_VALUE)*(_bufptr)++ << shift; \
            shift -= CHAR_BIT; \
        } \
    } \
    while(0)

#endif /* TREEREADER_H */
