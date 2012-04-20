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
 * Programmer:	Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              Tuesday, May  2, 2006
 *
 * Purpose:	Free space tracking functions.
 *
 * Note:	(Used to be in the H5HFflist.c file, prior to the date above)
 *
 */

/****************/
/* Module Setup */
/****************/

#define H5FS_PACKAGE		/*suppress error about including H5FSpkg  */

/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5ACprivate.h"        /* Metadata cache                       */
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FSpkg.h"		/* File free space			*/
#include "H5MFprivate.h"	/* File memory management		*/

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

/* Section info routines */
static herr_t H5FS_sinfo_free_sect_cb(void *item, void *key, void *op_data);
static herr_t H5FS_sinfo_free_node_cb(void *item, void *key, void *op_data);


/*********************/
/* Package Variables */
/*********************/

/* Declare a free list to manage the H5FS_section_class_t sequence information */
H5FL_SEQ_DEFINE(H5FS_section_class_t);

/* Declare a free list to manage the H5FS_t struct */
H5FL_DEFINE(H5FS_t);


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:	H5FS_create
 *
 * Purpose:	Allocate & initialize file free space info
 *
 * Return:	Success:	Pointer to free space structure
 *
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, March  7, 2006
 *
 * Modifications:
 *	Vailin Choi, July 29th, 2008
 *	  Add two more parameters for handling alignment: alignment & threshhold
 *
 *-------------------------------------------------------------------------
 */
H5FS_t *
H5FS_create(H5F_t *f, hid_t dxpl_id, haddr_t *fs_addr, const H5FS_create_t *fs_create,
    size_t nclasses, const H5FS_section_class_t *classes[], void *cls_init_udata, hsize_t alignment, hsize_t threshold)
{
    H5FS_t *fspace = NULL;      /* New free space structure */
    H5FS_t *ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5FS_create, NULL)
#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: Creating free space manager, nclasses = %Zu\n", FUNC, nclasses);
#endif /* H5FS_DEBUG */

    /* Check arguments. */
    HDassert(fs_create->shrink_percent);
    HDassert(fs_create->shrink_percent < fs_create->expand_percent);
    HDassert(fs_create->max_sect_size);
    HDassert(nclasses == 0 || classes);

    /*
     * Allocate free space structure
     */
    if(NULL == (fspace = H5FS_new(nclasses, classes, cls_init_udata)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for free space free list")

    /* Initialize creation information for free space manager */
    fspace->client = fs_create->client;
    fspace->shrink_percent = fs_create->shrink_percent;
    fspace->expand_percent = fs_create->expand_percent;
    fspace->max_sect_addr = fs_create->max_sect_addr;
    fspace->max_sect_size = fs_create->max_sect_size;

    fspace->alignment = alignment;
    fspace->threshold = threshold;

    /* Check if the free space tracker is supposed to be persistant */
    if(fs_addr) {
        /* Allocate space for the free space header */
        if(HADDR_UNDEF == (fspace->addr = H5MF_alloc(f, H5FD_MEM_FSPACE_HDR, dxpl_id, (hsize_t)H5FS_HEADER_SIZE(f))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "file allocation failed for free space header")

        /* Cache the new free space header (pinned) */
        if(H5AC_set(f, dxpl_id, H5AC_FSPACE_HDR, fspace->addr, fspace, H5AC__PIN_ENTRY_FLAG) < 0)
            HGOTO_ERROR(H5E_FSPACE, H5E_CANTINIT, NULL, "can't add free space header to cache")

        /* Return free space header address to caller, if desired */
        *fs_addr = fspace->addr;
    } /* end if */

    /* Set the reference count to 1, since we inserted the entry in the cache pinned */
    fspace->rc = 1;

    /* Set the return value */
    ret_value = fspace;
#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: fspace = %p, fspace->addr = %a\n", FUNC, fspace, fspace->addr);
#endif /* H5FS_DEBUG */

done:
    if(!ret_value && fspace)
        if(H5FS_hdr_dest(fspace) < 0)
            HDONE_ERROR(H5E_FSPACE, H5E_CANTFREE, NULL, "unable to destroy free space header")

#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: Leaving, ret_value = %d\n", FUNC, ret_value);
#endif /* H5FS_DEBUG */
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5FS_create() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_open
 *
 * Purpose:	Open an existing file free space info structure on disk
 *
 * Return:	Success:	Pointer to free space structure
 *
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, May  2, 2006
 *
 * Modfications:
 *
 *	Vailin Choi, July 29th, 2008
 *	  Add two more parameters for handling alignment: alignment & threshhold
 *
 *-------------------------------------------------------------------------
 */
H5FS_t *
H5FS_open(H5F_t *f, hid_t dxpl_id, haddr_t fs_addr, size_t nclasses,
    const H5FS_section_class_t *classes[], void *cls_init_udata, hsize_t alignment, hsize_t threshold)
{
    H5FS_t *fspace = NULL;      /* New free space structure */
    H5FS_hdr_cache_ud_t cache_udata; /* User-data for metadata cache callback */
    H5FS_t *ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5FS_open, NULL)
#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: Opening free space manager, fs_addr = %a, nclasses = %Zu\n", FUNC, fs_addr, nclasses);
#endif /* H5FS_DEBUG */

    /* Check arguments. */
    HDassert(H5F_addr_defined(fs_addr));
    HDassert(nclasses);
    HDassert(classes);

    /* Initialize user data for protecting the free space manager */
    cache_udata.f = f;
    cache_udata.nclasses = nclasses;
    cache_udata.classes = classes;
    cache_udata.cls_init_udata = cls_init_udata;
    cache_udata.addr = fs_addr;

    /* Protect the free space header */
    if(NULL == (fspace = (H5FS_t *)H5AC_protect(f, dxpl_id, H5AC_FSPACE_HDR, fs_addr, &cache_udata, H5AC_READ)))
        HGOTO_ERROR(H5E_FSPACE, H5E_CANTPROTECT, NULL, "unable to load free space header")
#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: fspace->sect_addr = %a\n", FUNC, fspace->sect_addr);
HDfprintf(stderr, "%s: fspace->sect_size = %Hu\n", FUNC, fspace->sect_size);
HDfprintf(stderr, "%s: fspace->alloc_sect_size = %Hu\n", FUNC, fspace->alloc_sect_size);
HDfprintf(stderr, "%s: fspace->sinfo = %p\n", FUNC, fspace->sinfo);
HDfprintf(stderr, "%s: fspace->rc = %u\n", FUNC, fspace->rc);
#endif /* H5FS_DEBUG */

    /* Increment the reference count on the free space manager header */
    HDassert(fspace->rc <= 1);
    if(H5FS_incr(fspace) < 0)
        HGOTO_ERROR(H5E_FSPACE, H5E_CANTINC, NULL, "unable to increment ref. count on free space header")

    fspace->alignment = alignment;
    fspace->threshold = threshold;

    /* Unlock free space header */
    if(H5AC_unprotect(f, dxpl_id, H5AC_FSPACE_HDR, fs_addr, fspace, H5AC__NO_FLAGS_SET) < 0)
        HGOTO_ERROR(H5E_FSPACE, H5E_CANTUNPROTECT, NULL, "unable to release free space header")

    /* Set return value */
    ret_value = fspace;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5FS_open() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_delete
 *
 * Purpose:	Delete a free space manager on disk
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, May 30, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS_delete(H5F_t *f, hid_t dxpl_id, haddr_t fs_addr)
{
    H5FS_t *fspace = NULL;              /* Free space header loaded from file */
    H5FS_hdr_cache_ud_t cache_udata; /* User-data for metadata cache callback */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5FS_delete, FAIL)
#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: Deleting free space manager, fs_addr = %a\n", FUNC, fs_addr);
#endif /* H5FS_DEBUG */

    /* Check arguments. */
    HDassert(f);
    HDassert(H5F_addr_defined(fs_addr));

    /* Initialize user data for protecting the free space manager */
    /* (no class information necessary for delete) */
    cache_udata.f = f;
    cache_udata.nclasses = 0;
    cache_udata.classes = NULL;
    cache_udata.cls_init_udata = NULL;
    cache_udata.addr = fs_addr;

    /* Protect the free space header */
    if(NULL == (fspace = (H5FS_t *)H5AC_protect(f, dxpl_id, H5AC_FSPACE_HDR, fs_addr, &cache_udata, H5AC_WRITE)))
        HGOTO_ERROR(H5E_FSPACE, H5E_CANTPROTECT, FAIL, "unable to protect free space header")

    /* Sanity check */
    HDassert(fspace->sinfo == NULL);

    /* Delete serialized section storage, if there are any */
#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: fspace->sect_addr = %a\n", FUNC, fspace->sect_addr);
#endif /* H5FS_DEBUG */
    if(fspace->serial_sect_count > 0) {
        unsigned sinfo_status = 0;      /* Free space section info's status in the metadata cache */

        /* Sanity check */
        HDassert(H5F_addr_defined(fspace->sect_addr));
        HDassert(fspace->alloc_sect_size > 0);

        /* Check the free space section info's status in the metadata cache */
        if(H5AC_get_entry_status(f, fspace->sect_addr, &sinfo_status) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTGET, FAIL, "unable to check metadata cache status for free space section info")

        /* If the free space section info is in the cache, expunge it now */
        if(sinfo_status & H5AC_ES__IN_CACHE) {
            /* Sanity checks on direct block */
            HDassert(!(sinfo_status & H5AC_ES__IS_PINNED));
            HDassert(!(sinfo_status & H5AC_ES__IS_PROTECTED));

#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: Expunging free space section info from cache\n", FUNC);
#endif /* H5FS_DEBUG */
            /* Evict the free space section info from the metadata cache */
            /* (Free file space) */
            if(H5AC_expunge_entry(f, dxpl_id, H5AC_FSPACE_SINFO, fspace->sect_addr, H5AC__FREE_FILE_SPACE_FLAG) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTREMOVE, FAIL, "unable to remove free space section info from cache")
#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: Done expunging free space section info from cache\n", FUNC);
#endif /* H5FS_DEBUG */
        } /* end if */
        else {
            /* Release the space in the file */
            if(H5MF_xfree(f, H5FD_MEM_FSPACE_SINFO, dxpl_id, fspace->sect_addr, fspace->alloc_sect_size) < 0)
                HGOTO_ERROR(H5E_FSPACE, H5E_CANTFREE, FAIL, "unable to release free space sections")
        } /* end else */
    } /* end if */

done:
    if(fspace && H5AC_unprotect(f, dxpl_id, H5AC_FSPACE_HDR, fs_addr, fspace, H5AC__DELETED_FLAG | H5AC__FREE_FILE_SPACE_FLAG) < 0)
        HDONE_ERROR(H5E_FSPACE, H5E_CANTUNPROTECT, FAIL, "unable to release free space header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5FS_delete() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_close
 *
 * Purpose:	Destroy & deallocate free list structure, serializing sections
 *              in the bins
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, March  7, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS_close(H5F_t *f, hid_t dxpl_id, H5FS_t *fspace)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5FS_close, FAIL)

    /* Check arguments. */
    HDassert(f);
    HDassert(fspace);
#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: Entering, fspace = %p, fspace->addr = %a, fspace->sinfo = %p\n", FUNC, fspace, fspace->addr, fspace->sinfo);
#endif /* H5FS_DEBUG */

    /* Check if section info is valid */
    /* (i.e. the header "owns" the section info and it's not in the cache) */
    if(fspace->sinfo) {
#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: fspace->tot_sect_count = %Hu, fspace->serial_sect_count = %Hu, fspace->sect_addr = %a, fspace->rc = %u\n", FUNC, fspace->tot_sect_count, fspace->serial_sect_count, fspace->sect_addr, fspace->rc);
HDfprintf(stderr, "%s: fspace->alloc_sect_size = %Hu, fspace->sect_size = %Hu\n", FUNC, fspace->alloc_sect_size, fspace->sect_size);
#endif /* H5FS_DEBUG */
        /* If there are sections to serialize, update them */
        /* (if the free space manager is persistant) */
        if(fspace->serial_sect_count > 0 && H5F_addr_defined(fspace->addr)) {
#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: Real sections to store in file\n", FUNC);
#endif /* H5FS_DEBUG */
            if(fspace->sinfo->dirty) {
                /* Check if the section info is "floating" */
                if(!H5F_addr_defined(fspace->sect_addr)) {
                    /* Sanity check */
                    HDassert(fspace->sect_size > 0);

                    /* Allocate space for the section info in file */
                    if(HADDR_UNDEF == (fspace->sect_addr = H5MF_alloc(f, H5FD_MEM_FSPACE_SINFO, dxpl_id, fspace->sect_size)))
                        HGOTO_ERROR(H5E_FSPACE, H5E_NOSPACE, FAIL, "file allocation failed for free space sections")
                    fspace->alloc_sect_size = (size_t)fspace->sect_size;

                    /* Mark free space header as dirty */
                    if(H5AC_mark_entry_dirty(fspace) < 0)
                        HGOTO_ERROR(H5E_FSPACE, H5E_CANTMARKDIRTY, FAIL, "unable to mark free space header as dirty")
                } /* end if */
            } /* end if */
	    else
		/* Sanity check that section info has address */
		HDassert(H5F_addr_defined(fspace->sect_addr));

            /* Cache the free space section info */
            if(H5AC_set(f, dxpl_id, H5AC_FSPACE_SINFO, fspace->sect_addr, fspace->sinfo, H5AC__NO_FLAGS_SET) < 0)
                HGOTO_ERROR(H5E_FSPACE, H5E_CANTINIT, FAIL, "can't add free space sections to cache")
        } /* end if */
        else {
#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: NOT storing section info in file\n", FUNC);
#endif /* H5FS_DEBUG */
            /* Check if space for the section info is allocated */
            if(H5F_addr_defined(fspace->sect_addr)) {
                /* Sanity check */
                /* (section info should only be in the file if the header is */
                HDassert(H5F_addr_defined(fspace->addr));

#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: Section info allocated though\n", FUNC);
#endif /* H5FS_DEBUG */
                /* Check if the section info is for the free space in the file */
                /* (NOTE: This is the "bootstrapping" special case for the
                 *      free space manager, to avoid freeing the space for the
                 *      section info and re-creating it as a section in the
                 *      manager. -QAK)
                 */
                if(fspace->client == H5FS_CLIENT_FILE_ID) {
                    htri_t status;          /* "can absorb" status for section into */

#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: Section info is for file free space\n", FUNC);
#endif /* H5FS_DEBUG */
                    /* Try to shrink the file or absorb the section info into a block aggregator */
                    if((status = H5MF_try_shrink(f, H5FD_MEM_FSPACE_SINFO, dxpl_id, fspace->sect_addr, fspace->alloc_sect_size)) < 0)
                        HGOTO_ERROR(H5E_FSPACE, H5E_CANTMERGE, FAIL, "can't check for absorbing section info")
                    else if(status == FALSE) {
                        /* Section info can't "go away", but it's free.  Allow
                         *      header to record it
                         */
#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: Section info can't 'go away', header will own it\n", FUNC);
#endif /* H5FS_DEBUG */
                    } /* end if */
                    else {
#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: Section info went 'go away'\n", FUNC);
#endif /* H5FS_DEBUG */
                        /* Reset section info in header */
                        fspace->sect_addr = HADDR_UNDEF;
                        fspace->alloc_sect_size = 0;

                        /* Mark free space header as dirty */
                        if(H5AC_mark_entry_dirty(fspace) < 0)
                            HGOTO_ERROR(H5E_FSPACE, H5E_CANTMARKDIRTY, FAIL, "unable to mark free space header as dirty")
                    } /* end else */
                } /* end if */
                else {
                    haddr_t old_sect_addr = fspace->sect_addr;   /* Previous location of section info in file */
                    hsize_t old_alloc_sect_size = fspace->alloc_sect_size;       /* Previous size of section info in file */

#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: Section info is NOT for file free space\n", FUNC);
#endif /* H5FS_DEBUG */
                    /* Reset section info in header */
                    fspace->sect_addr = HADDR_UNDEF;
                    fspace->alloc_sect_size = 0;

                    /* Mark free space header as dirty */
                    if(H5AC_mark_entry_dirty(fspace) < 0)
                        HGOTO_ERROR(H5E_FSPACE, H5E_CANTMARKDIRTY, FAIL, "unable to mark free space header as dirty")

                    /* Free previous serialized sections disk space */
                    if(H5MF_xfree(f, H5FD_MEM_FSPACE_SINFO, dxpl_id, old_sect_addr, old_alloc_sect_size) < 0)
                        HGOTO_ERROR(H5E_FSPACE, H5E_CANTFREE, FAIL, "unable to free free space sections")
                } /* end if */
            } /* end else */

            /* Destroy section info */
            if(H5FS_sinfo_dest(fspace->sinfo) < 0)
                HGOTO_ERROR(H5E_FSPACE, H5E_CANTCLOSEOBJ, FAIL, "unable to destroy free space section info")
        } /* end else */

        /* Reset the header's pointer to the section info */
        fspace->sinfo = NULL;
    } /* end if */
    else {
        /* Just sanity checks... */
        if(fspace->serial_sect_count > 0)
            /* Sanity check that section info has address */
            HDassert(H5F_addr_defined(fspace->sect_addr));
        else
            /* Sanity check that section info doesn't have address */
            HDassert(!H5F_addr_defined(fspace->sect_addr));
    } /* end else */

    /* Decrement the reference count on the free space manager header */
    if(H5FS_decr(fspace) < 0)
        HGOTO_ERROR(H5E_FSPACE, H5E_CANTDEC, FAIL, "unable to decrement ref. count on free space header")

done:
#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: Leaving, ret_value = %d, fspace->rc = %u\n", FUNC, ret_value, fspace->rc);
#endif /* H5FS_DEBUG */
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5FS_close() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_new
 *
 * Purpose:	Create new free space manager structure
 *
 * Return:	Success:	non-NULL, pointer to new free space manager struct
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Monday, July 31, 2006
 *
 *-------------------------------------------------------------------------
 */
H5FS_t *
H5FS_new(size_t nclasses, const H5FS_section_class_t *classes[],
    void *cls_init_udata)
{
    H5FS_t *fspace = NULL;      /* Free space manager */
    size_t u;                   /* Local index variable */
    H5FS_t *ret_value;          /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5FS_new)

    /* Check arguments. */
    HDassert(nclasses == 0 || (nclasses > 0 && classes));

    /*
     * Allocate free space structure
     */
    if(NULL == (fspace = H5FL_CALLOC(H5FS_t)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for free space free list")

    /* Set immutable free list parameters */
    fspace->nclasses = nclasses;
    if(nclasses > 0) {
        if(NULL == (fspace->sect_cls = H5FL_SEQ_MALLOC(H5FS_section_class_t, nclasses)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for free space section class array")

        /* Initialize the section classes for this free space list */
        for(u = 0; u < nclasses; u++) {
            /* Make certain that section class type can be used as an array index into this array */
            HDassert(u == classes[u]->type);

            /* Copy the class information into the free space manager */
            HDmemcpy(&fspace->sect_cls[u], classes[u], sizeof(H5FS_section_class_t));

            /* Call the class initialization routine, if there is one */
            if(fspace->sect_cls[u].init_cls)
                if((fspace->sect_cls[u].init_cls)(&fspace->sect_cls[u], cls_init_udata) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, NULL, "unable to initialize section class")

            /* Determine maximum class-specific serialization size for each section */
            if(fspace->sect_cls[u].serial_size > fspace->max_cls_serial_size)
                fspace->max_cls_serial_size = fspace->sect_cls[u].serial_size;
        } /* end for */
    } /* end if */

    /* Initialize non-zero information for new free space manager */
    fspace->addr = HADDR_UNDEF;
    fspace->sect_addr = HADDR_UNDEF;

    /* Set return value */
    ret_value = fspace;

done:
    if(!ret_value)
        if(fspace) {
            /* Should probably call the class 'term' callback for all classes
             *  that have had their 'init' callback called... -QAK
             */
            if(fspace->sect_cls)
                fspace->sect_cls = (H5FS_section_class_t *)H5FL_SEQ_FREE(H5FS_section_class_t, fspace->sect_cls);
            fspace = H5FL_FREE(H5FS_t, fspace);
        } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5FS_new() */


/*-------------------------------------------------------------------------
 * Function:    H5FS_size
 *
 * Purpose:     Collect meta storage info used by the free space manager
 *
 * Return:      Success:        non-negative
 *              Failure:        negative
 *
 * Programmer:  Vailin Choi
 *              June 19, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS_size(const H5F_t *f, const H5FS_t *fspace, hsize_t *meta_size)
{
    FUNC_ENTER_NOAPI_NOFUNC(H5FS_size)

    /*
     * Check arguments.
     */
    HDassert(f);
    HDassert(fspace);
    HDassert(meta_size);

    /* Get the free space size info */
    *meta_size += H5FS_HEADER_SIZE(f) + (fspace->sinfo ? fspace->sect_size : fspace->alloc_sect_size);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5FS_size() */


/*-------------------------------------------------------------------------
 * Function:    H5FS_incr
 *
 * Purpose:     Increment reference count on free space header
 *
 * Return:      Success:        non-negative
 *              Failure:        negative
 *
 * Programmer:  Quincey Koziol
 *              February  7, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS_incr(H5FS_t *fspace)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5FS_incr, FAIL)
#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: Entering, fpace->addr = %a, fspace->rc = %u\n", FUNC, fspace->addr, fspace->rc);
#endif /* H5FS_DEBUG */

    /*
     * Check arguments.
     */
    HDassert(fspace);

    /* Check if we should pin the header in the cache */
    if(fspace->rc == 0 && H5F_addr_defined(fspace->addr))
        if(H5AC_pin_protected_entry(fspace) < 0)
            HGOTO_ERROR(H5E_FSPACE, H5E_CANTPIN, FAIL, "unable to pin free space header")

    /* Increment reference count on header */
    fspace->rc++;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FS_incr() */


/*-------------------------------------------------------------------------
 * Function:    H5FS_decr
 *
 * Purpose:     Decrement reference count on free space header
 *
 * Return:      Success:        non-negative
 *              Failure:        negative
 *
 * Programmer:  Quincey Koziol
 *              February  7, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS_decr(H5FS_t *fspace)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5FS_decr, FAIL)
#ifdef H5FS_DEBUG
HDfprintf(stderr, "%s: Entering, fpace->addr = %a, fspace->rc = %u\n", FUNC, fspace->addr, fspace->rc);
#endif /* H5FS_DEBUG */

    /*
     * Check arguments.
     */
    HDassert(fspace);

    /* Decrement reference count on header */
    fspace->rc--;

    /* Check if we should unpin the header in the cache */
    if(fspace->rc == 0) {
        if(H5F_addr_defined(fspace->addr)) {
            if(H5AC_unpin_entry(fspace) < 0)
                HGOTO_ERROR(H5E_FSPACE, H5E_CANTUNPIN, FAIL, "unable to unpin free space header")
        } /* end if */
        else {
            if(H5FS_hdr_dest(fspace) < 0)
                HGOTO_ERROR(H5E_FSPACE, H5E_CANTCLOSEOBJ, FAIL, "unable to destroy free space header")
        } /* end else */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FS_decr() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_dirty
 *
 * Purpose:	Mark free space header as dirty
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Feb 14 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS_dirty(H5FS_t *fspace)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5FS_dirty)
#ifdef QAK
HDfprintf(stderr, "%s: Marking free space header as dirty\n", FUNC);
#endif /* QAK */

    /* Sanity check */
    HDassert(fspace);

    /* Check if the free space manager is persistant */
    if(H5F_addr_defined(fspace->addr))
        /* Mark header as dirty in cache */
        if(H5AC_mark_entry_dirty(fspace) < 0)
            HGOTO_ERROR(H5E_FSPACE, H5E_CANTMARKDIRTY, FAIL, "unable to mark free space header as dirty")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FS_dirty() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_hdr_dest
 *
 * Purpose:	Destroys a free space header in memory.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May  2 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS_hdr_dest(H5FS_t *fspace)
{
    unsigned u;                 /* Local index variable */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5FS_hdr_dest)

    /*
     * Check arguments.
     */
    HDassert(fspace);

    /* Terminate the section classes for this free space list */
    for(u = 0; u < fspace->nclasses ; u++) {
        /* Call the class termination routine, if there is one */
        if(fspace->sect_cls[u].term_cls)
            if((fspace->sect_cls[u].term_cls)(&fspace->sect_cls[u]) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "unable to finalize section class")
    } /* end for */

    /* Release the memory for the free space section classes */
    if(fspace->sect_cls)
        fspace->sect_cls = (H5FS_section_class_t *)H5FL_SEQ_FREE(H5FS_section_class_t, fspace->sect_cls);

    /* Free free space info */
    fspace = H5FL_FREE(H5FS_t, fspace);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FS_hdr_dest() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_sinfo_free_sect_cb
 *
 * Purpose:	Free a size-tracking node for a bin
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Saturday, March 11, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FS_sinfo_free_sect_cb(void *_sect, void UNUSED *key, void *op_data)
{
    H5FS_section_info_t *sect = (H5FS_section_info_t *)_sect;   /* Section to free */
    const H5FS_sinfo_t *sinfo = (const H5FS_sinfo_t *)op_data;     /* Free space manager for section */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5FS_sinfo_free_sect_cb)

    HDassert(sect);
    HDassert(sinfo);

    /* Call the section's class 'free' method on the section */
    (*sinfo->fspace->sect_cls[sect->type].free)(sect);

    FUNC_LEAVE_NOAPI(0)
}   /* H5FS_sinfo_free_sect_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_sinfo_free_node_cb
 *
 * Purpose:	Free a size-tracking node for a bin
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Saturday, March 11, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FS_sinfo_free_node_cb(void *item, void UNUSED *key, void *op_data)
{
    H5FS_node_t *fspace_node = (H5FS_node_t *)item;       /* Temporary pointer to free space list node */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5FS_sinfo_free_node_cb)

    HDassert(fspace_node);
    HDassert(op_data);

    /* Release the skip list for sections of this size */
    H5SL_destroy(fspace_node->sect_list, H5FS_sinfo_free_sect_cb, op_data);

    /* Release free space list node */
    fspace_node = H5FL_FREE(H5FS_node_t, fspace_node);

    FUNC_LEAVE_NOAPI(0)
}   /* H5FS_sinfo_free_node_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_sinfo_dest
 *
 * Purpose:	Destroys a free space section info in memory.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		July 31 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS_sinfo_dest(H5FS_sinfo_t *sinfo)
{
    unsigned u;                 /* Local index variable */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5FS_sinfo_dest)

    /*
     * Check arguments.
     */
    HDassert(sinfo);
    HDassert(sinfo->fspace);
    HDassert(sinfo->bins);

    /* Clear out lists of nodes */
    for(u = 0; u < sinfo->nbins; u++)
        if(sinfo->bins[u].bin_list) {
            H5SL_destroy(sinfo->bins[u].bin_list, H5FS_sinfo_free_node_cb, sinfo);
            sinfo->bins[u].bin_list = NULL;
        } /* end if */

    /* Release bins for skip lists */
    sinfo->bins = H5FL_SEQ_FREE(H5FS_bin_t, sinfo->bins);

    /* Release skip list for merging sections */
    if(sinfo->merge_list)
        if(H5SL_close(sinfo->merge_list) < 0)
            HGOTO_ERROR(H5E_FSPACE, H5E_CANTCLOSEOBJ, FAIL, "can't destroy section merging skip list")

    /* Decrement the reference count on free space header */
    /* (make certain this is last action with section info, to allow for header
     *  disappearing immediately)
     */
    sinfo->fspace->sinfo = NULL;
    if(H5FS_decr(sinfo->fspace) < 0)
        HGOTO_ERROR(H5E_FSPACE, H5E_CANTDEC, FAIL, "unable to decrement ref. count on free space header")
    sinfo->fspace = NULL;

    /* Release free space section info */
    sinfo = H5FL_FREE(H5FS_sinfo_t, sinfo);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FS_sinfo_dest() */

#ifdef H5FS_DEBUG_ASSERT

/*-------------------------------------------------------------------------
 * Function:	H5FS_assert
 *
 * Purpose:	Verify that the free space manager is mostly sane
 *
 * Return:	Non-negative on success, negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jul 17 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS_assert(const H5FS_t *fspace)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5FS_assert)
#ifndef QAK
HDfprintf(stderr, "%s: fspace->tot_sect_count = %Hu\n", "H5FS_assert", fspace->tot_sect_count);
#endif /* QAK */

    /* Checks for section info, if it's available */
    if(fspace->sinfo) {
        /* Sanity check sections */
        H5FS_sect_assert(fspace);

        /* General assumptions about the section size counts */
        HDassert(fspace->sinfo->tot_size_count >= fspace->sinfo->serial_size_count);
        HDassert(fspace->sinfo->tot_size_count >= fspace->sinfo->ghost_size_count);
    } /* end if */

    /* General assumptions about the section counts */
    HDassert(fspace->tot_sect_count >= fspace->serial_sect_count);
    HDassert(fspace->tot_sect_count >= fspace->ghost_sect_count);
    HDassert(fspace->tot_sect_count == (fspace->serial_sect_count + fspace->ghost_sect_count));
#ifdef QAK
    HDassert(fspace->serial_sect_count > 0 || fspace->ghost_sect_count == 0);
#endif /* QAK */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5FS_assert() */
#endif /* H5FS_DEBUG_ASSERT */
