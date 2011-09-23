/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef __VPX_MEM_H__
#define __VPX_MEM_H__

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>

#if defined(__cplusplus)
extern "C" {
#endif

/* ************************************************************************** */

#define ADDRESS_STORAGE_SIZE sizeof(size_t)
#define DEFAULT_ALIGNMENT    32 // must be >= 1 !

/* returns an addr aligned to the byte boundary specified by align */
#define align_addr(addr,align) (void*)(((size_t)(addr) + ((align) - 1)) & (size_t)-(align))

#if defined(__GNUC__) && __GNUC__
#define DECLARE_ALIGNED(n,typ,val)  typ val __attribute__ ((aligned (n)))
#else
#warning No alignment directives known for this compiler.
#define DECLARE_ALIGNED(n,typ,val)  typ val
#endif

/* ************************************************************************** */

void *vpx_memalign(size_t align, size_t size);
void *vpx_malloc(size_t size);
void *vpx_calloc(size_t num, size_t size);
void *vpx_realloc(void *memblk, size_t size);
void vpx_free(void *memblk);

/* ************************************************************************** */

#if defined(__cplusplus)
}
#endif

#endif /* __VPX_MEM_H__ */
