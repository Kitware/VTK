/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
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
 *  Header file for API contexts, etc.
 */
#ifndef _H5CXprivate_H
#define _H5CXprivate_H

/* Include package's public header */
#ifdef NOT_YET
#include "H5CXpublic.h"
#endif /* NOT_YET */

/* Private headers needed by this file */
#include "H5private.h"          /* Generic Functions                    */
#include "H5ACprivate.h"        /* Metadata cache                       */
#ifdef H5_HAVE_PARALLEL
#include "H5FDprivate.h"        /* File drivers                         */
#endif /* H5_HAVE_PARALLEL */
#include "H5Zprivate.h"         /* Data filters                         */


/**************************/
/* Library Private Macros */
/**************************/


/****************************/
/* Library Private Typedefs */
/****************************/


/*****************************/
/* Library-private Variables */
/*****************************/


/***************************************/
/* Library-private Function Prototypes */
/***************************************/

/* Library private routines */
#ifndef _H5private_H
H5_DLL herr_t H5CX_push(void);
H5_DLL herr_t H5CX_pop(void);
#endif  /* _H5private_H */
H5_DLL void H5CX_push_special(void);
H5_DLL hbool_t H5CX_is_def_dxpl(void);

/* "Setter" routines for API context info */
H5_DLL void H5CX_set_dxpl(hid_t dxpl_id);
H5_DLL void H5CX_set_lapl(hid_t lapl_id);
H5_DLL herr_t H5CX_set_apl(hid_t *acspl_id, const H5P_libclass_t *libclass,
    hid_t loc_id, hbool_t is_collective);
H5_DLL herr_t H5CX_set_loc(hid_t loc_id);

/* "Getter" routines for API context info */
H5_DLL hid_t H5CX_get_dxpl(void);
H5_DLL hid_t H5CX_get_lapl(void);
H5_DLL haddr_t H5CX_get_tag(void);
H5_DLL H5AC_ring_t H5CX_get_ring(void);
#ifdef H5_HAVE_PARALLEL
H5_DLL hbool_t H5CX_get_coll_metadata_read(void);
H5_DLL herr_t H5CX_get_mpi_coll_datatypes(MPI_Datatype *btype, MPI_Datatype *ftype);
H5_DLL hbool_t H5CX_get_mpi_file_flushing(void);
H5_DLL hbool_t H5CX_get_mpio_rank0_bcast(void);
#endif /* H5_HAVE_PARALLEL */

/* "Getter" routines for DXPL properties cached in API context */
H5_DLL herr_t H5CX_get_btree_split_ratios(double split_ratio[3]);
H5_DLL herr_t H5CX_get_max_temp_buf(size_t *max_temp_buf);
H5_DLL herr_t H5CX_get_tconv_buf(void **tconv_buf);
H5_DLL herr_t H5CX_get_bkgr_buf(void **bkgr_buf);
H5_DLL herr_t H5CX_get_bkgr_buf_type(H5T_bkg_t *bkgr_buf_type);
H5_DLL herr_t H5CX_get_vec_size(size_t *vec_size);
#ifdef H5_HAVE_PARALLEL
H5_DLL herr_t H5CX_get_io_xfer_mode(H5FD_mpio_xfer_t *io_xfer_mode);
H5_DLL herr_t H5CX_get_mpio_coll_opt(H5FD_mpio_collective_opt_t *mpio_coll_opt);
H5_DLL herr_t H5CX_get_mpio_local_no_coll_cause(uint32_t *mpio_local_no_coll_cause);
H5_DLL herr_t H5CX_get_mpio_global_no_coll_cause(uint32_t *mpio_global_no_coll_cause);
H5_DLL herr_t H5CX_get_mpio_chunk_opt_mode(H5FD_mpio_chunk_opt_t *mpio_chunk_opt_mode);
H5_DLL herr_t H5CX_get_mpio_chunk_opt_num(unsigned *mpio_chunk_opt_num);
H5_DLL herr_t H5CX_get_mpio_chunk_opt_ratio(unsigned *mpio_chunk_opt_ratio);
#endif /* H5_HAVE_PARALLEL */
H5_DLL herr_t H5CX_get_err_detect(H5Z_EDC_t *err_detect);
H5_DLL herr_t H5CX_get_filter_cb(H5Z_cb_t *filter_cb);
H5_DLL herr_t H5CX_get_data_transform(H5Z_data_xform_t **data_transform);
H5_DLL herr_t H5CX_get_vlen_alloc_info(H5T_vlen_alloc_info_t *vl_alloc_info);
H5_DLL herr_t H5CX_get_dt_conv_cb(H5T_conv_cb_t *cb_struct);

/* "Getter" routines for LAPL properties cached in API context */
H5_DLL herr_t H5CX_get_nlinks(size_t *nlinks);

/* "Setter" routines for API context info */
H5_DLL void H5CX_set_tag(haddr_t tag);
H5_DLL void H5CX_set_ring(H5AC_ring_t ring);
#ifdef H5_HAVE_PARALLEL
H5_DLL void H5CX_set_coll_metadata_read(hbool_t cmdr);
H5_DLL herr_t H5CX_set_mpi_coll_datatypes(MPI_Datatype btype, MPI_Datatype ftype);
H5_DLL herr_t H5CX_set_mpio_coll_opt(H5FD_mpio_collective_opt_t mpio_coll_opt);
H5_DLL void H5CX_set_mpi_file_flushing(hbool_t flushing);
H5_DLL void H5CX_set_mpio_rank0_bcast(hbool_t rank0_bcast);
#endif /* H5_HAVE_PARALLEL */

/* "Setter" routines for DXPL properties cached in API context */
#ifdef H5_HAVE_PARALLEL
H5_DLL herr_t H5CX_set_io_xfer_mode(H5FD_mpio_xfer_t io_xfer_mode);
#endif /* H5_HAVE_PARALLEL */
H5_DLL herr_t H5CX_set_vlen_alloc_info(H5MM_allocate_t alloc_func,
    void *alloc_info, H5MM_free_t free_func, void *free_info);

/* "Setter" routines for LAPL properties cached in API context */
H5_DLL herr_t H5CX_set_nlinks(size_t nlinks);

/* "Setter" routines for cached DXPL properties that must be returned to application */
#ifdef H5_HAVE_PARALLEL
H5_DLL void H5CX_set_mpio_actual_chunk_opt(H5D_mpio_actual_chunk_opt_mode_t chunk_opt);
H5_DLL void H5CX_set_mpio_actual_io_mode(H5D_mpio_actual_io_mode_t actual_io_mode);
H5_DLL void H5CX_set_mpio_local_no_coll_cause(uint32_t mpio_local_no_coll_cause);
H5_DLL void H5CX_set_mpio_global_no_coll_cause(uint32_t mpio_global_no_coll_cause);
#ifdef H5_HAVE_INSTRUMENTED_LIBRARY
H5_DLL herr_t H5CX_test_set_mpio_coll_chunk_link_hard(int mpio_coll_chunk_link_hard);
H5_DLL herr_t H5CX_test_set_mpio_coll_chunk_multi_hard(int mpio_coll_chunk_multi_hard);
H5_DLL herr_t H5CX_test_set_mpio_coll_chunk_link_num_true(int mpio_coll_chunk_link_num_true);
H5_DLL herr_t H5CX_test_set_mpio_coll_chunk_link_num_false(int mpio_coll_chunk_link_num_false);
H5_DLL herr_t H5CX_test_set_mpio_coll_chunk_multi_ratio_coll(int mpio_coll_chunk_multi_ratio_coll);
H5_DLL herr_t H5CX_test_set_mpio_coll_chunk_multi_ratio_ind(int mpio_coll_chunk_multi_ratio_ind);
H5_DLL herr_t H5CX_test_set_mpio_coll_rank0_bcast(hbool_t rank0_bcast);
#endif /* H5_HAVE_INSTRUMENTED_LIBRARY */
#endif /* H5_HAVE_PARALLEL */

#endif /* _H5CXprivate_H */

