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

/* Programmer: 	Vailin Choi <vchoi@hdfgroup.org>
 *	       	September 2010
 *
 * Purpose:	Implicit (Non Index) chunked I/O functions.  
 *		This is used when the dataset is:
 *			extendible but with fixed max. dims
 *			with early allocation
 *			without filter
 *		The chunk coordinate is mapped into the actual disk addresses
 *		for the chunk without indexing.
 *
 */

/****************/
/* Module Setup */
/****************/

#include "H5Dmodule.h"          /* This source code file is part of the H5D module */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Dpkg.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free Lists                           */
#include "H5MFprivate.h"	/* File space management		*/
#include "H5VMprivate.h"         /* Vector functions			*/


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Local Prototypes */
/********************/

/* Non Index chunking I/O ops */
static herr_t H5D__none_idx_create(const H5D_chk_idx_info_t *idx_info);
static hbool_t H5D__none_idx_is_space_alloc(const H5O_storage_chunk_t *storage);
static herr_t H5D__none_idx_get_addr(const H5D_chk_idx_info_t *idx_info,
    H5D_chunk_ud_t *udata);
static int H5D__none_idx_iterate(const H5D_chk_idx_info_t *idx_info,
    H5D_chunk_cb_func_t chunk_cb, void *chunk_udata);
static herr_t H5D__none_idx_remove(const H5D_chk_idx_info_t *idx_info,
    H5D_chunk_common_ud_t *udata);
static herr_t H5D__none_idx_delete(const H5D_chk_idx_info_t *idx_info);
static herr_t H5D__none_idx_copy_setup(const H5D_chk_idx_info_t *idx_info_src,
    const H5D_chk_idx_info_t *idx_info_dst);
static herr_t H5D__none_idx_size(const H5D_chk_idx_info_t *idx_info,
    hsize_t *size);
static herr_t H5D__none_idx_reset(H5O_storage_chunk_t *storage, hbool_t reset_addr);
static herr_t H5D__none_idx_dump(const H5O_storage_chunk_t *storage, FILE *stream);


/*********************/
/* Package Variables */
/*********************/

/* Non Index chunk I/O ops */
const H5D_chunk_ops_t H5D_COPS_NONE[1] = {{
    FALSE,                      	/* Non-indexed chunking don't current support SWMR access */
    NULL,				/* init */
    H5D__none_idx_create,		/* create */
    H5D__none_idx_is_space_alloc, 	/* is_space_alloc */
    NULL,				/* insert */
    H5D__none_idx_get_addr,		/* get_addr */
    NULL,				/* resize */
    H5D__none_idx_iterate,		/* iterate */
    H5D__none_idx_remove,		/* remove */
    H5D__none_idx_delete,		/* delete */
    H5D__none_idx_copy_setup,		/* copy_setup */
    NULL,				/* copy_shutdown */
    H5D__none_idx_size,			/* size */
    H5D__none_idx_reset,		/* reset */
    H5D__none_idx_dump,			/* dump */
    NULL				/* dest */
}};


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:	H5D__none_idx_create
 *
 * Purpose:	Allocate memory for the maximum # of chunks in the dataset.
 *		
 * Return:	Non-negative on success 
 *		Negative on failure.
 *
 * Programmer:	Vailin Choi; September 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__none_idx_create(const H5D_chk_idx_info_t *idx_info)
{
    hsize_t 	nbytes;                 /* Total size of dataset chunks */
    haddr_t	addr;			/* The address of dataset chunks */
    herr_t 	ret_value = SUCCEED; 	/* Return value */

    FUNC_ENTER_STATIC

    /* Check args */
    HDassert(idx_info);
    HDassert(idx_info->f);
    HDassert(idx_info->pline);
    HDassert(idx_info->pline->nused == 0); /* Shouldn't have filter defined on entering here */
    HDassert(idx_info->layout);
    HDassert(idx_info->storage);
    HDassert(idx_info->layout->max_nchunks);
    HDassert(!H5F_addr_defined(idx_info->storage->idx_addr));	/* address of data shouldn't be defined */

    /* Calculate size of max dataset chunks */
    nbytes = idx_info->layout->max_nchunks * idx_info->layout->size;

    /* Allocate space for max dataset chunks */
    addr = H5MF_alloc(idx_info->f, H5FD_MEM_DRAW, nbytes);
    if(!H5F_addr_defined(addr))
	HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "file allocation failed")

    /* This is the address of the dataset chunks */
    idx_info->storage->idx_addr = addr;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__none_idx_create() */


/*-------------------------------------------------------------------------
 * Function:	H5D__none_idx_is_space_alloc
 *
 * Purpose:	Query if space for the dataset chunks is allocated
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Vailin Choi; September 2010
 *
 *-------------------------------------------------------------------------
 */
static hbool_t
H5D__none_idx_is_space_alloc(const H5O_storage_chunk_t *storage)
{
    FUNC_ENTER_STATIC_NOERR

    /* Check args */
    HDassert(storage);

    FUNC_LEAVE_NOAPI((hbool_t)H5F_addr_defined(storage->idx_addr))
} /* end H5D__none_idx_is_space_alloc() */


/*-------------------------------------------------------------------------
 * Function:	H5D__none_idx_get_addr
 *
 * Purpose:	Get the file address of a chunk.
 *		Save the retrieved information in the udata supplied.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Vailin Choi; Sept 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__none_idx_get_addr(const H5D_chk_idx_info_t *idx_info, H5D_chunk_ud_t *udata)
{
    FUNC_ENTER_STATIC_NOERR

    /* Sanity checks */
    HDassert(idx_info);
    HDassert(idx_info->f);
    HDassert(idx_info->pline);
    HDassert(idx_info->pline->nused == 0);
    HDassert(idx_info->layout);
    HDassert(idx_info->storage);
    HDassert(udata);
    HDassert(H5F_addr_defined(idx_info->storage->idx_addr));

    /* Calculate the index of this chunk */
    udata->chunk_idx = H5VM_array_offset_pre((idx_info->layout->ndims - 1), idx_info->layout->max_down_chunks, udata->common.scaled);

    /* Calculate the address of the chunk */
    udata->chunk_block.offset = idx_info->storage->idx_addr + udata->chunk_idx * idx_info->layout->size;

    /* Update the other (constant) information for the chunk */
    udata->chunk_block.length = idx_info->layout->size;
    udata->filter_mask = 0;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5D__none_idx_get_addr() */


/*-------------------------------------------------------------------------
 * Function:	H5D__none_idx_iterate
 *
 * Purpose:	Iterate over the chunks in an index, making a callback
 *              for each one.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Vailin Choi; September 2010
 *
 *-------------------------------------------------------------------------
 */
static int
H5D__none_idx_iterate(const H5D_chk_idx_info_t *idx_info,
    H5D_chunk_cb_func_t chunk_cb, void *chunk_udata)
{
    H5D_chunk_rec_t chunk_rec;			/* generic chunk record  */
    unsigned ndims; 	/* Rank of chunk */
    unsigned u;		/* Local index variable */
    int curr_dim;       /* Current rank */
    hsize_t idx;    	/* Array index of chunk */
    int ret_value = H5_ITER_CONT; /* Return value */

    FUNC_ENTER_STATIC_NOERR

    /* Sanity checks */
    HDassert(idx_info);
    HDassert(idx_info->f);
    HDassert(idx_info->pline);
    HDassert(!idx_info->pline->nused);
    HDassert(idx_info->layout);
    HDassert(idx_info->storage);
    HDassert(chunk_cb);
    HDassert(chunk_udata);
    HDassert(H5F_addr_defined(idx_info->storage->idx_addr));

    /* Initialize generic chunk record */
    HDmemset(&chunk_rec, 0, sizeof(chunk_rec));
    chunk_rec.nbytes = idx_info->layout->size;
    chunk_rec.filter_mask = 0;

    ndims = idx_info->layout->ndims - 1;
    HDassert(ndims > 0);

    /* Iterate over all the chunks in the dataset's dataspace */
    for(u = 0; u < idx_info->layout->nchunks && ret_value == H5_ITER_CONT; u++) {
	/* Calculate the index of this chunk */
	idx = H5VM_array_offset_pre(ndims, idx_info->layout->max_down_chunks, chunk_rec.scaled);

	/* Calculate the address of the chunk */
	chunk_rec.chunk_addr = idx_info->storage->idx_addr + idx * idx_info->layout->size;

	/* Make "generic chunk" callback */
	if((ret_value = (*chunk_cb)(&chunk_rec, chunk_udata)) < 0)
	    HERROR(H5E_DATASET, H5E_CALLBACK, "failure in generic chunk iterator callback");

	/* Update coordinates of chunk in dataset */
	curr_dim = (int)(ndims - 1);
	while(curr_dim >= 0) {
	    /* Increment coordinate in current dimension */
	    chunk_rec.scaled[curr_dim]++;

	    /* Check if we went off the end of the current dimension */
	    if(chunk_rec.scaled[curr_dim] >= idx_info->layout->chunks[curr_dim]) {
		/* Reset coordinate & move to next faster dimension */
		chunk_rec.scaled[curr_dim] = 0;
		curr_dim--;
	    } /* end if */
	    else
		break;
	} /* end while */
    } /* end for */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__none_idx_iterate() */


/*-------------------------------------------------------------------------
 * Function:	H5D__none_idx_remove
 *
 * Purpose:	Remove chunk from index.
 *
 * Note:	Chunks can't be removed (or added) to datasets with this
 *		form of index - all the space for all the chunks is always
 *		allocated in the file.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Vailin Choi; Sept 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__none_idx_remove(const H5D_chk_idx_info_t H5_ATTR_UNUSED *idx_info, H5D_chunk_common_ud_t H5_ATTR_UNUSED *udata)
{
    FUNC_ENTER_STATIC_NOERR

    /* NO OP */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5D__none_idx_remove() */


/*-------------------------------------------------------------------------
 * Function:	H5D__none_idx_delete
 *
 * Purpose:	Delete raw data storage for entire dataset (i.e. all chunks)
 *
 * Return:	Success:	Non-negative
 *		Failure:	negative
 *
 * Programmer:	Vailin Choi; Sept 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__none_idx_delete(const H5D_chk_idx_info_t *idx_info)
{
    hsize_t nbytes;                 /* Size of all chunks */
    herr_t ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity checks */
    HDassert(idx_info);
    HDassert(idx_info->f);
    HDassert(idx_info->pline);
    HDassert(!idx_info->pline->nused); /* Shouldn't have filter defined on entering here */
    HDassert(idx_info->layout);
    HDassert(idx_info->storage);
    HDassert(H5F_addr_defined(idx_info->storage->idx_addr));  /* should be defined */

    /* chunk size * max # of chunks */
    nbytes = idx_info->layout->max_nchunks * idx_info->layout->size;
    if(H5MF_xfree(idx_info->f, H5FD_MEM_DRAW, idx_info->storage->idx_addr, nbytes) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, H5_ITER_ERROR, "unable to free dataset chunks")

    idx_info->storage->idx_addr = HADDR_UNDEF;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__none_idx_delete() */


/*-------------------------------------------------------------------------
 * Function:	H5D__none_idx_copy_setup
 *
 * Purpose:	Set up any necessary information for copying chunks
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Vailin Choi; Sept 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__none_idx_copy_setup(const H5D_chk_idx_info_t *idx_info_src,
    const H5D_chk_idx_info_t *idx_info_dst)
{
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    /* Check args */
    HDassert(idx_info_src);
    HDassert(idx_info_src->f);
    HDassert(idx_info_src->pline);
    HDassert(!idx_info_src->pline->nused);
    HDassert(idx_info_src->layout);
    HDassert(idx_info_src->storage);
    HDassert(H5F_addr_defined(idx_info_src->storage->idx_addr));

    HDassert(idx_info_dst);
    HDassert(idx_info_dst->f);
    HDassert(idx_info_dst->pline);
    HDassert(!idx_info_dst->pline->nused);
    HDassert(idx_info_dst->layout);
    HDassert(idx_info_dst->storage);

    /* Set copied metadata tag */
    H5_BEGIN_TAG(H5AC__COPIED_TAG);

    /* Allocate dataset chunks in the dest. file */
    if(H5D__none_idx_create(idx_info_dst) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize chunked storage")

    /* Reset metadata tag */
    H5_END_TAG

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__none_idx_copy_setup() */


/*-------------------------------------------------------------------------
 * Function:    H5D__none_idx_size
 *
 * Purpose:     Retrieve the amount of index storage for chunked dataset
 *
 * Return:      Success:        Non-negative
 *              Failure:        negative
 *
 * Programmer:	Vailin Choi; Sept 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__none_idx_size(const H5D_chk_idx_info_t H5_ATTR_UNUSED *idx_info, hsize_t *index_size)
{
    FUNC_ENTER_STATIC_NOERR

    /* Check args */
    HDassert(index_size);

    *index_size = 0;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5D__none_idx_size() */


/*-------------------------------------------------------------------------
 * Function:	H5D__none_idx_reset
 *
 * Purpose:	Reset indexing information.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Vailin Choi; Sept 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__none_idx_reset(H5O_storage_chunk_t *storage, hbool_t reset_addr)
{
    FUNC_ENTER_STATIC_NOERR

    /* Check args */
    HDassert(storage);

    /* Reset index info */
    if(reset_addr)
	storage->idx_addr = HADDR_UNDEF;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5D__none_idx_reset() */


/*-------------------------------------------------------------------------
 * Function:	H5D__none_idx_dump
 *
 * Purpose:	Dump 
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Vailin Choi; September 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__none_idx_dump(const H5O_storage_chunk_t *storage, FILE *stream)
{
    FUNC_ENTER_STATIC_NOERR

    /* Check args */
    HDassert(storage);
    HDassert(stream);

    HDfprintf(stream, "    Address: %a\n", storage->idx_addr);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5D__none_idx_dump() */

