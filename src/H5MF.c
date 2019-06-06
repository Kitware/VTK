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
 * Created:             H5MF.c
 *                      Jul 11 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             File memory management functions.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5F_FRIEND		/*suppress error about including H5Fpkg	  */
#define H5FS_FRIEND		/*suppress error about including H5Fpkg	  */
#include "H5MFmodule.h"         /* This source code file is part of the H5MF module */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fpkg.h"             /* File access				*/
#include "H5FSpkg.h"            /* File free space                      */
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MFpkg.h"		/* File memory management		*/
#include "H5VMprivate.h"	/* Vectors and arrays 			*/


/****************/
/* Local Macros */
/****************/

#define H5MF_FSPACE_SHRINK      80              /* Percent of "normal" size to shrink serialized free space size */
#define H5MF_FSPACE_EXPAND      120             /* Percent of "normal" size to expand serialized free space size */

/******************/
/* Local Typedefs */
/******************/

/* Enum for kind of free space section+aggregator merging allowed for a file */
typedef enum {
    H5MF_AGGR_MERGE_SEPARATE,           /* Everything in separate free list */
    H5MF_AGGR_MERGE_DICHOTOMY,          /* Metadata in one free list and raw data in another */
    H5MF_AGGR_MERGE_TOGETHER            /* Metadata & raw data in one free list */
} H5MF_aggr_merge_t;

/* User data for section info iterator callback for iterating over free space sections */
typedef struct {
    H5F_sect_info_t *sects;     /* section info to be retrieved */
    size_t sect_count;          /* # of sections requested */
    size_t sect_idx;            /* the current count of sections */
} H5MF_sect_iter_ud_t;


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

/* Allocator routines */
static haddr_t H5MF__alloc_pagefs(H5F_t *f, H5FD_mem_t alloc_type, hsize_t size);

/* "File closing" routines */
static herr_t H5MF__close_aggrfs(H5F_t *f);
static herr_t H5MF__close_pagefs(H5F_t *f);
static herr_t H5MF__close_shrink_eoa(H5F_t *f);

/* General routines */
static herr_t H5MF__get_free_sects(H5F_t *f, H5FS_t *fspace, H5MF_sect_iter_ud_t *sect_udata, size_t *nums);
static hbool_t H5MF__fsm_type_is_self_referential(H5F_t *f, H5F_mem_page_t fsm_type);
static hbool_t H5MF__fsm_is_self_referential(H5F_t *f, H5FS_t *fspace);

/* Free-space type manager routines */
static herr_t H5MF__create_fstype(H5F_t *f, H5F_mem_page_t type);
static herr_t H5MF__close_fstype(H5F_t *f, H5F_mem_page_t type);
static herr_t H5MF__delete_fstype(H5F_t *f, H5F_mem_page_t type);
static herr_t H5MF__close_delete_fstype(H5F_t *f, H5F_mem_page_t type);

/* Callbacks */
static herr_t H5MF__sects_cb(H5FS_section_info_t *_sect, void *_udata);


/*********************/
/* Package Variables */
/*********************/

/* Package initialization variable */
hbool_t H5_PKG_INIT_VAR = FALSE;


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:    H5MF_init_merge_flags
 *
 * Purpose:     Initialize the free space section+aggregator merge flags
 *              for the file.
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:  Quincey Koziol
 *              Friday, February  1, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MF_init_merge_flags(H5F_t *f)
{
    H5MF_aggr_merge_t mapping_type;     /* Type of free list mapping */
    H5FD_mem_t type;                    /* Memory type for iteration */
    hbool_t all_same;                   /* Whether all the types map to the same value */
    herr_t ret_value = SUCCEED;        	/* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->lf);

    /* Iterate over all the free space types to determine if sections of that type
     *  can merge with the metadata or small 'raw' data aggregator
     */
    all_same = TRUE;
    for(type = H5FD_MEM_DEFAULT; type < H5FD_MEM_NTYPES; H5_INC_ENUM(H5FD_mem_t, type))
        /* Check for any different type mappings */
        if(f->shared->fs_type_map[type] != f->shared->fs_type_map[H5FD_MEM_DEFAULT]) {
            all_same = FALSE;
            break;
        } /* end if */

    /* Check for all allocation types mapping to the same free list type */
    if(all_same) {
        if(f->shared->fs_type_map[H5FD_MEM_DEFAULT] == H5FD_MEM_DEFAULT)
            mapping_type = H5MF_AGGR_MERGE_SEPARATE;
        else
            mapping_type = H5MF_AGGR_MERGE_TOGETHER;
    } /* end if */
    else {
        /* Check for raw data mapping into same list as metadata */
        if(f->shared->fs_type_map[H5FD_MEM_DRAW] == f->shared->fs_type_map[H5FD_MEM_SUPER])
            mapping_type = H5MF_AGGR_MERGE_SEPARATE;
        else {
            hbool_t all_metadata_same;              /* Whether all metadata go in same free list */

            /* One or more allocation type don't map to the same free list type */
            /* Check if all the metadata allocation types map to the same type */
            all_metadata_same = TRUE;
            for(type = H5FD_MEM_SUPER; type < H5FD_MEM_NTYPES; H5_INC_ENUM(H5FD_mem_t, type))
                /* Skip checking raw data free list mapping */
                /* (global heap is treated as raw data) */
                if(type != H5FD_MEM_DRAW && type != H5FD_MEM_GHEAP) {
                    /* Check for any different type mappings */
                    if(f->shared->fs_type_map[type] != f->shared->fs_type_map[H5FD_MEM_SUPER]) {
                        all_metadata_same = FALSE;
                        break;
                    } /* end if */
                } /* end if */

            /* Check for all metadata on same free list */
            if(all_metadata_same)
                mapping_type = H5MF_AGGR_MERGE_DICHOTOMY;
            else
                mapping_type = H5MF_AGGR_MERGE_SEPARATE;
        } /* end else */
    } /* end else */

    /* Based on mapping type, initialize merging flags for each free list type */
    switch(mapping_type) {
        case H5MF_AGGR_MERGE_SEPARATE:
            /* Don't merge any metadata together */
            HDmemset(f->shared->fs_aggr_merge, 0, sizeof(f->shared->fs_aggr_merge));

            /* Check if merging raw data should be allowed */
            /* (treat global heaps as raw data) */
            if(H5FD_MEM_DRAW == f->shared->fs_type_map[H5FD_MEM_DRAW] ||
                    H5FD_MEM_DEFAULT == f->shared->fs_type_map[H5FD_MEM_DRAW]) {
                f->shared->fs_aggr_merge[H5FD_MEM_DRAW] = H5F_FS_MERGE_RAWDATA;
                f->shared->fs_aggr_merge[H5FD_MEM_GHEAP] = H5F_FS_MERGE_RAWDATA;
	    } /* end if */
            break;

        case H5MF_AGGR_MERGE_DICHOTOMY:
            /* Merge all metadata together (but not raw data) */
            HDmemset(f->shared->fs_aggr_merge, H5F_FS_MERGE_METADATA, sizeof(f->shared->fs_aggr_merge));

            /* Allow merging raw data allocations together */
            /* (treat global heaps as raw data) */
            f->shared->fs_aggr_merge[H5FD_MEM_DRAW] = H5F_FS_MERGE_RAWDATA;
            f->shared->fs_aggr_merge[H5FD_MEM_GHEAP] = H5F_FS_MERGE_RAWDATA;
            break;

        case H5MF_AGGR_MERGE_TOGETHER:
            /* Merge all allocation types together */
            HDmemset(f->shared->fs_aggr_merge, (H5F_FS_MERGE_METADATA | H5F_FS_MERGE_RAWDATA), sizeof(f->shared->fs_aggr_merge));
            break;

        default:
            HGOTO_ERROR(H5E_RESOURCE, H5E_BADVALUE, FAIL, "invalid mapping type")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_init_merge_flags() */


/*-------------------------------------------------------------------------
 * Function:    H5MF__alloc_to_fs_type
 *
 * Purpose:     Map "alloc_type" to the free-space manager type
 *
 * Return:      Success:        non-negative
 *              Failure:        negative
 *
 * Programmer: Vailin Choi; Nov 2016
 *
 *-------------------------------------------------------------------------
 */
void
H5MF__alloc_to_fs_type(H5F_t *f, H5FD_mem_t alloc_type, hsize_t size, H5F_mem_page_t *fs_type)
{
    FUNC_ENTER_PACKAGE_NOERR

    HDassert(f);
    HDassert(fs_type);

    if(H5F_PAGED_AGGR(f)) { /* paged aggregation */
        if(size >= f->shared->fs_page_size) {
            if(H5F_HAS_FEATURE(f, H5FD_FEAT_PAGED_AGGR)) { /* multi or split driver */
                /* For non-contiguous address space, map to large size free-space manager for each alloc_type */
                if(H5FD_MEM_DEFAULT == f->shared->fs_type_map[alloc_type]) 
                    *fs_type = (H5F_mem_page_t) (alloc_type + (H5FD_MEM_NTYPES - 1));
                else
                    *fs_type = (H5F_mem_page_t) (f->shared->fs_type_map[alloc_type] + (H5FD_MEM_NTYPES - 1));
            } /* end if */
            else
                /* For contiguous address space, map to generic large size free-space manager */
                *fs_type = H5F_MEM_PAGE_GENERIC; /* H5F_MEM_PAGE_SUPER */
        } /* end if */
        else
            *fs_type = (H5F_mem_page_t)H5MF_ALLOC_TO_FS_AGGR_TYPE(f, alloc_type);
    } /* end if */
    else /* non-paged aggregation */
        *fs_type = (H5F_mem_page_t)H5MF_ALLOC_TO_FS_AGGR_TYPE(f, alloc_type);

    FUNC_LEAVE_NOAPI_VOID
} /* end H5MF__alloc_to_fs_type() */


/*-------------------------------------------------------------------------
 * Function:	H5MF__open_fstype
 *
 * Purpose:	Open an existing free space manager of TYPE for file by
 *          creating a free-space structure.
 *          Note that TYPE can be H5F_mem_page_t or H5FD_mem_t enum types.
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jan  8 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MF__open_fstype(H5F_t *f, H5F_mem_page_t type)
{
    const H5FS_section_class_t *classes[] = { /* Free space section classes implemented for file */
        H5MF_FSPACE_SECT_CLS_SIMPLE,
        H5MF_FSPACE_SECT_CLS_SMALL,
        H5MF_FSPACE_SECT_CLS_LARGE };
    hsize_t alignment;                      /* Alignment to use */
    hsize_t threshold;                      /* Threshold to use */
    H5AC_ring_t orig_ring = H5AC_RING_INV;  /* Original ring value */
    H5AC_ring_t fsm_ring;                   /* Ring of FSM */
    herr_t ret_value = SUCCEED;             /* Return value */

    FUNC_ENTER_PACKAGE_TAG(H5AC__FREESPACE_TAG)

    /*
     * Check arguments.
     */
    HDassert(f);
    if(H5F_PAGED_AGGR(f))
        HDassert(type < H5F_MEM_PAGE_NTYPES);
    else {
        HDassert((H5FD_mem_t)type < H5FD_MEM_NTYPES);
        HDassert((H5FD_mem_t)type != H5FD_MEM_NOLIST);
    } /* end else */
    HDassert(f->shared);
    HDassert(H5F_addr_defined(f->shared->fs_addr[type]));
    HDassert(f->shared->fs_state[type] == H5F_FS_STATE_CLOSED);

    /* Set up the alignment and threshold to use depending on the manager type */
    if(H5F_PAGED_AGGR(f)) {
        alignment = (type == H5F_MEM_PAGE_GENERIC) ? f->shared->fs_page_size : (hsize_t)H5F_ALIGN_DEF;
        threshold = H5F_ALIGN_THRHD_DEF;
    } /* end if */
    else {
        alignment = f->shared->alignment;
        threshold = f->shared->threshold;
    } /* end else */

    /* Set the ring type in the API context */
    if(H5MF__fsm_type_is_self_referential(f, type))
        fsm_ring = H5AC_RING_MDFSM;
    else
        fsm_ring = H5AC_RING_RDFSM;
    H5AC_set_ring(fsm_ring, &orig_ring);

    /* Open an existing free space structure for the file */
    if(NULL == (f->shared->fs_man[type] = H5FS_open(f, f->shared->fs_addr[type],
	    NELMTS(classes), classes, f, alignment, threshold)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize free space info")

    /* Set the state for the free space manager to "open", if it is now */
    if(f->shared->fs_man[type])
        f->shared->fs_state[type] = H5F_FS_STATE_OPEN;

done:
    /* Reset the ring in the API context */
    if(orig_ring != H5AC_RING_INV)
        H5AC_set_ring(orig_ring, NULL);

    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5MF__open_fstype() */


/*-------------------------------------------------------------------------
 * Function:	H5MF__create_fstype
 *
 * Purpose:	Create free space manager of TYPE for the file by creating
 *          a free-space structure
 *          Note that TYPE can be H5F_mem_page_t or H5FD_mem_t enum types.
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jan  8 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5MF__create_fstype(H5F_t *f, H5F_mem_page_t type)
{
    const H5FS_section_class_t *classes[] = { /* Free space section classes implemented for file */
        H5MF_FSPACE_SECT_CLS_SIMPLE,
        H5MF_FSPACE_SECT_CLS_SMALL,
        H5MF_FSPACE_SECT_CLS_LARGE };
    H5FS_create_t fs_create;    /* Free space creation parameters */
    hsize_t alignment;          /* Alignment to use */
    hsize_t threshold;		    /* Threshold to use */
    H5AC_ring_t orig_ring = H5AC_RING_INV;  /* Original ring value */
    H5AC_ring_t fsm_ring;                   /* Ring of FSM */
    herr_t ret_value = SUCCEED;             /* Return value */

    FUNC_ENTER_STATIC_TAG(H5AC__FREESPACE_TAG)

    /*
     * Check arguments.
     */
    HDassert(f);
    if(H5F_PAGED_AGGR(f))
        HDassert(type < H5F_MEM_PAGE_NTYPES);
    else {
        HDassert((H5FD_mem_t)type < H5FD_MEM_NTYPES);
        HDassert((H5FD_mem_t)type != H5FD_MEM_NOLIST);
    } /* end else */
    HDassert(f->shared);
    HDassert(!H5F_addr_defined(f->shared->fs_addr[type]));
    HDassert(f->shared->fs_state[type] == H5F_FS_STATE_CLOSED);

    /* Set the free space creation parameters */
    fs_create.client = H5FS_CLIENT_FILE_ID;
    fs_create.shrink_percent = H5MF_FSPACE_SHRINK;
    fs_create.expand_percent = H5MF_FSPACE_EXPAND;
    fs_create.max_sect_addr = 1 + H5VM_log2_gen((uint64_t)f->shared->maxaddr);
    fs_create.max_sect_size = f->shared->maxaddr;

    /* Set up alignment and threshold to use depending on TYPE */
    if(H5F_PAGED_AGGR(f)) {
        alignment = (type == H5F_MEM_PAGE_GENERIC) ? f->shared->fs_page_size : (hsize_t)H5F_ALIGN_DEF;
        threshold = H5F_ALIGN_THRHD_DEF;
    } /* end if */
    else {
        alignment = f->shared->alignment;
        threshold = f->shared->threshold;
    } /* end else */

    /* Set the ring type in the API context */
    if(H5MF__fsm_type_is_self_referential(f, type))
        fsm_ring = H5AC_RING_MDFSM;
    else
        fsm_ring = H5AC_RING_RDFSM;
    H5AC_set_ring(fsm_ring, &orig_ring);

    if(NULL == (f->shared->fs_man[type] = H5FS_create(f, NULL,
	    &fs_create, NELMTS(classes), classes, f, alignment, threshold)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize free space info")

    /* Set the state for the free space manager to "open", if it is now */
    if(f->shared->fs_man[type])
        f->shared->fs_state[type] = H5F_FS_STATE_OPEN;

done:
    /* Reset the ring in the API context */
    if(orig_ring != H5AC_RING_INV)
        H5AC_set_ring(orig_ring, NULL);

    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5MF__create_fstype() */


/*-------------------------------------------------------------------------
 * Function:	H5MF__start_fstype
 *
 * Purpose:	Open or create a free space manager of a given TYPE.
 *          Note that TYPE can be H5F_mem_page_t or H5FD_mem_t enum types.
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jan  8 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MF__start_fstype(H5F_t *f, H5F_mem_page_t type)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_PACKAGE_TAG(H5AC__FREESPACE_TAG)

    /*
     * Check arguments.
     */
    HDassert(f);
    HDassert(f->shared);
    if(H5F_PAGED_AGGR(f))
        HDassert(type < H5F_MEM_PAGE_NTYPES);
    else {
        HDassert((H5FD_mem_t)type < H5FD_MEM_NTYPES);
        HDassert((H5FD_mem_t)type != H5FD_MEM_NOLIST);
    } /* end else */

    /* Check if the free space manager exists already */
    if(H5F_addr_defined(f->shared->fs_addr[type])) {
        /* Open existing free space manager */
        if(H5MF__open_fstype(f, type) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTOPENOBJ, FAIL, "can't initialize file free space")
    } /* end if */
    else {
        /* Create new free space manager */
        if(H5MF__create_fstype(f, type) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTCREATE, FAIL, "can't initialize file free space")
    } /* end else */

done:
    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5MF__start_fstype() */


/*-------------------------------------------------------------------------
 * Function:    H5MF__delete_fstype
 *
 * Purpose:     Delete the free-space manager as specified by TYPE.
 *              Note that TYPE can be H5F_mem_page_t or H5FD_mem_t enum types.
 *
 * Return:      Success:        non-negative
 *              Failure:        negative
 *
 * Programmer: 	Vailin Choi; April 2013
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5MF__delete_fstype(H5F_t *f, H5F_mem_page_t type)
{
    H5AC_ring_t orig_ring = H5AC_RING_INV;      /* Original ring value */
    H5AC_ring_t fsm_ring = H5AC_RING_INV;       /* Ring of FSM */
    haddr_t tmp_fs_addr;       	        /* Temporary holder for free space manager address */
    herr_t ret_value = SUCCEED;	        /* Return value */

    FUNC_ENTER_STATIC_TAG(H5AC__FREESPACE_TAG)

    /* check args */
    HDassert(f);
    if(H5F_PAGED_AGGR(f))
        HDassert(type < H5F_MEM_PAGE_NTYPES);
    else
        HDassert((H5FD_mem_t)type < H5FD_MEM_NTYPES);
    HDassert(H5F_addr_defined(f->shared->fs_addr[type]));

    /* Put address into temporary variable and reset it */
    /* (Avoids loopback in file space freeing routine) */
    tmp_fs_addr = f->shared->fs_addr[type];
    f->shared->fs_addr[type] = HADDR_UNDEF;

    /* Shift to "deleting" state, to make certain we don't track any
     *  file space freed as a result of deleting the free space manager.
     */
    f->shared->fs_state[type] = H5F_FS_STATE_DELETING;

    /* Set the ring type in the API context */
    if(H5MF__fsm_type_is_self_referential(f, type))
        fsm_ring = H5AC_RING_MDFSM;
    else
        fsm_ring = H5AC_RING_RDFSM;
    H5AC_set_ring(fsm_ring, &orig_ring);

#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Before deleting free space manager\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG_MORE */

    /* Delete free space manager for this type */
    if(H5FS_delete(f, tmp_fs_addr) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, FAIL, "can't delete free space manager")

    /* Shift [back] to closed state */
    HDassert(f->shared->fs_state[type] == H5F_FS_STATE_DELETING);
    f->shared->fs_state[type] = H5F_FS_STATE_CLOSED;

    /* Sanity check that the free space manager for this type wasn't started up again */
    HDassert(!H5F_addr_defined(f->shared->fs_addr[type]));

done:
    /* Reset the ring in the API context */
    if(orig_ring != H5AC_RING_INV)
        H5AC_set_ring(orig_ring, NULL);

    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5MF__delete_fstype() */


/*-------------------------------------------------------------------------
 * Function:    H5MF__close_fstype
 *
 * Purpose:     Close the free space manager of TYPE for file
 *              Note that TYPE can be H5F_mem_page_t or H5FD_mem_t enum types.
 *
 * Return:      Success:        non-negative
 *              Failure:        negative
 *
 * Programmer: Vailin Choi; July 1st, 2009
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5MF__close_fstype(H5F_t *f, H5F_mem_page_t type)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC_TAG(H5AC__FREESPACE_TAG)

    /*
     * Check arguments.
     */
    HDassert(f);
    if(H5F_PAGED_AGGR(f))
        HDassert(type < H5F_MEM_PAGE_NTYPES);
    else
        HDassert((H5FD_mem_t)type < H5FD_MEM_NTYPES);
    HDassert(f->shared);
    HDassert(f->shared->fs_man[type]);
    HDassert(f->shared->fs_state[type] != H5F_FS_STATE_CLOSED);

#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Before closing free space manager\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG_MORE */

    /* Close an existing free space structure for the file */
    if(H5FS_close(f, f->shared->fs_man[type]) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't release free space info")
    f->shared->fs_man[type] = NULL;
    f->shared->fs_state[type] = H5F_FS_STATE_CLOSED;

done:
    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5MF__close_fstype() */



/*-------------------------------------------------------------------------
 * Function:    H5MF__add_sect
 *
 * Purpose:	    To add a section to the specified free-space manager.
 *
 * Return:      Success:        non-negative
 *              Failure:        negative
 *
 * Programmer:  Vailin Choi; April 2013
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MF__add_sect(H5F_t *f, H5FD_mem_t alloc_type, H5FS_t *fspace, H5MF_free_section_t *node)
{
    H5AC_ring_t orig_ring = H5AC_RING_INV;  /* Original ring value */
    H5AC_ring_t fsm_ring = H5AC_RING_INV;   /* Ring of FSM */
    H5MF_sect_ud_t udata;		            /* User data for callback */
    H5F_mem_page_t  fs_type;                /* Free space type (mapped from allocation type) */
    herr_t ret_value = SUCCEED; 	        /* Return value */

    FUNC_ENTER_PACKAGE_TAG(H5AC__FREESPACE_TAG)

    HDassert(f);
    HDassert(fspace);
    HDassert(node);

    H5MF__alloc_to_fs_type(f, alloc_type, node->sect_info.size, &fs_type);

    /* Construct user data for callbacks */
    udata.f = f;
    udata.alloc_type = alloc_type;
    udata.allow_sect_absorb = TRUE;
    udata.allow_eoa_shrink_only = FALSE;

    /* Set the ring type in the API context */
    if(H5MF__fsm_is_self_referential(f, fspace))
        fsm_ring = H5AC_RING_MDFSM;
    else
        fsm_ring = H5AC_RING_RDFSM;
    H5AC_set_ring(fsm_ring, &orig_ring);

#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: adding node, node->sect_info.addr = %a, node->sect_info.size = %Hu\n", FUNC, node->sect_info.addr, node->sect_info.size);
#endif /* H5MF_ALLOC_DEBUG_MORE */
    /* Add the section */
    if(H5FS_sect_add(f, fspace, (H5FS_section_info_t *)node, H5FS_ADD_RETURNED_SPACE, &udata) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINSERT, FAIL, "can't re-add section to file free space")

done:
    /* Reset the ring in the API context */
    if(orig_ring != H5AC_RING_INV)
        H5AC_set_ring(orig_ring, NULL);

    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5MF__add_sect() */


/*-------------------------------------------------------------------------
 * Function:    H5MF__find_sect
 *
 * Purpose:	To find a section from the specified free-space manager to fulfill the request.
 *		    If found, re-add the left-over space back to the manager.
 *
 * Return:	TRUE if a section is found to fulfill the request
 *		    FALSE if not
 *
 * Programmer:  Vailin Choi; April 2013
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5MF__find_sect(H5F_t *f, H5FD_mem_t alloc_type, hsize_t size, H5FS_t *fspace,
    haddr_t *addr)
{
    H5AC_ring_t orig_ring = H5AC_RING_INV;  /* Original ring value */
    H5AC_ring_t fsm_ring = H5AC_RING_INV;   /* Ring of FSM */
    H5MF_free_section_t *node;              /* Free space section pointer */
    htri_t ret_value = FAIL;      	    /* Whether an existing free list node was found */

    FUNC_ENTER_PACKAGE_TAG(H5AC__FREESPACE_TAG)

    HDassert(f);
    HDassert(fspace);

    /* Set the ring type in the API context */
    if(H5MF__fsm_is_self_referential(f, fspace))
        fsm_ring = H5AC_RING_MDFSM;
    else
        fsm_ring = H5AC_RING_RDFSM;
    H5AC_set_ring(fsm_ring, &orig_ring);

    /* Try to get a section from the free space manager */
    if((ret_value = H5FS_sect_find(f, fspace, size, (H5FS_section_info_t **)&node)) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "error locating free space in file")

#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: section found = %t\n", FUNC, ret_value);
#endif /* H5MF_ALLOC_DEBUG_MORE */

    /* Check for actually finding section */
    if(ret_value) {
        /* Sanity check */
        HDassert(node);

        /* Retrieve return value */
        if(addr)
            *addr = node->sect_info.addr;

        /* Check for eliminating the section */
        if(node->sect_info.size == size) {
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: freeing node\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG_MORE */

            /* Free section node */
            if(H5MF__sect_free((H5FS_section_info_t *)node) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't free simple section node")
        } /* end if */
        else {
            /* Adjust information for section */
            node->sect_info.addr += size;
            node->sect_info.size -= size;

#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: re-adding node, node->sect_info.size = %Hu\n", FUNC, node->sect_info.size);
#endif /* H5MF_ALLOC_DEBUG_MORE */

            /* Re-add the section to the free-space manager */
            if(H5MF__add_sect(f, alloc_type, fspace, node) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINSERT, FAIL, "can't re-add section to file free space")
        } /* end else */
    } /* end if */

done:
    /* Reset the ring in the API context */
    if(orig_ring != H5AC_RING_INV)
        H5AC_set_ring(orig_ring, NULL);

    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5MF__find_sect() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_alloc
 *
 * Purpose:     Allocate SIZE bytes of file memory and return the relative
 *		address where that contiguous chunk of file memory exists.
 *		The TYPE argument describes the purpose for which the storage
 *		is being requested.
 *
 * Return:      Success:        The file address of new chunk.
 *              Failure:        HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 11 1997
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5MF_alloc(H5F_t *f, H5FD_mem_t alloc_type, hsize_t size)
{
    H5AC_ring_t fsm_ring = H5AC_RING_INV;       /* free space manager ring */
    H5AC_ring_t orig_ring = H5AC_RING_INV;      /* Original ring value */
    H5F_mem_page_t  fs_type;            /* Free space type (mapped from allocation type) */
    haddr_t ret_value = HADDR_UNDEF;    /* Return value */

    FUNC_ENTER_NOAPI_TAG(H5AC__FREESPACE_TAG, HADDR_UNDEF)
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: alloc_type = %u, size = %Hu\n", FUNC, (unsigned)alloc_type, size);
#endif /* H5MF_ALLOC_DEBUG */

    /* check arguments */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->lf);
    HDassert(size > 0);

    if(f->shared->first_alloc_dealloc) {
        HDassert(! H5AC_cache_image_pending(f));
        if(H5MF_tidy_self_referential_fsm_hack(f) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, HADDR_UNDEF, "tidy of self referential fsm hack failed")
    } /* end if */

    H5MF__alloc_to_fs_type(f, alloc_type, size, &fs_type);

#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Check 1.0\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG_MORE */

    /* Set the ring type in the API context */
    if(H5MF__fsm_type_is_self_referential(f, fs_type))
        fsm_ring = H5AC_RING_MDFSM;
    else
        fsm_ring = H5AC_RING_RDFSM;
    H5AC_set_ring(fsm_ring, &orig_ring);

    /* Check if we are using the free space manager for this file */
    if(H5F_HAVE_FREE_SPACE_MANAGER(f)) {
        /* We are about to change the contents of the free space manager --
         * notify metadata cache that the associated fsm ring is
         * unsettled
         */
        if(H5AC_unsettle_ring(f, fsm_ring) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_SYSTEM, HADDR_UNDEF, "attempt to notify cache that ring is unsettled failed")

        /* Check if the free space manager for the file has been initialized */
        if(!f->shared->fs_man[fs_type] && H5F_addr_defined(f->shared->fs_addr[fs_type])) {
            /* Open the free-space manager */
            if(H5MF__open_fstype(f, fs_type) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTOPENOBJ, HADDR_UNDEF, "can't initialize file free space")
            HDassert(f->shared->fs_man[fs_type]);
        } /* end if */

        /* Search for large enough space in the free space manager */
        if(f->shared->fs_man[fs_type])
            if(H5MF__find_sect(f, alloc_type, size, f->shared->fs_man[fs_type], &ret_value) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, HADDR_UNDEF, "error locating a node")
    } /* end if */

    /* If no space is found from the free-space manager, continue further action */
    if(!H5F_addr_defined(ret_value)) {
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Check 2.0\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG_MORE */
        if(f->shared->fs_strategy == H5F_FSPACE_STRATEGY_PAGE) {
            HDassert(f->shared->fs_page_size >= H5F_FILE_SPACE_PAGE_SIZE_MIN);
            if(HADDR_UNDEF == (ret_value = H5MF__alloc_pagefs(f, alloc_type, size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, HADDR_UNDEF, "allocation failed from paged aggregation")
        } /* end if */
        else { /* For non-paged aggregation, continue further action */
            if(HADDR_UNDEF == (ret_value = H5MF_aggr_vfd_alloc(f, alloc_type, size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, HADDR_UNDEF, "allocation failed from aggr/vfd")
        } /* end else */
    } /* end if */
    HDassert(H5F_addr_defined(ret_value));
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Check 3.0\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG_MORE */

done:
    /* Reset the ring in the API context */
    if(orig_ring != H5AC_RING_INV)
        H5AC_set_ring(orig_ring, NULL);

#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Leaving: ret_value = %a, size = %Hu\n", FUNC, ret_value, size);
#endif /* H5MF_ALLOC_DEBUG */
#ifdef H5MF_ALLOC_DEBUG_DUMP
H5MF__sects_dump(f, stderr);
#endif /* H5MF_ALLOC_DEBUG_DUMP */

    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5MF_alloc() */


/*-------------------------------------------------------------------------
 * Function:    H5MF__alloc_pagefs
 *
 * Purpose:     Allocate space from either the large or small free-space manager.
 *              For "large" request:
 *                  Allocate request from VFD
 *                  Determine mis-aligned fragment and return the fragment to the
 *                  appropriate manager
 *              For "small" request:
 *                  Allocate a page from the large manager
 *                  Determine whether space is available from a mis-aligned fragment
 *                  being returned to the manager
 *              Return left-over space to the manager after fulfilling request
 *
 * Return:      Success:        The file address of new chunk.
 *              Failure:        HADDR_UNDEF
 *
 * Programmer:  Vailin Choi; Dec 2012
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5MF__alloc_pagefs(H5F_t *f, H5FD_mem_t alloc_type, hsize_t size)
{
    H5F_mem_page_t ptype;		        /* Free-space manager type */
    H5MF_free_section_t *node = NULL;  	/* Free space section pointer */
    haddr_t ret_value = HADDR_UNDEF; 	/* Return value */

    FUNC_ENTER_STATIC_TAG(H5AC__FREESPACE_TAG)

#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: alloc_type = %u, size = %Hu\n", FUNC, (unsigned)alloc_type, size);
#endif /* H5MF_ALLOC_DEBUG */

    H5MF__alloc_to_fs_type(f, alloc_type, size, &ptype);

    switch(ptype) {
	    case H5F_MEM_PAGE_GENERIC:  
	    case H5F_MEM_PAGE_LARGE_BTREE:
	    case H5F_MEM_PAGE_LARGE_DRAW:
	    case H5F_MEM_PAGE_LARGE_GHEAP:
	    case H5F_MEM_PAGE_LARGE_LHEAP:
	    case H5F_MEM_PAGE_LARGE_OHDR:
        {
            haddr_t eoa;            /* EOA for the file */
            hsize_t frag_size = 0;  /* Fragment size */

            /* Get the EOA for the file */
            if(HADDR_UNDEF == (eoa = H5F_get_eoa(f, alloc_type)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, HADDR_UNDEF, "Unable to get eoa")
            HDassert(!(eoa % f->shared->fs_page_size));

            H5MF_EOA_MISALIGN(f, (eoa+size), f->shared->fs_page_size, frag_size);

            /* Allocate from VFD */
            if(HADDR_UNDEF == (ret_value = H5F__alloc(f, alloc_type, size + frag_size, NULL, NULL)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, HADDR_UNDEF, "can't allocate file space")

            /* If there is a mis-aligned fragment at EOA */
            if(frag_size) {

                /* Start up the free-space manager */
                if(!(f->shared->fs_man[ptype]))
                    if(H5MF__start_fstype(f, ptype) < 0)
                        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, HADDR_UNDEF, "can't initialize file free space")

                /* Create free space section for the fragment */
                if(NULL == (node = H5MF__sect_new(H5MF_FSPACE_SECT_LARGE, ret_value + size, frag_size)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, HADDR_UNDEF, "can't initialize free space section")

                /* Add the fragment to the large free-space manager */
                if(H5MF__add_sect(f, alloc_type, f->shared->fs_man[ptype], node) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINSERT, HADDR_UNDEF, "can't re-add section to file free space")

                node = NULL;
            } /* end if */
        }
        break;

        case H5F_MEM_PAGE_META: 
        case H5F_MEM_PAGE_DRAW:
        case H5F_MEM_PAGE_BTREE:
        case H5F_MEM_PAGE_GHEAP:
        case H5F_MEM_PAGE_LHEAP:
        case H5F_MEM_PAGE_OHDR:
        {
            haddr_t new_page;		/* The address for the new file size page */

            /* Allocate one file space page */
            if(HADDR_UNDEF == (new_page = H5MF_alloc(f, alloc_type, f->shared->fs_page_size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, HADDR_UNDEF, "can't allocate file space")

            /* Start up the free-space manager */
            if(!(f->shared->fs_man[ptype]))
                if(H5MF__start_fstype(f, ptype) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, HADDR_UNDEF, "can't initialize file free space")
            HDassert(f->shared->fs_man[ptype]);

            if(NULL == (node = H5MF__sect_new(H5MF_FSPACE_SECT_SMALL, (new_page + size), (f->shared->fs_page_size - size))))
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, HADDR_UNDEF, "can't initialize free space section")

            /* Add the remaining space in the page to the manager */
            if(H5MF__add_sect(f, alloc_type, f->shared->fs_man[ptype], node) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINSERT, HADDR_UNDEF, "can't re-add section to file free space")

            node = NULL;

            /* Insert the new page into the Page Buffer list of new pages so 
               we don't read an empty page from disk */
            if(f->shared->page_buf != NULL && H5PB_add_new_page(f, alloc_type, new_page) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINSERT, HADDR_UNDEF, "can't add new page to Page Buffer new page list")

            ret_value = new_page;
        } 
        break;

        case H5F_MEM_PAGE_NTYPES:
        case H5F_MEM_PAGE_DEFAULT:
        default:
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, HADDR_UNDEF, "can't allocate file space: unrecognized type")
            break;
    } /* end switch */

done:
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Leaving: ret_value = %a, size = %Hu\n", FUNC, ret_value, size);
#endif /* H5MF_ALLOC_DEBUG */
#ifdef H5MF_ALLOC_DEBUG_DUMP
H5MF__sects_dump(f, stderr);
#endif /* H5MF_ALLOC_DEBUG_DUMP */

    /* Release section node, if allocated and not added to section list or merged */
    if(node)
        if(H5MF__sect_free((H5FS_section_info_t *)node) < 0)
            HDONE_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, HADDR_UNDEF, "can't free section node")

    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5MF__alloc_pagefs() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_alloc_tmp
 *
 * Purpose:     Allocate temporary space in the file
 *
 * Note:	The address returned is non-overlapping with any other address
 *		in the file and suitable for insertion into the metadata
 *		cache.
 *
 *		The address is _not_ suitable for actual file I/O and will
 *		cause an error if it is so used.
 *
 *		The space allocated with this routine should _not_ be freed,
 *		it should just be abandoned.  Calling H5MF_xfree() with space
 *              from this routine will cause an error.
 *
 * Return:      Success:        Temporary file address
 *              Failure:        HADDR_UNDEF
 *
 * Programmer:  Quincey Koziol
 *              Thursday, June  4, 2009
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5MF_alloc_tmp(H5F_t *f, hsize_t size)
{
    haddr_t eoa;                /* End of allocated space in the file */
    haddr_t ret_value = HADDR_UNDEF;    /* Return value */

    FUNC_ENTER_NOAPI(HADDR_UNDEF)
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: size = %Hu\n", FUNC, size);
#endif /* H5MF_ALLOC_DEBUG */

    /* check args */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->lf);
    HDassert(size > 0);

    /* Retrieve the 'eoa' for the file */
    if(HADDR_UNDEF == (eoa = H5F_get_eoa(f, H5FD_MEM_DEFAULT)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, HADDR_UNDEF, "driver get_eoa request failed")

    /* Compute value to return */
    ret_value = f->shared->tmp_addr - size;

    /* Check for overlap into the actual allocated space in the file */
    if(H5F_addr_le(ret_value, eoa))
	HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, HADDR_UNDEF, "driver get_eoa request failed")

    /* Adjust temporary address allocator in the file */
    f->shared->tmp_addr = ret_value;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_alloc_tmp() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_xfree
 *
 * Purpose:     Frees part of a file, making that part of the file
 *              available for reuse.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 17 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MF_xfree(H5F_t *f, H5FD_mem_t alloc_type, haddr_t addr, hsize_t size)
{
    H5F_mem_page_t  fs_type;            /* Free space type (mapped from allocation type) */
    H5MF_free_section_t *node = NULL;   /* Free space section pointer */
    unsigned ctype;			/* section class type */
    H5AC_ring_t orig_ring = H5AC_RING_INV;  /* Original ring value */
    H5AC_ring_t fsm_ring;               /* Ring of FSM */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_TAG(H5AC__FREESPACE_TAG, FAIL)
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Entering - alloc_type = %u, addr = %a, size = %Hu\n", FUNC, (unsigned)alloc_type, addr, size);
#endif /* H5MF_ALLOC_DEBUG */

    /* check arguments */
    HDassert(f);
    if(!H5F_addr_defined(addr) || 0 == size)
        HGOTO_DONE(SUCCEED)
    HDassert(addr != 0);        /* Can't deallocate the superblock :-) */

    if(f->shared->first_alloc_dealloc) {
        HDassert(!H5AC_cache_image_pending(f));
        if(H5MF_tidy_self_referential_fsm_hack(f) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, FAIL, "tidy of self referential fsm hack failed")
    } /* end if */

    H5MF__alloc_to_fs_type(f, alloc_type, size, &fs_type);

    /* Set the ring type in the API context */
    if(H5MF__fsm_type_is_self_referential(f, fs_type))
        fsm_ring = H5AC_RING_MDFSM;
    else
        fsm_ring = H5AC_RING_RDFSM;
    H5AC_set_ring(fsm_ring, &orig_ring);

    /* we are about to change the contents of the free space manager --
     * notify metadata cache that the associated fsm ring is
     * unsettled
     */
    /* Only do so for strategies that use free-space managers */
    if(H5F_HAVE_FREE_SPACE_MANAGER(f))
        if(H5AC_unsettle_ring(f, fsm_ring) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_SYSTEM, FAIL, "attempt to notify cache that ring is unsettled failed")

    /* Check for attempting to free space that's a 'temporary' file address */
    if(H5F_addr_le(f->shared->tmp_addr, addr))
        HGOTO_ERROR(H5E_RESOURCE, H5E_BADRANGE, FAIL, "attempting to free temporary file space")

    /* If it's metadata, check if the space to free intersects with the file's
     * metadata accumulator
     */
    if(H5FD_MEM_DRAW != alloc_type) {
        /* Check if the space to free intersects with the file's metadata accumulator */
        if(H5F__accum_free(f, alloc_type, addr, size) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, FAIL, "can't check free space intersection w/metadata accumulator")
    } /* end if */

    /* Check if the free space manager for the file has been initialized */
    if(!f->shared->fs_man[fs_type]) {
        /* If there's no free space manager for objects of this type,
         *  see if we can avoid creating one by checking if the freed
         *  space is at the end of the file
         */
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: fs_addr = %a\n", FUNC, f->shared->fs_addr[fs_type]);
#endif /* H5MF_ALLOC_DEBUG_MORE */
        if(!H5F_addr_defined(f->shared->fs_addr[fs_type])) {
            htri_t status;          /* "can absorb" status for section into */

#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Trying to avoid starting up free space manager\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG_MORE */
            /* Try to shrink the file or absorb the block into a block aggregator */
            if((status = H5MF_try_shrink(f, alloc_type, addr, size)) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTMERGE, FAIL, "can't check for absorbing block")
            else if(status > 0)
                /* Indicate success */
                HGOTO_DONE(SUCCEED)
	    else if(size < f->shared->fs_threshold) {
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: dropping addr = %a, size = %Hu, on the floor!\n", FUNC, addr, size);
#endif /* H5MF_ALLOC_DEBUG_MORE */
		HGOTO_DONE(SUCCEED)
	    } /* end else-if */
        } /* end if */

        /* If we are deleting the free space manager, leave now, to avoid
         *  [re-]starting it.
         * or if file space strategy type is not using a free space manager
         *  (H5F_FSPACE_STRATEGY_AGGR or H5F_FSPACE_STRATEGY_NONE), drop free space
         *   section on the floor.
         *
         * Note: this drops the space to free on the floor...
         *
         */
        if(f->shared->fs_state[fs_type] == H5F_FS_STATE_DELETING ||
	        !H5F_HAVE_FREE_SPACE_MANAGER(f)) {
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: dropping addr = %a, size = %Hu, on the floor!\n", FUNC, addr, size);
#endif /* H5MF_ALLOC_DEBUG_MORE */
            HGOTO_DONE(SUCCEED)
        } /* end if */

        /* There's either already a free space manager, or the freed
         *  space isn't at the end of the file, so start up (or create)
         *  the file space manager
         */
        if(H5MF__start_fstype(f, fs_type) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize file free space")
    } /* end if */

    /* Create the free-space section for the freed section */
    ctype = H5MF_SECT_CLASS_TYPE(f, size);
    if(NULL == (node = H5MF__sect_new(ctype, addr, size)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize free space section")

    /* If size of the freed section is larger than threshold, add it to the free space manager */
    if(size >= f->shared->fs_threshold) {
	HDassert(f->shared->fs_man[fs_type]);

#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Before H5FS_sect_add()\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG_MORE */

        /* Add to the free space for the file */
        if(H5MF__add_sect(f, alloc_type, f->shared->fs_man[fs_type], node) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINSERT, FAIL, "can't add section to file free space")
        node = NULL;

#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: After H5FS_sect_add()\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG_MORE */
    } /* end if */
    else {
        htri_t merged;          /* Whether node was merged */
        H5MF_sect_ud_t udata; 	/* User data for callback */

        /* Construct user data for callbacks */
        udata.f = f;
        udata.alloc_type = alloc_type;
        udata.allow_sect_absorb = TRUE;
        udata.allow_eoa_shrink_only = FALSE;

        /* Try to merge the section that is smaller than threshold */
	if((merged = H5FS_sect_try_merge(f, f->shared->fs_man[fs_type], (H5FS_section_info_t *)node, H5FS_ADD_RETURNED_SPACE, &udata)) < 0)
	    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINSERT, FAIL, "can't merge section to file free space")
	else if(merged == TRUE) /* successfully merged */
	    /* Indicate that the node was used */
            node = NULL;
    } /* end else */

done:
    /* Reset the ring in the API context */
    if(orig_ring != H5AC_RING_INV)
        H5AC_set_ring(orig_ring, NULL);

    /* Release section node, if allocated and not added to section list or merged */
    if(node)
        if(H5MF__sect_free((H5FS_section_info_t *)node) < 0)
            HDONE_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't free simple section node")

#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Leaving, ret_value = %d\n", FUNC, ret_value);
#endif /* H5MF_ALLOC_DEBUG */
#ifdef H5MF_ALLOC_DEBUG_DUMP
H5MF__sects_dump(f, stderr);
#endif /* H5MF_ALLOC_DEBUG_DUMP */
    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5MF_xfree() */


/*-------------------------------------------------------------------------
 * Function:	H5MF_try_extend
 *
 * Purpose:	Extend a block in the file if possible.
 *          For non-paged aggregation:
 *          --try to extend at EOA
 *          --try to extend into the aggregators
 *          --try to extend into a free-space section if adjoined
 *          For paged aggregation:
 *          --try to extend at EOA
 *          --try to extend into a free-space section if adjoined
 *          --try to extend into the page end threshold if a metadata block
 *
 * Return:	Success:	TRUE(1)  - Block was extended
 *                              FALSE(0) - Block could not be extended
 * 		Failure:	FAIL
 *
 * Programmer:	Quincey Koziol
 *              Friday, June 11, 2004
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5MF_try_extend(H5F_t *f, H5FD_mem_t alloc_type, haddr_t addr, hsize_t size,
    hsize_t extra_requested)
{
    H5AC_ring_t orig_ring = H5AC_RING_INV;  /* Original ring value */
    H5AC_ring_t fsm_ring;                   /* Ring of FSM */
    haddr_t end;                            /* End of block to extend */
    H5FD_mem_t  map_type;                   /* Mapped type */
    H5F_mem_page_t fs_type;                 /* free space type */
    htri_t allow_extend = TRUE;		    /* Possible to extend the block */
    hsize_t frag_size = 0;                  /* Size of mis-aligned fragment */
    htri_t ret_value = FALSE;      	    /* Return value */

    FUNC_ENTER_NOAPI_TAG(H5AC__FREESPACE_TAG, FAIL)
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Entering: alloc_type = %u, addr = %a, size = %Hu, extra_requested = %Hu\n", FUNC, (unsigned)alloc_type, addr, size, extra_requested);
#endif /* H5MF_ALLOC_DEBUG */

    /* Sanity check */
    HDassert(f);
    HDassert(H5F_INTENT(f) & H5F_ACC_RDWR);

    if(f->shared->first_alloc_dealloc) {
        HDassert(! H5AC_cache_image_pending(f));
        if(H5MF_tidy_self_referential_fsm_hack(f) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, FAIL, "tidy of self referential fsm hack failed")
    } /* end if */

    /* Set mapped type, treating global heap as raw data */
    map_type = (alloc_type == H5FD_MEM_GHEAP) ? H5FD_MEM_DRAW : alloc_type;

    /* Compute end of block to extend */
    end = addr + size;

    /* For paged aggregation:
     *   To extend a small block: can only extend if not crossing page boundary 
     *   To extend a large block at EOA: calculate in advance mis-aligned fragment so EOA will still end at page boundary
     */
    if(H5F_PAGED_AGGR(f)) {
        if(size < f->shared->fs_page_size) {
            /* To extend a small block: cannot cross page boundary */
            if((addr / f->shared->fs_page_size) != (((end + extra_requested) - 1) / f->shared->fs_page_size))
                allow_extend = FALSE;
        } /* end if */
        else {
            haddr_t eoa;  /* EOA for the file */

            /*   To extend a large block: calculate in advance the mis-aligned fragment so EOA will end at page boundary if extended */
            if(HADDR_UNDEF == (eoa = H5F_get_eoa(f, alloc_type)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "Unable to get eoa")
            HDassert(!(eoa % f->shared->fs_page_size));

            H5MF_EOA_MISALIGN(f, (eoa+extra_requested), f->shared->fs_page_size, frag_size);
        } /* end else */
    } /* end if */

    /* Get free space type from allocation type */
    H5MF__alloc_to_fs_type(f, alloc_type, size, &fs_type);

    /* Set the ring type in the API context */
    if(H5MF__fsm_type_is_self_referential(f, fs_type))
        fsm_ring = H5AC_RING_MDFSM;
    else
        fsm_ring = H5AC_RING_RDFSM;
    H5AC_set_ring(fsm_ring, &orig_ring);

    if(allow_extend) {
        /* Try extending the block at EOA */
        if((ret_value = H5F__try_extend(f, map_type, end, extra_requested + frag_size)) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTEXTEND, FAIL, "error extending file")
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: extended = %t\n", FUNC, ret_value);
#endif /* H5MF_ALLOC_DEBUG_MORE */

        /* If extending at EOA succeeds: */
        /*   for paged aggregation, put the fragment into the large-sized free-space manager */
        if(ret_value == TRUE && H5F_PAGED_AGGR(f) && frag_size) {
            H5MF_free_section_t *node = NULL;   /* Free space section pointer */

            /* Should be large-sized block */
            HDassert(size >= f->shared->fs_page_size);

            /* Start up the free-space manager */
            if(!(f->shared->fs_man[fs_type]))
                if(H5MF__start_fstype(f, fs_type) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize file free space")

            /* Create free space section for the fragment */
            if(NULL == (node = H5MF__sect_new(H5MF_FSPACE_SECT_LARGE, end + extra_requested, frag_size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize free space section")

            /* Add the fragment to the large-sized free-space manager */
            if(H5MF__add_sect(f, alloc_type, f->shared->fs_man[fs_type], node) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINSERT, FAIL, "can't re-add section to file free space")

            node = NULL;
        } /* end if */

        /* For non-paged aggregation: try to extend into the aggregators */
        if(ret_value == FALSE && (f->shared->fs_strategy == H5F_FSPACE_STRATEGY_FSM_AGGR ||
                                  f->shared->fs_strategy == H5F_FSPACE_STRATEGY_AGGR) ) {
            H5F_blk_aggr_t *aggr;   /* Aggregator to use */

            /* Check if the block is able to extend into aggregation block */
            aggr = (map_type == H5FD_MEM_DRAW) ?  &(f->shared->sdata_aggr) : &(f->shared->meta_aggr);
            if((ret_value = H5MF__aggr_try_extend(f, aggr, map_type, end, extra_requested)) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTEXTEND, FAIL, "error extending aggregation block")

#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: H5MF__aggr_try_extend = %t\n", FUNC, ret_value);
#endif /* H5MF_ALLOC_DEBUG_MORE */
        } /* end if */

        /* If no extension so far, try to extend into a free-space section */
        if(ret_value == FALSE && ((f->shared->fs_strategy == H5F_FSPACE_STRATEGY_FSM_AGGR) ||
                                  (H5F_PAGED_AGGR(f))) ) {
            H5MF_sect_ud_t udata;       /* User data */

            /* Construct user data for callbacks */
            udata.f = f;
            udata.alloc_type = alloc_type;

            /* Check if the free space for the file has been initialized */
            if(!f->shared->fs_man[fs_type] && H5F_addr_defined(f->shared->fs_addr[fs_type]))
                /* Open the free-space manager */
                if(H5MF__open_fstype(f, fs_type) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize file free space")

            /* Try to extend the block into a free-space section */
            if(f->shared->fs_man[fs_type]) {
                if((ret_value = H5FS_sect_try_extend(f, f->shared->fs_man[fs_type], addr, size, extra_requested, H5FS_ADD_RETURNED_SPACE, &udata)) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTEXTEND, FAIL, "error extending block in free space manager")
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Try to H5FS_sect_try_extend = %t\n", FUNC, ret_value);
#endif /* H5MF_ALLOC_DEBUG_MORE */
            } /* end if */

            /* For paged aggregation and a metadata block: try to extend into page end threshold */
            if(ret_value == FALSE && H5F_PAGED_AGGR(f) && map_type != H5FD_MEM_DRAW) {
                H5MF_EOA_MISALIGN(f, end, f->shared->fs_page_size, frag_size);

                if(frag_size <= H5F_PGEND_META_THRES(f) && extra_requested <= frag_size)
                    ret_value = TRUE;
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Try to extend into the page end threshold = %t\n", FUNC, ret_value);
#endif /* H5MF_ALLOC_DEBUG_MORE */
            } /* end if */
        } /* end if */
    } /* allow_extend */

done:
    /* Reset the ring in the API context */
    if(orig_ring != H5AC_RING_INV)
        H5AC_set_ring(orig_ring, NULL);

#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Leaving: ret_value = %t\n", FUNC, ret_value);
#endif /* H5MF_ALLOC_DEBUG */
#ifdef H5MF_ALLOC_DEBUG_DUMP
H5MF__sects_dump(f, stderr);
#endif /* H5MF_ALLOC_DEBUG_DUMP */

    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5MF_try_extend() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_try_shrink
 *
 * Purpose:     Try to shrink the size of a file with a block or absorb it
 *              into a block aggregator.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              koziol@hdfgroup.org
 *              Feb 14 2008
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5MF_try_shrink(H5F_t *f, H5FD_mem_t alloc_type, haddr_t addr, hsize_t size)
{
    H5MF_free_section_t *node = NULL;   /* Free space section pointer */
    H5MF_sect_ud_t udata;               /* User data for callback */
    H5FS_section_class_t *sect_cls;	/* Section class */
    H5AC_ring_t orig_ring = H5AC_RING_INV;  /* Original ring value */
    H5AC_ring_t fsm_ring = H5AC_RING_INV;   /* Ring of FSM */
    H5F_mem_page_t fs_type;                 /* Free space type */
    htri_t ret_value = FALSE;               /* Return value */

    FUNC_ENTER_NOAPI_TAG(H5AC__FREESPACE_TAG, FAIL)
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Entering - alloc_type = %u, addr = %a, size = %Hu\n", FUNC, (unsigned)alloc_type, addr, size);
#endif /* H5MF_ALLOC_DEBUG */

    /* check arguments */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->lf);
    HDassert(H5F_addr_defined(addr));
    HDassert(size > 0);

    /* Set up free-space section class information */
    sect_cls = H5MF_SECT_CLS_TYPE(f, size);
    HDassert(sect_cls);

    /* Get free space type from allocation type */
    H5MF__alloc_to_fs_type(f, alloc_type, size, &fs_type);

    /* Set the ring type in the API context */
    if(H5MF__fsm_type_is_self_referential(f, fs_type))
        fsm_ring = H5AC_RING_MDFSM;
    else
        fsm_ring = H5AC_RING_RDFSM;
    H5AC_set_ring(fsm_ring, &orig_ring);

    /* Create free-space section for block */
    if(NULL == (node = H5MF__sect_new(sect_cls->type, addr, size)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize free space section")

    /* Construct user data for callbacks */
    udata.f = f;
    udata.alloc_type = alloc_type;
    udata.allow_sect_absorb = FALSE;    /* Force section to be absorbed into aggregator */
    udata.allow_eoa_shrink_only = FALSE;

    /* Check if the block can shrink the container */
    if(sect_cls->can_shrink) {
        if((ret_value = (*sect_cls->can_shrink)((const H5FS_section_info_t *)node, &udata)) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTMERGE, FAIL, "can't check if section can shrink container")
        if(ret_value > 0) {
            HDassert(sect_cls->shrink);

            if((*sect_cls->shrink)((H5FS_section_info_t **)&node, &udata) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTSHRINK, FAIL, "can't shrink container")
        } /* end if */
    } /* end if */

done:
    /* Reset the ring in the API context */
    if(orig_ring != H5AC_RING_INV)
        H5AC_set_ring(orig_ring, NULL);

    /* Free section node allocated */
    if(node && H5MF__sect_free((H5FS_section_info_t *)node) < 0)
        HDONE_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't free simple section node")

#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Leaving, ret_value = %d\n", FUNC, ret_value);
#endif /* H5MF_ALLOC_DEBUG */
    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5MF_try_shrink() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_close
 *
 * Purpose:     Close the free space tracker(s) for a file:
 *              paged or non-paged aggregation
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:  Vailin Choi; Dec 2012
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MF_close(H5F_t *f)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_TAG(H5AC__FREESPACE_TAG, FAIL)
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Entering\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG */

    /* check args */
    HDassert(f);
    HDassert(f->shared);

    if(H5F_PAGED_AGGR(f)) {
        if((ret_value = H5MF__close_pagefs(f)) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTFREE, FAIL, "can't close free-space managers for 'page' file space")
    }
    else {
        if((ret_value = H5MF__close_aggrfs(f)) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTFREE, FAIL, "can't close free-space managers for 'aggr' file space")
    }

done:
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Leaving\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG */
    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5MF_close() */


/*-------------------------------------------------------------------------
 * Function:    H5MF__close_delete_fstype
 *
 * Purpose:     Common code for closing and deleting the freespace manager
 *              of TYPE for file.
 *              Note that TYPE can be H5F_mem_page_t or H5FD_mem_t enum types.
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:	Vailin Choi
 *              Jan 2016
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5MF__close_delete_fstype(H5F_t *f, H5F_mem_page_t type)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC_TAG(H5AC__FREESPACE_TAG)
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Entering\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG */

    /* check args */
    HDassert(f);
    HDassert(f->shared);
    if(H5F_PAGED_AGGR(f))
        HDassert(type < H5F_MEM_PAGE_NTYPES);
    else
        HDassert((H5FD_mem_t)type < H5FD_MEM_NTYPES);

#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Check 1.0 - f->shared->fs_man[%u] = %p, f->shared->fs_addr[%u] = %a\n", FUNC, (unsigned)type, f->shared->fs_man[type], (unsigned)type, f->shared->fs_addr[type]);
#endif /* H5MF_ALLOC_DEBUG_MORE */

    /* If the free space manager for this type is open, close it */
    if(f->shared->fs_man[type])
        if(H5MF__close_fstype(f, type) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't close the free space manager")

#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Check 2.0 - f->shared->fs_man[%u] = %p, f->shared->fs_addr[%u] = %a\n", FUNC, (unsigned)type, f->shared->fs_man[type], (unsigned)type, f->shared->fs_addr[type]);
#endif /* H5MF_ALLOC_DEBUG_MORE */

    /* If there is free space manager info for this type, delete it */
    if(H5F_addr_defined(f->shared->fs_addr[type]))
        if(H5MF__delete_fstype(f, type) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't delete the free space manager")

done:
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Leaving\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG */
    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* H5MF__close_delete() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_try_close
 *
 * Purpose:     This is called by H5Fformat_convert() to close and delete
 *              free-space managers when downgrading persistent free-space
 *              to non-persistent.
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:	Vailin Choi
 *              Jan 2016
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MF_try_close(H5F_t *f)
{
    H5AC_ring_t orig_ring = H5AC_RING_INV;      /* Original ring value */
    H5AC_ring_t curr_ring = H5AC_RING_INV;      /* Current ring value */
    H5AC_ring_t needed_ring = H5AC_RING_INV;    /* Ring value needed for this iteration */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_TAG(H5AC__FREESPACE_TAG, FAIL)
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Entering\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG */

    /* check args */
    HDassert(f);

    /* If there have been no file space allocations / deallocation so 
     * far, must call H5MF_tidy_self_referential_fsm_hack() to float 
     * all self referential FSMs and release file space allocated to
     * them.  Otherwise, the function will be called after the format
     * conversion, and will become very confused.
     *
     * The situation is further complicated if a cache image exists
     * and had not yet been loaded into the metadata cache.  In this
     * case, call H5AC_force_cache_image_load() instead of 
     * H5MF_tidy_self_referential_fsm_hack().  H5AC_force_cache_image_load()
     * will load the cache image, and then call 
     * H5MF_tidy_self_referential_fsm_hack() to discard the cache image 
     * block.
     */
    if(f->shared->first_alloc_dealloc) {
        if(H5AC_cache_image_pending(f)) {
            if(H5AC_force_cache_image_load(f) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, FAIL, "forced cache image load failed")
        } /* end if */
        else {
            if(H5MF_tidy_self_referential_fsm_hack(f) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, FAIL, "tidy of self referential fsm hack failed")
        } /* end else */
    } /* end if */

    /* Set the ring type in the API context.  In most cases, we will
     * need H5AC_RING_RDFSM, so initially set the ring in
     * the context to that value.  We will alter this later if needed.
     */
    H5AC_set_ring(H5AC_RING_RDFSM, &orig_ring);
    curr_ring = H5AC_RING_RDFSM;

    if(H5F_PAGED_AGGR(f)) {
        H5F_mem_page_t ptype; 	/* Memory type for iteration */

        /* Iterate over all the free space types that have managers and 
         * get each free list's space 
         */
        for(ptype = H5F_MEM_PAGE_META; ptype < H5F_MEM_PAGE_NTYPES; H5_INC_ENUM(H5F_mem_page_t, ptype)) {
            /* Test to see if we need to switch rings -- do so if required */
            if(H5MF__fsm_type_is_self_referential(f, ptype))
                needed_ring = H5AC_RING_MDFSM;
            else
                needed_ring = H5AC_RING_RDFSM;

            if(needed_ring != curr_ring ) {
                H5AC_set_ring(needed_ring, NULL);
                curr_ring = needed_ring;
            } /* end if */

            if(H5MF__close_delete_fstype(f, ptype) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't close the free space manager")
        } /* end for */
    } /* end if */
    else {
        H5FD_mem_t type;          	/* Memory type for iteration */

        /* Iterate over all the free space types that have managers and 
         * get each free list's space 
         */
        for(type = H5FD_MEM_DEFAULT; type < H5FD_MEM_NTYPES; H5_INC_ENUM(H5FD_mem_t, type)) {
            /* Test to see if we need to switch rings -- do so if required */
            if(H5MF__fsm_type_is_self_referential(f, (H5F_mem_page_t)type))
                needed_ring = H5AC_RING_MDFSM;
            else
                needed_ring = H5AC_RING_RDFSM;

            if(needed_ring != curr_ring) {
                H5AC_set_ring(needed_ring, NULL);
                curr_ring = needed_ring;
            } /* end if */

            if(H5MF__close_delete_fstype(f, (H5F_mem_page_t)type) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't close the free space manager")
        } /* end for */
    } /* end else */

done:
    /* Reset the ring in the API context */
    if(orig_ring != H5AC_RING_INV)
        H5AC_set_ring(orig_ring, NULL);

#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Leaving\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG */
    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* H5MF_try_close() */


/*-------------------------------------------------------------------------
 * Function:    H5MF__close_aggrfs
 *
 * Purpose:     Close the free space tracker(s) for a file: non-paged aggregation
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, January 22, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5MF__close_aggrfs(H5F_t *f)
{
    H5AC_ring_t orig_ring = H5AC_RING_INV;      /* Original ring value */
    H5AC_ring_t curr_ring = H5AC_RING_INV;      /* Current ring value */
    H5AC_ring_t needed_ring = H5AC_RING_INV;    /* Ring value needed for this iteration.  */
    H5FD_mem_t type;          	    /* Memory type for iteration */
    herr_t ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_STATIC_TAG(H5AC__FREESPACE_TAG)
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Entering\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG */

    /* check args */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->lf);
    HDassert(f->shared->sblock);

    /* Set the ring type in the API context.  In most cases, we will
     * need H5AC_RING_RDFSM, so initially set the ring in
     * the context to that value.  We will alter this later if needed.
     */
    H5AC_set_ring(H5AC_RING_RDFSM, &orig_ring);
    curr_ring = H5AC_RING_RDFSM;

    /* Free the space in aggregators */
    /* (for space not at EOA, it may be put into free space managers) */
    if(H5MF_free_aggrs(f) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTFREE, FAIL, "can't free aggregators")

    /* Trying shrinking the EOA for the file */
    if(H5MF__close_shrink_eoa(f) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTSHRINK, FAIL, "can't shrink eoa")

    /* Making free-space managers persistent for superblock version >= 2 */
    if(f->shared->sblock->super_vers >= HDF5_SUPERBLOCK_VERSION_2
            && f->shared->fs_persist) {
        H5O_fsinfo_t fsinfo;	/* File space info message */
        haddr_t final_eoa;      /* Final eoa -- for sanity check */
        H5F_mem_page_t ptype;   /* Memory type for iteration */

        /* superblock extension and free space manager message should
         * exist at this point -- verify at least the former.
         */
        HDassert(H5F_addr_defined(f->shared->sblock->ext_addr));

        /* file space for all non-empty free space managers should be 
         * allocated at this point, and these free space managers should
         * be written to file and thus their headers and section info 
         * entries in the metadata cache should be clean.
         */

        /* gather data for the free space manager superblock extension message.
         *
         * In passing, verify that all the free space managers are closed.
         */
        for(ptype = H5F_MEM_PAGE_META; ptype < H5F_MEM_PAGE_NTYPES; H5_INC_ENUM(H5F_mem_page_t, ptype))
            fsinfo.fs_addr[ptype - 1] = HADDR_UNDEF;
        for(type = H5FD_MEM_SUPER; type < H5FD_MEM_NTYPES; H5_INC_ENUM(H5FD_mem_t, type))
            fsinfo.fs_addr[type-1] = f->shared->fs_addr[type];
        fsinfo.strategy = f->shared->fs_strategy;
        fsinfo.persist = f->shared->fs_persist;
        fsinfo.threshold = f->shared->fs_threshold;
        fsinfo.page_size = f->shared->fs_page_size;
        fsinfo.pgend_meta_thres = f->shared->pgend_meta_thres;
        fsinfo.eoa_pre_fsm_fsalloc = f->shared->eoa_pre_fsm_fsalloc;

        /* Write the free space manager message -- message must already exist */
        if(H5F__super_ext_write_msg(f, H5O_FSINFO_ID, &fsinfo, FALSE, H5O_MSG_FLAG_MARK_IF_UNKNOWN) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_WRITEERROR, FAIL, "error in writing message to superblock extension")

        /* Close the free space managers */
        for(type = H5FD_MEM_SUPER; type < H5FD_MEM_NTYPES; H5_INC_ENUM(H5FD_mem_t, type)) {
            if(f->shared->fs_man[type]) {
                /* Test to see if we need to switch rings -- do so if required */
                if(H5MF__fsm_type_is_self_referential(f, (H5F_mem_page_t)type))
                    needed_ring = H5AC_RING_MDFSM;
                else 
                    needed_ring = H5AC_RING_RDFSM;

                if(needed_ring != curr_ring) {
                    H5AC_set_ring(needed_ring, NULL);
                    curr_ring = needed_ring;
                } /* end if */

                HDassert(f->shared->fs_state[type] == H5F_FS_STATE_OPEN);

                if(H5FS_close(f, f->shared->fs_man[type]) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't close free space manager")
                f->shared->fs_man[type] = NULL;
                f->shared->fs_state[type] = H5F_FS_STATE_CLOSED;
            } /* end if */
            f->shared->fs_addr[type] = HADDR_UNDEF;
        } /* end for */

        /* verify that we haven't dirtied any metadata cache entries
         * from the metadata free space manager ring out.
         */
        HDassert(H5AC_cache_is_clean(f, H5AC_RING_MDFSM));

        /* verify that the aggregators are still shutdown. */
        HDassert(f->shared->sdata_aggr.tot_size == 0);
        HDassert(f->shared->sdata_aggr.addr == 0);
        HDassert(f->shared->sdata_aggr.size == 0);

        HDassert(f->shared->meta_aggr.tot_size == 0);
        HDassert(f->shared->meta_aggr.addr == 0);
        HDassert(f->shared->meta_aggr.size == 0);

        /* Trying shrinking the EOA for the file */
        /* (in case any free space is now at the EOA) */
        if(H5MF__close_shrink_eoa(f) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTSHRINK, FAIL, "can't shrink eoa")

        /* get the eoa, and verify that it has the expected value */
        if(HADDR_UNDEF == (final_eoa = H5FD_get_eoa(f->shared->lf, H5FD_MEM_DEFAULT)) )
            HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get file size")

        /* f->shared->eoa_post_fsm_fsalloc is undefined if there has 
         * been no file space allocation or deallocation since file 
         * open.
         */
        HDassert((f->shared->first_alloc_dealloc) || (final_eoa == f->shared->eoa_post_fsm_fsalloc));
    } /* end if */
    else {  /* super_vers can be 0, 1, 2 */
        for(type = H5FD_MEM_DEFAULT; type < H5FD_MEM_NTYPES; H5_INC_ENUM(H5FD_mem_t, type))
            if(H5MF__close_delete_fstype(f, (H5F_mem_page_t)type) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize file free space")
    } /* end else */

    /* Free the space in aggregators (again) */
    /* (in case any free space information re-started them) */
    if(H5MF_free_aggrs(f) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTFREE, FAIL, "can't free aggregators")

    /* Trying shrinking the EOA for the file */
    /* (in case any free space is now at the EOA) */
    if(H5MF__close_shrink_eoa(f) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTSHRINK, FAIL, "can't shrink eoa")

done:
    /* Reset the ring in the API context */
    if(orig_ring != H5AC_RING_INV)
        H5AC_set_ring(orig_ring, NULL);

#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Leaving\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG */
    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5MF__close_aggrfs() */


/*-------------------------------------------------------------------------
 * Function:    H5MF__close_pagefs
 *
 * Purpose:     Close the free space tracker(s) for a file: paged aggregation
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Vailin Choi; Dec 2012
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5MF__close_pagefs(H5F_t *f)
{
    H5AC_ring_t orig_ring = H5AC_RING_INV;      /* Original ring value */
    H5AC_ring_t curr_ring = H5AC_RING_INV;      /* Current ring value */
    H5AC_ring_t needed_ring = H5AC_RING_INV;    /* Ring value needed for this iteration.  */
    H5F_mem_page_t ptype; 	/* Memory type for iteration */
    H5O_fsinfo_t fsinfo;	/* File space info message */
    herr_t ret_value = SUCCEED;	/* Return value */

    FUNC_ENTER_STATIC_TAG(H5AC__FREESPACE_TAG)
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Entering\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG */

    /* check args */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->lf);
    HDassert(f->shared->sblock);
    HDassert(f->shared->fs_page_size);
    HDassert(f->shared->sblock->super_vers >= HDF5_SUPERBLOCK_VERSION_2);

    /* Set the ring type in the API context.  In most cases, we will
     * need H5AC_RING_RDFSM, so initially set the ring in
     * the context to that value.  We will alter this later if needed.
     */
    H5AC_set_ring(H5AC_RING_RDFSM, &orig_ring);
    curr_ring = H5AC_RING_RDFSM;

    /* Trying shrinking the EOA for the file */
    if(H5MF__close_shrink_eoa(f) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTSHRINK, FAIL, "can't shrink eoa")

    /* Set up file space info message */
    fsinfo.strategy = f->shared->fs_strategy;
    fsinfo.persist = f->shared->fs_persist;
    fsinfo.threshold = f->shared->fs_threshold;
    fsinfo.page_size = f->shared->fs_page_size;
    fsinfo.pgend_meta_thres = f->shared->pgend_meta_thres;
    fsinfo.eoa_pre_fsm_fsalloc = HADDR_UNDEF;
    for(ptype = H5F_MEM_PAGE_META; ptype < H5F_MEM_PAGE_NTYPES; H5_INC_ENUM(H5F_mem_page_t, ptype))
        fsinfo.fs_addr[ptype - 1] = HADDR_UNDEF;

    if(f->shared->fs_persist) {
        haddr_t final_eoa;              /* final eoa -- for sanity check */

        /* superblock extension and free space manager message should
         * exist at this point -- verify at least the former.
         */
        HDassert(H5F_addr_defined(f->shared->sblock->ext_addr));

        /* file space for all non-empty free space managers should be
         * allocated at this point, and these free space managers should
         * be written to file and thus their headers and section info
         * entries in the metadata cache should be clean.
         */

        /* gather data for the free space manager superblock extension message.
         * Only need addresses of FSMs and eoa prior to allocation of
         * file space for the self referential free space managers.  Other 
         * data was gathered above.
         */
        for(ptype = H5F_MEM_PAGE_META; ptype < H5F_MEM_PAGE_NTYPES; H5_INC_ENUM(H5F_mem_page_t, ptype))
            fsinfo.fs_addr[ptype-1] = f->shared->fs_addr[ptype];
        fsinfo.eoa_pre_fsm_fsalloc = f->shared->eoa_pre_fsm_fsalloc;

        /* Write the free space manager message -- message must already exist */
        if(H5F__super_ext_write_msg(f, H5O_FSINFO_ID, &fsinfo, FALSE, H5O_MSG_FLAG_MARK_IF_UNKNOWN) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_WRITEERROR, FAIL, "error in writing message to superblock extension")

        /* Close the free space managers */
        /* use H5MF__close_fstype() for this? */
        for(ptype = H5F_MEM_PAGE_META; ptype < H5F_MEM_PAGE_NTYPES; H5_INC_ENUM(H5F_mem_page_t, ptype)) {
            if(f->shared->fs_man[ptype]) {
                /* Test to see if we need to switch rings -- do so if required */
                if(H5MF__fsm_type_is_self_referential(f, ptype))
                    needed_ring = H5AC_RING_MDFSM;
                else
                    needed_ring = H5AC_RING_RDFSM;

                if(needed_ring != curr_ring) {
                    H5AC_set_ring(needed_ring, NULL);
                    curr_ring = needed_ring;
                } /* end if */

                HDassert(f->shared->fs_state[ptype] == H5F_FS_STATE_OPEN);

                if(H5FS_close(f, f->shared->fs_man[ptype]) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't close free space manager")
                f->shared->fs_man[ptype] = NULL;
                f->shared->fs_state[ptype] = H5F_FS_STATE_CLOSED;
            } /* end if */
            f->shared->fs_addr[ptype] = HADDR_UNDEF;
        } /* end for */

        /* verify that we haven't dirtied any metadata cache entries
         * from the metadata free space manager ring out.
         */
        HDassert(H5AC_cache_is_clean(f, H5AC_RING_MDFSM));

        /* Trying shrinking the EOA for the file */
        /* (in case any free space is now at the EOA) */
        if(H5MF__close_shrink_eoa(f) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTSHRINK, FAIL, "can't shrink eoa")

        /* get the eoa, and verify that it has the expected value */
        if(HADDR_UNDEF == (final_eoa = H5FD_get_eoa(f->shared->lf, H5FD_MEM_DEFAULT)) )
            HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get file size")

        /* f->shared->eoa_post_fsm_fsalloc is undefined if there has
         * been no file space allocation or deallocation since file
         * open.
         *
         * If there is a cache image in the file at file open, 
         * f->shared->first_alloc_dealloc will always be FALSE unless 
         * the file is opened R/O, as otherwise, the image will have been
         * read and discarded by this point.
         *
         * If a cache image was created on file close, the actual EOA 
         * should be in f->shared->eoa_post_mdci_fsalloc.  Note that in 
         * this case, it is conceivable that f->shared->first_alloc_dealloc
         * will still be TRUE, as the cache image is allocated directly from
         * the file driver layer.  However, as this possibility seems remote,
         * it is ignored in the following assert.
         */
        HDassert((f->shared->first_alloc_dealloc) ||
                 (final_eoa == f->shared->eoa_post_fsm_fsalloc) ||
                 ((H5F_addr_defined(f->shared->eoa_post_mdci_fsalloc)) &&
                  (final_eoa == f->shared->eoa_post_mdci_fsalloc)));
    } /* end if */
    else {
        /* Iterate over all the free space types that have managers 
         * and get each free list's space 
         */
        for(ptype = H5F_MEM_PAGE_META; ptype < H5F_MEM_PAGE_NTYPES; H5_INC_ENUM(H5F_mem_page_t, ptype))
            if(H5MF__close_delete_fstype(f, ptype) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't close the free space manager")

        /* Write file space info message to superblock extension object header */
        /* Create the superblock extension object header in advance if needed */
        if(H5F__super_ext_write_msg(f, H5O_FSINFO_ID, &fsinfo, FALSE, H5O_MSG_FLAG_MARK_IF_UNKNOWN) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_WRITEERROR, FAIL, "error in writing message to superblock extension")
    } /* end else */

    /* Trying shrinking the EOA for the file */
    /* (in case any free space is now at the EOA) */
    if(H5MF__close_shrink_eoa(f) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTSHRINK, FAIL, "can't shrink eoa")

done:
    /* Reset the ring in the API context */
    if(orig_ring != H5AC_RING_INV)
        H5AC_set_ring(orig_ring, NULL);

#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Leaving\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG */
    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5MF__close_pagefs() */


/*-------------------------------------------------------------------------
 * Function:    H5MF__close_shrink_eoa
 *
 * Purpose:     Shrink the EOA while closing
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:  Quincey Koziol
 *              Saturday, July 7, 2012
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5MF__close_shrink_eoa(H5F_t *f)
{
    H5AC_ring_t orig_ring = H5AC_RING_INV;      /* Original ring value */
    H5AC_ring_t curr_ring = H5AC_RING_INV;      /* Current ring value */
    H5AC_ring_t needed_ring = H5AC_RING_INV;    /* Ring value needed for this iteration.  */
    H5F_mem_t type;
    H5F_mem_page_t ptype;       /* Memory type for iteration */
    hbool_t eoa_shrank;		/* Whether an EOA shrink occurs */
    htri_t status;		/* Status value */
    H5MF_sect_ud_t udata;	/* User data for callback */
    herr_t ret_value = SUCCEED;	/* Return value */

    FUNC_ENTER_STATIC_TAG(H5AC__FREESPACE_TAG)

    /* check args */
    HDassert(f);
    HDassert(f->shared);

    /* Construct user data for callbacks */
    udata.f = f;
    udata.allow_sect_absorb = FALSE;
    udata.allow_eoa_shrink_only = TRUE;

    /* Set the ring type in the API context */
    H5AC_set_ring(H5AC_RING_RDFSM, &orig_ring);
    curr_ring = H5AC_RING_RDFSM;

    /* Iterate until no more EOA shrinking occurs */
    do {
	eoa_shrank = FALSE;

        if(H5F_PAGED_AGGR(f)) {
            /* Check the last section of each free-space manager */
            for(ptype = H5F_MEM_PAGE_META; ptype < H5F_MEM_PAGE_NTYPES; H5_INC_ENUM(H5F_mem_page_t, ptype)) {
                if(f->shared->fs_man[ptype]) {
                    /* Test to see if we need to switch rings -- do so if required */
                    if(H5MF__fsm_type_is_self_referential(f, ptype))
                        needed_ring = H5AC_RING_MDFSM;
                    else
                        needed_ring = H5AC_RING_RDFSM;

                    if(needed_ring != curr_ring) {
                        H5AC_set_ring(needed_ring, NULL);
                        curr_ring = needed_ring;
                    } /* end if */

                    udata.alloc_type = (H5FD_mem_t)((H5FD_mem_t)ptype < H5FD_MEM_NTYPES ? ptype : ((ptype % H5FD_MEM_NTYPES) + 1));

                    if((status = H5FS_sect_try_shrink_eoa(f, f->shared->fs_man[ptype], &udata)) < 0)
                        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTSHRINK, FAIL, "can't check for shrinking eoa")
                    else if(status > 0)
                        eoa_shrank = TRUE;
                } /* end if */
            } /* end for */
        } /* end if */
        else {
            /* Check the last section of each free-space manager */
            for(type = H5FD_MEM_DEFAULT; type < H5FD_MEM_NTYPES; H5_INC_ENUM(H5FD_mem_t, type)) {
                if(f->shared->fs_man[type]) {
                    /* Test to see if we need to switch rings -- do so if required */
                    if(H5MF__fsm_type_is_self_referential(f, (H5F_mem_page_t)type))
                        needed_ring = H5AC_RING_MDFSM;
                    else
                        needed_ring = H5AC_RING_RDFSM;

                    if(needed_ring != curr_ring) {
                        H5AC_set_ring(needed_ring, NULL);
                        curr_ring = needed_ring;
                    } /* end if */

                    udata.alloc_type = type;

                    if((status = H5FS_sect_try_shrink_eoa(f, f->shared->fs_man[type], &udata)) < 0)
                        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTSHRINK, FAIL, "can't check for shrinking eoa")
                else if(status > 0)
                    eoa_shrank = TRUE;
                } /* end if */
            } /* end for */

            /* check the two aggregators */
            if((status = H5MF_aggrs_try_shrink_eoa(f)) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTSHRINK, FAIL, "can't check for shrinking eoa")
            else if(status > 0)
                eoa_shrank = TRUE;
        } /* end else */
    } while(eoa_shrank);

done:
    /* Reset the ring in the API context */
    if(orig_ring != H5AC_RING_INV)
        H5AC_set_ring(orig_ring, NULL);

    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5MF__close_shrink_eoa() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_get_freespace
 *
 * Purpose:     Retrieve the amount of free space in the file
 *
 * Return:      Success:        Amount of free space in file
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Monday, October  6, 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MF_get_freespace(H5F_t *f, hsize_t *tot_space, hsize_t *meta_size)
{
    haddr_t ma_addr = HADDR_UNDEF;  /* Base "metadata aggregator" address */
    hsize_t ma_size = 0;            /* Size of "metadata aggregator" */
    haddr_t sda_addr = HADDR_UNDEF; /* Base "small data aggregator" address */
    hsize_t sda_size = 0;           /* Size of "small data aggregator" */
    hsize_t tot_fs_size = 0;        /* Amount of all free space managed */
    hsize_t tot_meta_size = 0;      /* Amount of metadata for free space managers */
    H5FD_mem_t tt;                  /* Memory type for iteration */
    H5F_mem_page_t type;            /* Memory type for iteration */
    H5F_mem_page_t start_type;      /* Memory type for iteration */
    H5F_mem_page_t end_type;        /* Memory type for iteration */
    htri_t fs_started[H5F_MEM_PAGE_NTYPES]; /* Indicate whether the free-space manager has been started */
    haddr_t fs_eoa[H5FD_MEM_NTYPES];        /* EAO for each free-space manager */
    H5AC_ring_t orig_ring = H5AC_RING_INV;  /* Original ring value */
    H5AC_ring_t curr_ring = H5AC_RING_INV;      /* Current ring value */
    H5AC_ring_t needed_ring = H5AC_RING_INV;    /* Ring value needed for this iteration.  */
    herr_t ret_value = SUCCEED;             /* Return value */

    FUNC_ENTER_NOAPI_TAG(H5AC__FREESPACE_TAG, FAIL)

    /* check args */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->lf);

    /* Set the ring type in the API context.  In most cases, we will
     * need H5AC_RING_RDFSM, so initially set the ring in
     * the context to that value.  We will alter this later if needed.
     */
    H5AC_set_ring(H5AC_RING_RDFSM, &orig_ring);
    curr_ring = H5AC_RING_RDFSM;

    /* Determine start/end points for loop */
    if(H5F_PAGED_AGGR(f)) {
        start_type = H5F_MEM_PAGE_META;
        end_type = H5F_MEM_PAGE_NTYPES;
    } /* end if */
    else {
        start_type = (H5F_mem_page_t)H5FD_MEM_SUPER;
        end_type = (H5F_mem_page_t)H5FD_MEM_NTYPES;
    } /* end else */

    for(tt = H5FD_MEM_SUPER; tt < H5FD_MEM_NTYPES; H5_INC_ENUM(H5FD_mem_t, tt))
        if(HADDR_UNDEF == (fs_eoa[tt] = H5F_get_eoa(f, tt)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "driver get_eoa request failed")

    if(!H5F_PAGED_AGGR(f)) {
        /* Retrieve metadata aggregator info, if available */
        if(H5MF__aggr_query(f, &(f->shared->meta_aggr), &ma_addr, &ma_size) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "can't query metadata aggregator stats")

        /* Retrieve 'small data' aggregator info, if available */
        if(H5MF__aggr_query(f, &(f->shared->sdata_aggr), &sda_addr, &sda_size) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "can't query small data aggregator stats")
    } /* end if */

    /* Iterate over all the free space types that have managers and get each free list's space */
    for(type = start_type; type < end_type; H5_INC_ENUM(H5F_mem_page_t, type)) {
        fs_started[type] = FALSE;

        /* Check if the free space for the file has been initialized */
        if(!f->shared->fs_man[type] && H5F_addr_defined(f->shared->fs_addr[type])) {
            if(H5MF__open_fstype(f, type) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize file free space")
            HDassert(f->shared->fs_man[type]);
            fs_started[type] = TRUE;
        } /* end if */

        /* Test to see if we need to switch rings -- do so if required */
        if(H5MF__fsm_type_is_self_referential(f, (H5F_mem_page_t)type))
            needed_ring = H5AC_RING_MDFSM;
        else
            needed_ring = H5AC_RING_RDFSM;
        if(needed_ring != curr_ring) {
            H5AC_set_ring(needed_ring, NULL);
            curr_ring = needed_ring;
        } /* end if */

        /* Check if there's free space of this type */
        if(f->shared->fs_man[type]) {
            hsize_t type_fs_size = 0;    /* Amount of free space managed for each type */
            hsize_t type_meta_size = 0;  /* Amount of free space metadata for each type */

            /* Retrieve free space size from free space manager */
            if(H5FS_sect_stats(f->shared->fs_man[type], &type_fs_size, NULL) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "can't query free space stats")
            if(H5FS_size(f, f->shared->fs_man[type], &type_meta_size) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "can't query free space metadata stats")

            /* Increment total free space for types */
            tot_fs_size += type_fs_size;
            tot_meta_size += type_meta_size;
        } /* end if */
    } /* end for */

    /* Close the free-space managers if they were opened earlier in this routine */
    for(type = start_type; type < end_type; H5_INC_ENUM(H5F_mem_page_t, type)) {
        /* Test to see if we need to switch rings -- do so if required */
        if(H5MF__fsm_type_is_self_referential(f, (H5F_mem_page_t)type))
            needed_ring = H5AC_RING_MDFSM;
        else
            needed_ring = H5AC_RING_RDFSM;
        if(needed_ring != curr_ring) {
            H5AC_set_ring(needed_ring, &curr_ring);
            curr_ring = needed_ring;
        } /* end if */

        if(fs_started[type])
            if(H5MF__close_fstype(f, type) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't close file free space")
    } /* end for */

    /* Set the value(s) to return */
    /* (The metadata & small data aggregators count as free space now, since they aren't at EOA) */
    if(tot_space)
        *tot_space = tot_fs_size + ma_size + sda_size;
    if(meta_size)
        *meta_size = tot_meta_size;

done:
    /* Reset the ring in the API context */
    if(orig_ring != H5AC_RING_INV)
        H5AC_set_ring(orig_ring, NULL);

    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5MF_get_freespace() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_get_free_sections()
 *
 * Purpose: 	To retrieve free-space section information for
 *              paged or non-paged aggregation
 *
 * Return:      Success:    Number of free sections
 *              Failure:    -1
 *
 * Programmer:  Vailin Choi; Dec 2012
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5MF_get_free_sections(H5F_t *f, H5FD_mem_t type, size_t nsects, H5F_sect_info_t *sect_info)
{
    H5AC_ring_t orig_ring = H5AC_RING_INV;      /* Original ring value */
    H5AC_ring_t curr_ring = H5AC_RING_INV;      /* Current ring value */
    H5AC_ring_t needed_ring = H5AC_RING_INV;    /* Ring value needed for this iteration.  */
    size_t total_sects = 0;                     /* Total number of sections */
    H5MF_sect_iter_ud_t sect_udata;             /* User data for callback */
    H5F_mem_page_t start_type, end_type;        /* Memory types to iterate over */
    H5F_mem_page_t ty;                          /* Memory type for iteration */
    ssize_t ret_value = -1;                     /* Return value */

    FUNC_ENTER_NOAPI_TAG(H5AC__FREESPACE_TAG, (-1))

    /* check args */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->lf);

    /* H5MF_tidy_self_referential_fsm_hack() will fail if any self 
     * referential FSM is opened prior to the call to it.  Thus call
     * it here if necessary and if it hasn't been called already.
     *
     * The situation is further complicated if a cache image exists
     * and had not yet been loaded into the metadata cache.  In this
     * case, call H5AC_force_cache_image_load() instead of 
     * H5MF_tidy_self_referential_fsm_hack().  H5AC_force_cache_image_load()
     * will load the cache image, and then call 
     * H5MF_tidy_self_referential_fsm_hack() to discard the cache image 
     * block.
     */
    if(f->shared->first_alloc_dealloc) {
        if(H5AC_cache_image_pending(f)) {
            if(H5AC_force_cache_image_load(f) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, (-1), "forced cache image load failed")
        } /* end if */
        else {
            if(H5MF_tidy_self_referential_fsm_hack(f) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, (-1), "tidy of self referential fsm hack failed")
        } /* end else */
    } /* end if */

    if(type == H5FD_MEM_DEFAULT) {
        start_type = H5F_MEM_PAGE_SUPER;
        end_type = H5F_MEM_PAGE_NTYPES;
    } /* end if */
    else {
        start_type = end_type = (H5F_mem_page_t)type;
        if(H5F_PAGED_AGGR(f)) /* set to the corresponding LARGE free-space manager */
            end_type = (H5F_mem_page_t)(end_type + H5FD_MEM_NTYPES);
        else
            H5_INC_ENUM(H5F_mem_page_t, end_type);
    } /* end else */

    /* Set up user data for section iteration */
    sect_udata.sects = sect_info;
    sect_udata.sect_count = nsects;
    sect_udata.sect_idx = 0;

    /* Set the ring type in the API context.  In most cases, we will
     * need H5AC_RING_RDFSM, so initially set the ring in
     * the context to that value.  We will alter this later if needed.
     */
    H5AC_set_ring(H5AC_RING_RDFSM, &orig_ring);
    curr_ring = H5AC_RING_RDFSM;

    /* Iterate over memory types, retrieving the number of sections of each type */
    for(ty = start_type; ty < end_type; H5_INC_ENUM(H5F_mem_page_t, ty)) {
        hbool_t fs_started = FALSE;	/* The free-space manager is opened or not */
        size_t nums = 0;		/* The number of free-space sections */

        /* Test to see if we need to switch rings -- do so if required */
        if(H5MF__fsm_type_is_self_referential(f, ty))
            needed_ring = H5AC_RING_MDFSM;
        else
            needed_ring = H5AC_RING_RDFSM;
        if(needed_ring != curr_ring) {
            H5AC_set_ring(needed_ring, &curr_ring);
            curr_ring = needed_ring;
        } /* end if */

        if(!f->shared->fs_man[ty] && H5F_addr_defined(f->shared->fs_addr[ty])) {
            if(H5MF__open_fstype(f, ty) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, (-1), "can't open the free space manager")
            HDassert(f->shared->fs_man[ty]);
            fs_started = TRUE;
        } /* end if */

        /* Check if there's free space sections of this type */
        if(f->shared->fs_man[ty])
            if(H5MF__get_free_sects(f, f->shared->fs_man[ty], &sect_udata, &nums) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, (-1), "can't get section info for the free space manager")

        /* Increment total # of sections */
        total_sects += nums;

        /* Close the free space manager of this type, if we started it here */
        if(fs_started)
            if(H5MF__close_fstype(f, ty) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTCLOSEOBJ, (-1), "can't close file free space")
        if((H5F_PAGED_AGGR(f)) && (type != H5FD_MEM_DEFAULT))
            ty = (H5F_mem_page_t)(ty + H5FD_MEM_NTYPES - 2);
    } /* end for */

    /* Set return value */
    ret_value = (ssize_t)total_sects;

done:
    /* Reset the ring in the API context */
    if(orig_ring != H5AC_RING_INV)
        H5AC_set_ring(orig_ring, NULL);

    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* H5MF_get_free_sections() */


/*-------------------------------------------------------------------------
 * Function:    H5MF__sects_cb()
 *
 * Purpose:	Iterator callback for each free-space section
 *          Retrieve address and size into user data
 *
 * Return:	Always succeed
 *
 * Programmer:  Vailin Choi
 *              July 1st, 2009
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5MF__sects_cb(H5FS_section_info_t *_sect, void *_udata)
{
    H5MF_free_section_t *sect = (H5MF_free_section_t *)_sect;
    H5MF_sect_iter_ud_t *udata = (H5MF_sect_iter_ud_t *)_udata;

    FUNC_ENTER_STATIC_NOERR

    if(udata->sect_idx < udata->sect_count) {
        udata->sects[udata->sect_idx].addr = sect->sect_info.addr;
        udata->sects[udata->sect_idx].size  = sect->sect_info.size;
        udata->sect_idx++;
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5MF__sects_cb() */


/*-------------------------------------------------------------------------
 * Function:    H5MF__get_free_sects
 *
 * Purpose:	Retrieve section information for the specified free-space manager.
 *
 * Return:      Success:        non-negative
 *              Failure:        negative
 *
 * Programmer:  Vailin Choi; Dec 2012
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5MF__get_free_sects(H5F_t *f, H5FS_t *fspace, H5MF_sect_iter_ud_t *sect_udata, size_t *nums)
{
    hsize_t hnums = 0;          	/* # of sections */
    herr_t ret_value = SUCCEED; 	/* Return value */

    FUNC_ENTER_PACKAGE

    /* check args */
    HDassert(f);
    HDassert(sect_udata);
    HDassert(nums);
    HDassert(fspace);

    /* Query how many sections of this type */
    if(H5FS_sect_stats(fspace, NULL, &hnums) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "can't query free space stats")
    H5_CHECKED_ASSIGN(*nums, size_t, hnums, hsize_t);

    /* Check if we should retrieve the section info */
    if(sect_udata->sects && *nums > 0)
        /* Iterate over all the free space sections of this type, adding them to the user's section info */
        if(H5FS_sect_iterate(f, fspace, H5MF__sects_cb, sect_udata) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_BADITER, FAIL, "can't iterate over sections")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5MF__get_free_sects() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_settle_raw_data_fsm()
 *
 * Purpose: 	Handle any tasks required before the metadata cache 
 *		can serialize or flush the raw data free space manager
 *		and any metadata free space managers that reside in the 
 *		raw data free space manager ring.
 *
 *              Specifically, this means any metadata managers that DON'T 
 *              handle space allocation for free space manager header or 
 *              section info will reside in the raw data free space manager 
 *              ring.
 *
 *              In the absence of page allocation, there is at most one 
 *		free space manager per memory type defined in H5F_mem_t.
 *		Of these, the one that allocates H5FD_MEM_DRAW will 
 *		always reside in the raw data free space manager ring.
 *		If there is more than one metadata free space manager, 
 *		all that don't handle H5FD_MEM_FSPACE_HDR or 
 *              H5FD_MEM_FSPACE_SINFO (which map to H5FD_MEM_OHDR and 
 *              H5FD_MEM_LHEAP respectively) will reside in the raw 
 *		data free space manager ring as well
 *
 *		With page allocation, the situation is conceptually 
 *		identical, but more complex in practice.
 *
 *              In the worst case (multi file driver) page allocation 
 *		can result in two free space managers for each memory 
 *		type -- one for small (less than on equal to one page)
 *              allocations, and one for large (greater than one page)
 *              allocations.
 *
 *		In the more common one file case, page allocation will
 *              result in a total of three free space managers -- one for 
 *              small (<= one page) raw data allocations, one for small 
 *              metadata allocations (i.e, all memory types other than 
 *              H5FD_MEM_DRAW), and one for all large (> one page) 
 *              allocations.
 *
 *              Despite these complications, the solution is the same in
 *		the page allocation case -- free space managers (be they 
 *              small data or large) are assigned to the raw data free 
 *              space manager ring if they don't allocate file space for
 *              free space managers.  Note that in the one file case, the 
 *		large free space manager must be assigned to the metadata
 *		free space manager ring, as it both allocates pages for 
 *		the metadata free space manager, and allocates space for 
 *		large (> 1 page) metadata cache entries.
 *
 *              At present, the task list for this routine is:
 *
 *		1) Reduce the EOA to the extent possible.  To do this:
 *
 *		    a) Free both aggregators.  Space not at EOA will be
 *		       added to the appropriate free space manager.
 *
 *		       The raw data aggregator should not be restarted 
 *		       after this point.  It is possible that the metadata
 *		       aggregator will be.
 *
 *		    b) Free all file space currently allocated to free
 *		       space managers.
 *
 *		    c) Delete the free space manager superblock 
 *		       extension message if allocated.
 *
 *		   This done, reduce the EOA by moving it to just before
 *		   the last piece of free memory in the file.
 *
 *		2) Ensure that space is allocated for the free space
 *                 manager superblock extension message.  Must do this
 *                 now, before reallocating file space for free space 
 *		   managers, as it is possible that this allocation may
 *		   grab the last section in a FSM -- making it unnecessary
 *		   to re-allocate file space for it.
 *
 *		3) Scan all free space managers not involved in allocating
 *		   space for free space managers.  For each such free space
 *		   manager, test to see if it contains free space.  If 
 *		   it does, allocate file space for its header and section
 *		   data.  If it contains no free space, leave it without 
 *		   allocated file space as there is no need to save it to 
 *		   file.
 *
 *		   Note that all free space managers in this class should
 *		   see no further space allocations / deallocations as 
 *		   at this point, all raw data allocations should be 
 *		   finalized, as should all metadata allocations not 
 *		   involving free space managers.
 *
 *		   We will allocate space for free space managers involved
 *		   in the allocation of file space for free space managers
 *		   in H5MF_settle_meta_data_fsm()
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:  John Mainzer
 *	        5/25/16
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MF_settle_raw_data_fsm(H5F_t *f, hbool_t *fsm_settled)
{
    int pass_count;
    hsize_t alloc_size;
    H5F_mem_t mem_type;                 /* Memory type for iteration */
    H5F_mem_page_t fsm_type;            /* FSM type for iteration */
    H5O_fsinfo_t fsinfo;                /* Free space manager info message */
    H5FS_stat_t fs_stat;                /* Information for free-space manager */
    H5AC_ring_t orig_ring = H5AC_RING_INV;      /* Original ring value */
    H5AC_ring_t curr_ring = H5AC_RING_INV;      /* Current ring value */
    H5AC_ring_t needed_ring = H5AC_RING_INV;    /* Ring value needed for this iteration */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_TAG(H5AC__FREESPACE_TAG, FAIL)

    /* Check args */
    HDassert(f);
    HDassert(f->shared);
    HDassert(fsm_settled);

    /* Only need to settle things if we are persisting the free space info 
     * and allocation/deallocation has occurred.
     */
    if(f->shared->fs_persist && !f->shared->first_alloc_dealloc) {
        hbool_t fsm_opened[H5F_MEM_PAGE_NTYPES];        /* State of FSM */
        hbool_t fsm_visited[H5F_MEM_PAGE_NTYPES];       /* State of FSM */

        /* Sanity check */
        HDassert(f->shared->sblock);

        /* should only be called if file is opened R/W */
        HDassert(H5F_INTENT(f) & H5F_ACC_RDWR);

        /* shouldn't be called unless we have a superblock supporting the
         * superblock extension.
         */
        HDassert(f->shared->sblock->super_vers >= HDF5_SUPERBLOCK_VERSION_2);

        /* Initialize fsm_opened and fsm_visited */
        HDmemset(fsm_opened, 0, sizeof(fsm_opened));
        HDmemset(fsm_visited, 0, sizeof(fsm_visited));

        /* 1) Reduce the EOA to the extent possible. */

        /* a) Free the space in aggregators:
         *
         * (for space not at EOF, it may be put into free space managers)
         *
         * Do this now so that the raw data FSM (and any other FSM that isn't
         * involved in space allocation for FSMs) will have no further activity.
         *
         * Note that while the raw data aggregator should not be restarted during
         * the close process, this need not be the case for the metadata aggregator.
         *
         * Note also that the aggregators will not exist if page aggregation 
         * is enabled -- skip this if so.
         */
        /* Vailin -- is this correct? */
        if(!H5F_PAGED_AGGR(f) && (H5MF_free_aggrs(f) < 0))
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, FAIL, "can't free aggregators")

        /* Set the ring type in the DXPL.  In most cases, we will
         * need H5AC_RING_MDFSM first, so initially set the ring in
         * the DXPL to that value.  We will alter this later if
         * needed.
         */
        H5AC_set_ring(H5AC_RING_MDFSM, &orig_ring);
        curr_ring = H5AC_RING_MDFSM;

        /* b) Free the file space (if any) allocated to each free space manager.
         *
         * Do this to facilitate reduction of the size of the file to the
         * extent possible.  We will re-allocate space to free space managers
         * that have free space to save after this reduction.
         *
         * In the case of the raw data free space manager, and any other free
         * space manager that does not allocate space for free space managers,
         * allocations should be complete at this point, as all raw data should
         * have space allocated and be flushed to file by now.  Thus we
         * can examine such free space managers and only re-allocate space for
         * them if they contain free space.  Do this later in this function after
         * the EOA has been reduced to the extent possible.
         *
         * For free space managers that allocate file space for free space
         * managers (usually just a single metadata free space manager, but for
         * now at least, free space managers for different types of metadata
         * are possible), the matter is more ticklish due to the self-
         * referential nature of the problem.  These FSMs are dealt with in
         * H5MF_settle_meta_data_fsm().
         *
         * Since paged allocation may be enabled, there may be up to two 
         * free space managers per memory type -- one for small and one for
         * large allocation.  Hence we must loop over the memory types twice
         * setting the allocation size accordingly if paged allocation is 
         * enabled.
         */
        for(pass_count = 0; pass_count <= 1; pass_count++) {
            if(pass_count == 0)
                alloc_size = 1;
            else if ( H5F_PAGED_AGGR(f) )
                alloc_size = f->shared->fs_page_size + 1;
            else /* no need for a second pass */
                break;

            for(mem_type = H5FD_MEM_SUPER; mem_type < H5FD_MEM_NTYPES; H5_INC_ENUM(H5F_mem_t, mem_type)) {
                H5MF__alloc_to_fs_type(f, mem_type, alloc_size, &fsm_type);

                if(pass_count == 0) { /* this is the first pass */
                    HDassert(fsm_type > H5F_MEM_PAGE_DEFAULT);
                    HDassert(fsm_type < H5F_MEM_PAGE_LARGE_SUPER);
                } /* end if */
                else if(H5F_PAGED_AGGR(f)) { /* page alloc active */
                    HDassert(fsm_type >= H5F_MEM_PAGE_LARGE_SUPER);
                    HDassert(fsm_type < H5F_MEM_PAGE_NTYPES);
                } /* end else-if */
                else    /* paged allocation disabled -- should be unreachable */
                    HDassert(FALSE);

                if(!fsm_visited[fsm_type]) {
                    fsm_visited[fsm_type] = TRUE;

                    /* If there is no active FSM for this type, but such a FSM has
                     * space allocated in file, open it so that we can free its file
                     * space.
                     */
                    if(NULL == f->shared->fs_man[fsm_type]) {
                        if(H5F_addr_defined(f->shared->fs_addr[fsm_type])) {
                            /* Sanity check */
                            HDassert(fsm_opened[fsm_type] == FALSE);

                             if(H5MF__open_fstype(f, fsm_type) < 0)
                                 HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize file free space manager")
                            fsm_opened[fsm_type] = TRUE;
                        } /* end if */
                    } /* end if */

                    if(f->shared->fs_man[fsm_type]) {
                        /* Test to see if we need to switch rings -- do so if required */
                        if(H5MF__fsm_type_is_self_referential(f, fsm_type))
                             needed_ring = H5AC_RING_MDFSM;
                        else
                             needed_ring = H5AC_RING_RDFSM;

                        if(needed_ring != curr_ring) {
                            H5AC_set_ring(needed_ring, NULL);
                            curr_ring = needed_ring;
                        } /* end if */

                        /* Query free space manager info for this type */
                        if(H5FS_stat_info(f, f->shared->fs_man[fsm_type], &fs_stat) < 0)
                            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't get free-space info")

                        /* Check if the free space manager has space in the file */
                        if(H5F_addr_defined(fs_stat.addr) || H5F_addr_defined(fs_stat.sect_addr)) {
                            /* Delete the free space manager in the file.  Will 
                             * reallocate later if the free space manager contains
                             * any free space.
                             */
                            if(H5FS_free(f, f->shared->fs_man[fsm_type], TRUE) < 0)
                                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't release free-space headers")
                            f->shared->fs_addr[fsm_type] = HADDR_UNDEF;
                        } /* end if */
                    } /* end if */

                    /* note that we are tracking opened FSM -- we will close them
                     * at the end of the function.
                     */
                } /* end if */
            } /* end for */
        } /* end for */


        /* c) Delete the free space manager superblock extension message
         *    if allocated.
         *
         *    Must do this since the routine that writes / creates superblock
         *    extension messages will choke if the target message is
         *    unexpectedly either absent or present.
         *
         *    Update: This is probably unnecessary, as I gather that the 
         *            file space manager info message is guaranteed to exist.
         *            Leave it in for now, but consider removing it.
         */
        if(H5F_addr_defined(f->shared->sblock->ext_addr))
            if(H5F__super_ext_remove_msg(f, H5O_FSINFO_ID) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "error in removing message from superblock extension")

        /* As the final element in 1), shrink the EOA for the file */
        if(H5MF__close_shrink_eoa(f) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTSHRINK, FAIL, "can't shrink eoa")


        /* 2) Ensure that space is allocated for the free space manager superblock
         *    extension message.  Must do this now, before reallocating file space
         *    for free space managers, as it is possible that this allocation may
         *    grab the last section in a FSM -- making it unnecessary to
         *    re-allocate file space for it.
         *
         * Do this by writing a free space manager superblock extension message.
         *
         * Since no free space manager has file space allocated for it, this
         * message must be invalid since we can't save addresses of FSMs when
         * those addresses are unknown.  This is OK -- we will write the correct
         * values to the message at free space manager shutdown.
         */
        for(fsm_type = H5F_MEM_PAGE_SUPER; fsm_type < H5F_MEM_PAGE_NTYPES; H5_INC_ENUM(H5F_mem_page_t, fsm_type))
            fsinfo.fs_addr[fsm_type - 1] = HADDR_UNDEF;
        fsinfo.strategy = f->shared->fs_strategy;
        fsinfo.persist = f->shared->fs_persist;
        fsinfo.threshold = f->shared->fs_threshold;
        fsinfo.page_size = f->shared->fs_page_size;
        fsinfo.pgend_meta_thres = f->shared->pgend_meta_thres;
        fsinfo.eoa_pre_fsm_fsalloc = HADDR_UNDEF;

        if(H5F__super_ext_write_msg(f, H5O_FSINFO_ID, &fsinfo, TRUE, H5O_MSG_FLAG_MARK_IF_UNKNOWN) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_WRITEERROR, FAIL, "error in writing fsinfo message to superblock extension")


        /* 3) Scan all free space managers not involved in allocating
         *    space for free space managers.  For each such free space
         *    manager, test to see if it contains free space.  If
         *    it does, allocate file space for its header and section
         *    data.  If it contains no free space, leave it without
         *    allocated file space as there is no need to save it to
         *    file.
         *
         *    Note that all free space managers in this class should
         *    see no further space allocations / deallocations as
         *    at this point, all raw data allocations should be
         *    finalized, as should all metadata allocations not involving
         *    free space managers.
         *
         *    We will allocate space for free space managers involved
         *    in the allocation of file space for free space managers
         *    in H5MF_settle_meta_data_fsm()
         */

        /* Reinitialize fsm_visited */
        for(fsm_type = H5F_MEM_PAGE_SUPER; fsm_type < H5F_MEM_PAGE_NTYPES; H5_INC_ENUM(H5F_mem_page_t, fsm_type))
            fsm_visited[fsm_type] = FALSE;

        for(pass_count = 0; pass_count <= 1; pass_count++) {
            if(pass_count == 0)
                alloc_size = 1;
            else if(H5F_PAGED_AGGR(f))
                alloc_size = f->shared->fs_page_size + 1;
            else /* no need for a second pass */
                break;

            for(mem_type = H5FD_MEM_SUPER; mem_type < H5FD_MEM_NTYPES; H5_INC_ENUM(H5F_mem_t, mem_type)) {
                H5MF__alloc_to_fs_type(f, mem_type, alloc_size, &fsm_type);

                if(pass_count == 0) { /* this is the first pass */
                    HDassert(fsm_type > H5F_MEM_PAGE_DEFAULT);
                    HDassert(fsm_type < H5F_MEM_PAGE_LARGE_SUPER);
                } /* end if */
                else if(H5F_PAGED_AGGR(f)) { /* page alloc active */
                    HDassert(fsm_type >= H5F_MEM_PAGE_LARGE_SUPER);
                    HDassert(fsm_type < H5F_MEM_PAGE_NTYPES);
                } /* end else-if */
                else  /* paged allocation disabled -- should be unreachable */
                    HDassert(FALSE);

                /* Test to see if we need to switch rings -- do so if required */
                if(H5MF__fsm_type_is_self_referential(f, fsm_type))
                    needed_ring = H5AC_RING_MDFSM;
                else
                    needed_ring = H5AC_RING_RDFSM;

                if(needed_ring != curr_ring) {
                    H5AC_set_ring(needed_ring, NULL);
                    curr_ring = needed_ring;
                } /* end if */

                /* Since there can be a many-to-one mapping from memory types
                 * to free space managers, ensure that we don't visit any FSM
                 * more than once.
                 */
                if(!fsm_visited[fsm_type]) {
                    fsm_visited[fsm_type] = TRUE;

                    if(f->shared->fs_man[fsm_type]) {
                        /* Only allocate file space if the target free space manager 
                         * doesn't allocate file space for free space managers.  Note 
                         * that this is also the deciding factor as to whether a FSM 
                         * in in the raw data FSM ring.
                         */
                        if(!H5MF__fsm_type_is_self_referential(f, fsm_type)) {
                            /* The current ring should be H5AC_RING_RDFSM */
                            HDassert(curr_ring == H5AC_RING_RDFSM);

                            /* Query free space manager info for this type */
                            if(H5FS_stat_info(f, f->shared->fs_man[fsm_type], &fs_stat) < 0 )
                                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "can't get free-space info")

                            /* If the free space manager contains section info,
                             * allocate space for the header and sinfo (note that
                             * space must not be allocated at present -- verify
                             * verify this with assertions).
                             */
                            if(fs_stat.serial_sect_count > 0) {
                                /* Sanity check */
                                HDassert(!H5F_addr_defined(fs_stat.addr));

                                /* Allocate FSM header */
                                if(H5FS_alloc_hdr(f, f->shared->fs_man[fsm_type], &f->shared->fs_addr[fsm_type]) < 0)
                                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "can't allocated free-space header")

                                /* Allocate FSM section info */
                                HDassert(!H5F_addr_defined(fs_stat.sect_addr));
                                HDassert(fs_stat.alloc_sect_size == 0);
                                if(H5FS_alloc_sect(f, f->shared->fs_man[fsm_type]) < 0 )
                                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "can't allocate free-space section info")

#ifndef NDEBUG
                                /* Re-Query free space manager info for this type */
                                if(H5FS_stat_info(f, f->shared->fs_man[fsm_type], &fs_stat) < 0)
                                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't get free-space info")

                                HDassert(H5F_addr_defined(fs_stat.addr));
                                HDassert(H5F_addr_defined(fs_stat.sect_addr));
                                HDassert(fs_stat.serial_sect_count > 0);
                                HDassert(fs_stat.alloc_sect_size > 0);
                                HDassert(fs_stat.alloc_sect_size == fs_stat.sect_size);
#endif /* NDEBUG */
                            } /* end if */
                            else {
                                HDassert(!H5F_addr_defined(fs_stat.addr));
                                HDassert(!H5F_addr_defined(fs_stat.sect_addr));
                                HDassert(fs_stat.serial_sect_count == 0);
                                HDassert(fs_stat.alloc_sect_size == 0);
                            } /* end else */
                        } /* end if */
                    } /* end if */

                    /* Close any opened FSMs */
                    if(fsm_opened[fsm_type]) {
                        if(H5MF__close_fstype(f, fsm_type) < 0)
                            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't close file free space manager")
                        fsm_opened[fsm_type] = FALSE;
                    } /* end if */
                } /* end if */
            } /* end for */
        } /* end for */

        /* verify that all opened FSMs were closed */
        for(fsm_type = H5F_MEM_PAGE_SUPER; fsm_type < H5F_MEM_PAGE_NTYPES; H5_INC_ENUM(H5F_mem_page_t, fsm_type))
            HDassert(!fsm_opened[fsm_type]);

        /* Indicate that the FSM was settled successfully */
        *fsm_settled = TRUE;
    } /* end if */

done:
    /* Reset the ring in the API context */
    if(orig_ring != H5AC_RING_INV)
        H5AC_set_ring(orig_ring, NULL);

    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* H5MF_settle_raw_data_fsm() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_settle_meta_data_fsm()
 *
 * Purpose: 	If the free space manager is persistent, handle any tasks 
 *		required before the metadata cache can serialize or flush 
 *		the metadata free space manager(s) that handle file space 
 *		allocation for free space managers.
 *
 *		In most cases, there will be only one manager assigned 
 *		to this role.  However, since for reasons unknown,
 *		free space manager headers and section info blocks are 
 *		different classes of memory, it is possible that two free 
 *		space managers will be involved.
 *
 *		On entry to this function, the raw data settle routine
 *		(H5MF_settle_raw_data_fsm()) should have:
 *
 *              1) Freed the aggregators.
 *
 *		2) Freed all file space allocated to the free space managers.
 *
 *		3) Deleted the free space manager superblock extension message
 *
 *		4) Reduced the EOA to the extent possible.
 *
 *		5) Re-created the free space manager superblock extension
 *		   message.
 *
 *		6) Reallocated file space for all non-empty free space 
 *		   managers NOT involved in allocation of space for free 
 *		   space managers.
 *
 *		   Note that these free space managers (if not empty) should
 *		   have been written to file by this point, and that no
 *		   further space allocations involving them should take 
 *		   place during file close.
 *
 *		On entry to this routine, the free space manager(s) involved
 *		in allocation of file space for free space managers should
 *		still be floating. (i.e. should not have any file space 
 *		allocated to them.)
 *
 *		Similarly, the raw data aggregator should not have been 
 *		restarted.  Note that it is probable that reallocation of 
 *		space in 5) and 6) above will have re-started the metadata 
 *		aggregator.
 *
 *
 *		In this routine, we proceed as follows:
 *
 *		1) Verify that the free space manager(s) involved in file
 *		   space allocation for free space managers are still floating.
 *
 *              2) Free the aggregators.
 *
 *              3) Reduce the EOA to the extent possible, and make note
 *		   of the resulting value.  This value will be stored 
 *		   in the fsinfo superblock extension message and be used
 *                 in the subsequent file open.
 *
 *		4) Re-allocate space for any free space manager(s) that:
 *
 *		   a) are involved in allocation of space for free space 
 *		      managers, and 
 *
 *		   b) contain free space.
 *
 *		   It is possible that we could allocate space for one 
 *		   of these free space manager(s) only to have the allocation 
 *		   result in the free space manager being empty and thus 
 *		   obliging us to free the space again.  Thus there is the
 *		   potential for an infinite loop if we want to avoid saving
 *		   empty free space managers.
 *
 *		   Similarly, it is possible that we could allocate space 
 *		   for a section info block, only to discover that this 
 *		   allocation has changed the size of the section info -- 
 *		   forcing us to deallocate and start the loop over again.
 *
 *		   To avoid this, simply allocate file space for these
 *		   FSM(s) directly from the VFD layer if allocation is 
 *		   indicated.  This avoids the issue by bypassing the FSMs
 *		   in this case.  
 *
 *		   Note that this may increase the size of the file needlessly. 
 *		   A better solution would be to modify the FSM code to 
 *		   save empty FSMs to file, and to allow section info blocks
 *		   to be oversized.  However, given that the FSM code is 
 *		   also used by the fractal heaps, and that we are under 
 *		   severe time pressure at the moment, the above brute 
 *		   force solution is attractive. 
 *
 *              5) Make note of the EOA -- used for sanity checking on 
 *                 FSM shutdown.
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:  John Mainzer
 *	        5/25/16
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MF_settle_meta_data_fsm(H5F_t *f, hbool_t *fsm_settled)
{
    H5F_mem_page_t sm_fshdr_fs_type;	/* small fs hdr fsm */
    H5F_mem_page_t sm_fssinfo_fs_type;  /* small fs sinfo fsm */
    H5F_mem_page_t lg_fshdr_fs_type;    /* large fs hdr fsm */
    H5F_mem_page_t lg_fssinfo_fs_type;  /* large fs sinfo fsm */
    H5FS_t     *sm_hdr_fspace = NULL;   /* ptr to sm FSM hdr alloc FSM */
    H5FS_t     *sm_sinfo_fspace = NULL; /* ptr to sm FSM sinfo alloc FSM */
    H5FS_t     *lg_hdr_fspace = NULL;   /* ptr to lg FSM hdr alloc FSM */
    H5FS_t     *lg_sinfo_fspace = NULL; /* ptr to lg FSM sinfo alloc FSM */
    haddr_t eoa_pre_fsm_fsalloc;        /* eoa pre file space allocation */
                                        /* for self referential FSMs */
    haddr_t eoa_post_fsm_fsalloc;       /* eoa post file space allocation */
                                        /* for self referential FSMs */
    H5AC_ring_t orig_ring = H5AC_RING_INV; /* Original ring value */
    herr_t 	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_TAG(H5AC__FREESPACE_TAG, FAIL)

    /* Check args */
    HDassert(f);
    HDassert(f->shared);
    HDassert(fsm_settled);

    /* Only need to settle things if we are persisting the free space info 
     * and allocation/deallocation has occurred.
     */
    if(f->shared->fs_persist && !f->shared->first_alloc_dealloc) {
        /* Sanity check */
        HDassert(f->shared->lf);

        /* should only be called if file is opened R/W */
        HDassert(H5F_INTENT(f) & H5F_ACC_RDWR);

        H5MF__alloc_to_fs_type(f, H5FD_MEM_FSPACE_HDR, (size_t)1, &sm_fshdr_fs_type);
        H5MF__alloc_to_fs_type(f, H5FD_MEM_FSPACE_SINFO, (size_t)1, &sm_fssinfo_fs_type);

        HDassert(sm_fshdr_fs_type > H5F_MEM_PAGE_DEFAULT);
        HDassert(sm_fshdr_fs_type < H5F_MEM_PAGE_LARGE_SUPER);

        HDassert(sm_fssinfo_fs_type > H5F_MEM_PAGE_DEFAULT);
        HDassert(sm_fssinfo_fs_type < H5F_MEM_PAGE_LARGE_SUPER);

        HDassert(!H5F_addr_defined(f->shared->fs_addr[sm_fshdr_fs_type]));
        HDassert(!H5F_addr_defined(f->shared->fs_addr[sm_fssinfo_fs_type]));

        /* Note that in most cases, sm_hdr_fspace will equal sm_sinfo_fspace. */
        sm_hdr_fspace = f->shared->fs_man[sm_fshdr_fs_type];
        sm_sinfo_fspace = f->shared->fs_man[sm_fssinfo_fs_type];

        if(H5F_PAGED_AGGR(f)) {
            H5MF__alloc_to_fs_type(f, H5FD_MEM_FSPACE_HDR, f->shared->fs_page_size + 1, &lg_fshdr_fs_type);
            H5MF__alloc_to_fs_type(f, H5FD_MEM_FSPACE_SINFO, f->shared->fs_page_size + 1, &lg_fssinfo_fs_type);

            HDassert(lg_fshdr_fs_type >= H5F_MEM_PAGE_LARGE_SUPER);
            HDassert(lg_fshdr_fs_type < H5F_MEM_PAGE_NTYPES);

            HDassert(lg_fssinfo_fs_type >= H5F_MEM_PAGE_LARGE_SUPER);
            HDassert(lg_fssinfo_fs_type < H5F_MEM_PAGE_NTYPES);

            HDassert(!H5F_addr_defined(f->shared->fs_addr[lg_fshdr_fs_type]));
            HDassert(!H5F_addr_defined(f->shared->fs_addr[lg_fssinfo_fs_type]));

            /* Note that in most cases, lg_hdr_fspace will equal lg_sinfo_fspace. */
            lg_hdr_fspace = f->shared->fs_man[lg_fshdr_fs_type];
            lg_sinfo_fspace = f->shared->fs_man[lg_fssinfo_fs_type];
        } /* end if */

        /* Set the ring in the API context appropriately for subsequent calls */
        H5AC_set_ring(H5AC_RING_MDFSM, &orig_ring);

#ifndef NDEBUG
{
        H5FS_stat_t fs_stat;                /* Information for hdr FSM */

        /* Verify that sm_hdr_fspace is floating if it exists */
        if(sm_hdr_fspace) {
            /* Query free space manager info for this type */
            if(H5FS_stat_info(f, sm_hdr_fspace, &fs_stat) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL,  "can't get free-space info")

            HDassert(!H5F_addr_defined(fs_stat.addr));
            HDassert(!H5F_addr_defined(fs_stat.sect_addr));
            HDassert(fs_stat.alloc_sect_size == 0);
        } /* end if */

        /* Verify that sm_sinfo_fspace is floating if it exists and is distinct */
        if((sm_sinfo_fspace) && (sm_hdr_fspace != sm_sinfo_fspace)) {
            /* Query free space manager info for this type */
            if(H5FS_stat_info(f, sm_sinfo_fspace, &fs_stat) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "can't get free-space info")

            HDassert(!H5F_addr_defined(fs_stat.addr));
            HDassert(!H5F_addr_defined(fs_stat.sect_addr));
            HDassert(fs_stat.alloc_sect_size == 0);
        } /* end if */

        if(H5F_PAGED_AGGR(f)) {
            /* Verify that lg_hdr_fspace is floating if it exists */
            if(lg_hdr_fspace) {
                /* Query free space manager info for this type */
                if(H5FS_stat_info(f, lg_hdr_fspace, &fs_stat) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "can't get free-space info (3)")

                HDassert(!H5F_addr_defined(fs_stat.addr));
                HDassert(!H5F_addr_defined(fs_stat.sect_addr));
                HDassert(fs_stat.alloc_sect_size == 0);
            } /* end if */

            /* Verify that lg_sinfo_fspace is floating if it 
             * exists and is distinct 
             */
            if((lg_sinfo_fspace) && (lg_hdr_fspace != lg_sinfo_fspace)) {
                /* Query free space manager info for this type */
                if(H5FS_stat_info(f, lg_sinfo_fspace, &fs_stat) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "can't get free-space info (4)")

                HDassert(!H5F_addr_defined(fs_stat.addr));
                HDassert(!H5F_addr_defined(fs_stat.sect_addr));
                HDassert(fs_stat.alloc_sect_size == 0);
            } /* end if */
        } /* end if */
}
#endif /* NDEBUG */

        /* Free the space in the metadata aggregator.  Do this via the
         * H5MF_free_aggrs() call.  Note that the raw data aggregator must
         * have already been freed.  Sanity checks for this?
         *
         * Note that the aggregators will not exist if paged aggregation 
         * is enabled -- don't attempt to free if this is the case.
         */
        /* Vailin -- is this correct? */
        /* (for space not at EOF, it may be put into free space managers) */
        if((!H5F_PAGED_AGGR(f)) && (H5MF_free_aggrs(f) < 0))
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, FAIL, "can't free aggregators")

        /* Trying shrinking the EOA for the file */
        if(H5MF__close_shrink_eoa(f) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTSHRINK, FAIL, "can't shrink eoa")

        /* At this point, the EOA should be set to a value that contains 
         * the allocation for all user data, all non self referential FSMs,
         * the superblock and all superblock extension messages.
         * 
         * Make note of the current EOA.  We will store this value in the 
         * free space manager superblock extension message.  Since space for
         * everything other than the self referential FSMs (and possibly the
         * cache image) has been allocated at this point, this allows us to 
         * to float the self referential FSMs on the first file space allocation / 
         * deallocation and then set the EOA to this value before we handle
         * the allocation / deallocation. (If a cache image exists, the 
         * first allocation / deallocation will be the deallocation of space
         * for the cache image).   
         *
         * WARNING:  This approach settling the self referential free space 
         *           managers and allocating space for them in the file will 
         *           not work as currently implemented with the split and 
         *           multi file drivers, as the self referential free space 
         *           manager header and section info can be stored in up to 
         *           two different files -- requiring that up to two EOA's 
         *           be stored in the the free space managers super block 
         *           extension message.  
         *
         *           As of this writing, we are solving this problem by 
         *           simply not supporting persistent FSMs with the split 
         *           and multi file drivers.
         *
         *           Current plans are to do away with the multi file 
         *           driver, so this should be a non-issue in this case.
         *
         *           We should be able to support the split file driver 
         *           without a file format change.  However, the code to 
         *           do so does not exist at present.
         */
         if(HADDR_UNDEF == (eoa_pre_fsm_fsalloc = H5FD_get_eoa(f->shared->lf, H5FD_MEM_DEFAULT)))
            HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get EOA")


        /* ******************* PROBLEM: ********************
         *
         * If the file has an alignment other than 1, and if
         * the EOA is not a multiple of this alignment, allocating space
         * for the section via the VFD info has the potential of generating
         * a fragment that will be added to the free space manager.  This
         * of course undoes everything we have been doing here.
         *
         * Need a way around this.  Obvious solution is to force the EOA to
         * be a multiple of the alignment.
         *
         * Fortunately, alignment is typically 1, so this is a non-issue in
         * most cases.  In cases where the alignment is not 1, for now we
         * have decided to drop the fragment on the floor.
         *
         * Eventually, we should fix this by modifying the on disk representations
         * of free space managers to allow for empty space, so as to bypass the
         * issues created by self-referential free space managers, and make
         * this issue moot.
         */
        /* HDassert(f->shared->alignment == 1); */


        /* The free space manager(s) that handle space allocations for free
         * space managers should be settled now, albeit without file space
         * allocated to them.  To avoid the possibility of changing the sizes
         * of their section info blocks, allocate space for them now at the
         * end of file via H5FD_alloc().
         *
         * In the past, this issue of allocating space without touching the
         * free space managers has been dealt with by calling
         * H5MF_aggr_vfd_alloc(), which in turn calls H5MF_aggr_alloc().
         * This is problematic since (if I read the code correctly) it will
         * re-constitute the metadata aggregator, which will add any leftover
         * space to one of the free space managers when freed.
         *
         * This is a non-starter, since the entire objective is to settle the
         * free space managers.
         *
         * Hence the decision to call H5FD_alloc() directly.
         *
         * As discussed in PROBLEM above, if f->shared->alignment is not 1,
         * this has the possibility of generating a fragment of file space
         * that would typically be inserted into one of the free space managers.
         *
         * This is isn't good, but due to schedule pressure, we will just drop
         * the fragment on the floor for now.
         */
        if(sm_hdr_fspace)
            if(H5FS_vfd_alloc_hdr_and_section_info_if_needed(f, sm_hdr_fspace, &(f->shared->fs_addr[sm_fshdr_fs_type])) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "can't vfd allocate sm hdr FSM file space")

        if(sm_sinfo_fspace && (sm_sinfo_fspace != sm_hdr_fspace))
            if(H5FS_vfd_alloc_hdr_and_section_info_if_needed(f, sm_sinfo_fspace, &(f->shared->fs_addr[sm_fssinfo_fs_type])) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "can't vfd allocate sm sinfo FSM file space")

        if(H5F_PAGED_AGGR(f)) {
            if(lg_hdr_fspace)
                if(H5FS_vfd_alloc_hdr_and_section_info_if_needed(f, lg_hdr_fspace, &(f->shared->fs_addr[lg_fshdr_fs_type])) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "can't vfd allocate lg hdr FSM file space")

            if(lg_sinfo_fspace && (lg_sinfo_fspace != lg_hdr_fspace))
                if(H5FS_vfd_alloc_hdr_and_section_info_if_needed(f, lg_sinfo_fspace, &(f->shared->fs_addr[lg_fssinfo_fs_type])) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "can't vfd allocate lg sinfo FSM file space")
        } /* end if */

        /* Get the eoa after allocation of file space for the self referential
         * free space managers.  Assuming no cache image, this should be the 
         * final EOA of the file.
         */
        if(HADDR_UNDEF == (eoa_post_fsm_fsalloc = H5FD_get_eoa(f->shared->lf, H5FD_MEM_DEFAULT)))
            HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get file size")

        /* All free space managers should have file space allocated for them
         * now, and should see no further allocations / deallocations.  Store
         * the pre and post file space allocation for self referential FSMs EOA
         * for use when we actually write the free space manager superblock 
         * extension message.
         */
        f->shared->eoa_pre_fsm_fsalloc  = eoa_pre_fsm_fsalloc;
        f->shared->eoa_post_fsm_fsalloc = eoa_post_fsm_fsalloc;

        /* Indicate that the FSM was settled successfully */
        *fsm_settled = TRUE;
    } /* end if */

done:
    /* Reset the ring in the API context */
    if(orig_ring != H5AC_RING_INV)
        H5AC_set_ring(orig_ring, NULL);

    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* H5MF_settle_meta_data_fsm() */


/*-------------------------------------------------------------------------
 * Function:    H5MF__fsm_type_is_self_referential()
 *
 * Purpose:     Return TRUE if the indicated free space manager allocates
 *		file space for free space managers.  Return FALSE otherwise.
 *
 * Return:      TRUE/FALSE
 *
 * Programmer:  John Mainzer
 *              12/6/16
 *
 *-------------------------------------------------------------------------
 */
hbool_t
H5MF__fsm_type_is_self_referential(H5F_t *f, H5F_mem_page_t fsm_type)
{
    H5F_mem_page_t sm_fshdr_fsm;
    H5F_mem_page_t sm_fssinfo_fsm;
    H5F_mem_page_t lg_fshdr_fsm;
    H5F_mem_page_t lg_fssinfo_fsm;
    hbool_t result = FALSE;

    FUNC_ENTER_PACKAGE_NOERR

    /* Sanity check */
    HDassert(f);
    HDassert(f->shared);
    HDassert(fsm_type >= H5F_MEM_PAGE_DEFAULT);
    HDassert(fsm_type < H5F_MEM_PAGE_NTYPES);

    H5MF__alloc_to_fs_type(f, H5FD_MEM_FSPACE_HDR, (size_t)1, &sm_fshdr_fsm);
    H5MF__alloc_to_fs_type(f, H5FD_MEM_FSPACE_SINFO, (size_t)1, &sm_fssinfo_fsm);

    if(H5F_PAGED_AGGR(f)) {
        H5MF__alloc_to_fs_type(f, H5FD_MEM_FSPACE_HDR, f->shared->fs_page_size + 1, &lg_fshdr_fsm);
        H5MF__alloc_to_fs_type(f, H5FD_MEM_FSPACE_SINFO, f->shared->fs_page_size + 1, &lg_fssinfo_fsm);

        result = (fsm_type == sm_fshdr_fsm) || (fsm_type == sm_fssinfo_fsm)
                || (fsm_type == lg_fshdr_fsm) || (fsm_type == lg_fssinfo_fsm);
    } /* end if */
    else {
        /* In principle, fsm_type should always be less than 
         * H5F_MEM_PAGE_LARGE_SUPER whenever paged aggregation
         * is not enabled.  However, since there is code that does
         * not observe this principle, force the result to FALSE if
         * fsm_type is greater than or equal to H5F_MEM_PAGE_LARGE_SUPER.
         */
        if(fsm_type >= H5F_MEM_PAGE_LARGE_SUPER)
            result = FALSE;
        else
            result = (fsm_type == sm_fshdr_fsm) || (fsm_type == sm_fssinfo_fsm);
    } /* end else */

    FUNC_LEAVE_NOAPI(result)
} /* H5MF__fsm_type_is_self_referential() */


/*-------------------------------------------------------------------------
 * Function:    H5MF__fsm_is_self_referential()
 *
 * Purpose:     Return TRUE if the indicated free space manager allocates
 *		file space for free space managers.  Return FALSE otherwise.
 *
 * Return:      TRUE/FALSE
 *
 * Programmer:  John Mainzer
 *              12/6/16
 *
 *-------------------------------------------------------------------------
 */
static hbool_t
H5MF__fsm_is_self_referential(H5F_t *f, H5FS_t *fspace)
{
    H5F_mem_page_t sm_fshdr_fsm;
    H5F_mem_page_t sm_fssinfo_fsm;
    hbool_t result = FALSE;

    FUNC_ENTER_STATIC_NOERR

    /* Sanity check */
    HDassert(f);
    HDassert(f->shared);
    HDassert(fspace);

    H5MF__alloc_to_fs_type(f, H5FD_MEM_FSPACE_HDR, (size_t)1, &sm_fshdr_fsm);
    H5MF__alloc_to_fs_type(f, H5FD_MEM_FSPACE_SINFO, (size_t)1, &sm_fssinfo_fsm);

    if(H5F_PAGED_AGGR(f)) {
        H5F_mem_page_t lg_fshdr_fsm;
        H5F_mem_page_t lg_fssinfo_fsm;

        H5MF__alloc_to_fs_type(f, H5FD_MEM_FSPACE_HDR, f->shared->fs_page_size + 1, &lg_fshdr_fsm);
        H5MF__alloc_to_fs_type(f, H5FD_MEM_FSPACE_SINFO, f->shared->fs_page_size + 1, &lg_fssinfo_fsm);

        result = (fspace == f->shared->fs_man[sm_fshdr_fsm]) ||
                   (fspace == f->shared->fs_man[sm_fssinfo_fsm]) ||
                   (fspace == f->shared->fs_man[lg_fshdr_fsm]) ||
                   (fspace == f->shared->fs_man[lg_fssinfo_fsm]);
    } /* end if */
    else
        result = (fspace == f->shared->fs_man[sm_fshdr_fsm]) ||
                   (fspace == f->shared->fs_man[sm_fssinfo_fsm]);

    FUNC_LEAVE_NOAPI(result)
} /* H5MF__fsm_is_self_referential() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_tidy_self_referential_fsm_hack
 *
 * Purpose:     As discussed in the comments of the settle routines above,
 *		the existence of self referential free space managers
 *		as currently implemented creates the possibility of 
 *		infinite loops at file close.
 *
 *		As a hack to avoid this, we have added code to settle
 *		self referential free space managers, and then allocate 
 *		space for them directly from the file driver.
 *
 *		To avoid dropping ever increasing amounts of file space 
 *              on the floor with each subsequent file close/open cycle,
 *              we need to clean this up on file open.  To avoid this,
 *              this function is called on the first file space allocation
 *              or deallocation after file open to float the self referential
 *              free space managers and reduce the EOA to the value it 
 *              had before the direct allocation of space for the self 
 *              referential free space managers.
 *
 *              The function proceeds as follows:
 *
 *              1) Verify that f->shared->first_alloc_dealloc is TRUE, 
 *                 and then set it to FALSE.
 *
 *              2) Get the current EOA.  Verify that it is greater than
 *                 or equal to f->shared->eoa_pre_fsm_fsalloc.  If the 
 *                 current eoa is equal to f->shared->eoa_pre_fsm_fsalloc,
 *                 no self referential FSMs were stored, and we are done.
 *
 *                 NOTE:  This will have to be reworked somewhat for 
 *                 cache image.
 *
 *              3) Load the self referential FSMs.  In passing verify that
 *                 the lowest address of a FSM header is equal to 
 *                 f->shared->eoa_pre_fsm_fsalloc.
 *
 *                 Note that we don't have to use any special I/O for 
 *                 this -- we can use the regular I/O methods even if 
 *                 paged aggregation and page buffering is enabled.
 *
 *              4) Float the FSMs. Ensure that the file space is NOT
 *                 released.
 *
 *              5) Set EOA equal to f->shared->eoa_pre_fsm_fsalloc, 
 *                 and then set f->shared->eoa_pre_fsm_fsalloc to 
 *                 HADDR_UNDEF.
 *
 *		   If page buffering, verify that the new EOA is 
 *                 on a page boundary, and expunge any pages in the 
 *                 page buffer after the new EOA.
 *
 *              Note that this function is also called from test code
 *              when it is necessary to startup a self referential 
 *              free space manager prior to the first file space 
 *              allocation / deallocation.  Failure to do so will 
 *              result in assertion failures in this function on
 *              the first file space allocation / deallocation.
 *                 
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  John Mainzer
 *              12/11/16
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MF_tidy_self_referential_fsm_hack(H5F_t *f)
{
    haddr_t eoa;                        /* EOA of file */
    hsize_t tail_size = 0;              /* Size of chunk to free */
    H5AC_ring_t orig_ring = H5AC_RING_INV; /* Original ring value */
    haddr_t first_srfsm_hdr = HADDR_UNDEF; /* Addr of first self referential */
                                           /* fsm header in file             */
    H5FS_stat_t fs_stat;                /* Information for hdr FSM */
    H5F_mem_page_t sm_fshdr_fs_type;    /* Small fs hdr fsm */
    H5F_mem_page_t sm_fssinfo_fs_type;  /* Small fs sinfo fsm */
    H5F_mem_page_t lg_fshdr_fs_type;    /* Large fs hdr fsm */
    H5F_mem_page_t lg_fssinfo_fs_type;  /* Large fs sinfo fsm */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_TAG(H5AC__FREESPACE_TAG, FAIL)

    /* check args */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->fs_persist);
    HDassert(f->shared->first_alloc_dealloc);

    /* Set the ring type in the API context.  Since we are only dealing with 
     * self referential FSMs, we will only need H5AC_RING_MDFSM.
     */
    H5AC_set_ring(H5AC_RING_MDFSM, &orig_ring);

    /* 1) Verify that f->shared->first_alloc_dealloc is TRUE, 
     *    and then set it to FALSE.  
     */
    HDassert(f->shared->first_alloc_dealloc);
    f->shared->first_alloc_dealloc = FALSE;


    /* 2) Get the current EOA.  Verify that it is greater than
     *    or equal to f->shared->eoa_pre_fsm_fsalloc.  If the 
     *    current eoa is equal to f->shared->eoa_pre_fsm_fsalloc,
     *    no self referential FSMs were stored, and we are done.
     *
     *    NOTE:  This will have to be reworked somewhat for 
     *           cache image.
     */
    if(HADDR_UNDEF == (eoa = H5FD_get_eoa(f->shared->lf, H5FD_MEM_DEFAULT)))
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get EOA")
    HDassert(H5F_addr_le(f->shared->eoa_pre_fsm_fsalloc, eoa));

    if(H5F_addr_eq(f->shared->eoa_pre_fsm_fsalloc, eoa))
        HGOTO_DONE(SUCCEED)


    /* 3) Load the self referential FSMs.  In passing verify that
     *    the lowest address of a FSM header is equal to 
     *    f->shared->eoa_pre_fsm_fsalloc.'
     *
     *    Note that we don't have to use any special I/O for 
     *    this -- we can use the regular I/O methods even if 
     *    paged aggregation and page buffering is enabled.
     */
    H5MF__alloc_to_fs_type(f, H5FD_MEM_FSPACE_HDR, (size_t)1, &sm_fshdr_fs_type);
    H5MF__alloc_to_fs_type(f, H5FD_MEM_FSPACE_SINFO, (size_t)1, &sm_fssinfo_fs_type);
    HDassert(sm_fshdr_fs_type > H5F_MEM_PAGE_DEFAULT);
    HDassert(sm_fshdr_fs_type < H5F_MEM_PAGE_LARGE_SUPER);

    HDassert(sm_fssinfo_fs_type > H5F_MEM_PAGE_DEFAULT);
    HDassert(sm_fssinfo_fs_type < H5F_MEM_PAGE_LARGE_SUPER);

    HDassert(NULL == f->shared->fs_man[sm_fshdr_fs_type]);
    HDassert(NULL == f->shared->fs_man[sm_fssinfo_fs_type]);

    if(H5F_addr_defined(f->shared->fs_addr[sm_fshdr_fs_type])) {
        first_srfsm_hdr = f->shared->fs_addr[sm_fshdr_fs_type];

        /* open the FSM */
        if(H5MF__open_fstype(f, sm_fshdr_fs_type) < 0)
             HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL,  "can't initialize file free space manager")

        HDassert(f->shared->fs_man[sm_fshdr_fs_type]);
    } /* end if */

    if((sm_fshdr_fs_type != sm_fssinfo_fs_type) &&
            (H5F_addr_defined(f->shared->fs_addr[sm_fssinfo_fs_type]))) {

        if(!H5F_addr_defined(first_srfsm_hdr) ||
                (H5F_addr_defined(first_srfsm_hdr) &&
                    H5F_addr_lt(f->shared->fs_addr[sm_fssinfo_fs_type], first_srfsm_hdr)))
            first_srfsm_hdr = f->shared->fs_addr[sm_fssinfo_fs_type];

        HDassert(NULL == f->shared->fs_man[sm_fssinfo_fs_type]);

        /* open the FSM */
        if(H5MF__open_fstype(f, sm_fssinfo_fs_type) < 0)
             HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize file free space manager")

        HDassert(f->shared->fs_man[sm_fssinfo_fs_type]);
    } /* end if */

    if(H5F_PAGED_AGGR(f)) {
        H5MF__alloc_to_fs_type(f, H5FD_MEM_FSPACE_HDR, f->shared->fs_page_size + 1, &lg_fshdr_fs_type);
        H5MF__alloc_to_fs_type(f, H5FD_MEM_FSPACE_SINFO, f->shared->fs_page_size + 1, &lg_fssinfo_fs_type);

        HDassert(lg_fshdr_fs_type >= H5F_MEM_PAGE_LARGE_SUPER);
        HDassert(lg_fshdr_fs_type < H5F_MEM_PAGE_NTYPES);

        HDassert(lg_fssinfo_fs_type >= H5F_MEM_PAGE_LARGE_SUPER);
        HDassert(lg_fssinfo_fs_type < H5F_MEM_PAGE_NTYPES);

        HDassert(NULL == f->shared->fs_man[lg_fshdr_fs_type]);
        HDassert(NULL == f->shared->fs_man[lg_fssinfo_fs_type]);

        if(H5F_addr_defined(f->shared->fs_addr[lg_fshdr_fs_type])) {
            if(!H5F_addr_defined(first_srfsm_hdr) ||
                    (H5F_addr_defined(first_srfsm_hdr) &&
                        H5F_addr_lt(f->shared->fs_addr[lg_fshdr_fs_type], first_srfsm_hdr)))
                first_srfsm_hdr = f->shared->fs_addr[lg_fshdr_fs_type];

            HDassert(NULL == f->shared->fs_man[lg_fshdr_fs_type]);

            /* open the FSM */
            if(H5MF__open_fstype(f, lg_fshdr_fs_type) < 0)
                 HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize file free space manager")
            HDassert(f->shared->fs_man[lg_fshdr_fs_type]);
        } /* end if */

        if(lg_fshdr_fs_type != lg_fssinfo_fs_type && H5F_addr_defined(f->shared->fs_addr[lg_fssinfo_fs_type])) {
            if(!H5F_addr_defined(first_srfsm_hdr) ||
                    (H5F_addr_defined(first_srfsm_hdr) &&
                        H5F_addr_lt(f->shared->fs_addr[lg_fssinfo_fs_type], first_srfsm_hdr)))
                first_srfsm_hdr = f->shared->fs_addr[lg_fssinfo_fs_type];

            HDassert(NULL == f->shared->fs_man[lg_fssinfo_fs_type]);

            /* open the FSM */
            if(H5MF__open_fstype(f, lg_fssinfo_fs_type) < 0)
                 HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize file free space manager")
            HDassert(f->shared->fs_man[lg_fssinfo_fs_type]);
        } /* end if */
    } /* end if */
    HDassert(H5F_addr_eq(first_srfsm_hdr, f->shared->eoa_pre_fsm_fsalloc));
 
    /* 4) Float the FSMs. Ensure that the file space is NOT released. */
    if(f->shared->fs_man[sm_fshdr_fs_type]) {
        /* Sanity check: Query free space manager info for this type */
        if(H5FS_stat_info(f, f->shared->fs_man[sm_fshdr_fs_type], &fs_stat) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't get free-space info")

        HDassert(H5F_addr_defined(fs_stat.addr));
        HDassert(H5F_addr_defined(fs_stat.sect_addr));
        if(H5FS_free(f, f->shared->fs_man[sm_fshdr_fs_type], FALSE) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't release free-space headers")
        f->shared->fs_addr[sm_fshdr_fs_type] = HADDR_UNDEF;
    } /* end if */

    if(sm_fshdr_fs_type != sm_fssinfo_fs_type && f->shared->fs_man[sm_fssinfo_fs_type]) {
        /* Sanity check: Query free space manager info for this type */
        if(H5FS_stat_info(f, f->shared->fs_man[sm_fssinfo_fs_type], &fs_stat) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't get free-space info")

        HDassert(H5F_addr_defined(fs_stat.addr));
        HDassert(H5F_addr_defined(fs_stat.sect_addr));
        if(H5FS_free(f, f->shared->fs_man[sm_fssinfo_fs_type], FALSE) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't release free-space headers")
        f->shared->fs_addr[sm_fssinfo_fs_type] = HADDR_UNDEF;
    } /* end if */

    if(H5F_PAGED_AGGR(f)) {
        if(f->shared->fs_man[lg_fshdr_fs_type]) {
            /* Sanity check: Query free space manager info for this type */
            if(H5FS_stat_info(f, f->shared->fs_man[lg_fshdr_fs_type], &fs_stat) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "can't get free-space info")

            HDassert(H5F_addr_defined(fs_stat.addr));
            HDassert(H5F_addr_defined(fs_stat.sect_addr));
            if(H5FS_free(f, f->shared->fs_man[lg_fshdr_fs_type], FALSE) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't float free-space headers")
            f->shared->fs_addr[lg_fshdr_fs_type] = HADDR_UNDEF;
        } /* end if */

        if(lg_fshdr_fs_type != lg_fssinfo_fs_type && f->shared->fs_man[lg_fssinfo_fs_type]) {
            /* Sanity check: Query free space manager info for this type */
            if(H5FS_stat_info(f, f->shared->fs_man[lg_fssinfo_fs_type], &fs_stat) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "can't get free-space info")

            HDassert(H5F_addr_defined(fs_stat.addr));
            HDassert(H5F_addr_defined(fs_stat.sect_addr));
            if(H5FS_free(f, f->shared->fs_man[lg_fssinfo_fs_type], FALSE) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't float free-space headers")
            f->shared->fs_addr[lg_fssinfo_fs_type] = HADDR_UNDEF;
        } /* end if */
    } /* end if */

    /* 5) Set EOA equal to f->shared->eoa_pre_fsm_fsalloc, 
     *    and then set f->shared->eoa_pre_fsm_fsalloc to 
     *    HADDR_UNDEF.
     *
     *    If page buffering, verify that the new EOA is 
     *    on a page boundary, and expunge any pages in the 
     *    page buffer after the new EOA.
     */
    if(!H5F_PAGED_AGGR(f)) {
        /* Verify that the aggregators are still shutdown. */
        HDassert(f->shared->sdata_aggr.tot_size == 0);
        HDassert(f->shared->sdata_aggr.addr == 0);
        HDassert(f->shared->sdata_aggr.size == 0);

        HDassert(f->shared->meta_aggr.tot_size == 0);
        HDassert(f->shared->meta_aggr.addr == 0);
        HDassert(f->shared->meta_aggr.size == 0);
    } /* end if */

    tail_size = (hsize_t)(eoa - f->shared->eoa_pre_fsm_fsalloc);

    /* Release file space allocated to self referential FSMs */
    if(H5F__free(f, H5FD_MEM_DEFAULT, f->shared->eoa_pre_fsm_fsalloc, tail_size) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, FAIL, "driver free request failed")
    if(HADDR_UNDEF == (eoa = H5FD_get_eoa(f->shared->lf, H5FD_MEM_DEFAULT)))
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get EOA")
    HDassert(H5F_addr_eq(f->shared->eoa_pre_fsm_fsalloc, eoa));

    f->shared->eoa_pre_fsm_fsalloc = HADDR_UNDEF;

    HDassert((!H5F_PAGED_AGGR(f)) || (0 == (eoa % f->shared->fs_page_size)));

done:
    /* Reset the ring in the API context */
    if(orig_ring != H5AC_RING_INV)
        H5AC_set_ring(orig_ring, NULL);

    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* H5MF_tidy_self_referential_fsm_hack() */

