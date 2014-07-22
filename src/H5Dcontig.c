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
 * Programmer: 	Quincey Koziol <koziol@ncsa.uiuc.edu>
 *	       	Thursday, September 28, 2000
 *
 * Purpose:
 *      Contiguous dataset I/O functions. These routines are similar to
 *      the H5D_chunk_* routines and really only an abstract way of dealing
 *      with the data sieve buffer from H5F_seq_read/write.
 */

/****************/
/* Module Setup */
/****************/

#define H5D_PACKAGE		/*suppress error about including H5Dpkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Dpkg.h"		/* Dataset functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"		/* Files				*/
#include "H5FDprivate.h"	/* File drivers				*/
#include "H5FLprivate.h"	/* Free Lists                           */
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MFprivate.h"	/* File memory management		*/
#include "H5Oprivate.h"		/* Object headers		  	*/
#include "H5Pprivate.h"		/* Property lists			*/
#include "H5VMprivate.h"		/* Vector and array functions		*/


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/

/* Callback info for sieve buffer readvv operation */
typedef struct H5D_contig_readvv_sieve_ud_t {
    H5F_t *file;                /* File for dataset */
    H5D_rdcdc_t *dset_contig;   /* Cached information about contiguous data */
    const H5D_contig_storage_t *store_contig;    /* Contiguous storage info for this I/O operation */
    unsigned char *rbuf;        /* Pointer to buffer to fill */
    hid_t dxpl_id;              /* DXPL for operation */
} H5D_contig_readvv_sieve_ud_t;

/* Callback info for [plain] readvv operation */
typedef struct H5D_contig_readvv_ud_t {
    H5F_t *file;                /* File for dataset */
    haddr_t dset_addr;          /* Address of dataset */
    unsigned char *rbuf;        /* Pointer to buffer to fill */
    hid_t dxpl_id;              /* DXPL for operation */
} H5D_contig_readvv_ud_t;

/* Callback info for sieve buffer writevv operation */
typedef struct H5D_contig_writevv_sieve_ud_t {
    H5F_t *file;                /* File for dataset */
    H5D_rdcdc_t *dset_contig;   /* Cached information about contiguous data */
    const H5D_contig_storage_t *store_contig;    /* Contiguous storage info for this I/O operation */
    const unsigned char *wbuf;  /* Pointer to buffer to write */
    hid_t dxpl_id;              /* DXPL for operation */
} H5D_contig_writevv_sieve_ud_t;

/* Callback info for [plain] writevv operation */
typedef struct H5D_contig_writevv_ud_t {
    H5F_t *file;                /* File for dataset */
    haddr_t dset_addr;          /* Address of dataset */
    const unsigned char *wbuf;  /* Pointer to buffer to write */
    hid_t dxpl_id;              /* DXPL for operation */
} H5D_contig_writevv_ud_t;


/********************/
/* Local Prototypes */
/********************/

/* Layout operation callbacks */
static herr_t H5D__contig_construct(H5F_t *f, H5D_t *dset);
static herr_t H5D__contig_io_init(const H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t nelmts, const H5S_t *file_space, const H5S_t *mem_space,
    H5D_chunk_map_t *cm);
static ssize_t H5D__contig_readvv(const H5D_io_info_t *io_info,
    size_t dset_max_nseq, size_t *dset_curr_seq, size_t dset_len_arr[], hsize_t dset_offset_arr[],
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_len_arr[], hsize_t mem_offset_arr[]);
static ssize_t H5D__contig_writevv(const H5D_io_info_t *io_info,
    size_t dset_max_nseq, size_t *dset_curr_seq, size_t dset_len_arr[], hsize_t dset_offset_arr[],
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_len_arr[], hsize_t mem_offset_arr[]);
static herr_t H5D__contig_flush(H5D_t *dset, hid_t dxpl_id);

/* Helper routines */
static herr_t H5D__contig_write_one(H5D_io_info_t *io_info, hsize_t offset,
    size_t size);


/*********************/
/* Package Variables */
/*********************/

/* Contiguous storage layout I/O ops */
const H5D_layout_ops_t H5D_LOPS_CONTIG[1] = {{
    H5D__contig_construct,
    NULL,
    H5D__contig_is_space_alloc,
    H5D__contig_io_init,
    H5D__contig_read,
    H5D__contig_write,
#ifdef H5_HAVE_PARALLEL
    H5D__contig_collective_read,
    H5D__contig_collective_write,
#endif /* H5_HAVE_PARALLEL */
    H5D__contig_readvv,
    H5D__contig_writevv,
    H5D__contig_flush,
    NULL
}};


/*******************/
/* Local Variables */
/*******************/

/* Declare a PQ free list to manage the sieve buffer information */
H5FL_BLK_DEFINE(sieve_buf);

/* Declare extern the free list to manage blocks of type conversion data */
H5FL_BLK_EXTERN(type_conv);



/*-------------------------------------------------------------------------
 * Function:	H5D__contig_alloc
 *
 * Purpose:	Allocate file space for a contiguously stored dataset
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		April 19, 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__contig_alloc(H5F_t *f, hid_t dxpl_id, H5O_storage_contig_t *storage /*out */ )
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_PACKAGE

    /* check args */
    HDassert(f);
    HDassert(storage);

    /* Allocate space for the contiguous data */
    if(HADDR_UNDEF == (storage->addr = H5MF_alloc(f, H5FD_MEM_DRAW, dxpl_id, storage->size)))
        HGOTO_ERROR(H5E_IO, H5E_NOSPACE, FAIL, "unable to reserve file space")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__contig_alloc */


/*-------------------------------------------------------------------------
 * Function:	H5D__contig_fill
 *
 * Purpose:	Write fill values to a contiguously stored dataset.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		August 22, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__contig_fill(const H5D_t *dset, hid_t dxpl_id)
{
    H5D_io_info_t ioinfo;       /* Dataset I/O info */
    H5D_storage_t store;        /* Union of storage info for dataset */
    H5D_dxpl_cache_t _dxpl_cache;       /* Data transfer property cache buffer */
    H5D_dxpl_cache_t *dxpl_cache = &_dxpl_cache;   /* Data transfer property cache */
    hssize_t    snpoints;       /* Number of points in space (for error checking) */
    size_t      npoints;        /* Number of points in space */
    hsize_t	offset;         /* Offset of dataset */
#ifdef H5_HAVE_PARALLEL
    MPI_Comm	mpi_comm = MPI_COMM_NULL;	/* MPI communicator for file */
    int         mpi_rank = (-1);  /* This process's rank  */
    int         mpi_code;       /* MPI return code */
    hbool_t     blocks_written = FALSE; /* Flag to indicate that chunk was actually written */
    hbool_t     using_mpi = FALSE;      /* Flag to indicate that the file is being accessed with an MPI-capable file driver */
#endif /* H5_HAVE_PARALLEL */
    H5D_fill_buf_info_t fb_info;        /* Dataset's fill buffer info */
    hbool_t     fb_info_init = FALSE;   /* Whether the fill value buffer has been initialized */
    hid_t       my_dxpl_id;     /* DXPL ID to use for this operation */
    herr_t	ret_value = SUCCEED;	/* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(TRUE == H5P_isa_class(dxpl_id, H5P_DATASET_XFER));
    HDassert(dset && H5D_CONTIGUOUS == dset->shared->layout.type);
    HDassert(H5F_addr_defined(dset->shared->layout.storage.u.contig.addr));
    HDassert(dset->shared->layout.storage.u.contig.size > 0);
    HDassert(dset->shared->space);
    HDassert(dset->shared->type);

#ifdef H5_HAVE_PARALLEL
    /* Retrieve MPI parameters */
    if(IS_H5FD_MPI(dset->oloc.file)) {
        /* Get the MPI communicator */
        if(MPI_COMM_NULL == (mpi_comm = H5F_mpi_get_comm(dset->oloc.file)))
            HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "Can't retrieve MPI communicator")

        /* Get the MPI rank */
        if((mpi_rank = H5F_mpi_get_rank(dset->oloc.file)) < 0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "Can't retrieve MPI rank")

        /* Set the MPI-capable file driver flag */
        using_mpi = TRUE;

        /* Use the internal "independent" DXPL */
        my_dxpl_id = H5AC_ind_dxpl_id;
    } /* end if */
    else {
#endif  /* H5_HAVE_PARALLEL */
        /* Use the DXPL we were given */
        my_dxpl_id = dxpl_id;
#ifdef H5_HAVE_PARALLEL
    } /* end else */
#endif  /* H5_HAVE_PARALLEL */

    /* Fill the DXPL cache values for later use */
    if(H5D__get_dxpl_cache(my_dxpl_id, &dxpl_cache) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't fill dxpl cache")

    /* Initialize storage info for this dataset */
    store.contig.dset_addr = dset->shared->layout.storage.u.contig.addr;
    store.contig.dset_size = dset->shared->layout.storage.u.contig.size;

    /* Get the number of elements in the dataset's dataspace */
    if((snpoints = H5S_GET_EXTENT_NPOINTS(dset->shared->space)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "dataset has negative number of elements")
    H5_ASSIGN_OVERFLOW(npoints, snpoints, hssize_t, size_t);

    /* Initialize the fill value buffer */
    if(H5D__fill_init(&fb_info, NULL, NULL, NULL, NULL, NULL,
            &dset->shared->dcpl_cache.fill,
            dset->shared->type, dset->shared->type_id, npoints,
            dxpl_cache->max_temp_buf, my_dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't initialize fill buffer info")
    fb_info_init = TRUE;

    /* Start at the beginning of the dataset */
    offset = 0;

    /* Simple setup for dataset I/O info struct */
    H5D_BUILD_IO_INFO_WRT(&ioinfo, dset, dxpl_cache, my_dxpl_id, &store, fb_info.fill_buf);

    /*
     * Fill the entire current extent with the fill value.  We can do
     * this quite efficiently by making sure we copy the fill value
     * in relatively large pieces.
     */

    /* Loop through writing the fill value to the dataset */
    while(npoints > 0) {
        size_t curr_points;     /* Number of elements to write on this iteration of the loop */
        size_t size;            /* Size of buffer to write */

        /* Compute # of elements and buffer size to write for this iteration */
        curr_points = MIN(fb_info.elmts_per_buf, npoints);
        size = curr_points * fb_info.file_elmt_size;

        /* Check for VL datatype & non-default fill value */
        if(fb_info.has_vlen_fill_type)
            /* Re-fill the buffer to use for this I/O operation */
            if(H5D__fill_refill_vl(&fb_info, curr_points, my_dxpl_id) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, FAIL, "can't refill fill value buffer")

#ifdef H5_HAVE_PARALLEL
            /* Check if this file is accessed with an MPI-capable file driver */
            if(using_mpi) {
                /* Write the chunks out from only one process */
                /* !! Use the internal "independent" DXPL!! -QAK */
                if(H5_PAR_META_WRITE == mpi_rank)
                    if(H5D__contig_write_one(&ioinfo, offset, size) < 0)
                        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to write fill value to dataset")

                /* Indicate that blocks are being written */
                blocks_written = TRUE;
            } /* end if */
            else {
#endif /* H5_HAVE_PARALLEL */
                H5_CHECK_OVERFLOW(size, size_t, hsize_t);
                if(H5D__contig_write_one(&ioinfo, offset, size) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to write fill value to dataset")
#ifdef H5_HAVE_PARALLEL
            } /* end else */
#endif /* H5_HAVE_PARALLEL */

          npoints -= curr_points;
          offset += size;
      } /* end while */

#ifdef H5_HAVE_PARALLEL
    /* Only need to block at the barrier if we actually wrote fill values */
    /* And if we are using an MPI-capable file driver */
    if(using_mpi && blocks_written) {
        /* Wait at barrier to avoid race conditions where some processes are
         * still writing out fill values and other processes race ahead to data
         * in, getting bogus data.
         */
        if(MPI_SUCCESS != (mpi_code = MPI_Barrier(mpi_comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Barrier failed", mpi_code)
    } /* end if */
#endif /* H5_HAVE_PARALLEL */

done:
    /* Release the fill buffer info, if it's been initialized */
    if(fb_info_init && H5D__fill_term(&fb_info) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't release fill buffer info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__contig_fill() */


/*-------------------------------------------------------------------------
 * Function:	H5D__contig_delete
 *
 * Purpose:	Delete the file space for a contiguously stored dataset
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		March 20, 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__contig_delete(H5F_t *f, hid_t dxpl_id, const H5O_storage_t *storage)
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_PACKAGE

    /* check args */
    HDassert(f);
    HDassert(storage);

    /* Free the file space for the chunk */
    if(H5MF_xfree(f, H5FD_MEM_DRAW, dxpl_id, storage->u.contig.addr, storage->u.contig.size) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "unable to free contiguous storage space")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__contig_delete */


/*-------------------------------------------------------------------------
 * Function:	H5D__contig_construct
 *
 * Purpose:	Constructs new contiguous layout information for dataset
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, May 22, 2008
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5D__contig_construct(H5F_t *f, H5D_t *dset)
{
    hssize_t snelmts;                   /* Temporary holder for number of elements in dataspace */
    hsize_t nelmts;                     /* Number of elements in dataspace */
    size_t dt_size;                     /* Size of datatype */
    hsize_t tmp_size;                   /* Temporary holder for raw data size */
    size_t tmp_sieve_buf_size;          /* Temporary holder for sieve buffer size */
    hsize_t dim[H5O_LAYOUT_NDIMS];	/* Current size of data in elements */
    hsize_t max_dim[H5O_LAYOUT_NDIMS];  /* Maximum size of data in elements */
    int ndims;                          /* Rank of dataspace */
    int i;                              /* Local index variable */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity checks */
    HDassert(f);
    HDassert(dset);

    /*
     * The maximum size of the dataset cannot exceed the storage size.
     * Also, only the slowest varying dimension of a simple data space
     * can be extendible (currently only for external data storage).
     */

    /* Check for invalid dataset dimensions */
    if((ndims = H5S_get_simple_extent_dims(dset->shared->space, dim, max_dim)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize contiguous storage")
    for(i = 0; i < ndims; i++)
        if(max_dim[i] > dim[i])
            HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "extendible contiguous non-external dataset")

    /* Retrieve the number of elements in the dataspace */
    if((snelmts = H5S_GET_EXTENT_NPOINTS(dset->shared->space)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to retrieve number of elements in dataspace")
    nelmts = (hsize_t)snelmts;

    /* Get the datatype's size */
    if(0 == (dt_size = H5T_GET_SIZE(dset->shared->type)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to retrieve size of datatype")

    /* Compute the size of the dataset's contiguous storage */
    tmp_size = nelmts * dt_size;

    /* Check for overflow during multiplication */
    if(nelmts != (tmp_size / dt_size))
        HGOTO_ERROR(H5E_DATASET, H5E_OVERFLOW, FAIL, "size of dataset's storage overflowed")

    /* Assign the dataset's contiguous storage size */
    dset->shared->layout.storage.u.contig.size = tmp_size;

    /* Get the sieve buffer size for the file */
    tmp_sieve_buf_size = H5F_SIEVE_BUF_SIZE(f);

    /* Adjust the sieve buffer size to the smaller one between the dataset size and the buffer size
     * from the file access property. (SLU - 2012/3/30) */
    if(tmp_size < tmp_sieve_buf_size)
        dset->shared->cache.contig.sieve_buf_size = tmp_size;
    else
        dset->shared->cache.contig.sieve_buf_size = tmp_sieve_buf_size;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__contig_construct() */


/*-------------------------------------------------------------------------
 * Function:	H5D__contig_is_space_alloc
 *
 * Purpose:	Query if space is allocated for layout
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, January 15, 2009
 *
 *-------------------------------------------------------------------------
 */
hbool_t
H5D__contig_is_space_alloc(const H5O_storage_t *storage)
{
    hbool_t ret_value;                  /* Return value */

    FUNC_ENTER_PACKAGE_NOERR

    /* Sanity checks */
    HDassert(storage);

    /* Set return value */
    ret_value = (hbool_t)H5F_addr_defined(storage->u.contig.addr);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__contig_is_space_alloc() */


/*-------------------------------------------------------------------------
 * Function:	H5D__contig_io_init
 *
 * Purpose:	Performs initialization before any sort of I/O on the raw data
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, March 20, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__contig_io_init(const H5D_io_info_t *io_info, const H5D_type_info_t UNUSED *type_info,
    hsize_t UNUSED nelmts, const H5S_t UNUSED *file_space, const H5S_t UNUSED *mem_space,
    H5D_chunk_map_t UNUSED *cm)
{
    FUNC_ENTER_STATIC_NOERR

    io_info->store->contig.dset_addr = io_info->dset->shared->layout.storage.u.contig.addr;
    io_info->store->contig.dset_size = io_info->dset->shared->layout.storage.u.contig.size;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5D__contig_io_init() */


/*-------------------------------------------------------------------------
 * Function:	H5D__contig_read
 *
 * Purpose:	Read from a contiguous dataset.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *		Thursday, April 10, 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__contig_read(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t nelmts, const H5S_t *file_space, const H5S_t *mem_space,
    H5D_chunk_map_t UNUSED *fm)
{
    herr_t	ret_value = SUCCEED;	/*return value		*/

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    HDassert(io_info);
    HDassert(io_info->u.rbuf);
    HDassert(type_info);
    HDassert(mem_space);
    HDassert(file_space);

    /* Read data */
    if((io_info->io_ops.single_read)(io_info, type_info, nelmts, file_space, mem_space) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "contiguous read failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__contig_read() */


/*-------------------------------------------------------------------------
 * Function:	H5D__contig_write
 *
 * Purpose:	Write to a contiguous dataset.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *		Thursday, April 10, 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__contig_write(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t nelmts, const H5S_t *file_space, const H5S_t *mem_space,
    H5D_chunk_map_t UNUSED *fm)
{
    herr_t	ret_value = SUCCEED;	/*return value		*/

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    HDassert(io_info);
    HDassert(io_info->u.wbuf);
    HDassert(type_info);
    HDassert(mem_space);
    HDassert(file_space);

    /* Write data */
    if((io_info->io_ops.single_write)(io_info, type_info, nelmts, file_space, mem_space) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "contiguous write failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__contig_write() */


/*-------------------------------------------------------------------------
 * Function:	H5D__contig_write_one
 *
 * Purpose:	Writes some data from a dataset into a buffer.
 *		The data is contiguous.	 The address is relative to the base
 *		address for the file.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, September 28, 2000
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__contig_write_one(H5D_io_info_t *io_info, hsize_t offset, size_t size)
{
    hsize_t dset_off = offset;  /* Offset in dataset */
    size_t dset_len = size;     /* Length in dataset */
    size_t dset_curr_seq = 0;   /* "Current sequence" in dataset */
    hsize_t mem_off = 0;        /* Offset in memory */
    size_t mem_len = size;      /* Length in memory */
    size_t mem_curr_seq = 0;    /* "Current sequence" in memory */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    HDassert(io_info);

    if(H5D__contig_writevv(io_info, (size_t)1, &dset_curr_seq, &dset_len, &dset_off,
            (size_t)1, &mem_curr_seq, &mem_len, &mem_off) < 0)
        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "vector write failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5D__contig_write_one() */


/*-------------------------------------------------------------------------
 * Function:	H5D__contig_readvv_sieve_cb
 *
 * Purpose:	Callback operator for H5D__contig_readvv() with sieve buffer.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, Sept 30, 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__contig_readvv_sieve_cb(hsize_t dst_off, hsize_t src_off, size_t len,
    void *_udata)
{
    H5D_contig_readvv_sieve_ud_t *udata = (H5D_contig_readvv_sieve_ud_t *)_udata; /* User data for H5VM_opvv() operator */
    H5F_t *file = udata->file;        /* File for dataset */
    H5D_rdcdc_t *dset_contig = udata->dset_contig; /* Cached information about contiguous data */
    const H5D_contig_storage_t *store_contig = udata->store_contig;    /* Contiguous storage info for this I/O operation */
    unsigned char *buf;         /* Pointer to buffer to fill */
    haddr_t addr;               /* Actual address to read */
    haddr_t sieve_start = HADDR_UNDEF, sieve_end = HADDR_UNDEF;     /* Start & end locations of sieve buffer */
    haddr_t contig_end;         /* End locations of block to write */
    size_t sieve_size = (size_t)-1;   /* Size of sieve buffer */
    haddr_t rel_eoa;	        /* Relative end of file address	*/
    hsize_t max_data;           /* Actual maximum size of data to cache */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Stash local copies of these value */
    if(dset_contig->sieve_buf != NULL) {
        sieve_start = dset_contig->sieve_loc;
        sieve_size = dset_contig->sieve_size;
        sieve_end = sieve_start + sieve_size;
    } /* end if */

    /* Compute offset on disk */
    addr = store_contig->dset_addr + dst_off;

    /* Compute offset in memory */
    buf = udata->rbuf + src_off;

    /* Check if the sieve buffer is allocated yet */
    if(NULL == dset_contig->sieve_buf) {
        /* Check if we can actually hold the I/O request in the sieve buffer */
        if(len > dset_contig->sieve_buf_size) {
            if(H5F_block_read(file, H5FD_MEM_DRAW, addr, len, udata->dxpl_id, buf) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "block read failed")
        } /* end if */
        else {
            /* Allocate room for the data sieve buffer */
            if(NULL == (dset_contig->sieve_buf = H5FL_BLK_MALLOC(sieve_buf, dset_contig->sieve_buf_size)))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "memory allocation failed")

            /* Determine the new sieve buffer size & location */
            dset_contig->sieve_loc = addr;

            /* Make certain we don't read off the end of the file */
            if(HADDR_UNDEF == (rel_eoa = H5F_get_eoa(file, H5FD_MEM_DRAW)))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to determine file size")

            /* Set up the buffer parameters */
            max_data = store_contig->dset_size - dst_off;

            /* Compute the size of the sieve buffer */
            H5_ASSIGN_OVERFLOW(dset_contig->sieve_size, MIN3(rel_eoa - dset_contig->sieve_loc, max_data, dset_contig->sieve_buf_size), hsize_t, size_t);

            /* Read the new sieve buffer */
            if(H5F_block_read(file, H5FD_MEM_DRAW, dset_contig->sieve_loc, dset_contig->sieve_size, udata->dxpl_id, dset_contig->sieve_buf) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "block read failed")

            /* Grab the data out of the buffer (must be first piece of data in buffer ) */
            HDmemcpy(buf, dset_contig->sieve_buf, len);

            /* Reset sieve buffer dirty flag */
            dset_contig->sieve_dirty = FALSE;

            /* Stash local copies of these value */
            sieve_start = dset_contig->sieve_loc;
            sieve_size = dset_contig->sieve_size;
            sieve_end = sieve_start+sieve_size;
        } /* end else */
    } /* end if */
    else {
        /* Compute end of sequence to retrieve */
        contig_end = addr + len - 1;

        /* If entire read is within the sieve buffer, read it from the buffer */
        if(addr >= sieve_start && contig_end < sieve_end) {
            unsigned char *base_sieve_buf = dset_contig->sieve_buf + (addr - sieve_start);

            /* Grab the data out of the buffer */
            HDmemcpy(buf, base_sieve_buf, len);
        } /* end if */
        /* Entire request is not within this data sieve buffer */
        else {
            /* Check if we can actually hold the I/O request in the sieve buffer */
            if(len > dset_contig->sieve_buf_size) {
                /* Check for any overlap with the current sieve buffer */
                if((sieve_start >= addr && sieve_start < (contig_end + 1))
                        || ((sieve_end - 1) >= addr && (sieve_end - 1) < (contig_end + 1))) {
                    /* Flush the sieve buffer, if it's dirty */
                    if(dset_contig->sieve_dirty) {
                        /* Write to file */
                        if(H5F_block_write(file, H5FD_MEM_DRAW, sieve_start, sieve_size, udata->dxpl_id, dset_contig->sieve_buf) < 0)
                            HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "block write failed")

                        /* Reset sieve buffer dirty flag */
                        dset_contig->sieve_dirty = FALSE;
                    } /* end if */
                } /* end if */

                /* Read directly into the user's buffer */
                if(H5F_block_read(file, H5FD_MEM_DRAW, addr, len, udata->dxpl_id, buf) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "block read failed")
            } /* end if */
            /* Element size fits within the buffer size */
            else {
                /* Flush the sieve buffer if it's dirty */
                if(dset_contig->sieve_dirty) {
                    /* Write to file */
                    if(H5F_block_write(file, H5FD_MEM_DRAW, sieve_start, sieve_size, udata->dxpl_id, dset_contig->sieve_buf) < 0)
                        HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "block write failed")

                    /* Reset sieve buffer dirty flag */
                    dset_contig->sieve_dirty = FALSE;
                } /* end if */

                /* Determine the new sieve buffer size & location */
                dset_contig->sieve_loc = addr;

                /* Make certain we don't read off the end of the file */
                if(HADDR_UNDEF == (rel_eoa = H5F_get_eoa(file, H5FD_MEM_DRAW)))
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to determine file size")

                /* Only need this when resizing sieve buffer */
                max_data = store_contig->dset_size - dst_off;

                /* Compute the size of the sieve buffer */
                /* Don't read off the end of the file, don't read past the end of the data element and don't read more than the buffer size */
                H5_ASSIGN_OVERFLOW(dset_contig->sieve_size, MIN3(rel_eoa - dset_contig->sieve_loc, max_data, dset_contig->sieve_buf_size), hsize_t, size_t);

                /* Update local copies of sieve information */
                sieve_start = dset_contig->sieve_loc;
                sieve_size = dset_contig->sieve_size;
                sieve_end = sieve_start + sieve_size;

                /* Read the new sieve buffer */
                if(H5F_block_read(file, H5FD_MEM_DRAW, dset_contig->sieve_loc, dset_contig->sieve_size, udata->dxpl_id, dset_contig->sieve_buf) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "block read failed")

                /* Grab the data out of the buffer (must be first piece of data in buffer ) */
                HDmemcpy(buf, dset_contig->sieve_buf, len);

                /* Reset sieve buffer dirty flag */
                dset_contig->sieve_dirty = FALSE;
            } /* end else */
        } /* end else */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5D__contig_readvv_sieve_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D__contig_readvv_cb
 *
 * Purpose:	Callback operator for H5D__contig_readvv() without sieve buffer.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, Sept 30, 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__contig_readvv_cb(hsize_t dst_off, hsize_t src_off, size_t len, void *_udata)
{
    H5D_contig_readvv_ud_t *udata = (H5D_contig_readvv_ud_t *)_udata; /* User data for H5VM_opvv() operator */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

    /* Write data */
    if(H5F_block_read(udata->file, H5FD_MEM_DRAW, (udata->dset_addr + dst_off),
            len, udata->dxpl_id, (udata->rbuf + src_off)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "block write failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5D__contig_readvv_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D__contig_readvv
 *
 * Purpose:	Reads some data vectors from a dataset into a buffer.
 *		The data is contiguous.	 The address is the start of the dataset,
 *              relative to the base address for the file and the offsets and
 *              sequence lengths are in bytes.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Friday, May 3, 2001
 *
 * Notes:
 *      Offsets in the sequences must be monotonically increasing
 *
 *-------------------------------------------------------------------------
 */
static ssize_t
H5D__contig_readvv(const H5D_io_info_t *io_info,
    size_t dset_max_nseq, size_t *dset_curr_seq, size_t dset_len_arr[], hsize_t dset_off_arr[],
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_len_arr[], hsize_t mem_off_arr[])
{
    ssize_t ret_value;          /* Return value */

    FUNC_ENTER_STATIC

    /* Check args */
    HDassert(io_info);
    HDassert(dset_curr_seq);
    HDassert(dset_len_arr);
    HDassert(dset_off_arr);
    HDassert(mem_curr_seq);
    HDassert(mem_len_arr);
    HDassert(mem_off_arr);

    /* Check if data sieving is enabled */
    if(H5F_HAS_FEATURE(io_info->dset->oloc.file, H5FD_FEAT_DATA_SIEVE)) {
        H5D_contig_readvv_sieve_ud_t udata;     /* User data for H5VM_opvv() operator */

        /* Set up user data for H5VM_opvv() */
        udata.file = io_info->dset->oloc.file;
        udata.dset_contig = &(io_info->dset->shared->cache.contig);
        udata.store_contig = &(io_info->store->contig);
        udata.rbuf = (unsigned char *)io_info->u.rbuf;
        udata.dxpl_id = io_info->dxpl_id;

        /* Call generic sequence operation routine */
        if((ret_value = H5VM_opvv(dset_max_nseq, dset_curr_seq, dset_len_arr, dset_off_arr,
                mem_max_nseq, mem_curr_seq, mem_len_arr, mem_off_arr,
                H5D__contig_readvv_sieve_cb, &udata)) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTOPERATE, FAIL, "can't perform vectorized sieve buffer read")
    } /* end if */
    else {
        H5D_contig_readvv_ud_t udata;     /* User data for H5VM_opvv() operator */

        /* Set up user data for H5VM_opvv() */
        udata.file = io_info->dset->oloc.file;
        udata.dset_addr = io_info->store->contig.dset_addr;
        udata.rbuf = (unsigned char *)io_info->u.rbuf;
        udata.dxpl_id = io_info->dxpl_id;

        /* Call generic sequence operation routine */
        if((ret_value = H5VM_opvv(dset_max_nseq, dset_curr_seq, dset_len_arr, dset_off_arr,
                mem_max_nseq, mem_curr_seq, mem_len_arr, mem_off_arr,
                H5D__contig_readvv_cb, &udata)) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTOPERATE, FAIL, "can't perform vectorized read")
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5D__contig_readvv() */


/*-------------------------------------------------------------------------
 * Function:	H5D__contig_writevv_sieve_cb
 *
 * Purpose:	Callback operator for H5D__contig_writevv() with sieve buffer.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, Sept 30, 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__contig_writevv_sieve_cb(hsize_t dst_off, hsize_t src_off, size_t len,
    void *_udata)
{
    H5D_contig_writevv_sieve_ud_t *udata = (H5D_contig_writevv_sieve_ud_t *)_udata; /* User data for H5VM_opvv() operator */
    H5F_t *file = udata->file;        /* File for dataset */
    H5D_rdcdc_t *dset_contig = udata->dset_contig; /* Cached information about contiguous data */
    const H5D_contig_storage_t *store_contig = udata->store_contig;    /* Contiguous storage info for this I/O operation */
    const unsigned char *buf;   /* Pointer to buffer to fill */
    haddr_t addr;               /* Actual address to read */
    haddr_t sieve_start = HADDR_UNDEF, sieve_end = HADDR_UNDEF;     /* Start & end locations of sieve buffer */
    haddr_t contig_end;         /* End locations of block to write */
    size_t sieve_size = (size_t)-1; /* size of sieve buffer */
    haddr_t rel_eoa;	        /* Relative end of file address	*/
    hsize_t max_data;           /* Actual maximum size of data to cache */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Stash local copies of these values */
    if(dset_contig->sieve_buf != NULL) {
        sieve_start = dset_contig->sieve_loc;
        sieve_size = dset_contig->sieve_size;
        sieve_end = sieve_start + sieve_size;
    } /* end if */

    /* Compute offset on disk */
    addr = store_contig->dset_addr + dst_off;

    /* Compute offset in memory */
    buf = udata->wbuf + src_off;

    /* No data sieve buffer yet, go allocate one */
    if(NULL == dset_contig->sieve_buf) {
        /* Check if we can actually hold the I/O request in the sieve buffer */
        if(len > dset_contig->sieve_buf_size) {
            if(H5F_block_write(file, H5FD_MEM_DRAW, addr, len, udata->dxpl_id, buf) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "block write failed")
        } /* end if */
        else {
            /* Allocate room for the data sieve buffer */
            if(NULL == (dset_contig->sieve_buf = H5FL_BLK_MALLOC(sieve_buf, dset_contig->sieve_buf_size)))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "memory allocation failed")

#ifdef H5_CLEAR_MEMORY
if(dset_contig->sieve_size > len)
    HDmemset(dset_contig->sieve_buf + len, 0, (dset_contig->sieve_size - len));
#endif /* H5_CLEAR_MEMORY */

            /* Determine the new sieve buffer size & location */
            dset_contig->sieve_loc = addr;

            /* Make certain we don't read off the end of the file */
            if(HADDR_UNDEF == (rel_eoa = H5F_get_eoa(file, H5FD_MEM_DRAW)))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to determine file size")

            /* Set up the buffer parameters */
            max_data = store_contig->dset_size - dst_off;

            /* Compute the size of the sieve buffer */
            H5_ASSIGN_OVERFLOW(dset_contig->sieve_size, MIN3(rel_eoa - dset_contig->sieve_loc, max_data, dset_contig->sieve_buf_size), hsize_t, size_t);

            /* Check if there is any point in reading the data from the file */
            if(dset_contig->sieve_size > len) {
                /* Read the new sieve buffer */
                if(H5F_block_read(file, H5FD_MEM_DRAW, dset_contig->sieve_loc, dset_contig->sieve_size, udata->dxpl_id, dset_contig->sieve_buf) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "block read failed")
            } /* end if */

            /* Grab the data out of the buffer (must be first piece of data in buffer ) */
            HDmemcpy(dset_contig->sieve_buf, buf, len);

            /* Set sieve buffer dirty flag */
            dset_contig->sieve_dirty = TRUE;

            /* Stash local copies of these values */
            sieve_start = dset_contig->sieve_loc;
            sieve_size = dset_contig->sieve_size;
            sieve_end = sieve_start + sieve_size;
        } /* end else */
    } /* end if */
    else {
        /* Compute end of sequence to retrieve */
        contig_end = addr + len - 1;

        /* If entire write is within the sieve buffer, write it to the buffer */
        if(addr >= sieve_start && contig_end < sieve_end) {
            unsigned char *base_sieve_buf = dset_contig->sieve_buf + (addr - sieve_start);

            /* Put the data into the sieve buffer */
            HDmemcpy(base_sieve_buf, buf, len);

            /* Set sieve buffer dirty flag */
            dset_contig->sieve_dirty = TRUE;
        } /* end if */
        /* Entire request is not within this data sieve buffer */
        else {
            /* Check if we can actually hold the I/O request in the sieve buffer */
            if(len > dset_contig->sieve_buf_size) {
                /* Check for any overlap with the current sieve buffer */
                if((sieve_start >= addr && sieve_start < (contig_end + 1))
                        || ((sieve_end - 1) >= addr && (sieve_end - 1) < (contig_end + 1))) {
                    /* Flush the sieve buffer, if it's dirty */
                    if(dset_contig->sieve_dirty) {
                        /* Write to file */
                        if(H5F_block_write(file, H5FD_MEM_DRAW, sieve_start, sieve_size, udata->dxpl_id, dset_contig->sieve_buf) < 0)
                            HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "block write failed")

                        /* Reset sieve buffer dirty flag */
                        dset_contig->sieve_dirty = FALSE;
                    } /* end if */

                    /* Force the sieve buffer to be re-read the next time */
                    dset_contig->sieve_loc = HADDR_UNDEF;
                    dset_contig->sieve_size = 0;
                } /* end if */

                /* Write directly from the user's buffer */
                if(H5F_block_write(file, H5FD_MEM_DRAW, addr, len, udata->dxpl_id, buf) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "block write failed")
            } /* end if */
            /* Element size fits within the buffer size */
            else {
                /* Check if it is possible to (exactly) prepend or append to existing (dirty) sieve buffer */
                if(((addr + len) == sieve_start || addr == sieve_end) &&
                        (len + sieve_size) <= dset_contig->sieve_buf_size &&
                        dset_contig->sieve_dirty) {
                    /* Prepend to existing sieve buffer */
                    if((addr + len) == sieve_start) {
                        /* Move existing sieve information to correct location */
                        HDmemmove(dset_contig->sieve_buf + len, dset_contig->sieve_buf, dset_contig->sieve_size);

                        /* Copy in new information (must be first in sieve buffer) */
                        HDmemcpy(dset_contig->sieve_buf, buf, len);

                        /* Adjust sieve location */
                        dset_contig->sieve_loc = addr;

                    } /* end if */
                    /* Append to existing sieve buffer */
                    else {
                        /* Copy in new information */
                        HDmemcpy(dset_contig->sieve_buf + sieve_size, buf, len);
                    } /* end else */

                    /* Adjust sieve size */
                    dset_contig->sieve_size += len;

                    /* Update local copies of sieve information */
                    sieve_start = dset_contig->sieve_loc;
                    sieve_size = dset_contig->sieve_size;
                    sieve_end = sieve_start + sieve_size;
                } /* end if */
                /* Can't add the new data onto the existing sieve buffer */
                else {
                    /* Flush the sieve buffer if it's dirty */
                    if(dset_contig->sieve_dirty) {
                        /* Write to file */
                        if(H5F_block_write(file, H5FD_MEM_DRAW, sieve_start, sieve_size, udata->dxpl_id, dset_contig->sieve_buf) < 0)
                            HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "block write failed")

                        /* Reset sieve buffer dirty flag */
                        dset_contig->sieve_dirty = FALSE;
                    } /* end if */

                    /* Determine the new sieve buffer size & location */
                    dset_contig->sieve_loc = addr;

                    /* Make certain we don't read off the end of the file */
                    if(HADDR_UNDEF == (rel_eoa = H5F_get_eoa(file, H5FD_MEM_DRAW)))
                        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to determine file size")

                    /* Only need this when resizing sieve buffer */
                    max_data = store_contig->dset_size - dst_off;

                    /* Compute the size of the sieve buffer */
                    /* Don't read off the end of the file, don't read past the end of the data element and don't read more than the buffer size */
                    H5_ASSIGN_OVERFLOW(dset_contig->sieve_size, MIN3(rel_eoa - dset_contig->sieve_loc, max_data, dset_contig->sieve_buf_size), hsize_t, size_t);

                    /* Update local copies of sieve information */
                    sieve_start = dset_contig->sieve_loc;
                    sieve_size = dset_contig->sieve_size;
                    sieve_end = sieve_start + sieve_size;

                    /* Check if there is any point in reading the data from the file */
                    if(dset_contig->sieve_size > len) {
                        /* Read the new sieve buffer */
                        if(H5F_block_read(file, H5FD_MEM_DRAW, dset_contig->sieve_loc, dset_contig->sieve_size, udata->dxpl_id, dset_contig->sieve_buf) < 0)
                            HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "block read failed")
                    } /* end if */

                    /* Grab the data out of the buffer (must be first piece of data in buffer ) */
                    HDmemcpy(dset_contig->sieve_buf, buf, len);

                    /* Set sieve buffer dirty flag */
                    dset_contig->sieve_dirty = TRUE;
                } /* end else */
            } /* end else */
        } /* end else */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5D__contig_writevv_sieve_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D__contig_writevv_cb
 *
 * Purpose:	Callback operator for H5D__contig_writevv().
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, Sept 30, 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__contig_writevv_cb(hsize_t dst_off, hsize_t src_off, size_t len, void *_udata)
{
    H5D_contig_writevv_ud_t *udata = (H5D_contig_writevv_ud_t *)_udata; /* User data for H5VM_opvv() operator */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Write data */
    if(H5F_block_write(udata->file, H5FD_MEM_DRAW, (udata->dset_addr + dst_off), len, udata->dxpl_id, (udata->wbuf + src_off)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "block write failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5D__contig_writevv_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D__contig_writevv
 *
 * Purpose:	Writes some data vectors into a dataset from vectors into a
 *              buffer.  The address is the start of the dataset,
 *              relative to the base address for the file and the offsets and
 *              sequence lengths are in bytes.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Friday, May 2, 2003
 *
 * Notes:
 *      Offsets in the sequences must be monotonically increasing
 *
 *-------------------------------------------------------------------------
 */
static ssize_t
H5D__contig_writevv(const H5D_io_info_t *io_info,
    size_t dset_max_nseq, size_t *dset_curr_seq, size_t dset_len_arr[], hsize_t dset_off_arr[],
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_len_arr[], hsize_t mem_off_arr[])
{
    ssize_t ret_value;          /* Return value (Size of sequence in bytes) */

    FUNC_ENTER_STATIC

    /* Check args */
    HDassert(io_info);
    HDassert(dset_curr_seq);
    HDassert(dset_len_arr);
    HDassert(dset_off_arr);
    HDassert(mem_curr_seq);
    HDassert(mem_len_arr);
    HDassert(mem_off_arr);

    /* Check if data sieving is enabled */
    if(H5F_HAS_FEATURE(io_info->dset->oloc.file, H5FD_FEAT_DATA_SIEVE)) {
        H5D_contig_writevv_sieve_ud_t udata;    /* User data for H5VM_opvv() operator */

        /* Set up user data for H5VM_opvv() */
        udata.file = io_info->dset->oloc.file;
        udata.dset_contig = &(io_info->dset->shared->cache.contig);
        udata.store_contig = &(io_info->store->contig);
        udata.wbuf = (const unsigned char *)io_info->u.wbuf;
        udata.dxpl_id = io_info->dxpl_id;

        /* Call generic sequence operation routine */
        if((ret_value = H5VM_opvv(dset_max_nseq, dset_curr_seq, dset_len_arr, dset_off_arr,
                mem_max_nseq, mem_curr_seq, mem_len_arr, mem_off_arr,
                H5D__contig_writevv_sieve_cb, &udata)) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTOPERATE, FAIL, "can't perform vectorized sieve buffer write")
    } /* end if */
    else {
        H5D_contig_writevv_ud_t udata;     /* User data for H5VM_opvv() operator */

        /* Set up user data for H5VM_opvv() */
        udata.file = io_info->dset->oloc.file;
        udata.dset_addr = io_info->store->contig.dset_addr;
        udata.wbuf = (const unsigned char *)io_info->u.wbuf;
        udata.dxpl_id = io_info->dxpl_id;

        /* Call generic sequence operation routine */
        if((ret_value = H5VM_opvv(dset_max_nseq, dset_curr_seq, dset_len_arr, dset_off_arr,
                mem_max_nseq, mem_curr_seq, mem_len_arr, mem_off_arr,
                H5D__contig_writevv_cb, &udata)) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTOPERATE, FAIL, "can't perform vectorized read")
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5D__contig_writevv() */


/*-------------------------------------------------------------------------
 * Function:	H5D__contig_flush
 *
 * Purpose:	Writes all dirty data to disk.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, July 27, 2009
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__contig_flush(H5D_t *dset, hid_t dxpl_id)
{
    herr_t ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(dset);

    /* Flush any data in sieve buffer */
    if(H5D__flush_sieve_buf(dset, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTFLUSH, FAIL, "unable to flush sieve buffer")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__contig_flush() */


/*-------------------------------------------------------------------------
 * Function:	H5D__contig_copy
 *
 * Purpose:	Copy contiguous storage raw data from SRC file to DST file.
 *
 * Return:	Non-negative on success, negative on failure.
 *
 * Programmer:  Quincey Koziol
 *	        Monday, November 21, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__contig_copy(H5F_t *f_src, const H5O_storage_contig_t *storage_src,
    H5F_t *f_dst, H5O_storage_contig_t *storage_dst, H5T_t *dt_src,
    H5O_copy_t *cpy_info, hid_t dxpl_id)
{
    haddr_t     addr_src;               /* File offset in source dataset */
    haddr_t     addr_dst;               /* File offset in destination dataset */
    H5T_path_t  *tpath_src_mem = NULL, *tpath_mem_dst = NULL;   /* Datatype conversion paths */
    H5T_t       *dt_dst = NULL;         /* Destination datatype */
    H5T_t       *dt_mem = NULL;         /* Memory datatype */
    hid_t       tid_src = -1;           /* Datatype ID for source datatype */
    hid_t       tid_dst = -1;           /* Datatype ID for destination datatype */
    hid_t       tid_mem = -1;           /* Datatype ID for memory datatype */
    size_t      src_dt_size = 0;        /* Source datatype size */
    size_t      mem_dt_size = 0;        /* Memory datatype size */
    size_t      dst_dt_size = 0;        /* Destination datatype size */
    size_t      max_dt_size;            /* Max. datatype size */
    size_t      nelmts = 0;             /* Number of elements in buffer */
    size_t      src_nbytes;             /* Number of bytes to read from source */
    size_t      mem_nbytes;             /* Number of bytes to convert in memory */
    size_t      dst_nbytes;             /* Number of bytes to write to destination */
    hsize_t     total_src_nbytes;       /* Total number of bytes to copy */
    size_t      buf_size;               /* Size of copy buffer */
    void       *buf = NULL;             /* Buffer for copying data */
    void       *bkg = NULL;             /* Temporary buffer for copying data */
    void       *reclaim_buf = NULL;     /* Buffer for reclaiming data */
    H5S_t      *buf_space = NULL;       /* Dataspace describing buffer */
    hid_t       buf_sid = -1;           /* ID for buffer dataspace */
    hsize_t     buf_dim[1] = {0};       /* Dimension for buffer */
    hbool_t     is_vlen = FALSE;        /* Flag to indicate that VL type conversion should occur */
    hbool_t     fix_ref = FALSE;        /* Flag to indicate that ref values should be fixed */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(f_src);
    HDassert(storage_src);
    HDassert(f_dst);
    HDassert(storage_dst);
    HDassert(dt_src);

    /* Allocate space for destination raw data */
    if(H5D__contig_alloc(f_dst, dxpl_id, storage_dst) < 0)
        HGOTO_ERROR(H5E_IO, H5E_CANTINIT, FAIL, "unable to allocate contiguous storage")

    /* Set up number of bytes to copy, and initial buffer size */
    /* (actually use the destination size, which has been fixed up, if necessary) */
    total_src_nbytes = storage_dst->size;
    H5_CHECK_OVERFLOW(total_src_nbytes, hsize_t, size_t);
    buf_size = MIN(H5D_TEMP_BUF_SIZE, (size_t)total_src_nbytes);

    /* Create datatype ID for src datatype.  We may or may not use this ID,
     * but this ensures that the src datatype will be freed.
     */
    if((tid_src = H5I_register(H5I_DATATYPE, dt_src, FALSE)) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to register source file datatype")

    /* If there's a VLEN source datatype, set up type conversion information */
    if(H5T_detect_class(dt_src, H5T_VLEN, FALSE) > 0) {
        /* create a memory copy of the variable-length datatype */
        if(NULL == (dt_mem = H5T_copy(dt_src, H5T_COPY_TRANSIENT)))
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to copy")
        if((tid_mem = H5I_register(H5I_DATATYPE, dt_mem, FALSE)) < 0) {
            H5T_close(dt_mem);
            HGOTO_ERROR(H5E_DATASET, H5E_CANTREGISTER, FAIL, "unable to register memory datatype")
        } /* end if */

        /* create variable-length datatype at the destinaton file */
        if(NULL == (dt_dst = H5T_copy(dt_src, H5T_COPY_TRANSIENT)))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to copy")
        if(H5T_set_loc(dt_dst, f_dst, H5T_LOC_DISK) < 0) {
            H5T_close(dt_dst);
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "cannot mark datatype on disk")
        } /* end if */
        if((tid_dst = H5I_register(H5I_DATATYPE, dt_dst, FALSE)) < 0) {
            H5T_close(dt_dst);
            HGOTO_ERROR(H5E_DATASET, H5E_CANTREGISTER, FAIL, "unable to register destination file datatype")
        } /* end if */

        /* Set up the conversion functions */
        if(NULL == (tpath_src_mem = H5T_path_find(dt_src, dt_mem, NULL, NULL, dxpl_id, FALSE)))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to convert between src and mem datatypes")
        if(NULL == (tpath_mem_dst = H5T_path_find(dt_mem, dt_dst, NULL, NULL, dxpl_id, FALSE)))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to convert between mem and dst datatypes")

        /* Determine largest datatype size */
        if(0 == (src_dt_size = H5T_get_size(dt_src)))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to determine datatype size")
        if(0 == (mem_dt_size = H5T_get_size(dt_mem)))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to determine datatype size")
        max_dt_size = MAX(src_dt_size, mem_dt_size);
        if(0 == (dst_dt_size = H5T_get_size(dt_dst)))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to determine datatype size")
        max_dt_size = MAX(max_dt_size, dst_dt_size);

        /* Set maximum number of whole elements that fit in buffer */
        if(0 == (nelmts = buf_size / max_dt_size))
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "element size too large")

        /* Set the number of bytes to transfer */
        src_nbytes = nelmts * src_dt_size;
        dst_nbytes = nelmts * dst_dt_size;
        mem_nbytes = nelmts * mem_dt_size;

        /* Adjust buffer size to be multiple of elements */
        buf_size = nelmts * max_dt_size;

        /* Create dataspace for number of elements in buffer */
        buf_dim[0] = nelmts;

        /* Create the space and set the initial extent */
        if(NULL == (buf_space = H5S_create_simple((unsigned)1, buf_dim, NULL)))
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCREATE, FAIL, "can't create simple dataspace")

        /* Atomize */
        if((buf_sid = H5I_register(H5I_DATASPACE, buf_space, FALSE)) < 0) {
            H5S_close(buf_space);
            HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register dataspace ID")
        } /* end if */

        /* Set flag to do type conversion */
        is_vlen = TRUE;
    } /* end if */
    else {
        /* Check for reference datatype */
        if(H5T_get_class(dt_src, FALSE) == H5T_REFERENCE) {
            /* Need to fix values of references when copying across files */
            if(f_src != f_dst)
                fix_ref = TRUE;
        } /* end if */

        /* Set the number of bytes to read & write to the buffer size */
        src_nbytes = dst_nbytes = mem_nbytes = buf_size;
    } /* end else */

    /* Allocate space for copy buffer */
    HDassert(buf_size);
    if(NULL == (buf = H5FL_BLK_MALLOC(type_conv, buf_size)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for copy buffer")

    /* Need extra buffer for datatype conversions, to prevent stranding/leaking memory */
    if(is_vlen || fix_ref) {
        if(NULL == (reclaim_buf = H5FL_BLK_MALLOC(type_conv, buf_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for copy buffer")

        /* allocate temporary bkg buff for data conversion */
        if(NULL == (bkg = H5FL_BLK_MALLOC(type_conv, buf_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for copy buffer")
    } /* end if */

    /* Loop over copying data */
    addr_src = storage_src->addr;
    addr_dst = storage_dst->addr;
    while(total_src_nbytes > 0) {
        /* Check if we should reduce the number of bytes to transfer */
        if(total_src_nbytes < src_nbytes) {
            /* Adjust bytes to transfer */
            src_nbytes = (size_t)total_src_nbytes;

            /* Adjust dataspace describing buffer */
            if(is_vlen) {
                /* Adjust destination & memory bytes to transfer */
                nelmts = src_nbytes / src_dt_size;
                dst_nbytes = nelmts * dst_dt_size;
                mem_nbytes = nelmts * mem_dt_size;

                /* Adjust size of buffer's dataspace dimension */
                buf_dim[0] = nelmts;

                /* Adjust size of buffer's dataspace */
                if(H5S_set_extent_real(buf_space, buf_dim) < 0)
                    HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSET, FAIL, "unable to change buffer dataspace size")
            } /* end if */
            else
                /* Adjust destination & memory bytes to transfer */
                dst_nbytes = mem_nbytes = src_nbytes;
        } /* end if */

        /* Read raw data from source file */
        if(H5F_block_read(f_src, H5FD_MEM_DRAW, addr_src, src_nbytes, H5P_DATASET_XFER_DEFAULT, buf) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "unable to read raw data")

        /* Perform datatype conversion, if necessary */
        if(is_vlen) {
            /* Convert from source file to memory */
	    if(H5T_convert(tpath_src_mem, tid_src, tid_mem, nelmts, (size_t)0, (size_t)0, buf, bkg, dxpl_id) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "datatype conversion failed")

            /* Copy into another buffer, to reclaim memory later */
            HDmemcpy(reclaim_buf, buf, mem_nbytes);

            /* Set background buffer to all zeros */
            HDmemset(bkg, 0, buf_size);

            /* Convert from memory to destination file */
	    if(H5T_convert(tpath_mem_dst, tid_mem, tid_dst, nelmts, (size_t)0, (size_t)0, buf, bkg, dxpl_id) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "datatype conversion failed")

            /* Reclaim space from variable length data */
            if(H5D_vlen_reclaim(tid_mem, buf_space, H5P_DATASET_XFER_DEFAULT, reclaim_buf) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_BADITER, FAIL, "unable to reclaim variable-length data")
	} /* end if */
        else if(fix_ref) {
            /* Check for expanding references */
            if(cpy_info->expand_ref) {
                size_t ref_count;

                /* Determine # of reference elements to copy */
                ref_count = src_nbytes / H5T_get_size(dt_src);

                /* Copy the reference elements */
                if(H5O_copy_expand_ref(f_src, buf, dxpl_id, f_dst, bkg, ref_count, H5T_get_ref_type(dt_src), cpy_info) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTCOPY, FAIL, "unable to copy reference attribute")

                /* After fix ref, copy the new reference elements to the buffer to write out */
                HDmemcpy(buf, bkg,  buf_size);
            } /* end if */
            else
                /* Reset value to zero */
                HDmemset(buf, 0, src_nbytes);
        } /* end if */

        /* Write raw data to destination file */
        if(H5F_block_write(f_dst, H5FD_MEM_DRAW, addr_dst, dst_nbytes, H5P_DATASET_XFER_DEFAULT, buf) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to write raw data")

        /* Adjust loop variables */
        addr_src += src_nbytes;
        addr_dst += dst_nbytes;
        total_src_nbytes -= src_nbytes;
    } /* end while */

done:
    if(buf_sid > 0 && H5I_dec_ref(buf_sid) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "can't decrement temporary dataspace ID")
    if(tid_src > 0 && H5I_dec_ref(tid_src) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't decrement temporary datatype ID")
    if(tid_dst > 0 && H5I_dec_ref(tid_dst) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't decrement temporary datatype ID")
    if(tid_mem > 0 && H5I_dec_ref(tid_mem) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't decrement temporary datatype ID")
    if(buf)
        buf = H5FL_BLK_FREE(type_conv, buf);
    if(reclaim_buf)
        reclaim_buf = H5FL_BLK_FREE(type_conv, reclaim_buf);
    if(bkg)
        bkg = H5FL_BLK_FREE(type_conv, bkg);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__contig_copy() */

