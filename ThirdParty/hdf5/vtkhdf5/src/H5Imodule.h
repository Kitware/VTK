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
 * Purpose:     This file contains declarations which define macros for the
 *              H5I package.  Including this header means that the source file
 *              is part of the H5I package.
 */
#ifndef H5Imodule_H
#define H5Imodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 * reporting macros.
 */
#define H5I_MODULE
#define H5_MY_PKG      H5I
#define H5_MY_PKG_INIT NO

/** \page H5I_UG HDF5 Identifiers
 *
 * Navigate back: \ref index "Main" / \ref UG
 * <hr>
 *
 * \section sec_identifier The HDF5 Identifier Interface
 *
 * \subsection subsec_identifier_intro Introduction
 *
 * The HDF5 Identifier interface (H5I) manages identifiers (type #hid_t) that serve as handles
 * to HDF5 objects and resources. Identifiers provide an abstraction layer between applications
 * and the internal HDF5 library structures, enabling safe and efficient object management.
 *
 * Every HDF5 object—file, group, dataset, datatype, dataspace, attribute, property list—is
 * accessed through an identifier. The H5I interface provides functions to query, validate,
 * and manage these identifiers and their associated resources.
 *
 * \subsection subsec_identifier_types Identifier Types
 *
 * HDF5 defines several built-in identifier types:
 *
 * \li #H5I_FILE - File identifiers
 * \li #H5I_GROUP - Group identifiers
 * \li #H5I_DATATYPE - Datatype identifiers
 * \li #H5I_DATASPACE - Dataspace identifiers
 * \li #H5I_DATASET - Dataset identifiers
 * \li #H5I_ATTR - Attribute identifiers
 * \li #H5I_MAP - Map identifiers
 * \li #H5I_VFL - Virtual File Layer driver identifiers
 * \li #H5I_VOL - Virtual Object Layer connector identifiers
 * \li Property list identifiers (various classes)
 *
 * Each identifier type is managed independently with its own reference counting system.
 *
 * \subsection subsec_identifier_refcount Reference Counting
 *
 * Identifiers use reference counting to manage object lifetimes. When an identifier
 * is created, its reference count is set to 1. The reference count can be manipulated
 * to control when resources are released:
 *
 * \li #H5Iget_ref retrieves the current reference count
 * \li #H5Iinc_ref increments the reference count
 * \li #H5Idec_ref decrements the reference count
 *
 * When a reference count reaches zero, the associated resources are automatically released.
 *
 * \subsection subsec_identifier_valid Identifier Validation
 *
 * The H5I interface provides functions to validate identifiers:
 *
 * \li #H5Iis_valid checks if an identifier is valid and has a positive reference count
 * \li #H5Iget_type retrieves the type of an identifier
 * \li #H5Itype_exists checks if an identifier type is registered
 *
 * \subsection subsec_identifier_query Querying Identifiers
 *
 * Applications can retrieve information about objects through their identifiers:
 *
 * \li #H5Iget_name retrieves the name of an object (for files, groups, datasets, etc.)
 * \li #H5Iget_file_id retrieves the file identifier associated with any object
 *
 * These functions are particularly useful for debugging and logging.
 *
 * \subsection subsec_identifier_user User-Defined Identifier Types
 *
 * Advanced applications can register custom identifier types using #H5Iregister.
 * This allows user-defined objects to benefit from HDF5's identifier management:
 *
 * \li Automatic reference counting
 * \li Type safety through identifier types
 * \li Integration with HDF5's resource management
 *
 * Custom identifier types can be destroyed using #H5Idestroy_type when no longer needed.
 *
 * \subsection subsec_identifier_iteration Identifier Iteration
 *
 * The H5I interface allows iteration over all identifiers of a given type:
 *
 * \li #H5Iiterate iterates over identifiers of a specified type with a callback function
 * \li #H5Inmembers returns the number of identifiers of a given type
 *
 * This is useful for resource tracking, debugging, and cleanup operations.
 *
 * \subsection subsec_identifier_summary Summary
 *
 * The H5I identifier interface provides essential functionality:
 * \li Safe handles to HDF5 objects and resources
 * \li Reference counting for automatic resource management
 * \li Type checking and validation
 * \li Object information retrieval
 * \li Support for user-defined identifier types
 * \li Iteration capabilities for resource tracking
 *
 * Proper identifier management is fundamental to writing robust HDF5 applications
 * that efficiently manage memory and avoid resource leaks.
 *
 * <hr>
 * Navigate back: \ref index "Main" / \ref UG
 */

/**
 * \defgroup H5I Identifiers (H5I)
 *
 * Use the functions in this module to manage identifiers defined by the HDF5
 * library. See \ref H5IUD for user-defined identifiers and identifier
 * types.
 *
 * HDF5 identifiers are usually created as a side-effect of creating HDF5
 * entities such as groups, datasets, attributes, or property lists.
 *
 * Identifiers defined by the HDF5 library can be used to retrieve information
 * such as path names and reference counts, and their validity can be checked.
 *
 * Identifiers can be updated by manipulating their reference counts.
 *
 * Unused identifiers should be reclaimed by closing the associated item, e.g.,
 * HDF5 object, or decrementing the reference count to 0.
 *
 * \note Identifiers (of type \ref hid_t) are run-time auxiliaries and
 * not persisted in the file.
 *
 * <table>
 * <tr><th>Create</th><th>Read</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5I_examples.c create
 *   </td>
 *   <td>
 *   \snippet{lineno} H5I_examples.c read
 *   </td>
 * <tr><th>Update</th><th>Delete</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5I_examples.c update
 *   </td>
 *   <td>
 *   \snippet{lineno} H5I_examples.c delete
 *   </td>
 * </tr>
 * </table>
 *
 * \defgroup H5IUD User-defined ID Types
 * \ingroup H5I
 *
 * The \ref H5I module contains functions to define new identifier types.
 * For convenience, handles of type \ref hid_t can then be associated with the
 * new identifier types and user objects.
 *
 * New identifier types can be created by registering a new identifier type
 * with the HDF5 library. Once a new identifier type has bee registered,
 * it can be used to generate identifiers for user objects.
 *
 * User-defined identifier types can be searched and iterated.
 *
 * Like library-defined identifiers, user-defined identifiers \Emph{and}
 * identifier types are reference counted, and the reference counts can be
 * manipulated accordingly.
 *
 * User-defined identifiers no longer in use should be deleted or reclaimed,
 * and identifier types should be destroyed if they are no longer required.
 *
 * <table>
 * <tr><th>Create</th><th>Read</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5I_examples.c create_ud
 *   </td>
 *   <td>
 *   \snippet{lineno} H5I_examples.c read_ud
 *   </td>
 * <tr><th>Update</th><th>Delete</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5I_examples.c update_ud
 *   </td>
 *   <td>
 *   \snippet{lineno} H5I_examples.c delete_ud
 *   </td>
 * </tr>
 * </table>
 *
 */

#endif /* H5Imodule_H */
