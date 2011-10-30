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

#include "../vp8_debug.h"
#include "yv12utils.h"

typedef void *VP8D_PTR;

VP8D_PTR vp8dx_create_decompressor(int input_partition);
void vp8dx_remove_decompressor(VP8D_PTR comp);

int vp8dx_receive_compressed_data(VP8D_PTR comp, const unsigned char *data, unsigned data_size, int64_t time_stamp);
int vp8dx_get_raw_frame(VP8D_PTR comp, YV12_BUFFER_CONFIG *sd, int64_t *time_stamp, int64_t *time_end_stamp);

#ifdef __cplusplus
}
#endif

#endif /* VP8D_H */
