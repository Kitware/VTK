/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Monday, July 26, 1999
 */
#ifndef _H5FDprivate_H
#define _H5FDprivate_H

/* Include package's public header */
#include "H5FDpublic.h"

/* Private headers needed by this file */

/*
 * The MPI drivers are needed because there are
 * places where we check for things that aren't handled by these drivers.
 */
#include "H5FDmpi.h"            /* MPI-based file drivers		*/

/* Macros */

/* Forward declarations for prototype arguments */
struct H5P_genplist_t;
struct H5F_t;

/* Prototypes */
H5_DLL int H5FD_term_interface(void);
H5_DLL H5FD_class_t *H5FD_get_class(hid_t id);
H5_DLL hsize_t H5FD_sb_size(H5FD_t *file);
H5_DLL herr_t H5FD_sb_encode(H5FD_t *file, char *name/*out*/, uint8_t *buf);
H5_DLL herr_t H5FD_sb_decode(H5FD_t *file, const char *name, const uint8_t *buf);
H5_DLL void *H5FD_fapl_get(H5FD_t *file);
H5_DLL herr_t H5FD_fapl_open(struct H5P_genplist_t *plist, hid_t driver_id, const void *driver_info);
H5_DLL herr_t H5FD_fapl_copy(hid_t driver_id, const void *fapl, void **copied_fapl);
H5_DLL herr_t H5FD_fapl_close(hid_t driver_id, void *fapl);
H5_DLL herr_t H5FD_dxpl_open(struct H5P_genplist_t *plist, hid_t driver_id, const void *driver_info);
H5_DLL herr_t H5FD_dxpl_copy(hid_t driver_id, const void *dxpl, void **copied_dxpl);
H5_DLL herr_t H5FD_dxpl_close(hid_t driver_id, void *dxpl);
H5_DLL hid_t H5FD_register(const void *cls, size_t size, hbool_t app_ref);
H5_DLL H5FD_t *H5FD_open(const char *name, unsigned flags, hid_t fapl_id,
		  haddr_t maxaddr);
H5_DLL herr_t H5FD_close(H5FD_t *file);
H5_DLL int H5FD_cmp(const H5FD_t *f1, const H5FD_t *f2);
H5_DLL int H5FD_query(const H5FD_t *f, unsigned long *flags/*out*/);
H5_DLL haddr_t H5FD_alloc(H5FD_t *file, hid_t dxpl_id, H5FD_mem_t type, struct H5F_t *f,
    hsize_t size, haddr_t *align_addr, hsize_t *align_size);
H5_DLL herr_t H5FD_free(H5FD_t *file, hid_t dxpl_id, H5FD_mem_t type, struct H5F_t *f,
    haddr_t addr, hsize_t size);
H5_DLL htri_t H5FD_try_extend(H5FD_t *file, H5FD_mem_t type, struct H5F_t *f,
    haddr_t blk_end, hsize_t extra_requested);
H5_DLL haddr_t H5FD_get_eoa(const H5FD_t *file, H5FD_mem_t type);
H5_DLL herr_t H5FD_set_eoa(H5FD_t *file, H5FD_mem_t type, haddr_t addr);
H5_DLL haddr_t H5FD_get_eof(const H5FD_t *file);
H5_DLL haddr_t H5FD_get_maxaddr(const H5FD_t *file);
H5_DLL herr_t H5FD_get_feature_flags(const H5FD_t *file, unsigned long *feature_flags);
H5_DLL herr_t H5FD_get_fs_type_map(const H5FD_t *file, H5FD_mem_t *type_map);
H5_DLL herr_t H5FD_read(H5FD_t *file, hid_t dxpl_id, H5FD_mem_t type,
    haddr_t addr, size_t size, void *buf/*out*/);
H5_DLL herr_t H5FD_write(H5FD_t *file, hid_t dxpl_id, H5FD_mem_t type,
    haddr_t addr, size_t size, const void *buf);
H5_DLL herr_t H5FD_flush(H5FD_t *file, hid_t dxpl_id, hbool_t closing);
H5_DLL herr_t H5FD_truncate(H5FD_t *file, hid_t dxpl_id, hbool_t closing);
H5_DLL herr_t H5FD_get_fileno(const H5FD_t *file, unsigned long *filenum);
H5_DLL herr_t H5FD_get_vfd_handle(H5FD_t *file, hid_t fapl, void** file_handle);
H5_DLL herr_t H5FD_set_base_addr(H5FD_t *file, haddr_t base_addr);
H5_DLL haddr_t H5FD_get_base_addr(const H5FD_t *file);

#endif /* !_H5FDprivate_H */

