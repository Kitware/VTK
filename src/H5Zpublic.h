/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Programmer:  Robb Matzke
 *              Thursday, April 16, 1998
 */

#ifndef H5Zpublic_H
#define H5Zpublic_H

/* Public headers needed by this file */
#include "H5public.h"

/**
 * \brief Filter identifiers
 *
 * \details Values 0 through 255 are for filters defined by the HDF5 library.
 *          Values 256 through 511 are available for testing new filters.
 *          Subsequent values should be obtained from the HDF5 development team
 *          at mailto:help@hdfgroup.org. These values will never change because
 *          they appear in the HDF5 files.
 */
typedef int H5Z_filter_t;

/* Filter IDs */
/**
 * no filter
 */
#define H5Z_FILTER_ERROR (-1)
/**
 * reserved indefinitely
 */
#define H5Z_FILTER_NONE 0
/**
 * deflation like gzip
 */
#define H5Z_FILTER_DEFLATE 1
/**
 * shuffle the data
 */
#define H5Z_FILTER_SHUFFLE 2
/**
 * fletcher32 checksum of EDC
 */
#define H5Z_FILTER_FLETCHER32 3
/**
 * szip compression
 */
#define H5Z_FILTER_SZIP 4
/**
 * nbit compression
 */
#define H5Z_FILTER_NBIT 5
/**
 * scale+offset compression
 */
#define H5Z_FILTER_SCALEOFFSET 6
/**
 * filter ids below this value are reserved for library use
 */
#define H5Z_FILTER_RESERVED 256
/**
 * maximum filter id
 */
#define H5Z_FILTER_MAX 65535

/* General macros */
/**
 * Symbol to remove all filters in H5Premove_filter()
 */
#define H5Z_FILTER_ALL 0
/**
 * Maximum number of filters allowed in a pipeline
 *
 * \internal (should probably be allowed to be an unlimited amount, but
 *            currently each filter uses a bit in a 32-bit field, so the format
 *            would have to be changed to accommodate that)
 */
#define H5Z_MAX_NFILTERS 32

/* Flags for filter definition (stored) */
/**
 * definition flag mask
 */
#define H5Z_FLAG_DEFMASK 0x00ff
/**
 * filter is mandatory
 */
#define H5Z_FLAG_MANDATORY 0x0000
/**
 * filter is optional
 */
#define H5Z_FLAG_OPTIONAL 0x0001

/* Additional flags for filter invocation (not stored) */
/**
 * invocation flag mask
 */
#define H5Z_FLAG_INVMASK 0xff00
/**
 * reverse direction; read
 */
#define H5Z_FLAG_REVERSE 0x0100
/**
 * skip EDC filters for read
 */
#define H5Z_FLAG_SKIP_EDC 0x0200

/* Special parameters for szip compression */
/* [These are aliases for the similar definitions in szlib.h, which we can't
 * include directly due to the duplication of various symbols with the zlib.h
 * header file] */
/**
 * \ingroup SZIP */
#define H5_SZIP_ALLOW_K13_OPTION_MASK 1
/**
 * \ingroup SZIP */
#define H5_SZIP_CHIP_OPTION_MASK 2
/**
 * \ingroup SZIP */
#define H5_SZIP_EC_OPTION_MASK 4
/**
 * \ingroup SZIP */
#define H5_SZIP_NN_OPTION_MASK 32
/**
 * \ingroup SZIP */
#define H5_SZIP_MAX_PIXELS_PER_BLOCK 32

/* Macros for the shuffle filter */
/**
 * \ingroup SHUFFLE
 * Number of parameters that users can set for the shuffle filter
 */
#define H5Z_SHUFFLE_USER_NPARMS 0
/**
 * \ingroup SHUFFLE
 * Total number of parameters for the shuffle filter
 */
#define H5Z_SHUFFLE_TOTAL_NPARMS 1

/* Macros for the szip filter */
/**
 * \ingroup SZIP
 * Number of parameters that users can set for SZIP
 */
#define H5Z_SZIP_USER_NPARMS 2
/**
 * \ingroup SZIP
 * Total number of parameters for SZIP filter
 */
#define H5Z_SZIP_TOTAL_NPARMS 4
/**
 * \ingroup SZIP
 * "User" parameter for option mask
 */
#define H5Z_SZIP_PARM_MASK 0
/**
 * \ingroup SZIP
 * "User" parameter for pixels-per-block
 */
#define H5Z_SZIP_PARM_PPB 1
/**
 * \ingroup SZIP
 * "Local" parameter for bits-per-pixel
 */
#define H5Z_SZIP_PARM_BPP 2
/**
 * \ingroup SZIP
 * "Local" parameter for pixels-per-scanline
 */
#define H5Z_SZIP_PARM_PPS 3

/* Macros for the nbit filter */
/**
 * \ingroup NBIT
 * Number of parameters that users can set for the N-bit filter
 */
#define H5Z_NBIT_USER_NPARMS 0 /* Number of parameters that users can set */

/* Macros for the scale offset filter */
/**
 * \ingroup SCALEOFFSET
 * Number of parameters that users can set for the scale-offset filter
 */
#define H5Z_SCALEOFFSET_USER_NPARMS 2

/* Special parameters for ScaleOffset filter*/
/**
 * \ingroup SCALEOFFSET */
#define H5Z_SO_INT_MINBITS_DEFAULT 0
/**
 * \ingroup SCALEOFFSET */
typedef enum H5Z_SO_scale_type_t {
    H5Z_SO_FLOAT_DSCALE = 0,
    H5Z_SO_FLOAT_ESCALE = 1,
    H5Z_SO_INT          = 2
} H5Z_SO_scale_type_t;

/**
 * Current version of the H5Z_class_t struct
 */
#define H5Z_CLASS_T_VERS (1)

/**
 * \ingroup FLETCHER32
 * Values to decide if EDC is enabled for reading data
 */
typedef enum H5Z_EDC_t {
    H5Z_ERROR_EDC   = -1, /**< error value */
    H5Z_DISABLE_EDC = 0,
    H5Z_ENABLE_EDC  = 1,
    H5Z_NO_EDC      = 2 /**< sentinel */
} H5Z_EDC_t;

/* Bit flags for H5Zget_filter_info */
#define H5Z_FILTER_CONFIG_ENCODE_ENABLED (0x0001)
#define H5Z_FILTER_CONFIG_DECODE_ENABLED (0x0002)

/**
 * Return values for filter callback function
 */
typedef enum H5Z_cb_return_t {
    H5Z_CB_ERROR = -1, /**< error value */
    H5Z_CB_FAIL  = 0,  /**< I/O should fail if filter fails. */
    H5Z_CB_CONT  = 1,  /**< I/O continues if filter fails.   */
    H5Z_CB_NO    = 2   /**< sentinel */
} H5Z_cb_return_t;

//! <!-- [H5Z_filter_func_t_snip] -->
/**
 *  Filter callback function definition
 */
typedef H5Z_cb_return_t (*H5Z_filter_func_t)(H5Z_filter_t filter, void *buf, size_t buf_size, void *op_data);
//! <!-- [H5Z_filter_func_t_snip] -->

/**
 * Structure for filter callback property
 */
typedef struct H5Z_cb_t {
    H5Z_filter_func_t func;
    void *            op_data;
} H5Z_cb_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief This callback determines if a filter can be applied to the dataset
 *        with the characteristics provided
 *
 * \dcpl_id
 * \type_id
 * \space_id
 *
 * \return \htri_t
 *
 * \details Before a dataset gets created, the \ref H5Z_can_apply_func_t
 *          callbacks for any filters used in the dataset creation property list
 *          are called with the dataset's dataset creation property list, the
 *          dataset's datatype and a dataspace describing a chunk (for chunked
 *          dataset storage).
 *
 *          The \ref H5Z_can_apply_func_t callback must determine if the
 *          combination of the dataset creation property list setting, the
 *          datatype and the dataspace represent a valid combination to apply
 *          this filter to.  For example, some cases of invalid combinations may
 *          involve the filter not operating correctly on certain datatypes (or
 *          certain datatype sizes), or certain sizes of the chunk dataspace.
 *
 *          The \ref H5Z_can_apply_func_t callback can be the NULL pointer, in
 *          which case, the library will assume that it can apply to any
 *          combination of dataset creation property list values, datatypes and
 *          dataspaces.
 *
 *          The \ref H5Z_can_apply_func_t callback returns positive a valid
 *          combination, zero for an invalid combination and negative for an
 *          error.
 */
//! <!-- [H5Z_can_apply_func_t_snip] -->
typedef htri_t (*H5Z_can_apply_func_t)(hid_t dcpl_id, hid_t type_id, hid_t space_id);
//! <!-- [H5Z_can_apply_func_t_snip] -->
/**
 * \brief The filter operation callback function, defining a filter's operation
 *        on data
 *
 * \dcpl_id
 * \type_id
 * \space_id
 *
 * \return \herr_t
 *
 * \details After the \ref H5Z_can_apply_func_t callbacks are checked for new
 *          datasets, the \ref H5Z_set_local_func_t callbacks for any filters
 *          used in the dataset creation property list are called. These
 *          callbacks receive the dataset's private copy of the dataset creation
 *          property list passed in to H5Dcreate() (i.e. not the actual property
 *          list passed in to H5Dcreate()) and the datatype ID passed in to
 *          H5Dcreate() (which is not copied and should not be modified) and a
 *          dataspace describing the chunk (for chunked dataset storage) (which
 *          should also not be modified).
 *
 *          The \ref H5Z_set_local_func_t callback must set any parameters that
 *          are specific to this dataset, based on the combination of the
 *          dataset creation property list values, the datatype and the
 *          dataspace. For example, some filters perform different actions based
 *          on different datatypes (or datatype sizes) or different number of
 *          dimensions or dataspace sizes.
 *
 *          The \ref H5Z_set_local_func_t callback can be the NULL pointer, in
 *          which case, the library will assume that there are no
 *          dataset-specific settings for this filter.
 *
 *          The \ref H5Z_set_local_func_t callback must return non-negative on
 *          success and negative for an error.
 */
//! <!-- [H5Z_set_local_func_t_snip] -->
typedef herr_t (*H5Z_set_local_func_t)(hid_t dcpl_id, hid_t type_id, hid_t space_id);
//! <!-- [H5Z_set_local_func_t_snip] -->

/**
 * \brief The filter operation callback function, defining a filter's operation
 *        on data
 *
 * \param[in] flags Bit vector specifying certain general properties of the filter
 * \param[in] cd_nelmts Number of elements in \p cd_values
 * \param[in] cd_values Auxiliary data for the filter
 * \param[in] nbytes The number of valid bytes in \p buf to be filtered
 * \param[in,out] buf_size The size of \p buf
 * \param[in,out] buf The filter buffer
 *
 * \return Returns the number of valid bytes of data contained in \p buf. In the
 *         case of failure, the return value is 0 (zero) and all pointer
 *         arguments are left unchanged.
 *
 * \details A filter gets definition flags and invocation flags (defined
 *          above), the client data array and size defined when the filter was
 *          added to the pipeline, the size in bytes of the data on which to
 *          operate, and pointers to a buffer and its allocated size.
 *
 *          The filter should store the result in the supplied buffer if
 *          possible, otherwise it can allocate a new buffer, freeing the
 *          original. The allocated size of the new buffer should be returned
 *          through the \p buf_size pointer and the new buffer through the \p
 *          buf pointer.
 *
 *          The return value from the filter is the number of bytes in the
 *          output buffer. If an error occurs then the function should return
 *          zero and leave all pointer arguments unchanged.
 */
//! <!-- [H5Z_func_t_snip] -->
typedef size_t (*H5Z_func_t)(unsigned int flags, size_t cd_nelmts, const unsigned int cd_values[],
                             size_t nbytes, size_t *buf_size, void **buf);
//! <!-- [H5Z_func_t_snip] -->
/**
 * The filter table maps filter identification numbers to structs that
 * contain a pointers to the filter function and timing statistics.
 */
//! <!-- [H5Z_class2_t_snip] -->
typedef struct H5Z_class2_t {
    int                  version;         /**< Version number of the H5Z_class_t struct     */
    H5Z_filter_t         id;              /**< Filter ID number                             */
    unsigned             encoder_present; /**< Does this filter have an encoder?            */
    unsigned             decoder_present; /**< Does this filter have a decoder?             */
    const char *         name;            /**< Comment for debugging                        */
    H5Z_can_apply_func_t can_apply;       /**< The "can apply" callback for a filter        */
    H5Z_set_local_func_t set_local;       /**< The "set local" callback for a filter        */
    H5Z_func_t           filter;          /**< The actual filter function                   */
} H5Z_class2_t;
//! <!-- [H5Z_class2_t_snip] -->

/**
 * \ingroup H5Z
 *
 * \brief Registers a new filter with the HDF5 library
 *
 * \param[in] cls A pointer to a buffer for the struct containing the
 *                filter-definition
 *
 * \return \herr_t
 *
 * \details H5Zregister() registers a new filter with the HDF5 library.
 *
 * \details Making a new filter available to an application is a two-step
 *          process. The first step is to write the three filter callback
 *          functions described below: \c can_apply, \c set_local, and \c
 *          filter. This call to H5Zregister(), registering the filter with the
 *          library, is the second step. The can_apply and set_local fields can
 *          be set to NULL if they are not required for the filter being
 *          registered.
 *
 *          H5Zregister() accepts a single parameter, a pointer to a buffer for
 *          the \p cls data structure. That data structure must conform to one
 *          of the following definitions:
 *          \snippet this H5Z_class1_t_snip
 *          or
 *          \snippet this H5Z_class2_t_snip
 *
 *          \c version is a library-defined value reporting the version number
 *          of the #H5Z_class_t struct. This currently must be set to
 *          #H5Z_CLASS_T_VERS.
 *
 *          \c id is the identifier for the new filter. This is a user-defined
 *          value between #H5Z_FILTER_RESERVED and #H5Z_FILTER_MAX. These
 *          values are defined in the HDF5 source file H5Zpublic.h, but the
 *          symbols #H5Z_FILTER_RESERVED and #H5Z_FILTER_MAX should always be
 *          used instead of the literal values.
 *
 *          \c encoder_present is a library-defined value indicating whether
 *          the filter’s encoding capability is available to the application.
 *
 *          \c decoder_present is a library-defined value indicating whether
 *          the filter’s encoding capability is available to the application.
 *
 *          \c name is a descriptive comment used for debugging, may contain a
 *          descriptive name for the filter, and may be the null pointer.
 *
 *          \c can_apply, described in detail below, is a user-defined callback
 *          function which determines whether the combination of the dataset
 *          creation property list values, the datatype, and the dataspace
 *          represent a valid combination to apply this filter to.
 *
 *          \c set_local, described in detail below, is a user-defined callback
 *          function which sets any parameters that are specific to this
 *          dataset, based on the combination of the dataset creation property
 *          list values, the datatype, and the dataspace.
 *
 *          \c filter, described in detail below, is a user-defined callback
 *          function which performs the action of the filter.
 *
 *          The statistics associated with a filter are not reset by this
 *          function; they accumulate over the life of the library.
 *
 *          #H5Z_class_t is a macro which maps to either H5Z_class1_t or
 *          H5Z_class2_t, depending on the needs of the application. To affect
 *          only this macro, H5Z_class_t_vers may be defined to either 1 or 2.
 *          Otherwise, it will behave in the same manner as other API
 *          compatibility macros. See API Compatibility Macros in HDF5 for more
 *          information. H5Z_class1_t matches the #H5Z_class_t structure that is
 *          used in the 1.6.x versions of the HDF5 library.
 *
 *          H5Zregister() will automatically detect which structure type has
 *          been passed in, regardless of the mapping of the #H5Z_class_t macro.
 *          However, the application must make sure that the fields are filled
 *          in according to the correct structure definition if the macro is
 *          used to declare the structure.
 *
 *          \Bold{The callback functions:}\n Before H5Zregister() can link a
 *          filter into an application, three callback functions must be
 *          defined as described in the HDF5 library header file H5Zpublic.h.
 *
 *          When a filter is applied to the fractal heap for a group (e.g.,
 *          when compressing group metadata) and if the can apply and set local
 *          callback functions have been defined for that filter, HDF5 passes
 *          the value -1 for all parameters for those callback functions. This
 *          is done to ensure that the filter will not be applied to groups if
 *          it relies on these parameters, as they are not applicable to group
 *          fractal heaps; to operate on group fractal heaps, a filter must be
 *          capable of operating on an opaque block of binary data.
 *
 *          The \Emph{can apply} callback function must return a positive value
 *          for a valid combination, zero for an invalid combination, and a
 *          negative value for an error.
 *          \snippet this H5Z_can_apply_func_t_snip
 *
 *          Before a dataset is created, the \Emph{can apply} callbacks for any
 *          filters used in the dataset creation property list are called with
 *          the dataset's dataset creation property list, \c dcpl_id, the
 *          dataset's datatype, \p type_id, and a dataspace describing a chunk,
 *          \p space_id, (for chunked dataset storage).
 *
 *          This callback must determine whether the combination of the dataset
 *          creation property list settings, the datatype, and the dataspace
 *          represent a valid combination to which to apply this filter. For
 *          example, an invalid combination may involve the filter not
 *          operating correctly on certain datatypes, on certain datatype
 *          sizes, or on certain sizes of the chunk dataspace. If this filter
 *          is enabled through H5Pset_filter() as optional and the can apply
 *          function returns 0, the library will skip the filter in the filter
 *          pipeline.
 *
 *          This callback can be the NULL pointer, in which case the library
 *          will assume that the filter can be applied to a dataset with any
 *          combination of dataset creation property list values, datatypes,
 *          and dataspaces.
 *
 *          The \Emph{set local} callback function is defined as follows:
 *          \snippet this H5Z_set_local_func_t_snip
 *
 *          After the can apply callbacks are checked for a new dataset, the
 *          \Emph{set local} callback functions for any filters used in the
 *          dataset creation property list are called. These callbacks receive
 *          \c dcpl_id, the dataset's private copy of the dataset creation
 *          property list passed in to H5Dcreate() (i.e. not the actual
 *          property list passed in to H5Dcreate()); \c type_id, the datatype
 *          identifier passed in to H5Dcreate(), which is not copied and should
 *          not be modified; and \c space_id, a dataspace describing the chunk
 *          (for chunked dataset storage), which should also not be modified.
 *
 *          The set local callback must set any filter parameters that are
 *          specific to this dataset, based on the combination of the dataset
 *          creation property list values, the datatype, and the dataspace. For
 *          example, some filters perform different actions based on different
 *          datatypes, datatype sizes, numbers of dimensions, or dataspace
 *          sizes.
 *
 *          The \Emph{set local} callback may be the NULL pointer, in which
 *          case, the library will assume that there are no dataset-specific
 *          settings for this filter.
 *
 *          The \Emph{set local} callback function must return a non-negative
 *          value on success and a negative value for an error.
 *
 *          The \Emph{filter operation} callback function, defining the
 *          filter's operation on the data, is defined as follows:
 *          \snippet this H5Z_func_t_snip
 *
 *          The parameters \c flags, \c cd_nelmts, and \c cd_values are the
 *          same as for the function H5Pset_filter(). The one exception is that
 *          an additional flag, #H5Z_FLAG_REVERSE, is set when the filter is
 *          called as part of the input pipeline.
 *
 *          The parameter \c buf points to the input buffer which has a size of
 *          \c buf_size bytes, \c nbytes of which are valid data.
 *
 *          The filter should perform the transformation in place if possible.
 *          If the transformation cannot be done in place, then the filter
 *          should allocate a new buffer with malloc() and assign it to \c buf,
 *          assigning the allocated size of that buffer to \c buf_size. The old
 *          buffer should be freed by calling free().
 *
 *          If successful, the \Emph{filter operation} callback function
 *          returns the number of valid bytes of data contained in \c buf. In
 *          the case of failure, the return value is 0 (zero) and all pointer
 *          arguments are left unchanged.
 *
 * \version 1.8.6 Return type for the \Emph{can apply} callback function,
 *                \ref H5Z_can_apply_func_t, changed to \ref htri_t.
 * \version 1.8.5 Semantics of the \Emph{can apply} and \Emph{set local}
 *                callback functions changed to accommodate the use of filters
 *                with group fractal heaps.
 * \version 1.8.3 #H5Z_class_t renamed to H5Z_class2_t, H5Z_class1_t structure
 *                introduced for backwards compatibility with release 1.6.x,
 *                and #H5Z_class_t macro introduced in this release. Function
 *                modified to accept either structure type.
 * \version 1.8.0 The fields \c version, \c encoder_present, and
 *                \c decoder_present were added to the #H5Z_class_t \c struct
 *                in this release.
 * \version 1.6.0 This function was substantially revised in Release 1.6.0 with
 *                a new #H5Z_class_t struct and new set local and can apply
 *                callback functions.
 *
 */
H5_DLL herr_t H5Zregister(const void *cls);
/**
 * \ingroup H5Z
 *
 * \brief Unregisters a filter.
 *
 * \param[in] id Identifier of the filter to be unregistered.
 * \return \herr_t
 *
 * \details H5Zunregister() unregisters the filter specified in \p id.
 *
 * \details This function first iterates through all opened datasets and
 *          groups. If an open object that uses this filter is found, the
 *          function will fail with a message indicating that an object using
 *          the filter is still open. All open files are then flushed to make
 *          sure that all cached data that may use this filter are written out.
 *
 *          If the application is a parallel program, all processes that
 *          participate in collective data write should call this function to
 *          ensure that all data is flushed.
 *
 *          After a call to H5Zunregister(), the filter specified in filter
 *          will no longer be available to the application.
 *
 * \version 1.8.12 Function modified to check for open objects using the
 *                 filter.
 * \since 1.6.0
 */
H5_DLL herr_t H5Zunregister(H5Z_filter_t id);
/**
 * \ingroup H5Z
 *
 * \brief Determines whether a filter is available
 *
 * \param[in] id Filter identifier
 * \return \htri_t
 *
 * \details H5Zfilter_avail() determines whether the filter specified in \p id
 *          is available to the application.
 *
 * \since 1.6.0
 */
H5_DLL htri_t H5Zfilter_avail(H5Z_filter_t id);
/**
 * \ingroup H5Z
 *
 * \brief Retrieves information about a filter
 *
 * \param[in] filter Filter identifier
 * \param[out] filter_config_flags A bit field encoding the returned filter
 *                                 information
 * \return \herr_t
 *
 * \details H5Zget_filter_info() retrieves information about a filter. At
 *          present, this means that the function retrieves a filter's
 *          configuration flags, indicating whether the filter is configured to
 *          decode data, to encode data, neither, or both.
 *
 *          If \p filter_config_flags is not set to NULL prior to the function
 *          call, the returned parameter contains a bit field specifying the
 *          available filter configuration. The configuration flag values can
 *          then be determined through a series of bitwise AND operations, as
 *          described below.
 *
 *          Valid filter configuration flags include the following:
 *          <table>
 *            <tr><td>#H5Z_FILTER_CONFIG_ENCODE_ENABLED</td>
 *                <td>Encoding is enabled for this filter</td></tr>
 *            <tr><td>#H5Z_FILTER_CONFIG_DECODE_ENABLED</td>
 *                <td>Decoding is enabled for this filter</td></tr>
 *          </table>
 *
 *          A bitwise AND of the returned \p filter_config_flags and a valid
 *          filter configuration flag will reveal whether the related
 *          configuration option is available. For example, if the value of
 *          \code
 *          H5Z_FILTER_CONFIG_ENCODE_ENABLED & filter_config_flags
 *          \endcode
 *          is true, i.e., greater than 0 (zero), the queried filter
 *          is configured to encode data; if the value is \c FALSE, i.e., equal to
 *          0 (zero), the filter is not so configured.
 *
 *          If a filter is not encode-enabled, the corresponding \c H5Pset_*
 *          function will return an error if the filter is added to a dataset
 *          creation property list (which is required if the filter is to be
 *          used to encode that dataset). For example, if the
 *          #H5Z_FILTER_CONFIG_ENCODE_ENABLED flag is not returned for the SZIP
 *          filter, #H5Z_FILTER_SZIP, a call to H5Pset_szip() will fail.
 *
 *          If a filter is not decode-enabled, the application will not be able
 *          to read an existing file encoded with that filter.
 *
 *          This function should be called, and the returned \p
 *          filter_config_flags analyzed, before calling any other function,
 *          such as H5Pset_szip() , that might require a particular filter
 *          configuration.
 *
 * \since 1.6.3
 */
H5_DLL herr_t H5Zget_filter_info(H5Z_filter_t filter, unsigned int *filter_config_flags);

/* Symbols defined for compatibility with previous versions of the HDF5 API.
 *
 * Use of these symbols is deprecated.
 */
#ifndef H5_NO_DEPRECATED_SYMBOLS

/**
 * The filter table maps filter identification numbers to structs that
 * contain a pointers to the filter function and timing statistics.
 */
//! <!-- [H5Z_class1_t_snip] -->
typedef struct H5Z_class1_t {
    H5Z_filter_t         id;        /**< Filter ID number			     */
    const char *         name;      /**< Comment for debugging		     */
    H5Z_can_apply_func_t can_apply; /**< The "can apply" callback for a filter */
    H5Z_set_local_func_t set_local; /**< The "set local" callback for a filter */
    H5Z_func_t           filter;    /**< The actual filter function		     */
} H5Z_class1_t;
//! <!-- [H5Z_class1_t_snip] -->

#endif /* H5_NO_DEPRECATED_SYMBOLS */

#ifdef __cplusplus
}
#endif
#endif
