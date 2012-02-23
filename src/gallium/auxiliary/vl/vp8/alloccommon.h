/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef ALLOCCOMMON_H
#define ALLOCCOMMON_H

#include "vp8_decoder.h"

int vp8_alloc_frame_buffers(VP8_COMMON *common, int width, int height);
void vp8_dealloc_frame_buffers(VP8_COMMON *common);

void vp8_initialize_common(VP8_COMMON *common);
void vp8_setup_version(VP8_COMMON *common);

#endif /* ALLOCCOMMON_H */
