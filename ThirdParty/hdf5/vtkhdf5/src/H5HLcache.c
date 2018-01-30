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
 * Created:     H5HLcache.c
 *              Feb  5 2008
 *              Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:     Implement local heap metadata cache methods.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#include "H5HLmodule.h"         /* This source code file is part of the H5HL module */


/***********/
/* Headers */
/***********/
#include "H5private.h"      /* Generic Functions            */
#include "H5Eprivate.h"     /* Error handling               */
#include "H5HLpkg.h"        /* Local Heaps                  */
#include "H5MFprivate.h"    /* File memory management       */
#include "H5WBprivate.h"    /* Wrapped Buffers              */


/****************/
/* Local Macros */
/****************/

#define H5HL_VERSION    0               /* Local heap collection version    */

/* Set the local heap size to speculatively read in
 *      (needs to be more than the local heap prefix size to work at all and
 *      should be larger than the default local heap size to save the
 *      extra I/O operations)
 */
#define H5HL_SPEC_READ_SIZE     512


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

/* Metadata cache callbacks */
/* Local heap prefix */
static herr_t H5HL__cache_prefix_get_initial_load_size(void *udata, size_t *image_len);
static herr_t H5HL__cache_prefix_get_final_load_size(const void *_image,
    size_t image_len, void *udata, size_t *actual_len);
static void *H5HL__cache_prefix_deserialize(const void *image, size_t len,
    void *udata, hbool_t *dirty);
static herr_t H5HL__cache_prefix_image_len(const void *thing, size_t *image_len);
static herr_t H5HL__cache_prefix_serialize(const H5F_t *f, void *image,
    size_t len, void *thing); 
static herr_t H5HL__cache_prefix_free_icr(void *thing);

/* Local heap data block */
static herr_t H5HL__cache_datablock_get_initial_load_size(void *udata, size_t *image_len);
static void *H5HL__cache_datablock_deserialize(const void *image, size_t len,
    void *udata, hbool_t *dirty); 
static herr_t H5HL__cache_datablock_image_len(const void *thing, size_t *image_len);
static herr_t H5HL__cache_datablock_serialize(const H5F_t *f, void *image,
    size_t len, void *thing); 
static herr_t H5HL__cache_datablock_notify(H5C_notify_action_t action, void *_thing);
static herr_t H5HL__cache_datablock_free_icr(void *thing);

/* Header deserialization */
static herr_t H5HL__hdr_deserialize(H5HL_t *heap, const uint8_t *image,
    H5HL_cache_prfx_ud_t *udata);

/* Free list de/serialization */
static herr_t H5HL__fl_deserialize(H5HL_t *heap);
static void H5HL__fl_serialize(const H5HL_t *heap);

/*********************/
/* Package Variables */
/*********************/

/* H5HL inherits cache-like properties from H5AC */
const H5AC_class_t H5AC_LHEAP_PRFX[1] = {{
    H5AC_LHEAP_PRFX_ID,                 /* Metadata client ID */
    "local heap prefix",                /* Metadata client name (for debugging) */
    H5FD_MEM_LHEAP,                     /* File space memory type for client */
    H5AC__CLASS_SPECULATIVE_LOAD_FLAG,  /* Client class behavior flags */
    H5HL__cache_prefix_get_initial_load_size,   /* 'get_initial_load_size' callback */
    H5HL__cache_prefix_get_final_load_size, /* 'get_final_load_size' callback */
    NULL,				/* 'verify_chksum' callback */
    H5HL__cache_prefix_deserialize,     /* 'deserialize' callback */
    H5HL__cache_prefix_image_len,       /* 'image_len' callback */
    NULL,                               /* 'pre_serialize' callback */
    H5HL__cache_prefix_serialize,       /* 'serialize' callback */
    NULL,                               /* 'notify' callback */
    H5HL__cache_prefix_free_icr,        /* 'free_icr' callback */
    NULL,                               /* 'fsf_size' callback */
}};

const H5AC_class_t H5AC_LHEAP_DBLK[1] = {{
    H5AC_LHEAP_DBLK_ID,                 /* Metadata client ID */
    "local heap datablock",             /* Metadata client name (for debugging) */
    H5FD_MEM_LHEAP,                     /* File space memory type for client */
    H5AC__CLASS_NO_FLAGS_SET,           /* Client class behavior flags */
    H5HL__cache_datablock_get_initial_load_size,/* 'get_initial_load_size' callback */
    NULL,				/* 'get_final_load_size' callback */
    NULL,				/* 'verify_chksum' callback */
    H5HL__cache_datablock_deserialize,  /* 'deserialize' callback */
    H5HL__cache_datablock_image_len,    /* 'image_len' callback */
    NULL,                               /* 'pre_serialize' callback */
    H5HL__cache_datablock_serialize,    /* 'serialize' callback */
    H5HL__cache_datablock_notify,       /* 'notify' callback */
    H5HL__cache_datablock_free_icr,     /* 'free_icr' callback */
    NULL,                               /* 'fsf_size' callback */
}};


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:    H5HL__hdr_deserialize()
 *
 * Purpose:	Decode a local heap's header
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  Quincey Koziol
 *              December 15, 2016
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HL__hdr_deserialize( H5HL_t *heap, const uint8_t *image,
    H5HL_cache_prfx_ud_t *udata)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity checks */
    HDassert(heap);
    HDassert(image);
    HDassert(udata);

    /* Check magic number */
    if(HDmemcmp(image, H5HL_MAGIC, (size_t)H5_SIZEOF_MAGIC)) 
        HGOTO_ERROR(H5E_HEAP, H5E_BADVALUE, FAIL, "bad local heap signature")
    image += H5_SIZEOF_MAGIC;

    /* Version */
    if(H5HL_VERSION != *image++) 
        HGOTO_ERROR(H5E_HEAP, H5E_VERSION, FAIL, "wrong version number in local heap")

    /* Reserved */
    image += 3;

    /* Store the prefix's address & length */
    heap->prfx_addr = udata->prfx_addr;
    heap->prfx_size = udata->sizeof_prfx;

    /* Heap data size */
    H5F_DECODE_LENGTH_LEN(image, heap->dblk_size, udata->sizeof_size);

    /* Free list head */
    H5F_DECODE_LENGTH_LEN(image, heap->free_block, udata->sizeof_size);
    if(heap->free_block != H5HL_FREE_NULL && heap->free_block >= heap->dblk_size)
        HGOTO_ERROR(H5E_HEAP, H5E_BADVALUE, FAIL, "bad heap free list")

    /* Heap data address */
    H5F_addr_decode_len(udata->sizeof_addr, &image, &(heap->dblk_addr));

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HL__hdr_deserialize() */


/*-------------------------------------------------------------------------
 * Function:    H5HL__fl_deserialize
 *
 * Purpose:     Deserialize the free list for a heap data block
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Quincey Koziol
 *              Oct 12 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HL__fl_deserialize(H5HL_t *heap)
{
    H5HL_free_t *fl = NULL, *tail = NULL;      /* Heap free block nodes */
    hsize_t free_block;                 /* Offset of free block */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

    /* check arguments */
    HDassert(heap);
    HDassert(!heap->freelist);

    /* Build free list */
    free_block = heap->free_block;
    while(H5HL_FREE_NULL != free_block) {
        const uint8_t *image;           /* Pointer into image buffer */

        /* Sanity check */
        if(free_block >= heap->dblk_size)
            HGOTO_ERROR(H5E_HEAP, H5E_BADRANGE, FAIL, "bad heap free list")

        /* Allocate & initialize free list node */
        if(NULL == (fl = H5FL_MALLOC(H5HL_free_t)))
            HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, FAIL, "memory allocation failed")
        fl->offset = (size_t)free_block;
        fl->prev = tail;
        fl->next = NULL;

        /* Decode offset of next free block */
        image = heap->dblk_image + free_block;
        H5F_DECODE_LENGTH_LEN(image, free_block, heap->sizeof_size);
        if(0 == free_block)
            HGOTO_ERROR(H5E_HEAP, H5E_BADVALUE, FAIL, "free block size is zero?")

        /* Decode length of this free block */
        H5F_DECODE_LENGTH_LEN(image, fl->size, heap->sizeof_size);
        if((fl->offset + fl->size) > heap->dblk_size)
            HGOTO_ERROR(H5E_HEAP, H5E_BADRANGE, FAIL, "bad heap free list")

        /* Append node onto list */
        if(tail)
            tail->next = fl;
        else
            heap->freelist = fl;
        tail = fl;
        fl = NULL;
    } /* end while */

done:
    if(ret_value < 0)
        if(fl)
            /* H5FL_FREE always returns NULL so we can't check for errors */
            fl = H5FL_FREE(H5HL_free_t, fl);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HL__fl_deserialize() */


/*-------------------------------------------------------------------------
 * Function:    H5HL__fl_serialize
 *
 * Purpose:     Serialize the free list for a heap data block
 *
 * Return:      Nothing (void)
 *
 * Programmer:  Quincey Koziol
 *              Oct 12 2008
 *
 *-------------------------------------------------------------------------
 */
static void
H5HL__fl_serialize(const H5HL_t *heap)
{
    H5HL_free_t *fl;                    /* Pointer to heap free list node */

    FUNC_ENTER_STATIC_NOERR

    /* check arguments */
    HDassert(heap);

    /* Serialize the free list into the heap data's image */
    for(fl = heap->freelist; fl; fl = fl->next) {
        uint8_t     *image;     /* Pointer into raw data buffer */

        HDassert(fl->offset == H5HL_ALIGN(fl->offset));
        image = heap->dblk_image + fl->offset;

        if(fl->next)
            H5F_ENCODE_LENGTH_LEN(image, fl->next->offset, heap->sizeof_size)
        else
            H5F_ENCODE_LENGTH_LEN(image, H5HL_FREE_NULL, heap->sizeof_size)

        H5F_ENCODE_LENGTH_LEN(image, fl->size, heap->sizeof_size)
    } /* end for */

    FUNC_LEAVE_NOAPI_VOID

} /* end H5HL__fl_serialize() */


/*-------------------------------------------------------------------------
 * Function:    H5HL__cache_prefix_get_initial_load_size()
 *
 * Purpose:	Return the initial size of the buffer the metadata cache should 
 *		load from file and pass to the deserialize routine.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              6/21/14
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HL__cache_prefix_get_initial_load_size(void H5_ATTR_UNUSED *_udata, size_t *image_len)
{
    FUNC_ENTER_STATIC_NOERR

    /* Sanity check */
    HDassert(image_len);

    /* Set the image length size */
    *image_len = H5HL_SPEC_READ_SIZE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5HL__cache_prefix_get_initial_load_size() */


/*-------------------------------------------------------------------------
 * Function:    H5HL__cache_prefix_get_final_load_size()
 *
 * Purpose:	Return the final size of the buffer the metadata cache should 
 *		load from file and pass to the deserialize routine.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  Quincey Koziol
 *              November 18, 2016
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HL__cache_prefix_get_final_load_size(const void *_image, size_t image_len,
    void *_udata, size_t *actual_len)
{
    const uint8_t *image = (const uint8_t *)_image;   			/* Pointer into raw data buffer */
    H5HL_cache_prfx_ud_t *udata = (H5HL_cache_prfx_ud_t *)_udata; 	/* User data for callback */
    H5HL_t heap;        	/* Local heap */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity checks */
    HDassert(image);
    HDassert(udata);
    HDassert(actual_len);
    HDassert(*actual_len == image_len);

    /* Deserialize the heap's header */
    if(H5HL__hdr_deserialize(&heap, (const uint8_t *)image, udata) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTDECODE, FAIL, "can't decode local heap header")

    /* Set the final size for the cache image */
    *actual_len = heap.prfx_size;

    /* Check if heap block exists */
    if(heap.dblk_size)
        /* Check if heap data block is contiguous with header */
        if(H5F_addr_eq((heap.prfx_addr + heap.prfx_size), heap.dblk_addr))
            /* Note that the heap should be a single object in the cache */
            *actual_len += heap.dblk_size;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HL__cache_prefix_get_final_load_size() */


/*-------------------------------------------------------------------------
 * Function:    H5HL__cache_prefix_deserialize
 *
 * Purpose:	Given a buffer containing the on disk image of the local 
 *		heap prefix, deserialize it, load its contents into a newly allocated
 *		instance of H5HL_prfx_t, and return a pointer to the new instance.
 *
 * Return:      Success:        Pointer to in core representation
 *              Failure:        NULL
 *
 * Programmer:  John Mainzer
 *              6/21/14
 *
 *-------------------------------------------------------------------------
 */
static void *
H5HL__cache_prefix_deserialize(const void *_image, size_t len, void *_udata,
    hbool_t H5_ATTR_UNUSED *dirty)
{
    H5HL_t               *heap = NULL;  /* Local heap */
    H5HL_prfx_t          *prfx = NULL;  /* Heap prefix deserialized */
    const uint8_t        *image = (const uint8_t *)_image;      /* Pointer into decoding buffer */
    H5HL_cache_prfx_ud_t *udata = (H5HL_cache_prfx_ud_t *)_udata;       /* User data for callback */
    void                 *ret_value = NULL;     /* Return value */

    FUNC_ENTER_STATIC

    /* Check arguments */
    HDassert(image);
    HDassert(len > 0);
    HDassert(udata);
    HDassert(udata->sizeof_size > 0);
    HDassert(udata->sizeof_addr > 0);
    HDassert(udata->sizeof_prfx > 0);
    HDassert(H5F_addr_defined(udata->prfx_addr));
    HDassert(dirty);

    /* Allocate space in memory for the heap */
    if(NULL == (heap = H5HL__new(udata->sizeof_size, udata->sizeof_addr, udata->sizeof_prfx)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, NULL, "can't allocate local heap structure");

    /* Deserialize the heap's header */
    if(H5HL__hdr_deserialize(heap, (const uint8_t *)image, udata) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTDECODE, NULL, "can't decode local heap header")

    /* Allocate the heap prefix */
    if(NULL == (prfx = H5HL__prfx_new(heap)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, NULL, "can't allocate local heap prefix");

    /* Check if heap block exists */
    if(heap->dblk_size) {
        /* Check if heap data block is contiguous with header */
        if(H5F_addr_eq((heap->prfx_addr + heap->prfx_size), heap->dblk_addr)) {
            /* Note that the heap should be a single object in the cache */
            heap->single_cache_obj = TRUE;

            /* Allocate space for the heap data image */
            if(NULL == (heap->dblk_image = H5FL_BLK_MALLOC(lheap_chunk, heap->dblk_size)))
                HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, NULL, "memory allocation failed")

            /* Set image to the start of the data block.  This is necessary
             * because there may be a gap between the used portion of the
             * prefix and the data block due to alignment constraints. */
            image = ((const uint8_t *)_image) + heap->prfx_size;

            /* Copy the heap data from the speculative read buffer */
            HDmemcpy(heap->dblk_image, image, heap->dblk_size);

            /* Build free list */
            if(H5HL__fl_deserialize(heap) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTINIT, NULL, "can't initialize free list")
        } /* end if */
        else
            /* Note that the heap should _NOT_ be a single 
             * object in the cache 
             */
            heap->single_cache_obj = FALSE;
    } /* end if */

    /* Set return value */
    ret_value = prfx;

done:
    /* Release the [possibly partially initialized] local heap on errors */
    if(!ret_value) {
        if(prfx) {
            if(FAIL == H5HL__prfx_dest(prfx))
                HGOTO_ERROR(H5E_HEAP, H5E_CANTRELEASE, NULL, "unable to destroy local heap prefix");
        } /* end if */
        else {
            if(heap && FAIL == H5HL__dest(heap))
                HGOTO_ERROR(H5E_HEAP, H5E_CANTRELEASE, NULL, "unable to destroy local heap");
        } /* end else */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HL__cache_prefix_deserialize() */


/*-------------------------------------------------------------------------
 * Function:    H5HL__cache_prefix_image_len
 *
 * Purpose:	Return the on disk image size of a local heap prefix to the 
 *		metadata cache via the image_len.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              6/21/14
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HL__cache_prefix_image_len(const void *_thing, size_t *image_len)
{
    const H5HL_prfx_t *prfx = (const H5HL_prfx_t *)_thing;  /* Pointer to local heap prefix to query */

    FUNC_ENTER_STATIC_NOERR

    /* Check arguments */
    HDassert(prfx);
    HDassert(prfx->cache_info.magic == H5C__H5C_CACHE_ENTRY_T_MAGIC);
    HDassert(prfx->cache_info.type == H5AC_LHEAP_PRFX);
    HDassert(image_len);

    /* Set the prefix's size */
    *image_len = prfx->heap->prfx_size;

    /* If the heap is stored as a single object, add in the 
     * data block size also 
     */
    if(prfx->heap->single_cache_obj)
        *image_len += prfx->heap->dblk_size;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5HL__cache_prefix_image_len() */


/*-------------------------------------------------------------------------
 * Function:    H5HL__cache_prefix_serialize
 *
 * Purpose:	Given a pointer to an instance of H5HL_prfx_t and an 
 *		appropriately sized buffer, serialize the contents of the 
 *		instance for writing to disk, and copy the serialized data 
 *		into the buffer.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              7/21/14
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HL__cache_prefix_serialize(const H5F_t *f, void *_image, size_t len,
    void *_thing)
{
    H5HL_prfx_t *prfx = (H5HL_prfx_t *)_thing;  /* Pointer to local heap prefix to query */
    H5HL_t      *heap;          /* Pointer to the local heap */
    uint8_t     *image = (uint8_t *)_image;     /* Pointer into image buffer */
    size_t       buf_size;      /* expected size of the image buffer */

    FUNC_ENTER_STATIC_NOERR

    /* Check arguments */
    HDassert(f);
    HDassert(image);
    HDassert(prfx);
    HDassert(prfx->cache_info.magic == H5C__H5C_CACHE_ENTRY_T_MAGIC);
    HDassert(prfx->cache_info.type == H5AC_LHEAP_PRFX);
    HDassert(H5F_addr_eq(prfx->cache_info.addr, prfx->heap->prfx_addr));
    HDassert(prfx->heap);

    /* Get the pointer to the heap */
    heap = prfx->heap;
    HDassert(heap);

    /* Compute the buffer size */
    buf_size = heap->prfx_size;
    if(heap->single_cache_obj)
        buf_size += heap->dblk_size;
    HDassert(len == buf_size);

    /* Update the free block value from the free list */
    heap->free_block = heap->freelist ? heap->freelist->offset : H5HL_FREE_NULL;

    /* Serialize the heap prefix */
    HDmemcpy(image, H5HL_MAGIC, (size_t)H5_SIZEOF_MAGIC);
    image += H5_SIZEOF_MAGIC;
    *image++ = H5HL_VERSION;
    *image++ = 0;       /*reserved*/
    *image++ = 0;       /*reserved*/
    *image++ = 0;       /*reserved*/
    H5F_ENCODE_LENGTH_LEN(image, heap->dblk_size, heap->sizeof_size);
    H5F_ENCODE_LENGTH_LEN(image, heap->free_block, heap->sizeof_size);
    H5F_addr_encode_len(heap->sizeof_addr, &image, heap->dblk_addr);

    /* Check if the local heap is a single object in cache */
    if(heap->single_cache_obj) {
        if((size_t)(image - (uint8_t *)_image) < heap->prfx_size) {
            size_t gap;         /* Size of gap between prefix and data block */

            /* Set image to the start of the data block.  This is necessary 
             * because there may be a gap between the used portion of 
             * the prefix and the data block due to alignment constraints. 
             */
            gap = heap->prfx_size - (size_t)(image - (uint8_t *)_image);
            HDmemset(image, 0, gap);
            image += gap;
        } /* end if */

        /* Serialize the free list into the heap data's image */
        H5HL__fl_serialize(heap);

        /* Copy the heap data block into the cache image */
        HDmemcpy(image, heap->dblk_image, heap->dblk_size);

        /* Sanity check */
        HDassert((size_t)(image - (uint8_t *)_image) + heap->dblk_size == len);
    } /* end if */
    else {
        /* Sanity check */
        HDassert((size_t)(image - (uint8_t *)_image) <= len);

        /* Clear rest of local heap */
        HDmemset(image, 0, len - (size_t)(image - (uint8_t *)_image));
    } /* end else */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5HL__cache_prefix_serialize() */


/*-------------------------------------------------------------------------
 * Function:    H5HL__cache_prefix_free_icr
 *
 * Purpose:	Free the supplied in core representation of a local heap 
 *		prefix.
 *
 *		Note that this function handles the partially initialize prefix 
 *		from a failed speculative load attempt.  See comments below for 
 *		details.
 *
 * Note:	The metadata cache sets the object's cache_info.magic to
 *		H5C__H5C_CACHE_ENTRY_T_BAD_MAGIC before calling a free_icr
 *		callback (checked in assert).
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              6/21/14
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HL__cache_prefix_free_icr(void *_thing)
{
    H5HL_prfx_t *prfx = (H5HL_prfx_t *)_thing;  /* Pointer to local heap prefix to query */
    herr_t       ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    /* Check arguments */
    HDassert(prfx);
    HDassert(prfx->cache_info.magic == H5C__H5C_CACHE_ENTRY_T_BAD_MAGIC);
    HDassert(prfx->cache_info.type == H5AC_LHEAP_PRFX);
    HDassert(H5F_addr_eq(prfx->cache_info.addr, prfx->heap->prfx_addr));

    /* Destroy local heap prefix */
    if(H5HL__prfx_dest(prfx) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTRELEASE, FAIL, "can't destroy local heap prefix")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HL__cache_prefix_free_icr() */


/*-------------------------------------------------------------------------
 * Function:    H5HL__cache_datablock_get_initial_load_size()
 *
 * Purpose:	Tell the metadata cache how large a buffer to read from 
 *		file when loading a datablock.  In this case, we simply lookup
 *		the correct value in the user data, and return it in *image_len.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              6/21/14
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HL__cache_datablock_get_initial_load_size(void *_udata, size_t *image_len)
{
    H5HL_t *heap = (H5HL_t *)_udata;    /* User data for callback */

    FUNC_ENTER_STATIC_NOERR

    /* Check arguments */
    HDassert(heap);
    HDassert(heap->dblk_size > 0);
    HDassert(image_len);

    /* Set the image length size */
    *image_len = heap->dblk_size;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5HL__cache_datablock_get_initial_load_size() */


/*-------------------------------------------------------------------------
 * Function:    H5HL__cache_datablock_deserialize
 *
 * Purpose:	Given a buffer containing the on disk image of a local 
 *		heap data block, deserialize it, load its contents into a newly allocated
 *		instance of H5HL_dblk_t, and return a pointer to the new instance.
 *
 * Return:      Success:        Pointer to in core representation
 *              Failure:        NULL
 *
 * Programmer:  John Mainzer
 *              6/21/14
 *
 *-------------------------------------------------------------------------
 */
static void *
H5HL__cache_datablock_deserialize(const void *image, size_t len, void *_udata,
    hbool_t H5_ATTR_UNUSED *dirty)
{
    H5HL_dblk_t          *dblk = NULL;          /* Local heap data block deserialized */
    H5HL_t *heap = (H5HL_t *)_udata;            /* User data for callback */
    void                 *ret_value = NULL;     /* Return value */

    FUNC_ENTER_STATIC

    /* Check arguments */
    HDassert(image);
    HDassert(len > 0);
    HDassert(heap);
    HDassert(heap->dblk_size == len);
    HDassert(!heap->single_cache_obj);
    HDassert(NULL == heap->dblk);
    HDassert(dirty);

    /* Allocate space in memory for the heap data block */
    if(NULL == (dblk = H5HL__dblk_new(heap)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, NULL, "memory allocation failed");

    /* Check for heap still retaining image */
    if(NULL == heap->dblk_image) {
        /* Allocate space for the heap data image */
        if(NULL == (heap->dblk_image = H5FL_BLK_MALLOC(lheap_chunk, heap->dblk_size)))
            HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, NULL, "can't allocate data block image buffer");

        /* copy the datablock from the read buffer */
        HDmemcpy(heap->dblk_image, image, len);

        /* Build free list */
        if(FAIL == H5HL__fl_deserialize(heap))
            HGOTO_ERROR(H5E_HEAP, H5E_CANTINIT, NULL, "can't initialize free list");
    } /* end if */

    /* Set return value */
    ret_value = dblk;

done:
    /* Release the [possibly partially initialized] local heap on errors */
    if(!ret_value && dblk)
        if(FAIL == H5HL__dblk_dest(dblk))
            HGOTO_ERROR(H5E_HEAP, H5E_CANTRELEASE, NULL, "unable to destroy local heap data block");

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HL__cache_datablock_deserialize() */


/*-------------------------------------------------------------------------
 * Function:    H5HL__cache_datablock_image_len
 *
 * Purpose:	Return the size of the on disk image of the datablock.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              6/21/14
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HL__cache_datablock_image_len(const void *_thing, size_t *image_len)
{
    const H5HL_dblk_t *dblk = (const H5HL_dblk_t *)_thing;    /* Pointer to the local heap data block */

    FUNC_ENTER_STATIC_NOERR

    /* Check arguments */
    HDassert(dblk);
    HDassert(dblk->cache_info.magic == H5C__H5C_CACHE_ENTRY_T_MAGIC);
    HDassert(dblk->cache_info.type == H5AC_LHEAP_DBLK);
    HDassert(dblk->heap);
    HDassert(dblk->heap->dblk_size > 0);
    HDassert(image_len);

    *image_len = dblk->heap->dblk_size;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5HL__cache_datablock_image_len() */


/*-------------------------------------------------------------------------
 * Function:    H5HL__cache_datablock_serialize
 *
 * Purpose:	Serialize the supplied datablock, and copy the serialized
 *		image into the supplied image buffer.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              6/21/14
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HL__cache_datablock_serialize(const H5F_t *f, void *image, size_t len,
    void *_thing)
{
    H5HL_t      *heap;          /* Pointer to the local heap */
    H5HL_dblk_t *dblk = (H5HL_dblk_t *)_thing;  /* Pointer to the local heap data block */

    FUNC_ENTER_STATIC_NOERR

    /* Check arguments */
    HDassert(f);
    HDassert(image);
    HDassert(dblk);
    HDassert(dblk->cache_info.magic == H5C__H5C_CACHE_ENTRY_T_MAGIC);
    HDassert(dblk->cache_info.type == H5AC_LHEAP_DBLK);
    HDassert(dblk->heap);
    heap = dblk->heap;
    HDassert(heap->dblk_size == len);
    HDassert(!heap->single_cache_obj);

    /* Update the free block value from the free list */
    heap->free_block = heap->freelist ? heap->freelist->offset : H5HL_FREE_NULL;

    /* Serialize the free list into the heap data's image */
    H5HL__fl_serialize(heap);

    /* Copy the heap's data block into the cache's image */
    HDmemcpy(image, heap->dblk_image, heap->dblk_size);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5HL__cache_datablock_serialize() */


/*-------------------------------------------------------------------------
 * Function:	H5HL__cache_datablock_notify
 *
 * Purpose:	This function is used to create and destroy pinned
 *		relationships between datablocks and their prefix parent.
 *
 * Return:	Success:	SUCCEED
 *		Failure:	FAIL
 *
 * Programmer:	Quincey Koziol
 *		November 19, 2016
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5HL__cache_datablock_notify(H5C_notify_action_t action, void *_thing)
{
    H5HL_dblk_t *dblk = (H5HL_dblk_t *)_thing;  /* Pointer to the local heap data block */
    herr_t      	 ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(dblk);

    switch(action) {
        case H5AC_NOTIFY_ACTION_AFTER_INSERT:
	    /* do nothing */
	    break;

        case H5AC_NOTIFY_ACTION_AFTER_LOAD:
            /* Sanity checks */
            HDassert(dblk->heap);
            HDassert(dblk->heap->prfx);

            /* Pin the heap's prefix */
            if(FAIL == H5AC_pin_protected_entry(dblk->heap->prfx))
                HGOTO_ERROR(H5E_HEAP, H5E_CANTPIN, FAIL, "unable to pin local heap prefix")
            break;

	case H5AC_NOTIFY_ACTION_AFTER_FLUSH:
        case H5AC_NOTIFY_ACTION_ENTRY_DIRTIED:
        case H5AC_NOTIFY_ACTION_ENTRY_CLEANED:
        case H5AC_NOTIFY_ACTION_CHILD_DIRTIED:
        case H5AC_NOTIFY_ACTION_CHILD_CLEANED:
        case H5AC_NOTIFY_ACTION_CHILD_UNSERIALIZED:
        case H5AC_NOTIFY_ACTION_CHILD_SERIALIZED:
	    /* do nothing */
	    break;

        case H5AC_NOTIFY_ACTION_BEFORE_EVICT:
            /* Sanity checks */
            HDassert(dblk->heap);
            HDassert(dblk->heap->prfx);

            /* Unpin the local heap prefix */
            if(FAIL == H5AC_unpin_entry(dblk->heap->prfx))
                HGOTO_ERROR(H5E_HEAP, H5E_CANTUNPIN, FAIL, "unable to unpin local heap prefix")
            break;

        default:
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unknown action from metadata cache")
            break;
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HL__cache_datablock_notify() */


/*-------------------------------------------------------------------------
 * Function:    H5HL__cache_datablock_free_icr
 *
 * Purpose:	Free the in memory representation of the supplied local heap data block.
 *
 * Note:	The metadata cache sets the object's cache_info.magic to
 *		H5C__H5C_CACHE_ENTRY_T_BAD_MAGIC before calling a free_icr
 *		callback (checked in assert).
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              6/21/14
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HL__cache_datablock_free_icr(void *_thing)
{
    H5HL_dblk_t *dblk = (H5HL_dblk_t *)_thing; /* Pointer to the local heap data block */
    herr_t       ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    /* Check arguments */
    HDassert(dblk);
    HDassert(dblk->cache_info.magic == H5C__H5C_CACHE_ENTRY_T_BAD_MAGIC);
    HDassert(dblk->cache_info.type == H5AC_LHEAP_DBLK);

    /* Destroy the data block */
    if(H5HL__dblk_dest(dblk) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "unable to destroy local heap data block")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HL__cache_datablock_free_icr() */

