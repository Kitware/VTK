/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Programmer:	Quincey Koziol
 *		Saturday, September 12, 2015
 *
 * Purpose:	This file contains declarations which define macros for the
 *		H5Z package.  Including this header means that the source file
 *		is part of the H5Z package.
 */
#ifndef H5Zmodule_H
#define H5Zmodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5Z_MODULE
#define H5_MY_PKG      H5Z
#define H5_MY_PKG_ERR  H5E_PLINE
#define H5_MY_PKG_INIT YES

/**
 * \defgroup H5Z H5Z
 *
 *
 * \brief Filter and Compression Interface
 *
 * \details The functions in this module let you configure filters that process
 *          data during I/O operation.
 *
 *          HDF5 supports a filter pipeline that provides the capability for
 *          standard and customized raw data processing during I/O operations.
 *          HDF5 is distributed with a small set of standard filters such as
 *          compression (gzip, SZIP, and a shuffling algorithm) and error
 *          checking (Fletcher32 checksum). For further flexibility, the
 *          library allows a user application to extend the pipeline through
 *          the creation and registration of customized filters.
 *
 *          The flexibility of the filter pipeline implementation enables the
 *          definition of additional filters by a user application. A filter
 *          \li is associated with a dataset when the dataset is created,
 *          \li can be used only with chunked data (i.e., datasets stored in
 *              the #H5D_CHUNKED storage layout), and
 *          \li is applied independently to each chunk of the dataset.
 *
 *          The HDF5 library does not support filters for contiguous datasets
 *          because of the difficulty of implementing random access for partial
 *          I/O. Compact dataset filters are not supported because it would not
 *          produce significant results.
 *
 *          Filter identifiers for the filters distributed with the HDF5
 *          Library are as follows:
 *          <table>
 *            <tr><td>#H5Z_FILTER_DEFLATE</td><td>The gzip compression, or
 *                    deflation, filter</td></tr>
 *            <tr><td>#H5Z_FILTER_SZIP</td><td>The SZIP compression
 *                    filter</td></tr>
 *            <tr><td>#H5Z_FILTER_NBIT</td><td>The N-bit compression
 *                    filter</td></tr>
 *            <tr><td>#H5Z_FILTER_SCALEOFFSET</td><td>The scale-offset
 *                    compression filter</td></tr>
 *            <tr><td>#H5Z_FILTER_SHUFFLE</td><td>The shuffle algorithm
 *                    filter</td></tr>
 *            <tr><td>#H5Z_FILTER_FLETCHER32</td><td>The Fletcher32 checksum,
 *                    or error checking, filter</td></tr>
 *          </table>
 *          Custom filters that have been registered with the library will have
 *          additional unique identifiers.
 *
 *          See \ref_dld_filters for more information on how an HDF5
 *          application can apply a filter that is not registered with the HDF5
 *          library.
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
 *
 */

#endif /* H5Zmodule_H */
