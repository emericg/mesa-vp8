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

/* Create/destroy static data structures. */
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#include "../vpx_codec.h"
#include "../vpx_mem.h"
#include "yv12utils.h"

typedef void *VP8D_PTR;

typedef struct
{
    int Width;
    int Height;
    int input_partition;
} VP8D_CONFIG;

/*!\brief reference frame type
 * The set of macros define the type of VP8 reference frames
 */
typedef enum
{
    VP8_LAST_FRAME = 1,
    VP8_GOLD_FRAME = 2,
    VP8_ALTR_FRAME = 4
} VP8_REF_FRAME;

typedef enum
{
    VP8D_OK = 0
} VP8D_SETTING;

void vp8dx_initialize(void);

int vp8dx_receive_compressed_data(VP8D_PTR comp, unsigned long size, const unsigned char *dest, int64_t time_stamp);
int vp8dx_get_raw_frame(VP8D_PTR comp, YV12_BUFFER_CONFIG *sd, int64_t *time_stamp, int64_t *time_end_stamp);

vpx_codec_err_t vp8dx_get_reference(VP8D_PTR comp, VP8_REF_FRAME ref_frame_flag, YV12_BUFFER_CONFIG *sd);
vpx_codec_err_t vp8dx_set_reference(VP8D_PTR comp, VP8_REF_FRAME ref_frame_flag, YV12_BUFFER_CONFIG *sd);

VP8D_PTR vp8dx_create_decompressor(VP8D_CONFIG *oxcf);

void vp8dx_remove_decompressor(VP8D_PTR comp);

#ifdef __cplusplus
}
#endif

#endif /* VP8D_H */
