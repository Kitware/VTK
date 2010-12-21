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
 *              Friday, March 27, 1998
 *
 * Purpose:	Operations on the global heap.  The global heap is the set of
 *		all collections and each collection contains one or more
 *		global heap objects.  An object belongs to exactly one
 *		collection.  A collection is treated as an atomic entity for
 *		the purposes of I/O and caching.
 *
 *		Each file has a small cache of global heap collections called
 *		the CWFS list and recently accessed collections with free
 *		space appear on this list.  As collections are accessed the
 *		collection is moved toward the front of the list.  New
 *		collections are added to the front of the list while old
 *		collections are added to the end of the list.
 *
 *		The collection model reduces the overhead which would be
 *		incurred if the global heap were a single object, and the
 *		CWFS list allows the library to cheaply choose a collection
 *		for a new object based on object size, amount of free space
 *		in the collection, and temporal locality.
 */

/****************/
/* Module Setup */
/****************/

#define H5F_PACKAGE		/*suppress error about including H5Fpkg	  */
#define H5HG_PACKAGE		/*suppress error about including H5HGpkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fpkg.h"             /* File access				*/
#include "H5HGpkg.h"		/* Global heaps				*/
#include "H5MFprivate.h"	/* File memory management		*/
#include "H5MMprivate.h"	/* Memory management			*/


/****************/
/* Local Macros */
/****************/

/*
 * Limit global heap collections to the some reasonable size.  This is
 * fairly arbitrary, but needs to be small enough that no more than H5HG_MAXIDX
 * objects will be allocated from a single heap.
 */
#define H5HG_MAXSIZE	65536

/*
 * The maximum number of links allowed to a global heap object.
 */
#define H5HG_MAXLINK	65535

/*
 * The maximum number of indices allowed in a global heap object.
 */
#define H5HG_MAXIDX	65535


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

static haddr_t H5HG_create(H5F_t *f, hid_t dxpl_id, size_t size);


/*********************/
/* Package Variables */
/*********************/

/* Declare a free list to manage the H5HG_t struct */
H5FL_DEFINE(H5HG_heap_t);

/* Declare a free list to manage sequences of H5HG_obj_t's */
H5FL_SEQ_DEFINE(H5HG_obj_t);

/* Declare a PQ free list to manage heap chunks */
H5FL_BLK_DEFINE(gheap_chunk);


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:	H5HG_create
 *
 * Purpose:	Creates a global heap collection of the specified size.  If
 *		SIZE is less than some minimum it will be readjusted.  The
 *		new collection is allocated in the file and added to the
 *		beginning of the CWFS list.
 *
 * Return:	Success:	Ptr to a cached heap.  The pointer is valid
 *				only until some other hdf5 library function
 *				is called.
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Friday, March 27, 1998
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5HG_create(H5F_t *f, hid_t dxpl_id, size_t size)
{
    H5HG_heap_t	*heap = NULL;
    uint8_t	*p = NULL;
    haddr_t	addr;
    size_t	n;
    haddr_t	ret_value = HADDR_UNDEF;        /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HG_create)

    /* Check args */
    HDassert(f);
    if(size < H5HG_MINSIZE)
        size = H5HG_MINSIZE;
    size = H5HG_ALIGN(size);

    /* Create it */
    H5_CHECK_OVERFLOW(size, size_t, hsize_t);
    if(HADDR_UNDEF == (addr = H5MF_alloc(f, H5FD_MEM_GHEAP, dxpl_id, (hsize_t)size)))
	HGOTO_ERROR(H5E_HEAP, H5E_CANTINIT, HADDR_UNDEF, "unable to allocate file space for global heap")
    if(NULL == (heap = H5FL_MALLOC(H5HG_heap_t)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, HADDR_UNDEF, "memory allocation failed")
    heap->addr = addr;
    heap->size = size;
    heap->shared = f->shared;

    if(NULL == (heap->chunk = H5FL_BLK_MALLOC(gheap_chunk, size)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, HADDR_UNDEF, "memory allocation failed")
#ifdef H5_CLEAR_MEMORY
HDmemset(heap->chunk, 0, size);
#endif /* H5_CLEAR_MEMORY */
    heap->nalloc = H5HG_NOBJS(f, size);
    heap->nused = 1; /* account for index 0, which is used for the free object */
    if(NULL == (heap->obj = H5FL_SEQ_MALLOC(H5HG_obj_t, heap->nalloc)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, HADDR_UNDEF, "memory allocation failed")

    /* Initialize the header */
    HDmemcpy(heap->chunk, H5HG_MAGIC, (size_t)H5_SIZEOF_MAGIC);
    p = heap->chunk + H5_SIZEOF_MAGIC;
    *p++ = H5HG_VERSION;
    *p++ = 0; /*reserved*/
    *p++ = 0; /*reserved*/
    *p++ = 0; /*reserved*/
    H5F_ENCODE_LENGTH(f, p, size);

    /*
     * Padding so free space object is aligned. If malloc returned memory
     * which was always at least H5HG_ALIGNMENT aligned then we could just
     * align the pointer, but this might not be the case.
     */
    n = H5HG_ALIGN(p - heap->chunk) - (p - heap->chunk);
#ifdef OLD_WAY
/* Don't bother zeroing out the rest of the info in the heap -QAK */
    HDmemset(p, 0, n);
#endif /* OLD_WAY */
    p += n;

    /* The freespace object */
    heap->obj[0].size = size - H5HG_SIZEOF_HDR(f);
    assert(H5HG_ISALIGNED(heap->obj[0].size));
    heap->obj[0].nrefs = 0;
    heap->obj[0].begin = p;
    UINT16ENCODE(p, 0);	/*object ID*/
    UINT16ENCODE(p, 0);	/*reference count*/
    UINT32ENCODE(p, 0); /*reserved*/
    H5F_ENCODE_LENGTH(f, p, heap->obj[0].size);
#ifdef OLD_WAY
/* Don't bother zeroing out the rest of the info in the heap -QAK */
    HDmemset (p, 0, (size_t)((heap->chunk+heap->size) - p));
#endif /* OLD_WAY */

    /* Add this heap to the beginning of the CWFS list */
    if(NULL == f->shared->cwfs) {
	f->shared->cwfs = (H5HG_heap_t **)H5MM_malloc(H5HG_NCWFS * sizeof(H5HG_heap_t *));
	if(NULL == (f->shared->cwfs))
	    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, HADDR_UNDEF, "memory allocation failed")
	f->shared->cwfs[0] = heap;
	f->shared->ncwfs = 1;
    } /* end if */
    else {
	HDmemmove(f->shared->cwfs + 1, f->shared->cwfs,
                   MIN(f->shared->ncwfs, H5HG_NCWFS - 1) * sizeof(H5HG_heap_t *));
	f->shared->cwfs[0] = heap;
	f->shared->ncwfs = MIN(H5HG_NCWFS, f->shared->ncwfs + 1);
    } /* end else */

    /* Add the heap to the cache */
    if (H5AC_set (f, dxpl_id, H5AC_GHEAP, addr, heap, H5AC__NO_FLAGS_SET)<0)
	HGOTO_ERROR (H5E_HEAP, H5E_CANTINIT, HADDR_UNDEF, \
                     "unable to cache global heap collection");

    ret_value = addr;

done:
    /* Cleanup on error */
    if(!H5F_addr_defined(ret_value)) {
        if(H5F_addr_defined(addr)) {
            /* Release the space on disk */
            if(H5MF_xfree(f, H5FD_MEM_GHEAP, dxpl_id, addr, (hsize_t)size) < 0)
                HDONE_ERROR(H5E_BTREE, H5E_CANTFREE, HADDR_UNDEF, "unable to free global heap")

            /* Check if the heap object was allocated */
            if(heap)
                /* Destroy the heap object */
                if(H5HG_free(heap) < 0)
                    HDONE_ERROR(H5E_HEAP, H5E_CANTFREE, HADDR_UNDEF, "unable to destroy global heap collection")
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
} /* H5HG_create() */


/*-------------------------------------------------------------------------
 * Function:	H5HG_protect
 *
 * Purpose:	Convenience wrapper around H5AC_protect on an indirect block
 *
 * Return:	Pointer to indirect block on success, NULL on failure
 *
 * Programmer:	Quincey Koziol
 *              Wednesday, May  5, 2010
 *
 *-------------------------------------------------------------------------
 */
H5HG_t *
H5HG_protect(H5F_t *f, hid_t dxpl_id, haddr_t addr, H5AC_protect_t rw)
{
    H5HG_heap_t *heap;          /* Global heap */
    H5HG_heap_t *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HG_protect)

    /* Check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));

    /* Lock the heap into memory */
    if(NULL == (heap = (H5HG_heap_t *)H5AC_protect(f, dxpl_id, H5AC_GHEAP, addr, f, rw)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTPROTECT, NULL, "unable to protect global heap")

    /* Set the heap's address */
    heap->addr = addr;

    /* Set the return value */
    ret_value = heap;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5HG_protect() */


/*-------------------------------------------------------------------------
 * Function:	H5HG_alloc
 *
 * Purpose:	Given a heap with enough free space, this function will split
 *		the free space to make a new empty heap object and initialize
 *		the header.  SIZE is the exact size of the object data to be
 *		stored. It will be increased to make room for the object
 *		header and then rounded up for alignment.
 *
 * Return:	Success:	The heap object ID of the new object.
 *
 *		Failure:	0
 *
 * Programmer:	Robb Matzke
 *              Friday, March 27, 1998
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5HG_alloc (H5F_t *f, H5HG_heap_t *heap, size_t size, unsigned * heap_flags_ptr)
{
    size_t	idx;
    uint8_t	*p = NULL;
    size_t	need = H5HG_SIZEOF_OBJHDR(f) + H5HG_ALIGN(size);
    size_t ret_value;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HG_alloc);

    /* Check args */
    assert (heap);
    assert (heap->obj[0].size>=need);
    assert (heap_flags_ptr);

    /*
     * Find an ID for the new object. ID zero is reserved for the free space
     * object.
     */
    if(heap->nused<=H5HG_MAXIDX)
        idx=heap->nused++;
    else {
        for (idx=1; idx<heap->nused; idx++)
            if (NULL==heap->obj[idx].begin)
                break;
    } /* end else */

    HDassert(idx < heap->nused);

    /* Check if we need more room to store heap objects */
    if(idx>=heap->nalloc) {
        size_t new_alloc;       /* New allocation number */
        H5HG_obj_t *new_obj;	/* New array of object descriptions */

        /* Determine the new number of objects to index */
        /* nalloc is *not* guaranteed to be a power of 2! - NAF 10/26/09 */
        new_alloc = MIN(MAX(heap->nalloc * 2, (idx + 1)), (H5HG_MAXIDX + 1));
        HDassert(idx < new_alloc);

        /* Reallocate array of objects */
        if (NULL==(new_obj = H5FL_SEQ_REALLOC (H5HG_obj_t, heap->obj, new_alloc)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "memory allocation failed")

        /* Clear newly allocated space */
        HDmemset(&new_obj[heap->nalloc], 0, (new_alloc - heap->nalloc) * sizeof(heap->obj[0]));

        /* Update heap information */
        heap->nalloc=new_alloc;
        heap->obj=new_obj;
        HDassert(heap->nalloc>heap->nused);
    } /* end if */

    /* Initialize the new object */
    heap->obj[idx].nrefs = 0;
    heap->obj[idx].size = size;
    heap->obj[idx].begin = heap->obj[0].begin;
    p = heap->obj[idx].begin;
    UINT16ENCODE(p, idx);
    UINT16ENCODE(p, 0); /*nrefs*/
    UINT32ENCODE(p, 0); /*reserved*/
    H5F_ENCODE_LENGTH (f, p, size);

    /* Fix the free space object */
    if (need==heap->obj[0].size) {
	/*
	 * All free space has been exhausted from this collection.
	 */
	heap->obj[0].size = 0;
	heap->obj[0].begin = NULL;

    } else if (heap->obj[0].size-need >= H5HG_SIZEOF_OBJHDR (f)) {
	/*
	 * Some free space remains and it's larger than a heap object header,
	 * so write the new free heap object header to the heap.
	 */
	heap->obj[0].size -= need;
	heap->obj[0].begin += need;
	p = heap->obj[0].begin;
	UINT16ENCODE(p, 0);	/*id*/
	UINT16ENCODE(p, 0);	/*nrefs*/
	UINT32ENCODE(p, 0);	/*reserved*/
	H5F_ENCODE_LENGTH (f, p, heap->obj[0].size);
	assert(H5HG_ISALIGNED(heap->obj[0].size));

    } else {
	/*
	 * Some free space remains but it's smaller than a heap object header,
	 * so we don't write the header.
	 */
	heap->obj[0].size -= need;
	heap->obj[0].begin += need;
	assert(H5HG_ISALIGNED(heap->obj[0].size));
    }

    /* Mark the heap as dirty */
    *heap_flags_ptr |= H5AC__DIRTIED_FLAG;

    /* Set the return value */
    ret_value=idx;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5HG_extend
 *
 * Purpose:	Extend a heap to hold an object of SIZE bytes.
 *		SIZE is the exact size of the object data to be
 *		stored. It will be increased to make room for the object
 *		header and then rounded up for alignment.
 *
 * Return:	Success:	Non-negative
 *
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Saturday, June 12, 2004
 *
 * Modifications:
 *
 *		John Mainzer, 6/8/05
 *		Modified the function to use the new dirtied parameter of
 *		of H5AC_unprotect() instead of modifying the is_dirty
 *		field of the cache info.
 *
 *		In this case, that required adding the new heap_dirtied_ptr
 *		parameter to the function's argument list.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HG_extend(H5F_t *f, H5HG_heap_t *heap, size_t need, unsigned *heap_flags_ptr)
{
    size_t  old_size;               /* Previous size of the heap's chunk */
    uint8_t *new_chunk = NULL;      /* Pointer to new chunk information */
    uint8_t *p = NULL;              /* Pointer to raw heap info */
    unsigned u;                     /* Local index variable */
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HG_extend)

    /* Check args */
    HDassert(f);
    HDassert(heap);
    HDassert(heap_flags_ptr);

    /* Re-allocate the heap information in memory */
    if(NULL == (new_chunk = H5FL_BLK_REALLOC(gheap_chunk, heap->chunk, (heap->size + need))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "new heap allocation failed")
#ifdef H5_CLEAR_MEMORY
HDmemset(new_chunk + heap->size, 0, need);
#endif /* H5_CLEAR_MEMORY */

    /* Adjust the size of the heap */
    old_size = heap->size;
    heap->size += need;

    /* Encode the new size of the heap */
    p = new_chunk + H5_SIZEOF_MAGIC + 1 /* version */ + 3 /* reserved */;
    H5F_ENCODE_LENGTH(f, p, heap->size);

    /* Move the pointers to the existing objects to their new locations */
    for(u = 0; u < heap->nused; u++)
        if(heap->obj[u].begin)
            heap->obj[u].begin = new_chunk + (heap->obj[u].begin - heap->chunk);

    /* Update the heap chunk pointer now */
    heap->chunk = new_chunk;

    /* Update the free space information for the heap  */
    heap->obj[0].size += need;
    if(heap->obj[0].begin == NULL)
        heap->obj[0].begin = heap->chunk+old_size;
    p = heap->obj[0].begin;
    UINT16ENCODE(p, 0);	/*id*/
    UINT16ENCODE(p, 0);	/*nrefs*/
    UINT32ENCODE(p, 0);	/*reserved*/
    H5F_ENCODE_LENGTH(f, p, heap->obj[0].size);
    assert(H5HG_ISALIGNED(heap->obj[0].size));

    /* Mark the heap as dirty */
    *heap_flags_ptr |= H5AC__DIRTIED_FLAG;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HG_extend() */


/*-------------------------------------------------------------------------
 * Function:	H5HG_insert
 *
 * Purpose:	A new object is inserted into the global heap.  It will be
 *		placed in the first collection on the CWFS list which has
 *		enough free space and that collection will be advanced one
 *		position in the list.  If no collection on the CWFS list has
 *		enough space then  a new collection will be created.
 *
 *		It is legal to push a zero-byte object onto the heap to get
 *		the reference count features of heap objects.
 *
 * Return:	Success:	Non-negative, and a heap object handle returned
 *				through the HOBJ pointer.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Friday, March 27, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HG_insert(H5F_t *f, hid_t dxpl_id, size_t size, void *obj, H5HG_t *hobj/*out*/)
{
    size_t	need;		/*total space needed for object		*/
    unsigned    cwfsno;
    size_t	idx;
    haddr_t	addr = HADDR_UNDEF;
    H5HG_heap_t	*heap = NULL;
    unsigned 	heap_flags = H5AC__NO_FLAGS_SET;
    hbool_t     found = FALSE;          /* Flag to indicate a heap with enough space was found */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5HG_insert, FAIL)

    /* Check args */
    HDassert(f);
    HDassert(0 == size || obj);
    HDassert(hobj);

    if(0 == (f->intent & H5F_ACC_RDWR))
	HGOTO_ERROR(H5E_HEAP, H5E_WRITEERROR, FAIL, "no write intent on file")

    /* Find a large enough collection on the CWFS list */
    need = H5HG_SIZEOF_OBJHDR(f) + H5HG_ALIGN(size);

    /* Note that we don't have metadata cache locks on the entries in
     * f->shared->cwfs.
     *
     * In the current situation, this doesn't matter, as we are single
     * threaded, and as best I can tell, entries are added to and deleted
     * from f->shared->cwfs as they are added to and deleted from the
     * metadata cache.
     *
     * To be proper, we should either lock each entry in f->shared->cwfs
     * as we examine it, or lock the whole array.  However, at present
     * I don't see the point as there will be significant overhead,
     * and protecting and unprotecting all the collections in the global
     * heap on a regular basis will skew the replacement policy.
     *
     * However, there is a bigger issue -- as best I can tell, we only look
     * for free space in global heap chunks that are in cache.  If we can't
     * find any, we allocate a new chunk.  This may be a problem in FP mode,
     * as the metadata cache is disabled.  Do we allocate a new heap
     * collection for every entry in this case?
     *
     * Note that all this comes from a cursory read of the source.  Don't
     * take any of it as gospel.
     *                                        JRM - 5/24/04
     */
    for(cwfsno = 0; cwfsno < f->shared->ncwfs; cwfsno++)
	if(f->shared->cwfs[cwfsno]->obj[0].size >= need) {
	    addr = f->shared->cwfs[cwfsno]->addr;
            found = TRUE;
	    break;
	} /* end if */

    /*
     * If we didn't find any collection with enough free space the check if
     * we can extend any of the collections to make enough room.
     */
    if(!found) {
        size_t new_need;

        for(cwfsno = 0; cwfsno < f->shared->ncwfs; cwfsno++) {
            new_need = need;
            new_need -= f->shared->cwfs[cwfsno]->obj[0].size;
            new_need = MAX(f->shared->cwfs[cwfsno]->size, new_need);

            if((f->shared->cwfs[cwfsno]->size + new_need) <= H5HG_MAXSIZE) {
                htri_t extended;        /* Whether the heap was extended */

                extended = H5MF_try_extend(f, dxpl_id, H5FD_MEM_GHEAP, f->shared->cwfs[cwfsno]->addr, (hsize_t)f->shared->cwfs[cwfsno]->size, (hsize_t)new_need);
                if(extended < 0)
                    HGOTO_ERROR(H5E_HEAP, H5E_CANTEXTEND, FAIL, "error trying to extend heap")
                else if(extended == TRUE) {
                    if(H5HG_extend(f, f->shared->cwfs[cwfsno], new_need, &heap_flags) < 0)
                        HGOTO_ERROR(H5E_HEAP, H5E_CANTINIT, FAIL, "unable to extend global heap collection")
                    addr = f->shared->cwfs[cwfsno]->addr;
                    found = TRUE;
                    break;
                } /* end if */
            } /* end if */
        } /* end for */
    } /* end if */

    /*
     * If we didn't find any collection with enough free space then allocate a
     * new collection large enough for the message plus the collection header.
     */
    if(!found) {
        addr = H5HG_create(f, dxpl_id, need+H5HG_SIZEOF_HDR (f));

        if(!H5F_addr_defined(addr))
	    HGOTO_ERROR(H5E_HEAP, H5E_CANTINIT, FAIL, "unable to allocate a global heap collection")
    } /* end if */
    else {
        /* Move the collection forward in the CWFS list, if it's not
         * already at the front
         */
        if(cwfsno > 0) {
            H5HG_heap_t *tmp = f->shared->cwfs[cwfsno];

            f->shared->cwfs[cwfsno] = f->shared->cwfs[cwfsno - 1];
            f->shared->cwfs[cwfsno - 1] = tmp;
            --cwfsno;
        } /* end if */
    } /* end else */
    HDassert(H5F_addr_defined(addr));
    if(NULL == (heap = H5HG_protect(f, dxpl_id, addr, H5AC_WRITE)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTPROTECT, FAIL, "unable to protect global heap")

    /* Split the free space to make room for the new object */
    idx = H5HG_alloc(f, heap, size, &heap_flags);

    /* Copy data into the heap */
    if(size > 0) {
        HDmemcpy(heap->obj[idx].begin + H5HG_SIZEOF_OBJHDR(f), obj, size);
#ifdef OLD_WAY
/* Don't bother zeroing out the rest of the info in the heap -QAK */
        HDmemset(heap->obj[idx].begin + H5HG_SIZEOF_OBJHDR(f) + size, 0,
                 need - (H5HG_SIZEOF_OBJHDR(f) + size));
#endif /* OLD_WAY */
    } /* end if */
    heap_flags |= H5AC__DIRTIED_FLAG;

    /* Return value */
    hobj->addr = heap->addr;
    hobj->idx = idx;

done:
    if(heap && H5AC_unprotect(f, dxpl_id, H5AC_GHEAP, heap->addr, heap, heap_flags) < 0)
        HDONE_ERROR(H5E_HEAP, H5E_PROTECT, FAIL, "unable to unprotect heap.")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5HG_insert() */


/*-------------------------------------------------------------------------
 * Function:	H5HG_read
 *
 * Purpose:	Reads the specified global heap object into the buffer OBJECT
 *		supplied by the caller.  If the caller doesn't supply a
 *		buffer then one will be allocated.  The buffer should be
 *		large enough to hold the result.
 *
 * Return:	Success:	The buffer containing the result.
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Monday, March 30, 1998
 *
 *-------------------------------------------------------------------------
 */
void *
H5HG_read(H5F_t *f, hid_t dxpl_id, H5HG_t *hobj, void *object/*out*/,
    size_t *buf_size)
{
    H5HG_heap_t	*heap = NULL;           /* Pointer to global heap object */
    size_t	size;                   /* Size of the heap object */
    uint8_t	*p;                     /* Pointer to object in heap buffer */
    void        *orig_object = object;  /* Keep a copy of the original object pointer */
    void	*ret_value;             /* Return value */

    FUNC_ENTER_NOAPI(H5HG_read, NULL)

    /* Check args */
    HDassert(f);
    HDassert(hobj);

    /* Load the heap */
    if(NULL == (heap = H5HG_protect(f, dxpl_id, hobj->addr, H5AC_READ)))
	HGOTO_ERROR(H5E_HEAP, H5E_CANTPROTECT, NULL, "unable to protect global heap")

    HDassert(hobj->idx < heap->nused);
    HDassert(heap->obj[hobj->idx].begin);
    size = heap->obj[hobj->idx].size;
    p = heap->obj[hobj->idx].begin + H5HG_SIZEOF_OBJHDR(f);

    /* Allocate a buffer for the object read in, if the user didn't give one */
    if(!object && NULL == (object = H5MM_malloc(size)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
    HDmemcpy(object, p, size);

    /*
     * Advance the heap in the CWFS list. We might have done this already
     * with the H5AC_protect(), but it won't hurt to do it twice.
     */
    if(heap->obj[0].begin) {
        unsigned u;     /* Local index variable */

	for(u = 0; u < f->shared->ncwfs; u++)
	    if(f->shared->cwfs[u] == heap) {
		if(u) {
		    f->shared->cwfs[u] = f->shared->cwfs[u - 1];
		    f->shared->cwfs[u - 1] = heap;
		} /* end if */
		break;
	    } /* end if */
    } /* end if */

    /* If the caller would like to know the heap object's size, set that */
    if(buf_size)
        *buf_size = size;

    /* Set return value */
    ret_value = object;

done:
    if(heap && H5AC_unprotect(f, dxpl_id, H5AC_GHEAP, hobj->addr, heap, H5AC__NO_FLAGS_SET) < 0)
        HDONE_ERROR(H5E_HEAP, H5E_PROTECT, NULL, "unable to release object header")

    if(NULL == ret_value && NULL == orig_object && object)
        H5MM_free(object);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HG_read() */


/*-------------------------------------------------------------------------
 * Function:	H5HG_link
 *
 * Purpose:	Adjusts the link count for a global heap object by adding
 *		ADJUST to the current value.  This function will fail if the
 *		new link count would overflow.  Nothing special happens when
 *		the link count reaches zero; in order for a heap object to be
 *		removed one must call H5HG_remove().
 *
 * Return:	Success:	Number of links present after the adjustment.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Monday, March 30, 1998
 *
 *-------------------------------------------------------------------------
 */
int
H5HG_link(H5F_t *f, hid_t dxpl_id, const H5HG_t *hobj, int adjust)
{
    H5HG_heap_t *heap = NULL;
    unsigned heap_flags = H5AC__NO_FLAGS_SET;
    int ret_value;              /* Return value */

    FUNC_ENTER_NOAPI(H5HG_link, FAIL)

    /* Check args */
    HDassert(f);
    HDassert(hobj);
    if(0 == (f->intent & H5F_ACC_RDWR))
	HGOTO_ERROR(H5E_HEAP, H5E_WRITEERROR, FAIL, "no write intent on file")

    /* Load the heap */
    if(NULL == (heap = H5HG_protect(f, dxpl_id, hobj->addr, H5AC_WRITE)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTPROTECT, FAIL, "unable to protect global heap")

    if(adjust != 0) {
        HDassert(hobj->idx < heap->nused);
        HDassert(heap->obj[hobj->idx].begin);
        if((heap->obj[hobj->idx].nrefs + adjust) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_BADRANGE, FAIL, "new link count would be out of range")
        if((heap->obj[hobj->idx].nrefs + adjust) > H5HG_MAXLINK)
            HGOTO_ERROR(H5E_HEAP, H5E_BADVALUE, FAIL, "new link count would be out of range")
        heap->obj[hobj->idx].nrefs += adjust;
        heap_flags |= H5AC__DIRTIED_FLAG;
    } /* end if */

    /* Set return value */
    ret_value = heap->obj[hobj->idx].nrefs;

done:
    if(heap && H5AC_unprotect(f, dxpl_id, H5AC_GHEAP, hobj->addr, heap, heap_flags) < 0)
        HDONE_ERROR(H5E_HEAP, H5E_PROTECT, FAIL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HG_link() */


/*-------------------------------------------------------------------------
 * Function:	H5HG_remove
 *
 * Purpose:	Removes the specified object from the global heap.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, March 30, 1998
 *
 * Modifications:
 *
 *		John Mainzer - 6/8/05
 *		Modified function to use the dirtied parameter of
 *		H5AC_unprotect() instead of modifying the is_dirty
 *		field of the cache info.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HG_remove (H5F_t *f, hid_t dxpl_id, H5HG_t *hobj)
{
    H5HG_heap_t	*heap = NULL;
    uint8_t	*p = NULL, *obj_start = NULL;
    size_t	need;
    unsigned	u;
    unsigned    flags = H5AC__NO_FLAGS_SET;/* Whether the heap gets deleted */
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5HG_remove, FAIL);

    /* Check args */
    HDassert(f);
    HDassert(hobj);
    if(0 == (f->intent & H5F_ACC_RDWR))
        HGOTO_ERROR(H5E_HEAP, H5E_WRITEERROR, FAIL, "no write intent on file")

    /* Load the heap */
    if(NULL == (heap = H5HG_protect(f, dxpl_id, hobj->addr, H5AC_WRITE)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTPROTECT, FAIL, "unable to protect global heap")

    HDassert(hobj->idx < heap->nused);
    HDassert(heap->obj[hobj->idx].begin);
    obj_start = heap->obj[hobj->idx].begin;
    /* Include object header size */
    need = H5HG_ALIGN(heap->obj[hobj->idx].size) + H5HG_SIZEOF_OBJHDR(f);

    /* Move the new free space to the end of the heap */
    for(u = 0; u < heap->nused; u++)
        if(heap->obj[u].begin > heap->obj[hobj->idx].begin)
            heap->obj[u].begin -= need;
    if(NULL == heap->obj[0].begin) {
        heap->obj[0].begin = heap->chunk + (heap->size - need);
        heap->obj[0].size = need;
        heap->obj[0].nrefs = 0;
    } /* end if */
    else
        heap->obj[0].size += need;
    HDmemmove(obj_start, obj_start + need,
	       heap->size - ((obj_start + need) - heap->chunk));
    if(heap->obj[0].size >= H5HG_SIZEOF_OBJHDR(f)) {
        p = heap->obj[0].begin;
        UINT16ENCODE(p, 0); /*id*/
        UINT16ENCODE(p, 0); /*nrefs*/
        UINT32ENCODE(p, 0); /*reserved*/
        H5F_ENCODE_LENGTH (f, p, heap->obj[0].size);
    } /* end if */
    HDmemset(heap->obj + hobj->idx, 0, sizeof(H5HG_obj_t));
    flags |= H5AC__DIRTIED_FLAG;

    if((heap->obj[0].size + H5HG_SIZEOF_HDR(f)) == heap->size) {
        /*
         * The collection is empty. Remove it from the CWFS list and return it
         * to the file free list.
         */
        flags |= H5AC__DELETED_FLAG | H5AC__FREE_FILE_SPACE_FLAG; /* Indicate that the object was deleted, for the unprotect call */
    } /* end if */
    else {
        /*
         * If the heap is in the CWFS list then advance it one position.  The
         * H5AC_protect() might have done that too, but that's okay.  If the
         * heap isn't on the CWFS list then add it to the end.
         */
        for(u = 0; u < f->shared->ncwfs; u++)
            if(f->shared->cwfs[u] == heap) {
                if(u) {
                    f->shared->cwfs[u] = f->shared->cwfs[u - 1];
                    f->shared->cwfs[u - 1] = heap;
                } /* end if */
                break;
            } /* end if */
        if(u >= f->shared->ncwfs) {
            f->shared->ncwfs = MIN(f->shared->ncwfs + 1, H5HG_NCWFS);
            f->shared->cwfs[f->shared->ncwfs - 1] = heap;
        } /* end if */
    } /* end else */

done:
    if(heap && H5AC_unprotect(f, dxpl_id, H5AC_GHEAP, hobj->addr, heap, flags) < 0)
        HDONE_ERROR(H5E_HEAP, H5E_PROTECT, FAIL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5HG_remove() */


/*-------------------------------------------------------------------------
 * Function:	H5HG_free
 *
 * Purpose:	Destroys a global heap collection in memory
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Wednesday, January 15, 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HG_free(H5HG_heap_t *heap)
{
    unsigned u;         /* Local index variable */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5HG_free)

    /* Check arguments */
    HDassert(heap);

    /* Remove the heap from the CWFS list */
    for(u = 0; u < heap->shared->ncwfs; u++) {
        if(heap->shared->cwfs[u] == heap) {
            heap->shared->ncwfs -= 1;
            HDmemmove(heap->shared->cwfs + u, heap->shared->cwfs + u + 1, (heap->shared->ncwfs - u) * sizeof(H5HG_heap_t *));
            break;
        } /* end if */
    } /* end for */

    if(heap->chunk)
        heap->chunk = H5FL_BLK_FREE(gheap_chunk, heap->chunk);
    if(heap->obj)
        heap->obj = H5FL_SEQ_FREE(H5HG_obj_t, heap->obj);
    heap = H5FL_FREE(H5HG_heap_t, heap);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HG_free() */

