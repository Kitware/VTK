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
 *		H5P package.  Including this header means that the source file
 *		is part of the H5P package.
 */
#ifndef H5Pmodule_H
#define H5Pmodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5P_MODULE
#define H5_MY_PKG      H5P
#define H5_MY_PKG_ERR  H5E_PLIST
#define H5_MY_PKG_INIT YES

/**\defgroup H5P H5P
 *
 * Use the functions in this module to manage HDF5 property lists and property
 * list classes. HDF5 property lists are the main vehicle to configure the
 * behavior of HDF5 API functions.
 *
 * Typically, property lists are created by instantiating one of the built-in
 * or user-defined property list classes. After adding suitable properties,
 * property lists are used when opening or creating HDF5 items, or when reading
 * or writing data. Property lists can be modified by adding or changing
 * properties. Property lists are deleted by closing the associated handles.
 *
 * <table>
 * <tr><th>Create</th><th>Read</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5P_examples.c create
 *   </td>
 *   <td>
 *   \snippet{lineno} H5P_examples.c read
 *   </td>
 * <tr><th>Update</th><th>Delete</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5P_examples.c update
 *   </td>
 *   <td>
 *   \snippet{lineno} H5P_examples.c delete
 *   </td>
 * </tr>
 * </table>
 *
 * \defgroup ALCAPL Attribute and Link Creation Properties
 * \ingroup H5P
 * Currently, there are only two creation properties that you can use to control
 * the creation of HDF5 attributes and links. The first creation property, the
 * choice of a character encoding, applies to both attributes and links.
 * The second creation property applies to links only, and advises the library
 * to automatically create missing intermediate groups when creating new objects.
 *
 * \defgroup DAPL Dataset Access Properties
 * \ingroup H5P
 * Use dataset access properties to modify the default behavior of the HDF5
 * library when accessing datasets. The properties include adjusting the size
 * of the chunk cache, providing prefixes for external content and virtual
 * dataset file paths, and controlling flush behavior, etc. These properties
 * are \Emph{not} persisted with datasets, and can be adjusted at runtime before
 * a dataset is created or opened.
 *
 * \defgroup DCPL Dataset Creation Properties
 * \ingroup H5P
 * Use dataset creation properties to control aspects of dataset creation such
 * as fill time, storage layout, compression methods, etc.
 * Unlike dataset access and transfer properties, creation properties \Emph{are}
 * stored with the dataset, and cannot be changed once a dataset has been
 * created.
 *
 * \defgroup DXPL Dataset Transfer Properties
 * \ingroup H5P
 * Use dataset transfer properties to customize certain aspects of reading
 * and writing datasets such as transformations, MPI-IO I/O mode, error
 * detection, etc. These properties are \Emph{not} persisted with datasets,
 * and can be adjusted at runtime before a dataset is read or written.
 *
 * \defgroup FAPL File Access Properties
 * \ingroup H5P
 * Use file access properties to modify the default behavior of the HDF5
 * library when accessing files. The properties include selecting a virtual
 * file driver (VFD), configuring the metadata cache (MDC), control
 * file locking, etc. These properties are \Emph{not} persisted with files, and
 * can be adjusted at runtime before a file is created or opened.
 *
 * \defgroup FCPL File Creation Properties
 * \ingroup H5P
 * Use file creation properties to control aspects of file creation such
 * as setting a file space management strategy or creating a user block.
 * Unlike file access properties, creation properties \Emph{are}
 * stored with the file, and cannot be changed once a file has been
 * created.
 *
 * \defgroup GAPL General Access Properties
 * \ingroup H5P
 * The functions in this section can be applied to different kinds of property
 * lists.
 *
 * \defgroup GCPL Group Creation Properties
 * \ingroup H5P
 * Use group creation properties to control aspects of group creation such
 * as storage layout, compression, and link creation order tracking.
 * Unlike file access properties, creation properties \Emph{are}
 * stored with the group, and cannot be changed once a group has been
 * created.
 *
 * \defgroup GPLO General Property List Operations
 * \ingroup H5P
 *
 * Use the functions in this module to manage HDF5 property lists.
 *
 * <table>
 * <tr><th>Create</th><th>Read</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5P_examples.c create
 *   </td>
 *   <td>
 *   \snippet{lineno} H5P_examples.c read
 *   </td>
 * <tr><th>Update</th><th>Delete</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5P_examples.c update
 *   </td>
 *   <td>
 *   \snippet{lineno} H5P_examples.c delete
 *   </td>
 * </tr>
 * </table>
 *
 * \defgroup GPLOA General Property List Operations (Advanced)
 * \ingroup H5P
 *
 * You can create and customize user-defined property list classes using the
 * functions described below. Arbitrary user-defined properties can also
 * be inserted into existing property lists as so-called temporary properties.
 *
 * <table>
 * <tr><th>Create</th><th>Read</th></tr>
 *
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5P_examples.c create_class
 *   </td>
 *   <td>
 *   \snippet{lineno} H5P_examples.c read_class
 *   </td>
 * <tr><th>Update</th><th>Delete</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5P_examples.c update_class
 *   </td>
 *   <td>
 *   \snippet{lineno} H5P_examples.c delete_class
 *   </td>
 * </tr>
 * </table>
 *
 * \defgroup LAPL Link Access Properties
 * \ingroup H5P
 *
 *
 * \defgroup MAPL Map Access Properties
 * \ingroup H5P

 * \defgroup OCPL Object Creation Properties
 * \ingroup H5P
 *
 *
 * \defgroup OCPPL Object Copy Properties
 * \ingroup H5P
 *
 *
 */

#endif /* H5Pmodule_H */
