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
 * Programmer:	Quincey Koziol <koziol@hdfgroup.org>
 *              Tuesday, January  8, 2008
 *
 * Purpose:	Free space section callbacks for file.
 *
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

/* 'simple' section callbacks */
static H5FS_section_info_t *H5MF_sect_simple_deserialize(const H5FS_section_class_t *cls,
    hid_t dxpl_id, const uint8_t *buf, haddr_t sect_addr, hsize_t sect_size,
    unsigned *des_flags);
static htri_t H5MF_sect_simple_can_merge(const H5FS_section_info_t *sect1,
    const H5FS_section_info_t *sect2, void *udata);
static herr_t H5MF_sect_simple_merge(H5FS_section_info_t *sect1,
    H5FS_section_info_t *sect2, void *udata);
static herr_t H5MF_sect_simple_valid(const H5FS_section_class_t *cls,
    const H5FS_section_info_t *sect);
static H5FS_section_info_t *H5MF_sect_simple_split(H5FS_section_info_t *sect,
    hsize_t frag_size);

/*********************/
/* Package Variables */
/*********************/

/* Class info for "simple" free space sections */
H5FS_section_class_t H5MF_FSPACE_SECT_CLS_SIMPLE[1] = {{
    /* Class variables */
    H5MF_FSPACE_SECT_SIMPLE,		/* Section type                 */
    0,					/* Extra serialized size        */
    H5FS_CLS_MERGE_SYM | H5FS_CLS_ADJUST_OK, /* Class flags                  */
    NULL,				/* Class private info           */

    /* Class methods */
    NULL,				/* Initialize section class     */
    NULL,				/* Terminate section class      */

    /* Object methods */
    NULL,				/* Add section                  */
    NULL,				/* Serialize section            */
    H5MF_sect_simple_deserialize,	/* Deserialize section          */
    H5MF_sect_simple_can_merge,		/* Can sections merge?          */
    H5MF_sect_simple_merge,		/* Merge sections               */
    H5MF_sect_simple_can_shrink,	/* Can section shrink container?*/
    H5MF_sect_simple_shrink,		/* Shrink container w/section   */
    H5MF_sect_simple_free,		/* Free section                 */
    H5MF_sect_simple_valid,		/* Check validity of section    */
    H5MF_sect_simple_split,		/* Split section node for alignment */
    NULL,				/* Dump debugging for section   */
}};


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/

/* Declare a free list to manage the H5MF_free_section_t struct */
H5FL_DEFINE(H5MF_free_section_t);



/*-------------------------------------------------------------------------
 * Function:	H5MF_sect_simple_new
 *
 * Purpose:	Create a new 'simple' section and return it to the caller
 *
 * Return:	Pointer to new section on success/NULL on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		January  8 2008
 *
 *-------------------------------------------------------------------------
 */
H5MF_free_section_t *
H5MF_sect_simple_new(haddr_t sect_off, hsize_t sect_size)
{
    H5MF_free_section_t *sect = NULL;   /* 'Simple' free space section to add */
    H5MF_free_section_t *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5MF_sect_simple_new)

    /* Check arguments.  */
    HDassert(sect_size);

    /* Create free space section node */
    if(NULL == (sect = H5FL_MALLOC(H5MF_free_section_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for direct block free list section")

    /* Set the information passed in */
    sect->sect_info.addr = sect_off;
    sect->sect_info.size = sect_size;

    /* Set the section's class & state */
    sect->sect_info.type = H5MF_FSPACE_SECT_SIMPLE;
    sect->sect_info.state = H5FS_SECT_LIVE;

    /* Set return value */
    ret_value = sect;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_sect_simple_new() */


/*-------------------------------------------------------------------------
 * Function:	H5MF_sect_simple_deserialize
 *
 * Purpose:	Deserialize a buffer into a "live" single section
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, January  8, 2008
 *
 *-------------------------------------------------------------------------
 */
static H5FS_section_info_t *
H5MF_sect_simple_deserialize(const H5FS_section_class_t UNUSED *cls,
    hid_t UNUSED dxpl_id, const uint8_t UNUSED *buf, haddr_t sect_addr,
    hsize_t sect_size, unsigned UNUSED *des_flags)
{
    H5MF_free_section_t *sect;          /* New section */
    H5FS_section_info_t *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5MF_sect_simple_deserialize)

    /* Check arguments. */
    HDassert(H5F_addr_defined(sect_addr));
    HDassert(sect_size);

    /* Create free space section for block */
    if(NULL == (sect = H5MF_sect_simple_new(sect_addr, sect_size)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, NULL, "can't initialize free space section")

    /* Set return value */
    ret_value = (H5FS_section_info_t *)sect;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5MF_sect_simple_deserialize() */


/*-------------------------------------------------------------------------
 * Function:	H5MF_sect_simple_can_merge
 *
 * Purpose:	Can two sections of this type merge?
 *
 * Note:        Second section must be "after" first section
 *
 * Return:	Success:	non-negative (TRUE/FALSE)
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, January  8, 2008
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5MF_sect_simple_can_merge(const H5FS_section_info_t *_sect1,
    const H5FS_section_info_t *_sect2, void UNUSED *_udata)
{
    const H5MF_free_section_t *sect1 = (const H5MF_free_section_t *)_sect1;   /* File free section */
    const H5MF_free_section_t *sect2 = (const H5MF_free_section_t *)_sect2;   /* File free section */
    htri_t ret_value;                   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5MF_sect_simple_can_merge)

    /* Check arguments. */
    HDassert(sect1);
    HDassert(sect2);
    HDassert(sect1->sect_info.type == sect2->sect_info.type);   /* Checks "MERGE_SYM" flag */
    HDassert(H5F_addr_lt(sect1->sect_info.addr, sect2->sect_info.addr));

    /* Check if second section adjoins first section */
    ret_value = H5F_addr_eq(sect1->sect_info.addr + sect1->sect_info.size, sect2->sect_info.addr);

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5MF_sect_simple_can_merge() */


/*-------------------------------------------------------------------------
 * Function:	H5MF_sect_simple_merge
 *
 * Purpose:	Merge two sections of this type
 *
 * Note:        Second section always merges into first node
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, January  8, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5MF_sect_simple_merge(H5FS_section_info_t *_sect1, H5FS_section_info_t *_sect2,
    void UNUSED *_udata)
{
    H5MF_free_section_t *sect1 = (H5MF_free_section_t *)_sect1;   /* File free section */
    H5MF_free_section_t *sect2 = (H5MF_free_section_t *)_sect2;   /* File free section */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5MF_sect_simple_merge)

    /* Check arguments. */
    HDassert(sect1);
    HDassert(sect1->sect_info.type == H5MF_FSPACE_SECT_SIMPLE);
    HDassert(sect2);
    HDassert(sect2->sect_info.type == H5MF_FSPACE_SECT_SIMPLE);
    HDassert(H5F_addr_eq(sect1->sect_info.addr + sect1->sect_info.size, sect2->sect_info.addr));

    /* Add second section's size to first section */
    sect1->sect_info.size += sect2->sect_info.size;

    /* Get rid of second section */
    if(H5MF_sect_simple_free((H5FS_section_info_t *)sect2) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't free section node")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5MF_sect_simple_merge() */


/*-------------------------------------------------------------------------
 * Function:	H5MF_sect_simple_can_shrink
 *
 * Purpose:	Can this section shrink the container?
 *
 * Return:	Success:	non-negative (TRUE/FALSE)
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, January  8, 2008
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5MF_sect_simple_can_shrink(const H5FS_section_info_t *_sect, void *_udata)
{
    const H5MF_free_section_t *sect = (const H5MF_free_section_t *)_sect;   /* File free section */
    H5MF_sect_ud_t *udata = (H5MF_sect_ud_t *)_udata;   /* User data for callback */
    haddr_t eoa;                /* End of address space in the file */
    haddr_t end;                /* End of section to extend */
    htri_t ret_value;           /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5MF_sect_simple_can_shrink)

    /* Check arguments. */
    HDassert(sect);
    HDassert(udata);
    HDassert(udata->f);

    /* Retrieve the end of the file's address space */
    if(HADDR_UNDEF == (eoa = H5FD_get_eoa(udata->f->shared->lf, udata->alloc_type)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "driver get_eoa request failed")

    /* Compute address of end of section to check */
    end = sect->sect_info.addr + sect->sect_info.size;

    /* Check if the section is exactly at the end of the allocated space in the file */
    if(H5F_addr_eq(end, eoa)) {
        /* Set the shrinking type */
        udata->shrink = H5MF_SHRINK_EOA;
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: section {%a, %Hu}, shrinks file, eoa = %a\n", FUNC, sect->sect_info.addr, sect->sect_info.size, eoa);
#endif /* H5MF_ALLOC_DEBUG_MORE */

        /* Indicate shrinking can occur */
        HGOTO_DONE(TRUE)
    } /* end if */
    else {
        /* Check if this section is allowed to merge with metadata aggregation block */
        if(udata->f->shared->fs_aggr_merge[udata->alloc_type] & H5F_FS_MERGE_METADATA) {
            htri_t status;              /* Status from aggregator adjoin */

            /* See if section can absorb the aggregator & vice versa */
            if((status = H5MF_aggr_can_absorb(udata->f, &(udata->f->shared->meta_aggr), sect, &(udata->shrink))) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTMERGE, FAIL, "error merging section with aggregation block")
            else if(status > 0) {
                /* Set the aggregator to operate on */
                udata->aggr = &(udata->f->shared->meta_aggr);
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: section {%a, %Hu}, adjoins metadata aggregator\n", FUNC, sect->sect_info.addr, sect->sect_info.size);
#endif /* H5MF_ALLOC_DEBUG_MORE */

                /* Indicate shrinking can occur */
                HGOTO_DONE(TRUE)
            } /* end if */
        } /* end if */

        /* Check if this section is allowed to merge with small 'raw' aggregation block */
        if(udata->f->shared->fs_aggr_merge[udata->alloc_type] & H5F_FS_MERGE_RAWDATA) {
            htri_t status;              /* Status from aggregator adjoin */

            /* See if section can absorb the aggregator & vice versa */
            if((status = H5MF_aggr_can_absorb(udata->f, &(udata->f->shared->sdata_aggr), sect, &(udata->shrink))) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTMERGE, FAIL, "error merging section with aggregation block")
            else if(status > 0) {
                /* Set the aggregator to operate on */
                udata->aggr = &(udata->f->shared->sdata_aggr);
#ifdef H5MF_ALLOC_DEBUG_MORE
HDfprintf(stderr, "%s: section {%a, %Hu}, adjoins small data aggregator\n", FUNC, sect->sect_info.addr, sect->sect_info.size);
#endif /* H5MF_ALLOC_DEBUG_MORE */

                /* Indicate shrinking can occur */
                HGOTO_DONE(TRUE)
            } /* end if */
        } /* end if */
    } /* end else */

    /* Set return value */
    ret_value = FALSE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5MF_sect_simple_can_shrink() */


/*-------------------------------------------------------------------------
 * Function:	H5MF_sect_simple_shrink
 *
 * Purpose:	Shrink container with section
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, January  8, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MF_sect_simple_shrink(H5FS_section_info_t **_sect, void *_udata)
{
    H5MF_free_section_t **sect = (H5MF_free_section_t **)_sect;   /* File free section */
    H5MF_sect_ud_t *udata = (H5MF_sect_ud_t *)_udata;   /* User data for callback */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5MF_sect_simple_shrink)

    /* Check arguments. */
    HDassert(sect);
    HDassert(udata);
    HDassert(udata->f);

    /* Check for shrinking file */
    if(H5MF_SHRINK_EOA == udata->shrink) {
        /* Sanity check */
        HDassert(H5F_INTENT(udata->f) & H5F_ACC_RDWR);

        /* Release section's space at EOA with file driver */
        if(H5FD_free(udata->f->shared->lf, udata->dxpl_id, udata->alloc_type, udata->f, (*sect)->sect_info.addr, (*sect)->sect_info.size) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, FAIL, "driver free request failed")
    } /* end if */
    else {
        /* Sanity check */
        HDassert(udata->aggr);

        /* Absorb the section into the aggregator or vice versa */
        if(H5MF_aggr_absorb(udata->f, udata->aggr, *sect, udata->allow_sect_absorb) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTMERGE, FAIL, "can't absorb section into aggregator or vice versa")
    } /* end else */

    /* Check for freeing section */
    if(udata->shrink != H5MF_SHRINK_SECT_ABSORB_AGGR) {
        /* Free section */
        if(H5MF_sect_simple_free((H5FS_section_info_t *)*sect) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't free simple section node")

        /* Mark section as freed, for free space manager */
        *sect = NULL;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5MF_sect_simple_shrink() */


/*-------------------------------------------------------------------------
 * Function:	H5MF_sect_simple_free
 *
 * Purpose:	Free a 'single' section node
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, January  8, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MF_sect_simple_free(H5FS_section_info_t *_sect)
{
    H5MF_free_section_t *sect = (H5MF_free_section_t *)_sect;   /* File free section */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5MF_sect_simple_free)

    /* Check arguments. */
    HDassert(sect);

    /* Release the section */
    sect = H5FL_FREE(H5MF_free_section_t, sect);

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5MF_sect_simple_free() */


/*-------------------------------------------------------------------------
 * Function:	H5MF_sect_simple_valid
 *
 * Purpose:	Check the validity of a section
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, January  8, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5MF_sect_simple_valid(const H5FS_section_class_t UNUSED *cls,
    const H5FS_section_info_t
#ifdef NDEBUG
    UNUSED
#endif /* NDEBUG */
    *_sect)
{
#ifndef NDEBUG
    const H5MF_free_section_t *sect = (const H5MF_free_section_t *)_sect;   /* File free section */
#endif /* NDEBUG */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5MF_sect_simple_valid)

    /* Check arguments. */
    HDassert(sect);

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5MF_sect_simple_valid() */


/*-------------------------------------------------------------------------
 * Function:	H5MF_sect_simple_split
 *
 * Purpose:	Split SECT into 2 sections: fragment for alignment & the aligned section
 *		SECT's addr and size are updated to point to the aligned section
 *
 * Return:	Success:	the fragment for aligning sect
 *		Failure:	null
 *
 * Programmer:	Vailin Choi, July 29, 2008
 *
 *-------------------------------------------------------------------------
 */
static H5FS_section_info_t *
H5MF_sect_simple_split(H5FS_section_info_t *sect, hsize_t frag_size)
{
    H5MF_free_section_t *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5MF_sect_simple_split)

    /* Allocate space for new section */
    if(NULL == (ret_value = H5MF_sect_simple_new(sect->addr, frag_size)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, NULL, "can't initialize free space section")

    /* Set new section's info */
    sect->addr += frag_size;
    sect->size -= frag_size;

done:
    FUNC_LEAVE_NOAPI((H5FS_section_info_t *)ret_value)
} /* end H5MF_sect_simple_split() */

