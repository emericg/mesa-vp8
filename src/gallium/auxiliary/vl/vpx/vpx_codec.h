/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


/*!\defgroup codec Common Algorithm Interface
 * This abstraction allows applications to easily support multiple video
 * formats with minimal code duplication. This section describes the interface
 * common to all codecs (both encoders and decoders).
 * @{
 */

/*!\file
 * \brief Describes the codec algorithm interface to applications.
 *
 * This file describes the interface between an application and a
 * video codec algorithm.
 */

#ifndef VPX_CODEC_H
#define VPX_CODEC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <setjmp.h>
#include <stdarg.h>

#include "vpx_image.h"

    /*!\brief Algorithm return codes */
    typedef enum {
        /*!\brief Operation completed without error */
        VPX_CODEC_OK,

        /*!\brief Unspecified error */
        VPX_CODEC_ERROR,

        /*!\brief Memory operation failed */
        VPX_CODEC_MEM_ERROR,

        /*!\brief ABI version mismatch */
        VPX_CODEC_ABI_MISMATCH,

        /*!\brief Algorithm does not have required capability */
        VPX_CODEC_INCAPABLE,

        /*!\brief The given bitstream is not supported.
         *
         * The bitstream was unable to be parsed at the highest level. The decoder
         * is unable to proceed. This error \ref SHOULD be treated as fatal to the
         * stream. */
        VPX_CODEC_UNSUP_BITSTREAM,

        /*!\brief Encoded bitstream uses an unsupported feature
         *
         * The decoder does not implement a feature required by the encoder. This
         * return code should only be used for features that prevent future
         * pictures from being properly decoded. This error \ref MAY be treated as
         * fatal to the stream or \ref MAY be treated as fatal to the current GOP.
         */
        VPX_CODEC_UNSUP_FEATURE,

        /*!\brief The coded data for this stream is corrupt or incomplete
         *
         * There was a problem decoding the current frame.  This return code
         * should only be used for failures that prevent future pictures from
         * being properly decoded. This error \ref MAY be treated as fatal to the
         * stream or \ref MAY be treated as fatal to the current GOP. If decoding
         * is continued for the current GOP, artifacts may be present.
         */
        VPX_CODEC_CORRUPT_FRAME,

        /*!\brief An application-supplied parameter is not valid.
         *
         */
        VPX_CODEC_INVALID_PARAM,

        /*!\brief An iterator reached the end of list.
         *
         */
        VPX_CODEC_LIST_END
    } vpx_codec_err_t;


    /*! \brief Initialization-time Feature Enabling
     *
     *  Certain codec features must be known at initialization time, to allow for
     *  proper memory allocation.
     *
     *  The available flags are specified by VPX_CODEC_USE_* defines.
     */
    typedef long vpx_codec_flags_t;


    /*!\brief Iterator
     *
     * Opaque storage used for iterating over lists.
     */
    typedef const void *vpx_codec_iter_t;


    /*!\brief Instance private storage
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
        vpx_codec_flags_t          init_flags;
    } vpx_codec_priv_t;

    /*!\brief Codec context structure
     *
     * All codecs \ref MUST support this context structure fully. In general,
     * this data should be considered private to the codec algorithm, and
     * not be manipulated or examined by the calling application. Applications
     * may reference the 'name' member to get a printable description of the
     * algorithm.
     */
    typedef struct vpx_codec_ctx
    {
        const char                   *name;        /**< Printable interface name */
        vpx_codec_err_t               err;         /**< Last returned error */
        const char                   *err_detail;  /**< Detailed info, if available */
        vpx_codec_flags_t             init_flags;  /**< Flags passed at init time */
        union
        {
            struct vpx_codec_dec_cfg *dec;         /**< Decoder Configuration Pointer */
            void                     *raw;
        } config;                                  /**< Configuration pointer aliasing union */
        vpx_codec_priv_t             *priv;        /**< Algorithm private storage */
    } vpx_codec_ctx_t;

   /*!\brief Memory Map Entry
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

   #define VPX_CODEC_MEM_ZERO     0x1  /**< Segment must be zeroed by allocation */
   #define VPX_CODEC_MEM_WRONLY   0x2  /**< Segment need not be readable */
   #define VPX_CODEC_MEM_FAST     0x4  /**< Place in fast memory, if available */

       /* The following members are to be filled in by the allocation function */
       void *base;   /**< pointer to the allocated segment */
       void (*dtor)(struct vpx_codec_mmap *map);   /**< destructor to call */
       void *priv;   /**< allocator private storage */
   } vpx_codec_mmap_t; /**< alias for struct vpx_codec_mmap */


   typedef struct vpx_codec_alg_priv vpx_codec_alg_priv_t;


    /*!\brief Initialization Configurations
     *
     * This structure is used to pass init time configuration options to the
     * decoder.
     */
    typedef struct vpx_codec_dec_cfg
    {
        unsigned int w;       /**< Width */
        unsigned int h;       /**< Height */
    } vpx_codec_dec_cfg_t;


    struct vpx_internal_error_info
    {
        vpx_codec_err_t  error_code;
        int              has_detail;
        char             detail[80];
        int              setjmp;
        jmp_buf          jmp;
    };

    /*!\brief Convert error number to printable string
     *
     * Returns a human readable string for the last error returned by the
     * algorithm. The returned error will be one line and will not contain
     * any newline characters.
     *
     *
     * \param[in]    err     Error number.
     *
     */
    const char *vpx_codec_err_to_string(vpx_codec_err_t err);


    /*!\brief Retrieve error synopsis for codec context
     *
     * Returns a human readable string for the last error returned by the
     * algorithm. The returned error will be one line and will not contain
     * any newline characters.
     *
     *
     * \param[in]    ctx     Pointer to this instance's context.
     *
     */
    const char *vpx_codec_error(vpx_codec_ctx_t *ctx);


    /*!\brief Retrieve detailed error information for codec context
     *
     * Returns a human readable string providing detailed information about
     * the last error.
     *
     * \param[in]    ctx     Pointer to this instance's context.
     *
     * \retval NULL
     *     No detailed information is available.
     */
    const char *vpx_codec_error_detail(vpx_codec_ctx_t *ctx);


    /*!\brief Internal error
     */
    static void vpx_internal_error(struct vpx_internal_error_info *info,
                                   vpx_codec_err_t                 error,
                                   const char                     *fmt,
                                   ...)
    {
        va_list ap;

        info->error_code = error;
        info->has_detail = 0;

        if (fmt)
        {
            size_t  sz = sizeof(info->detail);

            info->has_detail = 1;
            va_start(ap, fmt);
            vsnprintf(info->detail, sz - 1, fmt, ap);
            va_end(ap);
            info->detail[sz-1] = '\0';
        }

        if (info->setjmp)
            longjmp(info->jmp, info->error_code);
    }

    /*!@} - end defgroup codec*/


#ifdef __cplusplus
}
#endif

#endif /* VPX_CODEC_H */
