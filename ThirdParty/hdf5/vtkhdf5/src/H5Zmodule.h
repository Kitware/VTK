/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the LICENSE file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Purpose: This file contains declarations which define macros for the
 *          H5Z package.  Including this header means that the source file
 *          is part of the H5Z package.
 */
#ifndef H5Zmodule_H
#define H5Zmodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5Z_MODULE
#define H5_MY_PKG      H5Z
#define H5_MY_PKG_INIT YES

/** \page H5Z_UG HDF5 Filters
 *
 * Navigate back: \ref index "Main" / \ref UG
 * <hr>
 *
 * \section sec_filter The HDF5 Filter Interface
 *
 * \subsection subsec_filter_intro Introduction
 *
 * The HDF5 Filter interface (H5Z) provides a flexible pipeline mechanism for processing
 * dataset data during I/O operations. Filters can perform data compression, error checking,
 * data transformation, and other custom operations on dataset chunks.
 *
 * Filters operate on chunked datasets only (see \ref subsec_dataset_filters for details on
 * dataset chunking) and are applied independently to each chunk. Multiple filters can be
 * chained together in a pipeline, where the output of one filter becomes the input to the next.
 *
 * \subsection subsec_filter_builtin Built-in Filters
 *
 * HDF5 includes several standard filters:
 *
 * \li \Bold{DEFLATE (gzip)}: General-purpose compression using the gzip algorithm
 *     (#H5Z_FILTER_DEFLATE). Provides good compression ratios with moderate CPU usage.
 *
 * \li \Bold{SZIP}: Compression algorithm designed for scientific data (#H5Z_FILTER_SZIP).
 *     Offers better performance than DEFLATE for certain data patterns.
 *
 * \li \Bold{SHUFFLE}: Rearranges byte order to improve compression (#H5Z_FILTER_SHUFFLE).
 *     Typically used before compression filters.
 *
 * \li \Bold{FLETCHER32}: Computes and verifies checksums for error detection
 *     (#H5Z_FILTER_FLETCHER32). Ensures data integrity.
 *
 * \li \Bold{NBIT}: Lossless compression for datasets with unused bits (#H5Z_FILTER_NBIT).
 *
 * \li \Bold{SCALEOFFSET}: Lossy compression using scaling and offset (#H5Z_FILTER_SCALEOFFSET).
 *
 * \subsection subsec_filter_usage Using Filters
 *
 * Filters are configured through dataset creation property lists. Enable chunking first
 * using #H5Pset_chunk, then add compression with functions like #H5Pset_deflate.
 *
 * \subsection subsec_filter_pipeline Filter Pipelines
 *
 * Multiple filters can be combined in a pipeline. Filters are applied in the order
 * they are added during write operations and in reverse order during read operations.
 * Common pipelines combine #H5Pset_shuffle, #H5Pset_deflate, and #H5Pset_fletcher32.
 *
 * \subsection subsec_filter_custom Custom Filters
 *
 * Applications can create and register custom filters:
 *
 * \li Define a filter function with signature matching #H5Z_func_t
 * \li Create a #H5Z_class_t structure describing the filter
 * \li Register the filter using #H5Zregister
 * \li Apply the filter using #H5Pset_filter with the filter ID
 *
 * Custom filters enable domain-specific data transformations, specialized compression
 * algorithms, encryption, and other custom processing.
 *
 * \subsection subsec_filter_query Querying Filters
 *
 * The H5Z interface provides functions to query available filters:
 *
 * \li #H5Zfilter_avail checks if a filter is available
 * \li #H5Zget_filter_info retrieves information about a filter's capabilities
 * \li #H5Zunregister removes a filter from the pipeline
 *
 * \subsection subsec_filter_plugins Filter Plugins
 *
 * HDF5 supports dynamic loading of filter plugins, allowing filters to be added
 * without recompiling applications. See \ref sec_filter_plugins for details on
 * creating and using filter plugins.
 *
 * \subsection subsec_filter_summary Summary
 *
 * The H5Z filter interface provides:
 * \li Built-in compression and error checking filters
 * \li Flexible filter pipeline mechanism
 * \li Support for custom user-defined filters
 * \li Dynamic filter plugin loading
 * \li Per-chunk processing for optimal performance
 *
 * Filters are essential for reducing storage requirements and ensuring data integrity
 * in HDF5 files while maintaining compatibility and performance.
 *
 * <hr>
 * Navigate back: \ref index "Main" / \ref UG
 */

/**
 * \defgroup H5Z Filters (H5Z)
 *
 * Use the functions in this module to manage HDF5 filters.
 *
 * User-defined filters are created by registering a filter descriptor of
 * type #H5Z_class_t with the library.
 *
 * Available filters can be read or examined at runtime.
 *
 * It is conceivable that filters are stateful and that that state be
 * updated at runtime.
 *
 * Filters are deleted by unregistering.
 *
 * <table>
 * <tr><th>Create</th><th>Read</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5Z_examples.c filter
 *   \snippet{lineno} H5Z_examples.c create
 *   </td>
 *   <td>
 *   \snippet{lineno} H5Z_examples.c read
 *   </td>
 * </tr>
 * <tr><th>Update</th><th>Delete</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5Z_examples.c update
 *   </td>
 *   <td>
 *   \snippet{lineno} H5Z_examples.c delete
 * </tr>
 * </table>
 *
 * HDF5 supports a filter pipeline that provides the capability for standard and
 * customized raw data processing during I/O operations.  HDF5 is distributed
 * with a small set of standard filters such as compression (gzip, SZIP, and a
 * shuffling algorithm) and error checking (Fletcher32 checksum). For further
 * flexibility, the library allows a user application to extend the pipeline
 * through the creation and registration of customized filters.
 * See \ref sec_filter_plugins
 *
 * The flexibility of the filter pipeline implementation enables the definition
 * of additional filters by a user application. A filter
 * \li is associated with a dataset when the dataset is created,
 * \li can be used only with chunked data (i.e., datasets stored in the
 *     #H5D_CHUNKED storage layout), and
 * \li is applied independently to each chunk of the dataset.
 *
 * The HDF5 library does not support filters for contiguous datasets because of
 * the difficulty of implementing random access for partial I/O. Compact dataset
 * filters are not supported because they would not produce significant results.
 *
 * HDF5 allows chunked data to pass through user-defined filters
 * on the way to or from disk.  The filters operate on chunks of an
 * #H5D_CHUNKED dataset can be arranged in a pipeline
 * so output of one filter becomes the input of the next filter.
 *
 * Each filter has a two-byte identification number (type
 * #H5Z_filter_t) allocated by The HDF Group and can also be
 * passed application-defined integer resources to control its
 * behavior.  Each filter also has an optional ASCII comment
 * string.
 *
 * \snippet{doc} H5Zpublic.h FiltersIdTable
 *
 * Filter identifiers for the filters distributed with the HDF5 Library are as follows:
//! [PreDefFilters]
<table>
<tr><td>#H5Z_FILTER_DEFLATE</td><td>The gzip compression, or deflation, filter</td></tr>
<tr><td>#H5Z_FILTER_SZIP</td><td>The SZIP compressionfilter</td></tr>
<tr><td>#H5Z_FILTER_NBIT</td><td>The N-bit compression filter</td></tr>
<tr><td>#H5Z_FILTER_SCALEOFFSET</td><td>The scale-offset compression filter</td></tr>
<tr><td>#H5Z_FILTER_SHUFFLE</td><td>The shuffle algorithm filter</td></tr>
<tr><td>#H5Z_FILTER_FLETCHER32</td><td>The Fletcher32 checksum, or error checking, filter</td></tr>
</table>
//! [PreDefFilters]
 *
 * Custom filters that have been registered with the library will have
 * additional unique identifiers.
 *
 * See \ref sec_filter_plugins for more information on how an HDF5 application can
 * apply a filter that is not registered with the HDF5 library.
 *
 * \defgroup H5ZPRE Predefined Filters
 * \ingroup H5Z
 * \defgroup FLETCHER32 Checksum Filter
 * \ingroup H5ZPRE
 * \defgroup SCALEOFFSET Scale-Offset Filter
 * \ingroup H5ZPRE
 * \defgroup SHUFFLE Shuffle Filter
 * \ingroup H5ZPRE
 * \defgroup SZIP Szip Filter
 * \ingroup H5ZPRE
 * \defgroup NBIT N-bit Filter
 * \ingroup H5ZPRE
 *
 */

#endif /* H5Zmodule_H */
