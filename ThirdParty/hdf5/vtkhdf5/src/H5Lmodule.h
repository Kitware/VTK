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
 *          H5L package.  Including this header means that the source file
 *          is part of the H5L package.
 */
#ifndef H5Lmodule_H
#define H5Lmodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5L_MODULE
#define H5_MY_PKG      H5L
#define H5_MY_PKG_INIT YES

/** \page H5L_UG HDF5 Links
 *
 * Navigate back: \ref index "Main" / \ref UG
 * <hr>
 *
 * \section sec_link The HDF5 Link Interface
 *
 * \subsection subsec_link_intro Introduction
 *
 * The HDF5 Link interface (H5L) provides functions for creating, querying, and manipulating
 * links between objects in an HDF5 file. Links are the primary mechanism for organizing objects
 * in the HDF5 file hierarchy and enabling navigation between related data.
 *
 * \subsection subsec_link_types Link Types
 *
 * HDF5 supports three types of links:
 *
 * \li \Bold{Hard Links}: Direct references to objects within the same file. An object exists
 *     as long as at least one hard link points to it. Hard links cannot cross file boundaries.
 *
 * \li \Bold{Soft Links (Symbolic Links)}: Path-based references that store the path to an object
 *     as a string. Soft links can "dangle" if the target object is deleted or moved.
 *
 * \li \Bold{External Links}: References to objects in other HDF5 files. External links store
 *     both the filename and the path to the object within that file.
 *
 * \subsection subsec_link_create Creating Links
 *
 * Links can be created using several methods:
 *
 * \li #H5Lcreate_hard creates hard links to objects
 * \li #H5Lcreate_soft creates soft (symbolic) links using path strings
 * \li #H5Lcreate_external creates links to objects in other HDF5 files
 *
 * \subsection subsec_link_ops Link Operations
 *
 * The H5L interface provides functions for:
 *
 * \li \Bold{Querying}: #H5Lexists checks if a link exists, #H5Lget_info retrieves link information
 * \li \Bold{Moving/Copying}: #H5Lmove and #H5Lcopy relocate or duplicate links
 * \li \Bold{Deleting}: #H5Ldelete removes links from the file
 * \li \Bold{Iterating}: #H5Literate and #H5Lvisit traverse links in groups
 *
 * \subsection subsec_link_traverse Link Traversal
 *
 * The H5L interface includes powerful traversal functions that can iterate over all links
 * in a group or recursively visit all links in a group hierarchy:
 *
 * \li #H5Literate iterates over links in a group using a group identifier
 * \li #H5Lvisit recursively visits all links in a group and its subgroups using a group identifier
 * \li #H5Literate_by_name and #H5Lvisit_by_name provide the same functionality but accept a
 *     location identifier and group name, allowing traversal without first opening the group
 *
 * These functions accept callbacks that are invoked for each link, enabling custom processing.
 *
 * \subsection subsec_link_user User-Defined Link Types
 *
 * HDF5 allows registration of custom link types through #H5Lregister. User-defined links
 * can implement custom traversal and access behaviors. Use #H5Lunregister to remove
 * user-defined link classes and #H5Lis_registered to check if a link class is registered.
 *
 * \subsection subsec_link_summary Summary
 *
 * The H5L interface provides flexible mechanisms for organizing and navigating HDF5 files:
 * \li Hard links provide reference-counted object management
 * \li Soft links enable flexible, path-based references
 * \li External links connect objects across multiple files
 * \li Iteration functions enable systematic traversal of file hierarchies
 * \li User-defined links support extensibility for custom use cases
 *
 * <hr>
 * Navigate back: \ref index "Main" / \ref UG
 */

/**
 * \defgroup H5L Links (H5L)
 *
 * Use the functions in this module to manage HDF5 links and link types.
 * @see \ref TRAV for #H5Literate, #H5Literate_by_name and #H5Lvisit, #H5Lvisit_by_name
 * @see \ref H5LA for #H5Lregister, #H5Lunregister and #H5Lis_registered
 *
 * \defgroup TRAV Link Traversal
 * \ingroup H5L
 * Traverse through links
 *
 * \defgroup H5LA Advanced Link Functions
 * \ingroup H5L
 * Registration of User-defined links
 */

#endif /* H5Lmodule_H */
