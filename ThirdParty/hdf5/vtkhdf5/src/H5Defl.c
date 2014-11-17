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
 * Programmer: Quincey Koziol <koziol@ncsa.uiuc.edu>
 *	       Thursday, September 30, 2004
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
#include "H5Fprivate.h"		/* Files				*/
#include "H5HLprivate.h"	/* Local Heaps				*/
#include "H5VMprivate.h"		/* Vector and array functions		*/


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/

/* Callback info for readvv operation */
typedef struct H5D_efl_readvv_ud_t {
    const H5O_efl_t *efl;       /* Pointer to efl info */
    unsigned char *rbuf;        /* Read buffer */
} H5D_efl_readvv_ud_t;

/* Callback info for writevv operation */
typedef struct H5D_efl_writevv_ud_t {
    const H5O_efl_t *efl;       /* Pointer to efl info */
    const unsigned char *wbuf;  /* Write buffer */
} H5D_efl_writevv_ud_t;


/********************/
/* Local Prototypes */
/********************/

/* Layout operation callbacks */
static herr_t H5D__efl_construct(H5F_t *f, H5D_t *dset);
static herr_t H5D__efl_io_init(const H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t nelmts, const H5S_t *file_space, const H5S_t *mem_space,
    H5D_chunk_map_t *cm);
static ssize_t H5D__efl_readvv(const H5D_io_info_t *io_info,
    size_t dset_max_nseq, size_t *dset_curr_seq, size_t dset_len_arr[], hsize_t dset_offset_arr[],
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_len_arr[], hsize_t mem_offset_arr[]);
static ssize_t H5D__efl_writevv(const H5D_io_info_t *io_info,
    size_t dset_max_nseq, size_t *dset_curr_seq, size_t dset_len_arr[], hsize_t dset_offset_arr[],
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_len_arr[], hsize_t mem_offset_arr[]);

/* Helper routines */
static herr_t H5D__efl_read(const H5O_efl_t *efl, haddr_t addr, size_t size,
    uint8_t *buf);
static herr_t H5D__efl_write(const H5O_efl_t *efl, haddr_t addr, size_t size,
    const uint8_t *buf);


/*********************/
/* Package Variables */
/*********************/

/* External File List (EFL) storage layout I/O ops */
const H5D_layout_ops_t H5D_LOPS_EFL[1] = {{
    H5D__efl_construct,
    NULL,
    H5D__efl_is_space_alloc,
    H5D__efl_io_init,
    H5D__contig_read,
    H5D__contig_write,
#ifdef H5_HAVE_PARALLEL
    NULL,
    NULL,
#endif /* H5_HAVE_PARALLEL */
    H5D__efl_readvv,
    H5D__efl_writevv,
    NULL,
    NULL
}};


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:	H5D__efl_construct
 *
 * Purpose:	Constructs new EFL layout information for dataset
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, May 22, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__efl_construct(H5F_t *f, H5D_t *dset)
{
    size_t dt_size;                     /* Size of datatype */
    hsize_t dim[H5O_LAYOUT_NDIMS];	/* Current size of data in elements */
    hsize_t max_dim[H5O_LAYOUT_NDIMS];  /* Maximum size of data in elements */
    hssize_t stmp_size;                 /* Temporary holder for raw data size */
    hsize_t tmp_size;                   /* Temporary holder for raw data size */
    hsize_t max_points;                 /* Maximum elements */
    hsize_t max_storage;                /* Maximum storage size */
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
    for(i = 1; i < ndims; i++)
        if(max_dim[i] > dim[i])
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "only the first dimension can be extendible")

    /* Retrieve the size of the dataset's datatype */
    if(0 == (dt_size = H5T_get_size(dset->shared->type)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to determine datatype size")

    /* Check for storage overflows */
    max_points = H5S_get_npoints_max(dset->shared->space);
    max_storage = H5O_efl_total_size(&dset->shared->dcpl_cache.efl);
    if(H5S_UNLIMITED == max_points) {
        if(H5O_EFL_UNLIMITED != max_storage)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unlimited dataspace but finite storage")
    } /* end if */
    else if((max_points * dt_size) < max_points)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "dataspace * type size overflowed")
    else if((max_points * dt_size) > max_storage)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "dataspace size exceeds external storage size")

    /* Compute the total size of dataset */
    stmp_size = H5S_GET_EXTENT_NPOINTS(dset->shared->space);
    HDassert(stmp_size >= 0);
    tmp_size = (hsize_t)stmp_size * dt_size;
    H5_ASSIGN_OVERFLOW(dset->shared->layout.storage.u.contig.size, tmp_size, hssize_t, hsize_t);

    /* Get the sieve buffer size for this dataset */
    dset->shared->cache.contig.sieve_buf_size = H5F_SIEVE_BUF_SIZE(f);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__efl_construct() */


/*-------------------------------------------------------------------------
 * Function:	H5D__efl_is_space_alloc
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
H5D__efl_is_space_alloc(const H5O_storage_t UNUSED *storage)
{
    FUNC_ENTER_PACKAGE_NOERR

    /* Sanity checks */
    HDassert(storage);

    /* EFL storage is currently always treated as allocated */
    FUNC_LEAVE_NOAPI(TRUE)
} /* end H5D__efl_is_space_alloc() */


/*-------------------------------------------------------------------------
 * Function:	H5D__efl_io_init
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
H5D__efl_io_init(const H5D_io_info_t *io_info, const H5D_type_info_t UNUSED *type_info,
    hsize_t UNUSED nelmts, const H5S_t UNUSED *file_space, const H5S_t UNUSED *mem_space,
    H5D_chunk_map_t UNUSED *cm)
{
    FUNC_ENTER_STATIC_NOERR

    HDmemcpy(&io_info->store->efl, &(io_info->dset->shared->dcpl_cache.efl), sizeof(H5O_efl_t));

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5D__efl_io_init() */


/*-------------------------------------------------------------------------
 * Function:	H5D__efl_read
 *
 * Purpose:	Reads data from an external file list.  It is an error to
 *		read past the logical end of file, but reading past the end
 *		of any particular member of the external file list results in
 *		zeros.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Wednesday, March  4, 1998
 *
 * Modifications:
 *		Robb Matzke, 1999-07-28
 *		The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__efl_read(const H5O_efl_t *efl, haddr_t addr, size_t size, uint8_t *buf)
{
    int		fd = -1;
    size_t	to_read;
#ifndef NDEBUG
    hsize_t     tempto_read;
#endif /* NDEBUG */
    hsize_t     skip = 0;
    haddr_t     cur;
    ssize_t	n;
    size_t      u;                      /* Local index variable */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    /* Check args */
    HDassert(efl && efl->nused > 0);
    HDassert(H5F_addr_defined(addr));
    HDassert(size < SIZET_MAX);
    HDassert(buf || 0 == size);

    /* Find the first efl member from which to read */
    for (u=0, cur=0; u<efl->nused; u++) {
	if(H5O_EFL_UNLIMITED == efl->slot[u].size || addr < cur + efl->slot[u].size) {
	    skip = addr - cur;
	    break;
	} /* end if */
  	cur += efl->slot[u].size;
    } /* end for */

    /* Read the data */
    while(size) {
        HDassert(buf);
	if(u >= efl->nused)
	    HGOTO_ERROR(H5E_EFL, H5E_OVERFLOW, FAIL, "read past logical end of file")
	if(H5F_OVERFLOW_HSIZET2OFFT(efl->slot[u].offset + skip))
	    HGOTO_ERROR(H5E_EFL, H5E_OVERFLOW, FAIL, "external file address overflowed")
	if((fd = HDopen(efl->slot[u].name, O_RDONLY, 0)) < 0)
	    HGOTO_ERROR(H5E_EFL, H5E_CANTOPENFILE, FAIL, "unable to open external raw data file")
	if(HDlseek(fd, (off_t)(efl->slot[u].offset + skip), SEEK_SET) < 0)
	    HGOTO_ERROR(H5E_EFL, H5E_SEEKERROR, FAIL, "unable to seek in external raw data file")
#ifndef NDEBUG
	tempto_read = MIN(efl->slot[u].size-skip, (hsize_t)size);
        H5_CHECK_OVERFLOW(tempto_read, hsize_t, size_t);
	to_read = (size_t)tempto_read;
#else /* NDEBUG */
	to_read = MIN((size_t)(efl->slot[u].size - skip), size);
#endif /* NDEBUG */
	if((n = HDread(fd, buf, to_read)) < 0)
	    HGOTO_ERROR(H5E_EFL, H5E_READERROR, FAIL, "read error in external raw data file")
	else if((size_t)n < to_read)
	    HDmemset(buf + n, 0, to_read - (size_t)n);
	HDclose(fd);
	fd = -1;
	size -= to_read;
	buf += to_read;
	skip = 0;
	u++;
    } /* end while */

done:
    if(fd >= 0)
        HDclose(fd);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__efl_read() */


/*-------------------------------------------------------------------------
 * Function:	H5D__efl_write
 *
 * Purpose:	Writes data to an external file list.  It is an error to
 *		write past the logical end of file, but writing past the end
 *		of any particular member of the external file list just
 *		extends that file.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Wednesday, March  4, 1998
 *
 * Modifications:
 *		Robb Matzke, 1999-07-28
 *		The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__efl_write(const H5O_efl_t *efl, haddr_t addr, size_t size, const uint8_t *buf)
{
    int		fd = -1;
    size_t	to_write;
#ifndef NDEBUG
    hsize_t	tempto_write;
#endif /* NDEBUG */
    haddr_t     cur;
    hsize_t     skip = 0;
    size_t	u;                      /* Local index variable */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    /* Check args */
    HDassert(efl && efl->nused > 0);
    HDassert(H5F_addr_defined(addr));
    HDassert(size < SIZET_MAX);
    HDassert(buf || 0 == size);

    /* Find the first efl member in which to write */
    for(u = 0, cur = 0; u < efl->nused; u++) {
	if(H5O_EFL_UNLIMITED == efl->slot[u].size || addr < cur + efl->slot[u].size) {
	    skip = addr - cur;
	    break;
	} /* end if */
	cur += efl->slot[u].size;
    } /* end for */

    /* Write the data */
    while(size) {
        HDassert(buf);
	if(u >= efl->nused)
	    HGOTO_ERROR(H5E_EFL, H5E_OVERFLOW, FAIL, "write past logical end of file")
	if(H5F_OVERFLOW_HSIZET2OFFT(efl->slot[u].offset + skip))
	    HGOTO_ERROR(H5E_EFL, H5E_OVERFLOW, FAIL, "external file address overflowed")
	if((fd = HDopen(efl->slot[u].name, O_CREAT | O_RDWR, 0666)) < 0) {
	    if(HDaccess(efl->slot[u].name, F_OK) < 0)
		HGOTO_ERROR(H5E_EFL, H5E_CANTOPENFILE, FAIL, "external raw data file does not exist")
	    else
		HGOTO_ERROR(H5E_EFL, H5E_CANTOPENFILE, FAIL, "unable to open external raw data file")
	} /* end if */
	if(HDlseek(fd, (off_t)(efl->slot[u].offset + skip), SEEK_SET) < 0)
	    HGOTO_ERROR(H5E_EFL, H5E_SEEKERROR, FAIL, "unable to seek in external raw data file")
#ifndef NDEBUG
	tempto_write = MIN(efl->slot[u].size - skip, (hsize_t)size);
        H5_CHECK_OVERFLOW(tempto_write, hsize_t, size_t);
        to_write = (size_t)tempto_write;
#else /* NDEBUG */
	to_write = MIN((size_t)(efl->slot[u].size - skip), size);
#endif /* NDEBUG */
	if((size_t)HDwrite(fd, buf, to_write) != to_write)
	    HGOTO_ERROR(H5E_EFL, H5E_READERROR, FAIL, "write error in external raw data file")
	HDclose (fd);
	fd = -1;
	size -= to_write;
	buf += to_write;
	skip = 0;
	u++;
    } /* end while */

done:
    if(fd >= 0)
        HDclose(fd);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__efl_write() */


/*-------------------------------------------------------------------------
 * Function:	H5D__efl_readvv_cb
 *
 * Purpose:	Callback operator for H5D__efl_readvv().
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, Sept 30, 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__efl_readvv_cb(hsize_t dst_off, hsize_t src_off, size_t len, void *_udata)
{
    H5D_efl_readvv_ud_t *udata = (H5D_efl_readvv_ud_t *)_udata; /* User data for H5VM_opvv() operator */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

    /* Read data */
    if(H5D__efl_read(udata->efl, dst_off, len, (udata->rbuf + src_off)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "EFL read failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__efl_readvv_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D__efl_readvv
 *
 * Purpose:	Reads data from an external file list.  It is an error to
 *		read past the logical end of file, but reading past the end
 *		of any particular member of the external file list results in
 *		zeros.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Wednesday, May  7, 2003
 *
 *-------------------------------------------------------------------------
 */
static ssize_t
H5D__efl_readvv(const H5D_io_info_t *io_info,
    size_t dset_max_nseq, size_t *dset_curr_seq, size_t dset_len_arr[], hsize_t dset_off_arr[],
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_len_arr[], hsize_t mem_off_arr[])
{
    H5D_efl_readvv_ud_t udata;  /* User data for H5VM_opvv() operator */
    ssize_t ret_value;          /* Return value (Total size of sequence in bytes) */

    FUNC_ENTER_STATIC

    /* Check args */
    HDassert(io_info);
    HDassert(io_info->store->efl.nused > 0);
    HDassert(io_info->u.rbuf);
    HDassert(dset_curr_seq);
    HDassert(dset_len_arr);
    HDassert(dset_off_arr);
    HDassert(mem_curr_seq);
    HDassert(mem_len_arr);
    HDassert(mem_off_arr);

    /* Set up user data for H5VM_opvv() */
    udata.efl = &(io_info->store->efl);
    udata.rbuf = (unsigned char *)io_info->u.rbuf;

    /* Call generic sequence operation routine */
    if((ret_value = H5VM_opvv(dset_max_nseq, dset_curr_seq, dset_len_arr, dset_off_arr,
            mem_max_nseq, mem_curr_seq, mem_len_arr, mem_off_arr,
            H5D__efl_readvv_cb, &udata)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTOPERATE, FAIL, "can't perform vectorized EFL read")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__efl_readvv() */


/*-------------------------------------------------------------------------
 * Function:	H5D__efl_writevv_cb
 *
 * Purpose:	Callback operator for H5D__efl_writevv().
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, Sept 30, 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__efl_writevv_cb(hsize_t dst_off, hsize_t src_off, size_t len, void *_udata)
{
    H5D_efl_writevv_ud_t *udata = (H5D_efl_writevv_ud_t *)_udata; /* User data for H5VM_opvv() operator */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

    /* Write data */
    if(H5D__efl_write(udata->efl, dst_off, len, (udata->wbuf + src_off)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "EFL write failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__efl_writevv_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D__efl_writevv
 *
 * Purpose:	Writes data to an external file list.  It is an error to
 *		write past the logical end of file, but writing past the end
 *		of any particular member of the external file list just
 *		extends that file.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Friday, May  2, 2003
 *
 *-------------------------------------------------------------------------
 */
static ssize_t
H5D__efl_writevv(const H5D_io_info_t *io_info,
    size_t dset_max_nseq, size_t *dset_curr_seq, size_t dset_len_arr[], hsize_t dset_off_arr[],
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_len_arr[], hsize_t mem_off_arr[])
{
    H5D_efl_writevv_ud_t udata;  /* User data for H5VM_opvv() operator */
    ssize_t ret_value;          /* Return value (Total size of sequence in bytes) */

    FUNC_ENTER_STATIC

    /* Check args */
    HDassert(io_info);
    HDassert(io_info->store->efl.nused > 0);
    HDassert(io_info->u.wbuf);
    HDassert(dset_curr_seq);
    HDassert(dset_len_arr);
    HDassert(dset_off_arr);
    HDassert(mem_curr_seq);
    HDassert(mem_len_arr);
    HDassert(mem_off_arr);

    /* Set up user data for H5VM_opvv() */
    udata.efl = &(io_info->store->efl);
    udata.wbuf = (const unsigned char *)io_info->u.wbuf;

    /* Call generic sequence operation routine */
    if((ret_value = H5VM_opvv(dset_max_nseq, dset_curr_seq, dset_len_arr, dset_off_arr,
            mem_max_nseq, mem_curr_seq, mem_len_arr, mem_off_arr,
            H5D__efl_writevv_cb, &udata)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTOPERATE, FAIL, "can't perform vectorized EFL write")
done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__efl_writevv() */


/*-------------------------------------------------------------------------
 * Function:    H5D__efl_bh_info
 *
 * Purpose:     Retrieve the amount of heap storage used for External File
 *		List message
 *
 * Return:      Success:        Non-negative
 *              Failure:        negative
 *
 * Programmer:  Vailin Choi; August 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__efl_bh_info(H5F_t *f, hid_t dxpl_id, H5O_efl_t *efl, hsize_t *heap_size)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(f);
    HDassert(efl);
    HDassert(H5F_addr_defined(efl->heap_addr));
    HDassert(heap_size);

    /* Get the size of the local heap for EFL's file list */
    if(H5HL_heapsize(f, dxpl_id, efl->heap_addr, heap_size) < 0)
        HGOTO_ERROR(H5E_EFL, H5E_CANTINIT, FAIL, "unable to retrieve local heap info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__efl_bh_info() */
