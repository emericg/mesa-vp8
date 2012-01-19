/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vp8_mem.h"

#define ADDRESS_STORAGE_SIZE sizeof(size_t)
#define DEFAULT_ALIGNMENT    32 // must be >= 1 !

/* returns an addr aligned to the byte boundary specified by align */
#define align_addr(addr,align) (void*)(((size_t)(addr) + ((align) - 1)) & (size_t)-(align))

void *vpx_memalign(size_t align, size_t size)
{
    void *addr = malloc(size + align - 1 + ADDRESS_STORAGE_SIZE);
    void *x = NULL;

    if (addr)
    {
        x = align_addr((unsigned char *)addr + ADDRESS_STORAGE_SIZE, (int)align);
        // save the actual malloc address
        ((size_t *)x)[-1] = (size_t)addr;
    }

    return x;
}

void *vpx_calloc(size_t num, size_t size)
{
    void *x = vpx_memalign(DEFAULT_ALIGNMENT, num * size);

    if (x)
        memset(x, 0, num * size);

    return x;
}

void vpx_free(void *memblk)
{
    if (memblk)
    {
        void *addr = (void *)(((size_t *)memblk)[-1]);
        free(addr);
    }
}
