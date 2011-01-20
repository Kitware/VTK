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

/* Programmer: 	Quincey Koziol <koziol@hdfgroup.org>
 *	       	Thursday, April 24, 2008
 *
 * Purpose:	Abstract indexed (chunked) I/O functions.  The logical
 *		multi-dimensional data space is regularly partitioned into
 *		same-sized "chunks", the first of which is aligned with the
 *		logical origin.  The chunks are indexed by different methods,
 *		that map a chunk index to disk address.  Each chunk can be
 *              compressed independently and the chunks may move around in the
 *              file as their storage requirements change.
 *
 * Cache:	Disk I/O is performed in units of chunks and H5MF_alloc()
 *		contains code to optionally align chunks on disk block
 *		boundaries for performance.
 *
 *		The chunk cache is an extendible hash indexed by a function
 *		of storage B-tree address and chunk N-dimensional offset
 *		within the dataset.  Collisions are not resolved -- one of
 *		the two chunks competing for the hash slot must be preempted
 *		from the cache.  All entries in the hash also participate in
 *		a doubly-linked list and entries are penalized by moving them
 *		toward the front of the list.  When a new chunk is about to
 *		be added to the cache the heap is pruned by preempting
 *		entries near the front of the list to make room for the new
 *		entry which is added to the end of the list.
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
#include "H5FLprivate.h"	/* Free Lists                           */
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Vprivate.h"		/* Vector and array functions		*/


/****************/
/* Local Macros */
/****************/

/* Macros for iterating over chunks to operate on */
#define H5D_CHUNK_GET_FIRST_NODE(map) (map->use_single ? (H5SL_node_t *)(1) : H5SL_first(map->sel_chunks))
#define H5D_CHUNK_GET_NODE_INFO(map, node)  (map->use_single ? map->single_chunk_info : (H5D_chunk_info_t *)H5SL_item(node))
#define H5D_CHUNK_GET_NEXT_NODE(map, node)  (map->use_single ? (H5SL_node_t *)NULL : H5SL_next(node))

/*
 * Feature: If this constant is defined then every cache preemption and load
 *	    causes a character to be printed on the standard error stream:
 *
 *     `.': Entry was preempted because it has been completely read or
 *	    completely written but not partially read and not partially
 *	    written. This is often a good reason for preemption because such
 *	    a chunk will be unlikely to be referenced in the near future.
 *
 *     `:': Entry was preempted because it hasn't been used recently.
 *
 *     `#': Entry was preempted because another chunk collided with it. This
 *	    is usually a relatively bad thing.  If there are too many of
 *	    these then the number of entries in the cache can be increased.
 *
 *       c: Entry was preempted because the file is closing.
 *
 *	 w: A chunk read operation was eliminated because the library is
 *	    about to write new values to the entire chunk.  This is a good
 *	    thing, especially on files where the chunk size is the same as
 *	    the disk block size, chunks are aligned on disk block boundaries,
 *	    and the operating system can also eliminate a read operation.
 */

/*#define H5D_CHUNK_DEBUG */


/******************/
/* Local Typedefs */
/******************/

/* Callback info for iteration to prune chunks */
typedef struct H5D_chunk_it_ud1_t {
    H5D_chunk_common_ud_t common;       /* Common info for B-tree user data (must be first) */
    const H5D_chk_idx_info_t *idx_info; /* Chunked index info */
    const H5D_io_info_t *io_info;       /* I/O info for dataset operation */
    const hsize_t	*space_dim;	/* New dataset dimensions	*/
    const hbool_t       *shrunk_dim;   /* Dimensions which have been shrunk */
    H5S_t               *chunk_space;   /* Dataspace for a chunk */
    uint32_t            elmts_per_chunk;/* Elements in chunk */
    hsize_t             *hyper_start;   /* Starting location of hyperslab */
    H5D_fill_buf_info_t fb_info;        /* Dataset's fill buffer info */
    hbool_t             fb_info_init;   /* Whether the fill value buffer has been initialized */
} H5D_chunk_it_ud1_t;

/* Callback info for iteration to obtain chunk address and the index of the chunk for all chunks in the B-tree. */
typedef struct H5D_chunk_it_ud2_t {
    /* down */
    H5D_chunk_common_ud_t common;               /* Common info for B-tree user data (must be first) */

    /* up */
    haddr_t             *chunk_addr;            /* Array of chunk addresses to fill in */
} H5D_chunk_it_ud2_t;

/* Callback info for iteration to copy data */
typedef struct H5D_chunk_it_ud3_t {
    H5D_chunk_common_ud_t common;           /* Common info for B-tree user data (must be first) */
    H5F_t               *file_src;              /* Source file for copy */
    H5D_chk_idx_info_t  *idx_info_dst;          /* Dest. chunk index info object */
    void                *buf;                   /* Buffer to hold chunk data for read/write */
    void                *bkg;                   /* Buffer for background information during type conversion */
    size_t              buf_size;               /* Buffer size */
    hbool_t             do_convert;             /* Whether to perform type conversions */

    /* needed for converting variable-length data */
    hid_t               tid_src;                /* Datatype ID for source datatype */
    hid_t               tid_dst;                /* Datatype ID for destination datatype */
    hid_t               tid_mem;                /* Datatype ID for memory datatype */
    const H5T_t         *dt_src;                /* Source datatype */
    H5T_path_t          *tpath_src_mem;         /* Datatype conversion path from source file to memory */
    H5T_path_t          *tpath_mem_dst;         /* Datatype conversion path from memory to dest. file */
    void                *reclaim_buf;           /* Buffer for reclaiming data */
    size_t              reclaim_buf_size;       /* Reclaim buffer size */
    uint32_t            nelmts;                 /* Number of elements in buffer */
    H5S_t               *buf_space;             /* Dataspace describing buffer */

    /* needed for compressed variable-length data */
    const H5O_pline_t   *pline;                 /* Filter pipeline */

    /* needed for copy object pointed by refs */
    H5O_copy_t          *cpy_info;              /* Copy options */
} H5D_chunk_it_ud3_t;

/* Callback info for iteration to dump index */
typedef struct H5D_chunk_it_ud4_t {
    FILE		*stream;		/* Output stream	*/
    hbool_t             header_displayed;       /* Node's header is displayed? */
    unsigned            ndims;                  /* Number of dimensions for chunk/dataset */
} H5D_chunk_it_ud4_t;


/********************/
/* Local Prototypes */
/********************/

/* Chunked layout operation callbacks */
static herr_t H5D_chunk_construct(H5F_t *f, H5D_t *dset);
static herr_t H5D_chunk_io_init(const H5D_io_info_t *io_info,
    const H5D_type_info_t *type_info, hsize_t nelmts, const H5S_t *file_space,
    const H5S_t *mem_space, H5D_chunk_map_t *fm);
static herr_t H5D_chunk_read(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t nelmts, const H5S_t *file_space, const H5S_t *mem_space,
    H5D_chunk_map_t *fm);
static herr_t H5D_chunk_write(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t nelmts, const H5S_t *file_space, const H5S_t *mem_space,
    H5D_chunk_map_t *fm);
static herr_t H5D_chunk_flush(H5D_t *dset, hid_t dxpl_id);
static herr_t H5D_chunk_io_term(const H5D_chunk_map_t *fm);

/* "Nonexistent" layout operation callback */
static ssize_t
H5D_nonexistent_readvv(const H5D_io_info_t *io_info,
    size_t chunk_max_nseq, size_t *chunk_curr_seq, size_t chunk_len_arr[], hsize_t chunk_offset_arr[],
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_len_arr[], hsize_t mem_offset_arr[]);

/* Helper routines */
static herr_t H5D_chunk_set_info_real(H5O_layout_chunk_t *layout, unsigned ndims,
    const hsize_t *curr_dims);
static void *H5D_chunk_alloc(size_t size, const H5O_pline_t *pline);
static void *H5D_chunk_xfree(void *chk, const H5O_pline_t *pline);
static herr_t H5D_chunk_cinfo_cache_update(H5D_chunk_cached_t *last,
    const H5D_chunk_ud_t *udata);
static herr_t H5D_free_chunk_info(void *item, void *key, void *opdata);
static herr_t H5D_create_chunk_map_single(H5D_chunk_map_t *fm,
    const H5D_io_info_t *io_info);
static herr_t H5D_create_chunk_file_map_hyper(H5D_chunk_map_t *fm,
    const H5D_io_info_t *io_info);
static herr_t H5D_create_chunk_mem_map_hyper(const H5D_chunk_map_t *fm);
static herr_t H5D_chunk_file_cb(void *elem, hid_t type_id, unsigned ndims,
    const hsize_t *coords, void *fm);
static herr_t H5D_chunk_mem_cb(void *elem, hid_t type_id, unsigned ndims,
    const hsize_t *coords, void *fm);
static herr_t H5D_chunk_flush_entry(const H5D_t *dset, hid_t dxpl_id,
    const H5D_dxpl_cache_t *dxpl_cache, H5D_rdcc_ent_t *ent, hbool_t reset);
static herr_t H5D_chunk_cache_evict(const H5D_t *dset, hid_t dxpl_id,
    const H5D_dxpl_cache_t *dxpl_cache, H5D_rdcc_ent_t *ent, hbool_t flush);


/*********************/
/* Package Variables */
/*********************/

/* Chunked storage layout I/O ops */
const H5D_layout_ops_t H5D_LOPS_CHUNK[1] = {{
    H5D_chunk_construct,
    H5D_chunk_init,
    H5D_chunk_is_space_alloc,
    H5D_chunk_io_init,
    H5D_chunk_read,
    H5D_chunk_write,
#ifdef H5_HAVE_PARALLEL
    H5D_chunk_collective_read,
    H5D_chunk_collective_write,
#endif /* H5_HAVE_PARALLEL */
    NULL,
    NULL,
    H5D_chunk_flush,
    H5D_chunk_io_term
}};


/*******************/
/* Local Variables */
/*******************/

/* "nonexistent" storage layout I/O ops */
const H5D_layout_ops_t H5D_LOPS_NONEXISTENT[1] = {{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
#ifdef H5_HAVE_PARALLEL
    NULL,
    NULL,
#endif /* H5_HAVE_PARALLEL */
    H5D_nonexistent_readvv,
    NULL,
    NULL,
    NULL
}};

/* Declare a free list to manage the H5F_rdcc_ent_ptr_t sequence information */
H5FL_SEQ_DEFINE_STATIC(H5D_rdcc_ent_ptr_t);

/* Declare a free list to manage H5D_rdcc_ent_t objects */
H5FL_DEFINE_STATIC(H5D_rdcc_ent_t);

/* Declare a free list to manage the H5D_chunk_info_t struct */
H5FL_DEFINE(H5D_chunk_info_t);

/* Declare a free list to manage the chunk sequence information */
H5FL_BLK_DEFINE_STATIC(chunk);



/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_set_info_real
 *
 * Purpose:	Internal routine to set the information about chunks for a dataset
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, June 30, 2009
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_chunk_set_info_real(H5O_layout_chunk_t *layout, unsigned ndims, const hsize_t *curr_dims)
{
    unsigned u;                 /* Local index variable */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(H5D_chunk_set_info_real, FAIL)

    /* Sanity checks */
    HDassert(layout);
    HDassert(ndims > 0);
    HDassert(curr_dims);

    /* Compute the # of chunks in dataset dimensions */
    for(u = 0, layout->nchunks = 1; u < ndims; u++) {
        /* Round up to the next integer # of chunks, to accomodate partial chunks */
	layout->chunks[u] = ((curr_dims[u] + layout->dim[u]) - 1) / layout->dim[u];

        /* Accumulate the # of chunks */
	layout->nchunks *= layout->chunks[u];
    } /* end for */

    /* Get the "down" sizes for each dimension */
    if(H5V_array_down(ndims, layout->chunks, layout->down_chunks) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't compute 'down' chunk size value")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_set_info_real() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_set_info
 *
 * Purpose:	Sets the information about chunks for a dataset
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, June 30, 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_set_info(const H5D_t *dset)
{
    hsize_t curr_dims[H5O_LAYOUT_NDIMS];    /* Curr. size of dataset dimensions */
    int sndims;                 /* Rank of dataspace */
    unsigned ndims;             /* Rank of dataspace */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(H5D_chunk_set_info, FAIL)

    /* Sanity checks */
    HDassert(dset);

    /* Get the dim info for dataset */
    if((sndims = H5S_get_simple_extent_dims(dset->shared->space, curr_dims, NULL)) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get dataspace dimensions")
    H5_ASSIGN_OVERFLOW(ndims, sndims, int, unsigned);

    /* Set the base layout information */
    if(H5D_chunk_set_info_real(&dset->shared->layout.u.chunk, ndims, curr_dims) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set layout's chunk info")

    /* Call the index's "resize" callback */
    if(dset->shared->layout.storage.u.chunk.ops->resize && (dset->shared->layout.storage.u.chunk.ops->resize)(&dset->shared->layout.u.chunk) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to resize chunk index information")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_set_info() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_construct
 *
 * Purpose:	Constructs new chunked layout information for dataset
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, May 22, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_chunk_construct(H5F_t UNUSED *f, H5D_t *dset)
{
    const H5T_t *type = dset->shared->type;     /* Convenience pointer to dataset's datatype */
    hsize_t max_dim[H5O_LAYOUT_NDIMS];          /* Maximum size of data in elements */
    uint64_t chunk_size;        /* Size of chunk in bytes */
    int ndims;                  /* Rank of dataspace */
    unsigned u;                 /* Local index variable */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_chunk_construct)

    /* Sanity checks */
    HDassert(f);
    HDassert(dset);

    /* Set up layout information */
    if((ndims = H5S_GET_EXTENT_NDIMS(dset->shared->space)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to get rank")
    if(dset->shared->layout.u.chunk.ndims != (unsigned)ndims)
        HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, FAIL, "dimensionality of chunks doesn't match the dataspace")

    /* Increment # of chunk dimensions, to account for datatype size as last element */
    dset->shared->layout.u.chunk.ndims++;
    HDassert((unsigned)(dset->shared->layout.u.chunk.ndims) <= NELMTS(dset->shared->layout.u.chunk.dim));

    /* Chunked storage is not compatible with external storage (currently) */
    if(dset->shared->dcpl_cache.efl.nused > 0)
        HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, FAIL, "external storage not supported with chunked layout")

    /* Set the last dimension of the chunk size to the size of the datatype */
    dset->shared->layout.u.chunk.dim[dset->shared->layout.u.chunk.ndims - 1] = (uint32_t)H5T_GET_SIZE(type);

    /* Get local copy of dataset dimensions (for sanity checking) */
    if(H5S_get_simple_extent_dims(dset->shared->space, NULL, max_dim) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to query maximum dimensions")

    /* Sanity check dimensions */
    for(u = 0; u < dset->shared->layout.u.chunk.ndims - 1; u++)
        /*
         * The chunk size of a dimension with a fixed size cannot exceed
         * the maximum dimension size
         */
        if(max_dim[u] != H5S_UNLIMITED && max_dim[u] < dset->shared->layout.u.chunk.dim[u])
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "chunk size must be <= maximum dimension size for fixed-sized dimensions")

    /* Compute the total size of a chunk */
    /* (Use 64-bit value to ensure that we can detect >4GB chunks) */
    for(u = 1, chunk_size = (uint64_t)dset->shared->layout.u.chunk.dim[0]; u < dset->shared->layout.u.chunk.ndims; u++)
        chunk_size *= (uint64_t)dset->shared->layout.u.chunk.dim[u];

    /* Check for chunk larger than can be represented in 32-bits */
    /* (Chunk size is encoded in 32-bit value in v1 B-tree records) */
    if(chunk_size > (uint64_t)0xffffffff)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "chunk size must be < 4GB")

    /* Retain computed chunk size */
    H5_ASSIGN_OVERFLOW(dset->shared->layout.u.chunk.size, chunk_size, uint64_t, uint32_t);

    /* Reset address and pointer of the array struct for the chunked storage index */
    if(H5D_chunk_idx_reset(&dset->shared->layout.storage.u.chunk, TRUE) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to reset chunked storage index")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_construct() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_init
 *
 * Purpose:	Initialize the raw data chunk cache for a dataset.  This is
 *		called when the dataset is initialized.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, May 18, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_init(H5F_t *f, hid_t dxpl_id, const H5D_t *dset, hid_t dapl_id)
{
    H5D_chk_idx_info_t idx_info;        /* Chunked index info */
    H5D_rdcc_t	*rdcc = &(dset->shared->cache.chunk);   /* Convenience pointer to dataset's chunk cache */
    H5P_genplist_t *dapl;               /* Data access property list object pointer */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5D_chunk_init, FAIL)

    /* Sanity check */
    HDassert(f);
    HDassert(dset);

    if(NULL == (dapl = (H5P_genplist_t *)H5I_object(dapl_id)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for fapl ID");

    /* Use the properties in dapl_id if they have been set, otherwise use the properties from the file */
    if(H5P_get(dapl, H5D_ACS_DATA_CACHE_NUM_SLOTS_NAME, &rdcc->nslots) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET,FAIL, "can't get data cache number of slots");
    if(rdcc->nslots == H5D_CHUNK_CACHE_NSLOTS_DEFAULT)
        rdcc->nslots = H5F_RDCC_NSLOTS(f);

    if(H5P_get(dapl, H5D_ACS_DATA_CACHE_BYTE_SIZE_NAME, &rdcc->nbytes_max) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET,FAIL, "can't get data cache byte size");
    if(rdcc->nbytes_max == H5D_CHUNK_CACHE_NBYTES_DEFAULT)
        rdcc->nbytes_max = H5F_RDCC_NBYTES(f);

    if(H5P_get(dapl, H5D_ACS_PREEMPT_READ_CHUNKS_NAME, &rdcc->w0) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET,FAIL, "can't get preempt read chunks");
    if(rdcc->w0 < 0)
        rdcc->w0 = H5F_RDCC_W0(f);

    /* If nbytes_max or nslots is 0, set them both to 0 and avoid allocating space */
    if(!rdcc->nbytes_max || !rdcc->nslots)
        rdcc->nbytes_max = rdcc->nslots = 0;
    else {
        rdcc->slot = H5FL_SEQ_CALLOC(H5D_rdcc_ent_ptr_t, rdcc->nslots);
        if(NULL == rdcc->slot)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

        /* Reset any cached chunk info for this dataset */
        H5D_chunk_cinfo_cache_reset(&(rdcc->last));
    } /* end else */

    /* Compose chunked index info struct */
    idx_info.f = f;
    idx_info.dxpl_id = dxpl_id;
    idx_info.pline = &dset->shared->dcpl_cache.pline;
    idx_info.layout = &dset->shared->layout.u.chunk;
    idx_info.storage = &dset->shared->layout.storage.u.chunk;

    /* Allocate any indexing structures */
    if(dset->shared->layout.storage.u.chunk.ops->init && (dset->shared->layout.storage.u.chunk.ops->init)(&idx_info, dset->shared->space, dset->oloc.addr) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't initialize indexing information")

    /* Set the number of chunks in dataset, etc. */
    if(H5D_chunk_set_info(dset) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to set # of chunks for dataset")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_init() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_is_space_alloc
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
H5D_chunk_is_space_alloc(const H5O_storage_t *storage)
{
    hbool_t ret_value;                  /* Return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5D_chunk_is_space_alloc)

    /* Sanity checks */
    HDassert(storage);

    /* Query index layer */
    ret_value = (storage->u.chunk.ops->is_space_alloc)(&storage->u.chunk);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_is_space_alloc() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_io_init
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
H5D_chunk_io_init(const H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t nelmts, const H5S_t *file_space, const H5S_t *mem_space,
    H5D_chunk_map_t *fm)
{
    H5D_t *dataset = io_info->dset;     /* Local pointer to dataset info */
    const H5T_t *mem_type = type_info->mem_type;        /* Local pointer to memory datatype */
    H5S_t *tmp_mspace = NULL;   /* Temporary memory dataspace */
    hssize_t old_offset[H5O_LAYOUT_NDIMS];  /* Old selection offset */
    htri_t file_space_normalized = FALSE;   /* File dataspace was normalized */
    hid_t f_tid = (-1);           /* Temporary copy of file datatype for iteration */
    hbool_t iter_init = FALSE;  /* Selection iteration info has been initialized */
    unsigned f_ndims;           /* The number of dimensions of the file's dataspace */
    int sm_ndims;               /* The number of dimensions of the memory buffer's dataspace (signed) */
    H5SL_node_t *curr_node;     /* Current node in skip list */
    H5S_sel_type fsel_type;     /* Selection type on disk */
    char bogus;                 /* "bogus" buffer to pass to selection iterator */
    unsigned u;                 /* Local index variable */
    hbool_t sel_hyper_flag;
    herr_t ret_value = SUCCEED;	/* Return value		*/

    FUNC_ENTER_NOAPI_NOINIT(H5D_chunk_io_init)

    /* Get layout for dataset */
    fm->layout = &(dataset->shared->layout);
    fm->nelmts = nelmts;

    /* Check if the memory space is scalar & make equivalent memory space */
    if((sm_ndims = H5S_GET_EXTENT_NDIMS(mem_space)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "unable to get dimension number")
    /* Set the number of dimensions for the memory dataspace */
    H5_ASSIGN_OVERFLOW(fm->m_ndims, sm_ndims, int, unsigned);

    /* Get dim number and dimensionality for each dataspace */
    fm->f_ndims = f_ndims = dataset->shared->layout.u.chunk.ndims - 1;
    if(H5S_get_simple_extent_dims(file_space, fm->f_dims, NULL) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "unable to get dimensionality")

    /* Normalize hyperslab selections by adjusting them by the offset */
    /* (It might be worthwhile to normalize both the file and memory dataspaces
     * before any (contiguous, chunked, etc) file I/O operation, in order to
     * speed up hyperslab calculations by removing the extra checks and/or
     * additions involving the offset and the hyperslab selection -QAK)
     */
    if((file_space_normalized = H5S_hyper_normalize_offset((H5S_t *)file_space, old_offset)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_BADSELECT, FAIL, "unable to normalize dataspace by offset")

    /* Decide the number of chunks in each dimension*/
    for(u = 0; u < f_ndims; u++) {
        /* Keep the size of the chunk dimensions as hsize_t for various routines */
        fm->chunk_dim[u] = fm->layout->u.chunk.dim[u];
    } /* end for */

#ifdef H5_HAVE_PARALLEL
    /* Calculate total chunk in file map*/
    fm->select_chunk = NULL;
    if(io_info->using_mpi_vfd) {
        H5_CHECK_OVERFLOW(fm->layout->u.chunk.nchunks, hsize_t, size_t);
        if(NULL == (fm->select_chunk = (H5D_chunk_info_t **)H5MM_calloc((size_t)fm->layout->u.chunk.nchunks * sizeof(H5D_chunk_info_t *))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate chunk info")
    } /* end if */
#endif /* H5_HAVE_PARALLEL */


    /* Initialize "last chunk" information */
    fm->last_index = (hsize_t)-1;
    fm->last_chunk_info = NULL;

    /* Point at the dataspaces */
    fm->file_space = file_space;
    fm->mem_space = mem_space;

    /* Special case for only one element in selection */
    /* (usually appending a record) */
    if(nelmts == 1
#ifdef H5_HAVE_PARALLEL
            && !(io_info->using_mpi_vfd)
#endif /* H5_HAVE_PARALLEL */
        ) {
        /* Initialize skip list for chunk selections */
        fm->sel_chunks = NULL;
        fm->use_single = TRUE;

        /* Initialize single chunk dataspace */
        if(NULL == dataset->shared->cache.chunk.single_space) {
            /* Make a copy of the dataspace for the dataset */
            if((dataset->shared->cache.chunk.single_space = H5S_copy(file_space, TRUE, FALSE)) == NULL)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "unable to copy file space")

            /* Resize chunk's dataspace dimensions to size of chunk */
            if(H5S_set_extent_real(dataset->shared->cache.chunk.single_space, fm->chunk_dim) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSET, FAIL, "can't adjust chunk dimensions")

            /* Set the single chunk dataspace to 'all' selection */
            if(H5S_select_all(dataset->shared->cache.chunk.single_space, TRUE) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTSELECT, FAIL, "unable to set all selection")
        } /* end if */
        fm->single_space = dataset->shared->cache.chunk.single_space;
        HDassert(fm->single_space);

        /* Allocate the single chunk information */
        if(NULL == dataset->shared->cache.chunk.single_chunk_info) {
            if(NULL == (dataset->shared->cache.chunk.single_chunk_info = H5FL_MALLOC(H5D_chunk_info_t)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate chunk info")
        } /* end if */
        fm->single_chunk_info = dataset->shared->cache.chunk.single_chunk_info;
        HDassert(fm->single_chunk_info);

        /* Reset chunk template information */
        fm->mchunk_tmpl = NULL;

        /* Set up chunk mapping for single element */
        if(H5D_create_chunk_map_single(fm, io_info) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to create chunk selections for single element")
    } /* end if */
    else {
        /* Initialize skip list for chunk selections */
        if(NULL == dataset->shared->cache.chunk.sel_chunks) {
            if(NULL == (dataset->shared->cache.chunk.sel_chunks = H5SL_create(H5SL_TYPE_HSIZE)))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTCREATE, FAIL, "can't create skip list for chunk selections")
        } /* end if */
        fm->sel_chunks = dataset->shared->cache.chunk.sel_chunks;
        HDassert(fm->sel_chunks);

        /* We are not using single element mode */
        fm->use_single = FALSE;

        /* Get type of selection on disk & in memory */
        if((fsel_type = H5S_GET_SELECT_TYPE(file_space)) < H5S_SEL_NONE)
            HGOTO_ERROR(H5E_DATASET, H5E_BADSELECT, FAIL, "unable to get type of selection")
        if((fm->msel_type = H5S_GET_SELECT_TYPE(mem_space)) < H5S_SEL_NONE)
            HGOTO_ERROR(H5E_DATASET, H5E_BADSELECT, FAIL, "unable to get type of selection")

        /* If the selection is NONE or POINTS, set the flag to FALSE */
        if(fsel_type == H5S_SEL_POINTS || fsel_type == H5S_SEL_NONE)
            sel_hyper_flag = FALSE;
        else
            sel_hyper_flag = TRUE;

        /* Check if file selection is a not a hyperslab selection */
        if(sel_hyper_flag) {
            /* Build the file selection for each chunk */
            if(H5D_create_chunk_file_map_hyper(fm, io_info) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to create file chunk selections")

            /* Clean file chunks' hyperslab span "scratch" information */
            curr_node = H5SL_first(fm->sel_chunks);
            while(curr_node) {
                H5D_chunk_info_t *chunk_info;   /* Pointer chunk information */

                /* Get pointer to chunk's information */
                chunk_info = (H5D_chunk_info_t *)H5SL_item(curr_node);
                HDassert(chunk_info);

                /* Clean hyperslab span's "scratch" information */
                if(H5S_hyper_reset_scratch(chunk_info->fspace) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "unable to reset span scratch info")

                /* Get the next chunk node in the skip list */
                curr_node = H5SL_next(curr_node);
            } /* end while */
        } /* end if */
        else {
            /* Create temporary datatypes for selection iteration */
            if((f_tid = H5I_register(H5I_DATATYPE, H5T_copy(dataset->shared->type, H5T_COPY_ALL), FALSE)) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to register file datatype")

            /* Spaces might not be the same shape, iterate over the file selection directly */
            if(H5S_select_iterate(&bogus, f_tid, file_space,  H5D_chunk_file_cb, fm) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to create file chunk selections")

            /* Reset "last chunk" info */
            fm->last_index = (hsize_t)-1;
            fm->last_chunk_info = NULL;
        } /* end else */

        /* Build the memory selection for each chunk */
        if(sel_hyper_flag && H5S_select_shape_same(file_space, mem_space) == TRUE) {
            /* Reset chunk template information */
            fm->mchunk_tmpl = NULL;

            /* If the selections are the same shape, use the file chunk information
             * to generate the memory chunk information quickly.
             */
            if(H5D_create_chunk_mem_map_hyper(fm) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to create memory chunk selections")
        } /* end if */
        else {
            size_t elmt_size;           /* Memory datatype size */

            /* Make a copy of equivalent memory space */
            if((tmp_mspace = H5S_copy(mem_space, TRUE, FALSE)) == NULL)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "unable to copy memory space")

            /* De-select the mem space copy */
            if(H5S_select_none(tmp_mspace) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to de-select memory space")

            /* Save chunk template information */
            fm->mchunk_tmpl = tmp_mspace;

            /* Create temporary datatypes for selection iteration */
            if(f_tid < 0) {
                if((f_tid = H5I_register(H5I_DATATYPE, H5T_copy(dataset->shared->type, H5T_COPY_ALL), FALSE)) < 0)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to register file datatype")
            } /* end if */

            /* Create selection iterator for memory selection */
            if(0 == (elmt_size = H5T_get_size(mem_type)))
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADSIZE, FAIL, "datatype size invalid")
            if(H5S_select_iter_init(&(fm->mem_iter), mem_space, elmt_size) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator")
            iter_init = TRUE;	/* Selection iteration info has been initialized */

            /* Spaces aren't the same shape, iterate over the memory selection directly */
            if(H5S_select_iterate(&bogus, f_tid, file_space, H5D_chunk_mem_cb, fm) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to create memory chunk selections")

            /* Clean up hyperslab stuff, if necessary */
            if(fm->msel_type != H5S_SEL_POINTS) {
                /* Clean memory chunks' hyperslab span "scratch" information */
                curr_node = H5SL_first(fm->sel_chunks);
                while(curr_node) {
                    H5D_chunk_info_t *chunk_info;   /* Pointer chunk information */

                    /* Get pointer to chunk's information */
                    chunk_info = (H5D_chunk_info_t *)H5SL_item(curr_node);
                    HDassert(chunk_info);

                    /* Clean hyperslab span's "scratch" information */
                    if(H5S_hyper_reset_scratch(chunk_info->mspace) < 0)
                        HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "unable to reset span scratch info")

                    /* Get the next chunk node in the skip list */
                    curr_node = H5SL_next(curr_node);
                } /* end while */
            } /* end if */
        } /* end else */
    } /* end else */

done:
    /* Release the [potentially partially built] chunk mapping information if an error occurs */
    if(ret_value < 0) {
        if(tmp_mspace && !fm->mchunk_tmpl) {
            if(H5S_close(tmp_mspace) < 0)
                HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "can't release memory chunk dataspace template")
        } /* end if */

        if(H5D_chunk_io_term(fm) < 0)
            HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release chunk mapping")
    } /* end if */

    /* Reset the global dataspace info */
    fm->file_space = NULL;
    fm->mem_space = NULL;

    if(iter_init) {
        if(H5S_SELECT_ITER_RELEASE(&(fm->mem_iter)) < 0)
            HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator")
    } /* end if */
    if(f_tid!=(-1)) {
        if(H5I_dec_ref(f_tid, FALSE) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't decrement temporary datatype ID")
    } /* end if */
    if(file_space_normalized) {
        /* (Casting away const OK -QAK) */
        if(H5S_hyper_denormalize_offset((H5S_t *)file_space, old_offset) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_BADSELECT, FAIL, "unable to normalize dataspace by offset")
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_io_init() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_alloc
 *
 * Purpose:	Allocate space for a chunk in memory.  This routine allocates
 *              memory space for non-filtered chunks from a block free list
 *              and uses malloc()/free() for filtered chunks.
 *
 * Return:	Pointer to memory for chunk on success/NULL on failure
 *
 * Programmer:	Quincey Koziol
 *              April 22, 2004
 *
 *-------------------------------------------------------------------------
 */
static void *
H5D_chunk_alloc(size_t size, const H5O_pline_t *pline)
{
    void *ret_value = NULL;		/* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_chunk_alloc)

    HDassert(size);
    HDassert(pline);

    if(pline->nused > 0)
        ret_value = H5MM_malloc(size);
    else
        ret_value = H5FL_BLK_MALLOC(chunk, size);

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D_chunk_alloc() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_xfree
 *
 * Purpose:	Free space for a chunk in memory.  This routine allocates
 *              memory space for non-filtered chunks from a block free list
 *              and uses malloc()/free() for filtered chunks.
 *
 * Return:	NULL (never fails)
 *
 * Programmer:	Quincey Koziol
 *              April 22, 2004
 *
 *-------------------------------------------------------------------------
 */
static void *
H5D_chunk_xfree(void *chk, const H5O_pline_t *pline)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_chunk_xfree)

    HDassert(pline);

    if(chk) {
        if(pline->nused > 0)
            H5MM_xfree(chk);
        else
            chk = H5FL_BLK_FREE(chunk, chk);
    } /* end if */

    FUNC_LEAVE_NOAPI(NULL)
} /* H5D_chunk_xfree() */


/*--------------------------------------------------------------------------
 NAME
    H5D_free_chunk_info
 PURPOSE
    Internal routine to destroy a chunk info node
 USAGE
    void H5D_free_chunk_info(chunk_info)
        void *chunk_info;    IN: Pointer to chunk info to destroy
 RETURNS
    No return value
 DESCRIPTION
    Releases all the memory for a chunk info node.  Called by H5SL_free
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5D_free_chunk_info(void *item, void UNUSED *key, void UNUSED *opdata)
{
    H5D_chunk_info_t *chunk_info = (H5D_chunk_info_t *)item;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_free_chunk_info)

    HDassert(chunk_info);

    /* Close the chunk's file dataspace, if it's not shared */
    if(!chunk_info->fspace_shared)
        (void)H5S_close(chunk_info->fspace);
    else
        H5S_select_all(chunk_info->fspace, TRUE);

    /* Close the chunk's memory dataspace, if it's not shared */
    if(!chunk_info->mspace_shared)
        (void)H5S_close(chunk_info->mspace);

    /* Free the actual chunk info */
    chunk_info = H5FL_FREE(H5D_chunk_info_t, chunk_info);

    FUNC_LEAVE_NOAPI(0)
}   /* H5D_free_chunk_info() */


/*-------------------------------------------------------------------------
 * Function:	H5D_create_chunk_map_single
 *
 * Purpose:	Create chunk selections when appending a single record
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, November 20, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_create_chunk_map_single(H5D_chunk_map_t *fm, const H5D_io_info_t
#ifndef H5_HAVE_PARALLEL
    UNUSED
#endif /* H5_HAVE_PARALLEL */
    *io_info)
{
    H5D_chunk_info_t *chunk_info;           /* Chunk information to insert into skip list */
    hsize_t     sel_start[H5O_LAYOUT_NDIMS]; /* Offset of low bound of file selection */
    hsize_t     sel_end[H5O_LAYOUT_NDIMS];  /* Offset of high bound of file selection */
    unsigned    u;                          /* Local index variable */
    herr_t	ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_create_chunk_map_single)

    /* Sanity check */
    HDassert(fm->f_ndims > 0);

    /* Get coordinate for selection */
    if(H5S_SELECT_BOUNDS(fm->file_space, sel_start, sel_end) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't get file selection bound info")

    /* Initialize the 'single chunk' file & memory chunk information */
    chunk_info = fm->single_chunk_info;
    chunk_info->chunk_points = 1;

    /* Set chunk location & hyperslab size */
    for(u = 0; u < fm->f_ndims; u++) {
        HDassert(sel_start[u] == sel_end[u]);
        chunk_info->coords[u] = (sel_start[u] / fm->layout->u.chunk.dim[u]) * fm->layout->u.chunk.dim[u];
    } /* end for */
    chunk_info->coords[fm->f_ndims] = 0;

    /* Calculate the index of this chunk */
    if(H5V_chunk_index(fm->f_ndims, chunk_info->coords, fm->layout->u.chunk.dim, fm->layout->u.chunk.down_chunks, &chunk_info->index) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "can't get chunk index")

    /* Copy selection for file's dataspace into chunk dataspace */
    if(H5S_select_copy(fm->single_space, fm->file_space, FALSE) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "unable to copy file selection")

    /* Move selection back to have correct offset in chunk */
    if(H5S_SELECT_ADJUST_U(fm->single_space, chunk_info->coords) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSELECT, FAIL, "can't adjust chunk selection")

#ifdef H5_HAVE_PARALLEL
    /* store chunk selection information */
    if(io_info->using_mpi_vfd)
        fm->select_chunk[chunk_info->index] = chunk_info;
#endif /* H5_HAVE_PARALLEL */

    /* Set the file dataspace for the chunk to the shared 'single' dataspace */
    chunk_info->fspace = fm->single_space;

    /* Indicate that the chunk's file dataspace is shared */
    chunk_info->fspace_shared = TRUE;

    /* Just point at the memory dataspace & selection */
    /* (Casting away const OK -QAK) */
    chunk_info->mspace = (H5S_t *)fm->mem_space;

    /* Indicate that the chunk's memory dataspace is shared */
    chunk_info->mspace_shared = TRUE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_create_chunk_map_single() */


/*-------------------------------------------------------------------------
 * Function:	H5D_create_chunk_file_map_hyper
 *
 * Purpose:	Create all chunk selections in file.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Thursday, May 29, 2003
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_create_chunk_file_map_hyper(H5D_chunk_map_t *fm, const H5D_io_info_t
#ifndef H5_HAVE_PARALLEL
    UNUSED
#endif /* H5_HAVE_PARALLEL */
    *io_info)
{
    hsize_t     sel_start[H5O_LAYOUT_NDIMS];   /* Offset of low bound of file selection */
    hsize_t     sel_end[H5O_LAYOUT_NDIMS];   /* Offset of high bound of file selection */
    hsize_t     sel_points;                 /* Number of elements in file selection */
    hsize_t     start_coords[H5O_LAYOUT_NDIMS];   /* Starting coordinates of selection */
    hsize_t     coords[H5O_LAYOUT_NDIMS];   /* Current coordinates of chunk */
    hsize_t     end[H5O_LAYOUT_NDIMS];      /* Current coordinates of chunk */
    hsize_t     chunk_index;                /* Index of chunk */
    int         curr_dim;                   /* Current dimension to increment */
    unsigned    u;                          /* Local index variable */
    herr_t	ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_create_chunk_file_map_hyper)

    /* Sanity check */
    assert(fm->f_ndims>0);

    /* Get number of elements selected in file */
    sel_points = fm->nelmts;

    /* Get bounding box for selection (to reduce the number of chunks to iterate over) */
    if(H5S_SELECT_BOUNDS(fm->file_space, sel_start, sel_end) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't get file selection bound info")

    /* Set initial chunk location & hyperslab size */
    for(u = 0; u < fm->f_ndims; u++) {
        start_coords[u] = (sel_start[u] / fm->layout->u.chunk.dim[u]) * fm->layout->u.chunk.dim[u];
        coords[u] = start_coords[u];
        end[u] = (coords[u] + fm->chunk_dim[u]) - 1;
    } /* end for */

    /* Calculate the index of this chunk */
    if(H5V_chunk_index(fm->f_ndims, coords, fm->layout->u.chunk.dim, fm->layout->u.chunk.down_chunks, &chunk_index) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "can't get chunk index")

    /* Iterate through each chunk in the dataset */
    while(sel_points) {
        /* Check for intersection of temporary chunk and file selection */
        /* (Casting away const OK - QAK) */
        if(TRUE == H5S_hyper_intersect_block((H5S_t *)fm->file_space, coords, end)) {
            H5S_t *tmp_fchunk;                  /* Temporary file dataspace */
            H5D_chunk_info_t *new_chunk_info;   /* chunk information to insert into skip list */
            hssize_t    schunk_points;          /* Number of elements in chunk selection */

            /* Create "temporary" chunk for selection operations (copy file space) */
            if(NULL == (tmp_fchunk = H5S_copy(fm->file_space, TRUE, FALSE)))
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "unable to copy memory space")

            /* Make certain selections are stored in span tree form (not "optimized hyperslab" or "all") */
            if(H5S_hyper_convert(tmp_fchunk) < 0) {
                (void)H5S_close(tmp_fchunk);
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to convert selection to span trees")
            } /* end if */

            /* "AND" temporary chunk and current chunk */
            if(H5S_select_hyperslab(tmp_fchunk,H5S_SELECT_AND,coords,NULL,fm->chunk_dim,NULL) < 0) {
                (void)H5S_close(tmp_fchunk);
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSELECT, FAIL, "can't create chunk selection")
            } /* end if */

            /* Resize chunk's dataspace dimensions to size of chunk */
            if(H5S_set_extent_real(tmp_fchunk,fm->chunk_dim) < 0) {
                (void)H5S_close(tmp_fchunk);
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSELECT, FAIL, "can't adjust chunk dimensions")
            } /* end if */

            /* Move selection back to have correct offset in chunk */
            if(H5S_SELECT_ADJUST_U(tmp_fchunk, coords) < 0) {
                (void)H5S_close(tmp_fchunk);
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSELECT, FAIL, "can't adjust chunk selection")
            } /* end if */

            /* Add temporary chunk to the list of chunks */

            /* Allocate the file & memory chunk information */
            if (NULL==(new_chunk_info = H5FL_MALLOC (H5D_chunk_info_t))) {
                (void)H5S_close(tmp_fchunk);
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate chunk info")
            } /* end if */

            /* Initialize the chunk information */

            /* Set the chunk index */
            new_chunk_info->index=chunk_index;

#ifdef H5_HAVE_PARALLEL
            /* store chunk selection information */
            if(io_info->using_mpi_vfd)
                fm->select_chunk[chunk_index] = new_chunk_info;
#endif /* H5_HAVE_PARALLEL */

            /* Set the file chunk dataspace */
            new_chunk_info->fspace = tmp_fchunk;
            new_chunk_info->fspace_shared = FALSE;

            /* Set the memory chunk dataspace */
            new_chunk_info->mspace=NULL;
            new_chunk_info->mspace_shared = FALSE;

            /* Copy the chunk's coordinates */
            for(u=0; u<fm->f_ndims; u++)
                new_chunk_info->coords[u]=coords[u];
            new_chunk_info->coords[fm->f_ndims]=0;

            /* Insert the new chunk into the skip list */
            if(H5SL_insert(fm->sel_chunks, new_chunk_info, &new_chunk_info->index) < 0) {
                H5D_free_chunk_info(new_chunk_info, NULL, NULL);
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINSERT, FAIL, "can't insert chunk into skip list")
            } /* end if */

            /* Get number of elements selected in chunk */
            if((schunk_points = H5S_GET_SELECT_NPOINTS(tmp_fchunk)) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't get file selection # of elements")
            H5_ASSIGN_OVERFLOW(new_chunk_info->chunk_points, schunk_points, hssize_t, uint32_t);

            /* Decrement # of points left in file selection */
            sel_points -= (hsize_t)schunk_points;

            /* Leave if we are done */
            if(sel_points == 0)
                HGOTO_DONE(SUCCEED)
        } /* end if */

        /* Increment chunk index */
        chunk_index++;

        /* Set current increment dimension */
        curr_dim=(int)fm->f_ndims-1;

        /* Increment chunk location in fastest changing dimension */
        H5_CHECK_OVERFLOW(fm->chunk_dim[curr_dim],hsize_t,hssize_t);
        coords[curr_dim]+=fm->chunk_dim[curr_dim];
        end[curr_dim]+=fm->chunk_dim[curr_dim];

        /* Bring chunk location back into bounds, if necessary */
        if(coords[curr_dim] > sel_end[curr_dim]) {
            do {
                /* Reset current dimension's location to 0 */
                coords[curr_dim] = start_coords[curr_dim]; /*lint !e771 The start_coords will always be initialized */
                end[curr_dim] = (coords[curr_dim] + fm->chunk_dim[curr_dim]) - 1;

                /* Decrement current dimension */
                curr_dim--;

                /* Increment chunk location in current dimension */
                coords[curr_dim] += fm->chunk_dim[curr_dim];
                end[curr_dim] = (coords[curr_dim] + fm->chunk_dim[curr_dim]) - 1;
            } while(coords[curr_dim] > sel_end[curr_dim]);

            /* Re-calculate the index of this chunk */
            if(H5V_chunk_index(fm->f_ndims, coords, fm->layout->u.chunk.dim, fm->layout->u.chunk.down_chunks, &chunk_index) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "can't get chunk index")
        } /* end if */
    } /* end while */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_create_chunk_file_map_hyper() */


/*-------------------------------------------------------------------------
 * Function:	H5D_create_chunk_mem_map_hyper
 *
 * Purpose:	Create all chunk selections in memory by copying the file
 *              chunk selections and adjusting their offsets to be correct
 *              for the memory.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Thursday, May 29, 2003
 *
 * Assumptions: That the file and memory selections are the same shape.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_create_chunk_mem_map_hyper(const H5D_chunk_map_t *fm)
{
    H5SL_node_t *curr_node;                 /* Current node in skip list */
    hsize_t    file_sel_start[H5O_LAYOUT_NDIMS];    /* Offset of low bound of file selection */
    hsize_t    file_sel_end[H5O_LAYOUT_NDIMS];  /* Offset of high bound of file selection */
    hsize_t    mem_sel_start[H5O_LAYOUT_NDIMS]; /* Offset of low bound of file selection */
    hsize_t    mem_sel_end[H5O_LAYOUT_NDIMS];   /* Offset of high bound of file selection */
    hssize_t adjust[H5O_LAYOUT_NDIMS];      /* Adjustment to make to all file chunks */
    hssize_t chunk_adjust[H5O_LAYOUT_NDIMS];  /* Adjustment to make to a particular chunk */
    unsigned    u;                          /* Local index variable */
    herr_t	ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_create_chunk_mem_map_hyper)

    /* Sanity check */
    assert(fm->f_ndims>0);

    /* Check for all I/O going to a single chunk */
    if(H5SL_count(fm->sel_chunks)==1) {
        H5D_chunk_info_t *chunk_info;   /* Pointer to chunk information */

        /* Get the node */
        curr_node=H5SL_first(fm->sel_chunks);

        /* Get pointer to chunk's information */
        chunk_info = (H5D_chunk_info_t *)H5SL_item(curr_node);
        assert(chunk_info);

        /* Just point at the memory dataspace & selection */
        /* (Casting away const OK -QAK) */
        chunk_info->mspace = (H5S_t *)fm->mem_space;

        /* Indicate that the chunk's memory space is shared */
        chunk_info->mspace_shared = TRUE;
    } /* end if */
    else {
        /* Get bounding box for file selection */
        if(H5S_SELECT_BOUNDS(fm->file_space, file_sel_start, file_sel_end) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't get file selection bound info")

        /* Get bounding box for memory selection */
        if(H5S_SELECT_BOUNDS(fm->mem_space, mem_sel_start, mem_sel_end) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't get file selection bound info")

        /* Calculate the adjustment for memory selection from file selection */
        assert(fm->m_ndims==fm->f_ndims);
        for(u=0; u<fm->f_ndims; u++) {
            H5_CHECK_OVERFLOW(file_sel_start[u],hsize_t,hssize_t);
            H5_CHECK_OVERFLOW(mem_sel_start[u],hsize_t,hssize_t);
            adjust[u]=(hssize_t)file_sel_start[u]-(hssize_t)mem_sel_start[u];
        } /* end for */

        /* Iterate over each chunk in the chunk list */
        curr_node=H5SL_first(fm->sel_chunks);
        while(curr_node) {
            H5D_chunk_info_t *chunk_info;   /* Pointer to chunk information */

            /* Get pointer to chunk's information */
            chunk_info = (H5D_chunk_info_t *)H5SL_item(curr_node);
            assert(chunk_info);

            /* Copy the information */

            /* Copy the memory dataspace */
            if((chunk_info->mspace = H5S_copy(fm->mem_space, TRUE, FALSE)) == NULL)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "unable to copy memory space")

            /* Release the current selection */
            if(H5S_SELECT_RELEASE(chunk_info->mspace) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection")

            /* Copy the file chunk's selection */
            if(H5S_select_copy(chunk_info->mspace,chunk_info->fspace,FALSE) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "unable to copy selection")

            /* Compensate for the chunk offset */
            for(u=0; u<fm->f_ndims; u++) {
                H5_CHECK_OVERFLOW(chunk_info->coords[u],hsize_t,hssize_t);
                chunk_adjust[u]=adjust[u]-(hssize_t)chunk_info->coords[u]; /*lint !e771 The adjust array will always be initialized */
            } /* end for */

            /* Adjust the selection */
            if(H5S_hyper_adjust_s(chunk_info->mspace,chunk_adjust) < 0) /*lint !e772 The chunk_adjust array will always be initialized */
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSELECT, FAIL, "can't adjust chunk selection")

            /* Get the next chunk node in the skip list */
            curr_node=H5SL_next(curr_node);
        } /* end while */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_create_chunk_mem_map_hyper() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_file_cb
 *
 * Purpose:	Callback routine for file selection iterator.  Used when
 *              creating selections in file for each point selected.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, July 23, 2003
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_chunk_file_cb(void UNUSED *elem, hid_t UNUSED type_id, unsigned ndims, const hsize_t *coords, void *_fm)
{
    H5D_chunk_map_t      *fm = (H5D_chunk_map_t *)_fm;  /* File<->memory chunk mapping info */
    H5D_chunk_info_t *chunk_info;               /* Chunk information for current chunk */
    hsize_t    coords_in_chunk[H5O_LAYOUT_NDIMS];        /* Coordinates of element in chunk */
    hsize_t     chunk_index;                    /* Chunk index */
    unsigned    u;                              /* Local index variable */
    herr_t	ret_value = SUCCEED;            /* Return value		*/

    FUNC_ENTER_NOAPI_NOINIT(H5D_chunk_file_cb)

    /* Calculate the index of this chunk */
    if(H5V_chunk_index(ndims, coords, fm->layout->u.chunk.dim, fm->layout->u.chunk.down_chunks, &chunk_index) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "can't get chunk index")

    /* Find correct chunk in file & memory skip list */
    if(chunk_index==fm->last_index) {
        /* If the chunk index is the same as the last chunk index we used,
         * get the cached info to operate on.
         */
        chunk_info=fm->last_chunk_info;
    } /* end if */
    else {
        /* If the chunk index is not the same as the last chunk index we used,
         * find the chunk in the skip list.
         */
        /* Get the chunk node from the skip list */
        if(NULL == (chunk_info = (H5D_chunk_info_t *)H5SL_search(fm->sel_chunks, &chunk_index))) {
            H5S_t *fspace;                      /* Memory chunk's dataspace */

            /* Allocate the file & memory chunk information */
            if (NULL==(chunk_info = H5FL_MALLOC (H5D_chunk_info_t)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate chunk info")

            /* Initialize the chunk information */

            /* Set the chunk index */
            chunk_info->index=chunk_index;

            /* Create a dataspace for the chunk */
            if((fspace = H5S_create_simple(fm->f_ndims,fm->chunk_dim,NULL))==NULL) {
                chunk_info = H5FL_FREE(H5D_chunk_info_t, chunk_info);
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCREATE, FAIL, "unable to create dataspace for chunk")
            } /* end if */

            /* De-select the chunk space */
            if(H5S_select_none(fspace) < 0) {
                (void)H5S_close(fspace);
                chunk_info = H5FL_FREE(H5D_chunk_info_t, chunk_info);
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to de-select dataspace")
            } /* end if */

            /* Set the file chunk dataspace */
            chunk_info->fspace = fspace;
            chunk_info->fspace_shared = FALSE;

            /* Set the memory chunk dataspace */
            chunk_info->mspace = NULL;
            chunk_info->mspace_shared = FALSE;

            /* Set the number of selected elements in chunk to zero */
            chunk_info->chunk_points = 0;

            /* Compute the chunk's coordinates */
            for(u = 0; u < fm->f_ndims; u++) {
                H5_CHECK_OVERFLOW(fm->layout->u.chunk.dim[u], hsize_t, hssize_t);
                chunk_info->coords[u] = (coords[u] / (hssize_t)fm->layout->u.chunk.dim[u]) * (hssize_t)fm->layout->u.chunk.dim[u];
            } /* end for */
            chunk_info->coords[fm->f_ndims] = 0;

            /* Insert the new chunk into the skip list */
            if(H5SL_insert(fm->sel_chunks,chunk_info,&chunk_info->index) < 0) {
                H5D_free_chunk_info(chunk_info,NULL,NULL);
                HGOTO_ERROR(H5E_DATASPACE,H5E_CANTINSERT,FAIL,"can't insert chunk into skip list")
            } /* end if */
        } /* end if */

        /* Update the "last chunk seen" information */
        fm->last_index=chunk_index;
        fm->last_chunk_info=chunk_info;
    } /* end else */

    /* Get the coordinates of the element in the chunk */
    for(u = 0; u < fm->f_ndims; u++)
        coords_in_chunk[u] = coords[u] % fm->layout->u.chunk.dim[u];

    /* Add point to file selection for chunk */
    if(H5S_select_elements(chunk_info->fspace, H5S_SELECT_APPEND, (size_t)1, coords_in_chunk) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSELECT, FAIL, "unable to select element")

    /* Increment the number of elemented selected in chunk */
    chunk_info->chunk_points++;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_file_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_mem_cb
 *
 * Purpose:	Callback routine for file selection iterator.  Used when
 *              creating selections in memory for each chunk.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *		Thursday, April 10, 2003
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5D_chunk_mem_cb(void UNUSED *elem, hid_t UNUSED type_id, unsigned ndims, const hsize_t *coords, void *_fm)
{
    H5D_chunk_map_t      *fm = (H5D_chunk_map_t *)_fm;  /* File<->memory chunk mapping info */
    H5D_chunk_info_t *chunk_info;               /* Chunk information for current chunk */
    hsize_t    coords_in_mem[H5O_LAYOUT_NDIMS];        /* Coordinates of element in memory */
    hsize_t     chunk_index;                    /* Chunk index */
    herr_t	ret_value = SUCCEED;            /* Return value		*/

    FUNC_ENTER_NOAPI_NOINIT(H5D_chunk_mem_cb)

    /* Calculate the index of this chunk */
    if(H5V_chunk_index(ndims, coords, fm->layout->u.chunk.dim, fm->layout->u.chunk.down_chunks, &chunk_index) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "can't get chunk index")

    /* Find correct chunk in file & memory skip list */
    if(chunk_index == fm->last_index) {
        /* If the chunk index is the same as the last chunk index we used,
         * get the cached spaces to operate on.
         */
        chunk_info = fm->last_chunk_info;
    } /* end if */
    else {
        /* If the chunk index is not the same as the last chunk index we used,
         * find the chunk in the skip list.
         */
        /* Get the chunk node from the skip list */
        if(NULL == (chunk_info = (H5D_chunk_info_t *)H5SL_search(fm->sel_chunks, &chunk_index)))
            HGOTO_ERROR(H5E_DATASPACE, H5E_NOTFOUND, FAIL, "can't locate chunk in skip list")

        /* Check if the chunk already has a memory space */
        if(NULL == chunk_info->mspace) {
            /* Copy the template memory chunk dataspace */
            if(NULL == (chunk_info->mspace = H5S_copy(fm->mchunk_tmpl, FALSE, FALSE)))
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "unable to copy file space")
        } /* end else */

        /* Update the "last chunk seen" information */
        fm->last_index = chunk_index;
        fm->last_chunk_info = chunk_info;
    } /* end else */

    /* Get coordinates of selection iterator for memory */
    if(H5S_SELECT_ITER_COORDS(&fm->mem_iter, coords_in_mem) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "unable to get iterator coordinates")

    /* Add point to memory selection for chunk */
    if(fm->msel_type == H5S_SEL_POINTS) {
        if(H5S_select_elements(chunk_info->mspace, H5S_SELECT_APPEND, (size_t)1, coords_in_mem) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSELECT, FAIL, "unable to select element")
    } /* end if */
    else {
        if(H5S_hyper_add_span_element(chunk_info->mspace, fm->m_ndims, coords_in_mem) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSELECT, FAIL, "unable to select element")
    } /* end else */

    /* Move memory selection iterator to next element in selection */
    if(H5S_SELECT_ITER_NEXT(&fm->mem_iter, (size_t)1) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTNEXT, FAIL, "unable to move to next iterator location")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_mem_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_cacheable
 *
 * Purpose:	A small internal function to if it's possible to load the
 *              chunk into cache.
 *
 * Return:	TRUE or FALSE
 *
 * Programmer:	Raymond Lu
 *		17 July 2007
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5D_chunk_cacheable(const H5D_io_info_t *io_info, haddr_t caddr, hbool_t write_op)
{
    const H5D_t *dataset = io_info->dset;
    htri_t ret_value = FAIL;

    FUNC_ENTER_NOAPI_NOINIT(H5D_chunk_cacheable)

    HDassert(io_info);
    HDassert(dataset);

    /* Must bring the whole chunk in if there are any filters */
    if(dataset->shared->dcpl_cache.pline.nused > 0)
        ret_value = TRUE;
    else {
#ifdef H5_HAVE_PARALLEL
         /* If MPI based VFD is used and the file is opened for write access, must
          *         bypass the chunk-cache scheme because other MPI processes could
          *         be writing to other elements in the same chunk.  Do a direct
          *         write-through of only the elements requested.
          */
        if(io_info->using_mpi_vfd && (H5F_ACC_RDWR & H5F_INTENT(dataset->oloc.file)))
            ret_value = FALSE;
        else {
#endif /* H5_HAVE_PARALLEL */
            /* If the chunk is too large to keep in the cache and if we don't
             * need to write the fill value, then don't load the chunk into the
             * cache, just write the data to it directly.
             */
            H5_CHECK_OVERFLOW(dataset->shared->layout.u.chunk.size, uint32_t, size_t);
            if((size_t)dataset->shared->layout.u.chunk.size > dataset->shared->cache.chunk.nbytes_max) {
                if(write_op && !H5F_addr_defined(caddr)) {
                    const H5O_fill_t *fill = &(dataset->shared->dcpl_cache.fill); /* Fill value info */
                    H5D_fill_value_t fill_status;    /* Fill value status */

                    /* Revtrieve the fill value status */
                    if(H5P_is_fill_value_defined(fill, &fill_status) < 0)
                        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't tell if fill value defined")

                    /* If the fill value needs to be written then we will need
                     * to use the cache to write the fill value */
                    if(fill->fill_time == H5D_FILL_TIME_ALLOC ||
                            (fill->fill_time == H5D_FILL_TIME_IFSET
                            && fill_status == H5D_FILL_VALUE_USER_DEFINED))
                        ret_value = TRUE;
                    else
                        ret_value = FALSE;
                } else
                    ret_value = FALSE;
            } else
                ret_value = TRUE;
#ifdef H5_HAVE_PARALLEL
        } /* end else */
#endif /* H5_HAVE_PARALLEL */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_cacheable() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_in_cache
 *
 * Purpose:	Check if a chunk is in the cache.
 *
 * Return:	TRUE or FALSE
 *
 * Programmer:	Quincey Koziol
 *		1 April 2008
 *
 *-------------------------------------------------------------------------
 */
static hbool_t
H5D_chunk_in_cache(const H5D_t *dset, const hsize_t *chunk_offset,
    hsize_t chunk_idx)
{
    H5D_rdcc_t	*rdcc = &(dset->shared->cache.chunk);/*raw data chunk cache*/
    hbool_t	found = FALSE;		/*already in cache?	*/

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_chunk_in_cache)

    /* Sanity checks */
    HDassert(dset);
    HDassert(chunk_offset);

    /* Check if the chunk is in the cache (but hasn't been written to disk yet) */
    if(rdcc->nslots > 0) {
        unsigned idx = H5D_CHUNK_HASH(dset->shared, chunk_idx); /* Cache entry index */
        H5D_rdcc_ent_t	*ent = rdcc->slot[idx]; /* Cache entry */

        /* Potential match... */
        if(ent) {
            size_t u;           /* Local index variable */

            for(u = 0, found = TRUE; u < dset->shared->layout.u.chunk.ndims; u++) {
                if(chunk_offset[u] != ent->offset[u]) {
                    found = FALSE;
                    break;
                } /* end if */
            } /* end for */
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(found)
} /* end H5D_chunk_in_cache() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_read
 *
 * Purpose:	Read from a chunked dataset.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *		Thursday, April 10, 2003
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_chunk_read(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t UNUSED nelmts, const H5S_t UNUSED *file_space, const H5S_t UNUSED *mem_space,
    H5D_chunk_map_t *fm)
{
    H5SL_node_t *chunk_node;            /* Current node in chunk skip list */
    H5D_io_info_t nonexistent_io_info;  /* "nonexistent" I/O info object */
    H5D_io_info_t ctg_io_info;          /* Contiguous I/O info object */
    H5D_storage_t ctg_store;            /* Chunk storage information as contiguous dataset */
    H5D_io_info_t cpt_io_info;          /* Compact I/O info object */
    H5D_storage_t cpt_store;            /* Chunk storage information as compact dataset */
    hbool_t     cpt_dirty;              /* Temporary placeholder for compact storage "dirty" flag */
    uint32_t    src_accessed_bytes = 0; /* Total accessed size in a chunk */
    hbool_t     skip_missing_chunks = FALSE;    /* Whether to skip missing chunks */
    unsigned    idx_hint = 0;           /* Cache index hint      */
    herr_t	ret_value = SUCCEED;	/*return value		*/

    FUNC_ENTER_NOAPI_NOINIT(H5D_chunk_read)

    /* Sanity check */
    HDassert(io_info);
    HDassert(io_info->u.rbuf);
    HDassert(type_info);
    HDassert(fm);

    /* Set up "nonexistent" I/O info object */
    HDmemcpy(&nonexistent_io_info, io_info, sizeof(nonexistent_io_info));
    nonexistent_io_info.layout_ops = *H5D_LOPS_NONEXISTENT;

    /* Set up contiguous I/O info object */
    HDmemcpy(&ctg_io_info, io_info, sizeof(ctg_io_info));
    ctg_io_info.store = &ctg_store;
    ctg_io_info.layout_ops = *H5D_LOPS_CONTIG;

    /* Initialize temporary contiguous storage info */
    H5_ASSIGN_OVERFLOW(ctg_store.contig.dset_size, io_info->dset->shared->layout.u.chunk.size, uint32_t, hsize_t);

    /* Set up compact I/O info object */
    HDmemcpy(&cpt_io_info, io_info, sizeof(cpt_io_info));
    cpt_io_info.store = &cpt_store;
    cpt_io_info.layout_ops = *H5D_LOPS_COMPACT;

    /* Initialize temporary compact storage info */
    cpt_store.compact.dirty = &cpt_dirty;

    {
        const H5O_fill_t *fill = &(io_info->dset->shared->dcpl_cache.fill);    /* Fill value info */
        H5D_fill_value_t fill_status;       /* Fill value status */

        /* Check the fill value status */
        if(H5P_is_fill_value_defined(fill, &fill_status) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't tell if fill value defined")

        /* If we are never to return fill values, or if we would return them
         * but they aren't set, set the flag to skip missing chunks.
         */
        if(fill->fill_time == H5D_FILL_TIME_NEVER ||
                (fill->fill_time == H5D_FILL_TIME_IFSET && fill_status != H5D_FILL_VALUE_USER_DEFINED))
            skip_missing_chunks = TRUE;
    }

    /* Iterate through nodes in chunk skip list */
    chunk_node = H5D_CHUNK_GET_FIRST_NODE(fm);
    while(chunk_node) {
        H5D_chunk_info_t *chunk_info;   /* Chunk information */
        H5D_io_info_t *chk_io_info;     /* Pointer to I/O info object for this chunk */
        void *chunk;                    /* Pointer to locked chunk buffer */
        H5D_chunk_ud_t udata;		/* B-tree pass-through	*/
        htri_t cacheable;               /* Whether the chunk is cacheable */

        /* Get the actual chunk information from the skip list node */
        chunk_info = H5D_CHUNK_GET_NODE_INFO(fm, chunk_node);

        /* Get the info for the chunk in the file */
        if(H5D_chunk_get_info(io_info->dset, io_info->dxpl_id, chunk_info->coords, &udata) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "error looking up chunk address")

        /* Check for non-existant chunk & skip it if appropriate */
        if(H5F_addr_defined(udata.addr) || H5D_chunk_in_cache(io_info->dset, chunk_info->coords, chunk_info->index)
                || !skip_missing_chunks) {
            /* Load the chunk into cache and lock it. */
            if((cacheable = H5D_chunk_cacheable(io_info, udata.addr, FALSE)) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't tell if chunk is cacheable")
            if(cacheable) {
                /* Pass in chunk's coordinates in a union. */
                io_info->store->chunk.offset = chunk_info->coords;
                io_info->store->chunk.index = chunk_info->index;

                /* Compute # of bytes accessed in chunk */
                H5_CHECK_OVERFLOW(type_info->src_type_size, /*From:*/ size_t, /*To:*/ uint32_t);
                src_accessed_bytes = chunk_info->chunk_points * (uint32_t)type_info->src_type_size;

                /* Lock the chunk into the cache */
                if(NULL == (chunk = H5D_chunk_lock(io_info, &udata, FALSE, &idx_hint)))
                    HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "unable to read raw data chunk")

                /* Set up the storage buffer information for this chunk */
                cpt_store.compact.buf = chunk;

                /* Point I/O info at contiguous I/O info for this chunk */
                chk_io_info = &cpt_io_info;
            } /* end if */
            else if(H5F_addr_defined(udata.addr)) {
                /* Set up the storage address information for this chunk */
                ctg_store.contig.dset_addr = udata.addr;

                /* No chunk cached */
                chunk = NULL;

                /* Point I/O info at temporary I/O info for this chunk */
                chk_io_info = &ctg_io_info;
            } /* end else if */
            else {
                /* No chunk cached */
                chunk = NULL;

                /* Point I/O info at "nonexistent" I/O info for this chunk */
                chk_io_info = &nonexistent_io_info;
            } /* end else */

            /* Perform the actual read operation */
            if((io_info->io_ops.single_read)(chk_io_info, type_info,
                    (hsize_t)chunk_info->chunk_points, chunk_info->fspace, chunk_info->mspace) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "chunked read failed")

            /* Release the cache lock on the chunk. */
            if(chunk && H5D_chunk_unlock(io_info, &udata, FALSE, idx_hint, chunk, src_accessed_bytes) < 0)
                HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "unable to unlock raw data chunk")
        } /* end if */

        /* Advance to next chunk in list */
        chunk_node = H5D_CHUNK_GET_NEXT_NODE(fm, chunk_node);
    } /* end while */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D_chunk_read() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_write
 *
 * Purpose:	Writes to a chunked dataset.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *		Thursday, April 10, 2003
 *
 * Modification:Raymond Lu
 *              4 Feb 2009
 *              One case that was considered cacheable was when the chunk
 *              was bigger than the cache size but not allocated on disk.
 *              I moved it to uncacheable branch to bypass the cache to
 *              improve performance.
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_chunk_write(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t UNUSED nelmts, const H5S_t UNUSED *file_space, const H5S_t UNUSED *mem_space,
    H5D_chunk_map_t *fm)
{
    H5SL_node_t *chunk_node;            /* Current node in chunk skip list */
    H5D_io_info_t ctg_io_info;          /* Contiguous I/O info object */
    H5D_storage_t ctg_store;            /* Chunk storage information as contiguous dataset */
    H5D_io_info_t cpt_io_info;          /* Compact I/O info object */
    H5D_storage_t cpt_store;            /* Chunk storage information as compact dataset */
    hbool_t     cpt_dirty;              /* Temporary placeholder for compact storage "dirty" flag */
    uint32_t    dst_accessed_bytes = 0; /* Total accessed size in a chunk */
    unsigned    idx_hint = 0;           /* Cache index hint      */
    herr_t	ret_value = SUCCEED;	/* Return value		*/

    FUNC_ENTER_NOAPI_NOINIT(H5D_chunk_write)

    /* Sanity check */
    HDassert(io_info);
    HDassert(io_info->u.wbuf);
    HDassert(type_info);
    HDassert(fm);

    /* Set up contiguous I/O info object */
    HDmemcpy(&ctg_io_info, io_info, sizeof(ctg_io_info));
    ctg_io_info.store = &ctg_store;
    ctg_io_info.layout_ops = *H5D_LOPS_CONTIG;

    /* Initialize temporary contiguous storage info */
    H5_ASSIGN_OVERFLOW(ctg_store.contig.dset_size, io_info->dset->shared->layout.u.chunk.size, uint32_t, hsize_t);

    /* Set up compact I/O info object */
    HDmemcpy(&cpt_io_info, io_info, sizeof(cpt_io_info));
    cpt_io_info.store = &cpt_store;
    cpt_io_info.layout_ops = *H5D_LOPS_COMPACT;

    /* Initialize temporary compact storage info */
    cpt_store.compact.dirty = &cpt_dirty;

    /* Iterate through nodes in chunk skip list */
    chunk_node = H5D_CHUNK_GET_FIRST_NODE(fm);
    while(chunk_node) {
        H5D_chunk_info_t *chunk_info;   /* Chunk information */
        H5D_io_info_t *chk_io_info;     /* Pointer to I/O info object for this chunk */
        void *chunk;                    /* Pointer to locked chunk buffer */
        H5D_chunk_ud_t udata;		/* Index pass-through	*/
        htri_t cacheable;               /* Whether the chunk is cacheable */

        /* Get the actual chunk information from the skip list node */
        chunk_info = H5D_CHUNK_GET_NODE_INFO(fm, chunk_node);

        /* Load the chunk into cache.  But if the whole chunk is written,
         * simply allocate space instead of load the chunk. */
        if(H5D_chunk_get_info(io_info->dset, io_info->dxpl_id, chunk_info->coords, &udata) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "error looking up chunk address")
        if((cacheable = H5D_chunk_cacheable(io_info, udata.addr, TRUE)) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't tell if chunk is cacheable")
        if(cacheable) {
            hbool_t entire_chunk = TRUE;       /* Whether whole chunk is selected */

            /* Pass in chunk's coordinates in a union. */
            io_info->store->chunk.offset = chunk_info->coords;
            io_info->store->chunk.index = chunk_info->index;

            /* Compute # of bytes accessed in chunk */
            H5_CHECK_OVERFLOW(type_info->dst_type_size, /*From:*/ size_t, /*To:*/ uint32_t);
            dst_accessed_bytes = chunk_info->chunk_points * (uint32_t)type_info->dst_type_size;

            /* Determine if we will access all the data in the chunk */
            if(dst_accessed_bytes != ctg_store.contig.dset_size ||
                    (chunk_info->chunk_points * type_info->src_type_size) != ctg_store.contig.dset_size)
                entire_chunk = FALSE;

            /* Lock the chunk into the cache */
            if(NULL == (chunk = H5D_chunk_lock(io_info, &udata, entire_chunk, &idx_hint)))
                HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "unable to read raw data chunk")

            /* Set up the storage buffer information for this chunk */
            cpt_store.compact.buf = chunk;

            /* Point I/O info at main I/O info for this chunk */
            chk_io_info = &cpt_io_info;
        } /* end if */
        else {
            /* If the chunk hasn't been allocated on disk, do so now. */
            if(!H5F_addr_defined(udata.addr)) {
                H5D_chk_idx_info_t idx_info;        /* Chunked index info */

                /* Compose chunked index info struct */
                idx_info.f = io_info->dset->oloc.file;
                idx_info.dxpl_id = io_info->dxpl_id;
                idx_info.pline = &(io_info->dset->shared->dcpl_cache.pline);
                idx_info.layout = &(io_info->dset->shared->layout.u.chunk);
                idx_info.storage = &(io_info->dset->shared->layout.storage.u.chunk);

                /* Set up the size of chunk for user data */
                udata.nbytes = io_info->dset->shared->layout.u.chunk.size;

                /* Create the chunk */
                if((io_info->dset->shared->layout.storage.u.chunk.ops->insert)(&idx_info, &udata) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, FAIL, "unable to insert/resize chunk")

                /* Make sure the address of the chunk is returned. */
                if(!H5F_addr_defined(udata.addr))
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "chunk address isn't defined")

                /* Cache the new chunk information */
                H5D_chunk_cinfo_cache_update(&io_info->dset->shared->cache.chunk.last, &udata);
            } /* end if */

            /* Set up the storage address information for this chunk */
            ctg_store.contig.dset_addr = udata.addr;

            /* No chunk cached */
            chunk = NULL;

            /* Point I/O info at temporary I/O info for this chunk */
            chk_io_info = &ctg_io_info;
        } /* end else */

        /* Perform the actual write operation */
        if((io_info->io_ops.single_write)(chk_io_info, type_info,
                (hsize_t)chunk_info->chunk_points, chunk_info->fspace, chunk_info->mspace) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "chunked write failed")

        /* Release the cache lock on the chunk. */
        if(chunk && H5D_chunk_unlock(io_info, &udata, TRUE, idx_hint, chunk, dst_accessed_bytes) < 0)
            HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "unable to unlock raw data chunk")

        /* Advance to next chunk in list */
        chunk_node = H5D_CHUNK_GET_NEXT_NODE(fm, chunk_node);
    } /* end while */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D_chunk_write() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_flush
 *
 * Purpose:	Writes all dirty chunks to disk and optionally preempts them
 *		from the cache.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, May 21, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_chunk_flush(H5D_t *dset, hid_t dxpl_id)
{
    H5D_dxpl_cache_t _dxpl_cache;       /* Data transfer property cache buffer */
    H5D_dxpl_cache_t *dxpl_cache = &_dxpl_cache;   /* Data transfer property cache */
    H5D_rdcc_t *rdcc = &(dset->shared->cache.chunk);
    H5D_rdcc_ent_t	*ent, *next;
    unsigned		nerrors = 0;    /* Count of any errors encountered when flushing chunks */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_chunk_flush)

    /* Sanity check */
    HDassert(dset);

    /* Flush any data caught in sieve buffer */
    if(H5D_flush_sieve_buf(dset, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTFLUSH, FAIL, "unable to flush sieve buffer")

    /* Fill the DXPL cache values for later use */
    if(H5D_get_dxpl_cache(dxpl_id, &dxpl_cache) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't fill dxpl cache")

    /* Loop over all entries in the chunk cache */
    for(ent = rdcc->head; ent; ent = next) {
	next = ent->next;
        if(H5D_chunk_flush_entry(dset, dxpl_id, dxpl_cache, ent, FALSE) < 0)
            nerrors++;
    } /* end for */
    if(nerrors)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTFLUSH, FAIL, "unable to flush one or more raw data chunks")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_flush() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_io_term
 *
 * Purpose:	Destroy I/O operation information.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Saturday, May 17, 2003
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_chunk_io_term(const H5D_chunk_map_t *fm)
{
    herr_t	ret_value = SUCCEED;	/*return value		*/

    FUNC_ENTER_NOAPI_NOINIT(H5D_chunk_io_term)

    /* Single element I/O vs. multiple element I/O cleanup */
    if(fm->use_single) {
        /* Sanity checks */
        HDassert(fm->sel_chunks == NULL);
        HDassert(fm->single_chunk_info);
        HDassert(fm->single_chunk_info->fspace_shared);
        HDassert(fm->single_chunk_info->mspace_shared);

        /* Reset the selection for the single element I/O */
        H5S_select_all(fm->single_space, TRUE);
    } /* end if */
    else {
        /* Release the nodes on the list of selected chunks */
        if(fm->sel_chunks)
            if(H5SL_free(fm->sel_chunks, H5D_free_chunk_info, NULL) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTNEXT, FAIL, "can't iterate over chunks")
    } /* end else */

    /* Free the memory chunk dataspace template */
    if(fm->mchunk_tmpl)
        if(H5S_close(fm->mchunk_tmpl) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "can't release memory chunk dataspace template")
#ifdef H5_HAVE_PARALLEL
    if(fm->select_chunk)
        H5MM_xfree(fm->select_chunk);
#endif /* H5_HAVE_PARALLEL */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_io_term() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_idx_reset
 *
 * Purpose:	Reset index information
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, January 15, 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_idx_reset(H5O_storage_chunk_t *storage, hbool_t reset_addr)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5D_chunk_idx_reset, FAIL)

    /* Sanity checks */
    HDassert(storage);
    HDassert(storage->ops);

    /* Reset index structures */
    if((storage->ops->reset)(storage, reset_addr) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "unable to reset chunk index info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_idx_reset() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_cinfo_cache_reset
 *
 * Purpose:	Reset the cached chunk info
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              November 27, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_cinfo_cache_reset(H5D_chunk_cached_t *last)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_chunk_cinfo_cache_reset)

    /* Sanity check */
    HDassert(last);

    /* Indicate that the cached info is not valid */
    last->valid = FALSE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5D_chunk_cinfo_cache_reset() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_cinfo_cache_update
 *
 * Purpose:	Update the cached chunk info
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              November 27, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_chunk_cinfo_cache_update(H5D_chunk_cached_t *last, const H5D_chunk_ud_t *udata)
{
    unsigned    u;                              /* Local index variable */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_chunk_cinfo_cache_update)

    /* Sanity check */
    HDassert(last);
    HDassert(udata);
    HDassert(udata->common.layout);
    HDassert(udata->common.storage);
    HDassert(udata->common.offset);

    /* Stored the information to cache */
    for(u = 0; u < udata->common.layout->ndims; u++)
        last->offset[u] = udata->common.offset[u];
    last->nbytes = udata->nbytes;
    last->filter_mask = udata->filter_mask;
    last->addr = udata->addr;

    /* Indicate that the cached info is valid */
    last->valid = TRUE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5D_chunk_cinfo_cache_update() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_cinfo_cache_found
 *
 * Purpose:	Look for chunk info in cache
 *
 * Return:	TRUE/FALSE/FAIL
 *
 * Programmer:	Quincey Koziol
 *              November 27, 2007
 *
 *-------------------------------------------------------------------------
 */
static hbool_t
H5D_chunk_cinfo_cache_found(const H5D_chunk_cached_t *last, H5D_chunk_ud_t *udata)
{
    hbool_t ret_value = FALSE;          /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_chunk_cinfo_cache_found)

    /* Sanity check */
    HDassert(last);
    HDassert(udata);
    HDassert(udata->common.layout);
    HDassert(udata->common.storage);
    HDassert(udata->common.offset);

    /* Check if the cached information is what is desired */
    if(last->valid) {
        unsigned    u;                      /* Local index variable */

        /* Check that the offset is the same */
        for(u = 0; u < udata->common.layout->ndims; u++)
            if(last->offset[u] != udata->common.offset[u])
                HGOTO_DONE(FALSE)

        /* Retrieve the information from the cache */
        udata->nbytes = last->nbytes;
        udata->filter_mask = last->filter_mask;
        udata->addr = last->addr;

        /* Indicate that the data was found */
        HGOTO_DONE(TRUE)
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D_chunk_cinfo_cache_found() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_create
 *
 * Purpose:	Creates a new chunked storage index and initializes the
 *		layout information with information about the storage.  The
 *		layout info should be immediately written to the object header.
 *
 * Return:	Non-negative on success (with the layout information initialized
 *		and ready to write to an object header). Negative on failure.
 *
 * Programmer:	Quincey Koziol
 *		Thursday, May 22, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_create(H5D_t *dset /*in,out*/, hid_t dxpl_id)
{
    H5D_chk_idx_info_t idx_info;        /* Chunked index info */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5D_chunk_create, FAIL)

    /* Check args */
    HDassert(dset);
    HDassert(H5D_CHUNKED == dset->shared->layout.type);
    HDassert(dset->shared->layout.u.chunk.ndims > 0 && dset->shared->layout.u.chunk.ndims <= H5O_LAYOUT_NDIMS);
#ifndef NDEBUG
{
    unsigned u;                         /* Local index variable */

    for(u = 0; u < dset->shared->layout.u.chunk.ndims; u++)
	HDassert(dset->shared->layout.u.chunk.dim[u] > 0);
}
#endif

    /* Compose chunked index info struct */
    idx_info.f = dset->oloc.file;
    idx_info.dxpl_id = dxpl_id;
    idx_info.pline = &dset->shared->dcpl_cache.pline;
    idx_info.layout = &dset->shared->layout.u.chunk;
    idx_info.storage = &dset->shared->layout.storage.u.chunk;

    /* Create the index for the chunks */
    if((dset->shared->layout.storage.u.chunk.ops->create)(&idx_info) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't create chunk index")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_create() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_get_info
 *
 * Purpose:	Get the info about a chunk if file space has been
 *		assigned.  Save the retrieved information in the udata
 *		supplied.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Albert Cheng
 *              June 27, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_get_info(const H5D_t *dset, hid_t dxpl_id, const hsize_t *chunk_offset,
    H5D_chunk_ud_t *udata)
{
    herr_t	ret_value = SUCCEED;	/* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_chunk_get_info)

    HDassert(dset);
    HDassert(dset->shared->layout.u.chunk.ndims > 0);
    HDassert(chunk_offset);
    HDassert(udata);

    /* Initialize the query information about the chunk we are looking for */
    udata->common.layout = &(dset->shared->layout.u.chunk);
    udata->common.storage = &(dset->shared->layout.storage.u.chunk);
    udata->common.offset = chunk_offset;

    /* Reset information about the chunk we are looking for */
    udata->nbytes = 0;
    udata->filter_mask = 0;
    udata->addr = HADDR_UNDEF;

    /* Check for cached information */
    if(!H5D_chunk_cinfo_cache_found(&dset->shared->cache.chunk.last, udata)) {
        H5D_chk_idx_info_t idx_info;        /* Chunked index info */

        /* Compose chunked index info struct */
        idx_info.f = dset->oloc.file;
        idx_info.dxpl_id = dxpl_id;
        idx_info.pline = &dset->shared->dcpl_cache.pline;
        idx_info.layout = &dset->shared->layout.u.chunk;
        idx_info.storage = &dset->shared->layout.storage.u.chunk;

        /* Go get the chunk information */
        if((dset->shared->layout.storage.u.chunk.ops->get_addr)(&idx_info, udata) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't query chunk address")

        /* Cache the information retrieved */
        H5D_chunk_cinfo_cache_update(&dset->shared->cache.chunk.last, udata);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D_chunk_get_info() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_flush_entry
 *
 * Purpose:	Writes a chunk to disk.  If RESET is non-zero then the
 *		entry is cleared -- it's slightly faster to flush a chunk if
 *		the RESET flag is turned on because it results in one fewer
 *		memory copy.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, May 21, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_chunk_flush_entry(const H5D_t *dset, hid_t dxpl_id, const H5D_dxpl_cache_t *dxpl_cache,
    H5D_rdcc_ent_t *ent, hbool_t reset)
{
    void	*buf = NULL;	        /* Temporary buffer		*/
    hbool_t	point_of_no_return = FALSE;
    herr_t	ret_value = SUCCEED;	/* Return value			*/

    FUNC_ENTER_NOAPI_NOINIT(H5D_chunk_flush_entry)

    HDassert(dset);
    HDassert(dset->shared);
    HDassert(dxpl_cache);
    HDassert(ent);
    HDassert(!ent->locked);

    buf = ent->chunk;
    if(ent->dirty && !ent->deleted) {
        H5D_chunk_ud_t 	udata;		/* pass through B-tree		*/
        hbool_t must_insert = FALSE;    /* Whether the chunk must go through the "insert" method */

        /* Set up user data for index callbacks */
        udata.common.layout = &dset->shared->layout.u.chunk;
        udata.common.storage = &dset->shared->layout.storage.u.chunk;
        udata.common.offset = ent->offset;
        udata.filter_mask = 0;
        udata.nbytes = dset->shared->layout.u.chunk.size;
        udata.addr = ent->chunk_addr;

        /* Should the chunk be filtered before writing it to disk? */
        if(dset->shared->dcpl_cache.pline.nused) {
            size_t alloc = udata.nbytes;        /* Bytes allocated for BUF	*/
            size_t nbytes;                      /* Chunk size (in bytes) */

            if(!reset) {
                /*
                 * Copy the chunk to a new buffer before running it through
                 * the pipeline because we'll want to save the original buffer
                 * for later.
                 */
                H5_ASSIGN_OVERFLOW(alloc, udata.nbytes, uint32_t, size_t);
                if(NULL == (buf = H5MM_malloc(alloc)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for pipeline")
                HDmemcpy(buf, ent->chunk, udata.nbytes);
            } /* end if */
            else {
                /*
                 * If we are reseting and something goes wrong after this
                 * point then it's too late to recover because we may have
                 * destroyed the original data by calling H5Z_pipeline().
                 * The only safe option is to continue with the reset
                 * even if we can't write the data to disk.
                 */
                point_of_no_return = TRUE;
                ent->chunk = NULL;
            } /* end else */
            H5_ASSIGN_OVERFLOW(nbytes, udata.nbytes, uint32_t, size_t);
            if(H5Z_pipeline(&(dset->shared->dcpl_cache.pline), 0, &(udata.filter_mask), dxpl_cache->err_detect,
                     dxpl_cache->filter_cb, &nbytes, &alloc, &buf) < 0)
                HGOTO_ERROR(H5E_PLINE, H5E_CANTFILTER, FAIL, "output pipeline failed")
#if H5_SIZEOF_SIZE_T > 4
            /* Check for the chunk expanding too much to encode in a 32-bit value */
            if(nbytes > ((size_t)0xffffffff))
                HGOTO_ERROR(H5E_DATASET, H5E_BADRANGE, FAIL, "chunk too large for 32-bit length")
#endif /* H5_SIZEOF_SIZE_T > 4 */
            H5_ASSIGN_OVERFLOW(udata.nbytes, nbytes, size_t, uint32_t);

            /* Indicate that the chunk must go through 'insert' method */
            must_insert = TRUE;
        } /* end if */
        else if(!H5F_addr_defined(udata.addr))
            /* Indicate that the chunk must go through 'insert' method */
            must_insert = TRUE;

        /* Check if the chunk needs to be 'inserted' (could exist already and
         *      the 'insert' operation could resize it)
         */
        if(must_insert) {
            H5D_chk_idx_info_t idx_info;        /* Chunked index info */

            /* Compose chunked index info struct */
            idx_info.f = dset->oloc.file;
            idx_info.dxpl_id = dxpl_id;
            idx_info.pline = &dset->shared->dcpl_cache.pline;
            idx_info.layout = &dset->shared->layout.u.chunk;
            idx_info.storage = &dset->shared->layout.storage.u.chunk;

            /* Create the chunk it if it doesn't exist, or reallocate the chunk
             *  if its size changed.
             */
            if((dset->shared->layout.storage.u.chunk.ops->insert)(&idx_info, &udata) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, FAIL, "unable to insert/resize chunk")

            /* Update the chunk entry's address, in case it was allocated or relocated */
            ent->chunk_addr = udata.addr;
        } /* end if */

        /* Write the data to the file */
        HDassert(H5F_addr_defined(udata.addr));
        if(H5F_block_write(dset->oloc.file, H5FD_MEM_DRAW, udata.addr, udata.nbytes, dxpl_id, buf) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to write raw data to file")

        /* Cache the chunk's info, in case it's accessed again shortly */
        H5D_chunk_cinfo_cache_update(&dset->shared->cache.chunk.last, &udata);

        /* Mark cache entry as clean */
        ent->dirty = FALSE;

        /* Increment # of flushed entries */
        dset->shared->cache.chunk.stats.nflushes++;
    } /* end if */

    /* Reset, but do not free or removed from list */
    if(reset) {
        point_of_no_return = FALSE;
        if(buf == ent->chunk)
            buf = NULL;
        if(ent->chunk != NULL)
            ent->chunk = (uint8_t *)H5D_chunk_xfree(ent->chunk, &(dset->shared->dcpl_cache.pline));
    } /* end if */

done:
    /* Free the temp buffer only if it's different than the entry chunk */
    if(buf != ent->chunk)
        H5MM_xfree(buf);

    /*
     * If we reached the point of no return then we have no choice but to
     * reset the entry.  This can only happen if RESET is true but the
     * output pipeline failed.  Do not free the entry or remove it from the
     * list.
     */
    if(ret_value < 0 && point_of_no_return) {
        if(ent->chunk)
            ent->chunk = (uint8_t *)H5D_chunk_xfree(ent->chunk, &(dset->shared->dcpl_cache.pline));
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_flush_entry() */


/*-------------------------------------------------------------------------
 * Function:    H5D_chunk_cache_evict
 *
 * Purpose:     Preempts the specified entry from the cache, flushing it to
 *              disk if necessary.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, May 21, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_chunk_cache_evict(const H5D_t *dset, hid_t dxpl_id, const H5D_dxpl_cache_t *dxpl_cache,
    H5D_rdcc_ent_t *ent, hbool_t flush)
{
    H5D_rdcc_t *rdcc = &(dset->shared->cache.chunk);
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_chunk_cache_evict)

    HDassert(dset);
    HDassert(dxpl_cache);
    HDassert(ent);
    HDassert(!ent->locked);
    HDassert(ent->idx < rdcc->nslots);

    if(flush) {
	/* Flush */
	if(H5D_chunk_flush_entry(dset, dxpl_id, dxpl_cache, ent, TRUE) < 0)
	    HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "cannot flush indexed storage buffer")
    } /* end if */
    else {
        /* Don't flush, just free chunk */
	if(ent->chunk != NULL)
	    ent->chunk = (uint8_t *)H5D_chunk_xfree(ent->chunk, &(dset->shared->dcpl_cache.pline));
    } /* end else */

    /* Unlink from list */
    if(ent->prev)
	ent->prev->next = ent->next;
    else
	rdcc->head = ent->next;
    if(ent->next)
	ent->next->prev = ent->prev;
    else
	rdcc->tail = ent->prev;
    ent->prev = ent->next = NULL;

    /* Remove from cache */
    rdcc->slot[ent->idx] = NULL;
    ent->idx = UINT_MAX;
    rdcc->nbytes_used -= dset->shared->layout.u.chunk.size;
    --rdcc->nused;

    /* Free */
    ent = H5FL_FREE(H5D_rdcc_ent_t, ent);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_cache_evict() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_cache_prune
 *
 * Purpose:	Prune the cache by preempting some things until the cache has
 *		room for something which is SIZE bytes.  Only unlocked
 *		entries are considered for preemption.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, May 21, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_chunk_cache_prune(const H5D_t *dset, hid_t dxpl_id,
    const H5D_dxpl_cache_t *dxpl_cache, size_t size)
{
    const H5D_rdcc_t	*rdcc = &(dset->shared->cache.chunk);
    size_t		total = rdcc->nbytes_max;
    const int		nmeth = 2;	/*number of methods		*/
    int		        w[1];		/*weighting as an interval	*/
    H5D_rdcc_ent_t	*p[2], *cur;	/*list pointers			*/
    H5D_rdcc_ent_t	*n[2];		/*list next pointers		*/
    int		nerrors = 0;            /* Accumulated error count during preemptions */
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_chunk_cache_prune)

    /*
     * Preemption is accomplished by having multiple pointers (currently two)
     * slide down the list beginning at the head. Pointer p(N+1) will start
     * traversing the list when pointer pN reaches wN percent of the original
     * list.  In other words, preemption method N gets to consider entries in
     * approximate least recently used order w0 percent before method N+1
     * where 100% means tha method N will run to completion before method N+1
     * begins.  The pointers participating in the list traversal are each
     * given a chance at preemption before any of the pointers are advanced.
     */
    w[0] = (int)(rdcc->nused * rdcc->w0);
    p[0] = rdcc->head;
    p[1] = NULL;

    while((p[0] || p[1]) && (rdcc->nbytes_used + size) > total) {
        int i;          /* Local index variable */

	/* Introduce new pointers */
	for(i = 0; i < nmeth - 1; i++)
            if(0 == w[i])
                p[i + 1] = rdcc->head;

	/* Compute next value for each pointer */
	for(i = 0; i < nmeth; i++)
            n[i] = p[i] ? p[i]->next : NULL;

	/* Give each method a chance */
	for(i = 0; i < nmeth && (rdcc->nbytes_used + size) > total; i++) {
	    if(0 == i && p[0] && !p[0]->locked &&
                    ((0 == p[0]->rd_count && 0 == p[0]->wr_count) ||
                     (0 == p[0]->rd_count && dset->shared->layout.u.chunk.size == p[0]->wr_count) ||
                     (dset->shared->layout.u.chunk.size == p[0]->rd_count && 0 == p[0]->wr_count))) {
		/*
		 * Method 0: Preempt entries that have been completely written
		 * and/or completely read but not entries that are partially
		 * written or partially read.
		 */
		cur = p[0];
	    } else if(1 == i && p[1] && !p[1]->locked) {
		/*
		 * Method 1: Preempt the entry without regard to
		 * considerations other than being locked.  This is the last
		 * resort preemption.
		 */
		cur = p[1];
	    } else {
		/* Nothing to preempt at this point */
		cur = NULL;
	    }

	    if(cur) {
                int j;          /* Local index variable */

		for(j = 0; j < nmeth; j++) {
		    if(p[j] == cur)
                        p[j] = NULL;
		    if(n[j] == cur)
                        n[j] = cur->next;
		} /* end for */
		if(H5D_chunk_cache_evict(dset, dxpl_id, dxpl_cache, cur, TRUE) < 0)
                    nerrors++;
	    } /* end if */
	} /* end for */

	/* Advance pointers */
	for(i = 0; i < nmeth; i++)
            p[i] = n[i];
	for(i = 0; i < nmeth - 1; i++)
            w[i] -= 1;
    } /* end while */

    if(nerrors)
	HGOTO_ERROR(H5E_IO, H5E_CANTFLUSH, FAIL, "unable to preempt one or more raw data cache entry")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_cache_prune() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_lock
 *
 * Purpose:	Return a pointer to a dataset chunk.  The pointer points
 *		directly into the chunk cache and should not be freed
 *		by the caller but will be valid until it is unlocked.  The
 *		input value IDX_HINT is used to speed up cache lookups and
 *		it's output value should be given to H5D_chunk_unlock().
 *		IDX_HINT is ignored if it is out of range, and if it points
 *		to the wrong entry then we fall back to the normal search
 *		method.
 *
 *		If RELAX is non-zero and the chunk isn't in the cache then
 *		don't try to read it from the file, but just allocate an
 *		uninitialized buffer to hold the result.  This is intended
 *		for output functions that are about to overwrite the entire
 *		chunk.
 *
 * Return:	Success:	Ptr to a file chunk.
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Thursday, May 21, 1998
 *
 *-------------------------------------------------------------------------
 */
void *
H5D_chunk_lock(const H5D_io_info_t *io_info, H5D_chunk_ud_t *udata,
    hbool_t relax, unsigned *idx_hint/*in,out*/)
{
    H5D_t *dset = io_info->dset;                /* Local pointer to the dataset info */
    const H5O_pline_t  *pline = &(dset->shared->dcpl_cache.pline);    /* I/O pipeline info */
    const H5O_layout_t *layout = &(dset->shared->layout);       /* Dataset layout */
    const H5O_fill_t    *fill = &(dset->shared->dcpl_cache.fill);    /* Fill value info */
    H5D_fill_buf_info_t fb_info;                /* Dataset's fill buffer info */
    hbool_t             fb_info_init = FALSE;   /* Whether the fill value buffer has been initialized */
    H5D_rdcc_t		*rdcc = &(dset->shared->cache.chunk);   /*raw data chunk cache*/
    H5D_rdcc_ent_t	*ent = NULL;		/*cache entry		*/
    unsigned		idx = 0;		/*hash index number	*/
    hbool_t		found = FALSE;		/*already in cache?	*/
    haddr_t             chunk_addr = HADDR_UNDEF; /* Address of chunk on disk */
    size_t		chunk_size;		/*size of a chunk	*/
    void		*chunk = NULL;		/*the file chunk	*/
    unsigned		u;			/*counters		*/
    void		*ret_value;	        /*return value		*/

    FUNC_ENTER_NOAPI_NOINIT(H5D_chunk_lock)

    HDassert(io_info);
    HDassert(io_info->dxpl_cache);
    HDassert(io_info->store);
    HDassert(udata);
    HDassert(dset);
    HDassert(TRUE == H5P_isa_class(io_info->dxpl_id, H5P_DATASET_XFER));

    /* Get the chunk's size */
    HDassert(layout->u.chunk.size > 0);
    H5_ASSIGN_OVERFLOW(chunk_size, layout->u.chunk.size, uint32_t, size_t);

    /* Search for the chunk in the cache */
    if(rdcc->nslots > 0) {
        idx = H5D_CHUNK_HASH(dset->shared, io_info->store->chunk.index);
        ent = rdcc->slot[idx];

        if(ent)
            for(u = 0, found = TRUE; u < layout->u.chunk.ndims; u++)
                if(io_info->store->chunk.offset[u] != ent->offset[u]) {
                    found = FALSE;
                    break;
                } /* end if */
    } /* end if */

    if(found) {
        /*
         * Already in the cache.  Count a hit.
         */
        rdcc->stats.nhits++;
    } /* end if */
    else if(relax) {
        /*
         * Not in the cache, but we're about to overwrite the whole thing
         * anyway, so just allocate a buffer for it but don't initialize that
         * buffer with the file contents. Count this as a hit instead of a
         * miss because we saved ourselves lots of work.
         */
        rdcc->stats.nhits++;

        /* Still save the chunk address so the cache stays consistent */
        chunk_addr = udata->addr;

        if(NULL == (chunk = H5D_chunk_alloc(chunk_size, pline)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for raw data chunk")

        /* In the case that some dataset functions look through this data,
         * clear it to all 0s. */
        HDmemset(chunk, 0, chunk_size);
    } /* end if */
    else {
        /*
         * Not in the cache.  Count this as a miss if it's in the file
         *      or an init if it isn't.
         */

        /* Save the chunk address */
        chunk_addr = udata->addr;

        /* Check if the chunk exists on disk */
        if(H5F_addr_defined(chunk_addr)) {
            size_t		chunk_alloc = 0;		/*allocated chunk size	*/

            /* Chunk size on disk isn't [likely] the same size as the final chunk
             * size in memory, so allocate memory big enough. */
            H5_ASSIGN_OVERFLOW(chunk_alloc, udata->nbytes, uint32_t, size_t);
            if(NULL == (chunk = H5D_chunk_alloc(chunk_alloc, pline)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for raw data chunk")
            if(H5F_block_read(dset->oloc.file, H5FD_MEM_DRAW, chunk_addr, chunk_alloc, io_info->dxpl_id, chunk) < 0)
                HGOTO_ERROR(H5E_IO, H5E_READERROR, NULL, "unable to read raw data chunk")

            if(pline->nused) {
                if(H5Z_pipeline(pline, H5Z_FLAG_REVERSE, &(udata->filter_mask), io_info->dxpl_cache->err_detect,
                        io_info->dxpl_cache->filter_cb, &chunk_alloc, &chunk_alloc, &chunk) < 0)
                    HGOTO_ERROR(H5E_PLINE, H5E_CANTFILTER, NULL, "data pipeline read failed")
                H5_ASSIGN_OVERFLOW(udata->nbytes, chunk_alloc, size_t, uint32_t);
            } /* end if */

            /* Increment # of cache misses */
            rdcc->stats.nmisses++;
        } /* end if */
        else {
            H5D_fill_value_t	fill_status;

            /* Chunk size on disk isn't [likely] the same size as the final chunk
             * size in memory, so allocate memory big enough. */
            if(NULL == (chunk = H5D_chunk_alloc(chunk_size, pline)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for raw data chunk")

            if(H5P_is_fill_value_defined(fill, &fill_status) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't tell if fill value defined")

            if(fill->fill_time == H5D_FILL_TIME_ALLOC ||
                    (fill->fill_time == H5D_FILL_TIME_IFSET && fill_status == H5D_FILL_VALUE_USER_DEFINED)) {
                /*
                 * The chunk doesn't exist in the file.  Replicate the fill
                 * value throughout the chunk, if the fill value is defined.
                 */

                /* Initialize the fill value buffer */
                /* (use the compact dataset storage buffer as the fill value buffer) */
                if(H5D_fill_init(&fb_info, chunk, FALSE,
                        NULL, NULL, NULL, NULL,
                        &dset->shared->dcpl_cache.fill, dset->shared->type,
                        dset->shared->type_id, (size_t)0, chunk_size, io_info->dxpl_id) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "can't initialize fill buffer info")
                fb_info_init = TRUE;

                /* Check for VL datatype & non-default fill value */
                if(fb_info.has_vlen_fill_type)
                    /* Fill the buffer with VL datatype fill values */
                    if(H5D_fill_refill_vl(&fb_info, fb_info.elmts_per_buf, io_info->dxpl_id) < 0)
                        HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, NULL, "can't refill fill value buffer")
            } /* end if */
            else
                HDmemset(chunk, 0, chunk_size);

            /* Increment # of creations */
            rdcc->stats.ninits++;
        } /* end else */
    } /* end else */
    HDassert(found || chunk_size > 0);

    if(!found && rdcc->nslots > 0 && chunk_size <= rdcc->nbytes_max &&
            (!ent || !ent->locked)) {
        /*
         * Add the chunk to the cache only if the slot is not already locked.
         * Preempt enough things from the cache to make room.
         */
        if(ent) {
            if(H5D_chunk_cache_evict(io_info->dset, io_info->dxpl_id, io_info->dxpl_cache, ent, TRUE) < 0)
                HGOTO_ERROR(H5E_IO, H5E_CANTINIT, NULL, "unable to preempt chunk from cache")
        } /* end if */
        if(H5D_chunk_cache_prune(io_info->dset, io_info->dxpl_id, io_info->dxpl_cache, chunk_size) < 0)
            HGOTO_ERROR(H5E_IO, H5E_CANTINIT, NULL, "unable to preempt chunk(s) from cache")

        /* Create a new entry */
        if(NULL == (ent = H5FL_MALLOC(H5D_rdcc_ent_t)))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, NULL, "can't allocate raw data chunk entry")

        ent->locked = 0;
        ent->dirty = FALSE;
        ent->deleted = FALSE;
        ent->chunk_addr = chunk_addr;
        for(u = 0; u < layout->u.chunk.ndims; u++)
            ent->offset[u] = io_info->store->chunk.offset[u];
        H5_ASSIGN_OVERFLOW(ent->rd_count, chunk_size, size_t, uint32_t);
        H5_ASSIGN_OVERFLOW(ent->wr_count, chunk_size, size_t, uint32_t);
        ent->chunk = (uint8_t *)chunk;

        /* Add it to the cache */
        HDassert(NULL == rdcc->slot[idx]);
        rdcc->slot[idx] = ent;
        ent->idx = idx;
        rdcc->nbytes_used += chunk_size;
        rdcc->nused++;

        /* Add it to the linked list */
        ent->next = NULL;
        if(rdcc->tail) {
            rdcc->tail->next = ent;
            ent->prev = rdcc->tail;
            rdcc->tail = ent;
        } /* end if */
        else {
            rdcc->head = rdcc->tail = ent;
            ent->prev = NULL;
        } /* end else */

        /* Indicate that the chunk is in the cache now */
        found = TRUE;
    } else if(!found) {
        /*
         * The chunk is larger than the entire cache so we don't cache it.
         * This is the reason all those arguments have to be repeated for the
         * unlock function.
         */
        ent = NULL;
        idx = UINT_MAX;
    } else {
        /*
         * The chunk is not at the beginning of the cache; move it backward
         * by one slot.  This is how we implement the LRU preemption
         * algorithm.
         */
        HDassert(ent);
        if(ent->next) {
            if(ent->next->next)
                ent->next->next->prev = ent;
            else
                rdcc->tail = ent;
            ent->next->prev = ent->prev;
            if(ent->prev)
                ent->prev->next = ent->next;
            else
                rdcc->head = ent->next;
            ent->prev = ent->next;
            ent->next = ent->next->next;
            ent->prev->next = ent;
        } /* end if */
    } /* end else */

    /* Lock the chunk into the cache */
    if(ent) {
        HDassert(!ent->locked);
        ent->locked = TRUE;
        chunk = ent->chunk;
    } /* end if */

    if(idx_hint)
        *idx_hint = idx;

    /* Set return value */
    ret_value = chunk;

done:
    /* Release the fill buffer info, if it's been initialized */
    if(fb_info_init && H5D_fill_term(&fb_info) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, NULL, "Can't release fill buffer info")

    /* Release the chunk allocated, on error */
    if(!ret_value)
        if(chunk)
            chunk = H5D_chunk_xfree(chunk, pline);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_lock() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_unlock
 *
 * Purpose:	Unlocks a previously locked chunk. The LAYOUT, COMP, and
 *		OFFSET arguments should be the same as for H5D_chunk_lock().
 *		The DIRTY argument should be set to non-zero if the chunk has
 *		been modified since it was locked. The IDX_HINT argument is
 *		the returned index hint from the lock operation and BUF is
 *		the return value from the lock.
 *
 *		The NACCESSED argument should be the number of bytes accessed
 *		for reading or writing (depending on the value of DIRTY).
 *		It's only purpose is to provide additional information to the
 *		preemption policy.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, May 21, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_unlock(const H5D_io_info_t *io_info, const H5D_chunk_ud_t *udata,
    hbool_t dirty, unsigned idx_hint, void *chunk, uint32_t naccessed)
{
    const H5O_layout_t *layout = &(io_info->dset->shared->layout); /* Dataset layout */
    const H5D_rdcc_t	*rdcc = &(io_info->dset->shared->cache.chunk);
    herr_t              ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_chunk_unlock)

    HDassert(io_info);
    HDassert(udata);

    if(UINT_MAX == idx_hint) {
        /*
         * It's not in the cache, probably because it's too big.  If it's
         * dirty then flush it to disk.  In any case, free the chunk.
         * Note: we have to copy the layout and filter messages so we
         *	 don't discard the `const' qualifier.
         */
        if(dirty) {
            H5D_rdcc_ent_t fake_ent;         /* "fake" chunk cache entry */

            HDmemset(&fake_ent, 0, sizeof(fake_ent));
            fake_ent.dirty = TRUE;
            HDmemcpy(fake_ent.offset, io_info->store->chunk.offset, layout->u.chunk.ndims * sizeof(fake_ent.offset[0]));
            HDassert(layout->u.chunk.size > 0);
            fake_ent.chunk_addr = udata->addr;
            fake_ent.chunk = (uint8_t *)chunk;

            if(H5D_chunk_flush_entry(io_info->dset, io_info->dxpl_id, io_info->dxpl_cache, &fake_ent, TRUE) < 0)
                HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "cannot flush indexed storage buffer")
        } /* end if */
        else {
            if(chunk)
                chunk = H5D_chunk_xfree(chunk, &(io_info->dset->shared->dcpl_cache.pline));
        } /* end else */
    } /* end if */
    else {
        H5D_rdcc_ent_t	*ent;   /* Chunk's entry in the cache */

        /* Sanity check */
	HDassert(idx_hint < rdcc->nslots);
	HDassert(rdcc->slot[idx_hint]);
	HDassert(rdcc->slot[idx_hint]->chunk == chunk);

        /*
         * It's in the cache so unlock it.
         */
        ent = rdcc->slot[idx_hint];
        HDassert(ent->locked);
        if(dirty) {
            ent->dirty = TRUE;
            ent->wr_count -= MIN(ent->wr_count, naccessed);
        } /* end if */
        else
            ent->rd_count -= MIN(ent->rd_count, naccessed);
        ent->locked = FALSE;
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_unlock() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_allocated_cb
 *
 * Purpose:	Simply counts the number of chunks for a dataset.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Wednesday, April 21, 1999
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
H5D_chunk_allocated_cb(const H5D_chunk_rec_t *chunk_rec, void *_udata)
{
    hsize_t *nbytes = (hsize_t *)_udata;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_chunk_allocated_cb)

    *(hsize_t *)nbytes += chunk_rec->nbytes;

    FUNC_LEAVE_NOAPI(H5_ITER_CONT)
} /* H5D_chunk_allocated_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_allocated
 *
 * Purpose:	Return the number of bytes allocated in the file for storage
 *		of raw data in the chunked dataset
 *
 * Return:	Success:	Number of bytes stored in all chunks.
 *		Failure:	0
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, May 20, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_allocated(H5D_t *dset, hid_t dxpl_id, hsize_t *nbytes)
{
    H5D_chk_idx_info_t idx_info;        /* Chunked index info */
    const H5D_rdcc_t   *rdcc = &(dset->shared->cache.chunk);	/* Raw data chunk cache */
    H5D_rdcc_ent_t     *ent;            /* Cache entry  */
    H5D_dxpl_cache_t _dxpl_cache;       /* Data transfer property cache buffer */
    H5D_dxpl_cache_t *dxpl_cache = &_dxpl_cache;   /* Data transfer property cache */
    hsize_t chunk_bytes = 0;            /* Number of bytes allocated for chunks */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5D_chunk_allocated, FAIL)

    HDassert(dset);
    HDassert(dset->shared);

    /* Fill the DXPL cache values for later use */
    if(H5D_get_dxpl_cache(dxpl_id, &dxpl_cache) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't fill dxpl cache")

    /* Search for cached chunks that haven't been written out */
    for(ent = rdcc->head; ent; ent = ent->next) {
        /* Flush the chunk out to disk, to make certain the size is correct later */
        if(H5D_chunk_flush_entry(dset, dxpl_id, dxpl_cache, ent, FALSE) < 0)
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "cannot flush indexed storage buffer")
    } /* end for */

    /* Compose chunked index info struct */
    idx_info.f = dset->oloc.file;
    idx_info.dxpl_id = dxpl_id;
    idx_info.pline = &dset->shared->dcpl_cache.pline;
    idx_info.layout = &dset->shared->layout.u.chunk;
    idx_info.storage = &dset->shared->layout.storage.u.chunk;

    /* Iterate over the chunks */
    if((dset->shared->layout.storage.u.chunk.ops->iterate)(&idx_info, H5D_chunk_allocated_cb, &chunk_bytes) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to retrieve allocated chunk information from index")

    /* Set number of bytes for caller */
    *nbytes = chunk_bytes;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_allocated() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_allocate
 *
 * Purpose:	Allocate file space for all chunks that are not allocated yet.
 *		Return SUCCEED if all needed allocation succeed, otherwise
 *		FAIL.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Albert Cheng
 *		June 26, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_allocate(H5D_t *dset, hid_t dxpl_id, hbool_t full_overwrite,
    hsize_t old_dim[])
{
    H5D_chk_idx_info_t idx_info;        /* Chunked index info */
    const H5D_chunk_ops_t *ops = dset->shared->layout.storage.u.chunk.ops;      /* Chunk operations */
    hsize_t     min_unalloc[H5O_LAYOUT_NDIMS]; /* First chunk in each dimension that is unallocated */
    hsize_t     max_unalloc[H5O_LAYOUT_NDIMS]; /* Last chunk in each dimension that is unallocated */
    hsize_t	chunk_offset[H5O_LAYOUT_NDIMS]; /* Offset of current chunk */
    size_t	orig_chunk_size; /* Original size of chunk in bytes */
    unsigned    filter_mask = 0; /* Filter mask for chunks that have them */
    const H5O_layout_t *layout = &(dset->shared->layout);       /* Dataset layout */
    const H5O_pline_t *pline = &(dset->shared->dcpl_cache.pline);    /* I/O pipeline info */
    const H5O_fill_t *fill = &(dset->shared->dcpl_cache.fill);    /* Fill value info */
    H5D_fill_value_t fill_status; /* The fill value status */
    hbool_t     should_fill = FALSE; /* Whether fill values should be written */
    H5D_dxpl_cache_t _dxpl_cache;       /* Data transfer property cache buffer */
    H5D_dxpl_cache_t *dxpl_cache = &_dxpl_cache;   /* Data transfer property cache */
#ifdef H5_HAVE_PARALLEL
    MPI_Comm	mpi_comm = MPI_COMM_NULL;	/* MPI communicator for file */
    int         mpi_rank = (-1); /* This process's rank  */
    int         mpi_code;       /* MPI return code */
    hbool_t     blocks_written = FALSE; /* Flag to indicate that chunk was actually written */
    hbool_t     using_mpi = FALSE;    /* Flag to indicate that the file is being accessed with an MPI-capable file driver */
#endif /* H5_HAVE_PARALLEL */
    hbool_t	carry;          /* Flag to indicate that chunk increment carrys to higher dimension (sorta) */
    int         space_ndims;    /* Dataset's space rank */
    hsize_t     space_dim[H5O_LAYOUT_NDIMS];    /* Dataset's dataspace dimensions */
    const uint32_t *chunk_dim = layout->u.chunk.dim; /* Convenience pointer to chunk dimensions */
    int         op_dim;                 /* Current operationg dimension */
    H5D_fill_buf_info_t fb_info;        /* Dataset's fill buffer info */
    hbool_t     fb_info_init = FALSE;   /* Whether the fill value buffer has been initialized */
    hid_t       data_dxpl_id;           /* DXPL ID to use for raw data I/O operations */
    herr_t	ret_value = SUCCEED;	/* Return value */

    FUNC_ENTER_NOAPI(H5D_chunk_allocate, FAIL)

    /* Check args */
    HDassert(dset && H5D_CHUNKED == layout->type);
    HDassert(layout->u.chunk.ndims > 0 && layout->u.chunk.ndims <= H5O_LAYOUT_NDIMS);
    HDassert(TRUE == H5P_isa_class(dxpl_id, H5P_DATASET_XFER));

    /* Retrieve the dataset dimensions */
    if((space_ndims = H5S_get_simple_extent_dims(dset->shared->space, space_dim, NULL)) < 0)
         HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to get simple dataspace info")
    space_dim[space_ndims] = layout->u.chunk.dim[space_ndims];

    /* Check if any space dimensions are 0, if so we do not have to do anything
     */
    for(op_dim=0; op_dim<space_ndims; op_dim++)
        if(space_dim[op_dim] == 0) {
            /* Reset any cached chunk info for this dataset */
            H5D_chunk_cinfo_cache_reset(&dset->shared->cache.chunk.last);
            HGOTO_DONE(SUCCEED)
        } /* end if */

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
        data_dxpl_id = H5AC_ind_dxpl_id;
    } /* end if */
    else {
#endif  /* H5_HAVE_PARALLEL */
        /* Use the DXPL we were given */
        data_dxpl_id = dxpl_id;
#ifdef H5_HAVE_PARALLEL
    } /* end else */
#endif  /* H5_HAVE_PARALLEL */

    /* Fill the DXPL cache values for later use */
    if(H5D_get_dxpl_cache(data_dxpl_id, &dxpl_cache) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't fill dxpl cache")

    /* Get original chunk size */
    H5_ASSIGN_OVERFLOW(orig_chunk_size, layout->u.chunk.size, uint32_t, size_t);

    /* Check the dataset's fill-value status */
    if(H5P_is_fill_value_defined(fill, &fill_status) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't tell if fill value defined")

    /* If we are filling the dataset on allocation or "if set" and
     * the fill value _is_ set, _and_ we are not overwriting the new blocks,
     * or if there are any pipeline filters defined,
     * set the "should fill" flag
     */
    if((!full_overwrite && (fill->fill_time == H5D_FILL_TIME_ALLOC ||
            (fill->fill_time == H5D_FILL_TIME_IFSET && fill_status == H5D_FILL_VALUE_USER_DEFINED)))
            || pline->nused > 0)
        should_fill = TRUE;

    /* Check if fill values should be written to chunks */
    if(should_fill) {
        /* Initialize the fill value buffer */
        /* (delay allocating fill buffer for VL datatypes until refilling) */
        /* (casting away const OK - QAK) */
        if(H5D_fill_init(&fb_info, NULL, (hbool_t)(pline->nused > 0),
                (H5MM_allocate_t)H5D_chunk_alloc, (void *)pline,
                (H5MM_free_t)H5D_chunk_xfree, (void *)pline,
                &dset->shared->dcpl_cache.fill, dset->shared->type,
                dset->shared->type_id, (size_t)0, orig_chunk_size, data_dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't initialize fill buffer info")
        fb_info_init = TRUE;

       /* Check if there are filters which need to be applied to the chunk */
       /* (only do this in advance when the chunk info can be re-used (i.e.
        *      it doesn't contain any non-default VL datatype fill values)
        */
       if(!fb_info.has_vlen_fill_type && pline->nused > 0) {
           size_t buf_size = orig_chunk_size;

           /* Push the chunk through the filters */
           if(H5Z_pipeline(pline, 0, &filter_mask, dxpl_cache->err_detect, dxpl_cache->filter_cb, &orig_chunk_size, &buf_size, &fb_info.fill_buf) < 0)
               HGOTO_ERROR(H5E_PLINE, H5E_WRITEERROR, FAIL, "output pipeline failed")
#if H5_SIZEOF_SIZE_T > 4
            /* Check for the chunk expanding too much to encode in a 32-bit value */
            if(orig_chunk_size > ((size_t)0xffffffff))
                HGOTO_ERROR(H5E_DATASET, H5E_BADRANGE, FAIL, "chunk too large for 32-bit length")
#endif /* H5_SIZEOF_SIZE_T > 4 */
       } /* end if */
    } /* end if */

    /* Compose chunked index info struct */
    idx_info.f = dset->oloc.file;
    idx_info.dxpl_id = dxpl_id;
    idx_info.pline = &dset->shared->dcpl_cache.pline;
    idx_info.layout = &dset->shared->layout.u.chunk;
    idx_info.storage = &dset->shared->layout.storage.u.chunk;

    /* Calculate the minimum and maximum chunk offsets in each dimension */
    for(op_dim=0; op_dim<space_ndims; op_dim++) {
        min_unalloc[op_dim] = ((old_dim[op_dim] + chunk_dim[op_dim] - 1)
                / chunk_dim[op_dim]) * chunk_dim[op_dim];
        if(space_dim[op_dim] == 0)
            max_unalloc[op_dim] = 0;
        else
            max_unalloc[op_dim] = ((space_dim[op_dim] - 1) / chunk_dim[op_dim])
                    * chunk_dim[op_dim];
    } /* end for */

    /* Loop over all chunks */
    /* The algorithm is:
     *  For each dimension:
     *   -Allocate all chunks in the new dataspace that are beyond the original
     *    dataspace in the operating dimension, except those that have already
     *    been allocated.
     *
     * This is accomplished mainly using the min_unalloc and max_unalloc arrays.
     * min_unalloc represents the lowest offset in each dimension of chunks that
     * have not been allocated (whether or not they need to be).  max_unalloc
     * represents the highest offset in each dimension of chunks in the new
     * dataset that have not been allocated by this routine (they may have been
     * allocated previously).
     *
     * Every time the algorithm finishes allocating chunks allocated beyond a
     * certain dimension, max_unalloc is updated in order to avoid allocating
     * those chunks again.
     */
    for(op_dim=0; op_dim<space_ndims; op_dim++) {
        H5D_chunk_ud_t udata;   /* User data for querying chunk info */
        int i;                  /* Local index variable */

        /* Check if allocation along this dimension is really necessary */
        if(min_unalloc[op_dim] > max_unalloc[op_dim])
            continue;
        else {
            /* Reset the chunk offset indices */
            HDmemset(chunk_offset, 0, (layout->u.chunk.ndims * sizeof(chunk_offset[0])));
            chunk_offset[op_dim] = min_unalloc[op_dim];

            carry = FALSE;
        } /* end if */

        while(!carry) {
            size_t chunk_size;      /* Size of chunk in bytes, possibly filtered */

#ifndef NDEBUG
            /* None of the chunks should be allocated */
            if(H5D_chunk_get_info(dset, dxpl_id, chunk_offset, &udata) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "error looking up chunk address")
            HDassert(!H5F_addr_defined(udata.addr));

            /* Make sure the chunk is really in the dataset and outside the
             * original dimensions */
            {
                hbool_t outside_orig = FALSE;
                for(i=0; i<space_ndims; i++) {
                    HDassert(chunk_offset[i] < space_dim[i]);
                    if(chunk_offset[i] >= old_dim[i])
                        outside_orig = TRUE;
                } /* end for */
                HDassert(outside_orig);
            } /* end block */
#endif /* NDEBUG */

            /* Check for VL datatype & non-default fill value */
            if(fb_info_init && fb_info.has_vlen_fill_type) {
                /* Sanity check */
                HDassert(should_fill);

                /* Fill the buffer with VL datatype fill values */
                if(H5D_fill_refill_vl(&fb_info, fb_info.elmts_per_buf, data_dxpl_id) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, FAIL, "can't refill fill value buffer")

                /* Check if there are filters which need to be applied to the chunk */
                if(pline->nused > 0) {
                    size_t buf_size = orig_chunk_size;
                    size_t nbytes = fb_info.fill_buf_size;

                    /* Push the chunk through the filters */
                    if(H5Z_pipeline(pline, 0, &filter_mask, dxpl_cache->err_detect, dxpl_cache->filter_cb, &nbytes, &buf_size, &fb_info.fill_buf) < 0)
                        HGOTO_ERROR(H5E_PLINE, H5E_WRITEERROR, FAIL, "output pipeline failed")

#if H5_SIZEOF_SIZE_T > 4
                    /* Check for the chunk expanding too much to encode in a 32-bit value */
                    if(nbytes > ((size_t)0xffffffff))
                        HGOTO_ERROR(H5E_DATASET, H5E_BADRANGE, FAIL, "chunk too large for 32-bit length")
#endif /* H5_SIZEOF_SIZE_T > 4 */

                    /* Keep the number of bytes the chunk turned in to */
                    chunk_size = nbytes;
                } /* end if */
                else
                    H5_ASSIGN_OVERFLOW(chunk_size, layout->u.chunk.size, uint32_t, size_t);
            } /* end if */
            else
                chunk_size = orig_chunk_size;

            /* Initialize the chunk information */
            udata.common.layout = &layout->u.chunk;
            udata.common.storage = &layout->storage.u.chunk;
            udata.common.offset = chunk_offset;
            H5_ASSIGN_OVERFLOW(udata.nbytes, chunk_size, size_t, uint32_t);
            udata.filter_mask = filter_mask;
            udata.addr = HADDR_UNDEF;

            /* Allocate the chunk with all processes */
            if((ops->insert)(&idx_info, &udata) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, FAIL, "unable to insert record into chunk index")
            HDassert(H5F_addr_defined(udata.addr));

            /* Check if fill values should be written to chunks */
            if(should_fill) {
                /* Sanity check */
                HDassert(fb_info_init);
                HDassert(udata.nbytes == chunk_size);

#ifdef H5_HAVE_PARALLEL
                /* Check if this file is accessed with an MPI-capable file driver */
                if(using_mpi) {
                    /* Write the chunks out from only one process */
                    /* !! Use the internal "independent" DXPL!! -QAK */
                    if(H5_PAR_META_WRITE == mpi_rank)
                        if(H5F_block_write(dset->oloc.file, H5FD_MEM_DRAW, udata.addr, chunk_size, data_dxpl_id, fb_info.fill_buf) < 0)
                            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to write raw data to file")

                    /* Indicate that blocks are being written */
                    blocks_written = TRUE;
                } /* end if */
                else {
#endif /* H5_HAVE_PARALLEL */
                    if(H5F_block_write(dset->oloc.file, H5FD_MEM_DRAW, udata.addr, chunk_size, data_dxpl_id, fb_info.fill_buf) < 0)
                        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to write raw data to file")
#ifdef H5_HAVE_PARALLEL
                } /* end else */
#endif /* H5_HAVE_PARALLEL */
            } /* end if */

            /* Release the fill buffer if we need to re-allocate it each time */
            if(fb_info_init && fb_info.has_vlen_fill_type && pline->nused > 0)
                H5D_fill_release(&fb_info);

            /* Increment indices */
            carry = TRUE;
            for(i = (int)(space_ndims - 1); i >= 0; --i) {
                chunk_offset[i] += chunk_dim[i];
                if(chunk_offset[i] > max_unalloc[i])
                    if(i == op_dim)
                        chunk_offset[i] = min_unalloc[i];
                    else
                        chunk_offset[i] = 0;
                else {
                    carry = FALSE;
                    break;
                } /* end else */
            } /* end for */
        } /* end while(!carry) */

        /* Adjust max_unalloc_dim_idx so we don't allocate the same chunk twice.
        * Also check if this dimension started from 0 (and hence allocated all
        * of the chunks. */
        if(min_unalloc[op_dim] == 0)
            break;
        else
            max_unalloc[op_dim] = min_unalloc[op_dim] - chunk_dim[op_dim];
    } /* end for(op_dim=0...) */

#ifdef H5_HAVE_PARALLEL
    /* Only need to block at the barrier if we actually initialized a chunk */
    /* using an MPI-capable file driver */
    if(using_mpi && blocks_written) {
        /* Wait at barrier to avoid race conditions where some processes are
         * still writing out chunks and other processes race ahead to read
         * them in, getting bogus data.
         */
        if(MPI_SUCCESS != (mpi_code = MPI_Barrier(mpi_comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Barrier failed", mpi_code)
    } /* end if */
#endif /* H5_HAVE_PARALLEL */

    /* Reset any cached chunk info for this dataset */
    H5D_chunk_cinfo_cache_reset(&dset->shared->cache.chunk.last);

done:
    /* Release the fill buffer info, if it's been initialized */
    if(fb_info_init && H5D_fill_term(&fb_info) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't release fill buffer info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_allocate() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_prune_fill
 *
 * Purpose:	Write the fill value to the parts of the chunk that are no
 *              longer part of the dataspace
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Pedro Vicente, pvn@ncsa.uiuc.edu
 * 		March 26, 2002
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_chunk_prune_fill(H5D_chunk_it_ud1_t *udata)
{
    const H5D_io_info_t *io_info = udata->io_info; /* Local pointer to I/O info */
    H5D_t       *dset = io_info->dset;  /* Local pointer to the dataset info */
    const H5O_layout_t *layout = &(dset->shared->layout); /* Dataset's layout */
    unsigned    rank = udata->common.layout->ndims - 1; /* Dataset rank */
    const hsize_t *chunk_offset = io_info->store->chunk.offset; /* Chunk offset */
    H5S_sel_iter_t chunk_iter;          /* Memory selection iteration info */
    hssize_t    sel_nelmts;             /* Number of elements in selection */
    hsize_t     count[H5O_LAYOUT_NDIMS]; /* Element count of hyperslab */
    void        *chunk;	                /* The file chunk  */
    unsigned    idx_hint;               /* Which chunk we're dealing with */
    H5D_chunk_ud_t chk_udata;           /* User data for locking chunk */
    uint32_t    bytes_accessed;         /* Bytes accessed in chunk */
    unsigned    u;                      /* Local index variable */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_chunk_prune_fill)

    /* Get the info for the chunk in the file */
    if(H5D_chunk_get_info(dset, io_info->dxpl_id, chunk_offset, &chk_udata) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "error looking up chunk address")

    /* If this chunk does not exist in cache or on disk, no need to do anything
     */
    if(!H5F_addr_defined(chk_udata.addr)
            && !H5D_chunk_in_cache(dset, chunk_offset,
            io_info->store->chunk.index))
        HGOTO_DONE(SUCCEED)

    /* Initialize the fill value buffer, if necessary */
    if(!udata->fb_info_init) {
        H5_CHECK_OVERFLOW(udata->elmts_per_chunk, uint32_t, size_t);
        if(H5D_fill_init(&udata->fb_info, NULL, FALSE, NULL, NULL, NULL, NULL,
                &dset->shared->dcpl_cache.fill,
                dset->shared->type, dset->shared->type_id, (size_t)udata->elmts_per_chunk,
                io_info->dxpl_cache->max_temp_buf, io_info->dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't initialize fill buffer info")
        udata->fb_info_init = TRUE;
    } /* end if */

    /* Compute the # of elements to leave with existing value, in each dimension */
    for(u = 0; u < rank; u++) {
        count[u] = MIN(layout->u.chunk.dim[u], (udata->space_dim[u]
                - chunk_offset[u]));
        HDassert(count[u] > 0);
    } /* end for */

    /* Select all elements in chunk, to begin with */
    if(H5S_select_all(udata->chunk_space, TRUE) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSELECT, FAIL, "unable to select space")

    /* "Subtract out" the elements to keep */
    if(H5S_select_hyperslab(udata->chunk_space, H5S_SELECT_NOTB, udata->hyper_start, NULL, count, NULL) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSELECT, FAIL, "unable to select hyperslab")

    /* Lock the chunk into the cache, to get a pointer to the chunk buffer */
    if(NULL == (chunk = (void *)H5D_chunk_lock(io_info, &chk_udata, FALSE,
            &idx_hint)))
        HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "unable to lock raw data chunk")


    /* Fill the selection in the memory buffer */
    /* Use the size of the elements in the chunk directly instead of */
    /* relying on the fill.size, which might be set to 0 if there is */
    /* no fill-value defined for the dataset -QAK */

    /* Get the number of elements in the selection */
    sel_nelmts = H5S_GET_SELECT_NPOINTS(udata->chunk_space);
    HDassert(sel_nelmts >= 0);
    H5_CHECK_OVERFLOW(sel_nelmts, hssize_t, size_t);

    /* Check for VL datatype & non-default fill value */
    if(udata->fb_info.has_vlen_fill_type)
        /* Re-fill the buffer to use for this I/O operation */
        if(H5D_fill_refill_vl(&udata->fb_info, (size_t)sel_nelmts, io_info->dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, FAIL, "can't refill fill value buffer")

    /* Create a selection iterator for scattering the elements to memory buffer */
    if(H5S_select_iter_init(&chunk_iter, udata->chunk_space, layout->u.chunk.dim[rank]) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize chunk selection information")

    /* Scatter the data into memory */
    if(H5D_scatter_mem(udata->fb_info.fill_buf, udata->chunk_space, &chunk_iter, (size_t)sel_nelmts, io_info->dxpl_cache, chunk/*out*/) < 0) {
        H5S_SELECT_ITER_RELEASE(&chunk_iter);
        HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "scatter failed")
    } /* end if */

    /* Release the selection iterator */
    if(H5S_SELECT_ITER_RELEASE(&chunk_iter) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't release selection iterator")


    /* The number of bytes accessed in the chunk */
    /* (i.e. the bytes replaced with fill values) */
    H5_CHECK_OVERFLOW(sel_nelmts, hssize_t, uint32_t);
    bytes_accessed = (uint32_t)sel_nelmts * layout->u.chunk.dim[rank];

    /* Release lock on chunk */
    if(H5D_chunk_unlock(io_info, &chk_udata, TRUE, idx_hint, chunk, bytes_accessed) < 0)
        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to unlock raw data chunk")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D_chunk_prune_fill */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_prune_by_extent
 *
 * Purpose:	This function searches for chunks that are no longer necessary
 *              both in the raw data cache and in the chunk index.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Pedro Vicente, pvn@ncsa.uiuc.edu
 * Algorithm:	Robb Matzke
 * 		March 27, 2002
 *
 * The algorithm is:
 *
 *  For chunks that are no longer necessary:
 *
 *  1. Search in the raw data cache for each chunk
 *  2. If found then preempt it from the cache
 *  3. Search in the B-tree for each chunk
 *  4. If found then remove it from the B-tree and deallocate file storage for the chunk
 *
 * This example shows a 2d dataset of 90x90 with a chunk size of 20x20.
 *
 *
 *     0         20        40        60        80    90   100
 *    0 +---------+---------+---------+---------+-----+...+
 *      |:::::X::::::::::::::         :         :     |   :
 *      |:::::::X::::::::::::         :         :     |   :   Key
 *      |::::::::::X:::::::::         :         :     |   :   --------
 *      |::::::::::::X:::::::         :         :     |   :  +-+ Dataset
 *    20+::::::::::::::::::::.........:.........:.....+...:  | | Extent
 *      |         :::::X:::::         :         :     |   :  +-+
 *      |         :::::::::::         :         :     |   :
 *      |         :::::::::::         :         :     |   :  ... Chunk
 *      |         :::::::X:::         :         :     |   :  : : Boundary
 *    40+.........:::::::::::.........:.........:.....+...:  :.:
 *      |         :         :         :         :     |   :
 *      |         :         :         :         :     |   :  ... Allocated
 *      |         :         :         :         :     |   :  ::: & Filled
 *      |         :         :         :         :     |   :  ::: Chunk
 *    60+.........:.........:.........:.........:.....+...:
 *      |         :         :::::::X:::         :     |   :   X  Element
 *      |         :         :::::::::::         :     |   :      Written
 *      |         :         :::::::::::         :     |   :
 *      |         :         :::::::::::         :     |   :
 *    80+.........:.........:::::::::::.........:.....+...:   O  Fill Val
 *      |         :         :         :::::::::::     |   :      Explicitly
 *      |         :         :         ::::::X::::     |   :      Written
 *    90+---------+---------+---------+---------+-----+   :
 *      :         :         :         :::::::::::         :
 *   100:.........:.........:.........:::::::::::.........:
 *
 *
 * We have 25 total chunks for this dataset, 5 of which have space
 * allocated in the file because they were written to one or more
 * elements. These five chunks (and only these five) also have entries in
 * the storage B-tree for this dataset.
 *
 * Now lets say we want to shrink the dataset down to 70x70:
 *
 *
 *      0         20        40        60   70   80    90   100
 *    0 +---------+---------+---------+----+----+-----+...+
 *      |:::::X::::::::::::::         :    |    :     |   :
 *      |:::::::X::::::::::::         :    |    :     |   :    Key
 *      |::::::::::X:::::::::         :    |    :     |   :    --------
 *      |::::::::::::X:::::::         :    |    :     |   :   +-+ Dataset
 *    20+::::::::::::::::::::.........:....+....:.....|...:   | | Extent
 *      |         :::::X:::::         :    |    :     |   :   +-+
 *      |         :::::::::::         :    |    :     |   :
 *      |         :::::::::::         :    |    :     |   :   ... Chunk
 *      |         :::::::X:::         :    |    :     |   :   : : Boundary
 *    40+.........:::::::::::.........:....+....:.....|...:   :.:
 *      |         :         :         :    |    :     |   :
 *      |         :         :         :    |    :     |   :   ... Allocated
 *      |         :         :         :    |    :     |   :   ::: & Filled
 *      |         :         :         :    |    :     |   :   ::: Chunk
 *    60+.........:.........:.........:....+....:.....|...:
 *      |         :         :::::::X:::    |    :     |   :    X  Element
 *      |         :         :::::::::::    |    :     |   :       Written
 *      +---------+---------+---------+----+    :     |   :
 *      |         :         :::::::::::         :     |   :
 *    80+.........:.........:::::::::X:.........:.....|...:    O  Fill Val
 *      |         :         :         :::::::::::     |   :       Explicitly
 *      |         :         :         ::::::X::::     |   :       Written
 *    90+---------+---------+---------+---------+-----+   :
 *      :         :         :         :::::::::::         :
 *   100:.........:.........:.........:::::::::::.........:
 *
 *
 * That means that the nine chunks along the bottom and right side should
 * no longer exist. Of those nine chunks, (0,80), (20,80), (40,80),
 * (60,80), (80,80), (80,60), (80,40), (80,20), and (80,0), one is actually allocated
 * that needs to be released.
 * To release the chunks, we traverse the B-tree to obtain a list of unused
 * allocated chunks, and then call H5B_remove() for each chunk.
 *
 * Modifications: Neil Fortner
 *                4 May 2010
 *                Rewrote algorithm to work in a way similar to
 *                H5D_chunk_allocate: it now iterates over all chunks that need
 *                to be filled or removed, and does so as appropriate.  This
 *                avoids various issues with coherency of locally cached data
 *                which could occur with the previous implementation.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_prune_by_extent(H5D_t *dset, hid_t dxpl_id, const hsize_t *old_dim)
{
    hsize_t                 min_mod_chunk_off[H5O_LAYOUT_NDIMS]; /* Offset of first chunk to modify in each dimension */
    hsize_t                 max_mod_chunk_off[H5O_LAYOUT_NDIMS]; /* Offset of last chunk to modify in each dimension */
    hssize_t                max_fill_chunk_off[H5O_LAYOUT_NDIMS]; /* Offset of last chunk that might be filled in each dimension */
    hbool_t                 fill_dim[H5O_LAYOUT_NDIMS]; /* Whether the plane of edge chunks in this dimension needs to be filled */
    hbool_t                 dims_outside_fill[H5O_LAYOUT_NDIMS]; /* Dimensions in chunk offset outside fill dimensions */
    int                     ndims_outside_fill = 0; /* Number of dimensions in chunk offset outside fill dimensions */
    hbool_t                 has_fill = FALSE;   /* Whether there are chunks that must be filled */
    H5D_chk_idx_info_t      idx_info;           /* Chunked index info */
    H5D_io_info_t           chk_io_info;        /* Chunked I/O info object */
    H5D_storage_t           chk_store;          /* Chunk storage information */
    H5D_dxpl_cache_t        _dxpl_cache;        /* Data transfer property cache buffer */
    H5D_dxpl_cache_t       *dxpl_cache = &_dxpl_cache;   /* Data transfer property cache */
    const H5O_layout_t     *layout = &(dset->shared->layout);   /* Dataset's layout */
    const H5D_rdcc_t       *rdcc = &(dset->shared->cache.chunk);	/*raw data chunk cache */
    H5D_rdcc_ent_t         *ent = NULL;	        /* Cache entry  */
    unsigned                idx = 0;            /* Hash index number     */
    int                     space_ndims;        /* Dataset's space rank */
    hsize_t                 space_dim[H5O_LAYOUT_NDIMS]; /* Current dataspace dimensions */
    int                     op_dim;             /* Current operationg dimension */
    hbool_t                 shrunk_dim[H5O_LAYOUT_NDIMS]; /* Dimensions which have shrunk */
    H5D_chunk_it_ud1_t      udata;      /* Chunk index iterator user data */
    hbool_t                 udata_init = FALSE; /* Whether the chunk index iterator user data has been initialized */
    H5D_chunk_common_ud_t   idx_udata;          /* User data for index removal routine */
    H5D_chunk_ud_t          chk_udata;          /* User data for getting chunk info */
    H5S_t                  *chunk_space = NULL;         /* Dataspace for a chunk */
    hsize_t                 chunk_dim[H5O_LAYOUT_NDIMS];   /* Chunk dimensions */
    hsize_t                 chunk_offset[H5O_LAYOUT_NDIMS]; /* Offset of current chunk */
    hsize_t                 hyper_start[H5O_LAYOUT_NDIMS];  /* Starting location of hyperslab */
    uint32_t                elmts_per_chunk;    /* Elements in chunk */
    hbool_t                 chk_on_disk;        /* Whether a chunk exists on disk */
    hbool_t                 carry;              /* Flag to indicate that chunk increment carrys to higher dimension (sorta) */
    int                     i;	        /* Local index variable */
    herr_t                  ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5D_chunk_prune_by_extent, FAIL)

    /* Check args */
    HDassert(dset && H5D_CHUNKED == layout->type);
    HDassert(layout->u.chunk.ndims > 0 && layout->u.chunk.ndims <= H5O_LAYOUT_NDIMS);
    HDassert(dxpl_cache);

    /* Fill the DXPL cache values for later use */
    if(H5D_get_dxpl_cache(dxpl_id, &dxpl_cache) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't fill dxpl cache")

    /* Go get the rank & dimensions (including the element size) */
    if((space_ndims = H5S_get_simple_extent_dims(dset->shared->space, space_dim,
            NULL)) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get dataset dimensions")
    space_dim[space_ndims] = layout->u.chunk.dim[space_ndims];

    /* The last dimension in chunk_offset is always 0 */
    chunk_offset[space_ndims] = (hsize_t)0;

    /* Check if any old dimensions are 0, if so we do not have to do anything */
    for(op_dim=0; op_dim<space_ndims; op_dim++)
        if(old_dim[op_dim] == 0) {
            /* Reset any cached chunk info for this dataset */
            H5D_chunk_cinfo_cache_reset(&dset->shared->cache.chunk.last);
            HGOTO_DONE(SUCCEED)
        } /* end if */

    /* Round up to the next integer # of chunks, to accomodate partial chunks */
    /* Use current dims because the indices have already been updated! -NAF */
    /* (also compute the number of elements per chunk) */
    /* (also copy the chunk dimensions into 'hsize_t' array for creating dataspace) */
    /* (also compute the dimensions which have been shrunk) */
    elmts_per_chunk = 1;
    for(i = 0; i < space_ndims; i++) {
        elmts_per_chunk *= layout->u.chunk.dim[i];
	chunk_dim[i] = layout->u.chunk.dim[i];
	shrunk_dim[i] = space_dim[i] < old_dim[i];
    } /* end for */

    /* Create a dataspace for a chunk & set the extent */
    if(NULL == (chunk_space = H5S_create_simple((unsigned)space_ndims,
            chunk_dim, NULL)))
	HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCREATE, FAIL, "can't create simple dataspace")

    /* Reset hyperslab start array */
    /* (hyperslabs will always start from origin) */
    HDmemset(hyper_start, 0, sizeof(hyper_start));

    /* Set up chunked I/O info object, for operations on chunks (in callback)
     * Note that we only need to set chunk_offset once, as the array's address
     * will never change. */
    chk_store.chunk.offset = chunk_offset;
    H5D_BUILD_IO_INFO_RD(&chk_io_info, dset, dxpl_cache, dxpl_id, &chk_store, NULL);

    /* Compose chunked index info struct */
    idx_info.f = dset->oloc.file;
    idx_info.dxpl_id = dxpl_id;
    idx_info.pline = &dset->shared->dcpl_cache.pline;
    idx_info.layout = &dset->shared->layout.u.chunk;
    idx_info.storage = &dset->shared->layout.storage.u.chunk;

    /* Initialize the user data for the iteration */
    HDmemset(&udata, 0, sizeof udata);
    udata.common.layout = &layout->u.chunk;
    udata.common.storage = &layout->storage.u.chunk;
    udata.io_info = &chk_io_info;
    udata.idx_info = &idx_info;
    udata.space_dim = space_dim;
    udata.shrunk_dim = shrunk_dim;
    udata.elmts_per_chunk = elmts_per_chunk;
    udata.chunk_space = chunk_space;
    udata.hyper_start = hyper_start;
    udata_init = TRUE;

    /* Initialize user data for removal */
    idx_udata.layout = &layout->u.chunk;
    idx_udata.storage = &layout->storage.u.chunk;

    /*
     * Determine the chunks which need to be filled or removed
     */
    for(op_dim=0; op_dim<space_ndims; op_dim++) {
        /* Calculate the largest offset of chunks that might need to be
         * modified in this dimension */
        max_mod_chunk_off[op_dim] = chunk_dim[op_dim] * ((old_dim[op_dim] - 1)
                / chunk_dim[op_dim]);

        /* Calculate the largest offset of chunks that might need to be
         * filled in this dimension */
        if(0 == space_dim[op_dim])
            max_fill_chunk_off[op_dim] = -1;
        else
            max_fill_chunk_off[op_dim] = (hssize_t)(chunk_dim[op_dim]
                    * ((MIN(space_dim[op_dim], old_dim[op_dim]) - 1)
                    / chunk_dim[op_dim]));

        if(shrunk_dim[op_dim]) {
            /* Calculate the smallest offset of chunks that might need to be
             * modified in this dimension.  Note that this array contains
             * garbage for all dimensions which are not shrunk.  These locations
             * must not be read from! */
            min_mod_chunk_off[op_dim] = chunk_dim[op_dim] * (space_dim[op_dim]
                    / chunk_dim[op_dim]);

            /* Determine if we need to fill chunks in this dimension */
            if((hssize_t)min_mod_chunk_off[op_dim]
                    == max_fill_chunk_off[op_dim]) {
                fill_dim[op_dim] = TRUE;
                has_fill = TRUE;
            } /* end if */
            else
                fill_dim[op_dim] = FALSE;
        } /* end if */
        else
            fill_dim[op_dim] = FALSE;
    } /* end for */

    /* Check the cache for any entries that are outside the bounds.  Mark these
     * entries as deleted so they are not flushed to disk accidentally.  This is
     * only necessary if there are chunks that need to be filled. */
    if(has_fill)
        for(ent = rdcc->head; ent; ent = ent->next)
            /* Check for chunk offset outside of new dimensions */
            for(i = 0; i<space_ndims; i++)
                if((hsize_t)ent->offset[i] >= space_dim[i]) {
                    /* Mark the entry as "deleted" */
                    ent->deleted = TRUE;
                    break;
                } /* end if */

    /* Main loop: fill or remove chunks */
    for(op_dim=0; op_dim<space_ndims; op_dim++) {
        /* Check if modification along this dimension is really necessary */
        if(!shrunk_dim[op_dim])
            continue;
        else {
            HDassert((hsize_t) max_mod_chunk_off[op_dim]
                    >= min_mod_chunk_off[op_dim]);

            /* Reset the chunk offset indices */
            HDmemset(chunk_offset, 0, ((unsigned)space_ndims
                    * sizeof(chunk_offset[0])));
            chunk_offset[op_dim] = min_mod_chunk_off[op_dim];

            /* Initialize "dims_outside_fill" array */
            ndims_outside_fill = 0;
            for(i=0; i<space_ndims; i++)
                if((hssize_t)chunk_offset[i] > max_fill_chunk_off[i]) {
                    dims_outside_fill[i] = TRUE;
                    ndims_outside_fill++;
                } /* end if */
                else
                    dims_outside_fill[i] = FALSE;

            carry = FALSE;
        } /* end if */

        while(!carry) {
            /* Calculate the index of this chunk */
            if(H5V_chunk_index((unsigned)space_ndims, chunk_offset,
                    layout->u.chunk.dim, layout->u.chunk.down_chunks,
                    &(chk_io_info.store->chunk.index)) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't get chunk index")

            if(0 == ndims_outside_fill) {
                HDassert(fill_dim[op_dim]);
                HDassert(chunk_offset[op_dim] == min_mod_chunk_off[op_dim]);

                /* Fill the unused parts of the chunk */
                if(H5D_chunk_prune_fill(&udata) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to write fill value")
            } /* end if */
            else {
                chk_on_disk = FALSE;

#ifndef NDEBUG
                /* Make sure this chunk is really outside the new dimensions */
                {
                    hbool_t outside_dim = FALSE;

                    for(i=0; i<space_ndims; i++)
                        if(chunk_offset[i] >= space_dim[i]){
                            outside_dim = TRUE;
                            break;
                        } /* end if */
                    HDassert(outside_dim);
                } /* end block */
#endif /* NDEBUG */

                /* Search for the chunk in the cache */
                if(rdcc->nslots > 0) {
                    idx = H5D_CHUNK_HASH(dset->shared,
                            chk_io_info.store->chunk.index);
                    ent = rdcc->slot[idx];

                    if(ent)
                        for(i=0; i<space_ndims; i++)
                            if(chunk_offset[i]
                                    != ent->offset[i]) {
                                ent = NULL;
                                break;
                            } /* end if */
                } /* end if */

                /* Evict the entry from the cache, but do not flush it to disk
                 */
                if(ent) {
                    /* Determine if the chunk is allocated on disk, and
                     * therefore needs to be removed from disk */
                    chk_on_disk = H5F_addr_defined(ent->chunk_addr);

                    /* Remove the chunk from cache */
                    if(H5D_chunk_cache_evict(dset, dxpl_id, dxpl_cache, ent,
                            FALSE) < 0)
                        HGOTO_ERROR(H5E_DATASET, H5E_CANTREMOVE, FAIL, "unable to evict chunk")

                    ent = NULL;
                } /* end if */
                else {
                    /* Determine if the chunk is allocated on disk, and
                     * therefore needs to be removed from disk */
                    /* Get the info for the chunk in the file */
                    if(H5D_chunk_get_info(dset, dxpl_id, chunk_offset,
                            &chk_udata) < 0)
                        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "error looking up chunk address")

                    chk_on_disk = H5F_addr_defined(chk_udata.addr);
                } /* end else */

                /* Remove the chunk from disk, if present */
                if(chk_on_disk) {
                    /* Update the offset in idx_udata */
                    idx_udata.offset = chunk_offset;

                    /* Remove the chunk from disk */
                    if((layout->storage.u.chunk.ops->remove)(&idx_info, &idx_udata)
                            < 0)
                        HGOTO_ERROR(H5E_DATASET, H5E_CANTDELETE, FAIL, "unable to remove chunk entry from index")
                } /* end if */
            } /* end else */

            /* Increment indices */
            carry = TRUE;
            for(i = (int)(space_ndims - 1); i >= 0; --i) {
                chunk_offset[i] += chunk_dim[i];
                if(chunk_offset[i] > (hsize_t) max_mod_chunk_off[i]) {
                    /* Left maximum dimensions, "wrap around" and check if this
                     * dimension is no longer outside the fill dimension */
                    if(i == op_dim) {
                        chunk_offset[i] = min_mod_chunk_off[i];
                        if(dims_outside_fill[i] && fill_dim[i]) {
                            dims_outside_fill[i] = FALSE;
                            ndims_outside_fill--;
                        } /* end if */
                    } /* end if */
                    else {
                        chunk_offset[i] = 0;
                        if(dims_outside_fill[i] && max_fill_chunk_off[i] >= 0) {
                            dims_outside_fill[i] = FALSE;
                            ndims_outside_fill--;
                        } /* end if */
                    } /* end else */
                } /* end if */
                else {
                    /* Check if we just went outside the fill dimension */
                    if(!dims_outside_fill[i] && (hssize_t)chunk_offset[i]
                            > max_fill_chunk_off[i]) {
                        dims_outside_fill[i] = TRUE;
                        ndims_outside_fill++;
                    } /* end if */

                    /* We found the next chunk, so leave the loop */
                    carry = FALSE;
                    break;
                } /* end else */
            } /* end for */
        } /* end while(!carry) */

        /* Adjust max_mod_chunk_off so we don't modify the same chunk twice.
         * Also check if this dimension started from 0 (and hence removed all
         * of the chunks). */
        if(min_mod_chunk_off[op_dim] == 0)
            break;
        else
            max_mod_chunk_off[op_dim] = min_mod_chunk_off[op_dim]
                    - chunk_dim[op_dim];
    } /* end for(op_dim=0...) */

    /* Reset any cached chunk info for this dataset */
    H5D_chunk_cinfo_cache_reset(&dset->shared->cache.chunk.last);

done:
    /* Release resources */
    if(chunk_space && H5S_close(chunk_space) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataspace")
    if(udata_init) {
        if(udata.fb_info_init && H5D_fill_term(&udata.fb_info) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't release fill buffer info")
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_prune_by_extent() */

#ifdef H5_HAVE_PARALLEL

/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_addrmap_cb
 *
 * Purpose:     Callback when obtaining the chunk addresses for all existing chunks
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Kent Yang
 *              Tuesday, November 15, 2005
 *
 *-------------------------------------------------------------------------
 */
static int
H5D_chunk_addrmap_cb(const H5D_chunk_rec_t *chunk_rec, void *_udata)
{
    H5D_chunk_it_ud2_t	*udata = (H5D_chunk_it_ud2_t *)_udata;  /* User data for callback */
    unsigned       rank = udata->common.layout->ndims - 1;    /* # of dimensions of dataset */
    hsize_t        chunk_index;
    int            ret_value = H5_ITER_CONT;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_chunk_addrmap_cb)

    /* Compute the index for this chunk */
    if(H5V_chunk_index(rank, chunk_rec->offset, udata->common.layout->dim, udata->common.layout->down_chunks, &chunk_index) < 0)
       HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, H5_ITER_ERROR, "can't get chunk index")

    /* Set it in the userdata to return */
    udata->chunk_addr[chunk_index] = chunk_rec->chunk_addr;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D_chunk_addrmap_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_addrmap
 *
 * Purpose:     Obtain the chunk addresses for all existing chunks
 *
 * Return:	Success:	Non-negative on succeed.
 *		Failure:	negative value
 *
 * Programmer:  Kent Yang
 *              November 15, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_addrmap(const H5D_io_info_t *io_info, haddr_t chunk_addr[])
{
    H5D_chk_idx_info_t idx_info;        /* Chunked index info */
    H5D_t *dset = io_info->dset;        /* Local pointer to dataset info */
    H5D_chunk_it_ud2_t udata;          	/* User data for iteration callback */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5D_chunk_addrmap, FAIL)

    HDassert(dset);
    HDassert(dset->shared);
    HDassert(chunk_addr);

    /* Set up user data for B-tree callback */
    HDmemset(&udata, 0, sizeof(udata));
    udata.common.layout = &dset->shared->layout.u.chunk;
    udata.common.storage = &dset->shared->layout.storage.u.chunk;
    udata.chunk_addr  = chunk_addr;

    /* Compose chunked index info struct */
    idx_info.f = dset->oloc.file;
    idx_info.dxpl_id = io_info->dxpl_id;
    idx_info.pline = &dset->shared->dcpl_cache.pline;
    idx_info.layout = &dset->shared->layout.u.chunk;
    idx_info.storage = &dset->shared->layout.storage.u.chunk;

    /* Iterate over chunks to build mapping of chunk addresses */
    if((dset->shared->layout.storage.u.chunk.ops->iterate)(&idx_info, H5D_chunk_addrmap_cb, &udata) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to iterate over chunk index to build address map")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_addrmap() */
#endif /* H5_HAVE_PARALLEL */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_delete
 *
 * Purpose:	Delete raw data storage for entire dataset (i.e. all chunks)
 *
 * Return:	Success:	Non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Thursday, March 20, 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_delete(H5F_t *f, hid_t dxpl_id, H5O_t *oh, H5O_storage_t *storage)
{
    H5D_chk_idx_info_t idx_info;        /* Chunked index info */
    H5O_layout_t layout;                /* Dataset layout  message */
    hbool_t layout_read = FALSE;        /* Whether the layout message was read from the file */
    H5O_pline_t pline;                  /* I/O pipeline message */
    hbool_t pline_read = FALSE;         /* Whether the I/O pipeline message was read from the file */
    htri_t	exists;                 /* Flag if header message of interest exists */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5D_chunk_delete, FAIL)

    /* Sanity check */
    HDassert(f);
    HDassert(oh);
    HDassert(storage);

    /* Check for I/O pipeline message */
    if((exists = H5O_msg_exists_oh(oh, H5O_PLINE_ID)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to check for object header message")
    else if(exists) {
        if(NULL == H5O_msg_read_oh(f, dxpl_id, oh, H5O_PLINE_ID, &pline))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get I/O pipeline message")
        pline_read = TRUE;
    } /* end else if */
    else
        HDmemset(&pline, 0, sizeof(pline));

    /* Retrieve dataset layout message */
    if((exists = H5O_msg_exists_oh(oh, H5O_LAYOUT_ID)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to check for object header message")
    else if(exists) {
        if(NULL == H5O_msg_read_oh(f, dxpl_id, oh, H5O_LAYOUT_ID, &layout))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get layout message")
        layout_read = TRUE;
    } /* end else if */
    else
        HGOTO_ERROR(H5E_DATASET, H5E_NOTFOUND, FAIL, "can't find layout message")

    /* Compose chunked index info struct */
    idx_info.f = f;
    idx_info.dxpl_id = dxpl_id;
    idx_info.pline = &pline;
    idx_info.layout = &layout.u.chunk;
    idx_info.storage = &storage->u.chunk;

    /* Delete the chunked storage information in the file */
    if((storage->u.chunk.ops->idx_delete)(&idx_info) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTDELETE, FAIL, "unable to delete chunk index")

done:
    /* Clean up any messages read in */
    if(pline_read)
        if(H5O_msg_reset(H5O_PLINE_ID, &pline) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTRESET, FAIL, "unable to reset I/O pipeline message")
    if(layout_read)
        if(H5O_msg_reset(H5O_LAYOUT_ID, &layout) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTRESET, FAIL, "unable to reset layout message")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_delete() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_update_cache
 *
 * Purpose:	Update any cached chunks index values after the dataspace
 *              size has changed
 *
 * Return:	Success:	Non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Saturday, May 29, 2004
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_update_cache(H5D_t *dset, hid_t dxpl_id)
{
    H5D_rdcc_t         *rdcc = &(dset->shared->cache.chunk);	/*raw data chunk cache */
    H5D_rdcc_ent_t     *ent, *next;	/*cache entry  */
    H5D_rdcc_ent_t     *old_ent;	/* Old cache entry  */
    H5D_dxpl_cache_t _dxpl_cache;       /* Data transfer property cache buffer */
    H5D_dxpl_cache_t *dxpl_cache = &_dxpl_cache;   /* Data transfer property cache */
    unsigned            rank;	        /*current # of dimensions */
    herr_t              ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5D_chunk_update_cache, FAIL)

    /* Check args */
    HDassert(dset && H5D_CHUNKED == dset->shared->layout.type);
    HDassert(dset->shared->layout.u.chunk.ndims > 0 && dset->shared->layout.u.chunk.ndims <= H5O_LAYOUT_NDIMS);

    /* Get the rank */
    rank = dset->shared->layout.u.chunk.ndims-1;
    HDassert(rank > 0);

    /* 1-D dataset's chunks can't have their index change */
    if(rank == 1)
        HGOTO_DONE(SUCCEED)

    /* Fill the DXPL cache values for later use */
    if(H5D_get_dxpl_cache(dxpl_id, &dxpl_cache) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't fill dxpl cache")

    /* Recompute the index for each cached chunk that is in a dataset */
    for(ent = rdcc->head; ent; ent = next) {
        hsize_t             idx;        /* Chunk index */
        unsigned	    old_idx;	/* Previous index number	*/

        /* Get the pointer to the next cache entry */
        next = ent->next;

        /* Calculate the index of this chunk */
        if(H5V_chunk_index(rank, ent->offset, dset->shared->layout.u.chunk.dim, dset->shared->layout.u.chunk.down_chunks, &idx) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "can't get chunk index")

        /* Compute the index for the chunk entry */
        old_idx = ent->idx;   /* Save for later */
        ent->idx = H5D_CHUNK_HASH(dset->shared, idx);

        if(old_idx != ent->idx) {
            /* Check if there is already a chunk at this chunk's new location */
            old_ent = rdcc->slot[ent->idx];
            if(old_ent != NULL) {
                HDassert(old_ent->locked == 0);

                /* Check if we are removing the entry we would walk to next */
                if(old_ent == next)
                    next = old_ent->next;

                /* Remove the old entry from the cache */
                if(H5D_chunk_cache_evict(dset, dxpl_id, dxpl_cache, old_ent, TRUE) < 0)
                    HGOTO_ERROR(H5E_IO, H5E_CANTFLUSH, FAIL, "unable to flush one or more raw data chunks")
            } /* end if */

            /* Insert this chunk into correct location in hash table */
            rdcc->slot[ent->idx] = ent;

            /* Null out previous location */
            rdcc->slot[old_idx] = NULL;
        } /* end if */
    } /* end for */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_update_cache() */


/*-------------------------------------------------------------------------
 * Function:    H5D_chunk_copy_cb
 *
 * Purpose:     Copy chunked raw data from source file and insert to the
 *              index in the destination file
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Peter Cao
 *              August 20, 2005
 *
 *-------------------------------------------------------------------------
 */
static int
H5D_chunk_copy_cb(const H5D_chunk_rec_t *chunk_rec, void *_udata)
{
    H5D_chunk_it_ud3_t      *udata = (H5D_chunk_it_ud3_t *)_udata;       /* User data for callback */
    H5D_chunk_ud_t          udata_dst;                  /* User data about new destination chunk */
    hbool_t                 is_vlen = FALSE;            /* Whether datatype is variable-length */
    hbool_t                 fix_ref = FALSE;            /* Whether to fix up references in the dest. file */

    /* General information about chunk copy */
    void                    *bkg = udata->bkg;          /* Background buffer for datatype conversion */
    void                    *buf = udata->buf;          /* Chunk buffer for I/O & datatype conversions */
    size_t                  buf_size = udata->buf_size; /* Size of chunk buffer */
    const H5O_pline_t       *pline = udata->pline;      /* I/O pipeline for applying filters */

    /* needed for commpressed variable length data */
    hbool_t                 has_filters = FALSE;        /* Whether chunk has filters */
    size_t                  nbytes;                     /* Size of chunk in file (in bytes) */
    H5Z_cb_t                cb_struct;                  /* Filter failure callback struct */

    int                     ret_value = H5_ITER_CONT;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_chunk_copy_cb)

    /* Get 'size_t' local value for number of bytes in chunk */
    H5_ASSIGN_OVERFLOW(nbytes, chunk_rec->nbytes, uint32_t, size_t);

    /* Check parameter for type conversion */
    if(udata->do_convert) {
        if(H5T_detect_class(udata->dt_src, H5T_VLEN, FALSE) > 0)
            is_vlen = TRUE;
        else if((H5T_get_class(udata->dt_src, FALSE) == H5T_REFERENCE) && (udata->file_src != udata->idx_info_dst->f))
            fix_ref = TRUE;
        else
            HGOTO_ERROR(H5E_DATASET, H5E_CANTCOPY, H5_ITER_ERROR, "unable to copy dataset elements")
    } /* end if */

    /* Check for filtered chunks */
    if(pline && pline->nused) {
        has_filters = TRUE;
        cb_struct.func = NULL; /* no callback function when failed */
    } /* end if */

    /* Resize the buf if it is too small to hold the data */
    if(nbytes > buf_size) {
        void *new_buf;          /* New buffer for data */

        /* Re-allocate memory for copying the chunk */
        if(NULL == (new_buf = H5MM_realloc(udata->buf, nbytes)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, H5_ITER_ERROR, "memory allocation failed for raw data chunk")
        udata->buf = new_buf;
        if(udata->bkg) {
            if(NULL == (new_buf = H5MM_realloc(udata->bkg, nbytes)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, H5_ITER_ERROR, "memory allocation failed for raw data chunk")
            udata->bkg = new_buf;
            if(!udata->cpy_info->expand_ref)
                HDmemset((uint8_t *)udata->bkg + buf_size, 0, (size_t)(nbytes - buf_size));

            bkg = udata->bkg;
        } /* end if */

        buf = udata->buf;
        udata->buf_size = buf_size = nbytes;
    } /* end if */

    /* read chunk data from the source file */
    if(H5F_block_read(udata->file_src, H5FD_MEM_DRAW, chunk_rec->chunk_addr, nbytes, udata->idx_info_dst->dxpl_id, buf) < 0)
        HGOTO_ERROR(H5E_IO, H5E_READERROR, H5_ITER_ERROR, "unable to read raw data chunk")

    /* Need to uncompress variable-length & reference data elements */
    if(has_filters && (is_vlen || fix_ref)) {
        unsigned filter_mask = chunk_rec->filter_mask;

        if(H5Z_pipeline(pline, H5Z_FLAG_REVERSE, &filter_mask, H5Z_NO_EDC, cb_struct, &nbytes, &buf_size, &buf) < 0)
            HGOTO_ERROR(H5E_PLINE, H5E_CANTFILTER, H5_ITER_ERROR, "data pipeline read failed")
    } /* end if */

    /* Perform datatype conversion, if necessary */
    if(is_vlen) {
        H5T_path_t              *tpath_src_mem = udata->tpath_src_mem;
        H5T_path_t              *tpath_mem_dst = udata->tpath_mem_dst;
        H5S_t                   *buf_space = udata->buf_space;
        hid_t                   tid_src = udata->tid_src;
        hid_t                   tid_dst = udata->tid_dst;
        hid_t                   tid_mem = udata->tid_mem;
        void                    *reclaim_buf = udata->reclaim_buf;
        size_t                  reclaim_buf_size = udata->reclaim_buf_size;

        /* Convert from source file to memory */
        H5_CHECK_OVERFLOW(udata->nelmts, uint32_t, size_t);
        if(H5T_convert(tpath_src_mem, tid_src, tid_mem, (size_t)udata->nelmts, (size_t)0, (size_t)0, buf, bkg, udata->idx_info_dst->dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, H5_ITER_ERROR, "datatype conversion failed")

        /* Copy into another buffer, to reclaim memory later */
        HDmemcpy(reclaim_buf, buf, reclaim_buf_size);

        /* Set background buffer to all zeros */
        HDmemset(bkg, 0, buf_size);

        /* Convert from memory to destination file */
        if(H5T_convert(tpath_mem_dst, tid_mem, tid_dst, udata->nelmts, (size_t)0, (size_t)0, buf, bkg, udata->idx_info_dst->dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, H5_ITER_ERROR, "datatype conversion failed")

        /* Reclaim space from variable length data */
        if(H5D_vlen_reclaim(tid_mem, buf_space, H5P_DATASET_XFER_DEFAULT, reclaim_buf) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_BADITER, H5_ITER_ERROR, "unable to reclaim variable-length data")
    } /* end if */
    else if(fix_ref) {
        /* Check for expanding references */
        /* (background buffer has already been zeroed out, if not expanding) */
        if(udata->cpy_info->expand_ref) {
            size_t ref_count;

            /* Determine # of reference elements to copy */
            ref_count = nbytes / H5T_get_size(udata->dt_src);

            /* Copy the reference elements */
            if(H5O_copy_expand_ref(udata->file_src, buf, udata->idx_info_dst->dxpl_id, udata->idx_info_dst->f, bkg, ref_count, H5T_get_ref_type(udata->dt_src), udata->cpy_info) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTCOPY, H5_ITER_ERROR, "unable to copy reference attribute")
        } /* end if */

        /* After fix ref, copy the new reference elements to the buffer to write out */
        HDmemcpy(buf, bkg, buf_size);
    } /* end if */

    /* Set up destination chunk callback information for insertion */
    udata_dst.common.layout = udata->idx_info_dst->layout;
    udata_dst.common.storage = udata->idx_info_dst->storage;
    udata_dst.common.offset = chunk_rec->offset;
    udata_dst.nbytes = chunk_rec->nbytes;
    udata_dst.filter_mask = chunk_rec->filter_mask;
    udata_dst.addr = HADDR_UNDEF;

    /* Need to compress variable-length & reference data elements before writing to file */
    if(has_filters && (is_vlen || fix_ref) ) {
        if(H5Z_pipeline(pline, 0, &(udata_dst.filter_mask), H5Z_NO_EDC, cb_struct, &nbytes, &buf_size, &buf) < 0)
            HGOTO_ERROR(H5E_PLINE, H5E_CANTFILTER, H5_ITER_ERROR, "output pipeline failed")
#if H5_SIZEOF_SIZE_T > 4
        /* Check for the chunk expanding too much to encode in a 32-bit value */
        if(nbytes > ((size_t)0xffffffff))
            HGOTO_ERROR(H5E_DATASET, H5E_BADRANGE, H5_ITER_ERROR, "chunk too large for 32-bit length")
#endif /* H5_SIZEOF_SIZE_T > 4 */
        H5_ASSIGN_OVERFLOW(udata_dst.nbytes, nbytes, size_t, uint32_t);
	udata->buf = buf;
	udata->buf_size = buf_size;
    } /* end if */

    /* Insert chunk into the destination index */
    if((udata->idx_info_dst->storage->ops->insert)(udata->idx_info_dst, &udata_dst) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, H5_ITER_ERROR, "unable to insert chunk into index")

    /* Write chunk data to destination file */
    HDassert(H5F_addr_defined(udata_dst.addr));
    if(H5F_block_write(udata->idx_info_dst->f, H5FD_MEM_DRAW, udata_dst.addr, nbytes, udata->idx_info_dst->dxpl_id, buf) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, H5_ITER_ERROR, "unable to write raw data to file")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_copy_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_copy
 *
 * Purpose:	Copy chunked storage from SRC file to DST file.
 *
 * Return:	Success:	Non-negative
 *		Failure:	negative
 *
 * Programmer:  Peter Cao
 *	        August 20, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_copy(H5F_t *f_src, H5O_storage_chunk_t *storage_src,
    H5O_layout_chunk_t *layout_src, H5F_t *f_dst, H5O_storage_chunk_t *storage_dst,
    const H5S_extent_t *ds_extent_src, const H5T_t *dt_src,
    const H5O_pline_t *pline_src, H5O_copy_t *cpy_info, hid_t dxpl_id)
{
    H5D_chunk_it_ud3_t udata;           /* User data for iteration callback */
    H5D_chk_idx_info_t idx_info_dst;    /* Dest. chunked index info */
    H5D_chk_idx_info_t idx_info_src;    /* Source chunked index info */
    H5O_pline_t _pline;                 /* Temporary pipeline info */
    const H5O_pline_t *pline;           /* Pointer to pipeline info to use */
    H5T_path_t  *tpath_src_mem = NULL, *tpath_mem_dst = NULL;   /* Datatype conversion paths */
    hid_t       tid_src = -1;           /* Datatype ID for source datatype */
    hid_t       tid_dst = -1;           /* Datatype ID for destination datatype */
    hid_t       tid_mem = -1;           /* Datatype ID for memory datatype */
    size_t      buf_size;               /* Size of copy buffer */
    size_t      reclaim_buf_size;       /* Size of reclaim buffer */
    void       *buf = NULL;             /* Buffer for copying data */
    void       *bkg = NULL;             /* Buffer for background during type conversion */
    void       *reclaim_buf = NULL;     /* Buffer for reclaiming data */
    H5S_t      *buf_space = NULL;       /* Dataspace describing buffer */
    hid_t       sid_buf = -1;           /* ID for buffer dataspace */
    uint32_t    nelmts = 0;             /* Number of elements in buffer */
    hbool_t     do_convert = FALSE;     /* Indicate that type conversions should be performed */
    hbool_t     copy_setup_done = FALSE;        /* Indicate that 'copy setup' is done */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5D_chunk_copy, FAIL)

    /* Check args */
    HDassert(f_src);
    HDassert(storage_src);
    HDassert(layout_src);
    HDassert(f_dst);
    HDassert(storage_dst);
    HDassert(ds_extent_src);
    HDassert(dt_src);

    /* Initialize the temporary pipeline info */
    if(NULL == pline_src) {
        HDmemset(&_pline, 0, sizeof(_pline));
        pline = &_pline;
    } /* end if */
    else
        pline = pline_src;

    /* Layout is not created in the destination file, reset index address */
    if(H5D_chunk_idx_reset(storage_dst, TRUE) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to reset chunked storage index in dest")

    /* Initialize layout information */
    {
        hsize_t     curr_dims[H5O_LAYOUT_NDIMS];    /* Curr. size of dataset dimensions */
        int         sndims;                 /* Rank of dataspace */
        unsigned    ndims;                  /* Rank of dataspace */

        /* Get the dim info for dataset */
        if((sndims = H5S_extent_get_dims(ds_extent_src, curr_dims, NULL)) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get dataspace dimensions")
        H5_ASSIGN_OVERFLOW(ndims, sndims, int, unsigned);

        /* Set the source layout chunk information */
        if(H5D_chunk_set_info_real(layout_src, ndims, curr_dims) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set layout's chunk info")
    } /* end block */

    /* Compose source & dest chunked index info structs */
    idx_info_src.f = f_src;
    idx_info_src.dxpl_id = dxpl_id;
    idx_info_src.pline = pline;
    idx_info_src.layout = layout_src;
    idx_info_src.storage = storage_src;

    idx_info_dst.f = f_dst;
    idx_info_dst.dxpl_id = dxpl_id;
    idx_info_dst.pline = pline;     /* Use same I/O filter pipeline for dest. */
    idx_info_dst.layout = layout_src /* Use same layout for dest. */;
    idx_info_dst.storage = storage_dst;

    /* Call the index-specific "copy setup" routine */
    if((storage_src->ops->copy_setup)(&idx_info_src, &idx_info_dst) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to set up index-specific chunk copying information")
    copy_setup_done = TRUE;

    /* Create datatype ID for src datatype */
    if((tid_src = H5I_register(H5I_DATATYPE, dt_src, FALSE)) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to register source file datatype")

    /* If there's a VLEN source datatype, set up type conversion information */
    if(H5T_detect_class(dt_src, H5T_VLEN, FALSE) > 0) {
        H5T_t *dt_dst;              /* Destination datatype */
        H5T_t *dt_mem;              /* Memory datatype */
        size_t mem_dt_size;         /* Memory datatype size */
        size_t tmp_dt_size;         /* Temp. datatype size */
        size_t max_dt_size;         /* Max atatype size */
        hsize_t buf_dim;            /* Dimension for buffer */
        unsigned u;

        /* create a memory copy of the variable-length datatype */
        if(NULL == (dt_mem = H5T_copy(dt_src, H5T_COPY_TRANSIENT)))
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to copy")
        if((tid_mem = H5I_register(H5I_DATATYPE, dt_mem, FALSE)) < 0) {
            (void)H5T_close(dt_mem);
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to register memory datatype")
        } /* end if */

        /* create variable-length datatype at the destinaton file */
        if(NULL == (dt_dst = H5T_copy(dt_src, H5T_COPY_TRANSIENT)))
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to copy")
        if(H5T_set_loc(dt_dst, f_dst, H5T_LOC_DISK) < 0) {
            (void)H5T_close(dt_dst);
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "cannot mark datatype on disk")
        } /* end if */
        if((tid_dst = H5I_register(H5I_DATATYPE, dt_dst, FALSE)) < 0) {
            (void)H5T_close(dt_dst);
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to register destination file datatype")
        } /* end if */

        /* Set up the conversion functions */
        if(NULL == (tpath_src_mem = H5T_path_find(dt_src, dt_mem, NULL, NULL, dxpl_id, FALSE)))
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to convert between src and mem datatypes")
        if(NULL == (tpath_mem_dst = H5T_path_find(dt_mem, dt_dst, NULL, NULL, dxpl_id, FALSE)))
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to convert between mem and dst datatypes")

        /* Determine largest datatype size */
        if(0 == (max_dt_size = H5T_get_size(dt_src)))
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to determine datatype size")
        if(0 == (mem_dt_size = H5T_get_size(dt_mem)))
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to determine datatype size")
        max_dt_size = MAX(max_dt_size, mem_dt_size);
        if(0 == (tmp_dt_size = H5T_get_size(dt_dst)))
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to determine datatype size")
        max_dt_size = MAX(max_dt_size, tmp_dt_size);

        /* Compute the number of elements per chunk */
        nelmts = 1;
        for(u = 0;  u < (layout_src->ndims - 1); u++)
            nelmts *= layout_src->dim[u];

        /* Create the space and set the initial extent */
        buf_dim = nelmts;
        if(NULL == (buf_space = H5S_create_simple((unsigned)1, &buf_dim, NULL)))
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCREATE, FAIL, "can't create simple dataspace")

        /* Atomize */
        if((sid_buf = H5I_register(H5I_DATASPACE, buf_space, FALSE)) < 0) {
            (void)H5S_close(buf_space);
            HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register dataspace ID")
        } /* end if */

        /* Set initial buffer sizes */
        buf_size = nelmts * max_dt_size;
        reclaim_buf_size = nelmts * mem_dt_size;

        /* Allocate memory for reclaim buf */
        if(NULL == (reclaim_buf = H5MM_malloc(reclaim_buf_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for raw data chunk")

        /* Indicate that type conversion should be performed */
        do_convert = TRUE;
    } /* end if */
    else {
        if(H5T_get_class(dt_src, FALSE) == H5T_REFERENCE) {
            /* Indicate that type conversion should be performed */
            do_convert = TRUE;
        } /* end if */

        H5_ASSIGN_OVERFLOW(buf_size, layout_src->size, uint32_t, size_t);
        reclaim_buf_size = 0;
    } /* end else */

    /* Set up conversion buffer, if appropriate */
    if(do_convert) {
        /* Allocate background memory for converting the chunk */
        if(NULL == (bkg = H5MM_malloc(buf_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for raw data chunk")

        /* Check for reference datatype and no expanding references & clear background buffer */
        if(!cpy_info->expand_ref &&
                ((H5T_get_class(dt_src, FALSE) == H5T_REFERENCE) && (f_src != f_dst)))
            /* Reset value to zero */
            HDmemset(bkg, 0, buf_size);
    } /* end if */

    /* Allocate memory for copying the chunk */
    if(NULL == (buf = H5MM_malloc(buf_size)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for raw data chunk")

    /* Initialize the callback structure for the source */
    HDmemset(&udata, 0, sizeof udata);
    udata.common.layout = layout_src;
    udata.common.storage = storage_src;
    udata.file_src = f_src;
    udata.idx_info_dst = &idx_info_dst;
    udata.buf = buf;
    udata.bkg = bkg;
    udata.buf_size = buf_size;
    udata.tid_src = tid_src;
    udata.tid_mem = tid_mem;
    udata.tid_dst = tid_dst;
    udata.dt_src = dt_src;
    udata.do_convert = do_convert;
    udata.tpath_src_mem = tpath_src_mem;
    udata.tpath_mem_dst = tpath_mem_dst;
    udata.reclaim_buf = reclaim_buf;
    udata.reclaim_buf_size = reclaim_buf_size;
    udata.buf_space = buf_space;
    udata.nelmts = nelmts;
    udata.pline = pline;
    udata.cpy_info = cpy_info;

    /* Iterate over chunks to copy data */
    if((storage_src->ops->iterate)(&idx_info_src, H5D_chunk_copy_cb, &udata) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_BADITER, FAIL, "unable to iterate over chunk index to copy data")

    /* I/O buffers may have been re-allocated */
    buf = udata.buf;
    bkg = udata.bkg;

done:
    if(sid_buf > 0 && H5I_dec_ref(sid_buf, FALSE) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "can't decrement temporary dataspace ID")
    if(tid_src > 0)
        if(H5I_dec_ref(tid_src, FALSE) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't decrement temporary datatype ID")
    if(tid_dst > 0)
        if(H5I_dec_ref(tid_dst, FALSE) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't decrement temporary datatype ID")
    if(tid_mem > 0)
        if(H5I_dec_ref(tid_mem, FALSE) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't decrement temporary datatype ID")
    if(buf)
        H5MM_xfree(buf);
    if(bkg)
        H5MM_xfree(bkg);
    if(reclaim_buf)
        H5MM_xfree(reclaim_buf);

    /* Clean up any index information */
    if(copy_setup_done)
        if((storage_src->ops->copy_shutdown)(storage_src, storage_dst, dxpl_id) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "unable to shut down index copying info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_copy() */


/*-------------------------------------------------------------------------
 * Function:    H5D_chunk_bh_info
 *
 * Purpose:     Retrieve the amount of index storage for chunked dataset
 *
 * Return:      Success:        Non-negative
 *              Failure:        negative
 *
 * Programmer:  Vailin Choi
 *              June 8, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_bh_info(H5F_t *f, hid_t dxpl_id, H5O_layout_t *layout,
    const H5O_pline_t *pline, hsize_t *index_size)
{
    H5D_chk_idx_info_t idx_info;        /* Chunked index info */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5D_chunk_bh_info, FAIL)

    /* Check args */
    HDassert(f);
    HDassert(layout);
    HDassert(pline);
    HDassert(index_size);

    /* Compose chunked index info struct */
    idx_info.f = f;
    idx_info.dxpl_id = dxpl_id;
    idx_info.pline = pline;
    idx_info.layout = &layout->u.chunk;
    idx_info.storage = &layout->storage.u.chunk;

    /* Get size of index structure */
    if((layout->storage.u.chunk.ops->size)(&idx_info, index_size) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to retrieve chunk index info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_bh_info() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_iter_dump
 *
 * Purpose:	If the UDATA.STREAM member is non-null then debugging
 *              information is written to that stream.
 *
 * Return:	Success:	Non-negative
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Wednesday, April 21, 1999
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
H5D_chunk_dump_index_cb(const H5D_chunk_rec_t *chunk_rec, void *_udata)
{
    H5D_chunk_it_ud4_t	*udata = (H5D_chunk_it_ud4_t *)_udata;  /* User data from caller */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_chunk_dump_index_cb)

    if(udata->stream) {
        unsigned u;     /* Local index variable */

        /* Print header if not already displayed */
        if(!udata->header_displayed) {
            HDfprintf(udata->stream, "           Flags    Bytes     Address          Logical Offset\n");
            HDfprintf(udata->stream, "        ========== ======== ========== ==============================\n");

            /* Set flag that the headers has been printed */
            udata->header_displayed = TRUE;
        } /* end if */

        /* Print information about this chunk */
        HDfprintf(udata->stream,     "        0x%08x %8Zu %10a [", chunk_rec->filter_mask, chunk_rec->nbytes, chunk_rec->chunk_addr);
        for(u = 0; u < udata->ndims; u++)
            HDfprintf(udata->stream, "%s%Hd", (u ? ", " : ""), chunk_rec->offset[u]);
        HDfputs("]\n", udata->stream);
    } /* end if */

    FUNC_LEAVE_NOAPI(H5_ITER_CONT)
} /* H5D_chunk_dump_index_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_dump_index
 *
 * Purpose:	Prints information about the storage index to the specified
 *		stream.
 *
 * Return:	Success:	Non-negative
 *		Failure:	negative
 *
 * Programmer:	Robb Matzke
 *              Wednesday, April 28, 1999
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_dump_index(H5D_t *dset, hid_t dxpl_id, FILE *stream)
{
    herr_t ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5D_chunk_dump_index, FAIL)

    /* Sanity check */
    HDassert(dset);

    /* Only display info if stream is defined */
    if(stream) {
        H5D_chk_idx_info_t idx_info;    /* Chunked index info */
        H5D_chunk_it_ud4_t udata;       /* User data for callback */

        /* Display info for index */
        if((dset->shared->layout.storage.u.chunk.ops->dump)(&dset->shared->layout.storage.u.chunk, stream) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "unable to dump chunk index info")

        /* Compose chunked index info struct */
        idx_info.f = dset->oloc.file;
        idx_info.dxpl_id = dxpl_id;
        idx_info.pline = &dset->shared->dcpl_cache.pline;
        idx_info.layout = &dset->shared->layout.u.chunk;
        idx_info.storage = &dset->shared->layout.storage.u.chunk;

        /* Set up user data for callback */
        udata.stream = stream;
        udata.header_displayed = FALSE;
        udata.ndims = dset->shared->layout.u.chunk.ndims;

        /* Iterate over index and dump chunk info */
        if((dset->shared->layout.storage.u.chunk.ops->iterate)(&idx_info, H5D_chunk_dump_index_cb, &udata) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_BADITER, FAIL, "unable to iterate over chunk index to dump chunk info")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_dump_index() */


/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_dest
 *
 * Purpose:	Destroy the entire chunk cache by flushing dirty entries,
 *		preempting all entries, and freeing the cache itself.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, May 21, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_dest(H5F_t *f, hid_t dxpl_id, H5D_t *dset)
{
    H5D_chk_idx_info_t idx_info;        /* Chunked index info */
    H5D_dxpl_cache_t _dxpl_cache;       /* Data transfer property cache buffer */
    H5D_dxpl_cache_t *dxpl_cache = &_dxpl_cache;   /* Data transfer property cache */
    H5D_rdcc_t	*rdcc = &(dset->shared->cache.chunk);   /* Dataset's chunk cache */
    H5D_rdcc_ent_t	*ent = NULL, *next = NULL;      /* Pointer to current & next cache entries */
    int		nerrors = 0;            /* Accumulated count of errors */
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5D_chunk_dest, FAIL)

    HDassert(f);
    HDassert(dset);

    /* Fill the DXPL cache values for later use */
    if(H5D_get_dxpl_cache(dxpl_id, &dxpl_cache) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't fill dxpl cache")

    /* Flush all the cached chunks */
    for(ent = rdcc->head; ent; ent = next) {
	next = ent->next;
	if(H5D_chunk_cache_evict(dset, dxpl_id, dxpl_cache, ent, TRUE) < 0)
	    nerrors++;
    } /* end for */
    if(nerrors)
	HGOTO_ERROR(H5E_IO, H5E_CANTFLUSH, FAIL, "unable to flush one or more raw data chunks")

    /* Release cache structures */
    if(rdcc->slot)
        rdcc->slot = H5FL_SEQ_FREE(H5D_rdcc_ent_ptr_t, rdcc->slot);
    HDmemset(rdcc, 0, sizeof(H5D_rdcc_t));

    /* Compose chunked index info struct */
    idx_info.f = f;
    idx_info.dxpl_id = dxpl_id;
    idx_info.pline = &dset->shared->dcpl_cache.pline;
    idx_info.layout = &dset->shared->layout.u.chunk;
    idx_info.storage = &dset->shared->layout.storage.u.chunk;

    /* Free any index structures */
    if((dset->shared->layout.storage.u.chunk.ops->dest)(&idx_info) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "unable to release chunk index info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_dest() */

#ifdef H5D_CHUNK_DEBUG

/*-------------------------------------------------------------------------
 * Function:	H5D_chunk_stats
 *
 * Purpose:	Print raw data cache statistics to the debug stream.  If
 *		HEADERS is non-zero then print table column headers,
 *		otherwise assume that the H5AC layer has already printed them.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, May 21, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_chunk_stats(const H5D_t *dset, hbool_t headers)
{
    H5D_rdcc_t	*rdcc = &(dset->shared->cache.chunk);
    double	miss_rate;
    char	ascii[32];
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5D_chunk_stats, FAIL)

    if (!H5DEBUG(AC))
        HGOTO_DONE(SUCCEED)

    if (headers) {
        fprintf(H5DEBUG(AC), "H5D: raw data cache statistics\n");
        fprintf(H5DEBUG(AC), "   %-18s %8s %8s %8s %8s+%-8s\n",
            "Layer", "Hits", "Misses", "MissRate", "Inits", "Flushes");
        fprintf(H5DEBUG(AC), "   %-18s %8s %8s %8s %8s-%-8s\n",
            "-----", "----", "------", "--------", "-----", "-------");
    }

#ifdef H5AC_DEBUG
    if (H5DEBUG(AC)) headers = TRUE;
#endif

    if (headers) {
        if (rdcc->nhits>0 || rdcc->nmisses>0) {
            miss_rate = 100.0 * rdcc->nmisses /
                    (rdcc->nhits + rdcc->nmisses);
        } else {
            miss_rate = 0.0;
        }
        if (miss_rate > 100) {
            sprintf(ascii, "%7d%%", (int) (miss_rate + 0.5));
        } else {
            sprintf(ascii, "%7.2f%%", miss_rate);
        }

        fprintf(H5DEBUG(AC), "   %-18s %8u %8u %7s %8d+%-9ld\n",
            "raw data chunks", rdcc->nhits, rdcc->nmisses, ascii,
            rdcc->ninits, (long)(rdcc->nflushes)-(long)(rdcc->ninits));
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_stats() */
#endif /* H5D_CHUNK_DEBUG */


/*-------------------------------------------------------------------------
 * Function:	H5D_nonexistent_readvv
 *
 * Purpose:	When the chunk doesn't exist on disk and the chunk is bigger
 *              than the cache size, performs fill value I/O operation on
 *              memory buffer, advancing through two I/O vectors, until one
 *              runs out.
 *
 * Note:	This algorithm is pretty inefficient about initializing and
 *              terminating the fill buffer info structure and it would be
 *              faster to refactor this into a "real" initialization routine,
 *              and a "vectorized fill" routine. -QAK
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *		6 Feb 2009
 *
 *-------------------------------------------------------------------------
 */
static ssize_t
H5D_nonexistent_readvv(const H5D_io_info_t *io_info,
    size_t chunk_max_nseq, size_t *chunk_curr_seq, size_t chunk_len_arr[], hsize_t chunk_offset_arr[],
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_len_arr[], hsize_t mem_offset_arr[])
{
    H5D_t *dset = io_info->dset;    /* Local pointer to the dataset info */
    H5D_fill_buf_info_t fb_info;    /* Dataset's fill buffer info */
    hbool_t fb_info_init = FALSE;   /* Whether the fill value buffer has been initialized */
    ssize_t bytes_processed = 0;    /* Eventual return value */
    size_t u, v;                    /* Local index variables */
    ssize_t ret_value;              /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_nonexistent_readvv)

    /* Check args */
    HDassert(chunk_len_arr);
    HDassert(chunk_offset_arr);
    HDassert(mem_len_arr);
    HDassert(mem_offset_arr);

    /* Work through all the sequences */
    for(u = *mem_curr_seq, v = *chunk_curr_seq; u < mem_max_nseq && v < chunk_max_nseq; ) {
        unsigned char *buf;     /* Temporary pointer into read buffer */
        size_t size;            /* Size of sequence in bytes */

        /* Choose smallest buffer to write */
        if(chunk_len_arr[v] < mem_len_arr[u])
            size = chunk_len_arr[v];
        else
            size = mem_len_arr[u];

        /* Compute offset in memory */
        buf = (unsigned char *)io_info->u.rbuf + mem_offset_arr[u];

	/* Initialize the fill value buffer */
	if(H5D_fill_init(&fb_info, buf, FALSE,
		NULL, NULL, NULL, NULL,
		&dset->shared->dcpl_cache.fill, dset->shared->type,
		dset->shared->type_id, (size_t)0, size, io_info->dxpl_id) < 0)
	    HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't initialize fill buffer info")
        fb_info_init = TRUE;

	/* Check for VL datatype & fill the buffer with VL datatype fill values */
	if(fb_info.has_vlen_fill_type && H5D_fill_refill_vl(&fb_info, fb_info.elmts_per_buf, io_info->dxpl_id) < 0)
	    HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, FAIL, "can't refill fill value buffer")

        /* Release the fill buffer info */
        if(H5D_fill_term(&fb_info) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't release fill buffer info")
        fb_info_init = FALSE;

        /* Update source information */
        chunk_len_arr[v] -= size;
        chunk_offset_arr[v] += size;
        if(chunk_len_arr[v] == 0)
            v++;

        /* Update destination information */
        mem_len_arr[u] -= size;
        mem_offset_arr[u] += size;
        if(mem_len_arr[u] == 0)
            u++;

        /* Increment number of bytes copied */
        bytes_processed += (ssize_t)size;
    } /* end for */

    /* Update current sequence vectors */
    *mem_curr_seq = u;
    *chunk_curr_seq = v;

    /* Set return value */
    ret_value = bytes_processed;

done:
    /* Release the fill buffer info, if it's been initialized */
    if(fb_info_init && H5D_fill_term(&fb_info) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't release fill buffer info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D_nonexistent_readvv() */

