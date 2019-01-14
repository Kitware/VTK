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
 * Created:		H5HGcache.c
 *			Feb  5 2008
 *			Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:		Implement global heap metadata cache methods.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#include "H5HGmodule.h"         /* This source code file is part of the H5HG module */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"		/* File access				*/
#include "H5HGpkg.h"		/* Global heaps				*/
#include "H5MFprivate.h"	/* File memory management		*/
#include "H5MMprivate.h"	/* Memory management			*/


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

/* Metadata cache callbacks */
static herr_t H5HG__cache_heap_get_initial_load_size(void *udata, size_t *image_len);
static herr_t H5HG__cache_heap_get_final_load_size(const void *_image,
    size_t image_len, void *udata, size_t *actual_len);
static void *H5HG__cache_heap_deserialize(const void *image, size_t len,
    void *udata, hbool_t *dirty); 
static herr_t H5HG__cache_heap_image_len(const void *thing, size_t *image_len);
static herr_t H5HG__cache_heap_serialize(const H5F_t *f, void *image,
    size_t len, void *thing); 
static herr_t H5HG__cache_heap_free_icr(void *thing);

/* Prefix deserialization */
static herr_t H5HG__hdr_deserialize(H5HG_heap_t *heap, const uint8_t *image,
    const H5F_t *f);


/*********************/
/* Package Variables */
/*********************/

/* H5HG inherits cache-like properties from H5AC */
const H5AC_class_t H5AC_GHEAP[1] = {{
    H5AC_GHEAP_ID,                      /* Metadata client ID */
    "global heap",                      /* Metadata client name (for debugging) */
    H5FD_MEM_GHEAP,                     /* File space memory type for client */
    H5AC__CLASS_SPECULATIVE_LOAD_FLAG,  /* Client class behavior flags */
    H5HG__cache_heap_get_initial_load_size,     /* 'get_initial_load_size' callback */
    H5HG__cache_heap_get_final_load_size, /* 'get_final_load_size' callback */
    NULL, 				/* 'verify_chksum' callback */
    H5HG__cache_heap_deserialize,       /* 'deserialize' callback */
    H5HG__cache_heap_image_len,         /* 'image_len' callback */
    NULL,                               /* 'pre_serialize' callback */
    H5HG__cache_heap_serialize,         /* 'serialize' callback */
    NULL,                               /* 'notify' callback */
    H5HG__cache_heap_free_icr,          /* 'free_icr' callback */
    NULL,                               /* 'fsf_size' callback */
}};


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:    H5HG__hdr_deserialize()
 *
 * Purpose:	Decode a global heap's header
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
H5HG__hdr_deserialize(H5HG_heap_t *heap, const uint8_t *image, const H5F_t *f)
{
    herr_t ret_value = SUCCEED;                 /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(heap);
    HDassert(image);
    HDassert(f);

    /* Magic number */
    if(HDmemcmp(image, H5HG_MAGIC, (size_t)H5_SIZEOF_MAGIC))
        HGOTO_ERROR(H5E_HEAP, H5E_BADVALUE, FAIL, "bad global heap collection signature")
    image += H5_SIZEOF_MAGIC;

    /* Version */
    if(H5HG_VERSION != *image++)
        HGOTO_ERROR(H5E_HEAP, H5E_VERSION, FAIL, "wrong version number in global heap")

    /* Reserved */
    image += 3;

    /* Size */
    H5F_DECODE_LENGTH(f, image, heap->size);
    HDassert(heap->size >= H5HG_MINSIZE);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HG__hdr_deserialize() */


/*-------------------------------------------------------------------------
 * Function:    H5HG__cache_heap_get_initial_load_size()
 *
 * Purpose:	Return the initial speculative read size to the metadata 
 *		cache.  This size will be used in the initial attempt to read 
 *		the global heap.  If this read is too small, the cache will 
 *		try again with the correct value obtained from 
 *		H5HG__cache_get_final_load_size().
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              7/27/14
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HG__cache_heap_get_initial_load_size(void H5_ATTR_UNUSED *_udata, size_t *image_len)
{
    FUNC_ENTER_STATIC_NOERR

    /* Sanity check */
    HDassert(image_len);

    /* Set the image length size */
    *image_len = (size_t)H5HG_MINSIZE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5HG__cache_heap_get_initial_load_size() */


/*-------------------------------------------------------------------------
 * Function:    H5HG__cache_heap_get_initial_load_size()
 *
 * Purpose:	Return the final read size for a speculatively ready heap to
 *		the metadata cache.
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
H5HG__cache_heap_get_final_load_size(const void *image, size_t image_len,
    void *udata, size_t *actual_len)
{
    H5HG_heap_t heap;                   /* Global heap */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(image);
    HDassert(udata);
    HDassert(actual_len);
    HDassert(*actual_len == image_len);
    HDassert(image_len == H5HG_MINSIZE);

    /* Deserialize the heap's header */
    if(H5HG__hdr_deserialize(&heap, (const uint8_t *)image, (const H5F_t *)udata) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTDECODE, FAIL, "can't decode global heap prefix")

    /* Set the final size for the cache image */
    *actual_len = heap.size;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HG__cache_heap_get_final_load_size() */


/*-------------------------------------------------------------------------
 * Function:    H5HG__cache_heap_deserialize
 *
 * Purpose:	Given a buffer containing the on disk image of the global 
 *		heap, deserialize it, load its contents into a newly allocated
 *		instance of H5HG_heap_t, and return a pointer to the new instance.
 *
 * Return:      Success:        Pointer to in core representation
 *              Failure:        NULL
 *
 * Programmer:  John Mainzer
 *              7/27/14
 *
 *-------------------------------------------------------------------------
 */
static void *
H5HG__cache_heap_deserialize(const void *_image, size_t len, void *_udata,
    hbool_t H5_ATTR_UNUSED *dirty)
{
    H5F_t       *f = (H5F_t *)_udata;   /* File pointer -- obtained from user data */
    H5HG_heap_t *heap = NULL;   /* New global heap */
    uint8_t     *image;         /* Pointer to image to decode */
    size_t       max_idx = 0;   /* Maximum heap object index seen */
    size_t       nalloc;        /* Number of objects allocated */
    void        *ret_value = NULL;      /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity checks */
    HDassert(_image);
    HDassert(len >= (size_t)H5HG_MINSIZE);
    HDassert(f);
    HDassert(dirty);

    /* Allocate a new global heap */
    if(NULL == (heap = H5FL_CALLOC(H5HG_heap_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
    heap->shared = H5F_SHARED(f);
    if(NULL == (heap->chunk = H5FL_BLK_MALLOC(gheap_chunk, len)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Copy the image buffer into the newly allocate chunk */
    HDmemcpy(heap->chunk, _image, len);

    /* Deserialize the heap's header */
    if(H5HG__hdr_deserialize(heap, (const uint8_t *)heap->chunk, f) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTDECODE, NULL, "can't decode global heap header")

    /* Decode each object */
    image = heap->chunk + H5HG_SIZEOF_HDR(f);
    nalloc = H5HG_NOBJS(f, heap->size);

    /* Calloc the obj array because the file format spec makes no guarantee
     * about the order of the objects, and unused slots must be set to zero.
     */
    if(NULL == (heap->obj = H5FL_SEQ_CALLOC(H5HG_obj_t, nalloc)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
    heap->nalloc = nalloc;

    while(image < (heap->chunk + heap->size)) {
        if((image + H5HG_SIZEOF_OBJHDR(f)) > (heap->chunk + heap->size)) {
            /*
             * The last bit of space is too tiny for an object header, so
             * we assume that it's free space.
             */
            HDassert(NULL == heap->obj[0].begin);
            heap->obj[0].size = (size_t)(((const uint8_t *)heap->chunk + heap->size) - image);
            heap->obj[0].begin = image;
            image += heap->obj[0].size;
        } /* end if */
        else {
            size_t need = 0;
            unsigned idx;
            uint8_t *begin = image;

            UINT16DECODE(image, idx);

            /* Check if we need more room to store heap objects */
            if(idx >= heap->nalloc) {
                size_t new_alloc;       /* New allocation number */
                H5HG_obj_t *new_obj;    /* New array of object   descriptions          */

                /* Determine the new number of objects to index */
                new_alloc = MAX(heap->nalloc * 2, (idx + 1));
                HDassert(idx < new_alloc);

                /* Reallocate array of objects */
                if(NULL == (new_obj = H5FL_SEQ_REALLOC(H5HG_obj_t, heap->obj, new_alloc)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

                /* Clear newly allocated space */
                HDmemset(&new_obj[heap->nalloc], 0, (new_alloc - heap->nalloc) * sizeof(heap->obj[0]));

                /* Update heap information */
                heap->nalloc = new_alloc;
                heap->obj = new_obj;
                HDassert(heap->nalloc > heap->nused);
            } /* end if */

            UINT16DECODE(image, heap->obj[idx].nrefs);
            image += 4; /*reserved*/
            H5F_DECODE_LENGTH(f, image, heap->obj[idx].size);
            heap->obj[idx].begin = begin;

            /*
             * The total storage size includes the size of the object 
             * header and is zero padded so the next object header is 
             * properly aligned. The entire obj array was calloc'ed, 
             * so no need to zero the space here. The last bit of space 
             * is the free space object whose size is never padded and 
             * already includes the object header.
             */
            if(idx > 0) {
                need = H5HG_SIZEOF_OBJHDR(f) + H5HG_ALIGN(heap->obj[idx].size);
                if(idx > max_idx)
                    max_idx = idx;
            } /* end if */
            else
                need = heap->obj[idx].size;

            image = begin + need;
        } /* end else */
    } /* end while */

    /* Sanity checks */
    HDassert(image == heap->chunk + heap->size);
    HDassert(H5HG_ISALIGNED(heap->obj[0].size));

    /* Set the next index value to use */
    if(max_idx > 0)
        heap->nused = max_idx + 1;
    else
        heap->nused = 1;

    /* Sanity check */
    HDassert(max_idx < heap->nused);

    /* Add the new heap to the CWFS list for the file */
    if(H5F_cwfs_add(f, heap) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTINIT, NULL, "unable to add global heap collection to file's CWFS")

    ret_value = heap;

done:
    if(!ret_value && heap)
        if(H5HG_free(heap) < 0)
            HDONE_ERROR(H5E_HEAP, H5E_CANTFREE, NULL, "unable to destroy global heap collection")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HG__cache_heap_deserialize() */


/*-------------------------------------------------------------------------
 * Function:    H5HG__cache_heap_image_len
 *
 * Purpose:	Return the on disk image size of the global heap to the 
 *		metadata cache via the image_len.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              7/27/14
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HG__cache_heap_image_len(const void *_thing, size_t *image_len)
{
    const H5HG_heap_t *heap = (const H5HG_heap_t *)_thing;

    FUNC_ENTER_STATIC_NOERR

    /* Sanity checks */
    HDassert(heap);
    HDassert(heap->cache_info.magic == H5C__H5C_CACHE_ENTRY_T_MAGIC);
    HDassert(heap->cache_info.type == H5AC_GHEAP);
    HDassert(heap->size >= H5HG_MINSIZE);
    HDassert(image_len);

    *image_len = heap->size;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5HG__cache_heap_image_len() */


/*-------------------------------------------------------------------------
 * Function:    H5HG__cache_heap_serialize
 *
 * Purpose:	Given an appropriately sized buffer and an instance of 
 *		H5HG_heap_t, serialize the global heap for writing to file,
 *		and copy the serialized version into the buffer.
 *
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              7/27/14
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HG__cache_heap_serialize(const H5F_t *f, void *image, size_t len,
    void *_thing)
{
    H5HG_heap_t *heap = (H5HG_heap_t *)_thing;

    FUNC_ENTER_STATIC_NOERR

    HDassert(f);
    HDassert(image);
    HDassert(heap);
    HDassert(heap->cache_info.magic == H5C__H5C_CACHE_ENTRY_T_MAGIC);
    HDassert(heap->cache_info.type == H5AC_GHEAP);
    HDassert(heap->size == len);
    HDassert(heap->chunk);

    /* copy the image into the buffer */
    HDmemcpy(image, heap->chunk, len);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5HG__cache_heap_serialize() */


/*-------------------------------------------------------------------------
 * Function:    H5HG__cache_heap_free_icr
 *
 * Purpose:	Free the in memory representation of the supplied global heap.
 *
 * Note:	The metadata cache sets the object's cache_info.magic to
 *		H5C__H5C_CACHE_ENTRY_T_BAD_MAGIC before calling a free_icr
 *		callback (checked in assert).
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              7/27/14
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HG__cache_heap_free_icr(void *_thing)
{
    H5HG_heap_t *heap = (H5HG_heap_t *)_thing;
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity checks */
    HDassert(heap);
    HDassert(heap->cache_info.magic == H5C__H5C_CACHE_ENTRY_T_BAD_MAGIC);
    HDassert(heap->cache_info.type == H5AC_GHEAP);

    /* Destroy global heap collection */
    if(H5HG_free(heap) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "unable to destroy global heap collection")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HG__cache_heap_free_icr() */

