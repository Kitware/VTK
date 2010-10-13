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
 * Programmer:  rky 980813
 * KY 2005 revised the code and made the change to support and optimize
 * collective IO support.
 * Purpose:	Functions to read/write directly between app buffer and file.
 *
 * 		Beware of the ifdef'ed print statements.
 *		I didn't make them portable.
 */

/****************/
/* Module Setup */
/****************/

#define H5D_PACKAGE		/*suppress error about including H5Dpkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Dpkg.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"		/* File access				*/
#include "H5FDprivate.h"	/* File drivers				*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Oprivate.h"		/* Object headers		  	*/
#include "H5Pprivate.h"         /* Property lists                       */
#include "H5Sprivate.h"		/* Dataspaces 				*/
#include "H5Vprivate.h"         /* Vector                               */

#ifdef H5_HAVE_PARALLEL

/****************/
/* Local Macros */
/****************/

/* Macros to represent different IO options */
#define H5D_ONE_LINK_CHUNK_IO          0
#define H5D_MULTI_CHUNK_IO             1
#define H5D_ONE_LINK_CHUNK_IO_MORE_OPT 2
#define H5D_MULTI_CHUNK_IO_MORE_OPT    3

/***** Macros for One linked collective IO case. *****/
/* The default value to do one linked collective IO for all chunks.
   If the average number of chunks per process is greater than this value,
      the library will create an MPI derived datatype to link all chunks to do collective IO.
      The user can set this value through an API. */

/* Macros to represent options on how to obtain chunk address for one linked-chunk IO case */
#define H5D_OBTAIN_ONE_CHUNK_ADDR_IND 0
#define H5D_OBTAIN_ALL_CHUNK_ADDR_COL 2

/* Macros to define the default ratio of obtaining all chunk addresses for one linked-chunk IO case */
#define H5D_ALL_CHUNK_ADDR_THRES_COL  30
#define H5D_ALL_CHUNK_ADDR_THRES_COL_NUM 10000

/***** Macros for multi-chunk collective IO case. *****/
/* The default value of the threshold to do collective IO for this chunk.
   If the average number of processes per chunk is greater than the default value,
   collective IO is done for this chunk.
*/

/* Macros to represent different IO modes(NONE, Independent or collective)for multiple chunk IO case */
#define H5D_CHUNK_IO_MODE_IND         0
#define H5D_CHUNK_IO_MODE_COL         1

/* Macros to represent the regularity of the selection for multiple chunk IO case. */
#define H5D_CHUNK_SELECT_REG          1
#define H5D_CHUNK_SELECT_IRREG        2
#define H5D_CHUNK_SELECT_NONE         0


/******************/
/* Local Typedefs */
/******************/
/* Combine chunk address and chunk info into a struct for better performance. */
typedef struct H5D_chunk_addr_info_t {
  haddr_t chunk_addr;
  H5D_chunk_info_t chunk_info;
} H5D_chunk_addr_info_t;


/********************/
/* Local Prototypes */
/********************/
static herr_t H5D_chunk_collective_io(H5D_io_info_t *io_info,
    const H5D_type_info_t *type_info, H5D_chunk_map_t *fm);
static herr_t H5D_multi_chunk_collective_io(H5D_io_info_t *io_info,
    const H5D_type_info_t *type_info, H5D_chunk_map_t *fm,
    H5P_genplist_t *dx_plist);
static herr_t H5D_multi_chunk_collective_io_no_opt(H5D_io_info_t *io_info,
    const H5D_type_info_t *type_info, H5D_chunk_map_t *fm, H5P_genplist_t *dx_plist);
#ifdef H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS
static herr_t H5D_link_chunk_collective_io(H5D_io_info_t *io_info,
    const H5D_type_info_t *type_info, H5D_chunk_map_t *fm, int sum_chunk);
#endif /* H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS */
static herr_t H5D_inter_collective_io(H5D_io_info_t *io_info,
    const H5D_type_info_t *type_info, const H5S_t *file_space,
    const H5S_t *mem_space);
static herr_t H5D_final_collective_io(H5D_io_info_t *io_info,
    const H5D_type_info_t *type_info, size_t nelmts, MPI_Datatype *mpi_file_type,
    MPI_Datatype *mpi_buf_type);
#ifdef H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS
static herr_t H5D_sort_chunk(H5D_io_info_t *io_info, const H5D_chunk_map_t *fm,
    H5D_chunk_addr_info_t chunk_addr_info_array[], int many_chunk_opt);
#endif /* H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS */
static herr_t H5D_obtain_mpio_mode(H5D_io_info_t *io_info, H5D_chunk_map_t *fm,
    H5P_genplist_t *dx_plist, uint8_t assign_io_mode[], haddr_t chunk_addr[]);
static herr_t H5D_ioinfo_xfer_mode(H5D_io_info_t *io_info, H5P_genplist_t *dx_plist,
    H5FD_mpio_xfer_t xfer_mode);
static herr_t H5D_ioinfo_coll_opt_mode(H5D_io_info_t *io_info, H5P_genplist_t *dx_plist,
    H5FD_mpio_collective_opt_t coll_opt_mode);
static herr_t H5D_mpio_get_min_chunk(const H5D_io_info_t *io_info,
    const H5D_chunk_map_t *fm, int *min_chunkf);
static int H5D_cmp_chunk_addr(const void *addr1, const void *addr2);
static herr_t H5D_mpio_get_sum_chunk(const H5D_io_info_t *io_info,
    const H5D_chunk_map_t *fm, int *sum_chunkf);


/*********************/
/* Package Variables */
/*********************/


/*******************/
/* Local Variables */
/*******************/


/*-------------------------------------------------------------------------
 * Function:	H5D_mpio_opt_possible
 *
 * Purpose:	Checks if an direct I/O transfer is possible between memory and
 *                  the file.
 *
 * Return:	Success:        Non-negative: TRUE or FALSE
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Wednesday, April 3, 2002
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5D_mpio_opt_possible(const H5D_io_info_t *io_info, const H5S_t *file_space,
    const H5S_t *mem_space, const H5D_type_info_t *type_info,
    const H5D_chunk_map_t *fm)
{
    int local_opinion = TRUE;   /* This process's idea of whether to perform collective I/O or not */
    int consensus;              /* Consensus opinion of all processes */
    int mpi_code;               /* MPI error code */
    htri_t ret_value = TRUE;

    FUNC_ENTER_NOAPI(H5D_mpio_opt_possible, FAIL)

    /* Check args */
    HDassert(io_info);
    HDassert(mem_space);
    HDassert(file_space);
    HDassert(type_info);

    /* For independent I/O, get out quickly and don't try to form consensus */
    if(io_info->dxpl_cache->xfer_mode == H5FD_MPIO_INDEPENDENT)
        HGOTO_DONE(FALSE);

    /* Don't allow collective operations if datatype conversions need to happen */
    if(!type_info->is_conv_noop) {
        local_opinion = FALSE;
        goto broadcast;
    } /* end if */

    /* Don't allow collective operations if data transform operations should occur */
    if(!type_info->is_xform_noop) {
        local_opinion = FALSE;
        goto broadcast;
    } /* end if */

    /* Optimized MPI types flag must be set and it must be collective IO */
    /* (Don't allow parallel I/O for the MPI-posix driver, since it doesn't do real collective I/O) */
    if(!(H5S_mpi_opt_types_g && io_info->dxpl_cache->xfer_mode == H5FD_MPIO_COLLECTIVE
            && !IS_H5FD_MPIPOSIX(io_info->dset->oloc.file))) {
        local_opinion = FALSE;
        goto broadcast;
    } /* end if */

    /* Check whether these are both simple or scalar dataspaces */
    if(!((H5S_SIMPLE == H5S_GET_EXTENT_TYPE(mem_space) || H5S_SCALAR == H5S_GET_EXTENT_TYPE(mem_space))
            && (H5S_SIMPLE == H5S_GET_EXTENT_TYPE(file_space) || H5S_SCALAR == H5S_GET_EXTENT_TYPE(file_space)))) {
        local_opinion = FALSE;
        goto broadcast;
    } /* end if */

    /* Can't currently handle point selections */
    if(H5S_SEL_POINTS == H5S_GET_SELECT_TYPE(mem_space)
            || H5S_SEL_POINTS == H5S_GET_SELECT_TYPE(file_space)) {
        local_opinion = FALSE;
        goto broadcast;
    } /* end if */

    /* Dataset storage must be contiguous or chunked */
    if(!(io_info->dset->shared->layout.type == H5D_CONTIGUOUS ||
            io_info->dset->shared->layout.type == H5D_CHUNKED)) {
        local_opinion = FALSE;
        goto broadcast;
    } /* end if */

    /* The handling of memory space is different for chunking and contiguous
     *  storage.  For contiguous storage, mem_space and file_space won't change
     *  when it it is doing disk IO.  For chunking storage, mem_space will
     *  change for different chunks. So for chunking storage, whether we can
     *  use collective IO will defer until each chunk IO is reached.  For
     *  contiguous storage, if we find MPI-IO cannot support complicated MPI
     *  derived data type and the shape of data space is not regular, we will
     *  set use_par_opt_io = FALSE.
     */
#ifndef H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS
    if(io_info->dset->shared->layout.type == H5D_CONTIGUOUS)
        if((H5S_SELECT_IS_REGULAR(file_space) != TRUE) ||
                (H5S_SELECT_IS_REGULAR(mem_space) != TRUE)) {
            local_opinion = FALSE;
            goto broadcast;
        } /* end if */
#endif

    /* Don't allow collective operations if filters need to be applied */
    if(io_info->dset->shared->layout.type == H5D_CHUNKED) {
        if(io_info->dset->shared->dcpl_cache.pline.nused > 0) {
            local_opinion = FALSE;
            goto broadcast;
        } /* end if */

/* If H5_MPI_SPECIAL_COLLECTIVE_IO_WORKS and H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS
 *      are defined, the HDF5 library will do collective IO if the application
 *      asks for it.
 *
 * If H5_MPI_SPECIAL_COLLECTIVE_IO_WORKS is not defined and one or more
 *      processes are not participating in the IO, then collective IO is not
 *      assured. The library will check each process for the number of chunks
 *      it involves. If any process involves zero chunks, the library will use
 *      independent IO mode instead.
 */
#ifndef H5_MPI_SPECIAL_COLLECTIVE_IO_WORKS
        /* Check the number of chunks to perform I/O on */
        if(0 == H5SL_count(fm->sel_chunks)) {
            local_opinion = FALSE;
            goto broadcast;
         } /* end if */
#endif /* H5_MPI_SPECIAL_COLLECTIVE_IO_WORKS */
    } /* end if */

broadcast:
    /* Form consensus opinion among all processes about whether to perform
     * collective I/O
     */
    if(MPI_SUCCESS != (mpi_code = MPI_Allreduce(&local_opinion, &consensus, 1, MPI_INT, MPI_LAND, io_info->comm)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Allreduce failed", mpi_code)

    ret_value = consensus > 0 ? TRUE : FALSE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D_mpio_opt_possible() */


/*-------------------------------------------------------------------------
 * Function:	H5D_mpio_select_read
 *
 * Purpose:	MPI-IO function to read directly from app buffer to file.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Programmer:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_mpio_select_read(const H5D_io_info_t *io_info, const H5D_type_info_t UNUSED *type_info,
    hsize_t mpi_buf_count, const H5S_t UNUSED *file_space, const H5S_t UNUSED *mem_space)
{
    const H5D_contig_storage_t *store_contig = &(io_info->store->contig);    /* Contiguous storage info for this I/O operation */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5D_mpio_select_read, FAIL)

    H5_CHECK_OVERFLOW(mpi_buf_count, hsize_t, size_t);
    if(H5F_block_read(io_info->dset->oloc.file, H5FD_MEM_DRAW, store_contig->dset_addr, (size_t)mpi_buf_count, io_info->dxpl_id, io_info->u.rbuf) < 0)
	HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "can't finish collective parallel read")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_mpio_select_read() */


/*-------------------------------------------------------------------------
 * Function:	H5D_mpio_select_write
 *
 * Purpose:	MPI-IO function to write directly from app buffer to file.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Programmer:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_mpio_select_write(const H5D_io_info_t *io_info, const H5D_type_info_t UNUSED *type_info,
    hsize_t mpi_buf_count, const H5S_t UNUSED *file_space, const H5S_t UNUSED *mem_space)
{
    const H5D_contig_storage_t *store_contig = &(io_info->store->contig);    /* Contiguous storage info for this I/O operation */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5D_mpio_select_write, FAIL)

    /*OKAY: CAST DISCARDS CONST QUALIFIER*/
    H5_CHECK_OVERFLOW(mpi_buf_count, hsize_t, size_t);
    if(H5F_block_write(io_info->dset->oloc.file, H5FD_MEM_DRAW, store_contig->dset_addr, (size_t)mpi_buf_count, io_info->dxpl_id, io_info->u.wbuf) < 0)
       HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "can't finish collective parallel write")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_mpio_select_write() */


/*-------------------------------------------------------------------------
 * Function:	H5D_ioinfo_xfer_mode
 *
 * Purpose:	Switch to between collective & independent MPI I/O
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Friday, August 12, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_ioinfo_xfer_mode(H5D_io_info_t *io_info, H5P_genplist_t *dx_plist,
    H5FD_mpio_xfer_t xfer_mode)
{
    herr_t	ret_value = SUCCEED;	/*return value		*/

    FUNC_ENTER_NOAPI_NOINIT(H5D_ioinfo_xfer_mode)

    /* Change the xfer_mode */
    io_info->dxpl_cache->xfer_mode = xfer_mode;
    if(H5P_set(dx_plist, H5D_XFER_IO_XFER_MODE_NAME, &io_info->dxpl_cache->xfer_mode) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set transfer mode")

    /* Change the "single I/O" function pointers */
    if(xfer_mode == H5FD_MPIO_INDEPENDENT) {
        /* Set the pointers to the original, non-MPI-specific routines */
        io_info->io_ops.single_read = io_info->orig.io_ops.single_read;
        io_info->io_ops.single_write = io_info->orig.io_ops.single_write;
    } /* end if */
    else {
        HDassert(xfer_mode == H5FD_MPIO_COLLECTIVE);

        /* Set the pointers to the MPI-specific routines */
        io_info->io_ops.single_read = H5D_mpio_select_read;
        io_info->io_ops.single_write = H5D_mpio_select_write;
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_ioinfo_xfer_mode() */


/*-------------------------------------------------------------------------
 * Function:	H5D_ioinfo_coll_opt_mode
 *
 * Purpose:	Switch between using collective & independent MPI I/O w/file
 *              set view
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	MuQun Yang
 *		Oct. 5th, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_ioinfo_coll_opt_mode(H5D_io_info_t *io_info, H5P_genplist_t *dx_plist,
    H5FD_mpio_collective_opt_t coll_opt_mode)
{
    herr_t	ret_value = SUCCEED;	/*return value		*/

    FUNC_ENTER_NOAPI_NOINIT(H5D_ioinfo_coll_opt_mode)

    /* Change the optimal xfer_mode */
    io_info->dxpl_cache->coll_opt_mode = coll_opt_mode;
    if(H5P_set(dx_plist, H5D_XFER_MPIO_COLLECTIVE_OPT_NAME, &io_info->dxpl_cache->coll_opt_mode) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set transfer mode")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_ioinfo_coll_opt_mode() */


/*-------------------------------------------------------------------------
 * Function:	H5D_mpio_get_min_chunk
 *
 * Purpose:	Routine for obtaining minimum number of chunks to cover
 *              hyperslab selection selected by all processors.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Muqun Yang
 *              Monday, Feb. 13th, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_mpio_get_min_chunk(const H5D_io_info_t *io_info, const H5D_chunk_map_t *fm,
    int *min_chunkf)
{
    int num_chunkf;             /* Number of chunks to iterate over */
    int mpi_code;               /* MPI return code */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5D_mpio_get_min_chunk)

    /* Get the number of chunks to perform I/O on */
    num_chunkf = H5SL_count(fm->sel_chunks);

    /* Determine the minimum # of chunks for all processes */
    if(MPI_SUCCESS != (mpi_code = MPI_Allreduce(&num_chunkf, min_chunkf, 1, MPI_INT, MPI_MIN, io_info->comm)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Allreduce failed", mpi_code)

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_mpio_get_min_chunk() */


/*-------------------------------------------------------------------------
 * Function:	H5D_mpio_get_sum_chunk
 *
 * Purpose:	Routine for obtaining total number of chunks to cover
 *              hyperslab selection selected by all processors.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Muqun Yang
 *              Monday, Feb. 13th, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_mpio_get_sum_chunk(const H5D_io_info_t *io_info, const H5D_chunk_map_t *fm,
    int *sum_chunkf)
{
    int num_chunkf;             /* Number of chunks to iterate over */
    size_t ori_num_chunkf;
    int mpi_code;               /* MPI return code */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5D_mpio_get_sum_chunk)

    /* Get the number of chunks to perform I/O on */
    num_chunkf = 0;
    ori_num_chunkf = H5SL_count(fm->sel_chunks);
    H5_ASSIGN_OVERFLOW(num_chunkf, ori_num_chunkf, size_t, int);

    /* Determine the summation of number of chunks for all processes */
    if(MPI_SUCCESS != (mpi_code = MPI_Allreduce(&num_chunkf, sum_chunkf, 1, MPI_INT, MPI_SUM, io_info->comm)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Allreduce failed", mpi_code)

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_mpio_get_sum_chunk() */


/*-------------------------------------------------------------------------
 * Function:	H5D_contig_collective_read
 *
 * Purpose:	Reads directly from contiguous data in file into application
 *              memory using collective I/O.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, March  4, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_contig_collective_read(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t UNUSED nelmts, const H5S_t *file_space, const H5S_t *mem_space,
    H5D_chunk_map_t UNUSED *fm)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5D_contig_collective_read, FAIL)

    /* Sanity check */
    HDassert(IS_H5FD_MPIO(io_info->dset->oloc.file));
    HDassert(TRUE == H5P_isa_class(io_info->dxpl_id, H5P_DATASET_XFER));

    /* Call generic internal collective I/O routine */
    if(H5D_inter_collective_io(io_info, type_info, file_space, mem_space) < 0)
	HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "couldn't finish shared collective MPI-IO")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_contig_collective_read() */


/*-------------------------------------------------------------------------
 * Function:	H5D_contig_collective_write
 *
 * Purpose:	Write directly to contiguous data in file from application
 *              memory using collective I/O.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, March  4, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_contig_collective_write(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t UNUSED nelmts, const H5S_t *file_space, const H5S_t *mem_space,
    H5D_chunk_map_t UNUSED *fm)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5D_contig_collective_write, FAIL)

    /* Sanity check */
    HDassert(IS_H5FD_MPIO(io_info->dset->oloc.file));
    HDassert(TRUE == H5P_isa_class(io_info->dxpl_id, H5P_DATASET_XFER));

    /* Call generic internal collective I/O routine */
    if(H5D_inter_collective_io(io_info, type_info, file_space, mem_space) < 0)
	HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "couldn't finish shared collective MPI-IO")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_contig_collective_write() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_collective_io
 *
 * Purpose:	Routine for
 *              1) choose an IO option:
 *                    a) One collective IO defined by one MPI derived datatype to link through all chunks
 *              or    b) multiple chunk IOs,to do MPI-IO for each chunk, the IO mode may be adjusted
 *                       due to the selection pattern for each chunk.
 *              For option a)
 *                      1. Sort the chunk address, obtain chunk info according to the sorted chunk address
 *                      2. Build up MPI derived datatype for each chunk
 *                      3. Build up the final MPI derived datatype
 *                      4. Set up collective IO property list
 *                      5. Do IO
 *             For option b)
 *                      1. Use MPI_gather and MPI_Bcast to obtain information of *collective/independent/none*
 *                         IO mode for each chunk of the selection
 *                      2. Depending on whether the IO mode is collective or independent or none,
 *                         Create either MPI derived datatype for each chunk to do collective IO or
 *                         just do independent IO or independent IO with file set view
 *                      3. Set up collective IO property list for collective mode
 *                      4. DO IO
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Muqun Yang
 *              Monday, Feb. 13th, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_chunk_collective_io(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    H5D_chunk_map_t *fm)
{
    H5P_genplist_t *dx_plist;           /* Pointer to DXPL */
    H5FD_mpio_chunk_opt_t chunk_opt_mode;
    int         io_option = H5D_MULTI_CHUNK_IO_MORE_OPT;
    int         sum_chunk = -1;
#ifdef H5_HAVE_INSTRUMENTED_LIBRARY
    htri_t      temp_not_link_io = FALSE;
#endif
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5D_chunk_collective_io)

    /* Sanity checks */
    HDassert(io_info);
    HDassert(io_info->using_mpi_vfd);
    HDassert(type_info);
    HDassert(fm);

    /* Obtain the data transfer properties */
    if(NULL == (dx_plist = H5I_object(io_info->dxpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list")

    /* Check the optional property list on what to do with collective chunk IO. */
    chunk_opt_mode = (H5FD_mpio_chunk_opt_t)H5P_peek_unsigned(dx_plist, H5D_XFER_MPIO_CHUNK_OPT_HARD_NAME);
    if(chunk_opt_mode == H5FD_MPIO_CHUNK_ONE_IO)
        io_option = H5D_ONE_LINK_CHUNK_IO;      /*no opt*/
    else if(chunk_opt_mode == H5FD_MPIO_CHUNK_MULTI_IO)
        io_option = H5D_MULTI_CHUNK_IO;         /*no opt */
    else {
        unsigned one_link_chunk_io_threshold;   /* Threshhold to use single collective I/O for all chunks */
        int mpi_size;                   /* Number of processes in MPI job */

        if(H5D_mpio_get_sum_chunk(io_info, fm, &sum_chunk) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSWAP, FAIL, "unable to obtain the total chunk number of all processes");
        if((mpi_size = H5F_mpi_get_size(io_info->dset->oloc.file)) < 0)
            HGOTO_ERROR(H5E_IO, H5E_MPI, FAIL, "unable to obtain mpi size")

        one_link_chunk_io_threshold = H5P_peek_unsigned(dx_plist, H5D_XFER_MPIO_CHUNK_OPT_NUM_NAME);

        /* step 1: choose an IO option */
        /* If the average number of chunk per process is greater than a threshold, we will do one link chunked IO. */
        if((unsigned)sum_chunk / mpi_size >= one_link_chunk_io_threshold)
            io_option = H5D_ONE_LINK_CHUNK_IO_MORE_OPT;
#ifdef H5_HAVE_INSTRUMENTED_LIBRARY
        else
            temp_not_link_io = TRUE;
#endif
   } /* end else */

#ifndef H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS
    if(io_option == H5D_ONE_LINK_CHUNK_IO) {
        io_option = H5D_MULTI_CHUNK_IO; /* We can not do this with one chunk IO. */
#ifdef  H5_HAVE_INSTRUMENTED_LIBRARY
    {  int new_value;
       htri_t check_prop;
       check_prop = H5Pexist(io_info->dxpl_id, H5D_XFER_COLL_CHUNK_LINK_TO_MULTI);
       if(check_prop > 0) {
           new_value = 1;
           if(H5Pset(io_info->dxpl_id, H5D_XFER_COLL_CHUNK_LINK_TO_MULTI, &new_value) < 0)
                HGOTO_ERROR(H5E_IO, H5E_CANTSET, FAIL, "unable to set property value")

       }
     }/* add this property because the library changes the option from one link to multiple chunks.*/
#endif
    }
    if(io_option == H5D_ONE_LINK_CHUNK_IO_MORE_OPT){
        io_option = H5D_MULTI_CHUNK_IO_MORE_OPT;
#ifdef  H5_HAVE_INSTRUMENTED_LIBRARY
      { int        new_value;
       htri_t check_prop;
       check_prop = H5Pexist(io_info->dxpl_id, H5D_XFER_COLL_CHUNK_LINK_TO_MULTI_OPT);
       if(check_prop > 0) {
           new_value = 1;
           if(H5Pset(io_info->dxpl_id, H5D_XFER_COLL_CHUNK_LINK_TO_MULTI_OPT, &new_value) < 0)
                HGOTO_ERROR(H5E_IO, H5E_CANTSET, FAIL, "unable to set property value")

       }
      }/* add this property because the library changes the option from one link to multiple chunks.*/
#endif
    }
#endif

#ifdef H5_HAVE_INSTRUMENTED_LIBRARY
{
    htri_t            check_prop;
    int               new_value;

    /*** Test collective chunk user-input optimization APIs. ***/
    check_prop = H5Pexist(io_info->dxpl_id, H5D_XFER_COLL_CHUNK_LINK_HARD_NAME);
    if(check_prop > 0) {
        if(io_option == H5D_ONE_LINK_CHUNK_IO) {
            new_value = 0;
            if(H5Pset(io_info->dxpl_id, H5D_XFER_COLL_CHUNK_LINK_HARD_NAME, &new_value) < 0)
                HGOTO_ERROR(H5E_IO, H5E_CANTSET, FAIL, "unable to set property value")
        } /* end if */
    } /* end if */
    check_prop = H5Pexist(io_info->dxpl_id, H5D_XFER_COLL_CHUNK_MULTI_HARD_NAME);
    if(check_prop > 0) {
        if(io_option == H5D_MULTI_CHUNK_IO) {
            new_value = 0;
            if(H5Pset(io_info->dxpl_id, H5D_XFER_COLL_CHUNK_MULTI_HARD_NAME, &new_value) < 0)
                HGOTO_ERROR(H5E_IO, H5E_CANTSET, FAIL, "unable to set property value")
        } /* end if */
    } /* end if */
    check_prop = H5Pexist(io_info->dxpl_id, H5D_XFER_COLL_CHUNK_LINK_NUM_TRUE_NAME);
    if(check_prop > 0) {
        if(io_option == H5D_ONE_LINK_CHUNK_IO_MORE_OPT) {
            new_value = 0;
            if(H5Pset(io_info->dxpl_id, H5D_XFER_COLL_CHUNK_LINK_NUM_TRUE_NAME, &new_value) < 0)
                HGOTO_ERROR(H5E_IO, H5E_CANTSET, FAIL, "unable to set property value")
        } /* end if */
    } /* end if */
    check_prop = H5Pexist(io_info->dxpl_id, H5D_XFER_COLL_CHUNK_LINK_NUM_FALSE_NAME);
    if(check_prop > 0) {
        if(temp_not_link_io) {
            new_value = 0;
            if(H5Pset(io_info->dxpl_id, H5D_XFER_COLL_CHUNK_LINK_NUM_FALSE_NAME, &new_value) < 0)
                HGOTO_ERROR(H5E_IO, H5E_CANTSET, FAIL, "unable to set property value")
        } /* end if */
    } /* end if */
}
#endif

    /* step 2:  Go ahead to do IO.*/
#ifdef H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS
    if(io_option == H5D_ONE_LINK_CHUNK_IO || io_option == H5D_ONE_LINK_CHUNK_IO_MORE_OPT) {
        if(H5D_link_chunk_collective_io(io_info, type_info, fm, sum_chunk) < 0)
            HGOTO_ERROR(H5E_IO, H5E_CANTGET, FAIL, "couldn't finish linked chunk MPI-IO")
    } /* end if */
    else
#endif /* H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS */
    if(io_option == H5D_MULTI_CHUNK_IO) {
        if(H5D_multi_chunk_collective_io_no_opt(io_info, type_info, fm, dx_plist) < 0)
            HGOTO_ERROR(H5E_IO, H5E_CANTGET, FAIL, "couldn't finish multiple chunk MPI-IO")
    } /* end if */
    else { /*multiple chunk IOs with opt */
        if(H5D_multi_chunk_collective_io(io_info, type_info, fm, dx_plist) < 0)
            HGOTO_ERROR(H5E_IO, H5E_CANTGET, FAIL, "couldn't finish optimized multiple chunk MPI-IO")
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_collective_io */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_collective_read
 *
 * Purpose:	Reads directly from chunks in file into application memory
 *              using collective I/O.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, March  4, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_collective_read(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t UNUSED nelmts, const H5S_t UNUSED *file_space, const H5S_t UNUSED *mem_space,
    H5D_chunk_map_t *fm)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5D_chunk_collective_read, FAIL)

    /* Call generic selection operation */
    if(H5D_chunk_collective_io(io_info, type_info, fm) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_READERROR, FAIL, "read error")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_collective_read() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_collective_write
 *
 * Purpose:	Write directly to chunks in file from application memory
 *              using collective I/O.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, March  4, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_collective_write(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t UNUSED nelmts, const H5S_t UNUSED *file_space, const H5S_t UNUSED *mem_space,
    H5D_chunk_map_t *fm)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5D_chunk_collective_write, FAIL)

    /* Call generic selection operation */
    if(H5D_chunk_collective_io(io_info, type_info, fm) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_WRITEERROR, FAIL, "write error")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_collective_write() */

#ifdef H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS

/*-------------------------------------------------------------------------
 * Function:	H5D_link_chunk_collective_io
 *
 * Purpose:	Routine for one collective IO with one MPI derived datatype to link with all chunks
 *
 *                      1. Sort the chunk address and chunk info
 *                      2. Build up MPI derived datatype for each chunk
 *                      3. Build up the final MPI derived datatype
 *                      4. Use common collective IO routine to do MPI-IO
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Muqun Yang
 *              Monday, Feb. 13th, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_link_chunk_collective_io(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    H5D_chunk_map_t *fm, int sum_chunk)
{
    H5D_chunk_addr_info_t *chunk_addr_info_array = NULL;
    hbool_t mbt_is_derived = FALSE;
    hbool_t mft_is_derived = FALSE;
    MPI_Datatype chunk_final_mtype;         /* Final memory MPI datatype for all chunks with seletion */
    MPI_Datatype chunk_final_ftype;         /* Final file MPI datatype for all chunks with seletion */
    H5D_storage_t ctg_store;                /* Storage info for "fake" contiguous dataset */
    size_t              total_chunks;
    haddr_t            *total_chunk_addr_array = NULL;
    MPI_Datatype       *chunk_mtype = NULL;
    MPI_Datatype       *chunk_ftype = NULL;
    MPI_Aint           *chunk_disp_array = NULL;
    MPI_Aint           *chunk_mem_disp_array = NULL;
    int                *blocklen = NULL;
    int                 mpi_code;           /* MPI return code */
    herr_t              ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5D_link_chunk_collective_io)

    /* Get the sum # of chunks, if not already available */
    if(sum_chunk < 0) {
        if(H5D_mpio_get_sum_chunk(io_info, fm, &sum_chunk) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSWAP, FAIL, "unable to obtain the total chunk number of all processes");
    } /* end if */

    /* Retrieve total # of chunks in dataset */
    H5_ASSIGN_OVERFLOW(total_chunks, fm->layout->u.chunk.nchunks, hsize_t, size_t);

    /* Handle special case when dataspace dimensions only allow one chunk in
     *  the dataset.  [This sometimes is used by developers who want the
     *  equivalent of compressed contiguous datasets - QAK]
     */
    if(total_chunks == 1) {
        H5D_chunk_ud_t udata;           /* User data for querying chunk info */
        hsize_t coords[H5O_LAYOUT_NDIMS];   /* Coordinates of chunk in file dataset's dataspace */
        H5SL_node_t *chunk_node;        /* Pointer to chunk node for selection */
        H5S_t *fspace;                  /* Dataspace describing chunk & selection in it */
        H5S_t *mspace;                  /* Dataspace describing selection in memory corresponding to this chunk */

        /* Initialize the chunk coordinates */
        /* (must be all zero, since there's only one chunk) */
        HDmemset(coords, 0, sizeof(coords));

        /* Look up address of chunk */
        if(H5D_chunk_get_info(io_info->dset, io_info->dxpl_id, coords, &udata) < 0)
            HGOTO_ERROR(H5E_STORAGE, H5E_CANTGET, FAIL, "couldn't get chunk info from skipped list")
        ctg_store.contig.dset_addr = udata.addr;

        /* Check for this process having selection in this chunk */
        chunk_node = H5SL_first(fm->sel_chunks);
	if(chunk_node == NULL) {
            /* Set the dataspace info for I/O to NULL, this process doesn't have any I/O to perform */
            fspace = mspace = NULL;
	} /* end if */
	else {
            H5D_chunk_info_t *chunk_info;

            /* Get the chunk info, for the selection in the chunk */
            if(NULL == (chunk_info = H5SL_item(chunk_node)))
                HGOTO_ERROR(H5E_STORAGE, H5E_CANTGET, FAIL, "couldn't get chunk info from skipped list")

            /* Set the dataspace info for I/O */
            fspace = chunk_info->fspace;
            mspace = chunk_info->mspace;
	} /* end else */

        /* Set up the base storage address for this chunk */
        io_info->store = &ctg_store;

#ifdef H5D_DEBUG
if(H5DEBUG(D))
    HDfprintf(H5DEBUG(D),"before inter_collective_io for total chunk = 1 \n");
#endif

        /* Perform I/O */
        if(H5D_inter_collective_io(io_info, type_info, fspace, mspace) < 0)
            HGOTO_ERROR(H5E_STORAGE, H5E_CANTGET, FAIL, "couldn't finish shared collective MPI-IO")
    } /* end if */
    else {
        size_t mpi_buf_count;   /* Number of MPI types */
        size_t num_chunk;       /* Number of chunks for this process */
        size_t u;               /* Local index variable */

        /* Get the number of chunks with a selection */
        num_chunk = H5SL_count(fm->sel_chunks);
        H5_CHECK_OVERFLOW(num_chunk, size_t, int);

#ifdef H5D_DEBUG
if(H5DEBUG(D))
    HDfprintf(H5DEBUG(D),"total_chunks = %Zu, num_chunk = %Zu\n", total_chunks, num_chunk);
#endif

        /* Set up MPI datatype for chunks selected */
        if(num_chunk) {
            hsize_t mpi_mem_extra_offset;       /* Extra offset for memory MPI datatype */
            hsize_t mpi_file_extra_offset;      /* Extra offset for file MPI datatype */
            size_t mpi_mem_count;               /* Memory MPI datatype count */
            size_t mpi_file_count;              /* File MPI datatype count */
            hbool_t locl_mbt_is_derived = FALSE,     /* Whether the buffer (memory) type is derived and needs to be free'd */
                    local_mft_is_derived = FALSE;     /* Whether the file type is derived and needs to be free'd */
            int blocklen_value;                 /* Placeholder for array fill */

            /* Allocate chunking information */
            chunk_addr_info_array= H5MM_malloc(num_chunk * sizeof(H5D_chunk_addr_info_t));
            chunk_mtype          = H5MM_malloc(num_chunk * sizeof(MPI_Datatype));
            chunk_ftype          = H5MM_malloc(num_chunk * sizeof(MPI_Datatype));
            chunk_disp_array     = H5MM_malloc(num_chunk * sizeof(MPI_Aint));
            chunk_mem_disp_array = H5MM_calloc(num_chunk * sizeof(MPI_Aint));
            blocklen             = H5MM_malloc(num_chunk * sizeof(int));

#ifdef H5D_DEBUG
if(H5DEBUG(D))
    HDfprintf(H5DEBUG(D),"before sorting the chunk address \n");
#endif
            /* Sort the chunk address */
            if(H5D_sort_chunk(io_info, fm, chunk_addr_info_array, sum_chunk) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSWAP, FAIL, "unable to sort chunk address")
            ctg_store.contig.dset_addr = chunk_addr_info_array[0].chunk_addr;
#ifdef H5D_DEBUG
if(H5DEBUG(D))
    HDfprintf(H5DEBUG(D),"after sorting the chunk address \n");
#endif

            /* Obtain MPI derived datatype from all individual chunks */
            for(u = 0; u < num_chunk; u++) {
                /* Disk MPI derived datatype */
                if(H5S_mpio_space_type(chunk_addr_info_array[u].chunk_info.fspace,
                        type_info->src_type_size, &chunk_ftype[u], &mpi_file_count,
                        &mpi_file_extra_offset, &local_mft_is_derived) < 0)
                    HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL, "couldn't create MPI file type")

                /* Buffer MPI derived datatype */
                if(H5S_mpio_space_type(chunk_addr_info_array[u].chunk_info.mspace,
                        type_info->dst_type_size, &chunk_mtype[u], &mpi_mem_count,
                        &mpi_mem_extra_offset, &locl_mbt_is_derived) < 0)
                    HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL, "couldn't create MPI buf type")

                /* Chunk address relative to the first chunk */
                chunk_addr_info_array[u].chunk_addr -= ctg_store.contig.dset_addr;

                /* Assign chunk address to MPI displacement */
                /* (assume MPI_Aint big enough to hold it) */
                chunk_disp_array[u] = (MPI_Aint)chunk_addr_info_array[u].chunk_addr;
            } /* end for */

            /* Initialize the buffer with the constant value 1 */
            blocklen_value = 1;
            H5V_array_fill(blocklen, &blocklen_value, sizeof(int), num_chunk);

            /* Create final MPI derived datatype for the file */
            if(MPI_SUCCESS != (mpi_code = MPI_Type_struct((int)num_chunk, blocklen, chunk_disp_array, chunk_ftype, &chunk_final_ftype)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Type_struct failed", mpi_code)
            if(MPI_SUCCESS != (mpi_code = MPI_Type_commit(&chunk_final_ftype)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code)

            /* Create final MPI derived datatype for memory */
            if(MPI_SUCCESS != (mpi_code = MPI_Type_struct(num_chunk, blocklen, chunk_mem_disp_array, chunk_mtype, &chunk_final_mtype)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Type_struct failed", mpi_code)
            if(MPI_SUCCESS != (mpi_code = MPI_Type_commit(&chunk_final_mtype)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code)

            /* Free the file & memory MPI datatypes for each chunk */
            for(u = 0; u < num_chunk; u++) {
                if(MPI_SUCCESS != (mpi_code = MPI_Type_free(chunk_mtype + u)))
                    HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)

                if(MPI_SUCCESS != (mpi_code = MPI_Type_free(chunk_ftype + u)))
                    HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)
            } /* end for */

            /* buffer, file derived datatypes should be true */
            mbt_is_derived = TRUE;
            mft_is_derived = TRUE;
            mpi_buf_count  = (size_t)1;
        } /* end if */
        else {      /* no selection at all for this process */
            /* Allocate chunking information */
            total_chunk_addr_array = H5MM_malloc(sizeof(haddr_t) * total_chunks);

            /* Retrieve chunk address map */
            if(H5D_chunk_addrmap(io_info, total_chunk_addr_array) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get chunk address")

            /* Get chunk with lowest address */
            ctg_store.contig.dset_addr = HADDR_MAX;
            for(u = 0; u < total_chunks; u++)
                if(total_chunk_addr_array[u] < ctg_store.contig.dset_addr)
                    ctg_store.contig.dset_addr = total_chunk_addr_array[u];
            HDassert(ctg_store.contig.dset_addr != HADDR_MAX);

            /* Set the MPI datatype */
            chunk_final_ftype = MPI_BYTE;
            chunk_final_mtype = MPI_BYTE;

            /* buffer, file derived datatypes should be true */
            mpi_buf_count  = (size_t)0;
        } /* end else */
#ifdef H5D_DEBUG
if(H5DEBUG(D))
    HDfprintf(H5DEBUG(D),"before coming to final collective IO\n");
#endif

        /* Set up the base storage address for this chunk */
        io_info->store = &ctg_store;

        /* Perform final collective I/O operation */
        if(H5D_final_collective_io(io_info, type_info, mpi_buf_count, &chunk_final_ftype, &chunk_final_mtype) < 0)
            HGOTO_ERROR(H5E_IO, H5E_CANTGET, FAIL, "couldn't finish MPI-IO")
    } /* end else */

done:
#ifdef H5D_DEBUG
if(H5DEBUG(D))
    HDfprintf(H5DEBUG(D),"before freeing memory inside H5D_link_collective_io ret_value = %d\n", ret_value);
#endif
    if(total_chunk_addr_array)
        H5MM_xfree(total_chunk_addr_array);
    if(chunk_addr_info_array)
        H5MM_xfree(chunk_addr_info_array);
    if(chunk_mtype)
        H5MM_xfree(chunk_mtype);
    if(chunk_ftype)
        H5MM_xfree(chunk_ftype);
    if(chunk_disp_array)
        H5MM_xfree(chunk_disp_array);
    if(chunk_mem_disp_array)
        H5MM_xfree(chunk_mem_disp_array);
    if(blocklen)
        H5MM_xfree(blocklen);

    /* Free the MPI buf and file types, if they were derived */
    if(mbt_is_derived && MPI_SUCCESS != (mpi_code = MPI_Type_free(&chunk_final_mtype)))
        HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)
    if(mft_is_derived && MPI_SUCCESS != (mpi_code = MPI_Type_free(&chunk_final_ftype)))
        HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_link_chunk_collective_io */
#endif /* H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS */


/*-------------------------------------------------------------------------
 * Function:	H5D_multi_chunk_collective_io
 *
 * Purpose:	To do IO per chunk according to IO mode(collective/independent/none)
 *
 *              1. Use MPI_gather and MPI_Bcast to obtain IO mode in each chunk(collective/independent/none)
 *              2. Depending on whether the IO mode is collective or independent or none,
 *                 Create either MPI derived datatype for each chunk or just do independent IO
 *              3. Use common collective IO routine to do MPI-IO
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Muqun Yang
 *              Monday, Feb. 13th, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_multi_chunk_collective_io(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    H5D_chunk_map_t *fm, H5P_genplist_t *dx_plist)
{
    H5D_io_info_t       ctg_io_info;          /* Contiguous I/O info object */
    H5D_storage_t       ctg_store;            /* Chunk storage information as contiguous dataset */
    H5D_io_info_t       cpt_io_info;          /* Compact I/O info object */
    H5D_storage_t       cpt_store;            /* Chunk storage information as compact dataset */
    hbool_t             cpt_dirty;            /* Temporary placeholder for compact storage "dirty" flag */
    uint8_t            *chunk_io_option = NULL;
    haddr_t            *chunk_addr = NULL;
    H5D_storage_t       store;                /* union of EFL and chunk pointer in file space */
    H5FD_mpio_xfer_t    last_xfer_mode = H5FD_MPIO_COLLECTIVE; /* Last parallel transfer for this request (H5D_XFER_IO_XFER_MODE_NAME) */
    H5FD_mpio_collective_opt_t last_coll_opt_mode = H5FD_MPIO_COLLECTIVE_IO; /* Last parallel transfer with independent IO or collective IO with this mode */
    size_t              total_chunk;          /* Total # of chunks in dataset */
#ifdef H5Dmpio_DEBUG
    int mpi_rank;
#endif
    size_t              u;                    /* Local index variable */
    herr_t              ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5D_multi_chunk_collective_io)

#ifdef H5Dmpio_DEBUG
    mpi_rank = H5F_mpi_get_rank(io_info->dset->oloc.file);
#endif

    /* Retrieve total # of chunks in dataset */
    H5_ASSIGN_OVERFLOW(total_chunk, fm->layout->u.chunk.nchunks, hsize_t, size_t);
    HDassert(total_chunk != 0);

    /* Allocate memories */
    chunk_io_option = (uint8_t *)H5MM_calloc(total_chunk);
    chunk_addr      = (haddr_t *)H5MM_calloc(total_chunk * sizeof(haddr_t));
#ifdef H5D_DEBUG
if(H5DEBUG(D))
    HDfprintf(H5DEBUG(D), "total_chunk %Zu\n", total_chunk);
#endif

    /* Obtain IO option for each chunk */
    if(H5D_obtain_mpio_mode(io_info, fm, dx_plist, chunk_io_option, chunk_addr) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTRECV, FAIL, "unable to obtain MPIO mode")

    /* Set up contiguous I/O info object */
    HDmemcpy(&ctg_io_info, io_info, sizeof(ctg_io_info));
    ctg_io_info.store = &ctg_store;
    ctg_io_info.layout_ops = *H5D_LOPS_CONTIG;

    /* Initialize temporary contiguous storage info */
    ctg_store.contig.dset_size = (hsize_t)io_info->dset->shared->layout.u.chunk.size;

    /* Set up compact I/O info object */
    HDmemcpy(&cpt_io_info, io_info, sizeof(cpt_io_info));
    cpt_io_info.store = &cpt_store;
    cpt_io_info.layout_ops = *H5D_LOPS_COMPACT;

    /* Initialize temporary compact storage info */
    cpt_store.compact.dirty = &cpt_dirty;

    /* Set dataset storage for I/O info */
    io_info->store = &store;

    /* Loop over _all_ the chunks */
    for(u = 0; u < total_chunk; u++) {
        H5D_chunk_info_t *chunk_info;    /* Chunk info for current chunk */
        H5S_t *fspace;              /* Dataspace describing chunk & selection in it */
        H5S_t *mspace;              /* Dataspace describing selection in memory corresponding to this chunk */

#ifdef H5D_DEBUG
if(H5DEBUG(D))
    HDfprintf(H5DEBUG(D),"mpi_rank = %d, chunk index = %Zu\n", mpi_rank, u);
#endif
        /* Get the chunk info for this chunk, if there are elements selected */
        chunk_info = fm->select_chunk[u];

        /* Set the storage information for chunks with selections */
        if(chunk_info) {
            HDassert(chunk_info->index == u);

            /* Pass in chunk's coordinates in a union. */
            store.chunk.offset  = chunk_info->coords;
            store.chunk.index   = chunk_info->index;
	} /* end if */

        /* Collective IO for this chunk,
         * Note: even there is no selection for this process, the process still
         *      needs to contribute MPI NONE TYPE.
         */
        if(chunk_io_option[u] == H5D_CHUNK_IO_MODE_COL) {
#ifdef H5D_DEBUG
if(H5DEBUG(D))
    HDfprintf(H5DEBUG(D),"inside collective chunk IO mpi_rank = %d, chunk index = %Zu\n", mpi_rank, u);
#endif

            /* Set the file & memory dataspaces */
            if(chunk_info) {
                fspace = chunk_info->fspace;
                mspace = chunk_info->mspace;
	    } /* end if */
	    else {
                fspace = mspace = NULL;
	    } /* end else */

            /* Switch back to collective I/O */
            if(last_xfer_mode != H5FD_MPIO_COLLECTIVE) {
                if(H5D_ioinfo_xfer_mode(io_info, dx_plist, H5FD_MPIO_COLLECTIVE) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't switch to collective I/O")
                last_xfer_mode = H5FD_MPIO_COLLECTIVE;
            } /* end if */
            if(last_coll_opt_mode != H5FD_MPIO_COLLECTIVE_IO) {
                if(H5D_ioinfo_coll_opt_mode(io_info, dx_plist, H5FD_MPIO_COLLECTIVE_IO) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't switch to collective I/O")
                last_coll_opt_mode = H5FD_MPIO_COLLECTIVE_IO;
            } /* end if */

            /* Initialize temporary contiguous storage address */
            ctg_store.contig.dset_addr = chunk_addr[u];

            /* Perform the I/O */
            if(H5D_inter_collective_io(&ctg_io_info, type_info, fspace, mspace) < 0)
                HGOTO_ERROR(H5E_IO, H5E_CANTGET, FAIL, "couldn't finish shared collective MPI-IO")
	} /* end if */
	else {  /* possible independent IO for this chunk */
#ifdef H5D_DEBUG
if(H5DEBUG(D))
    HDfprintf(H5DEBUG(D),"inside independent IO mpi_rank = %d, chunk index = %Zu\n", mpi_rank, u);
#endif

            HDassert(chunk_io_option[u] == 0);

#if !defined(H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS) || !defined(H5_MPI_SPECIAL_COLLECTIVE_IO_WORKS)
            /* Check if this process has something to do with this chunk */
            if(chunk_info) {
                H5D_io_info_t *chk_io_info;     /* Pointer to I/O info object for this chunk */
                H5D_chunk_ud_t udata;         /* B-tree pass-through	*/
                void *chunk;                    /* Pointer to the data chunk in cache */
                uint32_t accessed_bytes;          /* Total accessed size in a chunk */
                unsigned idx_hint = 0;          /* Cache index hint      */
                htri_t cacheable;               /* Whether the chunk is cacheable */

                /* Switch to independent I/O */
                if(last_xfer_mode != H5FD_MPIO_INDEPENDENT) {
                    if(H5D_ioinfo_xfer_mode(io_info, dx_plist, H5FD_MPIO_INDEPENDENT) < 0)
                        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't switch to independent I/O")
                    last_xfer_mode = H5FD_MPIO_INDEPENDENT;
                } /* end if */

                /* Load the chunk into cache.  But if the whole chunk is written,
                 * simply allocate space instead of load the chunk.
                 */
                if(H5D_chunk_get_info(io_info->dset, io_info->dxpl_id, chunk_info->coords, &udata) < 0)
                    HGOTO_ERROR(H5E_STORAGE, H5E_CANTGET, FAIL, "couldn't get chunk info from skipped list")

                /* Load the chunk into cache and lock it. */
                if((cacheable = H5D_chunk_cacheable(io_info, udata.addr,
                        io_info->op_type == H5D_IO_OP_WRITE)) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't tell if chunk is cacheable")
                if(cacheable) {
                    hbool_t entire_chunk = TRUE;         /* Whether whole chunk is selected */

                    /* Compute # of bytes accessed in chunk */
                    accessed_bytes = chunk_info->chunk_points * type_info->src_type_size;

                    /* Determine if we will access all the data in the chunk */
                    if(((io_info->op_type == H5D_IO_OP_WRITE) && (accessed_bytes != ctg_store.contig.dset_size))
                            || (io_info->op_type != H5D_IO_OP_WRITE))
                        entire_chunk = FALSE;

                    /* Lock the chunk into the cache */
                    if(NULL == (chunk = H5D_chunk_lock(io_info, &udata, entire_chunk, &idx_hint)))
                        HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "unable to read raw data chunk")

                    /* Set up the storage buffer information for this chunk */
                    cpt_store.compact.buf = chunk;

                    /* Point I/O info at contiguous I/O info for this chunk */
                    chk_io_info = &cpt_io_info;
                } /* end if */
                else {
                    /* Set up the storage address information for this chunk */
                    ctg_store.contig.dset_addr = udata.addr;

                    /* No chunk cached */
                    chunk = NULL;

                    /* Point I/O info at temporary I/O info for this chunk */
                    chk_io_info = &ctg_io_info;
                } /* end else */

                if(io_info->op_type == H5D_IO_OP_WRITE) {
                    if((io_info->io_ops.single_write)(chk_io_info, type_info,
                            (hsize_t)chunk_info->chunk_points, chunk_info->fspace, chunk_info->mspace) < 0)
                        HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "optimized write failed")
                } /* end if */
                else {
                    if((io_info->io_ops.single_read)(chk_io_info, type_info,
                            (hsize_t)chunk_info->chunk_points, chunk_info->fspace, chunk_info->mspace) < 0)
                        HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "optimized read failed")
                } /* end else */

                /* Release the cache lock on the chunk. */
                if(chunk && H5D_chunk_unlock(io_info, &udata, (io_info->op_type == H5D_IO_OP_WRITE), idx_hint, chunk, accessed_bytes) < 0)
                    HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "unable to unlock raw data chunk")
            } /* end if */
#else /* !defined(H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS) || !defined(H5_MPI_SPECIAL_COLLECTIVE_IO_WORKS) */
            /* Set the file & memory dataspaces */
            if(chunk_info) {
                fspace = chunk_info->fspace;
                mspace = chunk_info->mspace;
            } /* end if */
            else {
                fspace = mspace = NULL;
            } /* end else */

            /* Using independent I/O with file setview.*/
            if(last_coll_opt_mode != H5FD_MPIO_INDIVIDUAL_IO) {
                if(H5D_ioinfo_coll_opt_mode(io_info, dx_plist, H5FD_MPIO_INDIVIDUAL_IO) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't switch to individual I/O")
                last_coll_opt_mode = H5FD_MPIO_INDIVIDUAL_IO;
            } /* end if */

            /* Initialize temporary contiguous storage address */
            ctg_store.contig.dset_addr = chunk_addr[u];

            /* Perform the I/O */
            if(H5D_inter_collective_io(&ctg_io_info, type_info, fspace, mspace) < 0)
                HGOTO_ERROR(H5E_IO, H5E_CANTGET, FAIL, "couldn't finish shared collective MPI-IO")
#ifdef H5D_DEBUG
  if(H5DEBUG(D))
    HDfprintf(H5DEBUG(D),"after inter collective IO\n");
#endif
#endif /* !defined(H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS) || !defined(H5_MPI_SPECIAL_COLLECTIVE_IO_WORKS) */
        } /* end else */
    } /* end for */

done:
    if(chunk_io_option)
        H5MM_xfree(chunk_io_option);
    if(chunk_addr)
        H5MM_xfree(chunk_addr);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_multi_chunk_collective_io */


/*-------------------------------------------------------------------------
 * Function:	H5D_multi_chunk_collective_io_no_opt
 *
 * Purpose:	To do collective IO without any optimization per chunk base
 *              The internal independent IO inside HDF5 cannot handle
 *              non-contiguous(or with holes) storage efficiently.
 *              Under this case, the one independent IO call may consist of
 *              many small disk IOs. So we may use independent IO with derived datatype
 *              to replace the independent IO when we find this chunk is not good to
 *              do collective IO. However, according to our performance study,
 *              this approach may not overcome the overhead caused by MPI gather/scatter.
 *              So we decide to leave the original collective IO per chunk approach as
 *              an option for users. NO MPI gather/scatter calls are used.
 *              HDF5 will try to collective IO if possible.
 *              If users choose to use
 *              H5Pset_dxpl_mpio_chunk_opt(dxpl_id,H5FD_MPIO_OPT_MULTI_IO),
 *              this function will be called.
 *              The HDF5 library won't do any IO management but leave it to MPI-IO to figure
 *              out.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Muqun Yang
 *              Monday, Feb. 13th, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_multi_chunk_collective_io_no_opt(H5D_io_info_t *io_info,
    const H5D_type_info_t *type_info, H5D_chunk_map_t *fm, H5P_genplist_t *dx_plist)
{
    H5SL_node_t    *chunk_node;           /* Current node in chunk skip list */
    H5D_io_info_t   ctg_io_info;          /* Contiguous I/O info object */
    H5D_storage_t   ctg_store;            /* Chunk storage information as contiguous dataset */
    H5D_io_info_t   cpt_io_info;          /* Compact I/O info object */
    H5D_storage_t   cpt_store;            /* Chunk storage information as compact dataset */
    hbool_t         cpt_dirty;            /* Temporary placeholder for compact storage "dirty" flag */
    int             min_chunk = -1;       /* Minimum # of chunks all processes will operate on */
    int             count_chunk;          /* How many chunks have we operated on? */
    H5D_storage_t   store;                /* union of EFL and chunk pointer in file space */
    herr_t          ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5D_multi_chunk_collective_io_no_opt)

#ifdef H5D_DEBUG
if(H5DEBUG(D)) {
    int mpi_rank;

    mpi_rank = H5F_mpi_get_rank(io_info->dset->oloc.file);
    HDfprintf(H5DEBUG(D), "Rank %d: coming to multi_chunk_collective_io_no_opt\n", mpi_rank);
}
#endif

    /* Set up contiguous I/O info object */
    HDmemcpy(&ctg_io_info, io_info, sizeof(ctg_io_info));
    ctg_io_info.store = &ctg_store;
    ctg_io_info.layout_ops = *H5D_LOPS_CONTIG;

    /* Initialize temporary contiguous storage info */
    ctg_store.contig.dset_size = (hsize_t)io_info->dset->shared->layout.u.chunk.size;

    /* Set up compact I/O info object */
    HDmemcpy(&cpt_io_info, io_info, sizeof(cpt_io_info));
    cpt_io_info.store = &cpt_store;
    cpt_io_info.layout_ops = *H5D_LOPS_COMPACT;

    /* Initialize temporary compact storage info */
    cpt_store.compact.dirty = &cpt_dirty;

    /* Set dataset storage for I/O info */
    io_info->store = &store;

    /* Get the min. # of chunks */
    if(H5D_mpio_get_min_chunk(io_info, fm, &min_chunk) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get minimum number of chunk")
    HDassert(min_chunk >= 0);

    /* Get first node in chunk skip list */
    chunk_node = H5SL_first(fm->sel_chunks);
    count_chunk = 0;

     /* Iterate through chunks to be operated on */
    while(chunk_node) {
        H5D_chunk_info_t *chunk_info;   /* chunk information */
        H5D_chunk_ud_t udata;		/* B-tree pass-through	*/
        hbool_t make_ind, make_coll;    /* Flags to indicate that the MPI mode should change */

        /* Get the actual chunk information from the skip list node */
        chunk_info = H5SL_item(chunk_node);

        /* Pass in chunk's coordinates in a union. */
        store.chunk.offset = chunk_info->coords;
        store.chunk.index = chunk_info->index;

        /* Reset flags for changing parallel I/O mode */
        make_ind = make_coll = FALSE;

        count_chunk++;

        /* If the number of chunk is greater than minimum number of chunk,
         *      Do independent read.
         */
        if(count_chunk > min_chunk)
            /* Switch to independent I/O (permanently) */
            make_ind = TRUE;
#ifndef H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS
/* This case needs to be improved to check if the selected space
   is regular. If all selections are regular, collective IO can still be done.
   However, since we find an MPI-IO bug at a DOE machine(mcr) that cannot
   handle collective I/O selection for this case correctly,
   we turn off this optimization but leave the following code
   for future optimization. Otherwise, the following else {} doesn't make sense.
   KY 2006/8/4/ */
        else {
            /* Switch to independent I/O (temporarily) */
            make_ind = TRUE;
            make_coll = TRUE;
        } /* end else */
#endif /* H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS */

        /* Retrieve the chunk's address */
        if(H5D_chunk_get_info(io_info->dset, io_info->dxpl_id, chunk_info->coords, &udata) < 0)
            HGOTO_ERROR(H5E_STORAGE, H5E_CANTGET, FAIL, "couldn't get chunk info from skipped list")

        /* Independent I/O */
        if(make_ind) {
            void *chunk;                    /* Pointer to the data chunk in cache */
            H5D_io_info_t *chk_io_info;     /* Pointer to I/O info object for this chunk */
            uint32_t accessed_bytes = 0;    /* Total accessed size in a chunk */
            unsigned idx_hint = 0;          /* Cache index hint      */
            htri_t cacheable;               /* Whether the chunk is cacheable */

            /* Switch to independent I/O */
            if(H5D_ioinfo_xfer_mode(io_info, dx_plist, H5FD_MPIO_INDEPENDENT) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't switch to independent I/O")

            /* Load the chunk into cache and lock it. */
            if((cacheable = H5D_chunk_cacheable(io_info, udata.addr,
                    io_info->op_type == H5D_IO_OP_WRITE)) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't tell if chunk is cacheable")
            if(cacheable) {
                hbool_t entire_chunk = TRUE;         /* Whether whole chunk is selected */

                /* Compute # of bytes accessed in chunk */
                accessed_bytes = chunk_info->chunk_points * type_info->src_type_size;

                /* Determine if we will access all the data in the chunk */
                if(((io_info->op_type == H5D_IO_OP_WRITE) && (accessed_bytes != ctg_store.contig.dset_size))
                        || (io_info->op_type != H5D_IO_OP_WRITE))
                    entire_chunk = FALSE;

                /* Lock the chunk into the cache */
                if(NULL == (chunk = H5D_chunk_lock(io_info, &udata, entire_chunk, &idx_hint)))
                    HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "unable to read raw data chunk")

                /* Set up the storage buffer information for this chunk */
                cpt_store.compact.buf = chunk;

                /* Point I/O info at contiguous I/O info for this chunk */
                chk_io_info = &cpt_io_info;
            } /* end if */
            else {
                /* Set up the storage address information for this chunk */
                ctg_store.contig.dset_addr = udata.addr;

                /* No chunk cached */
                chunk = NULL;

                /* Point I/O info at temporary I/O info for this chunk */
                chk_io_info = &ctg_io_info;
            } /* end else */

            if(io_info->op_type == H5D_IO_OP_WRITE) {
                if((io_info->io_ops.single_write)(chk_io_info, type_info,
                        (hsize_t)chunk_info->chunk_points, chunk_info->fspace, chunk_info->mspace) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "optimized write failed")
            } /* end if */
            else {
                if((io_info->io_ops.single_read)(chk_io_info, type_info,
                        (hsize_t)chunk_info->chunk_points, chunk_info->fspace, chunk_info->mspace) < 0)
                   HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "optimized read failed")
            } /* end ese */

            /* Release the cache lock on the chunk. */
            if(chunk)
                if(H5D_chunk_unlock(io_info, &udata, (io_info->op_type == H5D_IO_OP_WRITE), idx_hint, chunk, accessed_bytes) < 0)
                    HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "unable to unlock raw data chunk")
        } /* end if */
        else { /*collective I/O */
            /* Set up the storage address information for this chunk */
            ctg_store.contig.dset_addr = udata.addr;

            if(H5D_inter_collective_io(&ctg_io_info, type_info, chunk_info->fspace, chunk_info->mspace) < 0)
                HGOTO_ERROR(H5E_IO, H5E_CANTGET, FAIL,"couldn't finish shared collective MPI-IO")
        } /* end else */

        if(make_coll)
            if(H5D_ioinfo_xfer_mode(io_info, dx_plist, H5FD_MPIO_COLLECTIVE) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't switch to independent I/O")

        /* Get the next chunk node in the skip list */
        chunk_node = H5SL_next(chunk_node);
    } /* end while */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_multi_chunk_collective_io_no_opt */


/*-------------------------------------------------------------------------
 * Function:	H5D_inter_collective_io
 *
 * Purpose:	Routine for the shared part of collective IO between multiple chunk
 *              collective IO and contiguous collective IO
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Muqun Yang
 *              Monday, Feb. 13th, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_inter_collective_io(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    const H5S_t *file_space, const H5S_t *mem_space)
{
    size_t mpi_buf_count;               /* # of MPI types */
    hbool_t mbt_is_derived = FALSE;
    hbool_t mft_is_derived = FALSE;
    MPI_Datatype        mpi_file_type, mpi_buf_type;
    int                 mpi_code;           /* MPI return code */
    herr_t       ret_value = SUCCEED;  /* return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_inter_collective_io)

    if((file_space != NULL) && (mem_space != NULL)) {
        hsize_t	mpi_buf_offset, mpi_file_offset;   /* Offset within dataset where selection (ie. MPI type) begins */
        size_t  mpi_file_count;         /* Number of file "objects" to transfer */

        /* Obtain disk and memory MPI derived datatype */
        if(H5S_mpio_space_type(file_space, type_info->src_type_size,
                &mpi_file_type, &mpi_file_count, &mpi_file_offset, &mft_is_derived) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL, "couldn't create MPI file type")
        if(H5S_mpio_space_type(mem_space, type_info->src_type_size,
                &mpi_buf_type, &mpi_buf_count, &mpi_buf_offset, &mbt_is_derived) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL, "couldn't create MPI buffer type")
    } /* end if */
    else {
        /* For non-selection, participate with a none MPI derived datatype, the count is 0.  */
        mpi_buf_type   = MPI_BYTE;
        mpi_file_type  = MPI_BYTE;
        mpi_buf_count  = (size_t)0;
        mbt_is_derived = FALSE;
        mft_is_derived = FALSE;
    } /* end else */

#ifdef H5D_DEBUG
if(H5DEBUG(D))
    HDfprintf(H5DEBUG(D),"before final collective IO \n");
#endif

    /* Perform final collective I/O operation */
    if(H5D_final_collective_io(io_info, type_info, mpi_buf_count, &mpi_file_type, &mpi_buf_type) < 0)
        HGOTO_ERROR(H5E_IO, H5E_CANTGET, FAIL, "couldn't finish collective MPI-IO")

done:
    /* Free the MPI buf and file types, if they were derived */
    if(mbt_is_derived && MPI_SUCCESS != (mpi_code = MPI_Type_free(&mpi_buf_type)))
        HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)
    if(mft_is_derived && MPI_SUCCESS != (mpi_code = MPI_Type_free(&mpi_file_type)))
        HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)

#ifdef H5D_DEBUG
if(H5DEBUG(D))
    HDfprintf(H5DEBUG(D),"before leaving inter_collective_io ret_value = %d\n",ret_value);
#endif

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_inter_collective_io() */


/*-------------------------------------------------------------------------
 * Function:	H5D_final_collective_io
 *
 * Purpose:	Routine for the common part of collective IO with different storages.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Muqun Yang
 *              Monday, Feb. 13th, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_final_collective_io(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    size_t mpi_buf_count, MPI_Datatype *mpi_file_type, MPI_Datatype *mpi_buf_type)
{
    hbool_t	plist_is_setup = FALSE; /* Whether the dxpl has been customized */
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5D_final_collective_io)

    /* Pass buf type, file type to the file driver.  */
    if(H5FD_mpi_setup_collective(io_info->dxpl_id, *mpi_buf_type, *mpi_file_type) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set MPI-I/O properties")
    plist_is_setup = TRUE;

    if(io_info->op_type == H5D_IO_OP_WRITE) {
        if((io_info->io_ops.single_write)(io_info, type_info, (hsize_t)mpi_buf_count, NULL, NULL) < 0)
	    HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "optimized write failed")
    } /* end if */
    else {
        if((io_info->io_ops.single_read)(io_info, type_info, (hsize_t)mpi_buf_count, NULL, NULL) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "optimized read failed")
    } /* end else */

done:
    /* Reset the dxpl settings */
    if(plist_is_setup)
        if(H5FD_mpi_teardown_collective(io_info->dxpl_id) < 0)
            HDONE_ERROR(H5E_DATASPACE, H5E_CANTFREE, FAIL, "unable to reset dxpl values")

#ifdef H5D_DEBUG
if(H5DEBUG(D))
    HDfprintf(H5DEBUG(D),"ret_value before leaving final_collective_io=%d\n",ret_value);
#endif
      FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_final_collective_io */

#ifdef H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS

/*-------------------------------------------------------------------------
 * Function:	H5D_sort_chunk
 *
 * Purpose:	Routine to sort chunks in increasing order of chunk address
 *              Each chunk address is also obtained.
 *
 * Description:
 *              For most cases, the chunk address has already been sorted in increasing order.
 *              The special sorting flag is used to optimize this common case.
 *              quick sort is used for necessary sorting.
 *
 * Parameters:
 *              Input: H5D_io_info_t* io_info,
 *                      H5D_chunk_map_t *fm(global chunk map struct)
 *              Input/Output:  H5D_chunk_addr_info_t chunk_addr_info_array[]   : array to store chunk address and information
 *                     many_chunk_opt                         : flag to optimize the way to obtain chunk addresses
 *                                                              for many chunks
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Muqun Yang
 *              Monday, Feb. 13th, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_sort_chunk(H5D_io_info_t *io_info, const H5D_chunk_map_t *fm,
    H5D_chunk_addr_info_t chunk_addr_info_array[], int sum_chunk)
{
    H5SL_node_t    *chunk_node;         /* Current node in chunk skip list */
    H5D_chunk_info_t *chunk_info;         /* Current chunking info. of this node. */
    haddr_t         chunk_addr;         /* Current chunking address of this node */
    haddr_t        *total_chunk_addr_array = NULL; /* The array of chunk address for the total number of chunk */
    hbool_t         do_sort = FALSE;    /* Whether the addresses need to be sorted */
    int             bsearch_coll_chunk_threshold;
    int             many_chunk_opt = H5D_OBTAIN_ONE_CHUNK_ADDR_IND;
    int             mpi_size;                   /* Number of MPI processes */
    int             mpi_code;                   /* MPI return code */
    int             i;                          /* Local index variable */
    herr_t	    ret_value = SUCCEED;        /* Return value		*/

    FUNC_ENTER_NOAPI_NOINIT(H5D_sort_chunk)

    /* Retrieve # of MPI processes */
    if((mpi_size = H5F_mpi_get_size(io_info->dset->oloc.file)) < 0)
        HGOTO_ERROR(H5E_IO, H5E_MPI, FAIL, "unable to obtain mpi size")

    /* Calculate the actual threshold to obtain all chunk addresses collectively
     *  The bigger this number is, the more possible the use of obtaining chunk
     * address collectively.
     */
    /* For non-optimization one-link IO, actual bsearch threshold is always
     *   0, we would always want to obtain the chunk addresses individually
     *   for each process.
     */
    bsearch_coll_chunk_threshold = (sum_chunk * 100) / ((int)fm->layout->u.chunk.nchunks * mpi_size);
    if((bsearch_coll_chunk_threshold > H5D_ALL_CHUNK_ADDR_THRES_COL)
            && ((sum_chunk / mpi_size) >= H5D_ALL_CHUNK_ADDR_THRES_COL_NUM))
        many_chunk_opt = H5D_OBTAIN_ALL_CHUNK_ADDR_COL;

#ifdef H5D_DEBUG
if(H5DEBUG(D))
    HDfprintf(H5DEBUG(D), "many_chunk_opt= %d\n", many_chunk_opt);
#endif

    /* If we need to optimize the way to obtain the chunk address */
    if(many_chunk_opt != H5D_OBTAIN_ONE_CHUNK_ADDR_IND) {
        int mpi_rank;

#ifdef H5D_DEBUG
if(H5DEBUG(D))
    HDfprintf(H5DEBUG(D), "Coming inside H5D_OBTAIN_ALL_CHUNK_ADDR_COL\n");
#endif
        /* Allocate array for chunk addresses */
        if(NULL == (total_chunk_addr_array = H5MM_malloc(sizeof(haddr_t) * (size_t)fm->layout->u.chunk.nchunks)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate memory chunk address array")

        /* Retrieve all the chunk addresses with process 0 */
        if((mpi_rank = H5F_mpi_get_rank(io_info->dset->oloc.file)) < 0)
            HGOTO_ERROR(H5E_IO, H5E_MPI, FAIL, "unable to obtain mpi rank")
	if(mpi_rank == 0) {
            if(H5D_chunk_addrmap(io_info, total_chunk_addr_array) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get chunk address")
	} /* end if */

	/* Broadcasting the MPI_IO option info. and chunk address info. */
	if(MPI_SUCCESS != (mpi_code = MPI_Bcast(total_chunk_addr_array, (int)(sizeof(haddr_t) * fm->layout->u.chunk.nchunks), MPI_BYTE, (int)0, io_info->comm)))
	   HMPI_GOTO_ERROR(FAIL, "MPI_BCast failed", mpi_code)
    } /* end if */

    /* Start at first node in chunk skip list */
    i = 0;
    if(NULL == (chunk_node = H5SL_first(fm->sel_chunks)))
        HGOTO_ERROR(H5E_STORAGE, H5E_CANTGET, FAIL,"couldn't get chunk node from skipped list")

    /* Iterate over all chunks for this process */
    while(chunk_node) {
        if(NULL == (chunk_info = H5SL_item(chunk_node)))
            HGOTO_ERROR(H5E_STORAGE, H5E_CANTGET, FAIL,"couldn't get chunk info from skipped list")

        if(many_chunk_opt == H5D_OBTAIN_ONE_CHUNK_ADDR_IND) {
            H5D_chunk_ud_t udata;   /* User data for querying chunk info */

            /* Get address of chunk */
            if(H5D_chunk_get_info(io_info->dset, io_info->dxpl_id, chunk_info->coords, &udata) < 0)
                HGOTO_ERROR(H5E_STORAGE, H5E_CANTGET, FAIL, "couldn't get chunk info from skipped list")
            chunk_addr = udata.addr;
        } /* end if */
        else
            chunk_addr = total_chunk_addr_array[chunk_info->index];

        /* Check if chunk addresses are not in increasing order in the file */
        if(i > 0 && chunk_addr < chunk_addr_info_array[i - 1].chunk_addr)
            do_sort = TRUE;

        /* Set the address & info for this chunk */
        chunk_addr_info_array[i].chunk_addr = chunk_addr;
        chunk_addr_info_array[i].chunk_info = *chunk_info;

        /* Advance to next chunk in list */
        i++;
        chunk_node = H5SL_next(chunk_node);
    } /* end while */

#ifdef H5D_DEBUG
if(H5DEBUG(D))
    HDfprintf(H5DEBUG(D), "before Qsort\n");
#endif
    if(do_sort) {
        size_t num_chunks = H5SL_count(fm->sel_chunks);

        HDqsort(chunk_addr_info_array, num_chunks, sizeof(chunk_addr_info_array[0]), H5D_cmp_chunk_addr);
    } /* end if */

done:
    if(total_chunk_addr_array)
        H5MM_xfree(total_chunk_addr_array);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_sort_chunk() */
#endif /* H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS */


/*-------------------------------------------------------------------------
 * Function:	H5D_obtain_mpio_mode
 *
 * Purpose:	Routine to obtain each io mode(collective,independent or none) for each chunk;
 *              Each chunk address is also obtained.
 *
 * Description:
 *
 *              1) Each process provides two piece of information for all chunks having selection
 *                 a) chunk index
 *                 b) wheather this chunk is regular(for MPI derived datatype not working case)
 *
 *              2) Gather all the information to the root process
 *
 *              3) Root process will do the following:
 *                 a) Obtain chunk addresses for all chunks in this data space
 *                 b) With the consideration of the user option, calculate IO mode for each chunk
 *                 c) Build MPI derived datatype to combine "chunk address" and "assign_io" information
 *                      in order to do MPI Bcast only once
 *                 d) MPI Bcast the IO mode and chunk address information for each chunk.
 *              4) Each process then retrieves IO mode and chunk address information to assign_io_mode and chunk_addr.
 *
 * Parameters:
 *
 *              Input: H5D_io_info_t* io_info,
 *                      H5D_chunk_map_t *fm,(global chunk map struct)
 *              Output: uint8_t assign_io_mode[], : IO mode, collective, independent or none
 *                      haddr_t chunk_addr[],     : chunk address array for each chunk
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Muqun Yang
 *              Monday, Feb. 13th, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_obtain_mpio_mode(H5D_io_info_t* io_info, H5D_chunk_map_t *fm,
    H5P_genplist_t *dx_plist, uint8_t assign_io_mode[], haddr_t chunk_addr[])
{
    int               total_chunks;
    unsigned          percent_nproc_per_chunk, threshold_nproc_per_chunk;
#if defined(H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS) && defined(H5_MPI_SPECIAL_COLLECTIVE_IO_WORKS)
    H5FD_mpio_chunk_opt_t chunk_opt_mode;
#endif
    uint8_t*          io_mode_info = NULL;
    uint8_t*          recv_io_mode_info = NULL;
    uint8_t*          mergebuf = NULL;
    uint8_t*          tempbuf;
    H5SL_node_t*      chunk_node;
    H5D_chunk_info_t* chunk_info;
    int               mpi_size, mpi_rank;
    MPI_Comm          comm;
    int               ic, root;
    int               mpi_code;
    hbool_t           mem_cleanup      = FALSE;
#ifdef H5_HAVE_INSTRUMENTED_LIBRARY
    int new_value;
    htri_t check_prop;
#endif
    herr_t            ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5D_obtain_mpio_mode)

    /* Assign the rank 0 to the root */
    root              = 0;
    comm              = io_info->comm;

    /* Obtain the number of process and the current rank of the process */
    if((mpi_rank = H5F_mpi_get_rank(io_info->dset->oloc.file)) < 0)
        HGOTO_ERROR(H5E_IO, H5E_MPI, FAIL, "unable to obtain mpi rank")
    if((mpi_size = H5F_mpi_get_size(io_info->dset->oloc.file)) < 0)
        HGOTO_ERROR(H5E_IO, H5E_MPI, FAIL, "unable to obtain mpi size")

    /* Setup parameters */
    H5_ASSIGN_OVERFLOW(total_chunks, fm->layout->u.chunk.nchunks, hsize_t, int);
    percent_nproc_per_chunk = H5P_peek_unsigned(dx_plist, H5D_XFER_MPIO_CHUNK_OPT_RATIO_NAME);
#if defined(H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS) && defined(H5_MPI_SPECIAL_COLLECTIVE_IO_WORKS)
    chunk_opt_mode = (H5FD_mpio_chunk_opt_t)H5P_peek_unsigned(dx_plist, H5D_XFER_MPIO_CHUNK_OPT_HARD_NAME);
    if((chunk_opt_mode == H5FD_MPIO_CHUNK_MULTI_IO) || (percent_nproc_per_chunk == 0)) {
        if(H5D_chunk_addrmap(io_info, chunk_addr) < 0)
           HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get chunk address");
        for(ic = 0; ic < total_chunks; ic++)
           assign_io_mode[ic] = H5D_CHUNK_IO_MODE_COL;

        HGOTO_DONE(SUCCEED)
    } /* end if */
#endif
    threshold_nproc_per_chunk = mpi_size * percent_nproc_per_chunk/100;

    /* Allocate memory */
    io_mode_info      = (uint8_t *)H5MM_calloc(total_chunks);
    mergebuf          = H5MM_malloc((sizeof(haddr_t) + 1) * total_chunks);
    tempbuf           = mergebuf + total_chunks;
    if(mpi_rank == root)
        recv_io_mode_info = (uint8_t *)H5MM_malloc(total_chunks * mpi_size);
    mem_cleanup       = TRUE;

    /* Obtain the regularity and selection information for all chunks in this process. */
    chunk_node        = H5SL_first(fm->sel_chunks);
    while(chunk_node) {
        chunk_info    = H5SL_item(chunk_node);

#ifndef H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS
        /* regularity information: 1, selection information: 2 */
        if(H5S_SELECT_IS_REGULAR(chunk_info->fspace) == TRUE &&
                H5S_SELECT_IS_REGULAR(chunk_info->mspace) == TRUE)
#endif
            io_mode_info[chunk_info->index] = H5D_CHUNK_SELECT_REG; /* this chunk is selected and is "regular" without defining H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS. */
#ifndef H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS
        else
            io_mode_info[chunk_info->index] = H5D_CHUNK_SELECT_IRREG; /* this chunk is selected and is irregular*/
#endif
        chunk_node = H5SL_next(chunk_node);
    } /* end while */

    /* Gather all the information */
    if(MPI_SUCCESS != (mpi_code = MPI_Gather(io_mode_info, total_chunks, MPI_BYTE, recv_io_mode_info, total_chunks, MPI_BYTE, root, comm)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Gather failed", mpi_code)

    /* Calculate the mode for IO(collective, independent or none) at root process */
    if(mpi_rank == root) {
        int               nproc;
        int*              nproc_per_chunk;
#if !defined(H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS) || !defined(H5_MPI_SPECIAL_COLLECTIVE_IO_WORKS)
        int*              ind_this_chunk;
#endif

        /* pre-computing: calculate number of processes and
            regularity of the selection occupied in each chunk */
        nproc_per_chunk = (int*)H5MM_calloc(total_chunks * sizeof(int));
#if !defined(H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS) || !defined(H5_MPI_SPECIAL_COLLECTIVE_IO_WORKS)
        ind_this_chunk   = (int*)H5MM_calloc(total_chunks * sizeof(int));
#endif

        /* calculating the chunk address */
        if(H5D_chunk_addrmap(io_info, chunk_addr) < 0) {
            HDfree(nproc_per_chunk);
#if !defined(H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS) || !defined(H5_MPI_SPECIAL_COLLECTIVE_IO_WORKS)
            HDfree(ind_this_chunk);
#endif
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get chunk address")
        } /* end if */

        /* checking for number of process per chunk and regularity of the selection*/
        for(nproc = 0; nproc < mpi_size; nproc++) {
            uint8_t *tmp_recv_io_mode_info = recv_io_mode_info + (nproc * total_chunks);

            /* Calculate the number of process per chunk and adding irregular selection option */
            for(ic = 0; ic < total_chunks; ic++, tmp_recv_io_mode_info++) {
                if(*tmp_recv_io_mode_info != 0) {
                    nproc_per_chunk[ic]++;
#ifndef H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS
                    if(*tmp_recv_io_mode_info == H5D_CHUNK_SELECT_IRREG)
                        ind_this_chunk[ic] = 1;
#endif
                } /* end if */
#ifndef H5_MPI_SPECIAL_COLLECTIVE_IO_WORKS
                else {
                    /*checking whether we have a selection in this chunk */
                    ind_this_chunk[ic] = 1;
                } /* end else */
#endif
            } /* end for */
        } /* end for */

        /* Calculating MPIO mode for each chunk (collective, independent, none) */
        for(ic = 0; ic < total_chunks; ic++) {
            if(nproc_per_chunk[ic] > MAX(1, threshold_nproc_per_chunk)) {
#if !defined(H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS) || !defined(H5_MPI_SPECIAL_COLLECTIVE_IO_WORKS)
                if(!ind_this_chunk[ic])
                    assign_io_mode[ic] = H5D_CHUNK_IO_MODE_COL;
#else
                assign_io_mode[ic] = H5D_CHUNK_IO_MODE_COL;
#endif
            } /* end if */
        } /* end for */


        /* merge buffer io_mode info and chunk addr into one */
        HDmemcpy(mergebuf, assign_io_mode, total_chunks);
        HDmemcpy(tempbuf, chunk_addr, sizeof(haddr_t) * total_chunks);

        HDfree(nproc_per_chunk);
#if !defined(H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS) || !defined(H5_MPI_SPECIAL_COLLECTIVE_IO_WORKS)
        HDfree(ind_this_chunk);
#endif
    } /* end if */

    /* Broadcasting the MPI_IO option info. and chunk address info. */
    if(MPI_SUCCESS != (mpi_code = MPI_Bcast(mergebuf, ((sizeof(haddr_t) + 1) * total_chunks), MPI_BYTE, root, comm)))
        HMPI_GOTO_ERROR(FAIL, "MPI_BCast failed", mpi_code)

    HDmemcpy(assign_io_mode, mergebuf, total_chunks);
    HDmemcpy(chunk_addr, tempbuf, sizeof(haddr_t) * total_chunks);

#ifdef H5_HAVE_INSTRUMENTED_LIBRARY
    check_prop = H5Pexist(io_info->dxpl_id, H5D_XFER_COLL_CHUNK_MULTI_RATIO_COLL_NAME);
    if(check_prop > 0) {
#if !defined(H5_MPI_COMPLEX_DERIVED_DATATYPE_WORKS) || !defined(H5_MPI_SPECIAL_COLLECTIVE_IO_WORKS)
        new_value = 0;
        if(H5Pset(io_info->dxpl_id, H5D_XFER_COLL_CHUNK_MULTI_RATIO_COLL_NAME, &new_value) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_UNSUPPORTED, FAIL, "unable to set property value")
#else
        for(ic = 0; ic < total_chunks; ic++) {
            if(assign_io_mode[ic] == H5D_CHUNK_IO_MODE_COL) {
                new_value = 0;
                if(H5Pset(io_info->dxpl_id,H5D_XFER_COLL_CHUNK_MULTI_RATIO_COLL_NAME,&new_value) < 0)
                    HGOTO_ERROR(H5E_PLIST, H5E_UNSUPPORTED, FAIL, "unable to set property value")
                break;
            } /* end if */
        } /* end for */
#endif
    } /* end if */

    check_prop = H5Pexist(io_info->dxpl_id, H5D_XFER_COLL_CHUNK_MULTI_RATIO_IND_NAME);
    if(check_prop > 0) {
        int temp_count = 0;

        for(ic = 0; ic < total_chunks; ic++) {
            if(assign_io_mode[ic] == H5D_CHUNK_IO_MODE_COL) {
                temp_count++;
                break;
            } /* end if */
        } /* end for */
        if(temp_count == 0) {
            new_value = 0;
            if(H5Pset(io_info->dxpl_id, H5D_XFER_COLL_CHUNK_MULTI_RATIO_IND_NAME, &new_value) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_UNSUPPORTED, FAIL, "unable to set property value")
        } /* end if */
    } /* end if */
#endif

done:
    if(mem_cleanup) {
        HDfree(io_mode_info);
        HDfree(mergebuf);
        if(mpi_rank == root)
            HDfree(recv_io_mode_info);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_obtain_mpio_mode() */

static int
H5D_cmp_chunk_addr(const void *chunk_addr_info1, const void *chunk_addr_info2)
{
   haddr_t addr1, addr2;

   FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_cmp_chunk_addr)

   addr1 = ((const H5D_chunk_addr_info_t *)chunk_addr_info1)->chunk_addr;
   addr2 = ((const H5D_chunk_addr_info_t *)chunk_addr_info2)->chunk_addr;

   FUNC_LEAVE_NOAPI(H5F_addr_cmp(addr1, addr2))
}
#endif  /* H5_HAVE_PARALLEL */

