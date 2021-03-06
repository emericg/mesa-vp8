/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef TREEREADER_COMMON_H
#define TREEREADER_COMMON_H

#define vp8_prob_half ((vp8_prob) 128)

typedef unsigned char vp8_prob;

typedef signed char vp8_tree_index;

/* We build coding trees compactly in arrays.
   Each node of the tree is a pair of vp8_tree_indices.
   Array index often references a corresponding probability table.
   Index <= 0 means done encoding/decoding and value = -Index,
   Index > 0 means need another bit, specification at index.
   Nonnegative indices are always even;  processing begins at node 0. */

typedef const vp8_tree_index vp8_tree[], *vp8_tree_p;

typedef const struct vp8_token_struct
{
    int value;
    int Len;
} vp8_token;

void vp8_tokens_from_tree(struct vp8_token_struct *, vp8_tree);

void vp8_tokens_from_tree_offset(struct vp8_token_struct *, vp8_tree, int offset);

void vp8_tree_probs_from_distribution(int n, /* n = size of alphabet */
                                      vp8_token tok[/* n */],
                                      vp8_tree tree,
                                      vp8_prob probs[/* n-1 */],
                                      unsigned int branch_ct[/* n-1 */][2],
                                      const unsigned int num_events[/* n */],
                                      unsigned int Pfactor,
                                      int Round);

#endif /* TREEREADER_COMMON_H */
