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

#define H5F_PACKAGE		/*suppress error about including H5Fpkg	  */
#define H5MF_PACKAGE		/*suppress error about including H5MFpkg  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fpkg.h"             /* File access				*/
#include "H5MFpkg.h"		/* File memory management		*/
#include "H5VMprivate.h"		/* Vectors and arrays 			*/


/****************/
/* Local Macros */
/****************/

#define H5MF_FSPACE_SHRINK      80              /* Percent of "normal" size to shrink serialized free space size */
#define H5MF_FSPACE_EXPAND      120             /* Percent of "normal" size to expand serialized free space size */

/* Map an allocation request type to a free list */
#define H5MF_ALLOC_TO_FS_TYPE(F, T)      ((H5FD_MEM_DEFAULT == (F)->shared->fs_type_map[T]) \
    ? (T) : (F)->shared->fs_type_map[T])


/******************/
/* Local Typedefs */
/******************/

/* Enum for kind of free space section+aggregator merging allowed for a file */
typedef enum {
    H5MF_AGGR_MERGE_SEPARATE,           /* Everything in separate free list */
    H5MF_AGGR_MERGE_DICHOTOMY,          /* Metadata in one free list and raw data in another */
    H5MF_AGGR_MERGE_TOGETHER            /* Metadata & raw data in one free list */
} H5MF_aggr_merge_t;


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

/* Allocator routines */
static herr_t H5MF_alloc_create(H5F_t *f, hid_t dxpl_id, H5FD_mem_t type);
static herr_t H5MF_alloc_close(H5F_t *f, hid_t dxpl_id, H5FD_mem_t type);


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
 * Function:	H5MF_alloc_open
 *
 * Purpose:	Open an existing free space manager of TYPE for file by
 *		creating a free-space structure
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
H5MF_alloc_open(H5F_t *f, hid_t dxpl_id, H5FD_mem_t type)
{
    const H5FS_section_class_t *classes[] = { /* Free space section classes implemented for file */
        H5MF_FSPACE_SECT_CLS_SIMPLE};
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /*
     * Check arguments.
     */
    HDassert(f);
    HDassert(f->shared);
    HDassert(type != H5FD_MEM_NOLIST);
    HDassert(H5F_addr_defined(f->shared->fs_addr[type]));
    HDassert(f->shared->fs_state[type] == H5F_FS_STATE_CLOSED);

    /* Open an existing free space structure for the file */
    if(NULL == (f->shared->fs_man[type] = H5FS_open(f, dxpl_id, f->shared->fs_addr[type],
	    NELMTS(classes), classes, f, f->shared->alignment, f->shared->threshold)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize free space info")

    /* Set the state for the free space manager to "open", if it is now */
    if(f->shared->fs_man[type])
        f->shared->fs_state[type] = H5F_FS_STATE_OPEN;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_alloc_open() */


/*-------------------------------------------------------------------------
 * Function:	H5MF_alloc_create
 *
 * Purpose:	Create free space manager of TYPE for the file by creating
 *		a free-space structure
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
H5MF_alloc_create(H5F_t *f, hid_t dxpl_id, H5FD_mem_t type)
{
    const H5FS_section_class_t *classes[] = { /* Free space section classes implemented for file */
        H5MF_FSPACE_SECT_CLS_SIMPLE};
    herr_t ret_value = SUCCEED;         /* Return value */
    H5FS_create_t fs_create; 		/* Free space creation parameters */

    FUNC_ENTER_NOAPI_NOINIT

    /*
     * Check arguments.
     */
    HDassert(f);
    HDassert(f->shared);
    HDassert(type != H5FD_MEM_NOLIST);
    HDassert(!H5F_addr_defined(f->shared->fs_addr[type]));
    HDassert(f->shared->fs_state[type] == H5F_FS_STATE_CLOSED);

    /* Set the free space creation parameters */
    fs_create.client = H5FS_CLIENT_FILE_ID;
    fs_create.shrink_percent = H5MF_FSPACE_SHRINK;
    fs_create.expand_percent = H5MF_FSPACE_EXPAND;
    fs_create.max_sect_addr = 1 + H5VM_log2_gen((uint64_t)f->shared->maxaddr);
    fs_create.max_sect_size = f->shared->maxaddr;

    if(NULL == (f->shared->fs_man[type] = H5FS_create(f, dxpl_id, NULL,
	    &fs_create, NELMTS(classes), classes, f, f->shared->alignment, f->shared->threshold)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize free space info")


    /* Set the state for the free space manager to "open", if it is now */
    if(f->shared->fs_man[type])
        f->shared->fs_state[type] = H5F_FS_STATE_OPEN;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_alloc_create() */


/*-------------------------------------------------------------------------
 * Function:	H5MF_alloc_start
 *
 * Purpose:	Open or create a free space manager of a given type
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
H5MF_alloc_start(H5F_t *f, hid_t dxpl_id, H5FD_mem_t type)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /*
     * Check arguments.
     */
    HDassert(f);
    HDassert(f->shared);
    HDassert(type != H5FD_MEM_NOLIST);

    /* Check if the free space manager exists already */
    if(H5F_addr_defined(f->shared->fs_addr[type])) {
        /* Open existing free space manager */
        if(H5MF_alloc_open(f, dxpl_id, type) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTOPENOBJ, FAIL, "can't initialize file free space")
    } /* end if */
    else {
        /* Create new free space manager */
        if(H5MF_alloc_create(f, dxpl_id, type) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTCREATE, FAIL, "can't initialize file free space")
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_alloc_start() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_alloc_close
 *
 * Purpose:     Close an existing free space manager of TYPE for file
 *
 * Return:      Success:        non-negative
 *              Failure:        negative
 *
 * Programmer: Vailin Choi; July 1st, 2009
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5MF_alloc_close(H5F_t *f, hid_t dxpl_id, H5FD_mem_t type)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /*
     * Check arguments.
     */
    HDassert(f);
    HDassert(f->shared);
    HDassert(type != H5FD_MEM_NOLIST);
    HDassert(f->shared->fs_man[type]);
    HDassert(f->shared->fs_state[type] != H5F_FS_STATE_CLOSED);

    /* Close an existing free space structure for the file */
    if(H5FS_close(f, dxpl_id, f->shared->fs_man[type]) < 0)
        HGOTO_ERROR(H5E_FSPACE, H5E_CANTRELEASE, FAIL, "can't release free space info")
    f->shared->fs_man[type] = NULL;
    f->shared->fs_state[type] = H5F_FS_STATE_CLOSED;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_alloc_close() */


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
H5MF_alloc(H5F_t *f, H5FD_mem_t alloc_type, hid_t dxpl_id, hsize_t size)
{
    H5FD_mem_t  fs_type;                /* Free space type (mapped from allocation type) */
    haddr_t	ret_value;              /* Return value */

    FUNC_ENTER_NOAPI(HADDR_UNDEF)
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: alloc_type = %u, size = %Hu\n", FUNC, (unsigned)alloc_type, size);
#endif /* H5MF_ALLOC_DEBUG */

    /* check arguments */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->lf);
    HDassert(size > 0);

    /* Get free space type from allocation type */
    fs_type = H5MF_ALLOC_TO_FS_TYPE(f, alloc_type);

    /* Check if we are using the free space manager for this file */
    if(H5F_HAVE_FREE_SPACE_MANAGER(f)) {
        /* Check if the free space manager for the file has been initialized */
        if(!f->shared->fs_man[fs_type] && H5F_addr_defined(f->shared->fs_addr[fs_type]))
            if(H5MF_alloc_open(f, dxpl_id, fs_type) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTOPENOBJ, HADDR_UNDEF, "can't initialize file free space")

        /* Search for large enough space in the free space manager */
        if(f->shared->fs_man[fs_type]) {
            H5MF_free_section_t *node;      /* Free space section pointer */
            htri_t node_found = FALSE;      /* Whether an existing free list node was found */

            /* Try to get a section from the free space manager */
            if((node_found = H5FS_sect_find(f, dxpl_id, f->shared->fs_man[fs_type], size, (H5FS_section_info_t **)&node)) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, HADDR_UNDEF, "error locating free space in file")
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Check 1.5, node_found = %t\n", FUNC, node_found);
#endif /* H5MF_ALLOC_DEBUG_MORE */

            /* Check for actually finding section */
            if(node_found) {
                /* Sanity check */
                HDassert(node);

                /* Retrieve return value */
                ret_value = node->sect_info.addr;

                /* Check for eliminating the section */
                if(node->sect_info.size == size) {
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Check 1.6, freeing node\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG_MORE */
                    /* Free section node */
                    if(H5MF_sect_simple_free((H5FS_section_info_t *)node) < 0)
                        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, HADDR_UNDEF, "can't free simple section node")
                } /* end if */
                else {
                    H5MF_sect_ud_t udata;               /* User data for callback */

                    /* Adjust information for section */
                    node->sect_info.addr += size;
                    node->sect_info.size -= size;

                    /* Construct user data for callbacks */
                    udata.f = f;
                    udata.dxpl_id = dxpl_id;
                    udata.alloc_type = alloc_type;
                    udata.allow_sect_absorb = TRUE;
		    udata.allow_eoa_shrink_only = FALSE; 

#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Check 1.7, re-adding node, node->sect_info.size = %Hu\n", FUNC, node->sect_info.size);
#endif /* H5MF_ALLOC_DEBUG_MORE */
                    /* Re-insert section node into file's free space */
                    if(H5FS_sect_add(f, dxpl_id, f->shared->fs_man[fs_type], (H5FS_section_info_t *)node, H5FS_ADD_RETURNED_SPACE, &udata) < 0)
                        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINSERT, HADDR_UNDEF, "can't re-add section to file free space")
                } /* end else */

                /* Leave now */
                HGOTO_DONE(ret_value)
            } /* end if */
        } /* end if */
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Check 2.0\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG_MORE */
    } /* end if */

    /* Allocate from the metadata aggregator (or the VFD) */
    if(HADDR_UNDEF == (ret_value = H5MF_aggr_vfd_alloc(f, alloc_type, dxpl_id, size)))
	HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, HADDR_UNDEF, "allocation failed from aggr/vfd")

done:
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Leaving: ret_value = %a, size = %Hu\n", FUNC, ret_value, size);
#endif /* H5MF_ALLOC_DEBUG */
#ifdef H5MF_ALLOC_DEBUG_DUMP
H5MF_sects_dump(f, dxpl_id, stderr);
#endif /* H5MF_ALLOC_DEBUG_DUMP */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_alloc() */


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
    haddr_t ret_value;          /* Return value */

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
    if(HADDR_UNDEF == (eoa = H5FD_get_eoa(f->shared->lf, H5FD_MEM_DEFAULT)))
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
H5MF_xfree(H5F_t *f, H5FD_mem_t alloc_type, hid_t dxpl_id, haddr_t addr,
    hsize_t size)
{
    H5MF_free_section_t *node = NULL;   /* Free space section pointer */
    H5MF_sect_ud_t udata;               /* User data for callback */
    H5FD_mem_t fs_type;                 /* Free space type (mapped from allocation type) */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Entering - alloc_type = %u, addr = %a, size = %Hu\n", FUNC, (unsigned)alloc_type, addr, size);
#endif /* H5MF_ALLOC_DEBUG */

    /* check arguments */
    HDassert(f);
    if(!H5F_addr_defined(addr) || 0 == size)
        HGOTO_DONE(SUCCEED);
    HDassert(addr != 0);        /* Can't deallocate the superblock :-) */

    /* Check for attempting to free space that's a 'temporary' file address */
    if(H5F_addr_le(f->shared->tmp_addr, addr))
        HGOTO_ERROR(H5E_RESOURCE, H5E_BADRANGE, FAIL, "attempting to free temporary file space")

    /* Check if the space to free intersects with the file's metadata accumulator */
    if(H5F_accum_free(f, dxpl_id, alloc_type, addr, size) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, FAIL, "can't check free space intersection w/metadata accumulator")

    /* Get free space type from allocation type */
    fs_type = H5MF_ALLOC_TO_FS_TYPE(f, alloc_type);
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: fs_type = %u\n", FUNC, (unsigned)fs_type);
#endif /* H5MF_ALLOC_DEBUG_MORE */

    /* Check if the free space manager for the file has been initialized */
    if(!f->shared->fs_man[fs_type]) {
        /* If there's no free space manager for objects of this type,
         *  see if we can avoid creating one by checking if the freed
         *  space is at the end of the file
         */
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: f->shared->fs_addr[%u] = %a\n", FUNC, (unsigned)fs_type, f->shared->fs_addr[fs_type]);
#endif /* H5MF_ALLOC_DEBUG_MORE */
        if(!H5F_addr_defined(f->shared->fs_addr[fs_type])) {
            htri_t status;          /* "can absorb" status for section into */

#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Trying to avoid starting up free space manager\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG_MORE */
            /* Try to shrink the file or absorb the block into a block aggregator */
            if((status = H5MF_try_shrink(f, alloc_type, dxpl_id, addr, size)) < 0)
                HGOTO_ERROR(H5E_FSPACE, H5E_CANTMERGE, FAIL, "can't check for absorbing block")
            else if(status > 0)
                /* Indicate success */
                HGOTO_DONE(SUCCEED)
        } /* end if */

        /* If we are deleting the free space manager, leave now, to avoid
         *  [re-]starting it.
	 * or if file space strategy type is not using a free space manager
	 *   (H5F_FILE_SPACE_AGGR_VFD or H5F_FILE_SPACE_VFD), drop free space
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
        if(H5MF_alloc_start(f, dxpl_id, fs_type) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize file free space")
    } /* end if */
    HDassert(f->shared->fs_man[fs_type]);

    /* Create free space section for block */
    if(NULL == (node = H5MF_sect_simple_new(addr, size)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize free space section")

    /* Construct user data for callbacks */
    udata.f = f;
    udata.dxpl_id = dxpl_id;
    udata.alloc_type = alloc_type;
    udata.allow_sect_absorb = TRUE;
    udata.allow_eoa_shrink_only = FALSE; 

    /* Add to the free space for the file */
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Before H5FS_sect_add()\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG_MORE */
    if(H5FS_sect_add(f, dxpl_id, f->shared->fs_man[fs_type], (H5FS_section_info_t *)node, H5FS_ADD_RETURNED_SPACE, &udata) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINSERT, FAIL, "can't add section to file free space")
    node = NULL;
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: After H5FS_sect_add()\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG_MORE */

done:
    /* Release section node, if allocated and not added to section list or merged */
    if(node)
        if(H5MF_sect_simple_free((H5FS_section_info_t *)node) < 0)
            HDONE_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't free simple section node")

#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Leaving, ret_value = %d\n", FUNC, ret_value);
#endif /* H5MF_ALLOC_DEBUG */
#ifdef H5MF_ALLOC_DEBUG_DUMP
H5MF_sects_dump(f, dxpl_id, stderr);
#endif /* H5MF_ALLOC_DEBUG_DUMP */
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_xfree() */


/*-------------------------------------------------------------------------
 * Function:	H5MF_try_extend
 *
 * Purpose:	Extend a block in the file if possible.
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
H5MF_try_extend(H5F_t *f, hid_t dxpl_id, H5FD_mem_t alloc_type, haddr_t addr,
    hsize_t size, hsize_t extra_requested)
{
    haddr_t     end;            /* End of block to extend */
    H5FD_mem_t  map_type;       /* Mapped type */
    htri_t	ret_value;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Entering: alloc_type = %u, addr = %a, size = %Hu, extra_requested = %Hu\n", FUNC, (unsigned)alloc_type, addr, size, extra_requested);
#endif /* H5MF_ALLOC_DEBUG */

    /* Sanity check */
    HDassert(f);
    HDassert(H5F_INTENT(f) & H5F_ACC_RDWR);

    /* Set mapped type, treating global heap as raw data */
    map_type = (alloc_type == H5FD_MEM_GHEAP) ? H5FD_MEM_DRAW : alloc_type;

    /* Compute end of block to extend */
    end = addr + size;

    /* Check if the block is exactly at the end of the file */
    if((ret_value = H5FD_try_extend(f->shared->lf, map_type, f, end, extra_requested)) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTEXTEND, FAIL, "error extending file")
    else if(ret_value == FALSE) {
        H5F_blk_aggr_t *aggr;   /* Aggregator to use */

        /* Check for test block able to extend aggregation block */
        aggr = (map_type == H5FD_MEM_DRAW) ?  &(f->shared->sdata_aggr) : &(f->shared->meta_aggr);
        if((ret_value = H5MF_aggr_try_extend(f, aggr, map_type, end, extra_requested)) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTEXTEND, FAIL, "error extending aggregation block")
        else if(ret_value == FALSE) {
            H5FD_mem_t  fs_type;                /* Free space type (mapped from allocation type) */

            /* Get free space type from allocation type */
            fs_type = H5MF_ALLOC_TO_FS_TYPE(f, alloc_type);

            /* Check if the free space for the file has been initialized */
            if(!f->shared->fs_man[fs_type] && H5F_addr_defined(f->shared->fs_addr[fs_type]))
                if(H5MF_alloc_open(f, dxpl_id, fs_type) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize file free space")

            /* Check for test block able to block in free space manager */
            if(f->shared->fs_man[fs_type])
                if((ret_value = H5FS_sect_try_extend(f, dxpl_id, f->shared->fs_man[fs_type], addr, size, extra_requested)) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTEXTEND, FAIL, "error extending block in free space manager")
        } /* end if */
    } /* end if */

done:
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Leaving: ret_value = %t\n", FUNC, ret_value);
#endif /* H5MF_ALLOC_DEBUG */
#ifdef H5MF_ALLOC_DEBUG_DUMP
H5MF_sects_dump(f, dxpl_id, stderr);
#endif /* H5MF_ALLOC_DEBUG_DUMP */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_try_extend() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_get_freespace
 *
 * Purpose:     Retrieve the amount of free space in a file.
 *
 * Return:      Success:        Amount of free space in file
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Monday, October  6, 2003
 *
 * Modifications:
 *      Vailin Choi; July 2012
 *      As the default free-list mapping is changed to H5FD_FLMAP_DICHOTOMY,
 *      checks are added to account for the last section of each free-space manager
 *      and the remaining space in the two aggregators are at EOF.
 *-------------------------------------------------------------------------
 */
herr_t
H5MF_get_freespace(H5F_t *f, hid_t dxpl_id, hsize_t *tot_space, hsize_t *meta_size)
{
    haddr_t eoa;                /* End of allocated space in the file */
    haddr_t ma_addr = HADDR_UNDEF;    /* Base "metadata aggregator" address */
    hsize_t ma_size = 0;        /* Size of "metadata aggregator" */
    haddr_t sda_addr = HADDR_UNDEF;    /* Base "small data aggregator" address */
    hsize_t sda_size = 0;       /* Size of "small data aggregator" */
    hsize_t tot_fs_size = 0;    /* Amount of all free space managed */
    hsize_t tot_meta_size = 0;  /* Amount of metadata for free space managers */
    H5FD_mem_t type;            /* Memory type for iteration */
    H5FD_mem_t fs_started[H5FD_MEM_NTYPES]; /* Indicate whether the free-space manager has been started */
    hbool_t eoa_shrank;		/* Whether an EOA shrink occurs */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->lf);

    /* Retrieve the 'eoa' for the file */
    if(HADDR_UNDEF == (eoa = H5FD_get_eoa(f->shared->lf, H5FD_MEM_DEFAULT)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "driver get_eoa request failed")

    /* Retrieve metadata aggregator info, if available */
    if(H5MF_aggr_query(f, &(f->shared->meta_aggr), &ma_addr, &ma_size) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "can't query metadata aggregator stats")

    /* Retrieve 'small data' aggregator info, if available */
    if(H5MF_aggr_query(f, &(f->shared->sdata_aggr), &sda_addr, &sda_size) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "can't query small data aggregator stats")

    /* Iterate over all the free space types that have managers and get each free list's space */
    for(type = H5FD_MEM_DEFAULT; type < H5FD_MEM_NTYPES; H5_INC_ENUM(H5FD_mem_t, type)) {

	fs_started[type] = FALSE;

	/* Check if the free space for the file has been initialized */
        if(!f->shared->fs_man[type] && H5F_addr_defined(f->shared->fs_addr[type])) {
            if(H5MF_alloc_open(f, dxpl_id, type) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize file free space")
            HDassert(f->shared->fs_man[type]);
            fs_started[type] = TRUE;
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

    /* Iterate until no more EOA shrink occurs */
    do {
	eoa_shrank = FALSE;

	/* Check the last section of each free-space manager */
	for(type = H5FD_MEM_DEFAULT; type < H5FD_MEM_NTYPES; H5_INC_ENUM(H5FD_mem_t, type)) {
	    haddr_t sect_addr = HADDR_UNDEF;
	    hsize_t sect_size = 0;

	    if(f->shared->fs_man[type]) {
		if(H5FS_sect_query_last_sect(f->shared->fs_man[type], &sect_addr, &sect_size) < 0)
		    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "can't query last section on merge list")

		/* Deduct space from previous accumulation if the section is at EOA */
		if(H5F_addr_eq(sect_addr + sect_size, eoa)) {
		    eoa = sect_addr;
		    eoa_shrank = TRUE;
		    tot_fs_size -= sect_size;
		} /* end if */
	    } /* end if */
	} /* end for */

	/* Check the metadata and raw data aggregators */
	if(ma_size > 0 && H5F_addr_eq(ma_addr + ma_size, eoa)) {
	    eoa = ma_addr;
	    eoa_shrank = TRUE;
	    ma_size = 0;
	} /* end if */
	if(sda_size > 0 && H5F_addr_eq(sda_addr + sda_size, eoa)) {
	    eoa = sda_addr;
	    eoa_shrank = TRUE;
	    sda_size = 0;
	} /* end if */
    } while(eoa_shrank);

    /* Close the free-space managers if they were opened earlier in this routine */
    for(type = H5FD_MEM_DEFAULT; type < H5FD_MEM_NTYPES; H5_INC_ENUM(H5FD_mem_t, type)) {
	if(fs_started[type])
            if(H5MF_alloc_close(f, dxpl_id, type) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't close file free space")
    } /* end for */

    /* Set the value(s) to return */
    /* (The metadata & small data aggregators count as free space now, since they aren't at EOA) */
    if(tot_space)
	*tot_space = tot_fs_size + ma_size + sda_size;
    if(meta_size)
	*meta_size = tot_meta_size;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_get_freespace() */


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
H5MF_try_shrink(H5F_t *f, H5FD_mem_t alloc_type, hid_t dxpl_id, haddr_t addr,
    hsize_t size)
{
    H5MF_free_section_t *node = NULL;   /* Free space section pointer */
    H5MF_sect_ud_t udata;               /* User data for callback */
    htri_t ret_value;                   /* Return value */

    FUNC_ENTER_NOAPI(FAIL)
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Entering - alloc_type = %u, addr = %a, size = %Hu\n", FUNC, (unsigned)alloc_type, addr, size);
#endif /* H5MF_ALLOC_DEBUG */

    /* check arguments */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->lf);
    HDassert(H5F_addr_defined(addr));
    HDassert(size > 0);

    /* Create free space section for block */
    if(NULL == (node = H5MF_sect_simple_new(addr, size)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't initialize free space section")

    /* Construct user data for callbacks */
    udata.f = f;
    udata.dxpl_id = dxpl_id;
    udata.alloc_type = alloc_type;
    udata.allow_sect_absorb = FALSE;    /* Force section to be absorbed into aggregator */
    udata.allow_eoa_shrink_only = FALSE; 

    /* Call the "can shrink" callback for the section */
    if((ret_value = H5MF_sect_simple_can_shrink((const H5FS_section_info_t *)node, &udata)) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTMERGE, FAIL, "can't check if section can shrink container")
    else if(ret_value > 0) {
        /* Shrink or absorb the section */
        if(H5MF_sect_simple_shrink((H5FS_section_info_t **)&node, &udata) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTSHRINK, FAIL, "can't shrink container")
    } /* end if */

done:
    /* Free section node allocated */
    if(node && H5MF_sect_simple_free((H5FS_section_info_t *)node) < 0)
        HDONE_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't free simple section node")

#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Leaving, ret_value = %d\n", FUNC, ret_value);
#endif /* H5MF_ALLOC_DEBUG */
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_try_shrink() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_close_shrink_eoa
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
H5MF_close_shrink_eoa(H5F_t *f, hid_t dxpl_id)
{
    H5FD_mem_t type;            /* Memory type for iteration */
    hbool_t eoa_shrank;		/* Whether an EOA shrink occurs */
    htri_t status;		/* Status value */
    H5MF_sect_ud_t udata;	/* User data for callback */
    herr_t ret_value = SUCCEED;	/* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(f);
    HDassert(f->shared);

    /* Construct user data for callbacks */
    udata.f = f;
    udata.dxpl_id = dxpl_id;
    udata.allow_sect_absorb = FALSE;    
    udata.allow_eoa_shrink_only = TRUE; 

    /* Iterate until no more EOA shrinking occurs */
    do {
	eoa_shrank = FALSE;

	/* Check the last section of each free-space manager */
	for(type = H5FD_MEM_DEFAULT; type < H5FD_MEM_NTYPES; H5_INC_ENUM(H5FD_mem_t, type)) {
	    if(f->shared->fs_man[type]) {
		udata.alloc_type = type;
		if((status = H5FS_sect_try_shrink_eoa(f, dxpl_id, f->shared->fs_man[type], &udata)) < 0)
		    HGOTO_ERROR(H5E_FSPACE, H5E_CANTSHRINK, FAIL, "can't check for shrinking eoa")
		else if(status > 0)
		    eoa_shrank = TRUE;
	    } /* end if */
	} /* end for */

	/* check the two aggregators */
	if((status = H5MF_aggrs_try_shrink_eoa(f, dxpl_id)) < 0)
	    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTSHRINK, FAIL, "can't check for shrinking eoa")
	else if(status > 0)
	    eoa_shrank = TRUE;
    } while(eoa_shrank);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_close_shrink_eoa() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_close
 *
 * Purpose:     Close the free space tracker(s) for a file
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, January 22, 2008
 *
 * Modifications:
 *      Vailin Choi; July 2012
 *      As the default free-list mapping is changed to H5FD_FLMAP_DICHOTOMY,
 *      modifications are needed to shrink EOA if the last section of each free-space manager
 *      and the remaining space in the two aggregators are at EOA.

 *-------------------------------------------------------------------------
 */
herr_t
H5MF_close(H5F_t *f, hid_t dxpl_id)
{
    H5FD_mem_t type;                    /* Memory type for iteration */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Entering\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG */

    /* check args */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->lf);

    /* Free the space in aggregators */
    /* (for space not at EOF, it may be put into free space managers) */
    if(H5MF_free_aggrs(f, dxpl_id) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTFREE, FAIL, "can't free aggregators")

    /* Trying shrinking the EOA for the file */
    if(H5MF_close_shrink_eoa(f, dxpl_id) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTSHRINK, FAIL, "can't shrink eoa")

    /* Iterate over all the free space types that have managers and get each free list's space */
    for(type = H5FD_MEM_DEFAULT; type < H5FD_MEM_NTYPES; H5_INC_ENUM(H5FD_mem_t, type)) {
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Check 1.0 - f->shared->fs_man[%u] = %p, f->shared->fs_addr[%u] = %a\n", FUNC, (unsigned)type, f->shared->fs_man[type], (unsigned)type, f->shared->fs_addr[type]);
#endif /* H5MF_ALLOC_DEBUG_MORE */
        /* If the free space manager for this type is open, close it */
        if(f->shared->fs_man[type]) {
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Before closing free space manager\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG_MORE */
            if(H5FS_close(f, dxpl_id, f->shared->fs_man[type]) < 0)
                HGOTO_ERROR(H5E_FSPACE, H5E_CANTRELEASE, FAIL, "can't release free space info")
            f->shared->fs_man[type] = NULL;
            f->shared->fs_state[type] = H5F_FS_STATE_CLOSED;
        } /* end if */
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Check 2.0 - f->shared->fs_man[%u] = %p, f->shared->fs_addr[%u] = %a\n", FUNC, (unsigned)type, f->shared->fs_man[type], (unsigned)type, f->shared->fs_addr[type]);
#endif /* H5MF_ALLOC_DEBUG_MORE */

        /* If there is free space manager info for this type, delete it */
        /* (XXX: Make this optional when free space for a file can be persistant) */
        if(H5F_addr_defined(f->shared->fs_addr[type])) {
            haddr_t tmp_fs_addr;            /* Temporary holder for free space manager address */

            /* Put address into temporary variable and reset it */
            /* (Avoids loopback in file space freeing routine) */
            tmp_fs_addr = f->shared->fs_addr[type];
            f->shared->fs_addr[type] = HADDR_UNDEF;

            /* Shift to "deleting" state, to make certain we don't track any
             *  file space freed as a result of deleting the free space manager.
             */
            f->shared->fs_state[type] = H5F_FS_STATE_DELETING;

#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: Before deleting free space manager\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG_MORE */
            /* Delete free space manager for this type */
	    if(H5FS_delete(f, dxpl_id, tmp_fs_addr) < 0)
		HGOTO_ERROR(H5E_FSPACE, H5E_CANTFREE, FAIL, "can't delete free space manager")

            /* Shift [back] to closed state */
            HDassert(f->shared->fs_state[type] == H5F_FS_STATE_DELETING);
            f->shared->fs_state[type] = H5F_FS_STATE_CLOSED;

            /* Sanity check that the free space manager for this type wasn't started up again */
            HDassert(!H5F_addr_defined(f->shared->fs_addr[type]));
        } /* end if */
    } /* end for */

    /* Free the space in aggregators (again) */
    /* (in case any free space information re-started them) */
    if(H5MF_free_aggrs(f, dxpl_id) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTFREE, FAIL, "can't free aggregators")

    /* Trying shrinking the EOA for the file */
    /* (in case any free space is now at the EOA) */
    if(H5MF_close_shrink_eoa(f, dxpl_id) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTSHRINK, FAIL, "can't shrink eoa")

done:
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Leaving\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG */
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_close() */

