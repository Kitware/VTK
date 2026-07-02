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
 *          H5FD package.  Including this header means that the source file
 *          is part of the H5FD package.
 */
#ifndef H5FDmodule_H
#define H5FDmodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5FD_MODULE
#define H5_MY_PKG      H5FD
#define H5_MY_PKG_INIT YES

/** \page H5FD_UG HDF5 Virtual File Drivers
 *
 * Navigate back: \ref index "Main" / \ref UG
 * <hr>
 *
 * \section sec_vfd The HDF5 Virtual File Driver Interface
 *
 * \subsection subsec_vfd_intro Introduction
 *
 * The HDF5 Virtual File Driver (VFD) interface provides an abstraction layer for file I/O operations,
 * enabling HDF5 to work with different file storage mechanisms. The VFD layer intercepts all low-level
 * file access operations and forwards them to a specific driver implementation, allowing HDF5 files
 * to be stored in various ways beyond simple POSIX files.
 *
 * @see H5FD Reference Manual
 *
 * \subsection subsec_vfd_purpose Purpose and Benefits
 *
 * The Virtual File Driver interface serves several important purposes:
 *
 * \li \Bold{Storage Flexibility}: Enables HDF5 to work with different storage backends including
 *     local files, parallel file systems, memory, and cloud storage.
 *
 * \li \Bold{Performance Optimization}: Allows selection of file drivers optimized for specific
 *     computing environments and I/O patterns.
 *
 * \li \Bold{Parallel I/O}: Provides support for MPI-based parallel I/O through specialized drivers.
 *
 * \li \Bold{Custom Storage}: Enables development of custom file drivers for specialized storage
 *     requirements.
 *
 * \subsection subsec_vfd_drivers Built-in File Drivers
 *
 * HDF5 includes several standard Virtual File Drivers:
 *
 * \li \Bold{SEC2 Driver}: The default POSIX I/O driver using standard system calls like read() and
 *     write(). Suitable for most serial applications on local file systems. Set with #H5Pset_fapl_sec2.
 *
 * \li \Bold{STDIO Driver}: Uses buffered I/O from the C standard library (fread/fwrite). May provide
 *     better performance for some applications. Set with #H5Pset_fapl_stdio.
 *
 * \li \Bold{Core Driver}: Stores the HDF5 file entirely in memory, with optional backing store to disk.
 *     Provides fastest I/O for temporary files or small datasets. Set with #H5Pset_fapl_core.
 *
 * \li \Bold{Family Driver}: Splits a logical HDF5 file across multiple physical files of equal size.
 *     Useful for circumventing file system limitations. Set with #H5Pset_fapl_family.
 *
 * \li \Bold{Multi Driver}: Stores different types of HDF5 data in separate files (metadata, raw data,
 *     etc.). Can optimize I/O by placing different data types on different storage devices. Set with
 *     #H5Pset_fapl_multi.
 *
 * \li \Bold{Split Driver}: A simplified version of the Multi driver that separates metadata and raw
 *     data into two files. Set with #H5Pset_fapl_split.
 *
 * \li \Bold{Log Driver}: Wraps another driver and logs all file access operations. Useful for
 *     debugging and I/O profiling. Set with #H5Pset_fapl_log.
 *
 * \li \Bold{MPI-IO Driver}: Enables parallel I/O using MPI-IO for HPC applications. Required for
 *     parallel HDF5 operations. Set with #H5Pset_fapl_mpio.
 *
 * \li \Bold{Subfiling Driver}: A parallel I/O driver that improves parallel I/O performance on parallel
 *     file systems by splitting the logical HDF5 file into multiple subfiles distributed across I/O
 *     concentrator nodes. Reduces contention and improves scalability for large-scale parallel
 *     applications. Set with #H5Pset_fapl_subfiling.
 *
 * \li \Bold{Direct Driver}: Uses direct I/O (O_DIRECT) to bypass OS caching. Can improve performance
 *     for large sequential I/O. Set with #H5Pset_fapl_direct.
 *
 * \li \Bold{Onion Driver}: Provides revision control for HDF5 files by storing file modifications
 *     as separate revisions. Enables tracking changes over time and accessing previous versions.
 *     Set with #H5Pset_fapl_onion.
 *
 * \li \Bold{Splitter Driver}: Writes file operations simultaneously to two different channels using
 *     different VFDs. Useful for creating redundant copies or logging I/O to separate locations.
 *     Set with #H5Pset_fapl_splitter.
 *
 * \li \Bold{Mirror Driver}: Mirrors all file operations to a remote server in real-time over a
 *     network connection. Enables remote backup and replication scenarios. Set with
 *     #H5Pset_fapl_mirror.
 *
 * \li \Bold{ROS3 Driver}: Read-only driver for accessing HDF5 files in S3-compatible object storage.
 *     Set with #H5Pset_fapl_ros3.
 *
 * \li \Bold{HDFS Driver}: Read-only driver for accessing HDF5 files in Hadoop Distributed File System.
 *     Set with #H5Pset_fapl_hdfs.
 *
 * \subsection subsec_vfd_selection Selecting a File Driver
 *
 * File drivers are selected through the file access property list when opening or creating a file.
 * The basic pattern is:
 *
 * \li Create a file access property list with #H5Pcreate
 * \li Set the desired file driver using the appropriate H5Pset_fapl_* function
 * \li Pass the property list to #H5Fcreate or #H5Fopen
 *
 * \subsection subsec_vfd_custom Custom File Drivers
 *
 * Applications can implement custom file drivers by:
 *
 * \li Defining a #H5FD_class_t structure with function pointers for all required operations
 * \li Implementing the driver callbacks (open, close, read, write, etc.)
 * \li Registering the driver with #H5FDregister
 * \li Setting the driver in a file access property list with #H5Pset_driver
 *
 * Custom drivers enable specialized I/O strategies such as:
 * \li Integration with custom storage systems
 * \li Transparent encryption or compression at the I/O layer
 * \li Specialized caching strategies
 * \li Network-based storage protocols
 *
 * \subsection subsec_vfd_parallel Parallel File Drivers
 *
 * For parallel HDF5 applications, the MPI-IO file driver is required (see \ref IntroParHDF5 for
 * details on parallel HDF5 programming). This driver coordinates file access across multiple
 * MPI processes, enabling collective I/O operations and preventing conflicts. Parallel applications must:
 *
 * \li Build HDF5 with parallel support enabled
 * \li Use the MPI-IO file driver via #H5Pset_fapl_mpio
 * \li Provide MPI communicator and info objects
 * \li Coordinate file access across processes
 *
 * The Subfiling driver provides additional performance benefits for
 * large-scale parallel applications on parallel file systems. It works by:
 *
 * \li Distributing the HDF5 file across multiple subfiles
 * \li Designating I/O concentrator processes (typically one per node)
 * \li Striping data across subfiles to reduce contention
 * \li Enabling better parallel I/O scaling on Lustre, GPFS, and similar file systems
 *
 * The Subfiling driver is particularly beneficial when running at scale on parallel file systems
 * where a single shared file can become a bottleneck.
 *
 * \subsection subsec_vfd_performance Performance Considerations
 *
 * Choosing the right file driver can significantly impact I/O performance:
 *
 * \li \Bold{Local Files}: SEC2 or STDIO drivers typically provide good performance
 * \li \Bold{Temporary Data}: Core driver provides fastest access by avoiding disk I/O
 * \li \Bold{Large Files}: Family driver can work around file size limitations
 * \li \Bold{Parallel Applications}: MPI-IO driver required for coordinated parallel access
 * \li \Bold{Large-Scale Parallel}: Subfiling driver can dramatically improve performance on shared
 *     parallel file systems by reducing metadata contention and enabling better striping
 * \li \Bold{Network Storage}: Consider drivers optimized for network protocols
 * \li \Bold{Redundancy}: Splitter or Mirror drivers enable real-time backup and replication
 * \li \Bold{Versioning}: Onion driver enables tracking file revisions for provenance and rollback
 *
 * \subsection subsec_vfd_query Querying File Driver Information
 *
 * Applications can query the current file driver:
 *
 * \li #H5Pget_driver retrieves the driver identifier from a file access property list
 * \li #H5FDis_driver_registered_by_name checks if a specific driver is available by name
 * \li #H5FDis_driver_registered_by_value checks if a specific driver is available by value
 * \li Driver-specific property list functions retrieve driver parameters
 *
 * \subsection subsec_vfd_summary Summary
 *
 * The HDF5 Virtual File Driver interface provides:
 * \li Abstraction of file I/O operations for flexibility and portability
 * \li Multiple built-in drivers for common storage scenarios
 * \li Support for parallel I/O via MPI-IO
 * \li Extensibility through custom driver implementation
 * \li Performance optimization opportunities through driver selection
 *
 * Proper selection and configuration of file drivers is essential for optimal HDF5
 * performance in different computing environments.
 *
 * <hr>
 * Navigate back: \ref index "Main" / \ref UG
 */

/**
 * \defgroup H5FD File Drivers (H5FD)
 *
 * Use the functions in this module to manage HDF5 Virtual File Drivers (VFDs).
 *
 * Virtual File Drivers (VFDs) provide an abstraction layer for file I/O, enabling HDF5 to work
 * with different storage mechanisms. VFDs can be selected to optimize performance for specific
 * environments or to enable specialized storage backends.
 *
 * \defgroup H5VFD Virtual File Driver Features
 * \ingroup H5FD
 *
 */

#endif /* H5FDmodule_H */
