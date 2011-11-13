/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef VP8D_H
#define VP8D_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#include "pipe/p_video_decoder.h"
#include "../vp8_debug.h"
#include "yv12utils.h"

typedef void* VP8D_PTR;

VP8D_PTR vp8_decoder_create();

int vp8_decoder_start(VP8D_PTR comp, struct pipe_vp8_picture_desc *frame_header,
                      const unsigned char *data, unsigned data_size,
                      int64_t timestamp_deadline);

int vp8_decoder_getframe(VP8D_PTR comp, YV12_BUFFER_CONFIG *sd,
                         int64_t *timestamp, int64_t *timestamp_end);

void vp8_decoder_remove(VP8D_PTR comp);

#ifdef __cplusplus
}
#endif

#endif /* VP8D_H */
