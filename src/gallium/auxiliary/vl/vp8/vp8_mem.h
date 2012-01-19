/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef VP8_MEM_H
#define VP8_MEM_H

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Use this define on systems where unaligned int reads and writes are
 * not allowed, i.e. ARM architectures.
 */
//#define MUST_BE_ALIGNED

#if defined(__GNUC__) && __GNUC__
    #define DECLARE_ALIGNED(n,typ,val) typ val __attribute__ ((aligned (n)))
#else
    #warning No alignment directives known for this compiler.
    #define DECLARE_ALIGNED(n,typ,val) typ val
#endif

void *vpx_memalign(size_t align, size_t size);
void *vpx_calloc(size_t num, size_t size);
void vpx_free(void *memblk);

#if defined(__cplusplus)
}
#endif

#endif /* VP8_MEM_H */
