/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP8_DX_IFACE_H
#define VP8_DX_IFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

#include "vpx_codec.h"
#include "vpx_image.h"

#include "common/onyxd.h"
#include "common/yv12utils.h"
#include "decoder/onyxd_int.h"

/*!\defgroup vp8 VP8
 * \ingroup codecs
 * VP8 is vpx's newest video compression algorithm that uses motion
 * compensated prediction, Discrete Cosine Transform (DCT) coding of the
 * prediction error signal and context dependent entropy coding techniques
 * based on arithmetic principles. It features:
 *  - YUV 4:2:0 image format
 *  - Macro-block based coding (16x16 luma plus two 8x8 chroma)
 *  - 1/4 (1/8) pixel accuracy motion compensated prediction
 *  - 4x4 DCT transform
 *  - 128 level linear quantizer
 *  - In loop deblocking filter
 *  - Context-based entropy coding
 *
 * @{
 */

#define NELEMENTS(x) ((int)(sizeof(x)/sizeof(x[0])))

/*!\brief Stream properties
 *
 * This structure is used to query or set properties of the decoded
 * stream. Algorithms may extend this structure with data specific
 * to their bitstream by setting the sz member appropriately.
 */
typedef struct vp8_stream_info
{
    unsigned int sz;    /**< Size of this structure */
    unsigned int w;     /**< Width (or 0 for unknown/default) */
    unsigned int h;     /**< Height (or 0 for unknown/default) */
    unsigned int is_kf; /**< Current frame is a keyframe */
} vp8_stream_info_t;

/* Structures for handling memory allocations */
typedef enum
{
    VP8_SEG_ALG_PRIV = 256,
    VP8_SEG_MAX
} mem_seg_id_t;

typedef struct
{
    unsigned int  id;
    unsigned long sz;
    unsigned int  align;
    unsigned int  flags;
} mem_req_t;

/* ************************************************************************** */

vpx_codec_err_t vp8_init(vpx_codec_ctx_t *ctx);

vpx_codec_err_t vp8_destroy(vpx_codec_alg_priv_t *ctx);

vpx_codec_err_t vp8_decode(vpx_codec_alg_priv_t *ctx,
                           const uint8_t        *data,
                           unsigned int          data_sz,
                           void                 *user_priv,
                           long                  deadline);

vpx_image_t *vp8_get_frame(vpx_codec_alg_priv_t *ctx,
                           vpx_codec_iter_t     *iter);

/* ************************************************************************** */

/*! @} - end defgroup vp8 */

#ifdef __cplusplus
}
#endif

#endif /* VP8_DX_IFACE_H */
