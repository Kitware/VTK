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
 * Purpose: This file contains declarations which define macros for the
 *          H5F package.  Including this header means that the source file
 *          is part of the H5F package.
 */
#ifndef H5Fmodule_H
#define H5Fmodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5F_MODULE
#define H5_MY_PKG     H5F
#define H5_MY_PKG_ERR H5E_FILE

/** \page H5F_UG HDF5 File
 *
 * \section sec_file The HDF5 File
 * \subsection subsec_file_intro Introduction
 * The purpose of this chapter is to describe how to work with HDF5 data files.
 *
 * If HDF5 data is to be written to or read from a file, the file must first be explicitly created or
 * opened with the appropriate file driver and access privileges. Once all work with the file is
 * complete, the file must be explicitly closed.
 *
 * This chapter discusses the following:
 * \li File access modes
 * \li Creating, opening, and closing files
 * \li The use of file creation property lists
 * \li The use of file access property lists
 * \li The use of low-level file drivers
 *
 * This chapter assumes an understanding of the material presented in the data model chapter. For
 * more information, see \ref sec_data_model.
 *
 * \subsection subsec_file_access_modes File Access Modes
 * There are two issues regarding file access:
 * <ul><li>What should happen when a new file is created but a file of the same name already
 * exists? Should the create action fail, or should the existing file be overwritten?</li>
 * <li>Is a file to be opened with read-only or read-write access?</li></ul>
 *
 * Four access modes address these concerns. Two of these modes can be used with #H5Fcreate, and
 * two modes can be used with #H5Fopen.
 * \li #H5Fcreate accepts #H5F_ACC_EXCL or #H5F_ACC_TRUNC
 * \li #H5Fopen accepts #H5F_ACC_RDONLY or #H5F_ACC_RDWR
 *
 * The access modes are described in the table below.
 *
 * <table>
 * <caption>Access flags and modes</caption>
 * <tr>
 * <th>Access Flag</th>
 * <th>Resulting Access Mode</th>
 * </tr>
 * <tr>
 * <td>#H5F_ACC_EXCL</td>
 * <td>If the file already exists, #H5Fcreate fails. If the file does not exist,
 * it is created and opened with read-write access. (Default)</td>
 * </tr>
 * <tr>
 * <td>#H5F_ACC_TRUNC</td>
 * <td>If the file already exists, the file is opened with read-write access,
 * and new data will overwrite any existing data. If the file does not exist,
 * it is created and opened with read-write access.</td>
 * </tr>
 * <tr>
 * <td>#H5F_ACC_RDONLY</td>
 * <td>An existing file is opened with read-only access. If the file does not
 * exist, #H5Fopen fails. (Default)</td>
 * </tr>
 * <tr>
 * <td>#H5F_ACC_RDWR</td>
 * <td>An existing file is opened with read-write access. If the file does not
 * exist, #H5Fopen fails.</td>
 * </tr>
 * </table>
 *
 * By default, #H5Fopen opens a file for read-only access; passing #H5F_ACC_RDWR allows
 * read-write access to the file.
 *
 * By default, #H5Fcreate fails if the file already exists; only passing #H5F_ACC_TRUNC allows
 * the truncating of an existing file.
 *
 * \subsection subsec_file_creation_access File Creation and File Access Properties
 * File creation and file access property lists control the more complex aspects of creating and
 * accessing files.
 *
 * File creation property lists control the characteristics of a file such as the size of the userblock,
 * a user-definable data block; the size of data address parameters; properties of the B-trees that are
 * used to manage the data in the file; and certain HDF5 Library versioning information.
 *
 * For more information, see \ref subsubsec_file_property_lists_props.
 *
 * This section has a more detailed discussion of file creation properties. If you have no special
 * requirements for these file characteristics, you can simply specify #H5P_DEFAULT for the default
 * file creation property list when a file creation property list is called for.
 *
 * File access property lists control properties and means of accessing a file such as data alignment
 * characteristics, metadata block and cache sizes, data sieve buffer size, garbage collection
 * settings, and parallel I/O. Data alignment, metadata block and cache sizes, and data sieve buffer
 * size are factors in improving I/O performance.
 *
 * For more information, see \ref subsubsec_file_property_lists_access.
 *
 * This section has a more detailed discussion of file access properties. If you have no special
 * requirements for these file access characteristics, you can simply specify #H5P_DEFAULT for the
 * default file access property list when a file access property list is called for.
 *
 * <table>
 * <caption>Figure 10 - More sample file structures</caption>
 * <tr>
 * <td>
 * \image html UML_FileAndProps.gif "UML model for an HDF5 file and its property lists"
 * </td>
 * </tr>
 * </table>
 *
 * \subsection subsec_file_drivers Low-level File Drivers
 * The concept of an HDF5 file is actually rather abstract: the address space for what is normally
 * thought of as an HDF5 file might correspond to any of the following at the storage level:
 * \li Single file on a standard file system
 * \li Multiple files on a standard file system
 * \li Multiple files on a parallel file system
 * \li Block of memory within an application's memory space
 * \li More abstract situations such as virtual files
 *
 * This HDF5 address space is generally referred to as an HDF5 file regardless of its organization at
 * the storage level.
 *
 * HDF5 accesses a file (the address space) through various types of low-level file drivers. The
 * default HDF5 file storage layout is as an unbuffered permanent file which is a single, contiguous
 * file on local disk. Alternative layouts are designed to suit the needs of a variety of systems,
 * environments, and applications.
 *
 * \subsection subsec_file_program_model Programming Model for Files
 * Programming models for creating, opening, and closing HDF5 files are described in the
 * sub-sections below.
 *
 * \subsubsection subsubsec_file_program_model_create Creating a New File
 * The programming model for creating a new HDF5 file can be summarized as follows:
 * \li Define the file creation property list
 * \li Define the file access property list
 * \li Create the file
 *
 * First, consider the simple case where we use the default values for the property lists. See the
 * example below.
 *
 * <em>Creating an HDF5 file using property list defaults</em>
 * \code
 *   file_id = H5Fcreate ("SampleFile.h5", H5F_ACC_EXCL, H5P_DEFAULT, H5P_DEFAULT)
 * \endcode
 *
 * Note: The example above specifies that #H5Fcreate should fail if SampleFile.h5 already exists.
 *
 * A more complex case is shown in the example below. In this example, we define file creation
 * and access property lists (though we do not assign any properties), specify that #H5Fcreate
 * should fail if SampleFile.h5 already exists, and create a new file named SampleFile.h5. The example
 * does not specify a driver, so the default driver, #H5FD_SEC2, will be used.
 *
 * <em>Creating an HDF5 file using property lists</em>
 * \code
 *   fcplist_id = H5Pcreate (H5P_FILE_CREATE)
 *   <...set desired file creation properties...>
 *   faplist_id = H5Pcreate (H5P_FILE_ACCESS)
 *   <...set desired file access properties...>
 *   file_id = H5Fcreate ("SampleFile.h5", H5F_ACC_EXCL, fcplist_id, faplist_id)
 * \endcode
 * Notes:
 * 1. A root group is automatically created in a file when the file is first created.
 *
 * 2. File property lists, once defined, can be reused when another file is created within the same
 * application.
 *
 * \subsubsection subsubsec_file_program_model_open Opening an Existing File
 * The programming model for opening an existing HDF5 file can be summarized as follows:
 * <ul><li>Define or modify the file access property list including a low-level file driver (optional)</li>
 * <li>Open the file</li></ul>
 *
 * The code in the example below shows how to open an existing file with read-only access.
 *
 * <em>Opening an HDF5 file</em>
 * \code
 *   faplist_id = H5Pcreate (H5P_FILE_ACCESS)
 *   status = H5Pset_fapl_stdio (faplist_id)
 *   file_id = H5Fopen ("SampleFile.h5", H5F_ACC_RDONLY, faplist_id)
 * \endcode
 *
 * \subsubsection subsubsec_file_program_model_close Closing a File
 * The programming model for closing an HDF5 file is very simple:
 * \li Close file
 *
 * We close SampleFile.h5 with the code in the example below.
 *
 * <em>Closing an HDF5 file</em>
 * \code
 *   status = H5Fclose (file_id)
 * \endcode
 * Note that #H5Fclose flushes all unwritten data to storage and that file_id is the identifier returned
 * for SampleFile.h5 by #H5Fopen.
 *
 * More comprehensive discussions regarding all of these steps are provided below.
 *
 * \subsection subsec_file_h5dump Using h5dump to View a File
 * h5dump is a command-line utility that is included in the HDF5 distribution. This program
 * provides a straight-forward means of inspecting the contents of an HDF5 file. You can use
 * h5dump to verify that a program is generating the intended HDF5 file. h5dump displays ASCII
 * output formatted according to the HDF5 DDL grammar.
 *
 * The following h5dump command will display the contents of SampleFile.h5:
 * \code
 *   h5dump SampleFile.h5
 * \endcode
 *
 * If no datasets or groups have been created in and no data has been written to the file, the output
 * will look something like the following:
 * \code
 *    HDF5 "SampleFile.h5" {
 *    GROUP "/" {
 *    }
 *    }
 * \endcode
 *
 * Note that the root group, indicated above by <b>/</b>, was automatically created when the file was created.
 *
 * h5dump is described on the
 * \ref subsecViewToolsViewContent_h5dump
 * page under
 * \ref ViewToolsCommand.
 * The HDF5 DDL grammar is described in the document \ref DDLBNF114.
 *
 * \subsection subsec_file_summary File Function Summaries
 * General library (\ref H5 functions and macros), (\ref H5F functions), file related
 * (\ref H5P functions), and file driver (\ref H5P functions) are listed below.
 *
 * <table>
 * <caption>General library functions and macros</caption>
 * <tr>
 * <th>Function</th>
 * <th>Purpose</th>
 * </tr>
 * <tr>
 * <td>#H5check_version</td>
 * <td>Verifies that HDF5 library versions are consistent.</td>
 * </tr>
 * <tr>
 * <td>#H5close</td>
 * <td>Flushes all data to disk, closes all open identifiers, and cleans up memory.</td>
 * </tr>
 * <tr>
 * <td>#H5dont_atexit</td>
 * <td>Instructs the library not to install the atexit cleanup routine.</td>
 * </tr>
 * <tr>
 * <td>#H5garbage_collect</td>
 * <td>Performs garbage collection (GC) on all free-lists of all types.</td>
 * </tr>
 * <tr>
 * <td>#H5get_libversion</td>
 * <td>Returns the HDF library release number.</td>
 * </tr>
 * <tr>
 * <td>#H5open</td>
 * <td>Initializes the HDF5 library.</td>
 * </tr>
 * <tr>
 * <td>#H5set_free_list_limits</td>
 * <td>Sets free-list size limits.</td>
 * </tr>
 * <tr>
 * <td>#H5_VERSION_GE</td>
 * <td>Determines whether the version of the library being used is greater than or equal
 * to the specified version.</td>
 * </tr>
 * <tr>
 * <td>#H5_VERSION_LE</td>
 * <td>Determines whether the version of the library being used is less than or equal
 * to the specified version.</td>
 * </tr>
 * </table>
 *
 * <table>
 * <caption>File functions </caption>
 * <tr>
 * <th>Function</th>
 * <th>Purpose</th>
 * </tr>
 * <tr>
 * <td>#H5Fclear_elink_file_cache</td>
 * <td>Clears the external link open file cache for a file.</td>
 * </tr>
 * <tr>
 * <td>#H5Fclose</td>
 * <td>Closes HDF5 file.</td>
 * </tr>
 * <tr>
 * <td>#H5Fcreate</td>
 * <td>Creates new HDF5 file.</td>
 * </tr>
 * <tr>
 * <td>#H5Fflush</td>
 * <td>Flushes data to HDF5 file on storage medium.</td>
 * </tr>
 * <tr>
 * <td>#H5Fget_access_plist</td>
 * <td>Returns a file access property list identifier.</td>
 * </tr>
 * <tr>
 * <td>#H5Fget_create_plist</td>
 * <td>Returns a file creation property list identifier.</td>
 * </tr>
 * <tr>
 * <td>#H5Fget_file_image</td>
 * <td>Retrieves a copy of the image of an existing, open file.</td>
 * </tr>
 * <tr>
 * <td>#H5Fget_filesize</td>
 * <td>Returns the size of an HDF5 file.</td>
 * </tr>
 * <tr>
 * <td>#H5Fget_freespace</td>
 * <td>Returns the amount of free space in a file.</td>
 * </tr>
 * <tr>
 * <td>#H5Fget_info</td>
 * <td>Returns global information for a file.</td>
 * </tr>
 * <tr>
 * <td>#H5Fget_intent</td>
 * <td>Determines the read/write or read-only status of a file.</td>
 * </tr>
 * <tr>
 * <td>#H5Fget_mdc_config</td>
 * <td>Obtains current metadata cache configuration for target file.</td>
 * </tr>
 * <tr>
 * <td>#H5Fget_mdc_hit_rate</td>
 * <td>Obtains target file's metadata cache hit rate.</td>
 * </tr>
 * <tr>
 * <td>#H5Fget_mdc_size</td>
 * <td>Obtains current metadata cache size data for specified file.</td>
 * </tr>
 * <tr>
 * <td>#H5Fget_mpi_atomicity</td>
 * <td>Retrieves the atomicity mode in use.</td>
 * </tr>
 * <tr>
 * <td>#H5Fget_name</td>
 * <td>Retrieves the name of the file to which the object belongs.</td>
 * </tr>
 * <tr>
 * <td>#H5Fget_obj_count</td>
 * <td>Returns the number of open object identifiers for an open file.</td>
 * </tr>
 * <tr>
 * <td>#H5Fget_obj_ids</td>
 * <td>Returns a list of open object identifiers.</td>
 * </tr>
 * <tr>
 * <td>#H5Fget_vfd_handle</td>
 * <td>Returns pointer to the file handle from the virtual file driver.</td>
 * </tr>
 * <tr>
 * <td>#H5Fis_hdf5</td>
 * <td>Determines whether a file is in the HDF5 format.</td>
 * </tr>
 * <tr>
 * <td>#H5Fmount</td>
 * <td>Mounts a file.</td>
 * </tr>
 * <tr>
 * <td>#H5Fopen</td>
 * <td>Opens an existing HDF5 file.</td>
 * </tr>
 * <tr>
 * <td>#H5Freopen</td>
 * <td>Returns a new identifier for a previously-opened HDF5 file.</td>
 * </tr>
 * <tr>
 * <td>#H5Freset_mdc_hit_rate_stats</td>
 * <td>Resets hit rate statistics counters for the target file.</td>
 * </tr>
 * <tr>
 * <td>#H5Fset_mdc_config</td>
 * <td>Configures metadata cache of target file.</td>
 * </tr>
 * <tr>
 * <td>#H5Fset_mpi_atomicity</td>
 * <td>Sets the MPI atomicity mode.</td>
 * </tr>
 * <tr>
 * <td>#H5Funmount</td>
 * <td>Unmounts a file.</td>
 * </tr>
 * </table>
 *
 * \anchor fcpl_table_tag File creation property list functions (H5P)
 * <div>
 * \snippet{doc} tables/propertyLists.dox fcpl_table
 * </div>
 *
 * \anchor fapl_table_tag File access property list functions (H5P)
 * <div>
 * \snippet{doc} tables/propertyLists.dox fapl_table
 * </div>
 *
 * \anchor fd_pl_table_tag File driver property list functions (H5P)
 * <div>
 * \snippet{doc} tables/propertyLists.dox fd_pl_table
 * </div>
 *
 * \subsection subsec_file_create Creating or Opening an HDF5 File
 * This section describes in more detail how to create and how to open files.
 *
 * New HDF5 files are created and opened with #H5Fcreate; existing files are opened with
 * #H5Fopen. Both functions return an object identifier which must eventually be released by calling
 * #H5Fclose.
 *
 * To create a new file, call #H5Fcreate:
 * \code
 *   hid_t H5Fcreate (const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id)
 * \endcode
 *
 * #H5Fcreate creates a new file named name in the current directory. The file is opened with read
 * and write access; if the #H5F_ACC_TRUNC flag is set, any pre-existing file of the same name in
 * the same directory is truncated. If #H5F_ACC_TRUNC is not set or #H5F_ACC_EXCL is set and
 * if a file of the same name exists, #H5Fcreate will fail.
 *
 * The new file is created with the properties specified in the property lists fcpl_id and fapl_id.
 * fcpl is short for file creation property list. fapl is short for file access property list. Specifying
 * #H5P_DEFAULT for either the creation or access property list will use the library's default
 * creation or access properties.
 *
 * If #H5Fcreate successfully creates the file, it returns a file identifier for the new file. This
 * identifier will be used by the application any time an object identifier, an OID, for the file is
 * required. Once the application has finished working with a file, the identifier should be released
 * and the file closed with #H5Fclose.
 *
 * To open an existing file, call #H5Fopen:
 * \code
 *   hid_t H5Fopen (const char *name, unsigned flags, hid_t fapl_id)
 * \endcode
 *
 * #H5Fopen opens an existing file with read-write access if #H5F_ACC_RDWR is set and read-only
 * access if #H5F_ACC_RDONLY is set.
 *
 * fapl_id is the file access property list identifier. Alternatively, #H5P_DEFAULT indicates that the
 * application relies on the default I/O access parameters. Creating and changing access property
 * lists is documented further below.
 *
 * A file can be opened more than once via multiple #H5Fopen calls. Each such call returns a unique
 * file identifier and the file can be accessed through any of these file identifiers as long as they
 * remain valid. Each of these file identifiers must be released by calling #H5Fclose when it is no
 * longer needed.
 *
 * For more information, see \ref subsubsec_file_property_lists_access.
 *
 * For more information, see \ref subsec_file_property_lists.
 *
 * \subsection subsec_file_closes Closing an HDF5 File
 * #H5Fclose both closes a file and releases the file identifier returned by #H5Fopen or #H5Fcreate.
 * #H5Fclose must be called when an application is done working with a file; while the HDF5
 * Library makes every effort to maintain file integrity, failure to call #H5Fclose may result in the
 * file being abandoned in an incomplete or corrupted state.
 *
 * To close a file, call #H5Fclose:
 * \code
 *   herr_t H5Fclose (hid_t file_id)
 * \endcode
 * This function releases resources associated with an open file. After closing a file, the file
 * identifier, file_id, cannot be used again as it will be undefined.
 *
 * #H5Fclose fulfills three purposes: to ensure that the file is left in an uncorrupted state, to ensure
 * that all data has been written to the file, and to release resources. Use #H5Fflush if you wish to
 * ensure that all data has been written to the file but it is premature to close it.
 *
 * Note regarding serial mode behavior: When #H5Fclose is called in serial mode, it closes the file
 * and terminates new access to it, but it does not terminate access to objects that remain
 * individually open within the file. That is, if #H5Fclose is called for a file but one or more objects
 * within the file remain open, those objects will remain accessible until they are individually
 * closed. To illustrate, assume that a file, fileA, contains a dataset, data_setA, and that both are
 * open when #H5Fclose is called for fileA. data_setA will remain open and accessible, including
 * writable, until it is explicitly closed. The file will be automatically and finally closed once all
 * objects within it have been closed.
 *
 * Note regarding parallel mode behavior: Once #H5Fclose has been called in parallel mode, access
 * is no longer available to any object within the file.
 *
 * \subsection subsec_file_property_lists File Property Lists
 * Additional information regarding file structure and access are passed to #H5Fcreate and
 * #H5Fopen through property list objects. Property lists provide a portable and extensible method of
 * modifying file properties via simple API functions. There are two kinds of file-related property
 * lists:
 * \li File creation property lists
 * \li File access property lists
 *
 * In the following sub-sections, we discuss only one file creation property, userblock size, in detail
 * as a model for the user. Other file creation and file access properties are mentioned and defined
 * briefly, but the model is not expanded for each; complete syntax, parameter, and usage
 * information for every property list function is provided in the \ref H5P
 * section of the \ref RM.
 *
 * For more information, @see \ref sec_plist.
 *
 * \subsubsection subsubsec_file_property_lists_create Creating a Property List
 * If you do not wish to rely on the default file creation and access properties, you must first create
 * a property list with #H5Pcreate.
 * \code
 *   hid_t H5Pcreate (hid_t cls_id)
 * \endcode
 * cls_id is the type of property list being created. In this case, the appropriate values are
 * #H5P_FILE_CREATE for a file creation property list and #H5P_FILE_ACCESS for a file access
 * property list.
 *
 * Thus, the following calls create a file creation property list and a file access property list with
 * identifiers fcpl_id and fapl_id, respectively:
 * \code
 *   fcpl_id = H5Pcreate (H5P_FILE_CREATE)
 *   fapl_id = H5Pcreate (H5P_FILE_ACCESS)
 * \endcode
 *
 * Once the property lists have been created, the properties themselves can be modified via the
 * functions described in the following sub-sections.
 *
 * \subsubsection subsubsec_file_property_lists_props File Creation Properties
 * File creation property lists control the file metadata, which is maintained in the superblock of the
 * file. These properties are used only when a file is first created.
 *
 * <h4>Userblock Size</h4>
 * \code
 *   herr_t H5Pset_userblock (hid_t plist, hsize_t size)
 *   herr_t H5Pget_userblock (hid_t plist, hsize_t *size)
 * \endcode
 *
 * The userblock is a fixed-length block of data located at the beginning of the file and is ignored
 * by the HDF5 library. This block is specifically set aside for any data or information that
 * developers determine to be useful to their applications but that will not be used by the HDF5
 * library. The size of the userblock is defined in bytes and may be set to any power of two with a
 * minimum size of 512 bytes. In other words, userblocks might be 512, 1024, or 2048 bytes in
 * size.
 *
 * This property is set with #H5Pset_userblock and queried via #H5Pget_userblock. For example, if
 * an application needed a 4K userblock, then the following function call could be used:
 * \code
 *   status = H5Pset_userblock(fcpl_id, 4096)
 * \endcode
 *
 * The property list could later be queried with:
 * \code
 *   status = H5Pget_userblock(fcpl_id, size)
 * \endcode
 * and the value 4096 would be returned in the parameter size.
 *
 * Other properties, described below, are set and queried in exactly the same manner. Syntax and
 * usage are detailed in the @ref H5P section of the \ref RM.
 *
 * <h4>Offset and Length Sizes</h4>
 * This property specifies the number of bytes used to store the offset and length of objects in the
 * HDF5 file. Values of 2, 4, and 8 bytes are currently supported to accommodate 16-bit, 32-bit,
 * and 64-bit file address spaces.
 *
 * These properties are set and queried via #H5Pset_sizes and #H5Pget_sizes.
 *
 * <h4>Symbol Table Parameters</h4>
 * The size of symbol table B-trees can be controlled by setting the 1/2-rank and 1/2-node size
 * parameters of the B-tree.
 *
 * These properties are set and queried via #H5Pset_sym_k and #H5Pget_sym_k
 *
 * <h4>Indexed Storage Parameters</h4>
 * The size of indexed storage B-trees can be controlled by setting the 1/2-rank and 1/2-node size
 * parameters of the B-tree.
 *
 * These properties are set and queried via #H5Pset_istore_k and #H5Pget_istore_k.
 *
 * <h4>Version Information</h4>
 * Various objects in an HDF5 file may over time appear in different versions. The HDF5 Library
 * keeps track of the version of each object in the file.
 *
 * Version information is retrieved via #H5Pget_version.
 *
 * \subsubsection subsubsec_file_property_lists_access File Access Properties
 * This section discusses file access properties that are not related to the low-level file drivers. File
 * drivers are discussed separately later in this chapter.
 * For more information, @see \ref subsec_file_alternate_drivers.
 *
 * File access property lists control various aspects of file I/O and structure.
 *
 * <h4>Data Alignment</h4>
 * Sometimes file access is faster if certain data elements are aligned in a specific manner. This can
 * be controlled by setting alignment properties via the #H5Pset_alignment function. There are two
 * values involved:
 * \li A threshold value
 * \li An alignment interval
 *
 * Any allocation request at least as large as the threshold will be aligned on an address that is a
 * multiple of the alignment interval.
 *
 * <h4>Metadata Block Allocation Size</h4>
 * Metadata typically exists as very small chunks of data; storing metadata elements in a file
 * without blocking them can result in hundreds or thousands of very small data elements in the
 * file. This can result in a highly fragmented file and seriously impede I/O. By blocking metadata
 * elements, these small elements can be grouped in larger sets, thus alleviating both problems.
 *
 * #H5Pset_meta_block_size sets the minimum size in bytes of metadata block allocations.
 * #H5Pget_meta_block_size retrieves the current minimum metadata block allocation size.
 *
 * <h4>Metadata Cache</h4>
 * Metadata and raw data I/O speed are often governed by the size and frequency of disk reads and
 * writes. In many cases, the speed can be substantially improved by the use of an appropriate
 * cache.
 *
 * #H5Pset_cache sets the minimum cache size for both metadata and raw data and a preemption
 * value for raw data chunks. #H5Pget_cache retrieves the current values.
 *
 * <h4>Data Sieve Buffer Size</h4>
 * Data sieve buffering is used by certain file drivers to speed data I/O and is most commonly when
 * working with dataset hyperslabs. For example, using a buffer large enough to hold several pieces
 * of a dataset as it is read in for hyperslab selections will boost performance noticeably.
 *
 * #H5Pset_sieve_buf_size sets the maximum size in bytes of the data sieve buffer.
 * #H5Pget_sieve_buf_size retrieves the current maximum size of the data sieve buffer.
 *
 * <h4>Garbage Collection References</h4>
 * Dataset region references and other reference types use space in an HDF5 file's global heap. If
 * garbage collection is on (1) and the user passes in an uninitialized value in a reference structure,
 * the heap might become corrupted. When garbage collection is off (0), however, and the user reuses
 * a reference, the previous heap block will be orphaned and not returned to the free heap
 * space. When garbage collection is on, the user must initialize the reference structures to 0 or risk
 * heap corruption.
 *
 * #H5Pset_gc_references sets the garbage collecting references flag.
 *
 * \subsection subsec_file_alternate_drivers Alternate File Storage Layouts and Low-level File Drivers
 * The concept of an HDF5 file is actually rather abstract: the address space for what is normally
 * thought of as an HDF5 file might correspond to any of the following:
 * \li Single file on standard file system
 * \li Multiple files on standard file system
 * \li Multiple files on parallel file system
 * \li Block of memory within application's memory space
 * \li More abstract situations such as virtual files
 *
 * This HDF5 address space is generally referred to as an HDF5 file regardless of its organization at
 * the storage level.
 *
 * HDF5 employs an extremely flexible mechanism called the virtual file layer, or VFL, for file
 * I/O. A full understanding of the VFL is only necessary if you plan to write your own drivers
 * see \ref VFLTN in the HDF5 \ref TN.
 *
 * For our
 * purposes here, it is sufficient to know that the low-level drivers used for file I/O reside in the
 * VFL, as illustrated in the following figure. Note that H5FD_STREAM is not available with 1.8.x
 * and later versions of the library.
 *
 * <table>
 * <tr>
 * <td>
 * \image html VFL_Drivers.gif "I/O path from application to VFL and low-level drivers to storage"
 * </td>
 * </tr>
 * </table>
 *
 * As mentioned above, HDF5 applications access HDF5 files through various low-level file
 * drivers. The default driver for that layout is the POSIX driver (also known as the SEC2 driver),
 * #H5FD_SEC2. Alternative layouts and drivers are designed to suit the needs of a variety of
 * systems, environments, and applications. The drivers are listed in the table below.
 *
 * <div>
 * \subsubsection table_file_drivers Supported file drivers
 * \snippet{doc} tables/fileDriverLists.dox supported_file_driver_table
 * </div>
 *
 * For more information, see the \ref RM entries for the function calls shown in
 * the column on the right in the table above.
 *
 * Note that the low-level file drivers manage alternative file storage layouts. Dataset storage
 * layouts (chunking, compression, and external dataset storage) are managed independently of file
 * storage layouts.
 *
 * If an application requires a special-purpose low-level driver, the VFL provides a public API for
 * creating one. For more information on how to create a driver,
 * see \ref VFLTN in the HDF5 \ref TN.
 *
 * \subsubsection subsubsec_file_alternate_drivers_id Identifying the Previously‐used File Driver
 * When creating a new HDF5 file, no history exists, so the file driver must be specified if it is to be
 * other than the default.
 *
 * When opening existing files, however, the application may need to determine which low-level
 * driver was used to create the file. The function #H5Pget_driver is used for this purpose. See the
 * example below.
 *
 * <em>Identifying a driver</em>
 * \code
 *   hid_t H5Pget_driver (hid_t fapl_id)
 * \endcode
 *
 * #H5Pget_driver returns a constant identifying the low-level driver for the access property list
 * fapl_id. For example, if the file was created with the POSIX (aka SEC2) driver,
 * #H5Pget_driver returns #H5FD_SEC2.
 *
 * If the application opens an HDF5 file without both determining the driver used to create the file
 * and setting up the use of that driver, the HDF5 Library will examine the superblock and the
 * driver definition block to identify the driver.
 * See the \ref_spec_fileformat
 * for detailed descriptions of the superblock and the driver definition block.
 *
 * \subsubsection subsubsec_file_alternate_drivers_sec2 The POSIX (aka SEC2) Driver
 * The POSIX driver, #H5FD_SEC2, uses functions from section 2 of the POSIX manual to access
 * unbuffered files stored on a local file system. This driver is also known as the SEC2 driver. The
 * HDF5 Library buffers metadata regardless of the low-level driver, but using this driver prevents
 * data from being buffered again by the lowest layers of the library.
 *
 * The function #H5Pset_fapl_sec2 sets the file access properties to use the POSIX driver. See the
 * example below.
 *
 * <em>Using the POSIX, aka SEC2, driver</em>
 * \code
 *   herr_t H5Pset_fapl_sec2 (hid_t fapl_id)
 * \endcode
 *
 * Any previously-defined driver properties are erased from the property list.
 *
 * Additional parameters may be added to this function in the future. Since there are no additional
 * variable settings associated with the POSIX driver, there is no H5Pget_fapl_sec2 function.
 *
 * \subsubsection subsubsec_file_alternate_drivers_direct The Direct Driver
 * The Direct driver, #H5FD_DIRECT, functions like the POSIX driver except that data is written to
 * or read from the file synchronously without being cached by the system.
 *
 * The functions #H5Pset_fapl_direct and #H5Pget_fapl_direct are used to manage file access properties.
 * See the example below.
 *
 * <em>Using the Direct driver</em>
 * \code
 *   herr_t H5Pset_fapl_direct(hid_t fapl_id, size_t alignment, size_t block_size, size_t cbuf_size)
 *   herr_t H5Pget_fapl_direct(hid_t fapl_id, size_t *alignment, size_t *block_size, size_t *cbuf_size)
 * \endcode
 *
 * #H5Pset_fapl_direct sets the file access properties to use the Direct driver; any previously defined
 * driver properties are erased from the property list. #H5Pget_fapl_direct retrieves the file access
 * properties used with the Direct driver. fapl_id is the file access property list identifier.
 * alignment is the memory alignment boundary. block_size is the file system block size.
 * cbuf_size is the copy buffer size.
 *
 * Additional parameters may be added to this function in the future.
 *
 * \subsubsection subsubsec_file_alternate_drivers_log The Log Driver
 * The Log driver, #H5FD_LOG, is designed for situations where it is necessary to log file access
 * activity.
 *
 * The function #H5Pset_fapl_log is used to manage logging properties. See the example below.
 *
 * <em>Logging file access</em>
 * \code
 *   herr_t H5Pset_fapl_log (hid_t fapl_id, const char *logfile, unsigned int flags, size_t buf_size)
 * \endcode
 *
 * #H5Pset_fapl_log sets the file access property list to use the Log driver. File access characteristics
 * are identical to access via the POSIX driver. Any previously defined driver properties are erased
 * from the property list.
 *
 * Log records are written to the file logfile.
 *
 * The logging levels set with the verbosity parameter are shown in the table below.
 *
 * <table>
 * <caption>Logging levels</caption>
 * <tr>
 * <th>Level</th>
 * <th>Comments</th>
 * </tr>
 * <tr>
 * <td>0</td>
 * <td>Performs no logging.</td>
 * </tr>
 * <tr>
 * <td>1</td>
 * <td>Records where writes and reads occur in the file.</td>
 * </tr>
 * <tr>
 * <td>2</td>
 * <td>Records where writes and reads occur in the file and what kind of data is written
 * at each location. This includes raw data or any of several types of metadata
 * (object headers, superblock, B-tree data, local headers, or global headers).</td>
 * </tr>
 * </table>
 *
 * There is no H5Pget_fapl_log function.
 *
 * Additional parameters may be added to this function in the future.
 *
 * \subsubsection subsubsec_file_alternate_drivers_win The Windows Driver
 * The Windows driver, #H5FD_WINDOWS, was modified in HDF5-1.8.8 to be a wrapper of the
 * POSIX driver, #H5FD_SEC2. In other words, if the Windows drivers is used, any file I/O will
 * instead use the functionality of the POSIX driver. This change should be transparent to all user
 * applications. The Windows driver used to be the default driver for Windows systems. The
 * POSIX driver is now the default.
 *
 * The function #H5Pset_fapl_windows sets the file access properties to use the Windows driver.
 * See the example below.
 *
 * <em>Using the Windows driver</em>
 * \code
 *   herr_t H5Pset_fapl_windows (hid_t fapl_id)
 * \endcode
 *
 * Any previously-defined driver properties are erased from the property list.
 *
 * Additional parameters may be added to this function in the future. Since there are no additional
 * variable settings associated with the POSIX driver, there is no H5Pget_fapl_windows function.
 *
 * \subsubsection subsubsec_file_alternate_drivers_stdio The STDIO Driver
 * The STDIO driver, #H5FD_STDIO, accesses permanent files in a local file system like the
 * POSIX driver does. The STDIO driver also has an additional layer of buffering beneath the
 * HDF5 Library.
 *
 * The function #H5Pset_fapl_stdio sets the file access properties to use the STDIO driver. See the
 * example below.
 *
 * <em>Using the STDIO driver</em>
 * \code
 *   herr_t H5Pset_fapl_stdio (hid_t fapl_id)
 * \endcode
 *
 * Any previously defined driver properties are erased from the property list.
 *
 * Additional parameters may be added to this function in the future. Since there are no additional
 * variable settings associated with the STDIO driver, there is no H5Pget_fapl_stdio function.
 *
 * \subsubsection subsubsec_file_alternate_drivers_mem The Memory (aka Core) Driver
 * There are several situations in which it is reasonable, sometimes even required, to maintain a file
 * entirely in system memory. You might want to do so if, for example, either of the following
 * conditions apply:
 * <ul><li>Performance requirements are so stringent that disk latency is a limiting factor</li>
 * <li>You are working with small, temporary files that will not be retained and, thus,
 * need not be written to storage media</li></ul>
 *
 * The Memory driver, #H5FD_CORE, provides a mechanism for creating and managing such in memory files.
 * The functions #H5Pset_fapl_core and #H5Pget_fapl_core manage file access
 * properties. See the example below.
 *
 * <em>Managing file access for in-memory files</em>
 * \code
 *   herr_t H5Pset_fapl_core (hid_t access_properties, size_t block_size, bool backing_store)
 *   herr_t H5Pget_fapl_core (hid_t access_properties, size_t *block_size), bool *backing_store)
 * \endcode
 *
 * #H5Pset_fapl_core sets the file access property list to use the Memory driver; any previously
 * defined driver properties are erased from the property list.
 *
 * Memory for the file will always be allocated in units of the specified block_size.
 *
 * The backing_store Boolean flag is set when the in-memory file is created.
 * backing_store indicates whether to write the file contents to disk when the file is closed. If
 * backing_store is set to 1 (true), the file contents are flushed to a file with the same name as the
 * in-memory file when the file is closed or access to the file is terminated in memory. If
 * backing_store is set to 0 (false), the file is not saved.
 *
 * The application is allowed to open an existing file with the #H5FD_CORE driver. While using
 * #H5Fopen to open an existing file, if backing_store is set to 1 and the flag for #H5Fopen is set to
 * #H5F_ACC_RDWR, changes to the file contents will be saved to the file when the file is closed.
 * If backing_store is set to 0 and the flag for #H5Fopen is set to #H5F_ACC_RDWR, changes to the
 * file contents will be lost when the file is closed. If the flag for #H5Fopen is set to
 * #H5F_ACC_RDONLY, no change to the file will be allowed either in memory or on file.
 *
 * If the file access property list is set to use the Memory driver, #H5Pget_fapl_core will return
 * block_size and backing_store with the relevant file access property settings.
 *
 * Note the following important points regarding in-memory files:
 * <ul><li>Local temporary files are created and accessed directly from memory without ever
 *  being written to disk</li>
 * <li>Total file size must not exceed the available virtual memory</li>
 * <li>Only one HDF5 file identifier can be opened for the file, the identifier returned by
 *  #H5Fcreate or #H5Fopen</li>
 * <li>The changes to the file will be discarded when access is terminated unless
 *  backing_store is set to 1</li></ul>
 *
 * Additional parameters may be added to these functions in the future.
 *
 * see \ref H5FIM_UG
 * section for information on more advanced usage of the Memory file driver, and
 * see <a
 * href="https://\DOCURL/advanced_topics/ModifiedRegionWrites.pdf">
 * Modified Region Writes</a>
 * section for information on how to set write operations so that only modified regions are written
 * to storage.
 *
 * \subsubsection subsubsec_file_alternate_drivers_family The Family Driver
 * HDF5 files can become quite large, and this can create problems on systems that do not support
 * files larger than 2 gigabytes. The HDF5 file family mechanism is designed to solve the problems
 * this creates by splitting the HDF5 file address space across several smaller files. This structure
 * does not affect how metadata and raw data are stored: they are mixed in the address space just as
 * they would be in a single, contiguous file.
 *
 * HDF5 applications access a family of files via the Family driver, #H5FD_FAMILY. The
 * functions #H5Pset_fapl_family and #H5Pget_fapl_family are used to manage file family
 * properties. See the example below.
 *
 * <em>Managing file family properties</em>
 * \code
 *   herr_t H5Pset_fapl_family (hid_t fapl_id,
 *   hsize_t memb_size, hid_t member_properties)
 *   herr_t H5Pget_fapl_family (hid_t fapl_id,
 *   hsize_t *memb_size, hid_t *member_properties)
 * \endcode
 *
 * Each member of the family is the same logical size though the size and disk storage reported by
 * file system listing tools may be substantially smaller. Examples of file system listing tools are
 * \code
 *  ls -l
 * \endcode
 * on a Unix system or the detailed folder listing on an Apple or Microsoft Windows
 * system. The name passed to #H5Fcreate or #H5Fopen should include a printf(3c)-style integer
 * format specifier which will be replaced with the family member number. The first family
 * member is numbered zero (0).
 *
 * #H5Pset_fapl_family sets the access properties to use the Family driver; any previously defined
 * driver properties are erased from the property list. member_properties will serve as the file
 * access property list for each member of the file family. memb_size specifies the logical size, in
 * bytes, of each family member. memb_size is used only when creating a new file or truncating an
 * existing file; otherwise the member size is determined by the size of the first member of the
 * family being opened.
 *
 * #H5Pget_fapl_family is used to retrieve file family properties. If the file access property list is set
 * to use the Family driver, member_properties will be returned with a pointer to a copy of the
 * appropriate member access property list. If memb_size is non-null, it will contain the logical
 * size, in bytes, of family members.
 *
 * Additional parameters may be added to these functions in the future.
 *
 * <h4>Unix Tools and an HDF5 Utility</h4>
 * It occasionally becomes necessary to repartition a file family. A command-line utility for this
 * purpose, h5repart, is distributed with the HDF5 library.
 *
 * \code
 * h5repart [-v] [-b block_size[suffix]] [-m member_size[suffix]] source destination
 * \endcode
 *
 * h5repart repartitions an HDF5 file by copying the source file or file family to the destination file
 * or file family, preserving holes in the underlying UNIX files. Families are used for the source
 * and/or destination if the name includes a printf-style integer format such as %d. The -v switch
 * prints input and output file names on the standard error stream for progress monitoring, -b sets
 * the I/O block size (the default is 1KB), and -m sets the output member size if the destination is a
 * family name (the default is 1GB). block_size and member_size may be suffixed with the letters
 * g, m, or k for GB, MB, or KB respectively.
 *
 * The h5repart utility is described on the Tools page of the \ref RM.
 *
 * An existing HDF5 file can be split into a family of files by running the file through split(1) on a
 * UNIX system and numbering the output files. However, the HDF5 Library is lazy about
 * extending the size of family members, so a valid file cannot generally be created by
 * concatenation of the family members.
 *
 * Splitting the file and rejoining the segments by concatenation (split(1) and cat(1) on UNIX
 * systems) does not generate files with holes; holes are preserved only through the use of h5repart.
 *
 * \subsubsection subsubsec_file_alternate_drivers_multi The Multi Driver
 * In some circumstances, it is useful to separate metadata from raw data and some types of
 * metadata from other types of metadata. Situations that would benefit from use of the Multi driver
 * include the following:
 * <ul><li>In networked situations where the small metadata files can be kept on local disks but
 * larger raw data files must be stored on remote media</li>
 * <li>In cases where the raw data is extremely large</li>
 * <li>In situations requiring frequent access to metadata held in RAM while the raw data
 * can be efficiently held on disk</li></ul>
 *
 * In either case, access to the metadata is substantially easier with the smaller, and possibly more
 * localized, metadata files. This often results in improved application performance.
 *
 * The Multi driver, #H5FD_MULTI, provides a mechanism for segregating raw data and different
 * types of metadata into multiple files. The functions #H5Pset_fapl_multi and
 * #H5Pget_fapl_multi are used to manage access properties for these multiple files. See the example
 * below.
 *
 * <em>Managing access properties for multiple files</em>
 * \code
 *   herr_t H5Pset_fapl_multi (hid_t fapl_id, const H5FD_mem_t *memb_map, const hid_t *memb_fapl,
 *                             const char * const *memb_name, const haddr_t *memb_addr,
 *                             bool relax)
 *  herr_t H5Pget_fapl_multi (hid_t fapl_id, const H5FD_mem_t *memb_map, const hid_t *memb_fapl,
 *                             const char **memb_name, const haddr_t *memb_addr, bool *relax)
 * \endcode
 *
 * #H5Pset_fapl_multi sets the file access properties to use the Multi driver; any previously defined
 * driver properties are erased from the property list. With the Multi driver invoked, the application
 * will provide a base name to #H5Fopen or #H5Fcreate. The files will be named by that base name as
 * modified by the rule indicated in memb_name. File access will be governed by the file access
 * property list memb_properties.
 *
 * See #H5Pset_fapl_multi and #H5Pget_fapl_multi in the \ref RM for descriptions
 * of these functions and their usage.
 *
 * Additional parameters may be added to these functions in the future.
 *
 * \subsubsection subsubsec_file_alternate_drivers_split The Split Driver
 * The Split driver is a limited case of the Multi driver where only two files are
 * created. One file holds metadata, and the other file holds raw data.
 * The function #H5Pset_fapl_split is used to manage Split file access properties. See the example
 * below.
 *
 * <em>Managing access properties for split files</em>
 * \code
 *   herr_t H5Pset_fapl_split (hid_t access_properties, const char *meta_extension,
 *                             hid_t meta_properties,const char *raw_extension, hid_t raw_properties)
 * \endcode
 *
 * #H5Pset_fapl_split sets the file access properties to use the Split driver; any previously defined
 * driver properties are erased from the property list.
 *
 * With the Split driver invoked, the application will provide a base file name such as file_name to
 * #H5Fcreate or #H5Fopen. The metadata and raw data files in storage will then be named
 * file_name.meta_extension and file_name.raw_extension, respectively. For example, if
 * meta_extension is defined as .meta and raw_extension is defined as .raw, the final filenames will
 * be file_name.meta and file_name.raw.
 *
 * Each file can have its own file access property list. This allows the creative use of other lowlevel
 * file drivers. For instance, the metadata file can be held in RAM and accessed via the
 * Memory driver while the raw data file is stored on disk and accessed via the POSIX driver.
 * Metadata file access will be governed by the file access property list in meta_properties. Raw
 * data file access will be governed by the file access property list in raw_properties.
 *
 * Additional parameters may be added to these functions in the future. Since there are no
 * additional variable settings associated with the Split driver, there is no H5Pget_fapl_split
 * function.
 *
 * \subsubsection subsubsec_file_alternate_drivers_par The Parallel Driver
 * Parallel environments require a parallel low-level driver. HDF5's default driver for parallel
 * systems is called the Parallel driver, #H5FD_MPIO. This driver uses the MPI standard for both
 * communication and file I/O.
 *
 * The functions #H5Pset_fapl_mpio and #H5Pget_fapl_mpio are used to manage file access
 * properties for the #H5FD_MPIO driver. See the example below.
 *
 * <em>Managing parallel file access properties</em>
 * \code
 *   herr_t H5Pset_fapl_mpio (hid_t fapl_id, MPI_Comm comm, MPI_info info)
 *   herr_t H5Pget_fapl_mpio (hid_t fapl_id, MPI_Comm *comm, MPI_info *info)
 * \endcode
 *
 * The file access properties managed by #H5Pset_fapl_mpio and retrieved by
 * #H5Pget_fapl_mpio are the MPI communicator, comm, and the MPI info object, info. comm and
 * info are used for file open. info is an information object much like an HDF5 property list. Both
 * are defined in MPI_FILE_OPEN of MPI.
 *
 * The communicator and the info object are saved in the file access property list fapl_id.
 * fapl_id can then be passed to MPI_FILE_OPEN to create and/or open the file.
 *
 * #H5Pset_fapl_mpio and #H5Pget_fapl_mpio are available only in the parallel HDF5 Library and
 * are not collective functions. The Parallel driver is available only in the parallel HDF5 Library.
 *
 * Additional parameters may be added to these functions in the future.
 *
 * \subsection subsec_file_examples Code Examples for Opening and Closing Files
 * \subsubsection subsubsec_file_examples_trunc Example Using the H5F_ACC_TRUNC Flag
 * The following example uses the #H5F_ACC_TRUNC flag when it creates a new file. The default
 * file creation and file access properties are also used. Using #H5F_ACC_TRUNC means the
 * function will look for an existing file with the name specified by the function. In this case, that
 * name is FILE. If the function does not find an existing file, it will create one. If it does find an
 * existing file, it will empty the file in preparation for a new set of data. The identifier for the
 * "new" file will be passed back to the application program.
 * For more information, @see \ref subsec_file_access_modes.
 *
 * <em>Creating a file with default creation and access properties</em>
 * \code
 *   hid_t file; // identifier
 *
 *   // Create a new file using H5F_ACC_TRUNC access, default
 *   // file creation properties, and default file access
 *   // properties.
 *   file = H5Fcreate(FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
 *
 *   // Close the file.
 *   status = H5Fclose(file);
 * \endcode
 *
 * \subsubsection subsubsec_file_examples_props Example with the File Creation Property List
 * The example below shows how to create a file with 64-bit object offsets and lengths.
 *
 * <em>Creating a file with 64-bit offsets</em>
 * \code
 *   hid_t create_plist;
 *   hid_t file_id;
 *
 *   create_plist = H5Pcreate(H5P_FILE_CREATE);
 *   H5Pset_sizes(create_plist, 8, 8);
 *   file_id = H5Fcreate(“test.h5”, H5F_ACC_TRUNC, create_plist, H5P_DEFAULT);
 *   .
 *   .
 *   .
 *
 *   H5Fclose(file_id);
 * \endcode
 *
 * \subsubsection subsubsec_file_examples_access Example with the File Access Property List
 * This example shows how to open an existing file for independent datasets access by MPI parallel
 * I/O:
 *
 * <em>Opening an existing file for parallel I/O</em>
 * \code
 *   hid_t access_plist;
 *   hid_t file_id;
 *
 *   access_plist = H5Pcreate(H5P_FILE_ACCESS);
 *   H5Pset_fapl_mpi(access_plist, MPI_COMM_WORLD, MPI_INFO_NULL);
 *
 *   // H5Fopen must be called collectively
 *   file_id = H5Fopen(“test.h5”, H5F_ACC_RDWR, access_plist);
 *   .
 *   .
 *   .
 *
 *   // H5Fclose must be called collectively
 *   H5Fclose(file_id);
 * \endcode
 *
 * \subsection subsec_file_multiple Working with Multiple HDF5 Files
 * Multiple HDF5 files can be associated so that the files can be worked with as though all the
 * information is in a single HDF5 file. A temporary association can be set up by means of the
 * #H5Fmount function. A permanent association can be set up by means of the external link
 * function #H5Lcreate_external.
 *
 * The purpose of this section is to describe what happens when the #H5Fmount function is used to
 * mount one file on another.
 *
 * When a file is mounted on another, the mounted file is mounted at a group, and the root group of
 * the mounted file takes the place of that group until the mounted file is unmounted or until the
 * files are closed.
 *
 * The figure below shows two files before one is mounted on the other. File1 has two groups and
 * three datasets. The group that is the target of the A link has links, Z and Y, to two of the datasets.
 * The group that is the target of the B link has a link, W, to the other dataset. File2 has three
 * groups and three datasets. The groups in File2 are the targets of the AA, BB, and CC links. The
 * datasets in File2 are the targets of the ZZ, YY, and WW links.
 *
 * <table>
 * <tr>
 * <td>
 * \image html Files_fig3.gif "Two separate files"
 * </td>
 * </tr>
 * </table>
 *
 * The figure below shows the two files after File2 has been mounted File1 at the group that is the
 * target of the B link.
 *
 * <table>
 * <tr>
 * <td>
 * \image html Files_fig4.gif "File2 mounted on File1"
 * </td>
 * </tr>
 * </table>
 *
 * Note: In the figure above, the dataset that is the target of the W link is not shown. That dataset is
 * masked by the mounted file.
 *
 * If a file is mounted on a group that has members, those members are hidden until the mounted
 * file is unmounted. There are two ways around this if you need to work with a group member.
 * One is to mount the file on an empty group. Another is to open the group member before you
 * mount the file. Opening the group member will return an identifier that you can use to locate the
 * group member.
 *
 * The example below shows how #H5Fmount might be used to mount File2 onto File1.
 *
 * <em>Using H5Fmount</em>
 * \code
 *   status = H5Fmount(loc_id, "/B", child_id, plist_id)
 * \endcode
 *
 * Note: In the code example above, loc_id is the file identifier for File1, /B is the link path to the
 * group where File2 is mounted, child_id is the file identifier for File2, and plist_id is a property
 * list identifier.
 * For more information, @see \ref sec_group.
 *
 * See the entries for #H5Fmount, #H5Funmount, and #H5Lcreate_external in the \ref RM.
 *
 * Previous Chapter \ref sec_program - Next Chapter \ref sec_group
 *
 * \page H5FIM_UG HDF5 File Image
 *
 * \section sec_file_image HDF5 File Image
 * \subsection subsec_file_image_intro Introduction to HDF5 File Image Operations
 * File image operations allow users to work with HDF5 files in memory in the same ways that users currently
 * work with HDF5 files on disk. Disk I/O is not required when file images are opened, created, read from,
 * or written to.
 *
 * An HDF5 file image is an HDF5 file that is held in a buffer in main memory. Setting up a file image in
 * memory involves using either a buffer in the file access property list or a buffer in the \ref
 * subsubsec_file_alternate_drivers_mem file driver. The advantage of working with a file in memory is faster
 * access to the data.
 *
 * The challenge of working with files in memory buffers is maximizing performance and minimizing memory
 * footprint while working within the constraints of the property list mechanism. This should be a non-issue
 * for small file images, but may be a major issue for large images.
 *
 * If invoked with the appropriate flags, the #H5LTopen_file_image high level library call should deal with
 * these challenges in most cases. However, some applications may require the programmer to address these
 * issues directly.
 *
 * \subsubsection subsubsec_file_image_intro_sum HDF5 File Image Operations Function Summary
 * Functions used in file image operations are listed below.
 * <table>
 * <caption>File image operations functions</caption>
 * <tr>
 * <th>C Function</th>
 * <th>Purpose</th>
 * </tr>
 * <tr>
 * <td>#H5Pset_file_image</td>
 * <td>Allows an application to specify an initial file image. For more information, see section \ref
 * FI211.</td>
 * </tr>
 * <tr>
 * <td>#H5Pget_file_image</td>
 * <td>Allows an application to retrieve a copy of the file image designated for a VFD to use as the initial
 * contents of a file. For more information, see section \ref FI212.</td>
 * </tr>
 * <tr>
 * <td>#H5Pset_file_image_callbacks</td>
 * <td>Allows an application to manage file image buffer allocation, copying, reallocation, and release. For
 * more information, see section \ref FI213.</td>
 * </tr>
 * <tr>
 * <td>#H5Pget_file_image_callbacks</td>
 * <td>Allows an application to obtain the current file image callbacks from a file access property list. For
 * more information, see section \ref FI214.</td>
 * </tr>
 * <tr>
 * <td>#H5Fget_file_image</td>
 * <td>Provides a simple way to retrieve a copy of the image of an existing, open file. For more information,
 * see section \ref FI216.</td>
 * </tr>
 * <tr>
 * <td>#H5LTopen_file_image</td>
 * <td>Provides a convenient way to open an initial file image with the Core VFD. For more information,
 * see section \ref FI221.</td>
 * </tr>
 * </table>
 *
 * \subsubsection subsubsec_file_image_intro_abbr Abbreviations
 * <table>
 * <caption>Abbreviations</caption>
 * <tr>
 * <th>Abbreviation</th>
 * <th>This abbreviation is short for</th>
 * </tr>
 * <tr>
 * <td>FAPL or fapl</td>
 * <td>File Access Property List. In code samples, fapl is used.</td>
 * </tr>
 * <tr>
 * <td>VFD</td>
 * <td>Virtual File Driver</td>
 * </tr>
 * <tr>
 * <td>VFL</td>
 * <td>Virtual File Layer</td>
 * </tr>
 * </table>
 *
 * \subsubsection subsubsec_file_image_intro_pre Developer Prerequisites
 * Developers who use the file image operations described in this document should be proficient
 * and experienced users of the HDF5 C Library APIs. More specifically, developers should have a
 * working knowledge of property lists, callbacks, and virtual file drivers.
 *
 * \subsubsection subsubsec_file_image_intro_res Resources
 * See the following for more information.
 *
 * The \ref subsec_file_alternate_drivers section is in \ref sec_file
 * chapter of the \ref UG.
 *
 * The #H5Pset_fapl_core function call can be used to modify the file access property list so
 * that the Memory virtual file driver, #H5FD_CORE, is used. The Memory file driver is also known
 * as the Core file driver.
 *
 * Links to the \ref VFLTN and List of Functions documents can be found in the HDF5 \ref TN.
 *
 * \subsection subsec_file_image_api File Image C API Call Syntax
 * The C API function calls described in this chapter fall into two categories: low-level routines that are
 * part of the main HDF5 C Library and one high-level routine that is part of the “lite” API in the
 * high-level wrapper library. The high-level routine uses the low-level routines and presents frequently
 * requested functionality conveniently packaged for application developers’ use.
 *
 * \subsubsection subsubsec_file_image_api_low Low-level C API Routines
 * The purpose of this section is to describe the low-level C API routines that support file image operations.
 * These routines allow an in-memory image of an HDF5 file to be opened without requiring file system I/O.
 *
 * The basic approach to opening an in-memory image of an HDF5 file is to pass the image to the Core file
 * driver, and then tell the Core file driver to open the file. We do this by using the
 * #H5Pget_file_image/#H5Pset_file_image calls. These calls allow the user to specify an initial file image.
 *
 * A potential problem with the #H5Pget_file_image/#H5Pset_file_image calls is the overhead of allocating and
 * copying of large file image buffers. The callback routines enable application programs to avoid this
 * problem. However, the use of these callbacks is complex and potentially hazardous: the particulars are
 * discussed in the semantics and examples chapters below (see section \ref subsubsec_file_image_semantics_cbk
 * and section \ref subsubsec_file_image_example_read respectively). Fortunately, use of the file image
 * callbacks should seldom be necessary: the #H5LTopen_file_image call should address most use cases.
 *
 * The property list facility in HDF5 is employed in file image operations. This facility was designed for
 * passing data, not consumable resources, into API calls. The peculiar ways in which the file image
 * allocation callbacks may be used allows us to avoid extending the property list structure to handle
 * consumable resources cleanly and to avoid constructing a new facility for the purpose.
 *
 * The sub-sections below describe the low-level C APIs that are used with file image operations.
 *
 * \anchor FI211 <h4>#H5Pset_file_image</h4>
 * The #H5Pset_file_image routine allows an application to provide an image for a file driver to use as the
 * initial contents of the file. This call was designed initially for use with the Core VFD, but it can be
 * used with any VFD that supports using an initial file image when opening a file. See the \ref FI215 section
 * for more information. Calling this routine makes a copy of the provided file image buffer. See the \ref
 * FI213 section for more information.
 *
 * The signature of #H5Pset_file_image is defined as follows:
 * \code
 *     herr_t H5Pset_file_image(hid_t fapl_id, void *buf_ptr, size_t buf_len)
 * \endcode
 *
 * The parameters of #H5Pset_file_image are defined as follows:
 * \li fapl_id contains the ID of the target file access property list.
 * \li buf_ptr supplies a pointer to the initial file image, or NULL if no initial file image is desired.
 * \li buf_len contains the size of the supplied buffer, or 0
 * if no initial image is desired. If either the buf_len parameter is zero, or the buf_ptr parameter is NULL,
 * no file image will be set in the FAPL, and any existing file image buffer in the FAPL will be released. If
 * a buffer is released, the FAPL’s file image buf_len will be set to 0 and buf_ptr will be set to NULL.
 *
 * Given the tight interaction between the file image callbacks and the file image, the file image callbacks
 * in a property list cannot be changed while a file image is defined.
 *
 * With properly constructed file image callbacks, it is possible to avoid actually copying the file image.
 * The particulars of this are discussed in greater detail in the \ref subsec_file_image_semantics chapter
 * and in the \ref subsec_file_image_example chapter.
 *
 * \anchor FI212 <h4>#H5Pget_file_image</h4>
 * The #H5Pget_file_image routine allows an application to retrieve a copy of the file image designated for a
 * VFD to use as the initial contents of a file. This routine uses the file image callbacks (if defined) when
 * allocating and loading the buffer to return to the application, or it uses malloc and memcpy if the
 * callbacks are undefined. When malloc and memcpy are used, it will be the caller’s responsibility to discard
 * the returned buffer via a call to free.
 *
 * The signature of #H5Pget_file_image is defined as follows:
 * \code
 * herr_t H5Pget_file_image(hid_t fapl_id, void **buf_ptr_ptr, size_t *buf_len_ptr)
 * \endcode
 *
 * The parameters of #H5Pget_file_image are defined as follows:
 * \li fapl_id contains the ID of the target file access property list.
 * \li buf_ptr_ptr contains a NULL or a pointer to a void*. If buf_ptr_ptr is not NULL, on successful
 *     return, *buf_ptr_ptr will contain a pointer to a copy of the initial image provided in the last
 *     call to #H5Pset_file_image for the supplied fapl_id. If no initial image has been
 *     set, *buf_ptr_ptr will be NULL.
 * \li buf_len_ptr contains a NULL or a pointer to size_t. If buf_len_ptr is not NULL, on
 *     successful return, *buf_len_ptr will contain the value of the buf_len parameter for the
 *     initial image in the supplied fapl_id. If no initial image is set, the value of *buf_len_ptr
 *     will be 0. As with #H5Pset_file_image, appropriately defined file image callbacks can allow
 *     this function to avoid buffer allocation and memory copy operations.
 *
 * \anchor FI213 <h4>#H5Pset_file_image_callbacks</h4>
 * The #H5Pset_file_image_callbacks API call exists to allow an application to control the management of
 * file image buffers through user defined callbacks. These callbacks will be used in the management of
 * file image buffers in property lists and in select file drivers. These routines are invoked when a new
 * file image buffer is allocated, when an existing file image buffer is copied or resized, or when a file
 * image buffer is released from use. From the perspective of the HDF5 Library, the operations of the
 * image_malloc, image_memcpy, image_realloc, and image_free callbacks must be identical to those of the
 * corresponding C standard library calls (malloc, memcpy, realloc, and free). While the operations must
 * be identical, the file image callbacks have more parameters. The callbacks and their parameters are
 * described below. The return values of image_malloc and image_realloc are identical to the return
 * values of malloc and realloc. However, the return values of image_memcpy and image_free are different
 * than the return values of memcpy and free: the return values of image_memcpy and image_free can also
 * indicate failure. See the \ref subsec_file_image_semantics section for more information.
 *
 * The signature of #H5Pset_file_image_callbacks is defined as follows:
 * \code
 *     typedef enum { H5FD_FILE_IMAGE_OP_PROPERTY_LIST_SET, H5FD_FILE_IMAGE_OP_PROPERTY_LIST_COPY,
 * H5FD_FILE_IMAGE_OP_PROPERTY_LIST_GET, H5FD_FILE_IMAGE_OP_PROPERTY_LIST_CLOSE, H5FD_FILE_IMAGE_OP_FILE_OPEN,
 * H5FD_FILE_IMAGE_OP_FILE_RESIZE, H5FD_FILE_IMAGE_OP_FILE_CLOSE } H5FD_file_image_op_t;
 *
 *     typedef struct {
 *         void *(*image_malloc)(size_t size, H5FD_file_image_op_t file_image_op, void *udata);
 *         void *(*image_memcpy)(void *dest, const void *src, size_t size, H5FD_file_image_op_t file_image_op,
 * void *udata); void *(*image_realloc)(void *ptr, size_t size, H5FD_file_image_op_t file_image_op, void
 * *udata); herr_t (*image_free)(void *ptr, H5FD_file_image_op_t file_image_op, void *udata); void
 * *(*udata_copy)(void *udata); herr_t (*udata_free)(void *udata); void *udata; } H5FD_file_image_callbacks_t;
 *
 *     herr_t H5Pset_file_image_callbacks(hid_t fapl_id, H5FD_file_image_callbacks_t *callbacks_ptr)
 * \endcode
 *
 * The parameters of #H5Pset_file_image_callbacks are defined as follows:
 * \li fapl_id contains the ID of the target file access property list.
 * \li callbacks_ptr contains a pointer to an instance of the #H5FD_file_image_callbacks_t structure.
 *
 * The fields of the #H5FD_file_image_callbacks_t structure are defined as follows:
 * \li image_malloc contains a pointer to a function with (from the perspective of HDF5) functionality
 * identical to the standard C library malloc() call.
 * The parameters of the image_malloc callback are defined as follows:
 *   <ul>
 *     <li>size contains the size in bytes of the image buffer to allocate.</li>
 *     <li>file_image_op contains one of the values of #H5FD_file_image_op_t.</li>
 *         These values indicate the operation being performed on the file image when this callback is
 * invoked. Possible values for file_image_op are discussed in \ref FITable2.</li> <li>udata holds the value
 * passed in for the udata parameter to #H5Pset_file_image_callbacks. Setting image_malloc to NULL indicates
 * that the HDF5 Library should invoke the standard C library malloc() routine when allocating file image
 * buffers.</li>
 *   </ul>
 * \li image_memcpy contains a pointer to a function with (from the perspective of HDF5) functionality
 * identical to the standard C library memcpy() call except that it returns NULL on failure. Recall that the
 * memcpy C Library routine is defined to return the dest parameter in all cases. The parameters of the
 * image_memcpy callback are defined as follows: <ul> <li>dest contains the address of the destination
 * buffer.</li> <li>src contains the address of the source buffer.</li> <li>size contains the number of bytes
 * to copy.</li> <li>file_image_op contains one of the values of #H5FD_file_image_op_t. These values indicate
 * the operation being performed on the file image when this callback is invoked. Possible values for
 *         file_image_op are discussed in \ref FITable2.</li>
 *     <li>udata holds the value passed in for the udata parameter to #H5Pset_file_image_callbacks.
 *         Setting image_memcpy to NULL indicates that the HDF5 Library should invoke the standard C
 *         library memcpy() routine when copying buffers.</li>
 *   </ul>
 * \li image_realloc contains a pointer to a function with (from the perspective of HDF5) functionality
 * identical to the standard C library realloc() call. The parameters of the image_realloc callback are
 * defined as follows: <ul> <li>ptr contains the pointer to the buffer being reallocated.</li> <li>size
 * contains the desired size in bytes of the buffer after realloc.</li> <li>file_image_op contains one of the
 * values of H5FD_file_image_op_t. These values indicate the operation being performed on the file image when
 * this callback is invoked. Possible values for file_image_op are discussed in \ref FITable2.</li> <li>udata
 * holds the value passed in for the udata parameter to #H5Pset_file_image_callbacks. Setting image_realloc to
 * NULL indicates that the HDF5 Library should invoke the standard C library realloc() routine when resizing
 * file image buffers.</li>
 *   </ul>
 * \li image_free contains a pointer to a function with (from the perspective of HDF5) functionality identical
 * to the standard C library free() call except that it will return 0 (SUCCEED) on success and -1 (FAIL) on
 * failure. The parameters of the image_free callback are defined as follows:
 *   <ul>
 *     <li>ptr contains the pointer to the buffer being released.</li>
 *     <li>file_image_op contains one of the values of H5FD_file_image_op_t. These values indicate the
 *         operation being performed on the file image when this callback is invoked. Possible values
 *         for file_image_op are discussed in \ref FITable2.</li>
 *     <li>udata holds the value passed in for the udata parameter to #H5Pset_file_image_callbacks.
 *         Setting image_free to NULL indicates that the HDF5 Library should invoke the standard C
 *         library free() routine when releasing file image buffers.</li>
 *   </ul>
 * \li udata_copy contains a pointer to a function that (from the perspective of HDF5) allocates a buffer of
 * suitable size, copies the contents of the supplied udata into the new buffer, and returns the address
 * of the new buffer. The function returns NULL on failure. This function is necessary if a non-NULL udata
 * parameter is supplied, so that property lists containing the image callbacks can be copied. If the udata
 * parameter (below) is NULL, then this parameter should be NULL as well. The parameter of the udata_copy
 * callback is defined as follows:
 *   <ul>
 *     <li>udata contains the pointer to the user data block being copied.</li>
 *     <li>udata_free contains a pointer to a function that (from the perspective of HDF5) frees a
 *         user data block. This function is necessary if a non-NULL udata parameter is supplied
 *         so that property lists containing image callbacks can be discarded without a memory leak.
 *         If the udata parameter (below) is NULL, this parameter should be NULL as well. The
 *         parameter of the udata_free callback is defined as follows:
 *        <ul>
 *            <li>udata contains the pointer to the user data block to be freed. </li>
 *            <li>udata_free returns 0 (SUCCEED) on success and -1 (FAIL) on failure.</li>
 *        </ul></li>
 *   </ul>
 * \li udata contains a pointer value, potentially to user-defined data, that will be passed to the
 * image_malloc, image_memcpy, image_realloc, and image_free callbacks. The semantics of the values
 * that can be set for the file_image_op parameter to the above callbacks are described in the table below:
 * <table>
 * <caption id="FITable2">Values for the file_image_op parameter</caption>
 * <tr>
 * <td>#H5FD_FILE_IMAGE_OP_PROPERTY_LIST_SET</td><td>This value is passed to the image_malloc and image_memcpy
 * callbacks when an image buffer is being copied while being set in a FAPL.</td>
 * </tr>
 * <tr>
 * <td>#H5FD_FILE_IMAGE_OP_PROPERTY_LIST_COPY</td><td>This
 * value is passed to the image_malloc and image_memcpy callbacks when an image buffer is being copied when
 * a FAPL is copied.</td>
 * </tr>
 * <tr>
 * <td>#H5FD_FILE_IMAGE_OP_PROPERTY_LIST_GET</td><td>This value is passed to the image_malloc and image_memcpy
 * callbacks when an image buffer is being copied while being retrieved from a FAPL.</td>
 * </tr>
 * <tr>
 * <td>#H5FD_FILE_IMAGE_OP_PROPERTY_LIST_CLOSE</td><td>This value is passed to the image_free callback
 * when an image buffer is being released during a FAPL close operation.</td>
 * </tr>
 * <tr>
 * <td>#H5FD_FILE_IMAGE_OP_FILE_OPEN</td><td>This value is passed to the image_malloc and image_memcpy
 * callbacks when an image buffer is copied during a file open operation. While the image being opened will
 * typically be copied from a FAPL, this need not always be the case. An example of an exception is when the
 * Core file driver takes its initial image from a file.</td>
 * </tr>
 * <tr>
 * <td>#H5FD_FILE_IMAGE_OP_FILE_RESIZE</td><td>This value is passed to the image_realloc
 * callback when a file driver needs to resize an image buffer.</td>
 * </tr>
 * <tr>
 * <td>#H5FD_FILE_IMAGE_OP_FILE_CLOSE</td><td>This value is passed
 * to the image_free callback when an image buffer is being released during a file close operation.</td>
 * </tr>
 * </table>
 *
 * In closing our discussion of #H5Pset_file_image_callbacks, we note the interaction between this call and
 * the #H5Pget_file_image/#H5Pset_file_image calls above: since the malloc, memcpy, and free callbacks defined
 * in the instance of #H5FD_file_image_callbacks_t are used by #H5Pget_file_image/#H5Pset_file_image,
 * #H5Pset_file_image_callbacks will fail if a file image is already set in the target property list.
 *
 * For more information on writing the file image to disk, set the backing_store parameter. See the
 * #H5Pset_fapl_core entry in the \ref RM.
 *
 * \anchor FI214 <h4>#H5Pget_file_image_callbacks</h4>
 * The #H5Pget_file_image_callbacks routine is designed to obtain the current file image callbacks from a file
 * access property list.
 *
 * The signature of #H5Pget_file_image_callbacks() is defined as follows:
 * \code
 * herr_t H5Pget_file_image_callbacks(hid_t fapl_id, H5FD_file_image_callbacks_t *callbacks_ptr)
 * \endcode
 * The parameters of #H5Pget_file_image_callbacks are defined as follows:
 * \li fapl_id contains the ID of the target file access property list.
 * \li callbacks_ptr contains a pointer to an
 * instance of the #H5FD_file_image_callbacks_t structure. All fields should be initialized to NULL. See
 * the \ref subsubsec_file_image_semantics_cbk section for more information on the
 * #H5FD_file_image_callbacks_t structure.
 *
 * Upon successful return, the fields of callbacks_ptr shall contain values as defined below:
 * \li callbacks_ptr->image_malloc will contain the pointer passed as the image_malloc
 *     field of the instance of #H5FD_file_image_callbacks_t pointed to by the callbacks_ptr parameter of the
 * last call to #H5Pset_file_image_callbacks for the specified FAPL, or NULL if there has been no such call.
 * \li callbacks_ptr->image_memcpy will contain the pointer passed as the image_memcpy field
 *     of the instance of #H5FD_file_image_callbacks_t pointed to by the callbacks_ptr parameter of the last
 * call to #H5Pset_file_image_callbacks for the specified FAPL, or NULL if there has been no such call. \li
 * callbacks_ptr->image_realloc will contain the pointer passed as the image_realloc field of the instance of
 * #H5FD_file_image_callbacks_t pointed to by the callbacks_ptr parameter of the last call to
 * #H5Pset_file_image_callbacks for the specified FAPL, or NULL if there has been no such call. \li
 * callbacks_ptr->image_free_ptr will contain the pointer passed as the image_free field of the instance of
 * #H5FD_file_image_callbacks_t pointed to by the callbacks_ptr parameter of the last call to
 * #H5Pset_file_image_callbacks for the specified FAPL, or NULL if there has been no such call. \li
 * callbacks_ptr->udata_copy will contain the pointer passed as the udata_copy field of the instance of
 * #H5FD_file_image_callbacks_t pointed to by the callbacks_ptr parameter of the last call to
 *     #H5Pset_file_image_callbacks for the specified FAPL, or NULL if there has been no such call.
 * \li callbacks_ptr-> udata_free will contain the pointer passed as the udata_free field of the instance
 *     of #H5FD_file_image_callbacks_t pointed to by the callbacks_ptr parameter of the last call to
 * #H5Pset_file_image_callbacks() for the specified FAPL, or NULL if there has been no such call. \li
 * callbacks_ptr->udata will contain the pointer passed as the udata field of the instance of
 * #H5FD_file_image_callbacks_t pointed to by the callbacks_ptr parameter of the last call to
 * #H5Pset_file_image_callbacks for the specified FAPL, or NULL if there has been no such call.
 *
 * \anchor FI215 <h4>Virtual File Driver Feature Flags</h4>
 * Implementation of the #H5Pget_file_image_callbacks/#H5Pset_file_image_callbacks
 * and #H5Pget_file_image/#H5Pset_file_image function calls requires a pair of
 * virtual file driver feature flags. The flags are #H5FD_FEAT_ALLOW_FILE_IMAGE and
 * #H5FD_FEAT_CAN_USE_FILE_IMAGE_CALLBACKS. Both of these are defined in H5FDpublic.h.
 *
 * The first flag, #H5FD_FEAT_ALLOW_FILE_IMAGE, allows a file driver to indicate whether or not it supports
 * file images. A VFD that sets this flag when its ‘query’ callback is invoked indicates that the file image
 * set in the FAPL will be used as the initial contents of a file. Support for setting an initial file image
 * is designed primarily for use with the Core VFD. However, any VFD can indicate support for this feature by
 * setting the flag and copying the image in an appropriate way for the VFD (possibly by writing the image to
 * a file and then opening the file). However, such a VFD need not employ the file image after file open time.
 * In such cases, the VFD will not make an in-memory copy of the file image and will not employ the file image
 * callbacks.
 *
 * File drivers that maintain a copy of the file in memory (only the Core file driver at present) can be
 * constructed to use the initial image callbacks (if defined). Those that do must set
 * the #H5FD_FEAT_CAN_USE_FILE_IMAGE_CALLBACKS flag, the second flag, when their ‘query’ callbacks are
 * invoked.
 *
 * Thus file drivers that set the #H5FD_FEAT_ALLOW_FILE_IMAGE flag but not the
 * #H5FD_FEAT_CAN_USE_FILE_IMAGE_CALLBACKS flag may read the supplied image from the property list (if
 * present) and use it to initialize the contents of the file. However, they will not discard the image when
 * done, nor will they make any use of any file image callbacks (if defined).
 *
 * If an initial file image appears in a file allocation property list that is used in an H5Fopen() call, and
 * if the underlying file driver does not set the #H5FD_FEAT_ALLOW_FILE_IMAGE flag, then the open will fail.
 *
 * If a driver sets both the #H5FD_FEAT_ALLOW_FILE_IMAGE flag and the #H5FD_FEAT_CAN_USE_FILE_IMAGE_CALLBACKS
 * flag, then that driver will allocate a buffer of the required size, copy the contents of the initial image
 * buffer from the file access property list, and then open the copy as if it had just loaded it from file. If
 * the file image allocation callbacks are defined, the driver shall use them for all memory management tasks.
 * Otherwise it will use the standard malloc, memcpy, realloc, and free C library calls for this purpose.
 *
 * If the VFD sets the #H5FD_FEAT_ALLOW_FILE_IMAGE flag, and an initial file image is defined by an
 * application, the VFD should ensure that file creation operations (as opposed to file open operations)
 * bypass use of the file image, and create a new, empty file.
 *
 * Finally, it is logically possible that a file driver would set the #H5FD_FEAT_CAN_USE_FILE_IMAGE_CALLBACKS
 * flag, but not the #H5FD_FEAT_ALLOW_FILE_IMAGE flag. While it is hard to think of a situation in which this
 * would be desirable, setting the flags this way will not cause any problems: the two capabilities are
 * logically distinct.
 *
 * \anchor FI216 <h4>#H5Fget_file_image</h4>
 * The purpose of the #H5Fget_file_image routine is to provide a simple way to retrieve a copy of the image of
 * an existing, open file. This routine can be used with files opened using the SEC2 (aka POSIX), STDIO,
 * and Core (aka Memory) VFDs.
 *
 * The signature of #H5Fget_file_image is defined as follows:
 * \code
 * ssize_t H5Fget_file_image(hid_t file_id, void *buf_ptr, size_t buf_len)
 * \endcode
 *
 * The parameters of #H5Fget_file_image are defined as follows:
 * \li file_id contains the ID of the target file.
 * \li buf_ptr contains a pointer to the buffer into which the image
 * of the HDF5 file is to be copied. If buf_ptr is NULL, no data will be copied, but the return value will
 * still indicate the buffer size required (or a negative value on error).
 * \li buf_len contains the size of the
 * supplied buffer. If the return value of #H5Fget_file_image is a positive value, then the value will be the
 * length of buffer required to store the file image (in other words, the length of the file). A negative
 * value might be returned if the file is too large to store in the supplied buffer or on failure.
 *
 * The current file size can be obtained via a call to #H5Fget_filesize. Note that this function returns the
 * value of the end of file (EOF) and not the end of address space (EOA). While these values are frequently
 * the same, it is possible for the EOF to be larger than the EOA. Since #H5Fget_file_image will only obtain
 * a copy of the file from the beginning of the superblock to the EOA, it will be best to use
 * #H5Fget_file_image to determine the size of the buffer required to contain the image.
 *
 * <h4>Other Design Considerations</h4>
 *
 * Here are some other notes regarding the design and implementation of #H5Fget_file_image.
 *
 * The #H5Fget_file_image call should be part of the high-level library. However, a file driver agnostic
 * implementation of the routine requires access to data structures that are hidden within the HDF5 Library.
 * We chose to implement the call in the library proper rather than expose those data structures.
 *
 * There is no reason why the #H5Fget_file_image API call could not work on files opened with any file driver.
 * However, the Family, Multi, and Split file drivers have issues that make the call problematic. At present,
 * files opened with the Family file driver are marked as being created with that file driver in the
 * superblock, and the HDF5 Library refuses to open files so marked with any other file driver. This negates
 * the purpose of the #H5Fget_file_image call. While this mark can be removed from the image, the necessary
 * code is not trivial.
 *
 * Thus we will not support the Family file driver in #H5Fget_file_image unless there is demand for it. Files
 * created with the Multi and Split file drivers are also marked in the superblock. In addition, they
 * typically use a very sparse address space. A sparse address space would require the use of an impractically
 * large buffer for an image, and most of the buffer would be empty. So, we see no point in supporting the
 * Multi and Split file drivers in #H5Fget_file_image under any foreseeable circumstances.
 *
 * \subsubsection subsubsec_file_image_api_high High-level C API Routine
 * The #H5LTopen_file_image high-level routine encapsulates the capabilities of routines in the main
 * HDF5 Library with conveniently accessible abstractions.
 *
 * \anchor FI221 <h4>#H5LTopen_file_image</h4>
 * The #H5LTopen_file_image routine is designed to provide an easier way to open an initial file image
 * with the Core VFD. Flags to #H5LTopen_file_image allow for various file image buffer ownership policies
 * to be requested. See the \ref RM for more information on high-level APIs.
 *
 * The signature of #H5LTopen_file_image is defined as follows:
 * \code
 *     hid_t H5LTopen_file_image(void *buf_ptr, size_t buf_len, unsigned flags)
 * \endcode
 *
 * The parameters of #H5LTopen_file_image are defined as follows:
 * \li buf_ptr contains a pointer to the supplied initial image. A NULL value is invalid and will
 *     cause #H5LTopen_file_image to fail.
 * \li buf_len contains the size of the supplied buffer. A value of 0 is invalid and will cause
 * #H5LTopen_file_image to fail. \li flags contains a set of flags indicating whether the image is to be
 * opened read/write, whether HDF5 is to take control of the buffer, and how long the application promises to
 * maintain the buffer. Possible flags are described in the table below: <table> <caption id="FITable3">Flags
 * for #H5LTopen_file_image</caption> <tr> <td>#H5LT_FILE_IMAGE_OPEN_RW</td><td>Indicates that the HDF5
 * Library should open the image read/write instead of the default read-only.</td>
 * </tr>
 * <tr>
 * <td>#H5LT_FILE_IMAGE_DONT_COPY</td><td>Indicates that the HDF5 Library should not copy the file image
 * buffer provided, but should use it directly. The HDF5 Library will release the file image when finished.
 * The supplied buffer must have been allocated via a call to the standard C library malloc() or calloc()
 * routines. The HDF5 Library will call free() to release the buffer. In the absence of this flag, the HDF5
 * Library will copy the buffer provided. The #H5LT_FILE_IMAGE_DONT_COPY flag provides an application with
 * the ability to “give ownership” of a file image buffer to the HDF5 Library.<br />
 * The HDF5 Library will modify the buffer on write if the image is opened read/write and
 * the #H5LT_FILE_IMAGE_DONT_COPY flag is set.<br />
 * The #H5LT_FILE_IMAGE_DONT_RELEASE flag, see below, is invalid unless the #H5LT_FILE_IMAGE_DONT_COPY flag is
 * set.</td>
 * </tr>
 * <tr>
 * <td>#H5LT_FILE_IMAGE_DONT_RELEASE</td><td>Indicates that the HDF5 Library should not attempt to release the
 * buffer when the file is closed. This implies that the application will tend to this detail and that the
 * application will not discard the buffer until after the file image is closed.<br />
 * Since there is no way to return a changed buffer base address to the application, and since realloc
 * can change this value, calls to realloc() must be barred when this flag is set. As a result, any
 * write that requires an increased buffer size will fail.<br />
 * This flag is invalid unless the #H5LT_FILE_IMAGE_DONT_COPY flag, see above, is set.<br />
 * If the #H5LT_FILE_IMAGE_DONT_COPY flag is set and this flag is not set, the HDF5 Library will release the
 * file image buffer after the file is closed using the standard C library free() routine.<br />
 * Using this flag and the #H5LT_FILE_IMAGE_DONT_COPY flag provides a way for the application to specify a
 * buffer that the HDF5 Library can use for opening and accessing as a file image while letting
 * the application retain ownership of the buffer.</td>
 * </tr>
 * </table>
 *
 * The following table is intended to summarize the semantics of the #H5LT_FILE_IMAGE_DONT_COPY
 * and #H5LT_FILE_IMAGE_DONT_RELEASE flags (shown as “Don’t Copy Flag” and “Don’t Release Flag”
 * respectively in the table):
 *
 * <table>
 * <caption id="FITable4">Summary of Don’t Copy and Don’t Release Flag Actions</caption>
 * <tr>
 * <th>Don’t Copy Flag</th><th>Don’t Release Flag</th><th>Make Copy of User Supplied Buffer</th><th>Pass User
 * Supplied Buffer to File Driver</th><th>Release User Supplied Buffer When Done</th><th>Permit realloc of
 * Buffer Used by File Driver</th>
 * </tr>
 * <tr>
 * <td>False</td><td>Don’t care</td><td>True</td><td>False</td><td>False</td><td>True</td>
 * </tr>
 * <tr>
 * <td>True</td><td>False</td><td>False</td><td>True</td><td>True</td><td>True</td>
 * </tr>
 * <tr>
 * <td>True</td><td>True</td><td>False</td><td>True</td><td>False</td><td>False</td>
 * </tr>
 * </table>
 *
 * The return value of #H5LTopen_file_image will be a file ID on success or a negative value on failure.
 * The file ID returned should be closed with #H5Fclose.
 *
 * Note that there is no way currently to specify a “backing store” file name in this definition of
 * #H5LTopen_file_image.
 *
 * \subsection subsec_file_image_semantics C API Call Semantics
 * The purpose of this chapter is to describe some issues that developers should consider when
 * using file image buffers, property lists, and callback APIs.
 *
 * \subsubsection subsubsec_file_image_semantics_cbk File Image Callback Semantics
 * The #H5Pget_file_image_callbacks/#H5Pset_file_image_callbacks API calls allow an application to hook the
 * memory management operations used when allocating, duplicating, and discarding file images in the property
 * list, in the Core file driver, and potentially in any in-memory file driver developed in the future.
 *
 * From the perspective of the HDF5 Library, the supplied image_malloc(), image_memcpy(), image_realloc(),
 * and image_free() callback routines must function identically to the C standard library malloc(),
 * memcpy(), realloc(), and free() calls. What happens on the application side can be much more nuanced,
 * particularly with the ability to pass user data to the callbacks. However, whatever the application
 * does with these calls, it must maintain the illusion that the calls have had the expected effect.
 * Maintaining this illusion requires some understanding of how the property list structure works, and what
 * HDF5 will do with the initial images passed to it.
 *
 * At the beginning of this document, we talked about the need to work within the constraints of the property
 * list mechanism. When we said “from the perspective of the HDF5 Library…” in the paragraph above, we are
 * making reference to this point.
 *
 * The property list mechanism was developed as a way to add parameters to functions without changing the
 * parameter list and breaking existing code. However, it was designed to use only “call by value” semantics,
 * not “call by reference”. The decision to use “call by value” semantics requires that the values of supplied
 * variables be copied into the property list. This has the advantage of simplifying the copying and deletion
 * of property lists. However, if the value to be copied is large (say a 2 GB file image), the overhead can be
 * unacceptable.
 *
 * The usual solution to this problem is to use “call by reference” where only a pointer to an object is
 * placed in a parameter list rather than a copy of the object itself. However, use of “call by reference”
 * semantics would greatly complicate the property list mechanism: at a minimum, it would be necessary to
 * maintain reference counts to dynamically allocated objects so that the owner of the object would know when
 * it was safe to free the object.
 *
 * After much discussion, we decided that the file image operations calls were sufficiently specialized that
 * it made no sense to rework the property list mechanism to support “call by reference.” Instead we provided
 * the file image callback mechanism to allow the user to implement some version of “call by reference” when
 * needed. It should be noted that we expect this mechanism to be used rarely if at all. For small file
 * images, the copying overhead should be negligible, and for large images, most use cases should be addressed
 * by the #H5LTopen_file_image call.
 *
 * In the (hopefully) rare event that use of the file image callbacks is necessary, the fundamental point to
 * remember is that the callbacks must be constructed and used in such a way as to maintain the library’s
 * illusion that it is using “call by value” semantics.
 *
 * Thus the property list mechanism must think that it is allocating a new buffer and copying the supplied
 * buffer into it when the file image property is set. Similarly, it must think that it is allocating a new
 * buffer and copying the contents of the existing buffer into it when it copies a property list that contains
 * a file image. Likewise, it must think it is de-allocating a buffer when it discards a property list that
 * contains a file image.
 *
 * Similar illusions must be maintained when a file image buffer is copied into the Core file driver (or any
 * future driver that uses the file image callbacks) when the file driver re-sizes the buffer containing the
 * image and finally when the driver discards the buffer.
 *
 * \anchor FI311 <h4>Buffer Ownership</h4>
 * The owner of a file image in a buffer is the party that has the responsibility to discard the file
 * image buffer when it is no longer needed. In this context, the owner is either the HDF5 Library or
 * the application program.
 *
 * We implemented the image_* callback facility to allow efficient management of large file images. These
 * facilities can be used to allow sharing of file image buffers between the application and the HDF5 library,
 * and also transfer of ownership in either direction. In such operations, care must be taken to ensure that
 * ownership is clear and that file image buffers are not discarded before all references to them are
 * discarded by the non-owning party.
 *
 * Ownership of a file image buffer will only be passed to the application program if the file image callbacks
 * are designed to do this. In such cases, the application program must refrain from freeing the buffer until
 * the library has deleted all references to it. This in turn will happen after all property lists (if any)
 * that refer to the buffer have been discarded, and the file driver (if any) that used the buffer has closed
 * the file and thinks it has discarded the buffer.
 *
 * \anchor FI312 <h4>Sharing a File image Buffer with the HDF5 Library</h4>
 * As mentioned above, the HDF5 property lists are a mechanism for passing values into HDF5 Library calls.
 * They were created to allow calls to be extended with new parameters without changing the actual API or
 * breaking existing code. They were designed based on the assumption that all new parameters would
 * be “call by value” and not “call by reference.” Having “call by value” parameters means property lists
 * can be copied, reused, and discarded with ease.
 *
 * Suppose an application wished to share a file image buffer with the HDF5 Library. This means the
 * library would be allowed to read the file image, but not free it. The file image callbacks might
 * be constructed as follows to share a buffer:
 * \li Construct the image_malloc() call so that it returns the address of the buffer instead of allocating
 *     new space. This will keep the library thinking that the buffers are distinct even when they are not.
 *     Support this by including the address of the buffer in the user data. As a sanity check, include
 *     the buffer’s size in the user data as well, and require image_malloc() to fail if the requested
 *     buffer size is unexpected. Finally, include a reference counter in the user data, and increment
 *     the reference counter on each call to image_malloc().
 * \li Construct the image_memcpy() call so that it
 *     does nothing. As a sanity check, make it fail if the source and destination pointers do not match
 *     the buffer address in the user data or if the size is unexpected.
 * \li Construct the image_free()
 *     routine so that it does nothing. As a sanity check, make it compare the supplied pointer with the
 *     expected pointer in the user data. Also, make it decrement the reference counter and notify the
 *     application that the HDF5 Library is done with the buffer when the reference count drops to 0.
 *
 * As  the property list code will never resize a buffer, we do not discuss the image_realloc() call here.
 * The behavior of image_realloc() in this scenario depends on what the application wants to do with
 * the file image after it has been opened. We discuss this issue in the next section. Note also that
 * the operation passed into the file image callbacks allow the callbacks to behave differently depending
 * on the context in which they are used.
 *
 * For more information on user defined data, see the \ref subsubsec_file_image_semantics_cbk section.
 *
 * \anchor FI313 <h4>File Driver Considerations</h4>
 * When a file image is opened by a driver that sets both the #H5FD_FEAT_ALLOW_FILE_IMAGE and
 * the #H5FD_FEAT_CAN_USE_FILE_IMAGE_CALLBACKS flags, the driver will allocate a buffer large enough
 * for the initial file image and then copy the image from the property list into this buffer. As
 * processing progresses, the driver will reallocate the image as necessary to increase its size and
 * will eventually discard the image at file close. If defined, the driver will use the file image
 * callbacks for these operations; otherwise, the driver will use the standard C library calls. See
 * the \ref subsubsec_file_image_semantics_cbk section for more information.
 *
 * As described above, the file image callbacks can be constructed so as to avoid the overhead of buffer
 * allocations and copies while allowing the HDF5 Library to maintain its illusions on the subject. There
 * are two possible complications involving the file driver. The complications are the possibility of
 * reallocation calls from the driver and the possibility of the continued existence of property lists
 * containing references to the buffer.
 *
 * Suppose an application wishes to share a file image buffer with the HDF5 Library. The application allows
 * the library to read (and possibly write) the image, but not free it. We must first decide whether the
 * image is to be opened read-only or read/write.
 *
 * If the image will be opened read-only (or if we know that any writes will not change the size of the
 * image), the image_realloc() call should never be invoked. Thus the image_realloc() routine can be
 * constructed so as to always fail, and the image_malloc(), image_memcpy(), and image_free() routines can be
 * constructed as described in the section above.
 *
 * Suppose, however, that the file image will be opened read/write and may grow during the computation. We
 * must now allow for the base address of the buffer to change due to reallocation calls, and we must employ
 * the user data structure to communicate any change in the buffer base address and size to the application.
 * We pass buffer changes to the application so that the application will be able to eventually free the
 * buffer. To this end, we might define a user data structure as shown in the example below:
 *
 * <em>Using a user data structure to communicate with an application</em>
 * \code
 *  typedef struct udata {
 *      void  *init_ptr;
 *      size_t init_size;
 *      int    init_ref_count;
 *      void  *mod_ptr;
 *      size_t mod_size;
 *      int    mod_ref_count;
 *  }
 * \endcode
 *
 * We initialize an instance of the structure so that init_ptr points to the buffer to be shared,
 * init_size contains the initial size of the buffer, and all other fields are initialized to
 * either NULL or 0 as indicated by their type. We then pass a pointer to the instance of the
 * user data structure to the HDF5 Library along with allocation callback functions constructed as follows:
 * \li Construct the image_malloc() call so that it returns the value in the init_ptr field of the user data
 *     structure and increments the init_ref_count. As a sanity check, the function should fail if the
 * requested size does not match the init_size field in the user data structure or if any of the modified
 * fields have values other than their initial values. \li Construct the image_memcpy() call so that it does
 * nothing. As a sanity check, it should be made to fail if the source, destination, and size parameters do
 * not match the init_ptr and init_size fields as appropriate. \li Construct the image_realloc() call so that
 * it performs a standard realloc. Sanity checking, assuming that the realloc is successful, should be as
 * follows: <ul> <li>If the mod_ptr, mod_size, or mod_ref_count fields of the user data structure still have
 * their initial values, verify that the supplied pointer matches the init_ptr field and that the supplied
 *     size does not match the init_size field. Decrement init_ref_count, set mod_ptr equal to the address
 *     returned by realloc, set mod_size equal to the supplied size, and set mod_ref_count to 1.</li>
 *     <li>If the mod_ptr, mod_size, or mod_ref_count fields of the user data structure are defined, verify
 *     that the supplied pointer matches the value of mod_ptr and that the supplied size does not match
 *     mod_size. Set mod_ptr equal to the value returned by realloc, and set mod_size equal to the supplied
 * size.</li>
 *     </ul>
 *     In both cases, if all sanity checks pass, return the value returned by the realloc call. Otherwise,
 * return NULL. \li Construct the image_free() routine so that it does nothing. Perform sanity checks as
 * follows: <ul> <li>If the #H5FD_FILE_IMAGE_OP_PROPERTY_LIST_CLOSE flag is set, decrement the init_ref_count
 * field of the user data structure. Flag an error if init_ref_count drops below zero.</li> <li>If the
 * #H5FD_FILE_IMAGE_OP_FILE_CLOSE flag is set, check to see if the mod_ptr, mod_size, or mod_ref_count fields
 * of the user data structure have been modified from their initial values. If they have, verify that
 *     mod_ref_count contains 1 and then set that field to zero. If they have not been modified, proceed as
 * per the #H5FD_FILE_IMAGE_OP_PROPERTY_LIST_CLOSE case.</li>
 *     </ul>
 *
 * In either case, if both the init_ref_count and mod_ref_count
 * fields have dropped to zero, notify the application that the HDF5 Library is done with the buffer. If the
 * mod_ptr or mod_size fields have been modified, pass these values on to the application as well.
 *
 * \subsubsection subsubsec_file_image_semantics_init Initial File Image Semantics
 * One can argue whether creating a file with an initial file image is closer to creating a file
 * or opening a file. The consensus seems to be that it is closer to a file open, and thus we shall
 * require that the initial image only be used for calls to #H5Fopen.
 *
 * Whatever our convention, from an internal perspective, opening a file with an initial file image is a
 * bit of both creating a file and opening a file. Conceptually, we will create a file on disk, write the
 * supplied image to the file, close the file, open the file as an HDF5 file, and then proceed as usual
 * (of course, the Core VFD will not write to the file system unless it is configured to do so). This process
 * is similar to a file create: we are creating a file that did not exist on disk to begin with and writing
 * data to it. Also, we must verify that no file of the supplied name is open. However, this process is
 * also similar to a file open: we must read the superblock and handle the usual file open tasks.
 *
 * Implementing the above sequence of actions has a number of implications on the behavior of the #H5Fopen
 * call when an initial file image is supplied:
 * \li #H5Fopen must fail if the target file driver does not set the #H5FD_FEAT_ALLOW_FILE_IMAGE flag and
 *     a file image is specified in the FAPL.
 * \li If the target file driver supports the #H5FD_FEAT_ALLOW_FILE_IMAGE flag, then #H5Fopen must fail if
 *     the file is already open or if a file of the specified name exists.
 * \li Even if the above constraints are satisfied, #H5Fopen must still fail if the image does not contain
 *     a valid (or perhaps just plausibly valid) image of an HDF5 file. In particular, the superblock must
 *     be processed, and the file structure be set up accordingly.
 *
 * See the \ref FI215 section for more information.
 *
 * As we indicated earlier, if an initial file image appears in the property list of an #H5Fcreate call, it is
 * ignored.
 *
 * While the above section on the semantics of the file image callbacks may seem rather gloomy,
 * we get the payback here. The above says everything that needs to be said about initial file
 * image semantics in general. The sub-section below has a few more observations on the Core file driver.
 *
 * \anchor FI321 <h4>Applying Initial File Image Semantics to the Core File Driver</h4>
 * At present, the Core file driver uses the open() and read() system calls to load an HDF5 file image from
 * the file system into RAM. Further, if the backing_store flag is set in the FAPL entry specifying the use of
 * the Core file driver, the Core file driver’s internal image will be used to overwrite the source file on
 * either flush or close. See the #H5Pset_fapl_core entry in the \ref RM for more information.
 *
 * This results in the following observations. In all cases assume that use of the Core file driver has been
 * specified in the FAPL. \li If the file specified in the #H5Fopen call does not exist, and no initial image
 * is specified in the FAPL, the open must fail because there is no source for the initial image needed by the
 * Core file driver. \li If the file specified in the #H5Fopen call does exist, and an initial image is
 * specified in the FAPL, the open must fail because the source of the needed initial image is ambiguous: the
 * file image could be taken either from file or from the FAPL. \li If the file specified in the #H5Fopen call
 * does not exist, and an initial image is specified in the FAPL, the open will succeed. This assumes that the
 * supplied image is valid. Further, if the backing store flag is set, the file specified in the #H5Fopen call
 * will be created, and the contents of the Core file driver’s internal buffer will be written to the new file
 * on flush or close.
 *
 * Thus a call to #H5Fopen can result in the creation of a new HDF5 file in the file system.
 *
 * \subsection subsec_file_image_example Examples
 * The purpose of this chapter is to provide examples of how to read or build an in-memory HDF5 file image.
 *
 * \subsubsection subsubsec_file_image_example_read Reading an In-memory HDF5 File Image
 * The #H5Pset_file_image function call allows the Core file driver to be initialized from an application
 * provided buffer. The following pseudo code illustrates its use:
 *
 * <em>Example 2. Using #H5Pset_file_image to initialize the Core file driver</em>
 * \code
 *     <allocate and initialize buf_len and buf>
 *     <allocate fapl_id>
 *     <set fapl to use Core file driver>
 *     H5Pset_file_image(fapl_id, buf, buf_len);
 *     <discard buf any time after this point>
 *     <open file>
 *     <discard fapl any time after this point>
 *     <read and/or write file as desired, close>
 * \endcode
 *
 * This solution is easy to code, but the supplied buffer is duplicated twice. The first time is in the call
 * to #H5Pset_file_image when the image is duplicated and the duplicate inserted into the property list. The
 * second time is when the file is opened: the image is copied from the property list into the initial buffer
 * allocated by the Core file driver. This is a non-issue for small images, but this could become a
 * significant performance hit for large images.
 *
 * If we want to avoid the extra malloc and memcpy calls, we must decide whether the application should
 * retain ownership of the buffer or pass ownership to the HDF5 Library.
 *
 * The following pseudo code illustrates opening the image read-only using the #H5LTopen_file_image()
 * routine. In this example, the application retains ownership of the buffer and avoids extra buffer
 * allocations and memcpy calls.
 *
 * <em>Example 3. Using H5LTopen_file_image to open a read-only file image where the application retains
 * ownership of the buffer</em> \code <allocate and initialize buf_len and buf> hid_t file_id; unsigned flags
 * = H5LT_FILE_IMAGE_DONT_COPY | H5LT_FILE_IMAGE_DONT_RELEASE; file_id = H5LTopen_file_image(buf, buf_len,
 * flags); <read file as desired, and then close> <discard buf any time after this point> \endcode
 *
 * If the application wants to transfer ownership of the buffer to the HDF5 Library, and the standard
 * C library routine free is an acceptable way of discarding it, the above example can be modified as follows:
 *
 * <em>Example 4. Using H5LTopen_file_image to open a read-only file image where the application transfers
 * ownership of the buffer</em> \code <allocate and initialize buf_len and buf> hid_t file_id; unsigned flags
 * = H5LT_FILE_IMAGE_DONT_COPY; file_id = H5LTopen_file_image(buf, buf_len, flags); <read file as desired, and
 * then close> \endcode
 *
 *  Again, file access is read-only. Read/write access can be obtained via the #H5LTopen_file_image call, but
 *  we will explore that in the section below.
 *
 * \subsubsection subsubsec_file_image_example_const In-memory HDF5 File Image Construction
 * Before the implementation of file image operations, HDF5 supported construction of an image of an
 * HDF5 file in memory with the Core file driver. The #H5Fget_file_image function call allows an
 * application access to the file image without first writing it to disk.
 * See the following code fragment:
 *
 * <em>Example 5. Accessing the image of a file in memory</em>
 * \code
 *     <Open and construct the desired file with the Core file driver>
 *     H5Fflush(fid);
 *     size = H5Fget_file_image(fid, NULL, 0);
 *     buffer_ptr = malloc(size);
 *     H5Fget_file_image(fid, buffer_ptr, size);
 * \endcode
 *
 * The use of #H5Fget_file_image may be acceptable for small images. For large images, the cost of the
 * malloc() and memcpy() operations may be excessive. To address this issue, the #H5Pset_file_image_callbacks
 * call allows an application to manage dynamic memory allocation for file images and memory-based file
 * drivers (only the Core file driver at present). The following code fragment illustrates its use. Note
 * that most error checking is omitted for simplicity and that #H5Pset_file_image is not used to set the
 * initial file image.
 *
 * <em>Example 6. Using #H5Pset_file_image_callbacks to improve memory allocation</em>
 * \code
 *     struct udata_t {
 *         void * image_ptr;
 *         size_t image_size;
 *     } udata = {NULL, 0};
 *
 *     void *image_malloc(size_t size, H5FD_file_image_op_t file_image_op, void *udata) {
 *         ((struct udata_t *)udata)->image_size = size;
 *         return(malloc(size));
 *     }
 *
 *     void *image_memcpy)(void *dest, const void *src, size_t size,
 *             H5FD_file_image_op_t file_image_op, void *udata) {
 *         assert(FALSE); // Should never be invoked in this scenario.
 *         return(NULL); // always fails
 *     }
 *
 *     void image_realloc(void *ptr, size_t size, H5FD_file_image_op_t file_image_op, void *udata) {
 *         ((struct udata_t *)udata)->image_size = size;
 *         return(realloc(ptr, size));
 *     }
 *
 *     herr_t image_free(void *ptr, H5FD_file_image_op_t file_image_op, void *udata) {
 *         assert(file_image_op == H5FD_FILE_IMAGE_OP_FILE_CLOSE);
 *         ((struct udata_t *)udata)->image_ptr = ptr;
 *         return(0); // if we get here, we must have been successful
 *     }
 *
 *     void *udata_copy(void *udata) {
 *         return(udata);
 *     }
 *
 *     herr_t udata_free(void *udata) {
 *         return(0);
 *     }
 *
 *     H5FD_file_image_callbacks_t callbacks = {image_malloc, image_memcpy,
 *              image_realloc, image_free, udata_copy, udata_free, (void *)(&udata)};
 *
 *     <allocate fapl_id>
 *
 *     H5Pset_file_image_callbacks(fapl_id, &callbacks);
 *
 *     <open core file using fapl_id, write file, close it>
 *
 *     assert(udata.image_ptr!= NULL);
 *
 *     // udata now contains the base address and length of the final version of the core file
 *
 *     <use image of file, and then discard it via free()>
 * \endcode
 *
 * The above code fragment gives the application full ownership of the buffer used by the Core file driver
 * after the file is closed, and it notifies the application that the HDF5 Library is done with the buffer by
 * setting udata.image_ptr to something other than NULL. If read access to the buffer is sufficient,
 * the #H5Fget_vfd_handle call can be used as an alternate solution to get access to the base address of
 * the Core file driver’s buffer.
 *
 * The above solution avoids some unnecessary malloc and memcpy calls and should be quite adequate if an
 * image of an HDF5 file is constructed only occasionally. However, if an HDF5 file image must be
 * constructed regularly, and if we can put a strong and tight upper bound on the size of the necessary
 * buffer, then the following pseudo code demonstrates a method of avoiding memory allocation completely.
 * The downside, however, is that buffer is allocated statically. Again, much error checking is omitted for
 * clarity.
 *
 * <em>Example 7. Using #H5Pset_file_image_callbacks with a static buffer</em>
 * \code
 *     char buf[BIG_ENOUGH];
 *     struct udata_t {
 *         void * image_ptr;
 *         size_t image_size;
 *         size_t max_image_size;
 *         int ref_count;
 *     } udata = {(void *)(&(buf[0]), 0, BIG_ENOUGH, 0};
 *
 *     void *image_malloc(size_t size, H5FD_file_image_op_t file_image_op, void *udata) {
 *         assert(size <= ((struct udata_t *)udata)->max_image_size);
 *         assert(((struct udata_t *)udata)->ref_count == 0);
 *         ((struct udata_t *)udata)->image_size = size;
 *         (((struct udata_t *)udata)->ref_count)++;
 *         return((((struct udata_t *)udata)->image_ptr);
 *     }
 *
 *     void *image_memcpy)(void *dest, const void *src, size_t size, H5FD_file_image_op_t file_image_op, void
 * *udata) { assert(FALSE); // Should never be invoked in this scenario. return(NULL); // always fails
 *     }
 *
 *     void *image_realloc(void *ptr, size_t size, H5FD_file_image_op_t file_image_op, void *udata) {
 *         assert(ptr == ((struct udata_t *)udata)->image_ptr);
 *         assert(size <= ((struct udata_t *)udata)->max_image_size);
 *         assert(((struct udata_t *)udata)->ref_count == 1);
 *         ((struct udata_t *)udata)->image_size = size;
 *         return((((struct udata_t *)udata)->image_ptr);
 *     }
 *
 *     herr_t image_free(void *ptr, H5FD_file_image_op_t file_image_op, void *udata) {
 *         assert(file_image_op == H5FD_FILE_IMAGE_OP_FILE_CLOSE);
 *         assert(ptr == ((struct udata_t *)udata)->image_ptr);
 *         assert(((struct udata_t *)udata)->ref_count == 1);
 *         (((struct udata_t *)udata)->ref_count)--;
 *         return(0); // if we get here, we must have been successful
 *     }
 *
 *     void *udata_copy(void *udata) {
 *         return(udata);
 *     }
 *
 *     herr_t udata_free(void *udata) {
 *         return(0);
 *     }
 *
 *     H5FD_file_image_callbacks_t callbacks = {image_malloc, image_memcpy, image_realloc, image_free,
 *                                            udata_copy, udata_free, (void *)(&udata)};
 *     // end of initialization
 *
 *     <allocate fapl_id>
 *
 *     H5Pset_file_image_callbacks(fapl_id, &callbacks);
 *
 *     <open core file using fapl_id>
 *
 *     <discard fapl any time after the open>
 *
 *     <write the file, flush it, and then close it>
 *
 *     assert(udata.ref_count == 0);
 *
 *     // udata now contains the base address and length of the final version of the core file
 *     <use the image of the file>
 *
 *     <reinitialize udata, and repeat the above from the end of initialization onwards to write a new file
 * image> \endcode
 *
 * If we can further arrange matters so that only the contents of the datasets in the HDF5 file image change,
 * but not the structure of the file itself, we can optimize still further by reusing the image and changing
 * only the contents of the datasets after the initial write to the buffer. The following pseudo code shows
 * how this might be done. Note that the code assumes that buf already contains the image of the HDF5 file
 * whose dataset contents are to be overwritten. Again, much error checking is omitted for clarity. Also,
 * observe that the file image callbacks do not support the #H5Pget_file_image call.
 *
 * <em>Example 8. Using #H5Pset_file_image_callbacks where only the datasets change</em>
 * \code
 *     <buf already defined and loaded with file image>
 *
 *     <udata already defined and initialized>
 *
 *     void *image_malloc(size_t size, H5FD_file_image_op_t file_image_op, void *udata) {
 *         assert(size <= ((struct udata_t *)udata)->max_image_size);
 *         assert(size == ((struct udata_t *)udata)->image_size);
 *         assert(((struct udata_t *)udata)->ref_count >= 0);
 *         ((struct udata_t *)udata)->image_size = size;
 *         (((struct udata_t *)udata)->ref_count)++;
 *         return((((struct udata_t *)udata)->image_ptr);
 *     }
 *
 *     void *image_memcpy)(void *dest, const void *src, size_t size,
 *         H5FD_file_image_op_t file_image_op, void *udata) {
 *         assert(dest == ((struct udata_t *)udata)->image_ptr);
 *         assert(src == ((struct udata_t *)udata)->image_ptr);
 *         assert(size <= ((struct udata_t *)udata)->max_image_size);
 *         assert(size == ((struct udata_t *)udata)->image_size);
 *         assert(((struct udata_t *)udata)->ref_count >= 1);
 *         return(dest); // if we get here, we must have been successful
 *     }
 *
 *     void *image_realloc(void *ptr, size_t size, H5FD_file_image_op_t file_image_op, void *udata) {
 *         // One would think that this function is not needed in this scenario, as
 *         // only the contents of the HDF5 file is being changed, not its size or
 *         // structure. However, the Core file driver calls realloc() just before
 *         // close to clip the buffer to the size indicated by the end of the
 *         // address space.
 *         //
 *         // While this call must be supported in this case, the size of
 *         // the image should never change. Hence the function can limit itself
 *         // to performing sanity checks, and returning the base address of the
 *         // statically allocated buffer.
 *         //
 *         assert(ptr == ((struct udata_t *)udata)->image_ptr);
 *         assert(size <= ((struct udata_t *)udata)->max_image_size);
 *         assert(((struct udata_t *)udata)->ref_count >= 1);
 *         assert(((struct udata_t *)udata)->image_size == size);
 *         return((((struct udata_t *)udata)->image_ptr);
 *     }
 *
 *     herr_t image_free(void *ptr, H5FD_file_image_op_t file_image_op, void *udata) {
 *         assert((file_image_op == H5FD_FILE_IMAGE_OP_PROPERTY_LIST_CLOSE) ||
 *                (file_image_op == H5FD_FILE_IMAGE_OP_FILE_CLOSE));
 *         assert(((struct udata_t *)udata)->ref_count >= 1);
 *         (((struct udata_t *)udata)->ref_count)--;
 *         return(0); // if we get here, we must have been successful
 *     }
 *
 *     void *udata_copy(void *udata) {
 *         return(udata);
 *     }
 *
 *     herr_t udata_free(void *udata) {
 *         return(0);
 *     }
 *
 *     H5FD_file_image_callbacks_t callbacks = {image_malloc, image_memcpy, image_realloc, image_free,
 *                                            udata_copy, udata_free, (void *)(&udata)};
 *     // end of initialization
 *
 *     <allocate fapl_id>
 *
 *     H5Pset_file_image_callbacks(fapl_id, &callbacks);
 *     H5Pset_file_image(fapl_id, udata.image_ptr, udata.image_len);
 *
 *     <open core file using fapl_id>
 *
 *     <discard fapl any time after the open>
 *
 *     <overwrite data in datasets in the file, and then close it>
 *
 *     assert(udata.ref_count == 0);
 *
 *     // udata now contains the base address and length of the final version of the core file
 *     <use the image of the file>
 *
 *     <repeat the above from the end of initialization onwards to write new data to datasets in file image>
 * \endcode
 *
 * Before we go on, we should note that the above pseudo code can be written more compactly, albeit with
 * fewer sanity checks, using the #H5LTopen_file_image call. See the example below:
 *
 * <em>Example 9. Using #H5LTopen_file_image where only the datasets change</em>
 * \code
 *     <buf already defined and loaded with file image>
 *
 *     <udata already defined and initialized>
 *
 *     hid_t file_id;
 *
 *     unsigned flags = H5LT_FILE_IMAGE_OPEN_RW | H5LT_FILE_IMAGE_DONT_COPY | H5LT_FILE_IMAGE_DONT_RELEASE;
 *     // end initialization
 *
 *     file_id = H5LTopen_file_image(udata.image_ptr, udata.image_len, flags);
 *
 *     <overwrite data in datasets in the file, and then close it>
 *
 *     // udata now contains the base address and length of the final version of the core file
 *     <use the image of the file>
 *
 *     <repeat the above from the end of initialization onwards to write new data to datasets in file image>
 * \endcode
 *
 * While the scenario above is plausible, we will finish this section with a more general scenario. In
 * the pseudo code below, we assume sufficient RAM to retain the HDF5 file image between uses, but we
 * do not assume that the HDF5 file structure remains constant or that we can place a hard per bound on the
 * image size.
 *
 * Since we must use malloc, realloc, and free in this example, and since realloc can change the base
 * address of a buffer, we must maintain two of ptr, size, and ref_count triples in the udata structure.
 * The first triple is for the property list (which will never change the buffer), and the second triple
 * is for the file driver. As shall be seen, this complicates the file image callbacks considerably.
 * Note also that while we do not use H5Pget_file_image() in this example, we do include support for it
 * in the file image callbacks. As usual, much error checking is omitted in favor of clarity.
 *
 * <em>Example 10. Using #H5LTopen_file_image where only the datasets change and where the file structure and
 * image size might not be constant</em> \code struct udata_t { void* fapl_image_ptr; size_t fapl_image_size;
 *         int fapl_ref_count;
 *         void* vfd_image_ptr;
 *         size_t vfd_image_size;
 *         int vfd_ref_count;
 *     } udata = {NULL, 0, 0, NULL, 0, 0};
 *
 *     boolean initial_file_open = TRUE;
 *     void *image_malloc(size_t size, H5FD_file_image_op_t file_image_op, void *udata) {
 *         void * return_value = NULL;
 *
 *         switch ( file_image_op ) {
 *             case H5FD_FILE_IMAGE_OP_PROPERTY_LIST_SET:
 *             case H5FD_FILE_IMAGE_OP_PROPERTY_LIST_COPY:
 *                 assert(((struct udata_t *)udata)->fapl_image_ptr != NULL);
 *                 assert(((struct udata_t *)udata)->fapl_image_size == size);
 *                 assert(((struct udata_t *)udata)->fapl_ref_count >= 0);
 *                 return_value = ((struct udata_t *)udata)->fapl_image_ptr;
 *                 (((struct udata_t *)udata)->fapl_ref_count)++;
 *                 break;
 *
 *             case H5FD_FILE_IMAGE_OP_PROPERTY_LIST_GET:
 *                 assert(((struct udata_t *)udata)->fapl_image_ptr != NULL);
 *                 assert(((struct udata_t *)udata)->vfd_image_size == size);
 *                 assert(((struct udata_t *)udata)->fapl_ref_count >= 1);
 *                 return_value = ((struct udata_t *)udata)->fapl_image_ptr;
 *                 // don’t increment ref count
 *                 break;
 *
 *             case H5FD_FILE_IMAGE_OP_FILE_OPEN:
 *                 assert(((struct udata_t *)udata)->vfd_image_ptr == NULL);
 *                 assert(((struct udata_t *)udata)->vfd_image_size == 0);
 *                 assert(((struct udata_t *)udata)->vfd_ref_count == 0);
 *                 if (((struct udata_t *)udata)->fapl_image_ptr == NULL ) {
 *                     ((struct udata_t *)udata)->vfd_image_ptr = malloc(size);
 *                     ((struct udata_t *)udata)->vfd_image_size = size;
 *                 }
 *                 else {
 *                     assert(((struct udata_t *)udata)->fapl_image_size == size);
 *                     assert(((struct udata_t *)udata)->fapl_ref_count >= 1);
 *                     ((struct udata_t *)udata)->vfd_image_ptr = ((struct udata_t *)udata)->fapl_image_ptr;
 *                     ((struct udata_t *)udata)->vfd_image_size = size;
 *                 }
 *                 return_value = ((struct udata_t *)udata)->vfd_image_ptr;
 *                 (((struct udata_t *)udata)->vfd_ref_count)++;
 *                 break;
 *
 *             default:
 *                 assert(FALSE);
 *         }
 *         return(return_value);
 *     }
 *
 *     void *image_memcpy)(void *dest, const void *src, size_t size, H5FD_file_image_op_t file_image_op, void
 * *udata) { switch(file_image_op) { case H5FD_FILE_IMAGE_OP_PROPERTY_LIST_SET: case
 * H5FD_FILE_IMAGE_OP_PROPERTY_LIST_COPY: case H5FD_FILE_IMAGE_OP_PROPERTY_LIST_GET: assert(dest == ((struct
 * udata_t *)udata)->fapl_image_ptr); assert(src == ((struct udata_t *)udata)->fapl_image_ptr); assert(size ==
 * ((struct udata_t *)udata)->fapl_image_size); assert(((struct udata_t *)udata)->fapl_ref_count >= 1); break;
 *
 *             case H5FD_FILE_IMAGE_OP_FILE_OPEN:
 *                 assert(dest == ((struct udata_t *)udata)->vfd_image_ptr);
 *                 assert(src == ((struct udata_t *)udata)->fapl_image_ptr);
 *                 assert(size == ((struct udata_t *)udata)->fapl_image_size);
 *                 assert(size == ((struct udata_t *)udata)->vfd_image_size);
 *                 assert(((struct udata_t *)udata)->fapl_ref_count >= 1);
 *                 assert(((struct udata_t *)udata)->vfd_ref_count == 1);
 *                 break;
 *
 *             default:
 *                 assert(FALSE);
 *                 break;
 *         }
 *         return(dest); // if we get here, we must have been successful
 *     }
 *
 *     void *image_realloc(void *ptr, size_t size, H5FD_file_image_op_t file_image_op, void *udata) {
 *         assert(ptr == ((struct udata_t *)udata)->vfd_image_ptr);
 *         assert(((struct udata_t *)udata)->vfd_ref_count == 1);
 *         ((struct udata_t *)udata)->vfd_image_ptr = realloc(ptr, size);
 *         ((struct udata_t *)udata)->vfd_image_size = size;
 *         return((((struct udata_t *)udata)->vfd_image_ptr);
 *     }
 *
 *     herr_t image_free(void *ptr, H5FD_file_image_op_t file_image_op, void *udata) {
 *         switch(file_image_op) {
 *             case H5FD_FILE_IMAGE_OP_PROPERTY_LIST_CLOSE:
 *                 assert(ptr == ((struct udata_t *)udata)->fapl_image_ptr);
 *                 assert(((struct udata_t *)udata)->fapl_ref_count >= 1);
 *                 (((struct udata_t *)udata)->fapl_ref_count)--;
 *                 break;
 *
 *             case H5FD_FILE_IMAGE_OP_FILE_CLOSE:
 *                 assert(ptr == ((struct udata_t *)udata)->vfd_image_ptr);
 *                 assert(((struct udata_t *)udata)->vfd_ref_count == 1);
 *                 (((struct udata_t *)udata)->vfd_ref_count)--;
 *                 break;
 *
 *             default:
 *                 assert(FALSE);
 *                 break;
 *         }
 *         return(0); // if we get here, we must have been successful
 *     }
 *
 *     void *udata_copy(void *udata) {
 *         return(udata);
 *     }
 *
 *     herr_t udata_free(void *udata) {
 *         return(0);
 *     }
 *
 *     H5FD_file_image_callbacks_t callbacks = {image_malloc, image_memcpy, image_realloc, image_free,
 *                                            udata_copy, udata_free, (void *)(&udata)};
 *     // end of initialization
 *
 *     <allocate fapl_id>
 *
 *     H5Pset_file_image_callbacks(fapl_id, &callbacks);
 *     if ( initial_file_open ) {
 *         initial_file_open = FALSE;
 *     }
 *     else {
 *         assert(udata.vfd_image_ptr != NULL);
 *         assert(udata.vfd_image_size > 0);
 *         assert(udata.vfd_ref_count == 0);
 *         assert(udata.fapl_ref_count == 0);
 *         udata.fapl_image_ptr = udata.vfd_image_ptr;
 *         udata.fapl_image_size = udata.vfd_image_size;
 *         udata.vfd_image_ptr = NULL;
 *         udata.vfd_image_size = 0;
 *         H5Pset_file_image(fapl_id, udata.fapl_image_ptr, udata.fapl_image_size);
 *     }
 *
 *     <open core file using fapl_id>
 *
 *     <discard fapl any time after the open>
 *
 *     <write/update the file, and then close it>
 *
 *     assert(udata.fapl_ref_count == 0);
 *     assert(udata.vfd_ref_count == 0);
 *
 *     // udata.vfd_image_ptr and udata.vfd_image_size now contain the base address
 *     // and length of the final version of the core file
 *     <use the image of the file>
 *
 *     <repeat the above from the end of initialization to modify the file image as needed>
 *
 *     <free the image when done>
 * \endcode
 *
 * The above pseudo code shows how a buffer can be passed back and forth between the application and the
 * HDF5 Library. The code also shows the application having control of the actual allocation, reallocation,
 * and freeing of the buffer.
 *
 * \subsubsection subsubsec_file_image_example_dp Using HDF5 to Construct and Read a Data Packet
 * Using the file image operations described in this document, we can bundle up data in an image of an
 * HDF5 file on one process, transmit the image to a second process, and then open and read the image on
 * the second process without any mandatory file system I/O.
 *
 * We have already demonstrated the construction and reading of such buffers above, but it may be useful
 * to offer an example of the full operation. We do so in the example below using as simple a set of calls
 * as possible. The set of calls in the example has extra buffer allocations. To reduce extra buffer
 * allocations, see the sections above.
 *
 * In the following example, we construct an HDF5 file image on process A and then transmit the image to
 * process B where we then open the image and extract the desired data. Note that no file system I/O is
 * performed: all the processing is done in memory with the Core file driver.
 *
 * <em>Example 11. Building and passing a file image from one process to another</em>
 * <table>
 * <tr><th>*** Process A ***</th><th>*** Process B ***</th></tr>
 * <tr><td>
 * \code
 * <Open and construct the desired file with the Core file driver>
 * H5Fflush(fid);
 * size = H5Fget_file_image(fid, NULL, 0);
 * buffer_ptr = malloc(size);
 * H5Fget_file_image(fid, buffer_ptr, size);
 * <transmit size>
 * <transmit *buffer_ptr>
 * free(buffer_ptr);
 * <close core file>
 * \endcode
 * </td>
 * <td>
 * \code
 * hid_t file_id;
 * <receive size>
 * buffer_ptr = malloc(size)
 * <receive image in *buffer_ptr>
 * file_id = H5LTopen_file_image(buf,
 *  buf_len,
 *  H5LT_FILE_IMAGE_DONT_COPY);
 * <read data from file, then close.
 * note that the Core file driver
 * will discard the buffer on close>
 * \endcode
 * </td></tr>
 * </table>
 *
 * \subsubsection subsubsec_file_image_example_template Using a Template File
 * After the above examples, an example of the use of a template file might seem anti-climactic. A
 * template file might be used to enforce consistency on file structure between files or in parallel
 * HDF5 to avoid long sequences of collective operations to create the desired groups, datatypes,
 * and possibly datasets. The following pseudo code outlines a potential use:
 *
 * <em>Example 12. Using a template file</em>
 * \code
 *     <allocate and initialize buf and buflen, with buf containing the desired initial
 *      image (which in turn contains the desired group, datatype, and dataset
 *      definitions), and buf_len containing the size of buf>
 *
 *     <allocate fapl_id>
 *
 *     <set fapl to use desired file driver that supports initial images>
 *
 *     <H5Pset_file_image(fapl_id, buf, buf_len);
 *
 *     <discard buf any time after this point>
 *
 *     <open file>
 *
 *     <discard fapl any time after this point>
 *
 *     <read and/or write file as desired, close>
 * \endcode
 *
 * Observe that the above pseudo code includes an unnecessary buffer allocation and copy in the call
 * to #H5Pset_file_image. As we have already discussed ways of avoiding this, we will not address that issue
 * here.
 *
 * What is interesting in this case is to consider why the application would find this use case attractive.
 *
 * In the serial case, at first glance there seems little reason to use the initial image facility at all.
 * It is easy enough to use standard C calls to duplicate a template file, rename it as desired, and then
 * open it as an HDF5 file.
 *
 * However, this assumes that the template file will always be available and in the expected place. This
 * is a questionable assumption for an application that will be widely distributed. Thus, we can at least
 * make an argument for either keeping an image of the template file in the executable or for including
 * code for writing the desired standard definitions to new HDF5 files.
 *
 * Assuming the image is relatively small, we can further make an argument for the image in place of the code,
 * as, quite simply, the image should be easier to maintain and modify with an HDF5 file editor.
 *
 * However, there remains the question of why one should pass the image to the HDF5 Library instead of writing
 * it directly with standard C calls and then using HDF5 to open it. Other than convenience and a slight
 * reduction in code size, we are hard pressed to offer a reason.
 *
 * In contrast, the argument is stronger in the parallel case since group, datatype, and dataset creations are
 * all expensive collective operations. The argument is also weaker: simply copying an existing template file
 * and opening it should lose many of its disadvantages in the HPC context although we would imagine that it
 * is always useful to reduce the number of files in a deployment.
 *
 * In closing, we would like to consider one last point. In the parallel case, we would expect template files
 * to be quite large. Parallel HDF5 requires eager space allocation for chunked datasets. For similar reasons,
 * we would expect template files in this context to contain long sequences of zeros with a scattering of
 * metadata here and there. Such files would compress well, and the compressed images would be cheap to
 * distribute across the available processes if necessary. Once distributed, each process could uncompress the
 * image and write to file those sections containing actual data that lay within the section of the file
 * assigned to the process. This approach might be significantly faster than a simple copy as it would allow
 * sparse writes, and thus it might provide a compelling use case for template files. However, this approach
 * would require extending our current API to allow compressed images. We would also have to add the
 * H5Pget_image_decompression_callback/H5Pset_image_decompression_callback API calls. We see no problem in
 * doing this. However, it is beyond the scope of the current effort, and thus we will not pursue the matter
 * further unless there is interest in our doing so.
 *
 * \subsection subsec_file_image_java Java Signatures for File Image Operations API Calls
 * Java function call signatures for the file image operation APIs have not yet been implemented, and there
 * are no immediate plans for implementation.
 *
 * \subsection subsec_file_image_fort Fortran Signatures for File Image Operations API Calls
 * Fortran function call signatures for the file image operation APIs are described in this section.
 *
 * \subsubsection subsubsec_file_image_fort_low Fortran Low-Level APIs
 * The Fortran low-level APIs make use of Fortran 2003’s ISO_C_BINDING module in order to achieve portable
 * and standard conforming interoperability with the C APIs. The C pointer (C_PTR) and function pointer
 * (C_FUN_PTR) types are returned from the intrinsic procedures C_LOC(X) and C_FUNLOC(X), respectively,
 * defined in the ISO_C_BINDING module. The argument X is the data or function to which the C pointers point
 * to and must have the TARGET attribute in the calling program. Note that the variable name lengths of the
 * Fortran equivalent of the predefined C constants were shortened to less than 31 characters in order to be
 * Fortran standard compliant.
 *
 * <table>
 * <tr><th>h5pget_file_image_f</th></tr>
 * <tr><td>
 * \code
 *      SUBROUTINE h5pget_file_image_f(fapl_id, buf_ptr, buf_len_ptr, hdferr)
 *          IMPLICIT NONE
 *          INTEGER(HID_T) , INTENT(IN)               :: fapl_id
 *          TYPE(C_PTR)    , INTENT(IN), DIMENSION(*) :: buf_ptr
 *          INTEGER(SIZE_T), INTENT(OUT)              :: buf_len_ptr
 *          INTEGER        , INTENT(OUT)              :: hdferr
 * \endcode
 * </td></tr>
 * </table>
 *
 * <table>
 * <tr><th>h5pset_file_image_f</th></tr>
 * <tr><td>
 * \code
 *       SUBROUTINE h5pset_file_image_f(fapl_id, buf_ptr, buf_len, hdferr)
 *           IMPLICIT NONE
 *           INTEGER(HID_T) , INTENT(IN)  :: fapl_id
 *           TYPE(C_PTR)    , INTENT(IN)  :: buf_ptr
 *           INTEGER(SIZE_T), INTENT(IN)  :: buf_len
 *           INTEGER        , INTENT(OUT) :: hdferr
 * \endcode
 * </td></tr>
 * </table>
 *
 * <table>
 * <tr><th>h5fget_file_image_f</th></tr>
 * <tr><td>
 * \code
 *       SUBROUTINE h5fget_file_image_f(file_id, buf_ptr, buf_len, hdferr, buf_size)
 *           IMPLICIT NONE
 *           INTEGER(HID_T) , INTENT(IN)              :: file_id
 *           TYPE(C_PTR)    , INTENT(INOUT)           :: buf_ptr
 *           INTEGER(SIZE_T), INTENT(IN)              :: buf_len
 *           INTEGER        , INTENT(OUT)             :: hdferr
 *           INTEGER(SIZE_T), INTENT(OUT)  , OPTIONAL :: buf_size
 * \endcode
 * </td></tr>
 * </table>
 *
 * \subsubsection subsubsec_file_image_fort_high Fortran High-Level APIs
 * Fortran function call signatures for the file image operation APIs have not yet been implemented yet.
 *
 * Previous Chapter \ref sec_vol - Next Chapter \ref sec_async
 *
 */

/**
 * \defgroup H5F Files (H5F)
 *
 * Use the functions in this module to manage HDF5 files.
 *
 * In the code snippets below, we show the skeletal life cycle of an HDF5 file,
 * when creating a new file (left) or when opening an existing file (right).
 * File creation is essentially controlled through \ref FCPL, and file access to
 * new and existing files is controlled through \ref FAPL. The file \c name and
 * creation or access \c mode control the interaction with the underlying
 * storage such as file systems.
 *
 * <table>
 * <tr><th>Create</th><th>Read</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5F_examples.c create
 *   </td>
 *   <td>
 *   \snippet{lineno} H5F_examples.c read
 *   </td>
 * </tr>
 * <tr><th>Update</th><th>Delete</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5F_examples.c update
 *   </td>
 *   <td>
 *   \snippet{lineno} H5F_examples.c delete
 *   </td>
 * </tr>
 * </table>
 *
 * In addition to general file management functions, there are three categories
 * of functions that deal with advanced file management tasks and use cases:
 * 1. The control of the HDF5 \ref MDC
 * 2. The use of (MPI-) \ref PH5F HDF5
 * 3. The \ref SWMRTN pattern
 *
 * \defgroup MDC Metadata Cache
 * \ingroup H5F
 * \defgroup PH5F Parallel
 * \ingroup H5F
 * \defgroup SWMR Single Writer Multiple Readers
 * \ingroup H5F
 */

#endif /* H5Fmodule_H */
