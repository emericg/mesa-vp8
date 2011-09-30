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

#define NELEMENTS(x) ((int)(sizeof(x)/sizeof(x[0])))

/**
 * \brief Instance private storage.
 *
 * Contains data private to the codec implementation. This structure is opaque
 * to the application.
 *
 * This structure is allocated by the algorithm's init function. It can be
 * extended in one of two ways. First, a second, algorithm specific structure
 * can be allocated and the priv member pointed to it. Alternatively, this
 * structure can be made the first member of the algorithm specific structure,
 * and the pointer cast to the proper type.
 */
typedef struct vpx_codec_priv
{
    unsigned int               sz;
    struct vpx_codec_alg_priv *alg_priv;
    const char                *err_detail;
} vpx_codec_priv_t;

/**
 * \brief Memory Map Entry.
 *
 * This structure is used to contain the properties of a memory segment. It
 * is populated by the codec in the request phase, and by the calling
 * application once the requested allocation has been performed.
 */
typedef struct vpx_codec_mmap
{
   /*
    * The following members are set by the codec when requesting a segment
    */
   unsigned int  id;     /**< identifier for the segment's contents */
   unsigned long sz;     /**< size of the segment, in bytes */
   unsigned int  align;  /**< required alignment of the segment, in bytes */
   unsigned int  flags;  /**< bitfield containing segment properties */

   /* The following members are to be filled in by the allocation function */
   void *base;   /**< pointer to the allocated segment */
   void (*dtor)(struct vpx_codec_mmap *map);   /**< destructor to call */
   void *priv;   /**< allocator private storage */
} vpx_codec_mmap_t; /**< alias for struct vpx_codec_mmap */

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

static const mem_req_t vp8_mem_req_segs[] =
{
    {VP8_SEG_ALG_PRIV, 0, 8, 1},
    {VP8_SEG_MAX, 0, 0, 0}
};

typedef struct vpx_codec_alg_priv
{
    vpx_codec_priv_t        base;
    vpx_codec_mmap_t        mmaps[NELEMENTS(vp8_mem_req_segs)-1];
    vp8_stream_info_t       si;
    int                     defer_alloc;
    int                     decoder_init;
    VP8D_PTR                pbi;
    YV12_BUFFER_CONFIG      img_yv12;
    int                     img_avail;
} vpx_codec_alg_priv_t;

/* ************************************************************************** */

vpx_codec_err_t vp8_init(vpx_codec_priv_t **priv);

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
