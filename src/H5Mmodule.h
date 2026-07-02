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
 *              H5M package.  Including this header means that the source file
 *              is part of the H5M package.
 */
#ifndef H5Mmodule_H
#define H5Mmodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5M_MODULE
#define H5_MY_PKG      H5M
#define H5_MY_PKG_INIT YES

/**
 * \page H5M_UG HDF5 VOL Data Mapping
 *
 * Navigate back: \ref index "Main" / \ref UG
 * <hr>
 *
 * \Bold{The HDF5 Data Mapping can only be used with the HDF5 VOL connectors that
 * implement map objects.} The native HDF5 library does not support this feature.
 *
 * \section sec_map HDF5 Map Object
 *
 * \subsection subsec_map_intro Introduction
 *
 * While the HDF5 data model provides a flexible way to store structured data, some applications
 * require a more general mechanism for indexing information with arbitrary keys. HDF5 Map objects
 * address this need by providing application-defined key-value stores where key-value pairs can
 * be added, retrieved by key, and iterated over.
 *
 * Map objects are available only through VOL connectors that implement the map interface. The
 * native HDF5 file format does not support map objects. VOL connectors with map support include
 * DAOS and other storage systems optimized for key-value operations.
 *
 * @see H5M Reference Manual
 *
 * \subsection subsec_map_lifecycle Map Object Life Cycle
 *
 * Map objects follow a well-defined life cycle from creation through cleanup:
 *
 * <ol>
 * <li><b>Creation</b>
 *     <ul>
 *     <li>Create a new map with #H5Mcreate() or #H5Mcreate_anon():
 *         <ul>
 *         <li>#H5Mcreate() creates a map and links it into the file hierarchy with a specified name</li>
 *         <li>#H5Mcreate_anon() creates an anonymous map that must be linked with #H5Olink before
 * closing</li>
 *         </ul>
 *     </li>
 *     <li>Specify the key datatype and value datatype during creation - these define how keys and values
 *         are stored in the map</li>
 *     <li>Optionally provide creation and access property lists (MCPL and MAPL) to control map behavior</li>
 *     <li>The function returns a map identifier (hid_t) used for all subsequent operations</li>
 *     </ul>
 * </li>
 *
 * <li><b>Opening</b>
 *     <ul>
 *     <li>Open an existing map with #H5Mopen() by providing its location and name</li>
 *     <li>Optionally specify a map access property list (MAPL) to control access behavior</li>
 *     <li>Multiple opens of the same map are allowed and each returns a separate identifier</li>
 *     </ul>
 * </li>
 *
 * <li><b>Data Operations</b>
 *     <ul>
 *     <li><b>Adding/Updating</b>: Use #H5Mput() to add new key-value pairs or update existing values
 *         <ul>
 *         <li>Specify memory datatypes for the key and value being written</li>
 *         <li>If the key already exists, its value is updated</li>
 *         <li>If the key is new, a new key-value pair is added</li>
 *         </ul>
 *     </li>
 *     <li><b>Retrieving</b>: Use #H5Mget() to retrieve a value by its key
 *         <ul>
 *         <li>Provide a buffer to receive the value</li>
 *         <li>Specify memory datatypes for key and value</li>
 *         <li>Type conversion is performed if memory and stored datatypes differ</li>
 *         </ul>
 *     </li>
 *     <li><b>Checking Existence</b>: Use #H5Mexists() to check if a key exists without retrieving its value
 *     </li>
 *     <li><b>Deleting</b>: Use #H5Mdelete() to remove a key-value pair from the map
 *     </li>
 *     <li><b>Iterating</b>: Use #H5Miterate() or #H5Miterate_by_name() to process all key-value pairs
 *         <ul>
 *         <li>Provide a callback function that is invoked for each key-value pair</li>
 *         <li>Iteration order is determined by the VOL connector implementation</li>
 *         <li>Can start iteration from a specific index position</li>
 *         </ul>
 *     </li>
 *     </ul>
 * </li>
 *
 * <li><b>Query Operations</b>
 *     <ul>
 *     <li>#H5Mget_count() retrieves the number of key-value pairs stored in the map</li>
 *     <li>#H5Mget_key_type() returns the datatype used for keys</li>
 *     <li>#H5Mget_val_type() returns the datatype used for values</li>
 *     <li>#H5Mget_create_plist() retrieves the map creation property list (MCPL)</li>
 *     <li>#H5Mget_access_plist() retrieves the map access property list (MAPL)</li>
 *     </ul>
 * </li>
 *
 * <li><b>Closing</b>
 *     <ul>
 *     <li>Close the map with #H5Mclose() when finished</li>
 *     <li>All map identifiers should be closed before closing the file</li>
 *     <li>Closing a map does not delete it from the file</li>
 *     <li>The library maintains reference counts and will not actually close the map until all
 *         identifiers are closed</li>
 *     </ul>
 * </li>
 * </ol>
 *
 * \subsection subsec_map_plist Map Property Lists
 *
 * Map objects use two types of property lists that fit into the HDF5 property list class hierarchy
 * (see \ref subsubsec_plist_class):
 *
 * <ul>
 * <li><b>Map Creation Property List (MCPL) - #H5P_MAP_CREATE</b>
 *     <ul>
 *     <li>Controls properties that are set when a map is created</li>
 *     <li>These properties are permanent characteristics of the map</li>
 *     <li>Inherits from the Object Creation Property List (OCPL) class</li>
 *     <li>Can be retrieved later with #H5Mget_create_plist()</li>
 *     <li>Default MCPL: #H5P_MAP_CREATE_DEFAULT or #H5P_DEFAULT</li>
 *     </ul>
 * </li>
 *
 * <li><b>Map Access Property List (MAPL) - #H5P_MAP_ACCESS</b>
 *     <ul>
 *     <li>Controls properties for accessing an existing map</li>
 *     <li>These properties affect how the map is opened and accessed</li>
 *     <li>Settings can vary between different opens of the same map</li>
 *     <li>Can be retrieved with #H5Mget_access_plist()</li>
 *     <li>Default MAPL: #H5P_MAP_ACCESS_DEFAULT or #H5P_DEFAULT</li>
 *     </ul>
 * </li>
 * </ul>
 *
 * These property list classes follow the same inheritance hierarchy as other HDF5 objects.
 * The MCPL inherits properties relevant to object creation (like character encoding for names),
 * while the MAPL controls access-specific settings.
 *
 * \subsection subsec_map_datatypes Key and Value Datatypes
 *
 * Map objects require two datatypes:
 *
 * <ul>
 * <li><b>Key Datatype</b>: Defines how keys are stored in the map
 *     <ul>
 *     <li>Can be any valid HDF5 datatype (integer, float, string, compound, etc.)</li>
 *     <li>Variable-length strings (#H5T_C_S1 with #H5T_VARIABLE size) are commonly used for keys</li>
 *     <li>Fixed at map creation time</li>
 *     </ul>
 * </li>
 *
 * <li><b>Value Datatype</b>: Defines how values are stored in the map
 *     <ul>
 *     <li>Can be any valid HDF5 datatype</li>
 *     <li>All values in a map must have the same datatype</li>
 *     <li>Fixed at map creation time</li>
 *     </ul>
 * </li>
 * </ul>
 *
 * During #H5Mput() and #H5Mget() operations, memory datatypes can differ from the stored datatypes.
 * The HDF5 library will perform type conversion as needed, similar to dataset I/O operations.
 *
 * \subsection subsec_map_example Example Usage
 *
 * The following example demonstrates creating a map, adding key-value pairs, and retrieving values.
 * Note that this example requires a VOL connector with map support (e.g., DAOS).
 * See \ref H5VL_UG for VOL connector configuration details.
 *
 * \code
 * hid_t file_id, map_id, vls_type_id, fapl_id;
 * const char *names[2] = {"Alice", "Bob"};
 * uint64_t IDs[2] = {25385486, 34873275};
 * uint64_t val_out;
 * herr_t ret;
 *
 * // Setup file access property list with VOL connector that supports maps
 * // (Configuration depends on specific VOL connector - see connector documentation)
 * fapl_id = H5Pcreate(H5P_FILE_ACCESS);
 * // ... configure VOL connector in fapl_id ...
 *
 * // Create variable-length string datatype for keys
 * vls_type_id = H5Tcopy(H5T_C_S1);
 * H5Tset_size(vls_type_id, H5T_VARIABLE);
 *
 * // Create file
 * file_id = H5Fcreate("file.h5", H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);
 *
 * // Create map with string keys and uint64 values
 * map_id = H5Mcreate(file_id, "map", vls_type_id, H5T_NATIVE_UINT64,
 *                    H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
 *
 * // Add key-value pairs
 * H5Mput(map_id, vls_type_id, &names[0], H5T_NATIVE_UINT64, &IDs[0], H5P_DEFAULT);
 * H5Mput(map_id, vls_type_id, &names[1], H5T_NATIVE_UINT64, &IDs[1], H5P_DEFAULT);
 *
 * // Retrieve a value by key
 * ret = H5Mget(map_id, vls_type_id, &names[0], H5T_NATIVE_UINT64, &val_out, H5P_DEFAULT);
 * if(ret < 0 || val_out != IDs[0]) {
 *     fprintf(stderr, "Failed to retrieve correct value from map\n");
 *     goto error;
 * }
 *
 * // Close map and other objects
 * H5Mclose(map_id);
 * H5Tclose(vls_type_id);
 * H5Pclose(fapl_id);
 * H5Fclose(file_id);
 * return 0;
 *
 * error:
 *     H5E_BEGIN_TRY {
 *         H5Mclose(map_id);
 *         H5Tclose(vls_type_id);
 *         H5Pclose(fapl_id);
 *         H5Fclose(file_id);
 *     } H5E_END_TRY;
 *     return -1;
 * \endcode
 *
 * \subsection subsec_map_notes Important Notes
 *
 * <ul>
 * <li><b>VOL Connector Requirement</b>: Map functionality is only available through VOL connectors
 *     that implement the map interface. The native HDF5 file format does not support maps.</li>
 *
 * <li><b>Experimental API</b>: The H5M interface is experimental and subject to change in future releases.
 *     Application code using maps should be prepared for potential API modifications.</li>
 *
 * <li><b>Key Uniqueness</b>: Each key in a map must be unique. Calling #H5Mput() with an existing
 *     key will update that key's value rather than creating a duplicate entry.</li>
 *
 * <li><b>Iteration Order</b>: The order in which key-value pairs are returned during iteration
 *     is determined by the VOL connector implementation and may not be predictable.</li>
 *
 * <li><b>Type Conversion</b>: As with datasets, the library performs datatype conversion between
 *     memory and storage representations. Ensure memory buffers are sized appropriately for the
 *     memory datatype specified.</li>
 *
 * <li><b>Asynchronous Operations</b>: Several map operations have asynchronous variants (e.g.,
 *     #H5Mcreate_async, #H5Mopen_async) for use with the asynchronous I/O interface.</li>
 * </ul>
 *
 * Previous Chapter \ref sec_async - Next Chapter \ref sec_reference
 *
 * <hr>
 * Navigate back: \ref index "Main" / \ref UG
 *
 */

/**
 * \defgroup H5M VOL Mapping (H5M)
 *
 * \details \Bold{The interface can only be used with the HDF5 VOL connectors that
 *          implement map objects.} The native HDF5 library does not support this
 *          feature.
 *
 *          While the HDF5 data model is a flexible way to store data, some
 *          applications require a more general way to index information. HDF5
 *          effectively uses key-value stores internally for a variety of
 *          purposes, but it does not expose a generic key-value store to the
 *          API. The Map APIs provide this capability to the HDF5 applications
 *          in the form of HDF5 map objects. These Map objects contain
 *          application-defined key-value stores, to which key-value pairs can
 *          be added, and from which values can be retrieved by key.
 *
 *          HDF5 VOL connectors with support for map objects:
 *          - DAOS
 *
 * \par Example:
 * \code
 * // NOTE: This example requires a VOL connector with map support (e.g., DAOS)
 * hid_t file_id, fapl_id, map_id, vls_type_id;
 * const char *names[2] = {"Alice", "Bob"};
 * uint64_t IDs[2] = {25385486, 34873275};
 * uint64_t val_out;
 * herr_t ret;
 *
 * // Setup file access property list with VOL connector that supports maps
 * fapl_id = H5Pcreate(H5P_FILE_ACCESS);
 * // ... configure VOL connector in fapl_id (see connector documentation) ...
 *
 * vls_type_id = H5Tcopy(H5T_C_S1);
 * H5Tset_size(vls_type_id, H5T_VARIABLE);
 * file_id = H5Fcreate("file.h5", H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);
 * map_id = H5Mcreate(file_id, "map", vls_type_id, H5T_NATIVE_UINT64,
 *                    H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
 * H5Mput(map_id, vls_type_id, &names[0], H5T_NATIVE_UINT64, &IDs[0], H5P_DEFAULT);
 * H5Mput(map_id, vls_type_id, &names[1], H5T_NATIVE_UINT64, &IDs[1], H5P_DEFAULT);
 * ret = H5Mget(map_id, vls_type_id, &names[0], H5T_NATIVE_UINT64, &val_out, H5P_DEFAULT);
 * if(ret < 0 || val_out != IDs[0]) {
 *     fprintf(stderr, "Map retrieval failed\n");
 *     // Handle error...
 * }
 * H5Mclose(map_id);
 * H5Tclose(vls_type_id);
 * H5Pclose(fapl_id);
 * H5Fclose(file_id);
 * \endcode
 *
 */

#endif /* H5Dmodule_H */
