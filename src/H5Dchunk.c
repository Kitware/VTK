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

/* Programmer: 	Quincey Koziol <koziol@hdfgroup.org>
 *	       	Thursday, April 24, 2008
 *
 * Purpose:	Abstract indexed (chunked) I/O functions.  The logical
 *		multi-dimensional dataspace is regularly partitioned into
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

#include "H5Dmodule.h"          /* This source code file is part of the H5D module */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#ifdef H5_HAVE_PARALLEL
#include "H5ACprivate.h"	/* Metadata cache			*/
#endif /* H5_HAVE_PARALLEL */
#include "H5Dpkg.h"		/* Dataset functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"		/* File functions			*/
#include "H5FLprivate.h"	/* Free Lists                           */
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5MFprivate.h"        /* File memory management               */
#include "H5VMprivate.h"	/* Vector and array functions		*/


/****************/
/* Local Macros */
/****************/

/* Macros for iterating over chunks to operate on */
#define H5D_CHUNK_GET_FIRST_NODE(map) (map->use_single ? (H5SL_node_t *)(1) : H5SL_first(map->sel_chunks))
#define H5D_CHUNK_GET_NODE_INFO(map, node)  (map->use_single ? map->single_chunk_info : (H5D_chunk_info_t *)H5SL_item(node))
#define H5D_CHUNK_GET_NEXT_NODE(map, node)  (map->use_single ? (H5SL_node_t *)NULL : H5SL_next(node))

/* Sanity check on chunk index types: commonly used by a lot of routines in this file */
#define H5D_CHUNK_STORAGE_INDEX_CHK(storage)                                                    \
    HDassert((H5D_CHUNK_IDX_EARRAY == storage->idx_type && H5D_COPS_EARRAY == storage->ops) ||  \
             (H5D_CHUNK_IDX_FARRAY == storage->idx_type && H5D_COPS_FARRAY == storage->ops) ||  \
             (H5D_CHUNK_IDX_BT2 == storage->idx_type && H5D_COPS_BT2 == storage->ops) ||        \
             (H5D_CHUNK_IDX_BTREE == storage->idx_type && H5D_COPS_BTREE == storage->ops) ||    \
             (H5D_CHUNK_IDX_SINGLE == storage->idx_type && H5D_COPS_SINGLE == storage->ops) ||  \
             (H5D_CHUNK_IDX_NONE == storage->idx_type && H5D_COPS_NONE == storage->ops));

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

/* Flags for the "edge_chunk_state" field below */
#define H5D_RDCC_DISABLE_FILTERS 0x01u          /* Disable filters on this chunk */
#define H5D_RDCC_NEWLY_DISABLED_FILTERS 0x02u   /* Filters have been disabled since
                                                 * the last flush */


/******************/
/* Local Typedefs */
/******************/

/* Raw data chunks are cached.  Each entry in the cache is: */
typedef struct H5D_rdcc_ent_t {
    hbool_t	locked;		/*entry is locked in cache		*/
    hbool_t	dirty;		/*needs to be written to disk?		*/
    hbool_t     deleted;        /*chunk about to be deleted		*/
    unsigned    edge_chunk_state; /*states related to edge chunks (see above) */
    hsize_t 	scaled[H5O_LAYOUT_NDIMS]; /*scaled chunk 'name' (coordinates) */
    uint32_t	rd_count;	/*bytes remaining to be read		*/
    uint32_t	wr_count;	/*bytes remaining to be written		*/
    H5F_block_t chunk_block;    /*offset/length of chunk in file        */
    hsize_t     chunk_idx;  	/*index of chunk in dataset             */
    uint8_t	*chunk;		/*the unfiltered chunk data		*/
    unsigned	idx;		/*index in hash table			*/
    struct H5D_rdcc_ent_t *next;/*next item in doubly-linked list	*/
    struct H5D_rdcc_ent_t *prev;/*previous item in doubly-linked list	*/
    struct H5D_rdcc_ent_t *tmp_next;/*next item in temporary doubly-linked list */
    struct H5D_rdcc_ent_t *tmp_prev;/*previous item in temporary doubly-linked list */
} H5D_rdcc_ent_t;
typedef H5D_rdcc_ent_t *H5D_rdcc_ent_ptr_t; /* For free lists */

/* Callback info for iteration to prune chunks */
typedef struct H5D_chunk_it_ud1_t {
    H5D_chunk_common_ud_t common;       /* Common info for B-tree user data (must be first) */
    const H5D_chk_idx_info_t *idx_info; /* Chunked index info */
    const H5D_io_info_t *io_info;       /* I/O info for dataset operation */
    const hsize_t	*space_dim;	/* New dataset dimensions	*/
    const hbool_t       *shrunk_dim;    /* Dimensions which have been shrunk */
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
    unsigned            dset_ndims;             /* Number of dimensions in dataset */
    const hsize_t       *dset_dims;             /* Dataset dimensions */

    /* needed for copy object pointed by refs */
    H5O_copy_t          *cpy_info;              /* Copy options */
} H5D_chunk_it_ud3_t;

/* Callback info for iteration to dump index */
typedef struct H5D_chunk_it_ud4_t {
    FILE		*stream;		/* Output stream	*/
    hbool_t             header_displayed;       /* Node's header is displayed? */
    unsigned            ndims;                  /* Number of dimensions for chunk/dataset */
    uint32_t            *chunk_dim;             /* Chunk dimensions */
} H5D_chunk_it_ud4_t;

/* Callback info for iteration to format convert chunks */
typedef struct H5D_chunk_it_ud5_t {
    H5D_chk_idx_info_t  *new_idx_info;          /* Dest. chunk index info object */
    unsigned            dset_ndims;             /* Number of dimensions in dataset */
    hsize_t       	*dset_dims;             /* Dataset dimensions */
} H5D_chunk_it_ud5_t;

/* Callback info for nonexistent readvv operation */
typedef struct H5D_chunk_readvv_ud_t {
    unsigned char *rbuf;        /* Read buffer to initialize */
    const H5D_t *dset;          /* Dataset to operate on */
    hid_t dxpl_id;              /* DXPL for operation */
} H5D_chunk_readvv_ud_t;

/* Callback info for file selection iteration */
typedef struct H5D_chunk_file_iter_ud_t {
    H5D_chunk_map_t *fm;                /* File->memory chunk mapping info */
#ifdef H5_HAVE_PARALLEL
    const H5D_io_info_t *io_info;       /* I/O info for operation */
#endif /* H5_HAVE_PARALLEL */
} H5D_chunk_file_iter_ud_t;

#ifdef H5_HAVE_PARALLEL
/* information to construct a collective I/O operation for filling chunks */
typedef struct H5D_chunk_coll_info_t {
    size_t num_io;       /* Number of write operations */
    haddr_t *addr;       /* array of the file addresses of the write operation */
} H5D_chunk_coll_info_t;
#endif /* H5_HAVE_PARALLEL */

/********************/
/* Local Prototypes */
/********************/

/* Chunked layout operation callbacks */
static herr_t H5D__chunk_construct(H5F_t *f, H5D_t *dset);
static herr_t H5D__chunk_init(H5F_t *f, hid_t dxpl_id, const H5D_t *dset,
    hid_t dapl_id);
static herr_t H5D__chunk_io_init(const H5D_io_info_t *io_info,
    const H5D_type_info_t *type_info, hsize_t nelmts, const H5S_t *file_space,
    const H5S_t *mem_space, H5D_chunk_map_t *fm);
static herr_t H5D__chunk_read(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t nelmts, const H5S_t *file_space, const H5S_t *mem_space,
    H5D_chunk_map_t *fm);
static herr_t H5D__chunk_write(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t nelmts, const H5S_t *file_space, const H5S_t *mem_space,
    H5D_chunk_map_t *fm);
static herr_t H5D__chunk_flush(H5D_t *dset, hid_t dxpl_id);
static herr_t H5D__chunk_io_term(const H5D_chunk_map_t *fm);
static herr_t H5D__chunk_dest(H5D_t *dset, hid_t dxpl_id);

/* "Nonexistent" layout operation callback */
static ssize_t
H5D__nonexistent_readvv(const H5D_io_info_t *io_info,
    size_t chunk_max_nseq, size_t *chunk_curr_seq, size_t chunk_len_arr[], hsize_t chunk_offset_arr[],
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_len_arr[], hsize_t mem_offset_arr[]);

/* format convert cb */
static int H5D__chunk_format_convert_cb(const H5D_chunk_rec_t *chunk_rec, void *_udata);

/* Helper routines */
static herr_t H5D__chunk_set_info_real(H5O_layout_chunk_t *layout, unsigned ndims,
    const hsize_t *curr_dims, const hsize_t *max_dims);
static void *H5D__chunk_mem_alloc(size_t size, const H5O_pline_t *pline);
static void *H5D__chunk_mem_xfree(void *chk, const H5O_pline_t *pline);
static void *H5D__chunk_mem_realloc(void *chk, size_t size,
    const H5O_pline_t *pline);
static herr_t H5D__chunk_cinfo_cache_reset(H5D_chunk_cached_t *last);
static herr_t H5D__chunk_cinfo_cache_update(H5D_chunk_cached_t *last,
    const H5D_chunk_ud_t *udata);
static hbool_t H5D__chunk_cinfo_cache_found(const H5D_chunk_cached_t *last,
    H5D_chunk_ud_t *udata);
static herr_t H5D__free_chunk_info(void *item, void *key, void *opdata);
static herr_t H5D__create_chunk_map_single(H5D_chunk_map_t *fm,
    const H5D_io_info_t *io_info);
static herr_t H5D__create_chunk_file_map_hyper(H5D_chunk_map_t *fm,
    const H5D_io_info_t *io_info);
static herr_t H5D__create_chunk_mem_map_hyper(const H5D_chunk_map_t *fm);
static herr_t H5D__chunk_file_cb(void *elem, const H5T_t *type, unsigned ndims,
    const hsize_t *coords, void *fm);
static herr_t H5D__chunk_mem_cb(void *elem, const H5T_t *type, unsigned ndims,
    const hsize_t *coords, void *fm);
static unsigned H5D__chunk_hash_val(const H5D_shared_t *shared, const hsize_t *scaled);
static herr_t H5D__chunk_flush_entry(const H5D_t *dset, hid_t dxpl_id,
    const H5D_dxpl_cache_t *dxpl_cache, H5D_rdcc_ent_t *ent, hbool_t reset);
static herr_t H5D__chunk_cache_evict(const H5D_t *dset, hid_t dxpl_id,
    const H5D_dxpl_cache_t *dxpl_cache, H5D_rdcc_ent_t *ent, hbool_t flush);
static hbool_t H5D__chunk_is_partial_edge_chunk(unsigned dset_ndims,
    const uint32_t *chunk_dims, const hsize_t *chunk_scaled, const hsize_t *dset_dims);
static void *H5D__chunk_lock(const H5D_io_info_t *io_info,
    H5D_chunk_ud_t *udata, hbool_t relax, hbool_t prev_unfilt_chunk);
static herr_t H5D__chunk_unlock(const H5D_io_info_t *io_info,
    const H5D_chunk_ud_t *udata, hbool_t dirty, void *chunk,
    uint32_t naccessed);
static herr_t H5D__chunk_cache_prune(const H5D_t *dset, hid_t dxpl_id,
    const H5D_dxpl_cache_t *dxpl_cache, size_t size);
static herr_t H5D__chunk_prune_fill(H5D_chunk_it_ud1_t *udata, hbool_t new_unfilt_chunk);
static herr_t H5D__chunk_file_alloc(const H5D_chk_idx_info_t *idx_info,
    const H5F_block_t *old_chunk, H5F_block_t *new_chunk, hbool_t *need_insert,
    hsize_t scaled[]);
#ifdef H5_HAVE_PARALLEL
static herr_t H5D__chunk_collective_fill(const H5D_t *dset, hid_t dxpl_id,
    H5D_chunk_coll_info_t *chunk_info, size_t chunk_size, const void *fill_buf);
#endif /* H5_HAVE_PARALLEL */

/*********************/
/* Package Variables */
/*********************/

/* Chunked storage layout I/O ops */
const H5D_layout_ops_t H5D_LOPS_CHUNK[1] = {{
    H5D__chunk_construct,
    H5D__chunk_init,
    H5D__chunk_is_space_alloc,
    H5D__chunk_io_init,
    H5D__chunk_read,
    H5D__chunk_write,
#ifdef H5_HAVE_PARALLEL
    H5D__chunk_collective_read,
    H5D__chunk_collective_write,
#endif /* H5_HAVE_PARALLEL */
    NULL,
    NULL,
    H5D__chunk_flush,
    H5D__chunk_io_term,
    H5D__chunk_dest
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
    H5D__nonexistent_readvv,
    NULL,
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

/* Declare extern free list to manage the H5S_sel_iter_t struct */
H5FL_EXTERN(H5S_sel_iter_t);


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_direct_write
 *
 * Purpose:	Internal routine to write a chunk 
 *              directly into the file.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *              30 July 2012
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__chunk_direct_write(const H5D_t *dset, hid_t dxpl_id, uint32_t filters, 
    hsize_t *offset, uint32_t data_size, const void *buf)
{
    const H5O_layout_t *layout = &(dset->shared->layout);       /* Dataset layout */
    H5D_chunk_ud_t udata;               /* User data for querying chunk info */
    H5F_block_t old_chunk;              /* Offset/length of old chunk */
    H5D_chk_idx_info_t idx_info;        /* Chunked index info */
    hsize_t scaled[H5S_MAX_RANK];       /* Scaled coordinates for this chunk */
    hbool_t need_insert = FALSE;        /* Whether the chunk needs to be inserted into the index */
    H5D_io_info_t io_info;              /* to hold the dset and two dxpls (meta and raw data) */
    hbool_t md_dxpl_generated = FALSE;  /* bool to indicate whether we should free the md_dxpl_id at exit */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC_TAG(dxpl_id, dset->oloc.addr, FAIL)

    io_info.dset = dset;
    io_info.raw_dxpl_id = dxpl_id;
    io_info.md_dxpl_id = dxpl_id;

    /* set the dxpl IO type for sanity checking at the FD layer */
#ifdef H5_DEBUG_BUILD
    if(H5D_set_io_info_dxpls(&io_info, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTCOPY, FAIL, "can't set metadata and raw data dxpls")
    md_dxpl_generated = TRUE;
#endif /* H5_DEBUG_BUILD */

    /* Allocate dataspace and initialize it if it hasn't been. */
    if(!(*layout->ops->is_space_alloc)(&layout->storage)) {
 	/* Allocate storage */
        if(H5D__alloc_storage(&io_info, H5D_ALLOC_WRITE, FALSE, NULL) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize storage")
    }
    /* Calculate the index of this chunk */
    H5VM_chunk_scaled(dset->shared->ndims, offset, layout->u.chunk.dim, scaled);
    scaled[dset->shared->ndims] = 0;

    /* Find out the file address of the chunk (if any) */
    if(H5D__chunk_lookup(dset, io_info.md_dxpl_id, scaled, &udata) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "error looking up chunk address")

    /* Sanity check */
    HDassert((H5F_addr_defined(udata.chunk_block.offset) && udata.chunk_block.length > 0) || 
            (!H5F_addr_defined(udata.chunk_block.offset) && udata.chunk_block.length == 0));

    /* Set the file block information for the old chunk */
    /* (Which is only defined when overwriting an existing chunk) */
    old_chunk.offset = udata.chunk_block.offset;
    old_chunk.length = udata.chunk_block.length;

    /* Check if the chunk needs to be inserted (it also could exist already
     *      and the chunk allocate operation could resize it)
     */

    /* Compose chunked index info struct */
    idx_info.f = dset->oloc.file;
    idx_info.dxpl_id = io_info.md_dxpl_id;
    idx_info.pline = &(dset->shared->dcpl_cache.pline);
    idx_info.layout = &(dset->shared->layout.u.chunk);
    idx_info.storage = &(dset->shared->layout.storage.u.chunk);

    /* Set up the size of chunk for user data */
    udata.chunk_block.length = data_size;

    /* Create the chunk it if it doesn't exist, or reallocate the chunk
     *  if its size changed.
     */
    if(H5D__chunk_file_alloc(&idx_info, &old_chunk, &udata.chunk_block, &need_insert, scaled) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "unable to allocate chunk")

    /* Make sure the address of the chunk is returned. */
    if(!H5F_addr_defined(udata.chunk_block.offset))
        HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, FAIL, "chunk address isn't defined")

    /* Evict the (old) entry from the cache if present, but do not flush
     * it to disk */
    if(UINT_MAX != udata.idx_hint) {
        H5D_dxpl_cache_t _dxpl_cache;       /* Data transfer property cache buffer */
        H5D_dxpl_cache_t *dxpl_cache = &_dxpl_cache;   /* Data transfer property cache */
        const H5D_rdcc_t *rdcc = &(dset->shared->cache.chunk);	/*raw data chunk cache */

        /* Fill the DXPL cache values for later use */
        if(H5D__get_dxpl_cache(io_info.raw_dxpl_id, &dxpl_cache) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't fill dxpl cache")

        if(H5D__chunk_cache_evict(dset, io_info.md_dxpl_id, dxpl_cache, rdcc->slot[udata.idx_hint], FALSE) < 0)
	    HGOTO_ERROR(H5E_DATASET, H5E_CANTREMOVE, FAIL, "unable to evict chunk")
    } /* end if */

    /* Write the data to the file */
    if(H5F_block_write(dset->oloc.file, H5FD_MEM_DRAW, udata.chunk_block.offset, data_size, io_info.raw_dxpl_id, buf) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to write raw data to file")

    /* Insert the chunk record into the index */
    if(need_insert && layout->storage.u.chunk.ops->insert) {
        /* Set the chunk's filter mask to the new settings */
        udata.filter_mask = filters;

        if((layout->storage.u.chunk.ops->insert)(&idx_info, &udata, NULL) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, FAIL, "unable to insert chunk addr into index")
    } /* end if */

done:
#ifdef H5_DEBUG_BUILD
    if(md_dxpl_generated && H5I_dec_ref(io_info.md_dxpl_id) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTDEC, FAIL, "can't close metadata dxpl")
#endif /* H5_DEBUG_BUILD */
    FUNC_LEAVE_NOAPI_TAG(ret_value, FAIL)
} /* end H5D__chunk_direct_write() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_set_info_real
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
H5D__chunk_set_info_real(H5O_layout_chunk_t *layout, unsigned ndims,
    const hsize_t *curr_dims, const hsize_t *max_dims)
{
    unsigned u;                 /* Local index variable */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity checks */
    HDassert(layout);
    HDassert(ndims > 0);
    HDassert(curr_dims);

    /* Compute the # of chunks in dataset dimensions */
    for(u = 0, layout->nchunks = 1, layout->max_nchunks = 1; u < ndims; u++) {
        /* Round up to the next integer # of chunks, to accomodate partial chunks */
	layout->chunks[u] = ((curr_dims[u] + layout->dim[u]) - 1) / layout->dim[u];
        if(H5S_UNLIMITED == max_dims[u])
            layout->max_chunks[u] = H5S_UNLIMITED;
        else
            layout->max_chunks[u] = ((max_dims[u] + layout->dim[u]) - 1) / layout->dim[u];

        /* Accumulate the # of chunks */
	layout->nchunks *= layout->chunks[u];
	layout->max_nchunks *= layout->max_chunks[u];
    } /* end for */

    /* Get the "down" sizes for each dimension */
    if(H5VM_array_down(ndims, layout->chunks, layout->down_chunks) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't compute 'down' chunk size value")
    if(H5VM_array_down(ndims, layout->max_chunks, layout->max_down_chunks) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't compute 'down' chunk size value")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_set_info_real() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_set_info
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
H5D__chunk_set_info(const H5D_t *dset)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity checks */
    HDassert(dset);

    /* Set the base layout information */
    if(H5D__chunk_set_info_real(&dset->shared->layout.u.chunk, dset->shared->ndims, dset->shared->curr_dims, dset->shared->max_dims) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set layout's chunk info")

    /* Call the index's "resize" callback */
    if(dset->shared->layout.storage.u.chunk.ops->resize && (dset->shared->layout.storage.u.chunk.ops->resize)(&dset->shared->layout.u.chunk) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to resize chunk index information")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_set_info() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_set_sizes
 *
 * Purpose:     Sets chunk and type sizes.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:	Dana Robinson
 *              December 2015
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__chunk_set_sizes(H5D_t *dset)
{
    uint64_t chunk_size;            /* Size of chunk in bytes */
    unsigned max_enc_bytes_per_dim; /* Max. number of bytes required to encode this dimension */
    unsigned u;                     /* Iterator */
    herr_t ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(dset);

    /* Increment # of chunk dimensions, to account for datatype size as last element */
    dset->shared->layout.u.chunk.ndims++;

    /* Set the last dimension of the chunk size to the size of the datatype */
    dset->shared->layout.u.chunk.dim[dset->shared->layout.u.chunk.ndims - 1] = (uint32_t)H5T_GET_SIZE(dset->shared->type);

    /* Compute number of bytes to use for encoding chunk dimensions */
    max_enc_bytes_per_dim = 0;
    for(u = 0; u < (unsigned)dset->shared->layout.u.chunk.ndims; u++) {
        unsigned enc_bytes_per_dim;     /* Number of bytes required to encode this dimension */

        /* Get encoded size of dim, in bytes */
        enc_bytes_per_dim = (H5VM_log2_gen(dset->shared->layout.u.chunk.dim[u]) + 8) / 8;

        /* Check if this is the largest value so far */
        if(enc_bytes_per_dim > max_enc_bytes_per_dim)
            max_enc_bytes_per_dim = enc_bytes_per_dim;
    } /* end for */
    HDassert(max_enc_bytes_per_dim > 0 && max_enc_bytes_per_dim <= 8);
    dset->shared->layout.u.chunk.enc_bytes_per_dim = max_enc_bytes_per_dim;

    /* Compute and store the total size of a chunk */
    /* (Use 64-bit value to ensure that we can detect >4GB chunks) */
    for(u = 1, chunk_size = (uint64_t)dset->shared->layout.u.chunk.dim[0]; u < dset->shared->layout.u.chunk.ndims; u++)
        chunk_size *= (uint64_t)dset->shared->layout.u.chunk.dim[u];

    /* Check for chunk larger than can be represented in 32-bits */
    /* (Chunk size is encoded in 32-bit value in v1 B-tree records) */
    if(chunk_size > (uint64_t)0xffffffff)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "chunk size must be < 4GB")

    H5_CHECKED_ASSIGN(dset->shared->layout.u.chunk.size, uint32_t, chunk_size, uint64_t);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_set_sizes */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_construct
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
H5D__chunk_construct(H5F_t H5_ATTR_UNUSED *f, H5D_t *dset)
{
    unsigned u;                 /* Local index variable */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity checks */
    HDassert(f);
    HDassert(dset);

    /* Check for invalid chunk dimension rank */
    if(0 == dset->shared->layout.u.chunk.ndims)
        HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, FAIL, "no chunk information set?")
    if(dset->shared->layout.u.chunk.ndims != dset->shared->ndims)
        HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, FAIL, "dimensionality of chunks doesn't match the dataspace")

    /* Set chunk sizes */
    if(H5D__chunk_set_sizes(dset) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, FAIL, "unable to set chunk sizes")
    HDassert((unsigned)(dset->shared->layout.u.chunk.ndims) <= NELMTS(dset->shared->layout.u.chunk.dim));

    /* Chunked storage is not compatible with external storage (currently) */
    if(dset->shared->dcpl_cache.efl.nused > 0)
        HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, FAIL, "external storage not supported with chunked layout")

    /* Sanity check dimensions */
    for(u = 0; u < dset->shared->layout.u.chunk.ndims - 1; u++) {
        /* Don't allow zero-sized chunk dimensions */
        if(0 == dset->shared->layout.u.chunk.dim[u])
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "chunk size must be > 0, dim = %u ", u)

        /*
         * The chunk size of a dimension with a fixed size cannot exceed
         * the maximum dimension size. If any dimension size is zero, there
         * will be no such restriction.
         */
        if(dset->shared->curr_dims[u] && dset->shared->max_dims[u] != H5S_UNLIMITED && dset->shared->max_dims[u] < dset->shared->layout.u.chunk.dim[u])
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "chunk size must be <= maximum dimension size for fixed-sized dimensions")
    } /* end for */

    /* Reset address and pointer of the array struct for the chunked storage index */
    if(H5D_chunk_idx_reset(&dset->shared->layout.storage.u.chunk, TRUE) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to reset chunked storage index")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_construct() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_init
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
static herr_t
H5D__chunk_init(H5F_t *f, hid_t dxpl_id, const H5D_t *dset, hid_t dapl_id)
{
    H5D_chk_idx_info_t idx_info;        /* Chunked index info */
    H5D_rdcc_t	*rdcc = &(dset->shared->cache.chunk);   /* Convenience pointer to dataset's chunk cache */
    H5P_genplist_t *dapl;               /* Data access property list object pointer */
    H5O_storage_chunk_t *sc = &(dset->shared->layout.storage.u.chunk);
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(f);
    HDassert(dset);
    H5D_CHUNK_STORAGE_INDEX_CHK(sc);

    if(NULL == (dapl = (H5P_genplist_t *)H5I_object(dapl_id)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for fapl ID")

    /* Use the properties in dapl_id if they have been set, otherwise use the properties from the file */
    if(H5P_get(dapl, H5D_ACS_DATA_CACHE_NUM_SLOTS_NAME, &rdcc->nslots) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET,FAIL, "can't get data cache number of slots")
    if(rdcc->nslots == H5D_CHUNK_CACHE_NSLOTS_DEFAULT)
        rdcc->nslots = H5F_RDCC_NSLOTS(f);

    if(H5P_get(dapl, H5D_ACS_DATA_CACHE_BYTE_SIZE_NAME, &rdcc->nbytes_max) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET,FAIL, "can't get data cache byte size")
    if(rdcc->nbytes_max == H5D_CHUNK_CACHE_NBYTES_DEFAULT)
        rdcc->nbytes_max = H5F_RDCC_NBYTES(f);

    if(H5P_get(dapl, H5D_ACS_PREEMPT_READ_CHUNKS_NAME, &rdcc->w0) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET,FAIL, "can't get preempt read chunks")
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
        H5D__chunk_cinfo_cache_reset(&(rdcc->last));
    } /* end else */

    /* Compute scaled dimension info, if dataset dims > 1 */
    if(dset->shared->ndims > 1) {
        unsigned u;                         /* Local index value */

        for(u = 0; u < dset->shared->ndims; u++) {
            /* Initial scaled dimension sizes */
            rdcc->scaled_dims[u] = dset->shared->curr_dims[u] / dset->shared->layout.u.chunk.dim[u];

            /* Inital 'power2up' values for scaled dimensions */
            rdcc->scaled_power2up[u] = H5VM_power2up(rdcc->scaled_dims[u]);

            /* Number of bits required to encode scaled dimension size */
            rdcc->scaled_encode_bits[u] = H5VM_log2_gen(rdcc->scaled_power2up[u]);
        } /* end for */
    } /* end if */

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
    if(H5D__chunk_set_info(dset) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to set # of chunks for dataset")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_init() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_is_space_alloc
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
H5D__chunk_is_space_alloc(const H5O_storage_t *storage)
{
    const H5O_storage_chunk_t *sc = &(storage->u.chunk);
    hbool_t ret_value = FALSE;          /* Return value */

    FUNC_ENTER_PACKAGE_NOERR

    /* Sanity checks */
    HDassert(storage);
    H5D_CHUNK_STORAGE_INDEX_CHK(sc);

    /* Query index layer */
    ret_value = (storage->u.chunk.ops->is_space_alloc)(&storage->u.chunk);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_is_space_alloc() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_io_init
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
H5D__chunk_io_init(const H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t nelmts, const H5S_t *file_space, const H5S_t *mem_space,
    H5D_chunk_map_t *fm)
{
    const H5D_t *dataset = io_info->dset;     /* Local pointer to dataset info */
    const H5T_t *mem_type = type_info->mem_type;        /* Local pointer to memory datatype */
    H5S_t *tmp_mspace = NULL;   /* Temporary memory dataspace */
    hssize_t old_offset[H5O_LAYOUT_NDIMS];  /* Old selection offset */
    htri_t file_space_normalized = FALSE;   /* File dataspace was normalized */
    H5T_t *file_type = NULL;    /* Temporary copy of file datatype for iteration */
    hbool_t iter_init = FALSE;  /* Selection iteration info has been initialized */
    unsigned f_ndims;           /* The number of dimensions of the file's dataspace */
    int sm_ndims;               /* The number of dimensions of the memory buffer's dataspace (signed) */
    H5SL_node_t *curr_node;     /* Current node in skip list */
    char bogus;                 /* "bogus" buffer to pass to selection iterator */
    unsigned u;                 /* Local index variable */
    herr_t ret_value = SUCCEED;	/* Return value		*/

    FUNC_ENTER_STATIC

    /* Get layout for dataset */
    fm->layout = &(dataset->shared->layout);
    fm->nelmts = nelmts;

    /* Check if the memory space is scalar & make equivalent memory space */
    if((sm_ndims = H5S_GET_EXTENT_NDIMS(mem_space)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "unable to get dimension number")
    /* Set the number of dimensions for the memory dataspace */
    H5_CHECKED_ASSIGN(fm->m_ndims, unsigned, sm_ndims, int);

    /* Get rank for file dataspace */
    fm->f_ndims = f_ndims = dataset->shared->layout.u.chunk.ndims - 1;

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
        if(fm->layout->u.chunk.nchunks) {
            if(NULL == (fm->select_chunk = (H5D_chunk_info_t **)H5MM_calloc((size_t)fm->layout->u.chunk.nchunks * sizeof(H5D_chunk_info_t *))))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate chunk info")
        }
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
            && H5S_SEL_ALL != H5S_GET_SELECT_TYPE(file_space)) {
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
        if(H5D__create_chunk_map_single(fm, io_info) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to create chunk selections for single element")
    } /* end if */
    else {
        hbool_t sel_hyper_flag;         /* Whether file selection is a hyperslab */

        /* Initialize skip list for chunk selections */
        if(NULL == dataset->shared->cache.chunk.sel_chunks) {
            if(NULL == (dataset->shared->cache.chunk.sel_chunks = H5SL_create(H5SL_TYPE_HSIZE, NULL)))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTCREATE, FAIL, "can't create skip list for chunk selections")
        } /* end if */
        fm->sel_chunks = dataset->shared->cache.chunk.sel_chunks;
        HDassert(fm->sel_chunks);

        /* We are not using single element mode */
        fm->use_single = FALSE;

        /* Get type of selection on disk & in memory */
        if((fm->fsel_type = H5S_GET_SELECT_TYPE(file_space)) < H5S_SEL_NONE)
            HGOTO_ERROR(H5E_DATASET, H5E_BADSELECT, FAIL, "unable to get type of selection")
        if((fm->msel_type = H5S_GET_SELECT_TYPE(mem_space)) < H5S_SEL_NONE)
            HGOTO_ERROR(H5E_DATASET, H5E_BADSELECT, FAIL, "unable to get type of selection")

        /* If the selection is NONE or POINTS, set the flag to FALSE */
        if(fm->fsel_type == H5S_SEL_POINTS || fm->fsel_type == H5S_SEL_NONE)
            sel_hyper_flag = FALSE;
        else
            sel_hyper_flag = TRUE;

        /* Check if file selection is a not a hyperslab selection */
        if(sel_hyper_flag) {
            /* Build the file selection for each chunk */
            if(H5D__create_chunk_file_map_hyper(fm, io_info) < 0)
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
            H5S_sel_iter_op_t iter_op;  /* Operator for iteration */
            H5D_chunk_file_iter_ud_t udata;     /* User data for iteration */

            /* Create temporary datatypes for selection iteration */
            if(NULL == (file_type = H5T_copy(dataset->shared->type, H5T_COPY_ALL)))
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, FAIL, "unable to copy file datatype")

            /* Initialize the user data */
            udata.fm = fm;
#ifdef H5_HAVE_PARALLEL
            udata.io_info = io_info;
#endif /* H5_HAVE_PARALLEL */

            iter_op.op_type = H5S_SEL_ITER_OP_LIB;
            iter_op.u.lib_op = H5D__chunk_file_cb;

            /* Spaces might not be the same shape, iterate over the file selection directly */
            if(H5S_select_iterate(&bogus, file_type, file_space, &iter_op, &udata) < 0)
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
            if(H5D__create_chunk_mem_map_hyper(fm) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to create memory chunk selections")
        } /* end if */
        else {
            H5S_sel_iter_op_t iter_op;  /* Operator for iteration */
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
            if(!file_type) {
                if(NULL == (file_type = H5T_copy(dataset->shared->type, H5T_COPY_ALL)))
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, FAIL, "unable to copy file datatype")
            } /* end if */

            /* Create selection iterator for memory selection */
            if(0 == (elmt_size = H5T_get_size(mem_type)))
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADSIZE, FAIL, "datatype size invalid")
            if(H5S_select_iter_init(&(fm->mem_iter), mem_space, elmt_size) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator")
            iter_init = TRUE;	/* Selection iteration info has been initialized */

            iter_op.op_type = H5S_SEL_ITER_OP_LIB;
            iter_op.u.lib_op = H5D__chunk_mem_cb;

            /* Spaces aren't the same shape, iterate over the memory selection directly */
            if(H5S_select_iterate(&bogus, file_type, file_space, &iter_op, fm) < 0)
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

        if(H5D__chunk_io_term(fm) < 0)
            HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release chunk mapping")
    } /* end if */

    /* Reset the global dataspace info */
    fm->file_space = NULL;
    fm->mem_space = NULL;

    if(iter_init && H5S_SELECT_ITER_RELEASE(&(fm->mem_iter)) < 0)
        HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator")
    if(file_type && (H5T_close(file_type) < 0))
        HDONE_ERROR(H5E_DATATYPE, H5E_CANTFREE, FAIL, "Can't free temporary datatype")
    if(file_space_normalized) {
        /* (Casting away const OK -QAK) */
        if(H5S_hyper_denormalize_offset((H5S_t *)file_space, old_offset) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_BADSELECT, FAIL, "unable to normalize dataspace by offset")
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_io_init() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_mem_alloc
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
H5D__chunk_mem_alloc(size_t size, const H5O_pline_t *pline)
{
    void *ret_value = NULL;		/* Return value */

    FUNC_ENTER_STATIC_NOERR

    HDassert(size);

    if(pline && pline->nused)
        ret_value = H5MM_malloc(size);
    else
        ret_value = H5FL_BLK_MALLOC(chunk, size);

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D__chunk_mem_alloc() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_mem_xfree
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
H5D__chunk_mem_xfree(void *chk, const H5O_pline_t *pline)
{
    FUNC_ENTER_STATIC_NOERR

    if(chk) {
        if(pline && pline->nused)
            H5MM_xfree(chk);
        else
            chk = H5FL_BLK_FREE(chunk, chk);
    } /* end if */

    FUNC_LEAVE_NOAPI(NULL)
} /* H5D__chunk_mem_xfree() */


/*-------------------------------------------------------------------------
 * Function:    H5D__chunk_mem_realloc
 *
 * Purpose:     Reallocate space for a chunk in memory.  This routine allocates
 *              memory space for non-filtered chunks from a block free list
 *              and uses malloc()/free() for filtered chunks.
 *
 * Return:      Pointer to memory for chunk on success/NULL on failure
 *
 * Programmer:  Neil Fortner
 *              May 3, 2010
 *
 *-------------------------------------------------------------------------
 */
static void *
H5D__chunk_mem_realloc(void *chk, size_t size, const H5O_pline_t *pline)
{
    void *ret_value = NULL;             /* Return value */

    FUNC_ENTER_STATIC_NOERR

    HDassert(size);
    HDassert(pline);

    if(pline->nused > 0)
        ret_value = H5MM_realloc(chk, size);
    else
        ret_value = H5FL_BLK_REALLOC(chunk, chk, size);

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D__chunk_mem_realloc() */


/*--------------------------------------------------------------------------
 NAME
    H5D__free_chunk_info
 PURPOSE
    Internal routine to destroy a chunk info node
 USAGE
    void H5D__free_chunk_info(chunk_info)
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
H5D__free_chunk_info(void *item, void H5_ATTR_UNUSED *key, void H5_ATTR_UNUSED *opdata)
{
    H5D_chunk_info_t *chunk_info = (H5D_chunk_info_t *)item;

    FUNC_ENTER_STATIC_NOERR

    HDassert(chunk_info);

    /* Close the chunk's file dataspace, if it's not shared */
    if(!chunk_info->fspace_shared)
        (void)H5S_close(chunk_info->fspace);
    else
        H5S_select_all(chunk_info->fspace, TRUE);

    /* Close the chunk's memory dataspace, if it's not shared */
    if(!chunk_info->mspace_shared && chunk_info->mspace)
        (void)H5S_close(chunk_info->mspace);

    /* Free the actual chunk info */
    chunk_info = H5FL_FREE(H5D_chunk_info_t, chunk_info);

    FUNC_LEAVE_NOAPI(0)
}   /* H5D__free_chunk_info() */


/*-------------------------------------------------------------------------
 * Function:	H5D__create_chunk_map_single
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
H5D__create_chunk_map_single(H5D_chunk_map_t *fm, const H5D_io_info_t
#ifndef H5_HAVE_PARALLEL
    H5_ATTR_UNUSED
#endif /* H5_HAVE_PARALLEL */
    *io_info)
{
    H5D_chunk_info_t *chunk_info;           /* Chunk information to insert into skip list */
    hsize_t     coords[H5O_LAYOUT_NDIMS];   /* Coordinates of chunk */
    hsize_t     sel_start[H5O_LAYOUT_NDIMS]; /* Offset of low bound of file selection */
    hsize_t     sel_end[H5O_LAYOUT_NDIMS];  /* Offset of high bound of file selection */
    unsigned    u;                          /* Local index variable */
    herr_t	ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_STATIC

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
        chunk_info->scaled[u] = sel_start[u] / fm->layout->u.chunk.dim[u];
        coords[u] = chunk_info->scaled[u] * fm->layout->u.chunk.dim[u];
    } /* end for */
    chunk_info->scaled[fm->f_ndims] = 0;

    /* Calculate the index of this chunk */
    chunk_info->index = H5VM_array_offset_pre(fm->f_ndims, fm->layout->u.chunk.down_chunks, chunk_info->scaled);

    /* Copy selection for file's dataspace into chunk dataspace */
    if(H5S_select_copy(fm->single_space, fm->file_space, FALSE) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "unable to copy file selection")

    /* Move selection back to have correct offset in chunk */
    if(H5S_SELECT_ADJUST_U(fm->single_space, coords) < 0)
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
} /* end H5D__create_chunk_map_single() */


/*-------------------------------------------------------------------------
 * Function:	H5D__create_chunk_file_map_hyper
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
H5D__create_chunk_file_map_hyper(H5D_chunk_map_t *fm, const H5D_io_info_t
#ifndef H5_HAVE_PARALLEL
    H5_ATTR_UNUSED
#endif /* H5_HAVE_PARALLEL */
    *io_info)
{
    hsize_t     sel_start[H5O_LAYOUT_NDIMS];   /* Offset of low bound of file selection */
    hsize_t     sel_end[H5O_LAYOUT_NDIMS];   /* Offset of high bound of file selection */
    hsize_t     sel_points;                 /* Number of elements in file selection */
    hsize_t     start_coords[H5O_LAYOUT_NDIMS];   /* Starting coordinates of selection */
    hsize_t     coords[H5O_LAYOUT_NDIMS];   /* Current coordinates of chunk */
    hsize_t     end[H5O_LAYOUT_NDIMS];      /* Final coordinates of chunk */
    hsize_t     chunk_index;                /* Index of chunk */
    hsize_t     start_scaled[H5S_MAX_RANK]; /* Starting scaled coordinates of selection */
    hsize_t 	scaled[H5S_MAX_RANK];       /* Scaled coordinates for this chunk */
    int         curr_dim;                   /* Current dimension to increment */
    unsigned    u;                          /* Local index variable */
    herr_t	ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(fm->f_ndims>0);

    /* Get number of elements selected in file */
    sel_points = fm->nelmts;

    /* Get bounding box for selection (to reduce the number of chunks to iterate over) */
    if(H5S_SELECT_BOUNDS(fm->file_space, sel_start, sel_end) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't get file selection bound info")

    /* Set initial chunk location & hyperslab size */
    for(u = 0; u < fm->f_ndims; u++) {
        scaled[u] = start_scaled[u] = sel_start[u] / fm->layout->u.chunk.dim[u];
        coords[u] = start_coords[u] = scaled[u] * fm->layout->u.chunk.dim[u];
        end[u] = (coords[u] + fm->chunk_dim[u]) - 1;
    } /* end for */

    /* Calculate the index of this chunk */
    chunk_index = H5VM_array_offset_pre(fm->f_ndims, fm->layout->u.chunk.down_chunks, scaled);

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
            if (NULL==(new_chunk_info = H5FL_MALLOC(H5D_chunk_info_t))) {
                (void)H5S_close(tmp_fchunk);
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate chunk info")
            } /* end if */

            /* Initialize the chunk information */

            /* Set the chunk index */
            new_chunk_info->index=chunk_index;

#ifdef H5_HAVE_PARALLEL
            /* Store chunk selection information, for multi-chunk I/O */
            if(io_info->using_mpi_vfd)
                fm->select_chunk[chunk_index] = new_chunk_info;
#endif /* H5_HAVE_PARALLEL */

            /* Set the file chunk dataspace */
            new_chunk_info->fspace = tmp_fchunk;
            new_chunk_info->fspace_shared = FALSE;

            /* Set the memory chunk dataspace */
            new_chunk_info->mspace=NULL;
            new_chunk_info->mspace_shared = FALSE;

            /* Copy the chunk's scaled coordinates */
	    HDmemcpy(new_chunk_info->scaled, scaled, sizeof(hsize_t) * fm->f_ndims);
            new_chunk_info->scaled[fm->f_ndims] = 0;

            /* Copy the chunk's scaled coordinates */
	    HDmemcpy(new_chunk_info->scaled, scaled, sizeof(hsize_t) * fm->f_ndims);

            /* Insert the new chunk into the skip list */
            if(H5SL_insert(fm->sel_chunks, new_chunk_info, &new_chunk_info->index) < 0) {
                H5D__free_chunk_info(new_chunk_info, NULL, NULL);
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINSERT, FAIL, "can't insert chunk into skip list")
            } /* end if */

            /* Get number of elements selected in chunk */
            if((schunk_points = H5S_GET_SELECT_NPOINTS(tmp_fchunk)) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't get file selection # of elements")
            H5_CHECKED_ASSIGN(new_chunk_info->chunk_points, uint32_t, schunk_points, hssize_t);

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
        scaled[curr_dim]++;

        /* Bring chunk location back into bounds, if necessary */
        if(coords[curr_dim] > sel_end[curr_dim]) {
            do {
                /* Reset current dimension's location to 0 */
                scaled[curr_dim] = start_scaled[curr_dim];
                coords[curr_dim] = start_coords[curr_dim]; /*lint !e771 The start_coords will always be initialized */
                end[curr_dim] = (coords[curr_dim] + fm->chunk_dim[curr_dim]) - 1;

                /* Decrement current dimension */
                curr_dim--;

                /* Increment chunk location in current dimension */
                scaled[curr_dim]++;
                coords[curr_dim] += fm->chunk_dim[curr_dim];
                end[curr_dim] = (coords[curr_dim] + fm->chunk_dim[curr_dim]) - 1;
            } while(coords[curr_dim] > sel_end[curr_dim]);

            /* Re-calculate the index of this chunk */
            chunk_index = H5VM_array_offset_pre(fm->f_ndims, fm->layout->u.chunk.down_chunks, scaled);
        } /* end if */
    } /* end while */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__create_chunk_file_map_hyper() */


/*-------------------------------------------------------------------------
 * Function:	H5D__create_chunk_mem_map_hyper
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
H5D__create_chunk_mem_map_hyper(const H5D_chunk_map_t *fm)
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

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(fm->f_ndims>0);

    /* Check for all I/O going to a single chunk */
    if(H5SL_count(fm->sel_chunks)==1) {
        H5D_chunk_info_t *chunk_info;   /* Pointer to chunk information */

        /* Get the node */
        curr_node=H5SL_first(fm->sel_chunks);

        /* Get pointer to chunk's information */
        chunk_info = (H5D_chunk_info_t *)H5SL_item(curr_node);
        HDassert(chunk_info);

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
        HDassert(fm->m_ndims==fm->f_ndims);
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
            HDassert(chunk_info);

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

            /* Compute the adjustment for this chunk */
            for(u = 0; u < fm->f_ndims; u++) {
                hsize_t coords[H5O_LAYOUT_NDIMS];   /* Current coordinates of chunk */

                /* Compute the chunk coordinates from the scaled coordinates */
                coords[u] = chunk_info->scaled[u] * fm->layout->u.chunk.dim[u];

                /* Compensate for the chunk offset */
                H5_CHECK_OVERFLOW(coords[u], hsize_t, hssize_t);
                chunk_adjust[u] = adjust[u] - (hssize_t)coords[u]; /*lint !e771 The adjust array will always be initialized */
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
} /* end H5D__create_chunk_mem_map_hyper() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_file_cb
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
H5D__chunk_file_cb(void H5_ATTR_UNUSED *elem, const H5T_t H5_ATTR_UNUSED *type, unsigned ndims, const hsize_t *coords, void *_udata)
{
    H5D_chunk_file_iter_ud_t *udata = (H5D_chunk_file_iter_ud_t *)_udata;       /* User data for operation */
    H5D_chunk_map_t      *fm = udata->fm;       /* File<->memory chunk mapping info */
    H5D_chunk_info_t *chunk_info;               /* Chunk information for current chunk */
    hsize_t     coords_in_chunk[H5O_LAYOUT_NDIMS];        /* Coordinates of element in chunk */
    hsize_t     chunk_index;                    /* Chunk index */
    hsize_t     scaled[H5S_MAX_RANK];	        /* Scaled coordinates for this chunk */
    unsigned    u;                              /* Local index variable */
    herr_t	ret_value = SUCCEED;            /* Return value		*/

    FUNC_ENTER_STATIC

    /* Calculate the index of this chunk */
    chunk_index = H5VM_chunk_index_scaled(ndims, coords, fm->layout->u.chunk.dim, fm->layout->u.chunk.down_chunks, scaled);

    /* Find correct chunk in file & memory skip list */
    if(chunk_index==fm->last_index) {
        /* If the chunk index is the same as the last chunk index we used,
         * get the cached info to operate on.
         */
        chunk_info = fm->last_chunk_info;
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

            /* Set the chunk's scaled coordinates */
            HDmemcpy(chunk_info->scaled, scaled, sizeof(hsize_t) * fm->f_ndims);
            chunk_info->scaled[fm->f_ndims] = 0;
            HDmemcpy(chunk_info->scaled, scaled, sizeof(hsize_t) * fm->f_ndims);

            /* Insert the new chunk into the skip list */
            if(H5SL_insert(fm->sel_chunks,chunk_info,&chunk_info->index) < 0) {
                    H5D__free_chunk_info(chunk_info,NULL,NULL);
                HGOTO_ERROR(H5E_DATASPACE,H5E_CANTINSERT,FAIL,"can't insert chunk into skip list")
            } /* end if */
        } /* end if */

#ifdef H5_HAVE_PARALLEL
        /* Store chunk selection information, for collective multi-chunk I/O */
        if(udata->io_info->using_mpi_vfd)
            fm->select_chunk[chunk_index] = chunk_info;
#endif /* H5_HAVE_PARALLEL */

        /* Update the "last chunk seen" information */
        fm->last_index = chunk_index;
        fm->last_chunk_info = chunk_info;
    } /* end else */

    /* Get the offset of the element within the chunk */
    for(u = 0; u < fm->f_ndims; u++)
        coords_in_chunk[u] = coords[u] - (scaled[u] * fm->layout->u.chunk.dim[u]);

    /* Add point to file selection for chunk */
    if(H5S_select_elements(chunk_info->fspace, H5S_SELECT_APPEND, (size_t)1, coords_in_chunk) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSELECT, FAIL, "unable to select element")

    /* Increment the number of elemented selected in chunk */
    chunk_info->chunk_points++;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_file_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_mem_cb
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
static herr_t
H5D__chunk_mem_cb(void H5_ATTR_UNUSED *elem, const H5T_t H5_ATTR_UNUSED *type, unsigned ndims, const hsize_t *coords, void *_fm)
{
    H5D_chunk_map_t      *fm = (H5D_chunk_map_t *)_fm;  /* File<->memory chunk mapping info */
    H5D_chunk_info_t *chunk_info;               /* Chunk information for current chunk */
    hsize_t    coords_in_mem[H5O_LAYOUT_NDIMS];        /* Coordinates of element in memory */
    hsize_t     chunk_index;                    /* Chunk index */
    herr_t	ret_value = SUCCEED;            /* Return value		*/

    FUNC_ENTER_STATIC

    /* Calculate the index of this chunk */
    chunk_index = H5VM_chunk_index(ndims, coords, fm->layout->u.chunk.dim, fm->layout->u.chunk.down_chunks);

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
} /* end H5D__chunk_mem_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_cacheable
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
H5D__chunk_cacheable(const H5D_io_info_t *io_info, haddr_t caddr, hbool_t write_op)
{
    const H5D_t *dataset = io_info->dset; /* Local pointer to dataset info */
    hbool_t has_filters = FALSE;        /* Whether there are filters on the chunk or not */
    htri_t ret_value = FAIL;            /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    HDassert(io_info);
    HDassert(dataset);

    /* Must bring the whole chunk in if there are any filters on the chunk.
     * Make sure to check if filters are on the dataset but disabled for the
     * chunk because it is a partial edge chunk. */
    if(dataset->shared->dcpl_cache.pline.nused > 0) {
        if(dataset->shared->layout.u.chunk.flags
                & H5O_LAYOUT_CHUNK_DONT_FILTER_PARTIAL_BOUND_CHUNKS) {
            has_filters = !H5D__chunk_is_partial_edge_chunk(
                    io_info->dset->shared->ndims,
                    io_info->dset->shared->layout.u.chunk.dim,
                    io_info->store->chunk.scaled,
                    io_info->dset->shared->curr_dims);
        } /* end if */
        else
            has_filters = TRUE;
    } /* end if */

    if(has_filters)
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
                            (fill->fill_time == H5D_FILL_TIME_IFSET &&
                            (fill_status == H5D_FILL_VALUE_USER_DEFINED ||
                             fill_status == H5D_FILL_VALUE_DEFAULT)))
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
} /* end H5D__chunk_cacheable() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_read
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
H5D__chunk_read(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t H5_ATTR_UNUSED nelmts, const H5S_t H5_ATTR_UNUSED *file_space, const H5S_t H5_ATTR_UNUSED *mem_space,
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
    herr_t	ret_value = SUCCEED;	/*return value		*/

    FUNC_ENTER_STATIC

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
    H5_CHECKED_ASSIGN(ctg_store.contig.dset_size, hsize_t, io_info->dset->shared->layout.u.chunk.size, uint32_t);

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
                (fill->fill_time == H5D_FILL_TIME_IFSET &&
                 fill_status != H5D_FILL_VALUE_USER_DEFINED &&
                 fill_status != H5D_FILL_VALUE_DEFAULT))
            skip_missing_chunks = TRUE;
    }

    /* Iterate through nodes in chunk skip list */
    chunk_node = H5D_CHUNK_GET_FIRST_NODE(fm);
    while(chunk_node) {
        H5D_chunk_info_t *chunk_info;   /* Chunk information */
        H5D_chunk_ud_t udata;		/* Chunk index pass-through	*/

        /* Get the actual chunk information from the skip list node */
        chunk_info = H5D_CHUNK_GET_NODE_INFO(fm, chunk_node);

        /* Get the info for the chunk in the file */
        if(H5D__chunk_lookup(io_info->dset, io_info->md_dxpl_id, chunk_info->scaled, &udata) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "error looking up chunk address")

        /* Sanity check */
        HDassert((H5F_addr_defined(udata.chunk_block.offset) && udata.chunk_block.length > 0) || 
                (!H5F_addr_defined(udata.chunk_block.offset) && udata.chunk_block.length == 0));

        /* Check for non-existant chunk & skip it if appropriate */
        if(H5F_addr_defined(udata.chunk_block.offset) || UINT_MAX != udata.idx_hint
                || !skip_missing_chunks) {
            H5D_io_info_t *chk_io_info;     /* Pointer to I/O info object for this chunk */
            void *chunk = NULL;             /* Pointer to locked chunk buffer */
            htri_t cacheable;               /* Whether the chunk is cacheable */

	    /* Set chunk's [scaled] coordinates */
	    io_info->store->chunk.scaled = chunk_info->scaled;

            /* Determine if we should use the chunk cache */
            if((cacheable = H5D__chunk_cacheable(io_info, udata.chunk_block.offset, FALSE)) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't tell if chunk is cacheable")
            if(cacheable) {
                /* Load the chunk into cache and lock it. */

                /* Compute # of bytes accessed in chunk */
                H5_CHECK_OVERFLOW(type_info->src_type_size, /*From:*/ size_t, /*To:*/ uint32_t);
                src_accessed_bytes = chunk_info->chunk_points * (uint32_t)type_info->src_type_size;

                /* Lock the chunk into the cache */
                if(NULL == (chunk = H5D__chunk_lock(io_info, &udata, FALSE, FALSE)))
                    HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "unable to read raw data chunk")

                /* Set up the storage buffer information for this chunk */
                cpt_store.compact.buf = chunk;

                /* Point I/O info at contiguous I/O info for this chunk */
                chk_io_info = &cpt_io_info;
            } /* end if */
            else if(H5F_addr_defined(udata.chunk_block.offset)) {
                /* Set up the storage address information for this chunk */
                ctg_store.contig.dset_addr = udata.chunk_block.offset;

                /* Point I/O info at temporary I/O info for this chunk */
                chk_io_info = &ctg_io_info;
            } /* end else if */
            else {
                /* Point I/O info at "nonexistent" I/O info for this chunk */
                chk_io_info = &nonexistent_io_info;
            } /* end else */

            /* Perform the actual read operation */
            if((io_info->io_ops.single_read)(chk_io_info, type_info,
                    (hsize_t)chunk_info->chunk_points, chunk_info->fspace, chunk_info->mspace) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "chunked read failed")

            /* Release the cache lock on the chunk. */
            if(chunk && H5D__chunk_unlock(io_info, &udata, FALSE, chunk, src_accessed_bytes) < 0)
                HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "unable to unlock raw data chunk")
        } /* end if */

        /* Advance to next chunk in list */
        chunk_node = H5D_CHUNK_GET_NEXT_NODE(fm, chunk_node);
    } /* end while */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D__chunk_read() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_write
 *
 * Purpose:	Writes to a chunked dataset.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *		Thursday, April 10, 2003
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__chunk_write(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
    hsize_t H5_ATTR_UNUSED nelmts, const H5S_t H5_ATTR_UNUSED *file_space, const H5S_t H5_ATTR_UNUSED *mem_space,
    H5D_chunk_map_t *fm)
{
    H5SL_node_t *chunk_node;            /* Current node in chunk skip list */
    H5D_io_info_t ctg_io_info;          /* Contiguous I/O info object */
    H5D_storage_t ctg_store;            /* Chunk storage information as contiguous dataset */
    H5D_io_info_t cpt_io_info;          /* Compact I/O info object */
    H5D_storage_t cpt_store;            /* Chunk storage information as compact dataset */
    hbool_t     cpt_dirty;              /* Temporary placeholder for compact storage "dirty" flag */
    uint32_t    dst_accessed_bytes = 0; /* Total accessed size in a chunk */
    herr_t	ret_value = SUCCEED;	/* Return value		*/

    FUNC_ENTER_STATIC

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
    H5_CHECKED_ASSIGN(ctg_store.contig.dset_size, hsize_t, io_info->dset->shared->layout.u.chunk.size, uint32_t);

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
	H5D_chk_idx_info_t idx_info;    /* Chunked index info */
        H5D_io_info_t *chk_io_info;     /* Pointer to I/O info object for this chunk */
        void *chunk;                    /* Pointer to locked chunk buffer */
        H5D_chunk_ud_t udata;		/* Index pass-through	*/
        htri_t cacheable;               /* Whether the chunk is cacheable */
        hbool_t need_insert = FALSE;    /* Whether the chunk needs to be inserted into the index */

        /* Get the actual chunk information from the skip list node */
        chunk_info = H5D_CHUNK_GET_NODE_INFO(fm, chunk_node);

        /* Look up the chunk */
        if(H5D__chunk_lookup(io_info->dset, io_info->md_dxpl_id, chunk_info->scaled, &udata) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "error looking up chunk address")

        /* Sanity check */
        HDassert((H5F_addr_defined(udata.chunk_block.offset) && udata.chunk_block.length > 0) || 
                (!H5F_addr_defined(udata.chunk_block.offset) && udata.chunk_block.length == 0));

	/* Set chunk's [scaled] coordinates */
	io_info->store->chunk.scaled = chunk_info->scaled;

        /* Determine if we should use the chunk cache */
        if((cacheable = H5D__chunk_cacheable(io_info, udata.chunk_block.offset, TRUE)) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't tell if chunk is cacheable")
        if(cacheable) {
            /* Load the chunk into cache.  But if the whole chunk is written,
             * simply allocate space instead of load the chunk. */
            hbool_t entire_chunk = TRUE;       /* Whether whole chunk is selected */

            /* Compute # of bytes accessed in chunk */
            H5_CHECK_OVERFLOW(type_info->dst_type_size, /*From:*/ size_t, /*To:*/ uint32_t);
            dst_accessed_bytes = chunk_info->chunk_points * (uint32_t)type_info->dst_type_size;

            /* Determine if we will access all the data in the chunk */
            if(dst_accessed_bytes != ctg_store.contig.dset_size ||
                    (chunk_info->chunk_points * type_info->src_type_size) != ctg_store.contig.dset_size ||
		    fm->fsel_type == H5S_SEL_POINTS)
                entire_chunk = FALSE;

            /* Lock the chunk into the cache */
            if(NULL == (chunk = H5D__chunk_lock(io_info, &udata, entire_chunk, FALSE)))
                HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "unable to read raw data chunk")

            /* Set up the storage buffer information for this chunk */
            cpt_store.compact.buf = chunk;

            /* Point I/O info at main I/O info for this chunk */
            chk_io_info = &cpt_io_info;
        } /* end if */
        else {
            /* If the chunk hasn't been allocated on disk, do so now. */
            if(!H5F_addr_defined(udata.chunk_block.offset)) {
                /* Compose chunked index info struct */
                idx_info.f = io_info->dset->oloc.file;
                idx_info.dxpl_id = io_info->md_dxpl_id;
                idx_info.pline = &(io_info->dset->shared->dcpl_cache.pline);
                idx_info.layout = &(io_info->dset->shared->layout.u.chunk);
                idx_info.storage = &(io_info->dset->shared->layout.storage.u.chunk);

                /* Set up the size of chunk for user data */
                udata.chunk_block.length = io_info->dset->shared->layout.u.chunk.size;

                /* Allocate the chunk */
		if(H5D__chunk_file_alloc(&idx_info, NULL, &udata.chunk_block, &need_insert, chunk_info->scaled) < 0)
		    HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, FAIL, "unable to insert/resize chunk on chunk level")

                /* Make sure the address of the chunk is returned. */
                if(!H5F_addr_defined(udata.chunk_block.offset))
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "chunk address isn't defined")

                /* Cache the new chunk information */
                H5D__chunk_cinfo_cache_update(&io_info->dset->shared->cache.chunk.last, &udata);
            } /* end if */

            /* Set up the storage address information for this chunk */
            ctg_store.contig.dset_addr = udata.chunk_block.offset;

            /* No chunk cached */
            chunk = NULL;

            /* Point I/O info at temporary I/O info for this chunk */
            chk_io_info = &ctg_io_info;
        } /* end else */

        /* Perform the actual write operation */
        if((io_info->io_ops.single_write)(chk_io_info, type_info,
                (hsize_t)chunk_info->chunk_points, chunk_info->fspace, chunk_info->mspace) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "chunked write failed")

	/* Release the cache lock on the chunk, or insert chunk into index. */
	if(chunk) {
	    if(H5D__chunk_unlock(io_info, &udata, TRUE, chunk, dst_accessed_bytes) < 0)
		HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "unable to unlock raw data chunk")
	} /* end if */
	else {
            if(need_insert && io_info->dset->shared->layout.storage.u.chunk.ops->insert)
                if((io_info->dset->shared->layout.storage.u.chunk.ops->insert)(&idx_info, &udata, NULL) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, FAIL, "unable to insert chunk addr into index")
	} /* end else */

        /* Advance to next chunk in list */
        chunk_node = H5D_CHUNK_GET_NEXT_NODE(fm, chunk_node);
    } /* end while */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D__chunk_write() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_flush
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
H5D__chunk_flush(H5D_t *dset, hid_t dxpl_id)
{
    H5D_dxpl_cache_t _dxpl_cache;       /* Data transfer property cache buffer */
    H5D_dxpl_cache_t *dxpl_cache = &_dxpl_cache;   /* Data transfer property cache */
    H5D_rdcc_t *rdcc = &(dset->shared->cache.chunk);
    H5D_rdcc_ent_t	*ent, *next;
    unsigned		nerrors = 0;    /* Count of any errors encountered when flushing chunks */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(dset);

    /* Fill the DXPL cache values for later use */
    if(H5D__get_dxpl_cache(dxpl_id, &dxpl_cache) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't fill dxpl cache")

    /* Loop over all entries in the chunk cache */
    for(ent = rdcc->head; ent; ent = next) {
	next = ent->next;
        if(H5D__chunk_flush_entry(dset, dxpl_id, dxpl_cache, ent, FALSE) < 0)
            nerrors++;
    } /* end for */
    if(nerrors)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTFLUSH, FAIL, "unable to flush one or more raw data chunks")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_flush() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_io_term
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
H5D__chunk_io_term(const H5D_chunk_map_t *fm)
{
    herr_t	ret_value = SUCCEED;	/*return value		*/

    FUNC_ENTER_STATIC

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
            if(H5SL_free(fm->sel_chunks, H5D__free_chunk_info, NULL) < 0)
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
} /* end H5D__chunk_io_term() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_dest
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
static herr_t
H5D__chunk_dest(H5D_t *dset, hid_t dxpl_id)
{
    H5D_chk_idx_info_t idx_info;        /* Chunked index info */
    H5D_dxpl_cache_t _dxpl_cache;       /* Data transfer property cache buffer */
    H5D_dxpl_cache_t *dxpl_cache = &_dxpl_cache;   /* Data transfer property cache */
    H5D_rdcc_t	*rdcc = &(dset->shared->cache.chunk);   /* Dataset's chunk cache */
    H5D_rdcc_ent_t	*ent = NULL, *next = NULL;      /* Pointer to current & next cache entries */
    int		nerrors = 0;            /* Accumulated count of errors */
    H5O_storage_chunk_t *sc = &(dset->shared->layout.storage.u.chunk);
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_STATIC_TAG(dxpl_id, dset->oloc.addr, FAIL)

    /* Sanity checks */
    HDassert(dset);
    H5D_CHUNK_STORAGE_INDEX_CHK(sc);

    /* Fill the DXPL cache values for later use */
    if(H5D__get_dxpl_cache(dxpl_id, &dxpl_cache) < 0)
        nerrors++;

    /* Flush all the cached chunks */
    for(ent = rdcc->head; ent; ent = next) {
	next = ent->next;
	if(H5D__chunk_cache_evict(dset, dxpl_id, dxpl_cache, ent, TRUE) < 0)
	    nerrors++;
    } /* end for */
    
    /* Continue even if there are failures. */
    if(nerrors)
	HDONE_ERROR(H5E_IO, H5E_CANTFLUSH, FAIL, "unable to flush one or more raw data chunks")

    /* Release cache structures */
    if(rdcc->slot)
        rdcc->slot = H5FL_SEQ_FREE(H5D_rdcc_ent_ptr_t, rdcc->slot);
    HDmemset(rdcc, 0, sizeof(H5D_rdcc_t));

    /* Compose chunked index info struct */
    idx_info.f = dset->oloc.file;
    idx_info.dxpl_id = dxpl_id;
    idx_info.pline = &dset->shared->dcpl_cache.pline;
    idx_info.layout = &dset->shared->layout.u.chunk;
    idx_info.storage = &dset->shared->layout.storage.u.chunk;

    /* Free any index structures */
    if(dset->shared->layout.storage.u.chunk.ops->dest &&
            (dset->shared->layout.storage.u.chunk.ops->dest)(&idx_info) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "unable to release chunk index info")

done:
    FUNC_LEAVE_NOAPI_TAG(ret_value, FAIL)
} /* end H5D__chunk_dest() */


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

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(storage);
    HDassert(storage->ops);
    H5D_CHUNK_STORAGE_INDEX_CHK(storage);

    /* Reset index structures */
    if((storage->ops->reset)(storage, reset_addr) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "unable to reset chunk index info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_chunk_idx_reset() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_cinfo_cache_reset
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
static herr_t
H5D__chunk_cinfo_cache_reset(H5D_chunk_cached_t *last)
{
    FUNC_ENTER_PACKAGE_NOERR

    /* Sanity check */
    HDassert(last);

    /* Indicate that the cached info is not valid */
    last->valid = FALSE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5D__chunk_cinfo_cache_reset() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_cinfo_cache_update
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
H5D__chunk_cinfo_cache_update(H5D_chunk_cached_t *last, const H5D_chunk_ud_t *udata)
{
    FUNC_ENTER_STATIC_NOERR

    /* Sanity check */
    HDassert(last);
    HDassert(udata);
    HDassert(udata->common.layout);
    HDassert(udata->common.scaled);

    /* Stored the information to cache */
    HDmemcpy(last->scaled, udata->common.scaled, sizeof(hsize_t) * udata->common.layout->ndims);
    last->addr = udata->chunk_block.offset;
    H5_CHECKED_ASSIGN(last->nbytes, uint32_t, udata->chunk_block.length, hsize_t);
    last->chunk_idx = udata->chunk_idx;
    last->filter_mask = udata->filter_mask;

    /* Indicate that the cached info is valid */
    last->valid = TRUE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5D__chunk_cinfo_cache_update() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_cinfo_cache_found
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
H5D__chunk_cinfo_cache_found(const H5D_chunk_cached_t *last, H5D_chunk_ud_t *udata)
{
    hbool_t ret_value = FALSE;          /* Return value */

    FUNC_ENTER_STATIC_NOERR

    /* Sanity check */
    HDassert(last);
    HDassert(udata);
    HDassert(udata->common.layout);
    HDassert(udata->common.scaled);

    /* Check if the cached information is what is desired */
    if(last->valid) {
        unsigned    u;                      /* Local index variable */

        /* Check that the scaled offset is the same */
        for(u = 0; u < udata->common.layout->ndims; u++)
            if(last->scaled[u] != udata->common.scaled[u])
                HGOTO_DONE(FALSE)

        /* Retrieve the information from the cache */
        udata->chunk_block.offset = last->addr;
        udata->chunk_block.length = last->nbytes;
	udata->chunk_idx = last->chunk_idx;
        udata->filter_mask = last->filter_mask;

        /* Indicate that the data was found */
        HGOTO_DONE(TRUE)
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D__chunk_cinfo_cache_found() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_create
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
H5D__chunk_create(const H5D_t *dset /*in,out*/, hid_t dxpl_id)
{
    H5D_chk_idx_info_t idx_info;        /* Chunked index info */
    H5O_storage_chunk_t *sc = &(dset->shared->layout.storage.u.chunk);
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(dset);
    HDassert(H5D_CHUNKED == dset->shared->layout.type);
    HDassert(dset->shared->layout.u.chunk.ndims > 0 && dset->shared->layout.u.chunk.ndims <= H5O_LAYOUT_NDIMS);
    H5D_CHUNK_STORAGE_INDEX_CHK(sc);

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
} /* end H5D__chunk_create() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_hash_val
 *
 * Purpose:	To calculate an index based on the dataset's scaled coordinates and
 *		sizes of the faster dimensions.
 *
 * Return:	Hash value index
 *
 * Programmer:	Vailin Choi; Nov 2014
 *
 *-------------------------------------------------------------------------
 */
static unsigned
H5D__chunk_hash_val(const H5D_shared_t *shared, const hsize_t *scaled)
{
    hsize_t val;        /* Intermediate value */
    unsigned ndims = shared->ndims;      /* Rank of dataset */
    unsigned ret = 0;   /* Value to return */

    FUNC_ENTER_STATIC_NOERR

    /* Sanity check */
    HDassert(shared);
    HDassert(scaled);

    /* If the fastest changing dimension doesn't have enough entropy, use
     *  other dimensions too
     */
    if(ndims > 1 && shared->cache.chunk.scaled_dims[ndims - 1] <= shared->cache.chunk.nslots) {
        unsigned u;          /* Local index variable */

        val = scaled[0];
        for(u = 1; u < ndims; u++) {
            val <<= shared->cache.chunk.scaled_encode_bits[u];
            val ^= scaled[u];
        } /* end for */
    } /* end if */
    else
        val = scaled[ndims - 1];

    /* Modulo value against the number of array slots */
    ret = (unsigned)(val % shared->cache.chunk.nslots);

    FUNC_LEAVE_NOAPI(ret)
} /* H5D__chunk_hash_val() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_lookup
 *
 * Purpose:	Loops up a chunk in cache and on disk, and retrieves
 *              information about that chunk.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Albert Cheng
 *              June 27, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__chunk_lookup(const H5D_t *dset, hid_t dxpl_id, const hsize_t *scaled,
    H5D_chunk_ud_t *udata)
{
    H5D_rdcc_ent_t  *ent = NULL;        /* Cache entry */
    H5O_storage_chunk_t *sc = &(dset->shared->layout.storage.u.chunk);
    unsigned idx;                       /* Index of chunk in cache, if present */
    hbool_t found = FALSE;              /* In cache? */
    herr_t ret_value = SUCCEED;	        /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity checks */
    HDassert(dset);
    HDassert(dset->shared->layout.u.chunk.ndims > 0);
    H5D_CHUNK_STORAGE_INDEX_CHK(sc);
    HDassert(scaled);
    HDassert(udata);

    /* Initialize the query information about the chunk we are looking for */
    udata->common.layout = &(dset->shared->layout.u.chunk);
    udata->common.storage = &(dset->shared->layout.storage.u.chunk);
    udata->common.scaled = scaled;

    /* Reset information about the chunk we are looking for */
    udata->chunk_block.offset = HADDR_UNDEF;
    udata->chunk_block.length = 0;
    udata->filter_mask = 0;
    udata->new_unfilt_chunk = FALSE;

    /* Check for chunk in cache */
    if(dset->shared->cache.chunk.nslots > 0) {
        /* Determine the chunk's location in the hash table */
	idx = H5D__chunk_hash_val(dset->shared, scaled);

        /* Get the chunk cache entry for that location */
        ent = dset->shared->cache.chunk.slot[idx];
        if(ent) {
            unsigned u;                  /* Counter */

            /* Speculatively set the 'found' flag */
            found = TRUE;

            /* Verify that the cache entry is the correct chunk */
            for(u = 0; u < dset->shared->ndims; u++)
                if(scaled[u] != ent->scaled[u]) {
                    found = FALSE;
                    break;
                } /* end if */
        } /* end if */
    } /* end if */

    /* Retrieve chunk addr */
    if(found) {
        udata->idx_hint = idx;
        udata->chunk_block.offset = ent->chunk_block.offset;
        udata->chunk_block.length = ent->chunk_block.length;;
	udata->chunk_idx = ent->chunk_idx;
    } /* end if */
    else {
        /* Invalidate idx_hint, to signal that the chunk is not in cache */
        udata->idx_hint = UINT_MAX;

        /* Check for cached information */
        if(!H5D__chunk_cinfo_cache_found(&dset->shared->cache.chunk.last, udata)) {
            H5D_chk_idx_info_t idx_info;        /* Chunked index info */
#ifdef H5_HAVE_PARALLEL
            H5P_coll_md_read_flag_t temp_cmr;   /* Temp value to hold the coll metadata read setting */
#endif /* H5_HAVE_PARALLEL */

            /* Compose chunked index info struct */
            idx_info.f = dset->oloc.file;
            idx_info.dxpl_id = dxpl_id;
            idx_info.pline = &dset->shared->dcpl_cache.pline;
            idx_info.layout = &dset->shared->layout.u.chunk;
            idx_info.storage = &dset->shared->layout.storage.u.chunk;

#ifdef H5_HAVE_PARALLEL
            if(H5F_HAS_FEATURE(idx_info.f, H5FD_FEAT_HAS_MPI)) {
                /* disable collective metadata read for chunk indexes
                   as it is highly unlikely that users would read the
                   same chunks from all processes. MSC - might turn on
                   for root node? */
                temp_cmr = H5F_COLL_MD_READ(idx_info.f);
                H5F_set_coll_md_read(idx_info.f, H5P_FORCE_FALSE);
            } /* end if */
#endif /* H5_HAVE_PARALLEL */

            /* Go get the chunk information */
            if((dset->shared->layout.storage.u.chunk.ops->get_addr)(&idx_info, udata) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't query chunk address")

#ifdef H5_HAVE_PARALLEL
            if(H5F_HAS_FEATURE(idx_info.f, H5FD_FEAT_HAS_MPI))
                H5F_set_coll_md_read(idx_info.f, temp_cmr);
#endif /* H5_HAVE_PARALLEL */

            /* Cache the information retrieved */
            H5D__chunk_cinfo_cache_update(&dset->shared->cache.chunk.last, udata);
        } /* end if */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D__chunk_lookup() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_flush_entry
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
H5D__chunk_flush_entry(const H5D_t *dset, hid_t dxpl_id, const H5D_dxpl_cache_t *dxpl_cache,
    H5D_rdcc_ent_t *ent, hbool_t reset)
{
    void	*buf = NULL;	        /* Temporary buffer		*/
    hbool_t	point_of_no_return = FALSE;
    H5O_storage_chunk_t *sc = &(dset->shared->layout.storage.u.chunk);
    herr_t	ret_value = SUCCEED;	/* Return value			*/

    FUNC_ENTER_STATIC_TAG(dxpl_id, dset->oloc.addr, FAIL)

    HDassert(dset);
    HDassert(dset->shared);
    H5D_CHUNK_STORAGE_INDEX_CHK(sc);
    HDassert(dxpl_cache);
    HDassert(ent);
    HDassert(!ent->locked);

    buf = ent->chunk;
    if(ent->dirty) {
	H5D_chk_idx_info_t idx_info;    /* Chunked index info */
        H5D_chunk_ud_t 	udata;		/* pass through B-tree		*/
        hbool_t must_alloc = FALSE;     /* Whether the chunk must be allocated */
	hbool_t need_insert = FALSE;    /* Whether the chunk needs to be inserted into the index */

        /* Set up user data for index callbacks */
        udata.common.layout = &dset->shared->layout.u.chunk;
        udata.common.storage = &dset->shared->layout.storage.u.chunk;
	udata.common.scaled = ent->scaled;
        udata.chunk_block.offset = ent->chunk_block.offset;
        udata.chunk_block.length = dset->shared->layout.u.chunk.size;
        udata.filter_mask = 0;
	udata.chunk_idx = ent->chunk_idx;

        /* Should the chunk be filtered before writing it to disk? */
        if(dset->shared->dcpl_cache.pline.nused
                && !(ent->edge_chunk_state & H5D_RDCC_DISABLE_FILTERS)) {
            size_t alloc = udata.chunk_block.length;        /* Bytes allocated for BUF	*/
            size_t nbytes;                      /* Chunk size (in bytes) */

            if(!reset) {
                /*
                 * Copy the chunk to a new buffer before running it through
                 * the pipeline because we'll want to save the original buffer
                 * for later.
                 */
                if(NULL == (buf = H5MM_malloc(alloc)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for pipeline")
                HDmemcpy(buf, ent->chunk, alloc);
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
            H5_CHECKED_ASSIGN(nbytes, size_t, udata.chunk_block.length, hsize_t);
            if(H5Z_pipeline(&(dset->shared->dcpl_cache.pline), 0, &(udata.filter_mask), dxpl_cache->err_detect,
                     dxpl_cache->filter_cb, &nbytes, &alloc, &buf) < 0)
                HGOTO_ERROR(H5E_PLINE, H5E_CANTFILTER, FAIL, "output pipeline failed")
#if H5_SIZEOF_SIZE_T > 4
            /* Check for the chunk expanding too much to encode in a 32-bit value */
            if(nbytes > ((size_t)0xffffffff))
                HGOTO_ERROR(H5E_DATASET, H5E_BADRANGE, FAIL, "chunk too large for 32-bit length")
#endif /* H5_SIZEOF_SIZE_T > 4 */
            H5_CHECKED_ASSIGN(udata.chunk_block.length, hsize_t, nbytes, size_t);

            /* Indicate that the chunk must be allocated */
            must_alloc = TRUE;
        } /* end if */
        else if(!H5F_addr_defined(udata.chunk_block.offset)) {
            /* Indicate that the chunk must be allocated */
            must_alloc = TRUE;

            /* This flag could be set for this chunk, just remove and ignore it
             */
            ent->edge_chunk_state &= ~H5D_RDCC_NEWLY_DISABLED_FILTERS;
        } /* end else */
        else if(ent->edge_chunk_state & H5D_RDCC_NEWLY_DISABLED_FILTERS) {
            /* Chunk on disk is still filtered, must insert to allocate correct
             * size */
            must_alloc = TRUE;

            /* Set the disable filters field back to the standard disable
             * filters setting, as it no longer needs to be inserted with every
             * flush */
            ent->edge_chunk_state &= ~H5D_RDCC_NEWLY_DISABLED_FILTERS;
        } /* end else */

        HDassert(!(ent->edge_chunk_state & H5D_RDCC_NEWLY_DISABLED_FILTERS));

        /* Check if the chunk needs to be allocated (it also could exist already
         *      and the chunk alloc operation could resize it)
         */
        if(must_alloc) {
            /* Compose chunked index info struct */
            idx_info.f = dset->oloc.file;
            idx_info.dxpl_id = dxpl_id;
            idx_info.pline = &dset->shared->dcpl_cache.pline;
            idx_info.layout = &dset->shared->layout.u.chunk;
            idx_info.storage = &dset->shared->layout.storage.u.chunk;

            /* Create the chunk it if it doesn't exist, or reallocate the chunk
             *  if its size changed.
             */
	    if(H5D__chunk_file_alloc(&idx_info, &(ent->chunk_block), &udata.chunk_block, &need_insert, ent->scaled) < 0)
		HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, FAIL, "unable to insert/resize chunk on chunk level")

            /* Update the chunk entry's info, in case it was allocated or relocated */
            ent->chunk_block.offset = udata.chunk_block.offset;
            ent->chunk_block.length = udata.chunk_block.length;
        } /* end if */

        /* Write the data to the file */
        HDassert(H5F_addr_defined(udata.chunk_block.offset));
        H5_CHECK_OVERFLOW(udata.chunk_block.length, hsize_t, size_t);
        if(H5F_block_write(dset->oloc.file, H5FD_MEM_DRAW, udata.chunk_block.offset, (size_t)udata.chunk_block.length, H5AC_rawdata_dxpl_id, buf) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to write raw data to file")

        /* Insert the chunk record into the index */
	if(need_insert && dset->shared->layout.storage.u.chunk.ops->insert)
            if((dset->shared->layout.storage.u.chunk.ops->insert)(&idx_info, &udata, dset) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, FAIL, "unable to insert chunk addr into index")

        /* Cache the chunk's info, in case it's accessed again shortly */
        H5D__chunk_cinfo_cache_update(&dset->shared->cache.chunk.last, &udata);

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
            ent->chunk = (uint8_t *)H5D__chunk_mem_xfree(ent->chunk,
                    ((ent->edge_chunk_state & H5D_RDCC_DISABLE_FILTERS) ? NULL
                    : &(dset->shared->dcpl_cache.pline)));
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
    if(ret_value < 0 && point_of_no_return)
        if(ent->chunk)
            ent->chunk = (uint8_t *)H5D__chunk_mem_xfree(ent->chunk,
                    ((ent->edge_chunk_state & H5D_RDCC_DISABLE_FILTERS) ? NULL
                    : &(dset->shared->dcpl_cache.pline)));

    FUNC_LEAVE_NOAPI_TAG(ret_value, FAIL)
} /* end H5D__chunk_flush_entry() */


/*-------------------------------------------------------------------------
 * Function:    H5D__chunk_cache_evict
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
H5D__chunk_cache_evict(const H5D_t *dset, hid_t dxpl_id, const H5D_dxpl_cache_t *dxpl_cache,
    H5D_rdcc_ent_t *ent, hbool_t flush)
{
    H5D_rdcc_t *rdcc = &(dset->shared->cache.chunk);
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_STATIC

    HDassert(dset);
    HDassert(dxpl_cache);
    HDassert(ent);
    HDassert(!ent->locked);
    HDassert(ent->idx < rdcc->nslots);

    if(flush) {
	/* Flush */
	if(H5D__chunk_flush_entry(dset, dxpl_id, dxpl_cache, ent, TRUE) < 0)
	    HDONE_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "cannot flush indexed storage buffer")
    } /* end if */
    else {
        /* Don't flush, just free chunk */
	if(ent->chunk != NULL)
	    ent->chunk = (uint8_t *)H5D__chunk_mem_xfree(ent->chunk,
                    ((ent->edge_chunk_state & H5D_RDCC_DISABLE_FILTERS) ? NULL
                    : &(dset->shared->dcpl_cache.pline)));
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

    /* Unlink from temporary list */
    if(ent->tmp_prev) {
        HDassert(rdcc->tmp_head->tmp_next);
        ent->tmp_prev->tmp_next = ent->tmp_next;
        if(ent->tmp_next) {
            ent->tmp_next->tmp_prev = ent->tmp_prev;
            ent->tmp_next = NULL;
        } /* end if */
        ent->tmp_prev = NULL;
    } /* end if */
    else
        /* Only clear hash table slot if the chunk was not on the temporary list
         */
        rdcc->slot[ent->idx] = NULL;

    /* Remove from cache */
    HDassert(rdcc->slot[ent->idx] != ent);
    ent->idx = UINT_MAX;
    rdcc->nbytes_used -= dset->shared->layout.u.chunk.size;
    --rdcc->nused;

    /* Free */
    ent = H5FL_FREE(H5D_rdcc_ent_t, ent);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_cache_evict() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_cache_prune
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
H5D__chunk_cache_prune(const H5D_t *dset, hid_t dxpl_id,
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

    FUNC_ENTER_STATIC

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
		if(H5D__chunk_cache_evict(dset, dxpl_id, dxpl_cache, cur, TRUE) < 0)
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
} /* end H5D__chunk_cache_prune() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_lock
 *
 * Purpose:	Return a pointer to a dataset chunk.  The pointer points
 *		directly into the chunk cache and should not be freed
 *		by the caller but will be valid until it is unlocked.  The
 *		input value IDX_HINT is used to speed up cache lookups and
 *		it's output value should be given to H5D__chunk_unlock().
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
static void *
H5D__chunk_lock(const H5D_io_info_t *io_info, H5D_chunk_ud_t *udata,
    hbool_t relax, hbool_t prev_unfilt_chunk)
{
    const H5D_t         *dset = io_info->dset;  /* Local pointer to the dataset info */
    const H5O_pline_t   *pline = &(dset->shared->dcpl_cache.pline); /* I/O pipeline info - always equal to the pline passed to H5D__chunk_mem_alloc */
    const H5O_pline_t   *old_pline = pline;     /* Old pipeline, i.e. pipeline used to read the chunk */
    const H5O_layout_t  *layout = &(dset->shared->layout); /* Dataset layout */
    const H5O_fill_t    *fill = &(dset->shared->dcpl_cache.fill); /* Fill value info */
    H5D_fill_buf_info_t fb_info;                /* Dataset's fill buffer info */
    hbool_t             fb_info_init = FALSE;   /* Whether the fill value buffer has been initialized */
    H5D_rdcc_t		*rdcc = &(dset->shared->cache.chunk); /*raw data chunk cache*/
    H5D_rdcc_ent_t	*ent;		        /*cache entry		*/
    size_t		chunk_size;		/*size of a chunk	*/
    hbool_t             disable_filters = FALSE; /* Whether to disable filters (when adding to cache) */
    void		*chunk = NULL;		/*the file chunk	*/
    void		*ret_value = NULL;	/* Return value         */

    FUNC_ENTER_STATIC

    HDassert(io_info);
    HDassert(io_info->dxpl_cache);
    HDassert(io_info->store);
    HDassert(udata);
    HDassert(dset);
    HDassert(TRUE == H5P_isa_class(io_info->md_dxpl_id, H5P_DATASET_XFER));
    HDassert(TRUE == H5P_isa_class(io_info->raw_dxpl_id, H5P_DATASET_XFER));
    HDassert(!(udata->new_unfilt_chunk && prev_unfilt_chunk));
    HDassert(!rdcc->tmp_head);

    /* Get the chunk's size */
    HDassert(layout->u.chunk.size > 0);
    H5_CHECKED_ASSIGN(chunk_size, size_t, layout->u.chunk.size, uint32_t);

    /* Check if the chunk is in the cache */
    if(UINT_MAX != udata->idx_hint) {
        /* Sanity check */
        HDassert(udata->idx_hint < rdcc->nslots);
        HDassert(rdcc->slot[udata->idx_hint]);

        /* Get the entry */
        ent = rdcc->slot[udata->idx_hint];

#ifndef NDEBUG
{
        unsigned		u;			/*counters		*/

        /* Make sure this is the right chunk */
        for(u = 0; u < layout->u.chunk.ndims - 1; u++)
            HDassert(io_info->store->chunk.scaled[u] == ent->scaled[u]);
}
#endif /* NDEBUG */

        /*
         * Already in the cache.  Count a hit.
         */
        rdcc->stats.nhits++;

        /* Make adjustments if the edge chunk status changed recently */
        if(pline->nused) {
            /* If the chunk recently became an unfiltered partial edge chunk
             * while in cache, we must make some changes to the entry */
            if(udata->new_unfilt_chunk) {
                /* If this flag is set then partial chunk filters must be
                 * disabled, and the chunk must not have previously been a
                 * partial chunk (with disabled filters) */
                HDassert(layout->u.chunk.flags
                    & H5O_LAYOUT_CHUNK_DONT_FILTER_PARTIAL_BOUND_CHUNKS);
                HDassert(!(ent->edge_chunk_state & H5D_RDCC_DISABLE_FILTERS));
                HDassert(old_pline->nused);

                /* Disable filters.  Set pline to NULL instead of just the
                 * default pipeline to make a quick failure more likely if the
                 * code is changed in an inappropriate/incomplete way. */
                pline = NULL;

                /* Reallocate the chunk so H5D__chunk_mem_xfree doesn't get confused
                 */
                if(NULL == (chunk = H5D__chunk_mem_alloc(chunk_size, pline)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for raw data chunk")
                HDmemcpy(chunk, ent->chunk, chunk_size);
                ent->chunk = (uint8_t *)H5D__chunk_mem_xfree(ent->chunk, old_pline);
                ent->chunk = (uint8_t *)chunk;
                chunk = NULL;

                /* Mark the chunk as having filters disabled as well as "newly
                 * disabled" so it is inserted on flush */
                ent->edge_chunk_state |= H5D_RDCC_DISABLE_FILTERS;
                ent->edge_chunk_state |= H5D_RDCC_NEWLY_DISABLED_FILTERS;
            } /* end if */
            else if(prev_unfilt_chunk) {
                /* If this flag is set then partial chunk filters must be
                 * disabled, and the chunk must have previously been a partial
                 * chunk (with disabled filters) */
                HDassert(layout->u.chunk.flags
                    & H5O_LAYOUT_CHUNK_DONT_FILTER_PARTIAL_BOUND_CHUNKS);
                HDassert((ent->edge_chunk_state & H5D_RDCC_DISABLE_FILTERS));
                HDassert(pline->nused);

                /* Mark the old pipeline as having been disabled */
                old_pline = NULL;

                /* Reallocate the chunk so H5D__chunk_mem_xfree doesn't get confused
                 */
                if(NULL == (chunk = H5D__chunk_mem_alloc(chunk_size, pline)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for raw data chunk")
                HDmemcpy(chunk, ent->chunk, chunk_size);

                ent->chunk = (uint8_t *)H5D__chunk_mem_xfree(ent->chunk, old_pline);
                ent->chunk = (uint8_t *)chunk;
                chunk = NULL;

                /* Mark the chunk as having filters enabled */
                ent->edge_chunk_state &= ~(H5D_RDCC_DISABLE_FILTERS
                        | H5D_RDCC_NEWLY_DISABLED_FILTERS);
            } /* end else */
        } /* end if */

        /*
         * If the chunk is not at the beginning of the cache; move it backward
         * by one slot.  This is how we implement the LRU preemption
         * algorithm.
         */
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
    } /* end if */
    else {
        haddr_t             chunk_addr;         /* Address of chunk on disk */
        hsize_t             chunk_alloc;        /* Length of chunk on disk */

        /* Save the chunk info so the cache stays consistent */
        chunk_addr = udata->chunk_block.offset;
        chunk_alloc = udata->chunk_block.length;

        /* Check if we should disable filters on this chunk */
        if(pline->nused) {
            if(udata->new_unfilt_chunk) {
                HDassert(layout->u.chunk.flags
                        & H5O_LAYOUT_CHUNK_DONT_FILTER_PARTIAL_BOUND_CHUNKS);

                /* Disable the filters for writing */
                disable_filters = TRUE;
                pline = NULL;
            } /* end if */
            else if(prev_unfilt_chunk) {
                HDassert(layout->u.chunk.flags
                        & H5O_LAYOUT_CHUNK_DONT_FILTER_PARTIAL_BOUND_CHUNKS);

                /* Mark the filters as having been previously disabled (for the
                 * chunk as currently on disk) - disable the filters for reading
                 */
                old_pline = NULL;
            } /* end if */
            else if(layout->u.chunk.flags
                    & H5O_LAYOUT_CHUNK_DONT_FILTER_PARTIAL_BOUND_CHUNKS) {
                /* Check if this is an edge chunk */
                if(H5D__chunk_is_partial_edge_chunk(io_info->dset->shared->ndims,
                        layout->u.chunk.dim, io_info->store->chunk.scaled,
                        io_info->dset->shared->curr_dims)) {
                    /* Disable the filters for both writing and reading */
                    disable_filters = TRUE;
                    old_pline = NULL;
                    pline = NULL;
                } /* end if */
            } /* end if */
        } /* end if */

        if(relax) {
            /*
             * Not in the cache, but we're about to overwrite the whole thing
             * anyway, so just allocate a buffer for it but don't initialize that
             * buffer with the file contents. Count this as a hit instead of a
             * miss because we saved ourselves lots of work.
             */
            rdcc->stats.nhits++;

            if(NULL == (chunk = H5D__chunk_mem_alloc(chunk_size, pline)))
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

            /* Check if the chunk exists on disk */
            if(H5F_addr_defined(chunk_addr)) {
                size_t my_chunk_alloc = chunk_alloc;	/* Allocated buffer size */
                size_t buf_alloc = chunk_alloc;	        /* [Re-]allocated buffer size */

                /* Chunk size on disk isn't [likely] the same size as the final chunk
                 * size in memory, so allocate memory big enough. */
                if(NULL == (chunk = H5D__chunk_mem_alloc(my_chunk_alloc, (udata->new_unfilt_chunk ? old_pline : pline))))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for raw data chunk")
                if(H5F_block_read(dset->oloc.file, H5FD_MEM_DRAW, chunk_addr, my_chunk_alloc, io_info->raw_dxpl_id, chunk) < 0)
                    HGOTO_ERROR(H5E_IO, H5E_READERROR, NULL, "unable to read raw data chunk")

		if(old_pline && old_pline->nused) {
                    if(H5Z_pipeline(old_pline, H5Z_FLAG_REVERSE,
                            &(udata->filter_mask),
                            io_info->dxpl_cache->err_detect,
                            io_info->dxpl_cache->filter_cb,
                            &my_chunk_alloc, &buf_alloc, &chunk) < 0)
                        HGOTO_ERROR(H5E_PLINE, H5E_CANTFILTER, NULL, "data pipeline read failed")

                    /* Reallocate chunk if necessary */
                    if(udata->new_unfilt_chunk) {
                        void *tmp_chunk = chunk;

                        if(NULL == (chunk = H5D__chunk_mem_alloc(my_chunk_alloc, pline))) {
                            (void)H5D__chunk_mem_xfree(tmp_chunk, old_pline);
                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for raw data chunk")
                        } /* end if */
                        HDmemcpy(chunk, tmp_chunk, chunk_size);
                        (void)H5D__chunk_mem_xfree(tmp_chunk, old_pline);
                    } /* end if */
                } /* end if */

                /* Increment # of cache misses */
                rdcc->stats.nmisses++;
            } /* end if */
            else {
                H5D_fill_value_t	fill_status;

                /* Sanity check */
                HDassert(fill->alloc_time != H5D_ALLOC_TIME_EARLY);

                /* Chunk size on disk isn't [likely] the same size as the final chunk
                 * size in memory, so allocate memory big enough. */
                if(NULL == (chunk = H5D__chunk_mem_alloc(chunk_size, pline)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for raw data chunk")

                if(H5P_is_fill_value_defined(fill, &fill_status) < 0)
                    HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't tell if fill value defined")

                if(fill->fill_time == H5D_FILL_TIME_ALLOC ||
                        (fill->fill_time == H5D_FILL_TIME_IFSET &&
                         (fill_status == H5D_FILL_VALUE_USER_DEFINED ||
                          fill_status == H5D_FILL_VALUE_DEFAULT))) {
                    /*
                     * The chunk doesn't exist in the file.  Replicate the fill
                     * value throughout the chunk, if the fill value is defined.
                     */

                    /* Initialize the fill value buffer */
                    /* (use the compact dataset storage buffer as the fill value buffer) */
                    if(H5D__fill_init(&fb_info, chunk, NULL, NULL, NULL, NULL,
                            &dset->shared->dcpl_cache.fill, dset->shared->type,
                            dset->shared->type_id, (size_t)0, chunk_size, io_info->md_dxpl_id) < 0)
                        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "can't initialize fill buffer info")
                    fb_info_init = TRUE;

                    /* Check for VL datatype & non-default fill value */
                    if(fb_info.has_vlen_fill_type)
                        /* Fill the buffer with VL datatype fill values */
                        if(H5D__fill_refill_vl(&fb_info, fb_info.elmts_per_buf, io_info->md_dxpl_id) < 0)
                            HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, NULL, "can't refill fill value buffer")
                } /* end if */
                else
                    HDmemset(chunk, 0, chunk_size);

                /* Increment # of creations */
                rdcc->stats.ninits++;
            } /* end else */
        } /* end else */

        /* See if the chunk can be cached */
        if(rdcc->nslots > 0 && chunk_size <= rdcc->nbytes_max) {
            /* Calculate the index */
            udata->idx_hint = H5D__chunk_hash_val(io_info->dset->shared, udata->common.scaled);

            /* Add the chunk to the cache only if the slot is not already locked */
            ent = rdcc->slot[udata->idx_hint];
            if(!ent || !ent->locked) {
                /* Preempt enough things from the cache to make room */
                if(ent) {
                    if(H5D__chunk_cache_evict(io_info->dset, io_info->md_dxpl_id, io_info->dxpl_cache, ent, TRUE) < 0)
                        HGOTO_ERROR(H5E_IO, H5E_CANTINIT, NULL, "unable to preempt chunk from cache")
                } /* end if */
                if(H5D__chunk_cache_prune(io_info->dset, io_info->md_dxpl_id, io_info->dxpl_cache, chunk_size) < 0)
                    HGOTO_ERROR(H5E_IO, H5E_CANTINIT, NULL, "unable to preempt chunk(s) from cache")

                /* Create a new entry */
                if(NULL == (ent = H5FL_CALLOC(H5D_rdcc_ent_t)))
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, NULL, "can't allocate raw data chunk entry")

		ent->edge_chunk_state = disable_filters ? H5D_RDCC_DISABLE_FILTERS : 0;
		if(udata->new_unfilt_chunk)
		    ent->edge_chunk_state |= H5D_RDCC_NEWLY_DISABLED_FILTERS;

                /* Initialize the new entry */
                ent->chunk_block.offset = chunk_addr;
                ent->chunk_block.length = chunk_alloc;
		ent->chunk_idx = udata->chunk_idx;
                HDmemcpy(ent->scaled, udata->common.scaled, sizeof(hsize_t) * layout->u.chunk.ndims);
                H5_CHECKED_ASSIGN(ent->rd_count, uint32_t, chunk_size, size_t);
                H5_CHECKED_ASSIGN(ent->wr_count, uint32_t, chunk_size, size_t);
                ent->chunk = (uint8_t *)chunk;

                /* Add it to the cache */
                HDassert(NULL == rdcc->slot[udata->idx_hint]);
                rdcc->slot[udata->idx_hint] = ent;
                ent->idx = udata->idx_hint;
                rdcc->nbytes_used += chunk_size;
                rdcc->nused++;

                /* Add it to the linked list */
                if(rdcc->tail) {
                    rdcc->tail->next = ent;
                    ent->prev = rdcc->tail;
                    rdcc->tail = ent;
                } /* end if */
                else
                    rdcc->head = rdcc->tail = ent;
		ent->tmp_next = NULL;
		ent->tmp_prev = NULL;

            } /* end if */
            else
                /* We did not add the chunk to cache */
                ent = NULL;
        } /* end else */
        else /* No cache set up, or chunk is too large: chunk is uncacheable */
            ent = NULL;
    } /* end else */

    /* Lock the chunk into the cache */
    if(ent) {
        HDassert(!ent->locked);
        ent->locked = TRUE;
        chunk = ent->chunk;
    } /* end if */
    else
        /*
         * The chunk cannot be placed in cache so we don't cache it. This is the
         * reason all those arguments have to be repeated for the unlock
         * function.
         */
        udata->idx_hint = UINT_MAX;

    /* Set return value */
    ret_value = chunk;

done:
    /* Release the fill buffer info, if it's been initialized */
    if(fb_info_init && H5D__fill_term(&fb_info) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, NULL, "Can't release fill buffer info")

    /* Release the chunk allocated, on error */
    if(!ret_value)
        if(chunk)
            chunk = H5D__chunk_mem_xfree(chunk, pline);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_lock() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_unlock
 *
 * Purpose:	Unlocks a previously locked chunk. The LAYOUT, COMP, and
 *		OFFSET arguments should be the same as for H5D__chunk_lock().
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
static herr_t
H5D__chunk_unlock(const H5D_io_info_t *io_info, const H5D_chunk_ud_t *udata,
    hbool_t dirty, void *chunk, uint32_t naccessed)
{
    const H5O_layout_t *layout = &(io_info->dset->shared->layout); /* Dataset layout */
    const H5D_rdcc_t	*rdcc = &(io_info->dset->shared->cache.chunk);
    herr_t              ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(io_info);
    HDassert(udata);

    if(UINT_MAX == udata->idx_hint) {
        /*
         * It's not in the cache, probably because it's too big.  If it's
         * dirty then flush it to disk.  In any case, free the chunk.
         */
        hbool_t is_unfiltered_edge_chunk = FALSE; /* Whether the chunk is an unfiltered edge chunk */

        /* Check if we should disable filters on this chunk */
        if(udata->new_unfilt_chunk) {
            HDassert(layout->u.chunk.flags & H5O_LAYOUT_CHUNK_DONT_FILTER_PARTIAL_BOUND_CHUNKS);

            is_unfiltered_edge_chunk = TRUE;
        } /* end if */
        else if(layout->u.chunk.flags & H5O_LAYOUT_CHUNK_DONT_FILTER_PARTIAL_BOUND_CHUNKS) {
            /* Check if the chunk is an edge chunk, and disable filters if so */
            is_unfiltered_edge_chunk = H5D__chunk_is_partial_edge_chunk(
                    io_info->dset->shared->ndims, layout->u.chunk.dim,
                    io_info->store->chunk.scaled, io_info->dset->shared->curr_dims);
        } /* end if */

        if(dirty) {
            H5D_rdcc_ent_t fake_ent;         /* "fake" chunk cache entry */

            HDmemset(&fake_ent, 0, sizeof(fake_ent));
            fake_ent.dirty = TRUE;
            if(is_unfiltered_edge_chunk)
                fake_ent.edge_chunk_state = H5D_RDCC_DISABLE_FILTERS;
            if(udata->new_unfilt_chunk)
                fake_ent.edge_chunk_state |= H5D_RDCC_NEWLY_DISABLED_FILTERS;
	    HDmemcpy(fake_ent.scaled, udata->common.scaled, sizeof(hsize_t) * layout->u.chunk.ndims);
            HDassert(layout->u.chunk.size > 0);
	    fake_ent.chunk_idx = udata->chunk_idx;
            fake_ent.chunk_block.offset = udata->chunk_block.offset;
            fake_ent.chunk_block.length = udata->chunk_block.length;
            fake_ent.chunk = (uint8_t *)chunk;

            if(H5D__chunk_flush_entry(io_info->dset, io_info->md_dxpl_id, io_info->dxpl_cache, &fake_ent, TRUE) < 0)
                HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "cannot flush indexed storage buffer")
        } /* end if */
        else {
            if(chunk)
                chunk = H5D__chunk_mem_xfree(chunk, (is_unfiltered_edge_chunk ? NULL
			: &(io_info->dset->shared->dcpl_cache.pline)));
        } /* end else */
    } /* end if */
    else {
        H5D_rdcc_ent_t	*ent;   /* Chunk's entry in the cache */

        /* Sanity check */
	HDassert(udata->idx_hint < rdcc->nslots);
	HDassert(rdcc->slot[udata->idx_hint]);
	HDassert(rdcc->slot[udata->idx_hint]->chunk == chunk);

        /*
         * It's in the cache so unlock it.
         */
        ent = rdcc->slot[udata->idx_hint];
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
} /* end H5D__chunk_unlock() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_allocated_cb
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
static int
H5D__chunk_allocated_cb(const H5D_chunk_rec_t *chunk_rec, void *_udata)
{
    hsize_t *nbytes = (hsize_t *)_udata;

    FUNC_ENTER_STATIC_NOERR

    *(hsize_t *)nbytes += chunk_rec->nbytes;

    FUNC_LEAVE_NOAPI(H5_ITER_CONT)
} /* H5D__chunk_allocated_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_allocated
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
H5D__chunk_allocated(H5D_t *dset, hid_t dxpl_id, hsize_t *nbytes)
{
    H5D_chk_idx_info_t idx_info;        /* Chunked index info */
    const H5D_rdcc_t   *rdcc = &(dset->shared->cache.chunk);	/* Raw data chunk cache */
    H5D_rdcc_ent_t     *ent;            /* Cache entry  */
    H5D_dxpl_cache_t _dxpl_cache;       /* Data transfer property cache buffer */
    H5D_dxpl_cache_t *dxpl_cache = &_dxpl_cache;   /* Data transfer property cache */
    hsize_t chunk_bytes = 0;            /* Number of bytes allocated for chunks */
    H5O_storage_chunk_t *sc = &(dset->shared->layout.storage.u.chunk);
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_PACKAGE

    HDassert(dset);
    HDassert(dset->shared);
    H5D_CHUNK_STORAGE_INDEX_CHK(sc);

    /* Fill the DXPL cache values for later use */
    if(H5D__get_dxpl_cache(dxpl_id, &dxpl_cache) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't fill dxpl cache")

    /* Search for cached chunks that haven't been written out */
    for(ent = rdcc->head; ent; ent = ent->next) {
        /* Flush the chunk out to disk, to make certain the size is correct later */
        if(H5D__chunk_flush_entry(dset, dxpl_id, dxpl_cache, ent, FALSE) < 0)
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "cannot flush indexed storage buffer")
    } /* end for */

    /* Compose chunked index info struct */
    idx_info.f = dset->oloc.file;
    idx_info.dxpl_id = dxpl_id;
    idx_info.pline = &dset->shared->dcpl_cache.pline;
    idx_info.layout = &dset->shared->layout.u.chunk;
    idx_info.storage = &dset->shared->layout.storage.u.chunk;

    /* Iterate over the chunks */
    if((dset->shared->layout.storage.u.chunk.ops->iterate)(&idx_info, H5D__chunk_allocated_cb, &chunk_bytes) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to retrieve allocated chunk information from index")

    /* Set number of bytes for caller */
    *nbytes = chunk_bytes;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_allocated() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_allocate
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
H5D__chunk_allocate(const H5D_io_info_t *io_info, hbool_t full_overwrite, hsize_t old_dim[])
{
    const H5D_t *dset = io_info->dset;  /* the dataset pointer */
    H5D_chk_idx_info_t idx_info;        /* Chunked index info */
    const H5D_chunk_ops_t *ops = dset->shared->layout.storage.u.chunk.ops;      /* Chunk operations */
    hsize_t     min_unalloc[H5O_LAYOUT_NDIMS]; /* First chunk in each dimension that is unallocated (in scaled coordinates) */
    hsize_t     max_unalloc[H5O_LAYOUT_NDIMS]; /* Last chunk in each dimension that is unallocated (in scaled coordinates) */
    hsize_t	scaled[H5O_LAYOUT_NDIMS]; /* Offset of current chunk (in scaled coordinates) */
    size_t	orig_chunk_size; /* Original size of chunk in bytes */
    size_t      chunk_size;      /* Actual size of chunk in bytes, possibly filtered */
    unsigned    filter_mask = 0; /* Filter mask for chunks that have them */
    const H5O_layout_t *layout = &(dset->shared->layout);       /* Dataset layout */
    const H5O_pline_t *pline = &(dset->shared->dcpl_cache.pline);    /* I/O pipeline info */
    const H5O_pline_t def_pline = H5O_CRT_PIPELINE_DEF;              /* Default pipeline */
    const H5O_fill_t *fill = &(dset->shared->dcpl_cache.fill);    /* Fill value info */
    H5D_fill_value_t fill_status; /* The fill value status */
    hbool_t     should_fill = FALSE; /* Whether fill values should be written */
    void        *unfilt_fill_buf = NULL; /* Unfiltered fill value buffer */
    void        **fill_buf = NULL;      /* Pointer to the fill buffer to use for a chunk */
    H5D_dxpl_cache_t _dxpl_cache;       /* Data transfer property cache buffer */
    H5D_dxpl_cache_t *dxpl_cache = &_dxpl_cache;   /* Data transfer property cache */
#ifdef H5_HAVE_PARALLEL
    hbool_t     blocks_written = FALSE; /* Flag to indicate that chunk was actually written */
    hbool_t     using_mpi = FALSE;    /* Flag to indicate that the file is being accessed with an MPI-capable file driver */
    H5D_chunk_coll_info_t chunk_info; /* chunk address information for doing I/O */
#endif /* H5_HAVE_PARALLEL */
    hid_t       md_dxpl_id = io_info->md_dxpl_id;
    hid_t       raw_dxpl_id = io_info->raw_dxpl_id;
    hbool_t	carry;          /* Flag to indicate that chunk increment carrys to higher dimension (sorta) */
    unsigned    space_ndims;    /* Dataset's space rank */
    const hsize_t *space_dim;   /* Dataset's dataspace dimensions */
    const uint32_t *chunk_dim = layout->u.chunk.dim; /* Convenience pointer to chunk dimensions */
    unsigned    op_dim;                 /* Current operating dimension */
    H5D_fill_buf_info_t fb_info;        /* Dataset's fill buffer info */
    hbool_t     fb_info_init = FALSE;   /* Whether the fill value buffer has been initialized */
    hbool_t     has_unfilt_edge_chunks = FALSE; /* Whether there are partial edge chunks with disabled filters */
    hbool_t     unfilt_edge_chunk_dim[H5O_LAYOUT_NDIMS]; /* Whether there are unfiltered edge chunks at the edge of each dimension */
    hsize_t     edge_chunk_scaled[H5O_LAYOUT_NDIMS]; /* Offset of the unfiltered edge chunks at the edge of each dimension */
    unsigned    nunfilt_edge_chunk_dims = 0; /* Number of dimensions on an edge */
    const H5O_storage_chunk_t *sc = &(layout->storage.u.chunk);
    herr_t	ret_value = SUCCEED;	/* Return value */

    FUNC_ENTER_PACKAGE_TAG(md_dxpl_id, dset->oloc.addr, FAIL)

    /* Check args */
    HDassert(dset && H5D_CHUNKED == layout->type);
    HDassert(layout->u.chunk.ndims > 0 && layout->u.chunk.ndims <= H5O_LAYOUT_NDIMS);
    H5D_CHUNK_STORAGE_INDEX_CHK(sc);
    HDassert(TRUE == H5P_isa_class(md_dxpl_id, H5P_DATASET_XFER));
    HDassert(TRUE == H5P_isa_class(raw_dxpl_id, H5P_DATASET_XFER));

    /* Retrieve the dataset dimensions */
    space_dim = dset->shared->curr_dims;
    space_ndims = dset->shared->ndims;

    /* The last dimension in scaled chunk coordinates is always 0 */
    scaled[space_ndims] = (hsize_t)0;

    /* Check if any space dimensions are 0, if so we do not have to do anything
     */
    for(op_dim = 0; op_dim < (unsigned)space_ndims; op_dim++)
        if(space_dim[op_dim] == 0) {
            /* Reset any cached chunk info for this dataset */
            H5D__chunk_cinfo_cache_reset(&dset->shared->cache.chunk.last);
            HGOTO_DONE(SUCCEED)
        } /* end if */

#ifdef H5_HAVE_PARALLEL
    /* Retrieve MPI parameters */
    if(H5F_HAS_FEATURE(dset->oloc.file, H5FD_FEAT_HAS_MPI)) {
        /* Set the MPI-capable file driver flag */
        using_mpi = TRUE;

        /* init chunk info stuff for collective I/O */
        chunk_info.num_io = 0;
        chunk_info.addr = NULL;
    } /* end if */
#endif  /* H5_HAVE_PARALLEL */

    /* Fill the DXPL cache values for later use */
    if(H5D__get_dxpl_cache(raw_dxpl_id, &dxpl_cache) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't fill dxpl cache")

    /* Calculate the minimum and maximum chunk offsets in each dimension, and
     * determine if there are any unfiltered partial edge chunks.  Note that we
     * assume here that all elements of space_dim are > 0.  This is checked at
     * the top of this function. */
    for(op_dim=0; op_dim<space_ndims; op_dim++) {
        min_unalloc[op_dim] = (old_dim[op_dim] + chunk_dim[op_dim] - 1) / chunk_dim[op_dim];
        max_unalloc[op_dim] = (space_dim[op_dim] - 1) / chunk_dim[op_dim];

        /* Calculate if there are unfiltered edge chunks at the edge of this
         * dimension.  Note the edge_chunk_scaled is uninitialized for
         * dimensions where unfilt_edge_chunk_dim is FALSE.  Also  */
        if((layout->u.chunk.flags
                & H5O_LAYOUT_CHUNK_DONT_FILTER_PARTIAL_BOUND_CHUNKS)
                && pline->nused > 0
                && space_dim[op_dim] % chunk_dim[op_dim] != 0) {
            has_unfilt_edge_chunks = TRUE;
            unfilt_edge_chunk_dim[op_dim] = TRUE;
            edge_chunk_scaled[op_dim] = max_unalloc[op_dim];
        } /* end if */
        else
            unfilt_edge_chunk_dim[op_dim] = FALSE;
    } /* end for */

    /* Get original chunk size */
    H5_CHECKED_ASSIGN(orig_chunk_size, size_t, layout->u.chunk.size, uint32_t);

    /* Check the dataset's fill-value status */
    if(H5P_is_fill_value_defined(fill, &fill_status) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't tell if fill value defined")

    /* If we are filling the dataset on allocation or "if set" and
     * the fill value _is_ set, _and_ we are not overwriting the new blocks,
     * or if there are any pipeline filters defined,
     * set the "should fill" flag
     */
    if((!full_overwrite && (fill->fill_time == H5D_FILL_TIME_ALLOC ||
            (fill->fill_time == H5D_FILL_TIME_IFSET &&
             (fill_status == H5D_FILL_VALUE_USER_DEFINED ||
              fill_status == H5D_FILL_VALUE_DEFAULT))))
            || pline->nused > 0)
        should_fill = TRUE;

    /* Check if fill values should be written to chunks */
    if(should_fill) {
        /* Initialize the fill value buffer */
        /* (delay allocating fill buffer for VL datatypes until refilling) */
        /* (casting away const OK - QAK) */
        if(H5D__fill_init(&fb_info, NULL, (H5MM_allocate_t)H5D__chunk_mem_alloc,
                (void *)pline, (H5MM_free_t)H5D__chunk_mem_xfree, (void *)pline,
                &dset->shared->dcpl_cache.fill, dset->shared->type,
                dset->shared->type_id, (size_t)0, orig_chunk_size, md_dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't initialize fill buffer info")
        fb_info_init = TRUE;

        /* Initialize the fill_buf pointer to the buffer in fb_info.  If edge
         * chunk filters are disabled, we will switch the buffer as appropriate
         * for each chunk. */
        fill_buf = &fb_info.fill_buf;

        /* Check if there are filters which need to be applied to the chunk */
        /* (only do this in advance when the chunk info can be re-used (i.e.
         *      it doesn't contain any non-default VL datatype fill values)
         */
        if(!fb_info.has_vlen_fill_type && pline->nused > 0) {
            size_t buf_size = orig_chunk_size;

            /* If the dataset has disabled partial chunk filters, create a copy
             * of the unfiltered fill_buf to use for partial chunks */
            if(has_unfilt_edge_chunks) {
                if(NULL == (unfilt_fill_buf = H5D__chunk_mem_alloc(orig_chunk_size, &def_pline)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for raw data chunk")
                HDmemcpy(unfilt_fill_buf, fb_info.fill_buf, orig_chunk_size);
            } /* end if */

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
    idx_info.dxpl_id = md_dxpl_id;
    idx_info.pline = &dset->shared->dcpl_cache.pline;
    idx_info.layout = &dset->shared->layout.u.chunk;
    idx_info.storage = &dset->shared->layout.storage.u.chunk;

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
     *
     * Note that min_unalloc & max_unalloc are in scaled coordinates.
     *
     */
    chunk_size = orig_chunk_size;
    for(op_dim = 0; op_dim < space_ndims; op_dim++) {
        H5D_chunk_ud_t udata;   /* User data for querying chunk info */
        unsigned u;             /* Local index variable */
        int i;                  /* Local index variable */

        /* Check if allocation along this dimension is really necessary */
        if(min_unalloc[op_dim] > max_unalloc[op_dim])
            continue;
        else {
            /* Reset the chunk offset indices */
            HDmemset(scaled, 0, (space_ndims * sizeof(scaled[0])));
            scaled[op_dim] = min_unalloc[op_dim];

            if(has_unfilt_edge_chunks) {
                /* Initialize nunfilt_edge_chunk_dims */
                nunfilt_edge_chunk_dims = 0;
                for(u = 0; u < space_ndims; u++)
                    if(unfilt_edge_chunk_dim[u] && scaled[u]
                            == edge_chunk_scaled[u])
                        nunfilt_edge_chunk_dims++;

                /* Initialize chunk_size and fill_buf */
                if(should_fill && !fb_info.has_vlen_fill_type) {
                    HDassert(fb_info_init);
                    HDassert(unfilt_fill_buf);
                    if(nunfilt_edge_chunk_dims) {
                        fill_buf = &unfilt_fill_buf;
                        chunk_size = layout->u.chunk.size;
                    } /* end if */
                    else {
                        fill_buf = &fb_info.fill_buf;
                        chunk_size = orig_chunk_size;
                    } /* end else */
                } /* end if */
            } /* end if */

            carry = FALSE;
        } /* end else */

        while(!carry) {
            hbool_t need_insert = FALSE;    /* Whether the chunk needs to be inserted into the index */

	    /* Look up this chunk */
	    if(H5D__chunk_lookup(dset, md_dxpl_id, scaled, &udata) < 0)
		HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "error looking up chunk address")
#ifndef NDEBUG
            /* None of the chunks should be allocated */
	    if(H5D_CHUNK_IDX_NONE != layout->storage.u.chunk.idx_type)
		HDassert(!H5F_addr_defined(udata.chunk_block.offset));

            /* Make sure the chunk is really in the dataset and outside the
             * original dimensions */
            {
                unsigned v;             /* Local index variable */
                hbool_t outside_orig = FALSE;

                for(v = 0; v < space_ndims; v++) {
                    HDassert((scaled[v] * chunk_dim[v]) < space_dim[v]);
                    if((scaled[v] * chunk_dim[v]) >= old_dim[v])
                        outside_orig = TRUE;
                } /* end for */
                HDassert(outside_orig);
            } /* end block */
#endif /* NDEBUG */

            /* Check for VL datatype & non-default fill value */
            if(fb_info_init && fb_info.has_vlen_fill_type) {
                /* Sanity check */
                HDassert(should_fill);
                HDassert(!unfilt_fill_buf);
#ifdef H5_HAVE_PARALLEL
                HDassert(!using_mpi);   /* Can't write VL datatypes in parallel currently */
#endif

                /* Check to make sure the buffer is large enough.  It is
                 * possible (though ill-advised) for the filter to shrink the
                 * buffer. */
                if(fb_info.fill_buf_size < orig_chunk_size) {
                    if(NULL == (fb_info.fill_buf = H5D__chunk_mem_realloc(fb_info.fill_buf, orig_chunk_size, pline)))
                        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory reallocation failed for raw data chunk")
                    fb_info.fill_buf_size = orig_chunk_size;
                } /* end if */

                /* Fill the buffer with VL datatype fill values */
                if(H5D__fill_refill_vl(&fb_info, fb_info.elmts_per_buf, md_dxpl_id) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, FAIL, "can't refill fill value buffer")

                /* Check if there are filters which need to be applied to the chunk */
                if((pline->nused > 0) && !nunfilt_edge_chunk_dims) {
                    size_t nbytes = orig_chunk_size;

                    /* Push the chunk through the filters */
                    if(H5Z_pipeline(pline, 0, &filter_mask, dxpl_cache->err_detect, dxpl_cache->filter_cb, &nbytes, &fb_info.fill_buf_size, &fb_info.fill_buf) < 0)
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
                    chunk_size = layout->u.chunk.size;

                HDassert(*fill_buf == fb_info.fill_buf);
            } /* end if */

            /* Initialize the chunk information */
            udata.common.layout = &layout->u.chunk;
            udata.common.storage = &layout->storage.u.chunk;
            udata.common.scaled = scaled;
            udata.chunk_block.offset = HADDR_UNDEF;
            H5_CHECKED_ASSIGN(udata.chunk_block.length, uint32_t, chunk_size, size_t);
            udata.filter_mask = filter_mask;

            /* Allocate the chunk (with all processes) */
	    if(H5D__chunk_file_alloc(&idx_info, NULL, &udata.chunk_block, &need_insert, scaled) < 0)
		HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, FAIL, "unable to insert/resize chunk on chunk level")
            HDassert(H5F_addr_defined(udata.chunk_block.offset));

            /* Check if fill values should be written to chunks */
            if(should_fill) {
                /* Sanity check */
                HDassert(fb_info_init);
                HDassert(udata.chunk_block.length == chunk_size);

#ifdef H5_HAVE_PARALLEL
                /* Check if this file is accessed with an MPI-capable file driver */
                if(using_mpi) {
                    /* collect all chunk addresses to be written to
                       write collectively at the end */
                    /* allocate/resize address array if no more space left */
                    /* Note that if we add support for parallel filters we must
                     * also store an array of chunk sizes and pass it to the
                     * apporpriate collective write function */
                    if(0 == chunk_info.num_io % 1024)
                        if(NULL == (chunk_info.addr = (haddr_t *)H5MM_realloc(chunk_info.addr, (chunk_info.num_io + 1024) * sizeof(haddr_t))))
                            HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "memory allocation failed for chunk addresses")

                    /* Store the chunk's address for later */
                    chunk_info.addr[chunk_info.num_io] = udata.chunk_block.offset;
                    chunk_info.num_io++;

                    /* Indicate that blocks will be written */
                    blocks_written = TRUE;
                } /* end if */
                else {
#endif /* H5_HAVE_PARALLEL */
                    if(H5F_block_write(dset->oloc.file, H5FD_MEM_DRAW, udata.chunk_block.offset, chunk_size, raw_dxpl_id, *fill_buf) < 0)
                        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to write raw data to file")
#ifdef H5_HAVE_PARALLEL
                } /* end else */
#endif /* H5_HAVE_PARALLEL */
            } /* end if */

            /* Insert the chunk record into the index */
	    if(need_insert && ops->insert)
                if((ops->insert)(&idx_info, &udata, dset) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, FAIL, "unable to insert chunk addr into index")

            /* Increment indices and adjust the edge chunk state */
            carry = TRUE;
            for(i = ((int)space_ndims - 1); i >= 0; --i) {
                scaled[i]++;
                if(scaled[i] > max_unalloc[i]) {
                    if((unsigned)i == op_dim)
                        scaled[i] = min_unalloc[i];
                    else
                        scaled[i] = 0;

                    /* Check if we just left the edge in this dimension */
                    if(unfilt_edge_chunk_dim[i]
                            && edge_chunk_scaled[i] == max_unalloc[i]
                            && scaled[i] < edge_chunk_scaled[i]) {
                        nunfilt_edge_chunk_dims--;
                        if(should_fill && nunfilt_edge_chunk_dims == 0 && !fb_info.has_vlen_fill_type) {
                            HDassert(!H5D__chunk_is_partial_edge_chunk(space_ndims, chunk_dim, scaled, space_dim));
                            fill_buf = &fb_info.fill_buf;
                            chunk_size = orig_chunk_size;
                        } /* end if */
                    } /* end if */
                } /* end if */
                else {
                    /* Check if we just entered the edge in this dimension */
                    if(unfilt_edge_chunk_dim[i] && scaled[i] == edge_chunk_scaled[i]) {
                        HDassert(edge_chunk_scaled[i] == max_unalloc[i]);
                        nunfilt_edge_chunk_dims++;
                        if(should_fill && nunfilt_edge_chunk_dims == 1 && !fb_info.has_vlen_fill_type) {
                            HDassert(H5D__chunk_is_partial_edge_chunk(space_ndims, chunk_dim, scaled, space_dim));
                            fill_buf = &unfilt_fill_buf;
                            chunk_size = layout->u.chunk.size;
                        } /* end if */
                    } /* end if */

                    carry = FALSE;
                    break;
                } /* end else */
            } /* end for */
        } /* end while(!carry) */

        /* Adjust max_unalloc so we don't allocate the same chunk twice.  Also
         * check if this dimension started from 0 (and hence allocated all of
         * the chunks. */
        if(min_unalloc[op_dim] == 0)
            break;
        else
            max_unalloc[op_dim] = min_unalloc[op_dim] - 1;
    } /* end for(op_dim=0...) */

#ifdef H5_HAVE_PARALLEL
    /* do final collective I/O */
    if(using_mpi && blocks_written)
        if(H5D__chunk_collective_fill(dset, raw_dxpl_id, &chunk_info, chunk_size, fb_info.fill_buf) < 0)
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to write raw data to file")
#endif /* H5_HAVE_PARALLEL */

    /* Reset any cached chunk info for this dataset */
    H5D__chunk_cinfo_cache_reset(&dset->shared->cache.chunk.last);

done:
    /* Release the fill buffer info, if it's been initialized */
    if(fb_info_init && H5D__fill_term(&fb_info) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't release fill buffer info")

    /* Free the unfiltered fill value buffer */
    unfilt_fill_buf = H5D__chunk_mem_xfree(unfilt_fill_buf, &def_pline);

#ifdef H5_HAVE_PARALLEL
    if(using_mpi && chunk_info.addr)
        H5MM_free(chunk_info.addr);
#endif

    FUNC_LEAVE_NOAPI_TAG(ret_value, FAIL)
} /* end H5D__chunk_allocate() */


/*-------------------------------------------------------------------------
 * Function:    H5D__chunk_update_old_edge_chunks
 *
 * Purpose:     Update all chunks which were previously partial edge
 *              chunks and are now complete.  Determines exactly which
 *              chunks need to be updated and locks each into cache using
 *              the 'prev_unfilt_chunk' flag, then unlocks it, causing
 *              filters to be applied as necessary.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Neil Fortner
 *              April 14, 2010
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__chunk_update_old_edge_chunks(H5D_t *dset, hid_t dxpl_id, hsize_t old_dim[])
{
    hsize_t     old_edge_chunk_sc[H5O_LAYOUT_NDIMS]; /* Offset of first previously incomplete chunk in each dimension */
    hsize_t     max_edge_chunk_sc[H5O_LAYOUT_NDIMS]; /* largest offset of chunks that might need to be modified in each dimension */
    hbool_t     new_full_dim[H5O_LAYOUT_NDIMS]; /* Whether the plane of chunks in this dimension needs to be modified */
    const H5O_layout_t *layout = &(dset->shared->layout); /* Dataset layout */
    const H5O_pline_t *pline = &(dset->shared->dcpl_cache.pline); /* I/O pipeline info */
    hsize_t     chunk_sc[H5O_LAYOUT_NDIMS]; /* Offset of current chunk */
    const uint32_t *chunk_dim = layout->u.chunk.dim; /* Convenience pointer to chunk dimensions */
    unsigned    space_ndims;            /* Dataset's space rank */
    const hsize_t *space_dim;           /* Dataset's dataspace dimensions */
    unsigned    op_dim;                 /* Current operationg dimension */
    H5D_io_info_t chk_io_info;          /* Chunked I/O info object */
    H5D_chunk_ud_t chk_udata;           /* User data for locking chunk */
    H5D_storage_t chk_store;            /* Chunk storage information */
    H5D_dxpl_cache_t _dxpl_cache;       /* Data transfer property cache buffer */
    H5D_dxpl_cache_t *dxpl_cache = &_dxpl_cache;   /* Data transfer property cache */
    void        *chunk;                 /* The file chunk  */
    hbool_t     carry;                  /* Flag to indicate that chunk increment carrys to higher dimension (sorta) */
    const H5O_storage_chunk_t *sc = &(layout->storage.u.chunk);
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(dset && H5D_CHUNKED == layout->type);
    HDassert(layout->u.chunk.ndims > 0 && layout->u.chunk.ndims <= H5O_LAYOUT_NDIMS);
    H5D_CHUNK_STORAGE_INDEX_CHK(sc);
    HDassert(TRUE == H5P_isa_class(dxpl_id, H5P_DATASET_XFER));
    HDassert(pline->nused > 0);
    HDassert(layout->u.chunk.flags
            & H5O_LAYOUT_CHUNK_DONT_FILTER_PARTIAL_BOUND_CHUNKS);

    /* Retrieve the dataset dimensions */
    space_dim = dset->shared->curr_dims;
    space_ndims = dset->shared->ndims;

    /* The last dimension in chunk_offset is always 0 */
    chunk_sc[space_ndims] = (hsize_t)0;

    /* Check if any current dimensions are smaller than the chunk size, or if
     * any old dimensions are 0.  If so we do not have to do anything. */
    for(op_dim=0; op_dim<space_ndims; op_dim++)
        if((space_dim[op_dim] < chunk_dim[op_dim])  || old_dim[op_dim] == 0) {
            /* Reset any cached chunk info for this dataset */
            H5D__chunk_cinfo_cache_reset(&dset->shared->cache.chunk.last);
            HGOTO_DONE(SUCCEED)
        } /* end if */

    /*
     * Initialize structures needed to lock chunks into cache
     */
    /* Fill the DXPL cache values for later use */
    if(H5D__get_dxpl_cache(dxpl_id, &dxpl_cache) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't fill dxpl cache")

    /* Set up chunked I/O info object, for operations on chunks (in callback).
     * Note that we only need to set chunk_offset once, as the array's address
     * will never change. */
    chk_store.chunk.scaled = chunk_sc;
    H5D_BUILD_IO_INFO_RD(&chk_io_info, dset, dxpl_cache, dxpl_id, H5AC_rawdata_dxpl_id, &chk_store, NULL);

    /*
     * Determine the edges of the dataset which need to be modified
     */
    for(op_dim=0; op_dim<space_ndims; op_dim++) {
        /* Start off with this dimension marked as not needing to be modified */
        new_full_dim[op_dim] = FALSE;

        /* Calulate offset of first previously incomplete chunk in this
         * dimension */
	old_edge_chunk_sc[op_dim] = (old_dim[op_dim] / chunk_dim[op_dim]);

        /* Calculate the largest offset of chunks that might need to be
         * modified in this dimension */
	max_edge_chunk_sc[op_dim] = MIN((old_dim[op_dim] - 1) / chunk_dim[op_dim],
					  MAX((space_dim[op_dim] / chunk_dim[op_dim]), 1) - 1);

        /* Check for old_dim aligned with chunk boundary in this dimension, if
         * so we do not need to modify chunks along the edge in this dimension
         */
        if(old_dim[op_dim] % chunk_dim[op_dim] == 0)
            continue;

        /* Check if the dataspace expanded enough to cause the old edge chunks
         * in this dimension to become full */
	if((space_dim[op_dim]/chunk_dim[op_dim]) >= (old_edge_chunk_sc[op_dim] + 1))
            new_full_dim[op_dim] = TRUE;
    } /* end for */

    /* Main loop: fix old edge chunks */
    for(op_dim=0; op_dim<space_ndims; op_dim++) {
        /* Check if allocation along this dimension is really necessary */
        if(!new_full_dim[op_dim])
            continue;
        else {
            HDassert(max_edge_chunk_sc[op_dim] == old_edge_chunk_sc[op_dim]);

            /* Reset the chunk offset indices */
            HDmemset(chunk_sc, 0, (space_ndims * sizeof(chunk_sc[0])));
            chunk_sc[op_dim] = old_edge_chunk_sc[op_dim];

            carry = FALSE;
        } /* end if */

        while(!carry) {
            int i;                  /* Local index variable */

            /* Make sure the chunk is really a former edge chunk */
            HDassert(H5D__chunk_is_partial_edge_chunk(space_ndims, chunk_dim, chunk_sc, old_dim)
                    && !H5D__chunk_is_partial_edge_chunk(space_ndims, chunk_dim, chunk_sc, space_dim));

            /* Lookup the chunk */
            if(H5D__chunk_lookup(dset, dxpl_id, chunk_sc, &chk_udata) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "error looking up chunk address")

            /* If this chunk does not exist in cache or on disk, no need to do
             * anything */
            if(H5F_addr_defined(chk_udata.chunk_block.offset)
                    || (UINT_MAX != chk_udata.idx_hint)) {
                /* Lock the chunk into cache.  H5D__chunk_lock will take care of
                * updating the chunk to no longer be an edge chunk. */
                if(NULL == (chunk = (void *)H5D__chunk_lock(&chk_io_info, &chk_udata, FALSE, TRUE)))
                    HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "unable to lock raw data chunk")

                /* Unlock the chunk */
                if(H5D__chunk_unlock(&chk_io_info, &chk_udata, TRUE, chunk, (uint32_t)0) < 0)
                    HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to unlock raw data chunk")
            } /* end if */

            /* Increment indices */
            carry = TRUE;
            for(i = ((int)space_ndims - 1); i >= 0; --i) {
                if((unsigned)i != op_dim) {
		    ++chunk_sc[i];
                    if(chunk_sc[i] > (hsize_t) max_edge_chunk_sc[i])
                        chunk_sc[i] = 0;
                    else {
                        carry = FALSE;
                        break;
                    } /* end else */
                } /* end if */
            } /* end for */
        } /* end while(!carry) */

        /* Adjust max_edge_chunk_sc so we don't modify the same chunk twice.
         * Also check if this dimension started from 0 (and hence modified all
         * of the old edge chunks. */
        if(old_edge_chunk_sc[op_dim] == 0)
            break;
        else
            --max_edge_chunk_sc[op_dim];
    } /* end for(op_dim=0...) */

    /* Reset any cached chunk info for this dataset */
    H5D__chunk_cinfo_cache_reset(&dset->shared->cache.chunk.last);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_update_old_edge_chunks() */

#ifdef H5_HAVE_PARALLEL

/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_collective_fill
 *
 * Purpose:     Use MPIO collective write to fill the chunks (if number of
 *              chunks to fill is greater than the number of MPI procs; 
 *              otherwise use independent I/O).
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Mohamad Chaarawi
 * 		July 30, 2014
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__chunk_collective_fill(const H5D_t *dset, hid_t dxpl_id,
    H5D_chunk_coll_info_t *chunk_info, size_t chunk_size, const void *fill_buf)
{
    MPI_Comm	mpi_comm = MPI_COMM_NULL;	/* MPI communicator for file */
    int         mpi_rank = (-1);    /* This process's rank  */
    int         mpi_size = (-1);    /* MPI Comm size  */
    int         mpi_code;           /* MPI return code */
    size_t      num_blocks;         /* Number of blocks between processes. */
    size_t      leftover_blocks;    /* Number of leftover blocks to handle */
    int         blocks, leftover, block_len; /* converted to int for MPI */
    MPI_Aint    *chunk_disp_array = NULL;
    int         *block_lens = NULL;
    MPI_Datatype mem_type, file_type;
    hid_t       data_dxpl_id = -1;  /* DXPL ID to use for raw data I/O operations */
    int         i;                  /* Local index variable */
    herr_t ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_STATIC

    /* Get the MPI communicator */
    if(MPI_COMM_NULL == (mpi_comm = H5F_mpi_get_comm(dset->oloc.file)))
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "Can't retrieve MPI communicator")

    /* Get the MPI rank */
    if((mpi_rank = H5F_mpi_get_rank(dset->oloc.file)) < 0)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "Can't retrieve MPI rank")

    /* Get the MPI size */
    if((mpi_size = H5F_mpi_get_size(dset->oloc.file)) < 0)
        HGOTO_ERROR(H5E_INTERNAL, H5E_MPI, FAIL, "Can't retrieve MPI size")

    /* Get a copy of the DXPL, to modify */
    if((data_dxpl_id = H5P_copy_plist((H5P_genplist_t *)H5I_object(dxpl_id), TRUE)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, FAIL, "can't copy property list")

    /* Distribute evenly the number of blocks between processes. */
    num_blocks = chunk_info->num_io / mpi_size; /* value should be the same on all procs */

    /* after evenly distributing the blocks between processes, are
       there any leftover blocks for each individual process
       (round-robin) */
    leftover_blocks = chunk_info->num_io % mpi_size;

    /* Cast values to types needed by MPI */
    H5_CHECKED_ASSIGN(blocks, int, num_blocks, size_t);
    H5_CHECKED_ASSIGN(leftover, int, leftover_blocks, size_t);
    H5_CHECKED_ASSIGN(block_len, int,  chunk_size, size_t);

    /* Allocate buffers */
    /* (MSC - should not need block_lens if MPI_type_create_hindexed_block is working) */
    if(NULL == (block_lens = (int *)H5MM_malloc((blocks + 1) * sizeof(int))))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate chunk lengths buffer")
    if(NULL == (chunk_disp_array = (MPI_Aint *)H5MM_malloc((blocks + 1) * sizeof(MPI_Aint))))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate chunk file displacement buffer")

    for(i = 0 ; i < blocks ; i++) {
        /* store the chunk address as an MPI_Aint */
        chunk_disp_array[i] = (MPI_Aint)(chunk_info->addr[i + mpi_rank*blocks]);

        /* MSC - should not need this if MPI_type_create_hindexed_block is working */
        block_lens[i] = block_len;

        /* make sure that the addresses in the datatype are
           monotonically non decreasing */
        if(i)
            HDassert(chunk_disp_array[i] > chunk_disp_array[i - 1]);
    } /* end if */

    /* calculate if there are any leftover blocks after evenly
       distributing. If there are, then round robin the distribution
       to processes 0 -> leftover. */
    if(leftover && leftover > mpi_rank) {
        chunk_disp_array[blocks] = (MPI_Aint)chunk_info->addr[blocks*mpi_size + mpi_rank];        
        block_lens[blocks] = block_len;
        blocks++;
    }

    /* MSC 
     * should use this if MPI_type_create_hindexed block is working 
     * mpi_code = MPI_Type_create_hindexed_block(blocks, block_len, chunk_disp_array, MPI_BYTE, &file_type);
     */
    mpi_code = MPI_Type_create_hindexed(blocks, block_lens, chunk_disp_array, MPI_BYTE, &file_type);
    if(mpi_code != MPI_SUCCESS)
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_create_hindexed failed", mpi_code)
    if(MPI_SUCCESS != (mpi_code = MPI_Type_commit(&file_type)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code)

    mpi_code = MPI_Type_create_hvector(blocks, block_len, 0, MPI_BYTE, &mem_type);
    if(mpi_code != MPI_SUCCESS)
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_create_hvector failed", mpi_code)
    if(MPI_SUCCESS != (mpi_code = MPI_Type_commit(&mem_type)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code)

    /* set MPI-IO VFD properties */
    {
        H5FD_mpio_xfer_t xfer_mode = H5FD_MPIO_COLLECTIVE;
        H5P_genplist_t  *plist;      /* Property list pointer */

        if(NULL == (plist = H5P_object_verify(data_dxpl_id, H5P_DATASET_XFER)))
            HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a dataset transfer list")

        /* Set buffer MPI type */
        if(H5P_set(plist, H5FD_MPI_XFER_MEM_MPI_TYPE_NAME, &mem_type) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set MPI-I/O property")

        /* Set File MPI type */
        if(H5P_set(plist, H5FD_MPI_XFER_FILE_MPI_TYPE_NAME, &file_type) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set MPI-I/O property")

        /* set transfer mode */
        if(H5P_set(plist, H5D_XFER_IO_XFER_MODE_NAME, &xfer_mode) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set transfer mode")
    }

    /* low level write (collective) */
    if(H5F_block_write(dset->oloc.file, H5FD_MEM_DRAW, (haddr_t)0, (blocks) ? (size_t)1 : (size_t)0, data_dxpl_id, fill_buf) < 0)
        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to write raw data to file")

    /* Barrier so processes don't race ahead */
    if(MPI_SUCCESS != (mpi_code = MPI_Barrier(mpi_comm)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Barrier failed", mpi_code)

done:
    if(data_dxpl_id > 0 && H5I_dec_ref(data_dxpl_id) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't free property list")

    /* free things */
    if(MPI_SUCCESS != (mpi_code = MPI_Type_free(&file_type)))
        HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)
    if(MPI_SUCCESS != (mpi_code = MPI_Type_free(&mem_type)))
        HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)
    H5MM_xfree(chunk_disp_array);
    H5MM_xfree(block_lens);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_collective_fill() */
#endif /* H5_HAVE_PARALLEL */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_prune_fill
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
H5D__chunk_prune_fill(H5D_chunk_it_ud1_t *udata, hbool_t new_unfilt_chunk)
{
    const H5D_io_info_t *io_info = udata->io_info; /* Local pointer to I/O info */
    const H5D_t *dset = io_info->dset;  /* Local pointer to the dataset info */
    const H5O_layout_t *layout = &(dset->shared->layout); /* Dataset's layout */
    unsigned    rank = udata->common.layout->ndims - 1; /* Dataset rank */
    const hsize_t *scaled = udata->common.scaled; /* Scaled chunk offset */
    H5S_sel_iter_t *chunk_iter = NULL;  /* Memory selection iteration info */
    hbool_t     chunk_iter_init = FALSE; /* Whether the chunk iterator has been initialized */
    hssize_t    sel_nelmts;             /* Number of elements in selection */
    hsize_t     count[H5O_LAYOUT_NDIMS]; /* Element count of hyperslab */
    size_t      chunk_size;             /*size of a chunk       */
    void        *chunk;	                /* The file chunk  */
    H5D_chunk_ud_t chk_udata;           /* User data for locking chunk */
    uint32_t    bytes_accessed;         /* Bytes accessed in chunk */
    unsigned    u;                      /* Local index variable */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    /* Get the chunk's size */
    HDassert(layout->u.chunk.size > 0);
    H5_CHECKED_ASSIGN(chunk_size, size_t, layout->u.chunk.size, uint32_t);

    /* Get the info for the chunk in the file */
    if(H5D__chunk_lookup(dset, io_info->md_dxpl_id, scaled, &chk_udata) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "error looking up chunk address")
    chk_udata.new_unfilt_chunk = new_unfilt_chunk;

    /* If this chunk does not exist in cache or on disk, no need to do anything */
    if(!H5F_addr_defined(chk_udata.chunk_block.offset) && UINT_MAX == chk_udata.idx_hint)
        HGOTO_DONE(SUCCEED)

    /* Initialize the fill value buffer, if necessary */
    if(!udata->fb_info_init) {
        H5_CHECK_OVERFLOW(udata->elmts_per_chunk, uint32_t, size_t);
        if(H5D__fill_init(&udata->fb_info, NULL, NULL, NULL, NULL, NULL,
                &dset->shared->dcpl_cache.fill,
                dset->shared->type, dset->shared->type_id, (size_t)udata->elmts_per_chunk,
                chunk_size, io_info->md_dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't initialize fill buffer info")
        udata->fb_info_init = TRUE;
    } /* end if */

    /* Compute the # of elements to leave with existing value, in each dimension */
    for(u = 0; u < rank; u++) {
        count[u] = MIN(layout->u.chunk.dim[u], (udata->space_dim[u] - (scaled[u] * layout->u.chunk.dim[u])));
        HDassert(count[u] > 0);
    } /* end for */

    /* Select all elements in chunk, to begin with */
    if(H5S_select_all(udata->chunk_space, TRUE) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSELECT, FAIL, "unable to select space")

    /* "Subtract out" the elements to keep */
    if(H5S_select_hyperslab(udata->chunk_space, H5S_SELECT_NOTB, udata->hyper_start, NULL, count, NULL) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSELECT, FAIL, "unable to select hyperslab")

    /* Lock the chunk into the cache, to get a pointer to the chunk buffer */
    if(NULL == (chunk = (void *)H5D__chunk_lock(io_info, &chk_udata, FALSE, FALSE)))
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
        if(H5D__fill_refill_vl(&udata->fb_info, (size_t)sel_nelmts, io_info->md_dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, FAIL, "can't refill fill value buffer")

    /* Allocate the chunk selection iterator */
    if(NULL == (chunk_iter = H5FL_MALLOC(H5S_sel_iter_t)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "can't allocate chunk selection iterator")

    /* Create a selection iterator for scattering the elements to memory buffer */
    if(H5S_select_iter_init(chunk_iter, udata->chunk_space, layout->u.chunk.dim[rank]) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize chunk selection information")
    chunk_iter_init = TRUE;

    /* Scatter the data into memory */
    if(H5D__scatter_mem(udata->fb_info.fill_buf, udata->chunk_space, chunk_iter, (size_t)sel_nelmts, io_info->dxpl_cache, chunk/*out*/) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "scatter failed")


    /* The number of bytes accessed in the chunk */
    /* (i.e. the bytes replaced with fill values) */
    H5_CHECK_OVERFLOW(sel_nelmts, hssize_t, uint32_t);
    bytes_accessed = (uint32_t)sel_nelmts * layout->u.chunk.dim[rank];

    /* Release lock on chunk */
    if(H5D__chunk_unlock(io_info, &chk_udata, TRUE, chunk, bytes_accessed) < 0)
        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to unlock raw data chunk")

done:
    /* Release the selection iterator */
    if(chunk_iter_init && H5S_SELECT_ITER_RELEASE(chunk_iter) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't release selection iterator")
    if(chunk_iter)
        chunk_iter = H5FL_FREE(H5S_sel_iter_t, chunk_iter);

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D__chunk_prune_fill */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_prune_by_extent
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
 *-------------------------------------------------------------------------
 */
herr_t
H5D__chunk_prune_by_extent(H5D_t *dset, hid_t dxpl_id, const hsize_t *old_dim)
{
    hsize_t                 min_mod_chunk_sc[H5O_LAYOUT_NDIMS]; /* Scaled offset of first chunk to modify in each dimension */
    hsize_t                 max_mod_chunk_sc[H5O_LAYOUT_NDIMS]; /* Scaled offset of last chunk to modify in each dimension */
    hssize_t                max_fill_chunk_sc[H5O_LAYOUT_NDIMS]; /* Scaled offset of last chunk that might be filled in each dimension */
    hbool_t                 fill_dim[H5O_LAYOUT_NDIMS]; /* Whether the plane of edge chunks in this dimension needs to be filled */
    hsize_t                 min_partial_chunk_sc[H5O_LAYOUT_NDIMS]; /* Offset of first partial (or empty) chunk in each dimension */
    hbool_t                 new_unfilt_dim[H5O_LAYOUT_NDIMS]; /* Whether the plane of edge chunks in this dimension are newly unfiltered */
    H5D_chk_idx_info_t      idx_info;           /* Chunked index info */
    H5D_io_info_t           chk_io_info;        /* Chunked I/O info object */
    H5D_storage_t           chk_store;          /* Chunk storage information */
    H5D_dxpl_cache_t        _dxpl_cache;        /* Data transfer property cache buffer */
    H5D_dxpl_cache_t       *dxpl_cache = &_dxpl_cache;   /* Data transfer property cache */
    const H5O_layout_t     *layout = &(dset->shared->layout);   /* Dataset's layout */
    const H5D_rdcc_t       *rdcc = &(dset->shared->cache.chunk);	/*raw data chunk cache */
    unsigned                space_ndims;        /* Dataset's space rank */
    const hsize_t          *space_dim;          /* Current dataspace dimensions */
    unsigned                op_dim;             /* Current operating dimension */
    hbool_t                 shrunk_dim[H5O_LAYOUT_NDIMS]; /* Dimensions which have shrunk */
    H5D_chunk_it_ud1_t      udata;      /* Chunk index iterator user data */
    hbool_t                 udata_init = FALSE; /* Whether the chunk index iterator user data has been initialized */
    H5D_chunk_common_ud_t   idx_udata;          /* User data for index removal routine */
    H5S_t                  *chunk_space = NULL;         /* Dataspace for a chunk */
    hsize_t                 chunk_dim[H5O_LAYOUT_NDIMS];   /* Chunk dimensions */
    hsize_t                 scaled[H5O_LAYOUT_NDIMS];   /* Scaled offset of current chunk */
    hsize_t                 hyper_start[H5O_LAYOUT_NDIMS];  /* Starting location of hyperslab */
    uint32_t                elmts_per_chunk;    /* Elements in chunk */
    hbool_t                 disable_edge_filters = FALSE; /* Whether to disable filters on partial edge chunks */
    hbool_t                 new_unfilt_chunk = FALSE; /* Whether the chunk is newly unfiltered */
    unsigned                u;	                /* Local index variable */
    const H5O_storage_chunk_t *sc = &(layout->storage.u.chunk);
    herr_t                  ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(dset && H5D_CHUNKED == layout->type);
    HDassert(layout->u.chunk.ndims > 0 && layout->u.chunk.ndims <= H5O_LAYOUT_NDIMS);
    H5D_CHUNK_STORAGE_INDEX_CHK(sc);
    HDassert(dxpl_cache);

    /* Fill the DXPL cache values for later use */
    if(H5D__get_dxpl_cache(dxpl_id, &dxpl_cache) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't fill dxpl cache")

    /* Go get the rank & dimensions (including the element size) */
    space_dim = dset->shared->curr_dims;
    space_ndims = dset->shared->ndims;

    /* The last dimension in scaled is always 0 */
    scaled[space_ndims] = (hsize_t)0;

    /* Check if any old dimensions are 0, if so we do not have to do anything */
    for(op_dim = 0; op_dim < (unsigned)space_ndims; op_dim++)
        if(old_dim[op_dim] == 0) {
            /* Reset any cached chunk info for this dataset */
            H5D__chunk_cinfo_cache_reset(&dset->shared->cache.chunk.last);
            HGOTO_DONE(SUCCEED)
        } /* end if */

    /* Round up to the next integer # of chunks, to accomodate partial chunks */
    /* Use current dims because the indices have already been updated! -NAF */
    /* (also compute the number of elements per chunk) */
    /* (also copy the chunk dimensions into 'hsize_t' array for creating dataspace) */
    /* (also compute the dimensions which have been shrunk) */
    elmts_per_chunk = 1;
    for(u = 0; u < space_ndims; u++) {
        elmts_per_chunk *= layout->u.chunk.dim[u];
	chunk_dim[u] = layout->u.chunk.dim[u];
	shrunk_dim[u] = (space_dim[u] < old_dim[u]);
    } /* end for */

    /* Create a dataspace for a chunk & set the extent */
    if(NULL == (chunk_space = H5S_create_simple(space_ndims, chunk_dim, NULL)))
	HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCREATE, FAIL, "can't create simple dataspace")

    /* Reset hyperslab start array */
    /* (hyperslabs will always start from origin) */
    HDmemset(hyper_start, 0, sizeof(hyper_start));

    /* Set up chunked I/O info object, for operations on chunks (in callback)
     * Note that we only need to set scaled once, as the array's address
     * will never change. */
    chk_store.chunk.scaled = scaled;
    H5D_BUILD_IO_INFO_RD(&chk_io_info, dset, dxpl_cache, dxpl_id, H5AC_rawdata_dxpl_id, &chk_store, NULL);

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
    udata.common.scaled = scaled;
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

    /* Determine if partial edge chunk filters are disabled */
    disable_edge_filters = (layout->u.chunk.flags
                & H5O_LAYOUT_CHUNK_DONT_FILTER_PARTIAL_BOUND_CHUNKS)
            && (idx_info.pline->nused > 0);

    /*
     * Determine the chunks which need to be filled or removed
     */
    HDmemset(min_mod_chunk_sc, 0, sizeof(min_mod_chunk_sc));
    HDmemset(max_mod_chunk_sc, 0, sizeof(max_mod_chunk_sc));
    for(op_dim = 0; op_dim < (unsigned)space_ndims; op_dim++) {
        /* Calculate the largest offset of chunks that might need to be
         * modified in this dimension */
        max_mod_chunk_sc[op_dim] = (old_dim[op_dim] - 1) / chunk_dim[op_dim];

        /* Calculate the largest offset of chunks that might need to be
         * filled in this dimension */
        if(0 == space_dim[op_dim])
            max_fill_chunk_sc[op_dim] = -1;
        else
            max_fill_chunk_sc[op_dim] = (hssize_t)(((MIN(space_dim[op_dim], old_dim[op_dim]) - 1)
                    / chunk_dim[op_dim]));

        if(shrunk_dim[op_dim]) {
            /* Calculate the smallest offset of chunks that might need to be
             * modified in this dimension.  Note that this array contains
             * garbage for all dimensions which are not shrunk.  These locations
             * must not be read from! */
            min_mod_chunk_sc[op_dim] = space_dim[op_dim] / chunk_dim[op_dim];

            /* Determine if we need to fill chunks in this dimension */
            if((hssize_t)min_mod_chunk_sc[op_dim] == max_fill_chunk_sc[op_dim]) {
                fill_dim[op_dim] = TRUE;

                /* If necessary, check if chunks in this dimension that need to
                 * be filled are new partial edge chunks */
                if(disable_edge_filters && old_dim[op_dim] >= (min_mod_chunk_sc[op_dim] + 1))
                    new_unfilt_dim[op_dim] = TRUE;
                else
                    new_unfilt_dim[op_dim] = FALSE;
            } /* end if */
            else {
                fill_dim[op_dim] = FALSE;
                new_unfilt_dim[op_dim] = FALSE;
            } /* end else */
        } /* end if */
        else {
            fill_dim[op_dim] = FALSE;
            new_unfilt_dim[op_dim] = FALSE;
        } /* end else */

        /* If necessary, calculate the smallest offset of non-previously full
         * chunks in this dimension, so we know these chunks were previously
         * unfiltered */
        if(disable_edge_filters)
            min_partial_chunk_sc[op_dim] = old_dim[op_dim] / chunk_dim[op_dim];
    } /* end for */

    /* Main loop: fill or remove chunks */
    for(op_dim = 0; op_dim < (unsigned)space_ndims; op_dim++) {
        hbool_t dims_outside_fill[H5O_LAYOUT_NDIMS]; /* Dimensions in chunk offset outside fill dimensions */
        int ndims_outside_fill;         /* Number of dimensions in chunk offset outside fill dimensions */
        hbool_t carry;                  /* Flag to indicate that chunk increment carrys to higher dimension (sorta) */

        /* Check if modification along this dimension is really necessary */
        if(!shrunk_dim[op_dim])
            continue;
        else {
            HDassert(max_mod_chunk_sc[op_dim] >= min_mod_chunk_sc[op_dim]);

            /* Reset the chunk offset indices */
            HDmemset(scaled, 0, (space_ndims * sizeof(scaled[0])));
            scaled[op_dim] = min_mod_chunk_sc[op_dim];

            /* Initialize "dims_outside_fill" array */
            ndims_outside_fill = 0;
            for(u = 0; u < space_ndims; u++)
                if((hssize_t)scaled[u] > max_fill_chunk_sc[u]) {
                    dims_outside_fill[u] = TRUE;
                    ndims_outside_fill++;
                } /* end if */
                else
                    dims_outside_fill[u] = FALSE;
	    } /* end if */

        carry = FALSE;
        while(!carry) {
            int i;	                        /* Local index variable */

            udata.common.scaled = scaled;

            if(0 == ndims_outside_fill) {
                HDassert(fill_dim[op_dim]);
                HDassert(scaled[op_dim] == min_mod_chunk_sc[op_dim]);

                /* Make sure this is an edge chunk */
                HDassert(H5D__chunk_is_partial_edge_chunk(space_ndims, layout->u.chunk.dim, scaled, space_dim));

                /* Determine if the chunk just became an unfiltered chunk */
                if(new_unfilt_dim[op_dim]) {
                    new_unfilt_chunk = TRUE;
                    for(u = 0; u < space_ndims; u++)
                        if(scaled[u] == min_partial_chunk_sc[u]) {
                            new_unfilt_chunk = FALSE;
                            break;
                        } /* end if */
                } /* end if */

                /* Make sure that, if we think this is a new unfiltered chunk,
                 * it was previously not an edge chunk */
                HDassert(!new_unfilt_dim[op_dim] || (!new_unfilt_chunk !=
                        !H5D__chunk_is_partial_edge_chunk(space_ndims, layout->u.chunk.dim, scaled, old_dim)));
                HDassert(!new_unfilt_chunk || new_unfilt_dim[op_dim]);

                /* Fill the unused parts of the chunk */
                if(H5D__chunk_prune_fill(&udata, new_unfilt_chunk) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to write fill value")
            } /* end if */
            else {
                H5D_chunk_ud_t          chk_udata;          /* User data for getting chunk info */

#ifndef NDEBUG
                /* Make sure this chunk is really outside the new dimensions */
                {
                    hbool_t outside_dim = FALSE;

                    for(u = 0; u < space_ndims; u++)
                        if((scaled[u] * chunk_dim[u]) >= space_dim[u]) {
                            outside_dim = TRUE;
                            break;
                        } /* end if */
                    HDassert(outside_dim);
                } /* end block */
#endif /* NDEBUG */

                /* Check if the chunk exists in cache or on disk */
                if(H5D__chunk_lookup(dset, dxpl_id, scaled, &chk_udata) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "error looking up chunk")

                /* Evict the entry from the cache if present, but do not flush
                 * it to disk */
                if(UINT_MAX != chk_udata.idx_hint)
                    if(H5D__chunk_cache_evict(dset, dxpl_id, dxpl_cache, rdcc->slot[chk_udata.idx_hint], FALSE) < 0)
                        HGOTO_ERROR(H5E_DATASET, H5E_CANTREMOVE, FAIL, "unable to evict chunk")

                /* Remove the chunk from disk, if present */
                if(H5F_addr_defined(chk_udata.chunk_block.offset)) {
                    /* Update the offset in idx_udata */
		    idx_udata.scaled = udata.common.scaled;

                    /* Remove the chunk from disk */
                    if((layout->storage.u.chunk.ops->remove)(&idx_info, &idx_udata) < 0)
                        HGOTO_ERROR(H5E_DATASET, H5E_CANTDELETE, FAIL, "unable to remove chunk entry from index")
                } /* end if */
            } /* end else */

            /* Increment indices */
            carry = TRUE;
            for(i = (int)(space_ndims - 1); i >= 0; --i) {
                scaled[i]++;
                if(scaled[i] > max_mod_chunk_sc[i]) {
                    /* Left maximum dimensions, "wrap around" and check if this
                     * dimension is no longer outside the fill dimension */
                    if((unsigned)i == op_dim) {
                        scaled[i] = min_mod_chunk_sc[i];
                        if(dims_outside_fill[i] && fill_dim[i]) {
                            dims_outside_fill[i] = FALSE;
                            ndims_outside_fill--;
                        } /* end if */
                    } /* end if */
                    else {
                        scaled[i] = 0;
                        if(dims_outside_fill[i] && max_fill_chunk_sc[i] >= 0) {
                            dims_outside_fill[i] = FALSE;
                            ndims_outside_fill--;
                        } /* end if */
                    } /* end else */
                } /* end if */
                else {
                    /* Check if we just went outside the fill dimension */
                    if(!dims_outside_fill[i] && (hssize_t)scaled[i] > max_fill_chunk_sc[i]) {
                        dims_outside_fill[i] = TRUE;
                        ndims_outside_fill++;
                    } /* end if */

                    /* We found the next chunk, so leave the loop */
                    carry = FALSE;
                    break;
                } /* end else */
            } /* end for */
        } /* end while(!carry) */

        /* Adjust max_mod_chunk_sc so we don't modify the same chunk twice.
         * Also check if this dimension started from 0 (and hence removed all
         * of the chunks). */
        if(min_mod_chunk_sc[op_dim] == 0)
            break;
        else
            max_mod_chunk_sc[op_dim] = min_mod_chunk_sc[op_dim] - 1;
    } /* end for(op_dim=0...) */

    /* Reset any cached chunk info for this dataset */
    H5D__chunk_cinfo_cache_reset(&dset->shared->cache.chunk.last);

done:
    /* Release resources */
    if(chunk_space && H5S_close(chunk_space) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataspace")
    if(udata_init)
        if(udata.fb_info_init && H5D__fill_term(&udata.fb_info) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't release fill buffer info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_prune_by_extent() */

#ifdef H5_HAVE_PARALLEL

/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_addrmap_cb
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
H5D__chunk_addrmap_cb(const H5D_chunk_rec_t *chunk_rec, void *_udata)
{
    H5D_chunk_it_ud2_t	*udata = (H5D_chunk_it_ud2_t *)_udata;  /* User data for callback */
    unsigned       rank = udata->common.layout->ndims - 1;    /* # of dimensions of dataset */
    hsize_t        chunk_index;
    int            ret_value = H5_ITER_CONT;     /* Return value */

    FUNC_ENTER_STATIC

    /* Compute the index for this chunk */
    chunk_index = H5VM_array_offset_pre(rank, udata->common.layout->down_chunks, chunk_rec->scaled);

    /* Set it in the userdata to return */
    udata->chunk_addr[chunk_index] = chunk_rec->chunk_addr;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D__chunk_addrmap_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_addrmap
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
H5D__chunk_addrmap(const H5D_io_info_t *io_info, haddr_t chunk_addr[])
{
    H5D_chk_idx_info_t idx_info;        /* Chunked index info */
    const H5D_t *dset = io_info->dset;  /* Local pointer to dataset info */
    H5D_chunk_it_ud2_t udata;          	/* User data for iteration callback */
    H5O_storage_chunk_t *sc = &(dset->shared->layout.storage.u.chunk);
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_PACKAGE

    HDassert(dset);
    HDassert(dset->shared);
    H5D_CHUNK_STORAGE_INDEX_CHK(sc);
    HDassert(chunk_addr);

    /* Set up user data for B-tree callback */
    HDmemset(&udata, 0, sizeof(udata));
    udata.common.layout = &dset->shared->layout.u.chunk;
    udata.common.storage = &dset->shared->layout.storage.u.chunk;
    udata.chunk_addr = chunk_addr;

    /* Compose chunked index info struct */
    idx_info.f = dset->oloc.file;
    idx_info.dxpl_id = io_info->md_dxpl_id;
    idx_info.pline = &dset->shared->dcpl_cache.pline;
    idx_info.layout = &dset->shared->layout.u.chunk;
    idx_info.storage = &dset->shared->layout.storage.u.chunk;

    /* Iterate over chunks to build mapping of chunk addresses */
    if((dset->shared->layout.storage.u.chunk.ops->iterate)(&idx_info, H5D__chunk_addrmap_cb, &udata) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to iterate over chunk index to build address map")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_addrmap() */
#endif /* H5_HAVE_PARALLEL */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_delete
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
H5D__chunk_delete(H5F_t *f, hid_t dxpl_id, H5O_t *oh, H5O_storage_t *storage)
{
    H5D_chk_idx_info_t idx_info;        /* Chunked index info */
    H5O_layout_t layout;                /* Dataset layout  message */
    hbool_t layout_read = FALSE;        /* Whether the layout message was read from the file */
    H5O_pline_t pline;                  /* I/O pipeline message */
    hbool_t pline_read = FALSE;         /* Whether the I/O pipeline message was read from the file */
    htri_t	exists;                 /* Flag if header message of interest exists */
    H5O_storage_chunk_t *sc = &(storage->u.chunk);
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    HDassert(f);
    HDassert(oh);
    HDassert(storage);
    H5D_CHUNK_STORAGE_INDEX_CHK(sc);

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
} /* end H5D__chunk_delete() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_update_cache
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
H5D__chunk_update_cache(H5D_t *dset, hid_t dxpl_id)
{
    H5D_rdcc_t         *rdcc = &(dset->shared->cache.chunk);	/*raw data chunk cache */
    H5D_rdcc_ent_t     *ent, *next;	/*cache entry  */
    H5D_rdcc_ent_t     tmp_head;        /* Sentinel entry for temporary entry list */
    H5D_rdcc_ent_t     *tmp_tail;       /* Tail pointer for temporary entry list */
    H5D_dxpl_cache_t _dxpl_cache;       /* Data transfer property cache buffer */
    H5D_dxpl_cache_t *dxpl_cache = &_dxpl_cache;   /* Data transfer property cache */
    herr_t              ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(dset && H5D_CHUNKED == dset->shared->layout.type);
    HDassert(dset->shared->layout.u.chunk.ndims > 0 && dset->shared->layout.u.chunk.ndims <= H5O_LAYOUT_NDIMS);

    /* Check the rank */
    HDassert((dset->shared->layout.u.chunk.ndims - 1) > 1);

    /* Fill the DXPL cache values for later use */
    if(H5D__get_dxpl_cache(dxpl_id, &dxpl_cache) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't fill dxpl cache")

    /* Add temporary entry list to rdcc */
    (void)HDmemset(&tmp_head, 0, sizeof(tmp_head));
    rdcc->tmp_head = &tmp_head;
    tmp_tail = &tmp_head;

    /* Recompute the index for each cached chunk that is in a dataset */
    for(ent = rdcc->head; ent; ent = next) {
        unsigned	    old_idx;	/* Previous index number	*/

        /* Get the pointer to the next cache entry */
        next = ent->next;

        /* Compute the index for the chunk entry */
        old_idx = ent->idx;   /* Save for later */
        ent->idx = H5D__chunk_hash_val(dset->shared, ent->scaled);

        if(old_idx != ent->idx) {
            H5D_rdcc_ent_t     *old_ent;	/* Old cache entry  */

            /* Check if there is already a chunk at this chunk's new location */
            old_ent = rdcc->slot[ent->idx];
            if(old_ent != NULL) {
                HDassert(old_ent->locked == FALSE);
                HDassert(old_ent->deleted == FALSE);

                /* Insert the old entry into the temporary list, but do not
                 * evict (yet).  Make sure we do not make any calls to the index
                 * until all chunks have updated indices! */
                HDassert(!old_ent->tmp_next);
                HDassert(!old_ent->tmp_prev);
                tmp_tail->tmp_next = old_ent;
                old_ent->tmp_prev = tmp_tail;
                tmp_tail = old_ent;
            } /* end if */

            /* Insert this chunk into correct location in hash table */
            rdcc->slot[ent->idx] = ent;

            /* If this chunk was previously on the temporary list and therefore
             * not in the hash table, remove it from the temporary list.
             * Otherwise clear the old hash table slot. */
            if(ent->tmp_prev) {
                HDassert(tmp_head.tmp_next);
                HDassert(tmp_tail != &tmp_head);
                ent->tmp_prev->tmp_next = ent->tmp_next;
                if(ent->tmp_next) {
                    ent->tmp_next->tmp_prev = ent->tmp_prev;
                    ent->tmp_next = NULL;
                } /* end if */
                else {
                    HDassert(tmp_tail == ent);
                    tmp_tail = ent->tmp_prev;
                } /* end else */
                ent->tmp_prev = NULL;
            } /* end if */
            else
                rdcc->slot[old_idx] = NULL;
        } /* end if */
    } /* end for */

    /* tmp_tail is no longer needed, and will be invalidated by
     * H5D_chunk_cache_evict anyways. */
    tmp_tail = NULL;

    /* Evict chunks that are still on the temporary list */
    while(tmp_head.tmp_next) {
        ent = tmp_head.tmp_next;

        /* Remove the old entry from the cache */
        if(H5D__chunk_cache_evict(dset, dxpl_id, dxpl_cache, ent, TRUE) < 0)
            HGOTO_ERROR(H5E_IO, H5E_CANTFLUSH, FAIL, "unable to flush one or more raw data chunks")
    } /* end while */

done:
    /* Remove temporary list from rdcc */
    rdcc->tmp_head = NULL;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_update_cache() */


/*-------------------------------------------------------------------------
 * Function:    H5D__chunk_copy_cb
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
H5D__chunk_copy_cb(const H5D_chunk_rec_t *chunk_rec, void *_udata)
{
    H5D_chunk_it_ud3_t      *udata = (H5D_chunk_it_ud3_t *)_udata;       /* User data for callback */
    H5D_chunk_ud_t          udata_dst;                  /* User data about new destination chunk */
    hbool_t                 is_vlen = FALSE;            /* Whether datatype is variable-length */
    hbool_t                 fix_ref = FALSE;            /* Whether to fix up references in the dest. file */
    hbool_t                 need_insert = FALSE;    /* Whether the chunk needs to be inserted into the index */

    /* General information about chunk copy */
    void                    *bkg = udata->bkg;          /* Background buffer for datatype conversion */
    void                    *buf = udata->buf;          /* Chunk buffer for I/O & datatype conversions */
    size_t                  buf_size = udata->buf_size; /* Size of chunk buffer */
    const H5O_pline_t       *pline = udata->pline;      /* I/O pipeline for applying filters */

    /* needed for commpressed variable length data */
    hbool_t                 must_filter = FALSE;        /* Whether chunk must be filtered during copy */
    size_t                  nbytes;                     /* Size of chunk in file (in bytes) */
    H5Z_cb_t                cb_struct;                  /* Filter failure callback struct */
    int                     ret_value = H5_ITER_CONT;   /* Return value */

    FUNC_ENTER_STATIC

    /* Get 'size_t' local value for number of bytes in chunk */
    H5_CHECKED_ASSIGN(nbytes, size_t, chunk_rec->nbytes, uint32_t);

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
    if((is_vlen || fix_ref) && pline && pline->nused) {
        /* Check if we should disable filters on this chunk */
        if(udata->common.layout->flags
                & H5O_LAYOUT_CHUNK_DONT_FILTER_PARTIAL_BOUND_CHUNKS) {
            /* Check if the chunk is an edge chunk, and disable filters if so */
            if(!H5D__chunk_is_partial_edge_chunk(udata->dset_ndims, udata->common.layout->dim, chunk_rec->scaled, udata->dset_dims))
                must_filter = TRUE;
        } /* end if */
        else
            must_filter = TRUE;
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
    if(H5F_block_read(udata->file_src, H5FD_MEM_DRAW, chunk_rec->chunk_addr, nbytes, H5AC_rawdata_dxpl_id, buf) < 0)
        HGOTO_ERROR(H5E_IO, H5E_READERROR, H5_ITER_ERROR, "unable to read raw data chunk")

    /* Need to uncompress variable-length & reference data elements */
    if(must_filter) {
        unsigned filter_mask = chunk_rec->filter_mask;

        cb_struct.func = NULL; /* no callback function when failed */
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
        if(H5D_vlen_reclaim(tid_mem, buf_space, udata->idx_info_dst->dxpl_id, reclaim_buf) < 0)
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
    udata_dst.common.scaled = chunk_rec->scaled;
    udata_dst.chunk_block.offset = HADDR_UNDEF;
    udata_dst.chunk_block.length = chunk_rec->nbytes;
    udata_dst.filter_mask = chunk_rec->filter_mask;

    /* Need to compress variable-length & reference data elements before writing to file */
    if(must_filter) {
        if(H5Z_pipeline(pline, 0, &(udata_dst.filter_mask), H5Z_NO_EDC, cb_struct, &nbytes, &buf_size, &buf) < 0)
            HGOTO_ERROR(H5E_PLINE, H5E_CANTFILTER, H5_ITER_ERROR, "output pipeline failed")
#if H5_SIZEOF_SIZE_T > 4
        /* Check for the chunk expanding too much to encode in a 32-bit value */
        if(nbytes > ((size_t)0xffffffff))
            HGOTO_ERROR(H5E_DATASET, H5E_BADRANGE, H5_ITER_ERROR, "chunk too large for 32-bit length")
#endif /* H5_SIZEOF_SIZE_T > 4 */
        H5_CHECKED_ASSIGN(udata_dst.chunk_block.length, uint32_t, nbytes, size_t);
	udata->buf = buf;
	udata->buf_size = buf_size;
    } /* end if */

    udata_dst.chunk_idx = H5VM_array_offset_pre(udata_dst.common.layout->ndims - 1, 
			    udata_dst.common.layout->max_down_chunks, udata_dst.common.scaled);

    /* Allocate chunk in the file */
    if(H5D__chunk_file_alloc(udata->idx_info_dst, NULL, &udata_dst.chunk_block, &need_insert, udata_dst.common.scaled) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, FAIL, "unable to insert/resize chunk on chunk level")

    /* Write chunk data to destination file */
    HDassert(H5F_addr_defined(udata_dst.chunk_block.offset));
    if(H5F_block_write(udata->idx_info_dst->f, H5FD_MEM_DRAW, udata_dst.chunk_block.offset, nbytes, H5AC_rawdata_dxpl_id, buf) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, H5_ITER_ERROR, "unable to write raw data to file")

    /* Set metadata tag in dxpl_id */
    H5_BEGIN_TAG(udata->idx_info_dst->dxpl_id, H5AC__COPIED_TAG, H5_ITER_ERROR);

    /* Insert chunk record into index */
    if(need_insert && udata->idx_info_dst->storage->ops->insert)
        if((udata->idx_info_dst->storage->ops->insert)(udata->idx_info_dst, &udata_dst, NULL) < 0)
            HGOTO_ERROR_TAG(H5E_DATASET, H5E_CANTINSERT, H5_ITER_ERROR, "unable to insert chunk addr into index")

    /* Reset metadata tag in dxpl_id */
    H5_END_TAG(H5_ITER_ERROR);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_copy_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_copy
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
H5D__chunk_copy(H5F_t *f_src, H5O_storage_chunk_t *storage_src,
    H5O_layout_chunk_t *layout_src, H5F_t *f_dst, H5O_storage_chunk_t *storage_dst,
    const H5S_extent_t *ds_extent_src, const H5T_t *dt_src,
    const H5O_pline_t *pline_src, H5O_copy_t *cpy_info, hid_t dxpl_id)
{
    H5D_chunk_it_ud3_t udata;           /* User data for iteration callback */
    H5D_chk_idx_info_t idx_info_dst;    /* Dest. chunked index info */
    H5D_chk_idx_info_t idx_info_src;    /* Source chunked index info */
    int         sndims;                 /* Rank of dataspace */
    hsize_t     curr_dims[H5O_LAYOUT_NDIMS]; /* Curr. size of dataset dimensions */
    hsize_t     max_dims[H5O_LAYOUT_NDIMS]; /* Curr. size of dataset dimensions */
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

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(f_src);
    HDassert(storage_src);
    H5D_CHUNK_STORAGE_INDEX_CHK(storage_src);
    HDassert(layout_src);
    HDassert(f_dst);
    HDassert(storage_dst);
    H5D_CHUNK_STORAGE_INDEX_CHK(storage_dst);
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
        unsigned    ndims;                  /* Rank of dataspace */

        /* Get the dim info for dataset */
        if((sndims = H5S_extent_get_dims(ds_extent_src, curr_dims, max_dims)) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get dataspace dimensions")
        H5_CHECKED_ASSIGN(ndims, unsigned, sndims, int);

        /* Set the source layout chunk information */
        if(H5D__chunk_set_info_real(layout_src, ndims, curr_dims, max_dims) < 0)
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

        H5_CHECKED_ASSIGN(buf_size, size_t, layout_src->size, uint32_t);
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
    udata.dset_ndims = (unsigned)sndims;
    udata.dset_dims = curr_dims;
    udata.cpy_info = cpy_info;

    /* Iterate over chunks to copy data */
    if((storage_src->ops->iterate)(&idx_info_src, H5D__chunk_copy_cb, &udata) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_BADITER, FAIL, "unable to iterate over chunk index to copy data")

    /* I/O buffers may have been re-allocated */
    buf = udata.buf;
    bkg = udata.bkg;

done:
    if(sid_buf > 0 && H5I_dec_ref(sid_buf) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "can't decrement temporary dataspace ID")
    if(tid_src > 0 && H5I_dec_ref(tid_src) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't decrement temporary datatype ID")
    if(tid_dst > 0 && H5I_dec_ref(tid_dst) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't decrement temporary datatype ID")
    if(tid_mem > 0 && H5I_dec_ref(tid_mem) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't decrement temporary datatype ID")
    if(buf)
        H5MM_xfree(buf);
    if(bkg)
        H5MM_xfree(bkg);
    if(reclaim_buf)
        H5MM_xfree(reclaim_buf);

    /* Clean up any index information */
    if(copy_setup_done)
        if(storage_src->ops->copy_shutdown && (storage_src->ops->copy_shutdown)(storage_src, storage_dst, dxpl_id) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "unable to shut down index copying info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_copy() */


/*-------------------------------------------------------------------------
 * Function:    H5D__chunk_bh_info
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
H5D__chunk_bh_info(const H5O_loc_t *loc, hid_t dxpl_id, H5O_t *oh, H5O_layout_t *layout,
    hsize_t *index_size)
{
    H5D_chk_idx_info_t idx_info;        /* Chunked index info */
    H5S_t *space = NULL;                /* Dataset's dataspace */
    H5O_pline_t pline;                  /* I/O pipeline message */
    H5O_storage_chunk_t *sc = &(layout->storage.u.chunk);
    htri_t exists;                      /* Flag if header message of interest exists */
    hbool_t idx_info_init = FALSE;      /* Whether the chunk index info has been initialized */
    hbool_t pline_read = FALSE;         /* Whether the I/O pipeline message was read */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(loc);
    HDassert(loc->file);
    HDassert(H5F_addr_defined(loc->addr));
    HDassert(layout);
    H5D_CHUNK_STORAGE_INDEX_CHK(sc);
    HDassert(index_size);

    /* Check for I/O pipeline message */
    if((exists = H5O_msg_exists_oh(oh, H5O_PLINE_ID)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to read object header")
    else if(exists) {
        if(NULL == H5O_msg_read_oh(loc->file, dxpl_id, oh, H5O_PLINE_ID, &pline))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't find I/O pipeline message")
        pline_read = TRUE;
    } /* end else if */
    else
        HDmemset(&pline, 0, sizeof(pline));

    /* Compose chunked index info struct */
    idx_info.f = loc->file;
    idx_info.dxpl_id = dxpl_id;
    idx_info.pline = &pline;
    idx_info.layout = &layout->u.chunk;
    idx_info.storage = &layout->storage.u.chunk;

    /* Get the dataspace for the dataset */
    if(NULL == (space = H5S_read(loc, dxpl_id)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to load dataspace info from dataset header")

    /* Allocate any indexing structures */
    if(layout->storage.u.chunk.ops->init && (layout->storage.u.chunk.ops->init)(&idx_info, space, loc->addr) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't initialize indexing information")
    idx_info_init = TRUE;

    /* Get size of index structure */
    if(layout->storage.u.chunk.ops->size && (layout->storage.u.chunk.ops->size)(&idx_info, index_size) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to retrieve chunk index info")

done:
    /* Free resources, if they've been initialized */
    if(idx_info_init && layout->storage.u.chunk.ops->dest &&
            (layout->storage.u.chunk.ops->dest)(&idx_info) < 0)
	HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "unable to release chunk index info")
    if(pline_read && H5O_msg_reset(H5O_PLINE_ID, &pline) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTRESET, FAIL, "unable to reset I/O pipeline message")
    if(space && H5S_close(space) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataspace")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_bh_info() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_dump_index_cb
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
static int
H5D__chunk_dump_index_cb(const H5D_chunk_rec_t *chunk_rec, void *_udata)
{
    H5D_chunk_it_ud4_t	*udata = (H5D_chunk_it_ud4_t *)_udata;  /* User data from caller */

    FUNC_ENTER_STATIC_NOERR

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
            HDfprintf(udata->stream, "%s%Hu", (u ? ", " : ""), (chunk_rec->scaled[u] * udata->chunk_dim[u]));
        HDfputs("]\n", udata->stream);
    } /* end if */

    FUNC_LEAVE_NOAPI(H5_ITER_CONT)
} /* H5D__chunk_dump_index_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_dump_index
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
H5D__chunk_dump_index(H5D_t *dset, hid_t dxpl_id, FILE *stream)
{
    H5O_storage_chunk_t *sc = &(dset->shared->layout.storage.u.chunk);
    herr_t ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    HDassert(dset);
    H5D_CHUNK_STORAGE_INDEX_CHK(sc);

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
        udata.chunk_dim = dset->shared->layout.u.chunk.dim;

        /* Iterate over index and dump chunk info */
        if((dset->shared->layout.storage.u.chunk.ops->iterate)(&idx_info, H5D__chunk_dump_index_cb, &udata) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_BADITER, FAIL, "unable to iterate over chunk index to dump chunk info")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_dump_index() */

#ifdef H5D_CHUNK_DEBUG

/*-------------------------------------------------------------------------
 * Function:	H5D__chunk_stats
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
H5D__chunk_stats(const H5D_t *dset, hbool_t headers)
{
    H5D_rdcc_t	*rdcc = &(dset->shared->cache.chunk);
    double	miss_rate;
    char	ascii[32];
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_PACKAGE_NOERR

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
        if (rdcc->stats.nhits>0 || rdcc->stats.nmisses>0) {
            miss_rate = 100.0 * rdcc->stats.nmisses /
                    (rdcc->stats.nhits + rdcc->stats.nmisses);
        } else {
            miss_rate = 0.0;
        }
        if (miss_rate > 100) {
            sprintf(ascii, "%7d%%", (int) (miss_rate + 0.5));
        } else {
            sprintf(ascii, "%7.2f%%", miss_rate);
        }

        fprintf(H5DEBUG(AC), "   %-18s %8u %8u %7s %8d+%-9ld\n",
            "raw data chunks", rdcc->stats.nhits, rdcc->stats.nmisses, ascii,
            rdcc->stats.ninits, (long)(rdcc->stats.nflushes)-(long)(rdcc->stats.ninits));
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_stats() */
#endif /* H5D_CHUNK_DEBUG */


/*-------------------------------------------------------------------------
 * Function:	H5D__nonexistent_readvv_cb
 *
 * Purpose:	Callback operation for performing fill value I/O operation
 *              on memory buffer.
 *
 * Note:	This algorithm is pretty inefficient about initializing and
 *              terminating the fill buffer info structure and it would be
 *              faster to refactor this into a "real" initialization routine,
 *              and a "vectorized fill" routine. -QAK
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		30 Sep 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__nonexistent_readvv_cb(hsize_t H5_ATTR_UNUSED dst_off, hsize_t src_off, size_t len,
    void *_udata)
{
    H5D_chunk_readvv_ud_t *udata = (H5D_chunk_readvv_ud_t *)_udata; /* User data for H5VM_opvv() operator */
    H5D_fill_buf_info_t fb_info;    /* Dataset's fill buffer info */
    hbool_t fb_info_init = FALSE;   /* Whether the fill value buffer has been initialized */
    herr_t ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_STATIC

    /* Initialize the fill value buffer */
    if(H5D__fill_init(&fb_info, (udata->rbuf + src_off), NULL, NULL, NULL, NULL,
            &udata->dset->shared->dcpl_cache.fill, udata->dset->shared->type,
            udata->dset->shared->type_id, (size_t)0, len, udata->dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't initialize fill buffer info")
    fb_info_init = TRUE;

    /* Check for VL datatype & fill the buffer with VL datatype fill values */
    if(fb_info.has_vlen_fill_type && H5D__fill_refill_vl(&fb_info, fb_info.elmts_per_buf, udata->dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, FAIL, "can't refill fill value buffer")

done:
    /* Release the fill buffer info, if it's been initialized */
    if(fb_info_init && H5D__fill_term(&fb_info) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't release fill buffer info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D__nonexistent_readvv_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D__nonexistent_readvv
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
H5D__nonexistent_readvv(const H5D_io_info_t *io_info,
    size_t chunk_max_nseq, size_t *chunk_curr_seq, size_t chunk_len_arr[], hsize_t chunk_off_arr[],
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_len_arr[], hsize_t mem_off_arr[])
{
    H5D_chunk_readvv_ud_t udata;        /* User data for H5VM_opvv() operator */
    ssize_t ret_value = -1;             /* Return value */

    FUNC_ENTER_STATIC

    /* Check args */
    HDassert(io_info);
    HDassert(chunk_curr_seq);
    HDassert(chunk_len_arr);
    HDassert(chunk_off_arr);
    HDassert(mem_curr_seq);
    HDassert(mem_len_arr);
    HDassert(mem_off_arr);

    /* Set up user data for H5VM_opvv() */
    udata.rbuf = (unsigned char *)io_info->u.rbuf;
    udata.dset = io_info->dset;
    udata.dxpl_id = io_info->md_dxpl_id;

    /* Call generic sequence operation routine */
    if((ret_value = H5VM_opvv(chunk_max_nseq, chunk_curr_seq, chunk_len_arr, chunk_off_arr,
            mem_max_nseq, mem_curr_seq, mem_len_arr, mem_off_arr,
            H5D__nonexistent_readvv_cb, &udata)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTOPERATE, FAIL, "can't perform vectorized fill value init")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D__nonexistent_readvv() */


/*-------------------------------------------------------------------------
 * Function:    H5D__chunk_is_partial_edge_chunk
 *
 * Purpose:     Checks to see if the chunk is a partial edge chunk.
 *              Either dset or (dset_dims and dset_ndims) must be
 *              provided.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Neil Fortner
 *              19 Nov 2009
 *
 *-------------------------------------------------------------------------
 */
static hbool_t
H5D__chunk_is_partial_edge_chunk(unsigned dset_ndims, const uint32_t *chunk_dims,
    const hsize_t scaled[], const hsize_t *dset_dims)
{
    unsigned    u;                      /* Local index variable */
    hbool_t      ret_value = FALSE;     /* Return value */

    FUNC_ENTER_STATIC_NOERR

    /* Check args */
    HDassert(scaled);
    HDassert(dset_ndims > 0);
    HDassert(dset_dims);
    HDassert(chunk_dims);

    /* check if this is a partial edge chunk */
    for(u = 0; u < dset_ndims; u++)
        if(((scaled[u] + 1) * chunk_dims[u]) > dset_dims[u])
            HGOTO_DONE(TRUE);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D__chunk_is_partial_edge_chunk() */


/*-------------------------------------------------------------------------
 * Function:    H5D__chunk_file_alloc()
 *
 * Purpose:     Chunk allocation:  
 *		  Create the chunk if it doesn't exist, or reallocate the
 *                chunk if its size changed.
 *		  The coding is moved and modified from each index structure.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Vailin Choi; June 2014
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__chunk_file_alloc(const H5D_chk_idx_info_t *idx_info, const H5F_block_t *old_chunk,
    H5F_block_t *new_chunk, hbool_t *need_insert, hsize_t scaled[])
{
    hbool_t alloc_chunk = FALSE;	/* Whether to allocate chunk */
    herr_t ret_value = SUCCEED;   	/* Return value         */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(idx_info);
    HDassert(idx_info->f);
    HDassert(idx_info->pline);
    HDassert(idx_info->layout);
    HDassert(idx_info->storage);
    HDassert(new_chunk);
    HDassert(need_insert);

    *need_insert = FALSE;

    /* Check for filters on chunks */
    if(idx_info->pline->nused > 0) {
        /* Sanity/error checking block */
	HDassert(idx_info->storage->idx_type != H5D_CHUNK_IDX_NONE);
        {
            unsigned allow_chunk_size_len;          /* Allowed size of encoded chunk size */
            unsigned new_chunk_size_len;            /* Size of encoded chunk size */

            /* Compute the size required for encoding the size of a chunk, allowing
             * for an extra byte, in case the filter makes the chunk larger.
             */
            allow_chunk_size_len = 1 + ((H5VM_log2_gen((uint64_t)(idx_info->layout->size)) + 8) / 8);
            if(allow_chunk_size_len > 8)
                allow_chunk_size_len = 8;

            /* Compute encoded size of chunk */
            new_chunk_size_len = (H5VM_log2_gen((uint64_t)(new_chunk->length)) + 8) / 8;
            if(new_chunk_size_len > 8)
                HGOTO_ERROR(H5E_DATASET, H5E_BADRANGE, FAIL, "encoded chunk size is more than 8 bytes?!?")

            /* Check if the chunk became too large to be encoded */
            if(new_chunk_size_len > allow_chunk_size_len)
                HGOTO_ERROR(H5E_DATASET, H5E_BADRANGE, FAIL, "chunk size can't be encoded")
        } /* end block */

	if(old_chunk && H5F_addr_defined(old_chunk->offset)) {
	    /* Sanity check */
            HDassert(!H5F_addr_defined(new_chunk->offset) || H5F_addr_eq(new_chunk->offset, old_chunk->offset));

            /* Check for chunk being same size */
	    if(new_chunk->length != old_chunk->length) {
		/* Release previous chunk */
		/* Only free the old location if not doing SWMR writes - otherwise
                 * we must keep the old chunk around in case a reader has an
                 * outdated version of the B-tree node
                 */
		if(!(H5F_INTENT(idx_info->f) & H5F_ACC_SWMR_WRITE))
		    if(H5MF_xfree(idx_info->f, H5FD_MEM_DRAW, idx_info->dxpl_id, old_chunk->offset, old_chunk->length) < 0)
			HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "unable to free chunk")
		alloc_chunk = TRUE;
	    } /* end if */
            else {
		/* Don't need to reallocate chunk, but send its address back up */
                if(!H5F_addr_defined(new_chunk->offset))
                    new_chunk->offset = old_chunk->offset;
	    } /* end else */
	} /* end if */
        else {
            HDassert(!H5F_addr_defined(new_chunk->offset));
	    alloc_chunk = TRUE;
        } /* end else */
    } /* end if */
    else {
	HDassert(!H5F_addr_defined(new_chunk->offset));
	HDassert(new_chunk->length == idx_info->layout->size);
	alloc_chunk = TRUE;
    }  /* end else */

    /* Actually allocate space for the chunk in the file */
    if(alloc_chunk) {
        switch(idx_info->storage->idx_type) {
            case H5D_CHUNK_IDX_NONE:
            {
                H5D_chunk_ud_t udata;

                udata.common.scaled = scaled;
                if((idx_info->storage->ops->get_addr)(idx_info, &udata) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't query chunk address")
                new_chunk->offset = udata.chunk_block.offset;
                HDassert(new_chunk->length == udata.chunk_block.length);
                break;
            }

            case H5D_CHUNK_IDX_EARRAY:
            case H5D_CHUNK_IDX_FARRAY:
            case H5D_CHUNK_IDX_BT2:
            case H5D_CHUNK_IDX_BTREE:
            case H5D_CHUNK_IDX_SINGLE:
                HDassert(new_chunk->length > 0);
                H5_CHECK_OVERFLOW(new_chunk->length, /*From: */uint32_t, /*To: */hsize_t);
                new_chunk->offset = H5MF_alloc(idx_info->f, H5FD_MEM_DRAW, idx_info->dxpl_id, (hsize_t)new_chunk->length);
                if(!H5F_addr_defined(new_chunk->offset))
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "file allocation failed")
                *need_insert = TRUE;
                break;

            case H5D_CHUNK_IDX_NTYPES:
            default:
                HDassert(0 && "This should never be executed!");
                break;
        } /* end switch */
    } /* end if */

    HDassert(H5F_addr_defined(new_chunk->offset));

done: 
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D__chunk_file_alloc() */


/*-------------------------------------------------------------------------
 * Function:    H5D__chunk_format_convert_cb
 *
 * Purpose:     Callback routine to insert chunk address into v1 B-tree
 *              chunk index.
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Vailin Choi
 *              Feb 2015
 *
 *-------------------------------------------------------------------------
 */
static int
H5D__chunk_format_convert_cb(const H5D_chunk_rec_t *chunk_rec, void *_udata)
{
    H5D_chunk_it_ud5_t *udata = (H5D_chunk_it_ud5_t *)_udata;	/* User data */
    H5D_chk_idx_info_t *new_idx_info;  	/* The new chunk index information */
    H5D_chunk_ud_t  insert_udata;	/* Chunk information to be inserted */
    haddr_t chunk_addr;			/* Chunk address */
    size_t nbytes;			/* Chunk size */
    void *buf = NULL;			/* Pointer to buffer of chunk data */
    int  ret_value = H5_ITER_CONT;   	/* Return value */

    FUNC_ENTER_STATIC

    /* Set up */
    new_idx_info = udata->new_idx_info;
    H5_CHECKED_ASSIGN(nbytes, size_t, chunk_rec->nbytes, uint32_t);
    chunk_addr = chunk_rec->chunk_addr;

    if(new_idx_info->pline->nused &&
          (new_idx_info->layout->flags & H5O_LAYOUT_CHUNK_DONT_FILTER_PARTIAL_BOUND_CHUNKS) &&
          (H5D__chunk_is_partial_edge_chunk(udata->dset_ndims, new_idx_info->layout->dim, chunk_rec->scaled, udata->dset_dims))) {
	/* This is a partial non-filtered edge chunk */
	/* Convert the chunk to a filtered edge chunk for v1 B-tree chunk index */

	unsigned filter_mask = chunk_rec->filter_mask;
	H5Z_cb_t  cb_struct;        	/* Filter failure callback struct */
	size_t read_size = nbytes;	/* Bytes to read */

	HDassert(read_size == new_idx_info->layout->size);

	cb_struct.func = NULL; /* no callback function when failed */

	/* Allocate buffer for chunk data */
	if(NULL == (buf = H5MM_malloc(read_size)))
	    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, H5_ITER_ERROR, "memory allocation failed for raw data chunk")

	/* Read the non-filtered edge chunk */
        if(H5F_block_read(new_idx_info->f, H5FD_MEM_DRAW, chunk_addr, read_size, H5AC_rawdata_dxpl_id, buf) < 0)
	    HGOTO_ERROR(H5E_IO, H5E_READERROR, H5_ITER_ERROR, "unable to read raw data chunk")

	/* Pass the chunk through the pipeline */
	if(H5Z_pipeline(new_idx_info->pline, 0, &filter_mask, H5Z_NO_EDC, cb_struct, &nbytes, &read_size, &buf) < 0)
	    HGOTO_ERROR(H5E_PLINE, H5E_CANTFILTER, H5_ITER_ERROR, "output pipeline failed")

#if H5_SIZEOF_SIZE_T > 4
        /* Check for the chunk expanding too much to encode in a 32-bit value */
	if(nbytes > ((size_t)0xffffffff))
	    HGOTO_ERROR(H5E_DATASET, H5E_BADRANGE, H5_ITER_ERROR, "chunk too large for 32-bit length")
#endif /* H5_SIZEOF_SIZE_T > 4 */

	/* Allocate space for the filtered chunk */
	if((chunk_addr = H5MF_alloc(new_idx_info->f, H5FD_MEM_DRAW, new_idx_info->dxpl_id, (hsize_t)nbytes)) == HADDR_UNDEF)
	    HGOTO_ERROR(H5E_DATASET, H5E_NOSPACE, H5_ITER_ERROR, "file allocation failed for filtered chunk")
	HDassert(H5F_addr_defined(chunk_addr));

	/* Write the filtered chunk to disk */
	if(H5F_block_write(new_idx_info->f, H5FD_MEM_DRAW, chunk_addr, nbytes, H5AC_rawdata_dxpl_id, buf) < 0)
	    HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, H5_ITER_ERROR, "unable to write raw data to file")
    } /* end if */

    /* Set up chunk information for insertion to chunk index */
    insert_udata.chunk_block.offset = chunk_addr;
    insert_udata.chunk_block.length = nbytes;
    insert_udata.filter_mask = chunk_rec->filter_mask;
    insert_udata.common.scaled = chunk_rec->scaled;
    insert_udata.common.layout = new_idx_info->layout;
    insert_udata.common.storage = new_idx_info->storage;

    /* Insert chunk into the v1 B-tree chunk index */
    if((new_idx_info->storage->ops->insert)(new_idx_info, &insert_udata, NULL) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, H5_ITER_ERROR, "unable to insert chunk addr into index")

done:
    if(buf)
        H5MM_xfree(buf);

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D__chunk_format_convert_cb() */


/*-------------------------------------------------------------------------
 * Function:    H5D__chunk_format_convert
 *
 * Purpose:     Iterate over the chunks for the current chunk index and insert the
 *		the chunk addresses into v1 B-tree chunk index via callback.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Vailin Choi
 *              Feb 2015
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__chunk_format_convert(H5D_t *dset, H5D_chk_idx_info_t *idx_info, H5D_chk_idx_info_t *new_idx_info)
{
    H5D_chunk_it_ud5_t udata;		/* User data */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(dset);

    /* Set up user data */
    udata.new_idx_info = new_idx_info;
    udata.dset_ndims = dset->shared->ndims;
    udata.dset_dims = dset->shared->curr_dims;

    /*  terate over the chunks in the current index and insert the chunk addresses into version 1 B-tree index */
    if((idx_info->storage->ops->iterate)(idx_info, H5D__chunk_format_convert_cb, &udata) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_BADITER, FAIL, "unable to iterate over chunk index to chunk info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_format_convert() */

