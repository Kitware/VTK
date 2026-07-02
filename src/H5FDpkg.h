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
 * Purpose: This file contains declarations which are visible only within
 *          the H5FD package.  Source files outside the H5FD package should
 *          include H5FDprivate.h instead.
 */
#if !(defined H5FD_FRIEND || defined H5FD_MODULE)
#error "Do not include this file outside the H5FD package!"
#endif

#ifndef H5FDpkg_H
#define H5FDpkg_H

/* Get package's private header */
#include "H5FDprivate.h" /* File drivers				*/

/* Other private headers needed by this file */

/**************************/
/* Package Private Macros */
/**************************/

/* These macros check for overflow of various quantities. They are suitable
 * for VFDs that are "file-like" where lseek(2), etc. is used to move around
 * via HDoff_t units (i.e., most VFDs aside from the core VFD).
 *
 * These macros assume that HDoff_t is signed and haddr_t and size_t are unsigned.
 *
 * H5FD_ADDR_OVERFLOW:   Checks whether a file address of type `haddr_t'
 *                       is too large to be represented by the second argument
 *                       of the file seek function.
 *
 * H5FD_SIZE_OVERFLOW:   Checks whether a buffer size of type `hsize_t' is too
 *                       large to be represented by the `size_t' type.
 *
 * H5FD_REGION_OVERFLOW: Checks whether an address and size pair describe data
 *                       which can be addressed entirely by the second
 *                       argument of the file seek function.
 */
#define H5FD_MAXADDR          (((haddr_t)1 << (8 * sizeof(HDoff_t) - 1)) - 1)
#define H5FD_ADDR_OVERFLOW(A) (HADDR_UNDEF == (A) || ((A) & ~(haddr_t)H5FD_MAXADDR))
#define H5FD_SIZE_OVERFLOW(Z) ((Z) & ~(hsize_t)H5FD_MAXADDR)
#define H5FD_REGION_OVERFLOW(A, Z)                                                                           \
    (H5FD_ADDR_OVERFLOW(A) || H5FD_SIZE_OVERFLOW(Z) || HADDR_UNDEF == (A) + (Z) ||                           \
     (HDoff_t)((A) + (Z)) < (HDoff_t)(A))

/****************************/
/* Package Private Typedefs */
/****************************/

/*****************************/
/* Package Private Variables */
/*****************************/

/* Whether to ignore file locks when disabled (env var value) */
H5_DLLVAR htri_t H5FD_ignore_disabled_file_locks_p;

/******************************/
/* Package Private Prototypes */
/******************************/
H5_DLL haddr_t H5FD__alloc_real(H5FD_t *file, H5FD_mem_t type, hsize_t size, haddr_t *align_addr,
                                hsize_t *align_size);
H5_DLL herr_t  H5FD__free_real(H5FD_t *file, H5FD_mem_t type, haddr_t addr, hsize_t size);

/* Internal VFD init/term routines */
H5_DLL herr_t H5FD__core_register(void);
H5_DLL herr_t H5FD__core_unregister(void);
#ifdef H5_HAVE_DIRECT
H5_DLL herr_t H5FD__direct_register(void);
H5_DLL herr_t H5FD__direct_unregister(void);
#endif
H5_DLL herr_t H5FD__family_register(void);
H5_DLL herr_t H5FD__family_unregister(void);
#ifdef H5_HAVE_LIBHDFS
H5_DLL herr_t H5FD__hdfs_register(void);
H5_DLL herr_t H5FD__hdfs_unregister(void);
#endif
#ifdef H5_HAVE_IOC_VFD
H5_DLL herr_t H5FD__ioc_register(void);
H5_DLL herr_t H5FD__ioc_unregister(void);
#endif
H5_DLL herr_t H5FD__log_register(void);
H5_DLL herr_t H5FD__log_unregister(void);
#ifdef H5_HAVE_MIRROR_VFD
H5_DLL herr_t H5FD__mirror_register(void);
H5_DLL herr_t H5FD__mirror_unregister(void);
#endif
#ifdef H5_HAVE_PARALLEL
H5_DLL herr_t H5FD__mpio_register(void);
H5_DLL herr_t H5FD__mpio_unregister(void);
#endif
H5_DLL herr_t H5FD__multi_register(void);
H5_DLL herr_t H5FD__multi_unregister(void);
H5_DLL herr_t H5FD__onion_register(void);
H5_DLL herr_t H5FD__onion_unregister(void);
#ifdef H5_HAVE_ROS3_VFD
H5_DLL herr_t H5FD__ros3_register(void);
H5_DLL herr_t H5FD__ros3_unregister(void);
#endif
H5_DLL herr_t H5FD__sec2_register(void);
H5_DLL herr_t H5FD__sec2_unregister(void);
H5_DLL herr_t H5FD__splitter_register(void);
H5_DLL herr_t H5FD__splitter_unregister(void);
H5_DLL herr_t H5FD__stdio_register(void);
H5_DLL herr_t H5FD__stdio_unregister(void);
#ifdef H5_HAVE_SUBFILING_VFD
H5_DLL herr_t H5FD__subfiling_register(void);
H5_DLL herr_t H5FD__subfiling_unregister(void);
#endif

/* Testing functions */
#ifdef H5FD_TESTING
H5_DLL bool H5FD__supports_swmr_test(const char *vfd_name);
#endif /* H5FD_TESTING */

#endif /* H5FDpkg_H */
