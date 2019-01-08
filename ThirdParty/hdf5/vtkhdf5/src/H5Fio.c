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

/*-------------------------------------------------------------------------
 *
 * Created:             H5Fio.c
 *                      Jan 10 2008
 *                      Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:             File I/O routines.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#include "H5Fmodule.h"          /* This source code file is part of the H5F module */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fpkg.h"             /* File access				*/
#include "H5FDprivate.h"	/* File drivers				*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5PBprivate.h"	/* Page Buffer				*/


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/


/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:	H5F_block_read
 *
 * Purpose:	Reads some data from a file/server/etc into a buffer.
 *		The data is contiguous.	 The address is relative to the base
 *		address for the file.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jul 10 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_block_read(H5F_t *f, H5FD_mem_t type, haddr_t addr, size_t size,
    void *buf/*out*/)
{
    H5FD_mem_t  map_type;               /* Mapped memory type */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(f);
    HDassert(f->shared);
    HDassert(buf);
    HDassert(H5F_addr_defined(addr));

    /* Check for attempting I/O on 'temporary' file address */
    if(H5F_addr_le(f->shared->tmp_addr, (addr + size)))
        HGOTO_ERROR(H5E_IO, H5E_BADRANGE, FAIL, "attempting I/O in temporary file space")

    /* Treat global heap as raw data */
    map_type = (type == H5FD_MEM_GHEAP) ? H5FD_MEM_DRAW : type;

    /* Pass through page buffer layer */
    if(H5PB_read(f, map_type, addr, size, buf) < 0)
        HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "read through page buffer failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_block_read() */


/*-------------------------------------------------------------------------
 * Function:	H5F_block_write
 *
 * Purpose:	Writes some data from memory to a file/server/etc.  The
 *		data is contiguous.  The address is relative to the base
 *		address.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jul 10 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_block_write(H5F_t *f, H5FD_mem_t type, haddr_t addr, size_t size,
    const void *buf)
{
    H5FD_mem_t  map_type;               /* Mapped memory type */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(f);
    HDassert(f->shared);
    HDassert(H5F_INTENT(f) & H5F_ACC_RDWR);
    HDassert(buf);
    HDassert(H5F_addr_defined(addr));

    /* Check for attempting I/O on 'temporary' file address */
    if(H5F_addr_le(f->shared->tmp_addr, (addr + size)))
        HGOTO_ERROR(H5E_IO, H5E_BADRANGE, FAIL, "attempting I/O in temporary file space")

    /* Treat global heap as raw data */
    map_type = (type == H5FD_MEM_GHEAP) ? H5FD_MEM_DRAW : type;

    /* Pass through page buffer layer */
    if(H5PB_write(f, map_type, addr, size, buf) < 0)
        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "write through page buffer failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_block_write() */


/*-------------------------------------------------------------------------
 * Function:    H5F_flush_tagged_metadata
 *
 * Purpose:     Flushes metadata with specified tag in the metadata cache 
 *              to disk.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:	Mike McGreevy
 *              September 9, 2010
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_flush_tagged_metadata(H5F_t *f, haddr_t tag)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    /* Use tag to search for and flush associated metadata */
    if(H5AC_flush_tagged_metadata(f, tag) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "unable to flush tagged metadata")

    /* Flush and reset the accumulator */
    if(H5F__accum_reset(f, TRUE) < 0)
        HGOTO_ERROR(H5E_IO, H5E_CANTRESET, FAIL, "can't reset accumulator")

    /* Flush file buffers to disk. */
    if(H5FD_flush(f->shared->lf, FALSE) < 0)
        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "low level flush failed")

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5F_flush_tagged_metadata */


/*-------------------------------------------------------------------------
 * Function:    H5F_evict_tagged_metadata
 *
 * Purpose:     Evicts metadata from the cache with specified tag.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:	Mike McGreevy
 *              September 9, 2010
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_evict_tagged_metadata(H5F_t *f, haddr_t tag)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    /* Evict the object's metadata */
    if(H5AC_evict_tagged_metadata(f, tag, TRUE) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTEXPUNGE, FAIL, "unable to evict tagged metadata")

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5F_evict_tagged_metadata */


/*-------------------------------------------------------------------------
 * Function:    H5F__evict_cache_entries
 *
 * Purpose:     To evict all cache entries except the pinned superblock entry
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Vailin Choi
 *		Dec 2013
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F__evict_cache_entries(H5F_t *f)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    HDassert(f);
    HDassert(f->shared);

    /* Evict all except pinned entries in the cache */
    if(H5AC_evict(f) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTEXPUNGE, FAIL, "unable to evict all except pinned entries")

#ifndef NDEBUG
{
    unsigned status = 0;
    uint32_t cur_num_entries;

    /* Retrieve status of the superblock */
    if(H5AC_get_entry_status(f, (haddr_t)0, &status) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTGET, FAIL, "unable to get entry status")

    /* Verify status of the superblock entry in the cache */
    if(!(status & H5AC_ES__IN_CACHE) || !(status & H5AC_ES__IS_PINNED))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTGET, FAIL, "unable to get entry status")

    /* Get the number of cache entries */
    if(H5AC_get_cache_size(f->shared->cache, NULL, NULL, NULL, &cur_num_entries) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5AC_get_cache_size() failed.")

    /* Should be the only one left in the cache (the superblock) */
    if(cur_num_entries != 1)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "number of cache entries is not correct")
}
#endif /* NDEBUG */

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5F__evict_cache_entries() */


/*-------------------------------------------------------------------------
 * Function:    H5F_get_checksums
 *
 * Purpose:   	Decode checksum stored in the buffer
 *		Calculate checksum for the data in the buffer
 *
 * Note:	Assumes that the checksum is the last data in the buffer
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:	Vailin Choi
 *		Sept 2013
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_get_checksums(const uint8_t *buf, size_t buf_size, uint32_t *s_chksum/*out*/, uint32_t *c_chksum/*out*/)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check arguments */
    HDassert(buf);
    HDassert(buf_size);

    /* Return the stored checksum */
    if(s_chksum) {
        const uint8_t *chk_p;      	/* Pointer into raw data buffer */

        /* Offset to the checksum in the buffer */
        chk_p = buf + buf_size - H5_SIZEOF_CHKSUM;

        /* Decode the checksum stored in the buffer */
        UINT32DECODE(chk_p, *s_chksum);
    } /* end if */

    /* Return the computed checksum for the buffer */
    if(c_chksum)
	*c_chksum = H5_checksum_metadata(buf, buf_size - H5_SIZEOF_CHKSUM, 0);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5F_get_chksums() */

