/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef tree_reader_h
#define tree_reader_h

#include "../common/treecoder.h"
#include "dboolhuff.h"

#define vp8_read vp8dx_decode_bool
#define vp8_read_literal vp8_decode_value
#define vp8_read_bit( R) vp8_read( R, vp8_prob_half)

/* Intent of tree data structure is to make decoding trivial. */

/** must return a 0 or 1 !!! */
static int vp8_treed_read(BOOL_DECODER *const bd, vp8_tree t, const vp8_prob *const p)
{
    register vp8_tree_index i = 0;

    while ((i = t[i + vp8_read(bd, p[i >> 1])]) > 0);

    return -i;
}

#endif /* tree_reader_h */
