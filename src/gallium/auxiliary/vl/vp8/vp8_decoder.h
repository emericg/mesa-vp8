/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP8_DECODER_H
#define VP8_DECODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vp8_debug.h"

#include "common/onyxd.h"
#include "common/yv12utils.h"
#include "decoder/onyxd_int.h"

/**
 * \defgroup vp8 VP8
 * \ingroup codecs
 *
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

/* ************************************************************************** */

/**
 * \brief Stream properties.
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

/**
 * \brief Instance private storage.
 *
 * Contains data private to the codec implementation. This structure is opaque
 * to the application.
 */
typedef struct vpx_codec_alg_priv
{
    int                     decoder_init;
    vp8_stream_info_t       si;
    VP8D_PTR                pbi;
    int                     img_avail;
    YV12_BUFFER_CONFIG      img_yv12;
} vpx_codec_alg_priv_t;

/* ************************************************************************** */

vpx_codec_err_t vp8_init(vpx_codec_alg_priv_t *ctx);

vpx_codec_err_t vp8_destroy(vpx_codec_alg_priv_t *ctx);

vpx_codec_err_t vp8_decode(vpx_codec_alg_priv_t *ctx,
                           const uint8_t        *data,
                           unsigned int          data_sz,
                           long                  deadline);

vpx_codec_err_t vp8_dump_frame(vpx_codec_alg_priv_t *ctx);

YV12_BUFFER_CONFIG *vp8_get_frame(vpx_codec_alg_priv_t *ctx);

/* ************************************************************************** */

/*! @} - end defgroup vp8 */

#ifdef __cplusplus
}
#endif

#endif /* VP8_DECODER_H */
