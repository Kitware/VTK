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
 *          H5O package.  Including this header means that the source file
 *          is part of the H5O package.
 */
#ifndef H5Omodule_H
#define H5Omodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5O_MODULE
#define H5_MY_PKG      H5O
#define H5_MY_PKG_INIT YES

/** \page H5O_UG HDF5 Objects
 *
 * Navigate back: \ref index "Main" / \ref UG
 * <hr>
 *
 * \section sec_object The HDF5 Object Interface
 *
 * \subsection subsec_object_intro Introduction
 *
 * The HDF5 Object interface (H5O) provides functions for managing HDF5 objects at a generic level.
 * While specific object types (groups, datasets, datatypes) have their own interfaces (\ref H5G,
 * \ref H5D, \ref H5T), the H5O interface enables operations that apply to all object types uniformly.
 *
 * @see H5O Reference Manual
 *
 * \subsection subsec_object_ops Object Operations
 *
 * The H5O interface provides several categories of operations:
 *
 * \li \Bold{Object Information}: Retrieve metadata about objects using #H5Oget_info, #H5Oget_info_by_name,
 *     #H5Oget_info_by_idx, and #H5Oget_native_info. These functions return information such as
 *     reference counts, modification times, and storage details.
 *
 * \li \Bold{Object Copying}: Copy objects between locations or files using #H5Ocopy. This creates
 *     a complete copy of the source object including all its attributes and, for groups, can
 *     optionally copy contained objects recursively.
 *
 * \li \Bold{Object Opening}: Open objects by address or index using #H5Oopen, #H5Oopen_by_idx,
 *     or #H5Oopen_by_token. This enables access to objects when the specific type is unknown.
 *
 * \li \Bold{Object Linking}: Create links to objects using #H5Olink, enabling multiple names
 *     to reference the same object.
 *
 * \subsection subsec_object_visit Object Traversal
 *
 * The H5O interface provides powerful traversal capabilities:
 *
 * \li #H5Ovisit recursively visits all objects in a group hierarchy
 * \li #H5Ovisit_by_name performs recursive visitation starting from a named location
 *
 * These functions invoke a user-supplied callback for each object encountered, enabling
 * custom processing, cataloging, or analysis of file contents.
 *
 * \subsection subsec_object_comments Object Comments
 *
 * Objects can have associated comment strings for documentation purposes:
 *
 * \li #H5Oset_comment and #H5Oset_comment_by_name attach comments to objects
 * \li #H5Oget_comment and #H5Oget_comment_by_name retrieve object comments
 *
 * \subsection subsec_object_refcount Reference Counting
 *
 * HDF5 uses reference counting to manage object lifetimes. The library automatically
 * deletes objects when their reference count reaches zero:
 *
 * \li #H5Oincr_refcount manually increments an object's reference count
 * \li #H5Odecr_refcount manually decrements an object's reference count
 *
 * \attention Manual manipulation of reference counts should be used with extreme caution.
 * Improper use can lead to memory leaks or premature object deletion.
 *
 * \subsection subsec_object_cache Cache Control
 *
 * For performance optimization, the H5O interface provides cache management functions:
 *
 * \li #H5Orefresh reloads object metadata from disk, discarding cached data
 * \li #H5Oflush writes object metadata to disk immediately
 *
 * These functions are particularly useful in SWMR (Single Writer Multiple Reader) scenarios.
 *
 * \subsection subsec_object_summary Summary
 *
 * The H5O interface provides essential generic object operations:
 * \li Query object metadata and properties across all object types
 * \li Copy objects within and between HDF5 files
 * \li Traverse object hierarchies with custom processing
 * \li Manage object comments for documentation
 * \li Control object caching and persistence
 *
 * <hr>
 * Navigate back: \ref index "Main" / \ref UG
 */

/**
 * \defgroup H5O Objects (H5O)
 *
 * Use the functions in this module to manage HDF5 objects.
 *
 * HDF5 objects (groups, datasets, datatype objects) are usually created
 * using functions in the object-specific modules (\ref H5G, \ref H5D,
 * \ref H5T). However, new objects can also be created by copying existing
 * objects.
 *
 * Many functions in this module are variations on object introspection,
 * that is, the retrieval of detailed information about HDF5 objects in a file.
 * Objects in an HDF5 file can be "visited" in an iterative fashion.
 *
 * HDF5 objects are usually updated using functions in the object-specific
 * modules. However, there are certain generic object properties, such as
 * reference counts, that can be manipulated using functions in this module.
 *
 * HDF5 objects are deleted as a side effect of rendering them unreachable
 * from the root group. The net effect is the diminution of the object's
 * reference count to zero, which can (but should not usually) be affected
 * by a function in this module.
 *
 * <table>
 * <tr><th>Create</th><th>Read</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5O_examples.c create
 *   </td>
 *   <td>
 *   \snippet{lineno} H5O_examples.c read
 *   </td>
 * <tr><th>Update</th><th>Delete</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5O_examples.c update
 *   </td>
 *   <td>
 *   \snippet{lineno} H5O_examples.c delete
 *   </td>
 * </tr>
 * </table>
 *
 */
#endif /* H5Omodule_H */
