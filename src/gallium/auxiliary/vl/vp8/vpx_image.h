/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


/*!\file
 * \brief Describes the vpx image descriptor and associated operations
 *
 */

#ifndef VPX_IMAGE_H
#define VPX_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#define VPX_PLANE_Y      0   /**< Y (Luminance) plane */
#define VPX_PLANE_U      1   /**< U (Chroma) plane */
#define VPX_PLANE_V      2   /**< V (Chroma) plane */
#define VPX_PLANE_ALPHA  3   /**< A (Transparency) plane */

#define VPX_IMG_FMT_PLANAR     0x100  /**< Image is a planar format */
#define VPX_IMG_FMT_UV_FLIP    0x200  /**< V plane precedes U plane in memory */
#define VPX_IMG_FMT_HAS_ALPHA  0x400  /**< Image has an alpha channel component */

    /*!\brief List of supported image formats */
    typedef enum vpx_img_fmt {
        VPX_IMG_FMT_NONE,
        VPX_IMG_FMT_YV12    = VPX_IMG_FMT_PLANAR | VPX_IMG_FMT_UV_FLIP | 1, /**< planar YVU */
        VPX_IMG_FMT_I420    = VPX_IMG_FMT_PLANAR | 2,
        VPX_IMG_FMT_VPXYV12 = VPX_IMG_FMT_PLANAR | VPX_IMG_FMT_UV_FLIP | 3, /** < planar 4:2:0 format with vpx color space */
        VPX_IMG_FMT_VPXI420 = VPX_IMG_FMT_PLANAR | 4   /** < planar 4:2:0 format with vpx color space */
    } vpx_img_fmt_t; /**< alias for enum vpx_img_fmt */

    /**\brief Image Descriptor */
    typedef struct vpx_image
    {
        vpx_img_fmt_t fmt; /**< Image Format */

        /* Image storage dimensions */
        unsigned int  w;   /**< Stored image width */
        unsigned int  h;   /**< Stored image height */

        /* Image display dimensions */
        unsigned int  d_w;   /**< Displayed image width */
        unsigned int  d_h;   /**< Displayed image height */

        /* Chroma subsampling info */
        unsigned int  x_chroma_shift;   /**< subsampling order, X */
        unsigned int  y_chroma_shift;   /**< subsampling order, Y */

        /* Image data pointers. */
        unsigned char *planes[4];  /**< pointer to the top left pixel for each plane */
        int            stride[4];  /**< stride between rows for each plane */
        int            bps;        /**< bits per sample (for packed formats) */

        /* The following member may be set by the application to associate data
         * with this image.
         */
        void *user_priv; /**< may be set by the application to associate data
                          *   with this image. */

        /* The following members should be treated as private. */
        unsigned char *img_data;       /**< private */
        int            img_data_owner; /**< private */
        int            self_allocd;    /**< private */
    } vpx_image_t; /**< alias for struct vpx_image */

#ifdef __cplusplus
}
#endif

#endif /* VPX_IMAGE_H */
