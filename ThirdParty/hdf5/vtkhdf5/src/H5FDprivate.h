/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
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
#include "H5Pprivate.h"		/* Property lists			*/

/*
 * The MPI drivers are needed because there are
 * places where we check for things that aren't handled by these drivers.
 */
#include "H5FDmpi.h"            /* MPI-based file drivers		*/


/**************************/
/* Library Private Macros */
/**************************/

/* Length of filename buffer */
#define H5FD_MAX_FILENAME_LEN      1024

#ifdef H5_HAVE_PARALLEL
/* ======== Temporary data transfer properties ======== */
/* Definitions for memory MPI type property */
#define H5FD_MPI_XFER_MEM_MPI_TYPE_NAME        "H5FD_mpi_mem_mpi_type"
/* Definitions for file MPI type property */
#define H5FD_MPI_XFER_FILE_MPI_TYPE_NAME       "H5FD_mpi_file_mpi_type"

/* Sub-class the H5FD_class_t to add more specific functions for MPI-based VFDs */
typedef struct H5FD_class_mpi_t {
    H5FD_class_t        super;          /* Superclass information & methods */
    int  (*get_rank)(const H5FD_t *file);     /* Get the MPI rank of a process */
    int  (*get_size)(const H5FD_t *file);     /* Get the MPI size of a communicator */
    MPI_Comm (*get_comm)(const H5FD_t *file); /* Get the communicator for a file */
    herr_t (*get_mpi_info)(H5FD_t *file, void** mpi_info); /* get MPI_Info for a file */
} H5FD_class_mpi_t;
#endif

/****************************/
/* Library Private Typedefs */
/****************************/

/* File operations */
typedef enum {
    OP_UNKNOWN = 0,             /* Unknown last file operation */
    OP_READ = 1,                /* Last file I/O operation was a read */
    OP_WRITE = 2                /* Last file I/O operation was a write */
} H5FD_file_op_t;


/* Define structure to hold initial file image and other relevant information */
typedef struct {
    void *buffer;
    size_t size;
    H5FD_file_image_callbacks_t callbacks;
} H5FD_file_image_info_t;

/* Define default file image info */
#define H5FD_DEFAULT_FILE_IMAGE_INFO { \
    /* file image buffer */ NULL,       \
    /* buffer size */       0,          \
    { /* Callbacks */                   \
        /* image_malloc */      NULL,   \
        /* image_memcpy */      NULL,   \
        /* image_realloc */     NULL,   \
        /* image_free */        NULL,   \
        /* udata_copy */        NULL,   \
        /* udata_free */        NULL,   \
        /* udata */             NULL,   \
    }                                   \
}

/* Define structure to hold driver ID & info for FAPLs */
typedef struct {
    hid_t driver_id;            /* Driver's ID */
    const void *driver_info;    /* Driver info, for open callbacks */
} H5FD_driver_prop_t;

#ifdef H5_HAVE_PARALLEL
/* MPIO-specific file access properties */
typedef struct H5FD_mpio_fapl_t {
    MPI_Comm		comm;		/*communicator			*/
    MPI_Info		info;		/*file information		*/
} H5FD_mpio_fapl_t;
#endif /* H5_HAVE_PARALLEL */


/*****************************/
/* Library Private Variables */
/*****************************/


/******************************/
/* Library Private Prototypes */
/******************************/

/* Forward declarations for prototype arguments */
struct H5F_t;

H5_DLL int H5FD_term_interface(void);
H5_DLL herr_t H5FD_locate_signature(H5FD_t *file, haddr_t *sig_addr);
H5_DLL H5FD_class_t *H5FD_get_class(hid_t id);
H5_DLL hsize_t H5FD_sb_size(H5FD_t *file);
H5_DLL herr_t H5FD_sb_encode(H5FD_t *file, char *name/*out*/, uint8_t *buf);
H5_DLL herr_t H5FD_sb_load(H5FD_t *file, const char *name, const uint8_t *buf);
H5_DLL void *H5FD_fapl_get(H5FD_t *file);
H5_DLL herr_t H5FD_fapl_close(hid_t driver_id, const void *fapl);
H5_DLL hid_t H5FD_register(const void *cls, size_t size, hbool_t app_ref);
H5_DLL H5FD_t *H5FD_open(const char *name, unsigned flags, hid_t fapl_id,
		  haddr_t maxaddr);
H5_DLL herr_t H5FD_close(H5FD_t *file);
H5_DLL int H5FD_cmp(const H5FD_t *f1, const H5FD_t *f2);
H5_DLL herr_t H5FD_driver_query(const H5FD_class_t *driver, unsigned long *flags/*out*/);
H5_DLL haddr_t H5FD_alloc(H5FD_t *file, H5FD_mem_t type, 
    struct H5F_t *f, hsize_t size, haddr_t *frag_addr, hsize_t *frag_size);
H5_DLL herr_t H5FD_free(H5FD_t *file, H5FD_mem_t type, struct H5F_t *f,
    haddr_t addr, hsize_t size);
H5_DLL htri_t H5FD_try_extend(H5FD_t *file, H5FD_mem_t type, struct H5F_t *f,
    haddr_t blk_end, hsize_t extra_requested);
H5_DLL haddr_t H5FD_get_eoa(const H5FD_t *file, H5FD_mem_t type);
H5_DLL herr_t H5FD_set_eoa(H5FD_t *file, H5FD_mem_t type, haddr_t addr);
H5_DLL haddr_t H5FD_get_eof(const H5FD_t *file, H5FD_mem_t type);
H5_DLL haddr_t H5FD_get_maxaddr(const H5FD_t *file);
H5_DLL herr_t H5FD_get_feature_flags(const H5FD_t *file, unsigned long *feature_flags);
H5_DLL herr_t H5FD_set_feature_flags(H5FD_t *file, unsigned long feature_flags);
H5_DLL herr_t H5FD_get_fs_type_map(const H5FD_t *file, H5FD_mem_t *type_map);
H5_DLL herr_t H5FD_read(H5FD_t *file, H5FD_mem_t type, haddr_t addr,
    size_t size, void *buf/*out*/);
H5_DLL herr_t H5FD_write(H5FD_t *file, H5FD_mem_t type, haddr_t addr,
    size_t size, const void *buf);
H5_DLL herr_t H5FD_flush(H5FD_t *file, hbool_t closing);
H5_DLL herr_t H5FD_truncate(H5FD_t *file, hbool_t closing);
H5_DLL herr_t H5FD_lock(H5FD_t *file, hbool_t rw);
H5_DLL herr_t H5FD_unlock(H5FD_t *file);
H5_DLL herr_t H5FD_get_fileno(const H5FD_t *file, unsigned long *filenum);
H5_DLL herr_t H5FD_get_vfd_handle(H5FD_t *file, hid_t fapl, void** file_handle);
H5_DLL herr_t H5FD_set_base_addr(H5FD_t *file, haddr_t base_addr);
H5_DLL haddr_t H5FD_get_base_addr(const H5FD_t *file);
H5_DLL herr_t H5FD_set_paged_aggr(H5FD_t *file, hbool_t paged);

/* Function prototypes for MPI based VFDs*/
#ifdef H5_HAVE_PARALLEL
/* General routines */
H5_DLL haddr_t H5FD_mpi_MPIOff_to_haddr(MPI_Offset mpi_off);
H5_DLL herr_t H5FD_mpi_haddr_to_MPIOff(haddr_t addr, MPI_Offset *mpi_off/*out*/);
H5_DLL herr_t H5FD_mpi_comm_info_dup(MPI_Comm comm, MPI_Info info,
				MPI_Comm *comm_new, MPI_Info *info_new);
H5_DLL herr_t H5FD_mpi_comm_info_free(MPI_Comm *comm, MPI_Info *info);
#ifdef NOT_YET
H5_DLL herr_t H5FD_mpio_wait_for_left_neighbor(H5FD_t *file);
H5_DLL herr_t H5FD_mpio_signal_right_neighbor(H5FD_t *file);
#endif /* NOT_YET */
H5_DLL herr_t H5FD_set_mpio_atomicity(H5FD_t *file, hbool_t flag);
H5_DLL herr_t H5FD_get_mpio_atomicity(H5FD_t *file, hbool_t *flag);

/* Driver specific methods */
H5_DLL int H5FD_mpi_get_rank(const H5FD_t *file);
H5_DLL int H5FD_mpi_get_size(const H5FD_t *file);
H5_DLL MPI_Comm H5FD_mpi_get_comm(const H5FD_t *_file);
H5_DLL herr_t H5FD_get_mpi_info(H5FD_t *file, void** file_info);
#endif /* H5_HAVE_PARALLEL */

#endif /* !_H5FDprivate_H */

