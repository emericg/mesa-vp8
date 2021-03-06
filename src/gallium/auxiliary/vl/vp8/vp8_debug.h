/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP8_DEBUG_H
#define VP8_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <setjmp.h>

/**
 * \brief Algorithm return codes.
 */
typedef enum {
    /** \brief Operation completed without error */
    VPX_CODEC_OK,

    /** \brief Unspecified error */
    VPX_CODEC_ERROR,

    /** \brief Memory operation failed */
    VPX_CODEC_MEM_ERROR,

    /**
     * \brief The coded data for this stream is corrupt or incomplete.
     *
     * There was a problem decoding the current frame.  This return code
     * should only be used for failures that prevent future pictures from
     * being properly decoded. This error \ref MAY be treated as fatal to the
     * stream or \ref MAY be treated as fatal to the current GOP. If decoding
     * is continued for the current GOP, artifacts may be present.
     */
    VPX_CODEC_CORRUPT_FRAME

} vpx_codec_err_t;

struct vpx_internal_error_info
{
    vpx_codec_err_t  error_code;
    int              has_detail;
    char             detail[80];
    int              setjmp;
    jmp_buf          jmp;
};

/** \brief Internal error */
void vpx_internal_error(struct vpx_internal_error_info *info,
                        vpx_codec_err_t                 error,
                        const char                     *fmt,
                        ...);

#ifdef __cplusplus
}
#endif

#endif /* VP8_DEBUG_H */
