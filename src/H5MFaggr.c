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
 * Purpose:	Routines for aggregating free space allocations
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
 * Function:    H5MF_aggr_vfd_alloc
 *
 * Purpose:     Allocate SIZE bytes of file memory via H5MF_aggr_alloc()
 *		and return the relative address where that contiguous chunk
 *		of file memory exists.
 *		The TYPE argument describes the purpose for which the storage
 *		is being requested.
 *
 * Return:      Success:        The file address of new chunk.
 *              Failure:        HADDR_UNDEF
 *
 * Programmer:  Vailin Choi; July 1st, 2009
 *		(The coding is from H5MF_alloc().)
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5MF_aggr_vfd_alloc(H5F_t *f, H5FD_mem_t alloc_type, hid_t dxpl_id, hsize_t size)
{
    haddr_t	ret_value;              /* Return value */

    FUNC_ENTER_NOAPI(H5MF_aggr_vfd_alloc, HADDR_UNDEF)
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: alloc_type = %u, size = %Hu\n", FUNC, (unsigned)alloc_type, size);
#endif /* H5MF_ALLOC_DEBUG */

    /* check arguments */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->lf);
    HDassert(size > 0);

    /* Couldn't find anything from the free space manager, go allocate some */
    if(alloc_type != H5FD_MEM_DRAW) {
        /* Handle metadata differently from "raw" data */
        if(HADDR_UNDEF == (ret_value = H5MF_aggr_alloc(f, dxpl_id, &(f->shared->meta_aggr), &(f->shared->sdata_aggr), alloc_type, size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, HADDR_UNDEF, "can't allocate metadata")
    } /* end if */
    else {
        /* Allocate "raw" data */
        if(HADDR_UNDEF == (ret_value = H5MF_aggr_alloc(f, dxpl_id, &(f->shared->sdata_aggr), &(f->shared->meta_aggr), alloc_type, size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, HADDR_UNDEF, "can't allocate raw data")
    } /* end else */

    /* Sanity check for overlapping into file's temporary allocation space */
    HDassert(H5F_addr_le((ret_value + size), f->shared->tmp_addr));

done:
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Leaving: ret_value = %a, size = %Hu\n", FUNC, ret_value, size);
#endif /* H5MF_ALLOC_DEBUG */
#ifdef H5MF_ALLOC_DEBUG_DUMP
H5MF_sects_dump(f, dxpl_id, stderr);
#endif /* H5MF_ALLOC_DEBUG_DUMP */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_aggr_vfd_alloc() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_aggr_alloc
 *
 * Purpose:     Try to allocate SIZE bytes of memory from an aggregator
 *              block if possible.
 *
 * Return:      Success:    The format address of the new file memory.
 *              Failure:    The undefined address HADDR_UNDEF
 *
 * Programmer:  Quincey Koziol
 *              Thursday, December 13, 2007
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5MF_aggr_alloc(H5F_t *f, hid_t dxpl_id, H5F_blk_aggr_t *aggr,
    H5F_blk_aggr_t *other_aggr, H5FD_mem_t type, hsize_t size)
{
    haddr_t	eoa_frag_addr = HADDR_UNDEF;    /* Address of fragment at EOA */
    hsize_t	eoa_frag_size = 0;      /* Size of fragment at EOA */
    haddr_t	eoa = HADDR_UNDEF;      /* Initial EOA for the file */
    haddr_t 	ret_value;              /* Return value */

    FUNC_ENTER_NOAPI(H5MF_aggr_alloc, HADDR_UNDEF)
#ifdef H5MF_AGGR_DEBUG
HDfprintf(stderr, "%s: type = %u, size = %Hu\n", FUNC, (unsigned)type, size);
#endif /* H5MF_AGGR_DEBUG */

    /* check args */
    HDassert(f);
    HDassert(aggr);
    HDassert(aggr->feature_flag == H5FD_FEAT_AGGREGATE_METADATA || aggr->feature_flag == H5FD_FEAT_AGGREGATE_SMALLDATA);
    HDassert(other_aggr);
    HDassert(other_aggr->feature_flag == H5FD_FEAT_AGGREGATE_METADATA || other_aggr->feature_flag == H5FD_FEAT_AGGREGATE_SMALLDATA);
    HDassert(other_aggr->feature_flag != aggr->feature_flag);
    HDassert(type >= H5FD_MEM_DEFAULT && type < H5FD_MEM_NTYPES);
    HDassert(size > 0);

    /* Get the EOA for the file */
    if(HADDR_UNDEF == (eoa = H5F_get_eoa(f, type)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, HADDR_UNDEF, "Unable to get eoa")

    /*
     * If the aggregation feature is enabled for this file, allocate "generic"
     * space and sub-allocate out of that, if possible. Otherwise just allocate
     * through H5FD_alloc()
     */
    if(f->shared->feature_flags & aggr->feature_flag) {
        haddr_t	aggr_frag_addr = HADDR_UNDEF;   /* Address of aggregrator fragment */
        hsize_t	aggr_frag_size = 0;             /* Size of aggregator fragment */
        hsize_t alignment;                      /* Alignment of this section */
        hsize_t aggr_mis_align = 0;             /* Mis-alignment of aggregator */
        H5FD_mem_t alloc_type, other_alloc_type;/* Current aggregator & 'other' aggregator types */

#ifdef H5MF_AGGR_DEBUG
HDfprintf(stderr, "%s: aggr = {%a, %Hu, %Hu}\n", FUNC, aggr->addr, aggr->tot_size, aggr->size);
#endif /* H5MF_AGGR_DEBUG */

        /* Turn off alignment if allocation < threshold */
	alignment = f->shared->alignment;
	if(!((alignment > 1) && (size >= f->shared->threshold)))
	    alignment = 0; /* no alignment */

        /* Generate fragment if aggregator is mis-aligned */
	if(alignment && aggr->addr > 0 && aggr->size > 0 && (aggr_mis_align = (aggr->addr + H5FD_get_base_addr(f->shared->lf)) % alignment)) {
	    aggr_frag_addr = aggr->addr;
	    aggr_frag_size = alignment - aggr_mis_align;
	} /* end if */

	alloc_type = aggr->feature_flag == H5FD_FEAT_AGGREGATE_METADATA ? H5FD_MEM_DEFAULT : H5FD_MEM_DRAW;
	other_alloc_type = other_aggr->feature_flag == H5FD_FEAT_AGGREGATE_METADATA ? H5FD_MEM_DEFAULT : H5FD_MEM_DRAW;

        /* Check if the space requested is larger than the space left in the block */
        if((size + aggr_frag_size) > aggr->size) {
            htri_t  	extended = FALSE;   /* Whether the file was extended */

            /* Check if the block asked for is too large for 'normal' aggregator block */
            if(size >= aggr->alloc_size) {
		hsize_t ext_size = size + aggr_frag_size;

                /* Check for overlapping into file's temporary allocation space */
                if(H5F_addr_gt((aggr->addr + aggr->size + ext_size), f->shared->tmp_addr))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_BADRANGE, HADDR_UNDEF, "'normal' file space allocation request will overlap into 'temporary' file space")

		if ((aggr->addr > 0) && (extended = H5FD_try_extend(f->shared->lf, alloc_type, f, aggr->addr + aggr->size, ext_size)) < 0)
		    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, HADDR_UNDEF, "can't extending space")
		else if (extended) {
		    /* aggr->size is unchanged */
		    ret_value = aggr->addr + aggr_frag_size;
		    aggr->addr += ext_size;
		    aggr->tot_size += ext_size;
		} else {
                    /* Check for overlapping into file's temporary allocation space */
                    if(H5F_addr_gt((eoa + size), f->shared->tmp_addr))
                        HGOTO_ERROR(H5E_RESOURCE, H5E_BADRANGE, HADDR_UNDEF, "'normal' file space allocation request will overlap into 'temporary' file space")

		    if ((other_aggr->size > 0) && (H5F_addr_eq((other_aggr->addr + other_aggr->size), eoa)) &&
			((other_aggr->tot_size - other_aggr->size) >= other_aggr->alloc_size)) {

                            if(H5FD_free(f->shared->lf, dxpl_id, other_alloc_type, f, other_aggr->addr, other_aggr->size) < 0)
                                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, HADDR_UNDEF, "can't free aggregation block")

                            other_aggr->addr = 0;
                            other_aggr->tot_size = 0;
                            other_aggr->size = 0;
		    } /* end if */

                    /* Allocate space from the VFD (i.e. at the end of the file) */
		    if(HADDR_UNDEF == (ret_value = H5FD_alloc(f->shared->lf, dxpl_id, type, f, size, &eoa_frag_addr, &eoa_frag_size)))
			HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, HADDR_UNDEF, "can't allocate aggregation block")
                } /* end else */
            } /* end if */
	    else {
		hsize_t ext_size = aggr->alloc_size;

                /* Allocate another block */
#ifdef H5MF_AGGR_DEBUG
HDfprintf(stderr, "%s: Allocating block\n", FUNC);
#endif /* H5MF_AGGR_DEBUG */

		if(aggr_frag_size > (ext_size - size))
		    ext_size += (aggr_frag_size - (ext_size - size));

                /* Check for overlapping into file's temporary allocation space */
                if(H5F_addr_gt((aggr->addr + aggr->size + ext_size), f->shared->tmp_addr))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_BADRANGE, HADDR_UNDEF, "'normal' file space allocation request will overlap into 'temporary' file space")

		if((aggr->addr > 0) && (extended = H5FD_try_extend(f->shared->lf, alloc_type, f, aggr->addr + aggr->size, ext_size)) < 0)
		    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, HADDR_UNDEF, "can't extending space")
		else if (extended) {
		    aggr->addr += aggr_frag_size;
		    aggr->size += (ext_size - aggr_frag_size);
		    aggr->tot_size += ext_size;
		} else {
                    haddr_t new_space;          /* Address of new space allocated */

                    /* Check for overlapping into file's temporary allocation space */
                    if(H5F_addr_gt((eoa + aggr->alloc_size), f->shared->tmp_addr))
                        HGOTO_ERROR(H5E_RESOURCE, H5E_BADRANGE, HADDR_UNDEF, "'normal' file space allocation request will overlap into 'temporary' file space")

		    if((other_aggr->size > 0) && (H5F_addr_eq((other_aggr->addr + other_aggr->size), eoa)) &&
			((other_aggr->tot_size - other_aggr->size) >= other_aggr->alloc_size)) {

                            if(H5FD_free(f->shared->lf, dxpl_id, other_alloc_type, f, other_aggr->addr, other_aggr->size) < 0)
                                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, HADDR_UNDEF, "can't free aggregation block")
                            other_aggr->addr = 0;
                            other_aggr->tot_size = 0;
                            other_aggr->size = 0;
		    } /* end if */

                    /* Allocate space from the VFD (i.e. at the end of the file) */
		    if(HADDR_UNDEF == (new_space = H5FD_alloc(f->shared->lf, dxpl_id, alloc_type, f, aggr->alloc_size, &eoa_frag_addr, &eoa_frag_size)))
			HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, HADDR_UNDEF, "can't allocate aggregation block")

                    /* Return the unused portion of the block to a free list */
                    if(aggr->size > 0)
                        if(H5MF_xfree(f, alloc_type, dxpl_id, aggr->addr, aggr->size) < 0)
                            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, HADDR_UNDEF, "can't free aggregation block")

                    /* Point the aggregator at the newly allocated block */
                    aggr->addr = new_space;
                    aggr->size = aggr->alloc_size;
                    aggr->tot_size = aggr->alloc_size;
                } /* end else */

		/* Allocate space out of the metadata block */
		ret_value = aggr->addr;
		aggr->size -= size;
		aggr->addr += size;
            } /* end else */

	    /* Freeing any possible fragment due to file allocation */
	    if(eoa_frag_size)
		if(H5MF_xfree(f, type, dxpl_id, eoa_frag_addr, eoa_frag_size) < 0)
		    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, HADDR_UNDEF, "can't free eoa fragment")

	    /* Freeing any possible fragment due to alignment in the block after extension */
	    if(extended && aggr_frag_size)
		if(H5MF_xfree(f, type, dxpl_id, aggr_frag_addr, aggr_frag_size) < 0)
		    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, HADDR_UNDEF, "can't free aggregation fragment")
        } /* end if */
        else {
            /* Allocate space out of the block */
	    ret_value = aggr->addr + aggr_frag_size;
	    aggr->size -= (size + aggr_frag_size);
	    aggr->addr += (size + aggr_frag_size);

	    /* free any possible fragment */
	    if(aggr_frag_size)
		if(H5MF_xfree(f, type, dxpl_id, aggr_frag_addr, aggr_frag_size) < 0)
		    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, HADDR_UNDEF, "can't free aggregation fragment")
        } /* end else */
    } /* end if */
    else {
        /* Check for overlapping into file's temporary allocation space */
        if(H5F_addr_gt((eoa + size), f->shared->tmp_addr))
            HGOTO_ERROR(H5E_RESOURCE, H5E_BADRANGE, HADDR_UNDEF, "'normal' file space allocation request will overlap into 'temporary' file space")

        /* Allocate data from the file */
        if(HADDR_UNDEF == (ret_value = H5FD_alloc(f->shared->lf, dxpl_id, type, f, size, &eoa_frag_addr, &eoa_frag_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, HADDR_UNDEF, "can't allocate file space")

        /* Check if fragment was generated */
	if(eoa_frag_size)
            /* Put fragment on the free list */
	    if(H5MF_xfree(f, type, dxpl_id, eoa_frag_addr, eoa_frag_size) < 0)
		HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, HADDR_UNDEF, "can't free eoa fragment")
    } /* end else */

    /* Sanity check for overlapping into file's temporary allocation space */
    HDassert(H5F_addr_le((ret_value + size), f->shared->tmp_addr));

    /* Post-condition sanity check */
    if(f->shared->alignment && size >= f->shared->threshold)
	HDassert(!((ret_value + H5FD_get_base_addr(f->shared->lf)) % f->shared->alignment));

done:
#ifdef H5MF_AGGR_DEBUG
HDfprintf(stderr, "%s: ret_value = %a\n", FUNC, ret_value);
#endif /* H5MF_AGGR_DEBUG */
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_aggr_alloc() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_aggr_try_extend
 *
 * Purpose:	Check if a block is inside an aggregator block and extend it
 *              if possible.
 *
 * Return:	Success:	TRUE(1)  - Block was extended
 *                              FALSE(0) - Block could not be extended
 * 		Failure:	FAIL
 *
 * Programmer:  Quincey Koziol
 *              Thursday, December 13, 2007
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5MF_aggr_try_extend(H5F_t *f, H5F_blk_aggr_t *aggr, H5FD_mem_t type,
    haddr_t blk_end, hsize_t extra_requested)
{
    htri_t ret_value = FALSE;           /* Return value */

    FUNC_ENTER_NOAPI(H5MF_aggr_try_extend, FAIL)

    /* Check args */
    HDassert(f);
    HDassert(aggr);
    HDassert(aggr->feature_flag == H5FD_FEAT_AGGREGATE_METADATA || aggr->feature_flag == H5FD_FEAT_AGGREGATE_SMALLDATA);

    /* Check if this aggregator is active */
    if(f->shared->feature_flags & aggr->feature_flag) {
        /* If the block being tested adjoins the beginning of the aggregator
         *      block, check if the aggregator can accomodate the extension.
         */
        if(H5F_addr_eq(blk_end, aggr->addr)) {
            /* If the aggregator block is at the end of the file, extend the
             * file and "bubble" the aggregator up
             */
            if((ret_value = H5FD_try_extend(f->shared->lf, type, f, (aggr->addr + aggr->size), extra_requested)) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTEXTEND, FAIL, "error extending file")
            else if(ret_value == TRUE) {
                /* Shift the aggregator block by the extra requested */
                aggr->addr += extra_requested;

                /* Add extra requested to the aggregator block's total amount allocated */
                aggr->tot_size += extra_requested;
            } /* end if */
            else {
                /* Check if the aggregator block has enough internal space to satisfy
                 *      extending the block.
                 */
                if(aggr->size >= extra_requested) {
                    /* Extend block into aggregator */
                    aggr->size -= extra_requested;
                    aggr->addr += extra_requested;

                    /* Indicate success */
                    HGOTO_DONE(TRUE);
                 } /* end if */
            } /* end else */
        } /* end if */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_aggr_try_extend() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_aggr_can_absorb
 *
 * Purpose:	Check if a section adjoins an aggregator block and one can
 *              absorb the other.
 *
 * Return:	Success:	TRUE(1)  - Section or aggregator can be absorbed
 *                              FALSE(0) - Section and aggregator can not be absorbed
 * 		Failure:	FAIL
 *
 * Programmer:  Quincey Koziol
 *              Friday, February  1, 2008
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5MF_aggr_can_absorb(const H5F_t *f, const H5F_blk_aggr_t *aggr,
    const H5MF_free_section_t *sect, H5MF_shrink_type_t *shrink)
{
    htri_t ret_value = FALSE;           /* Return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5MF_aggr_can_absorb)

    /* Check args */
    HDassert(f);
    HDassert(aggr);
    HDassert(aggr->feature_flag == H5FD_FEAT_AGGREGATE_METADATA || aggr->feature_flag == H5FD_FEAT_AGGREGATE_SMALLDATA);
    HDassert(sect);
    HDassert(shrink);

    /* Check if this aggregator is active */
    if(f->shared->feature_flags & aggr->feature_flag) {
        /* Check if the block adjoins the beginning or end of the aggregator */
        if(H5F_addr_eq((sect->sect_info.addr + sect->sect_info.size), aggr->addr)
                || H5F_addr_eq((aggr->addr + aggr->size), sect->sect_info.addr)) {
#ifdef H5MF_AGGR_DEBUG
HDfprintf(stderr, "%s: section {%a, %Hu} adjoins aggr = {%a, %Hu}\n", "H5MF_aggr_can_absorb", sect->sect_info.addr, sect->sect_info.size, aggr->addr, aggr->size);
#endif /* H5MF_AGGR_DBEUG */
            /* Check if aggregator would get too large and should be absorbed into section */
            if((aggr->size + sect->sect_info.size) >= aggr->alloc_size)
                *shrink = H5MF_SHRINK_SECT_ABSORB_AGGR;
            else
                *shrink = H5MF_SHRINK_AGGR_ABSORB_SECT;

            /* Indicate success */
            HGOTO_DONE(TRUE)
        } /* end if */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_aggr_can_absorb() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_aggr_absorb
 *
 * Purpose:	Absorb a free space section into an aggregator block or
 *              vice versa.
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Friday, February  1, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MF_aggr_absorb(const H5F_t UNUSED *f, H5F_blk_aggr_t *aggr, H5MF_free_section_t *sect,
    hbool_t allow_sect_absorb)
{
    FUNC_ENTER_NOAPI_NOFUNC(H5MF_aggr_absorb)

    /* Check args */
    HDassert(f);
    HDassert(aggr);
    HDassert(aggr->feature_flag == H5FD_FEAT_AGGREGATE_METADATA || aggr->feature_flag == H5FD_FEAT_AGGREGATE_SMALLDATA);
    HDassert(f->shared->feature_flags & aggr->feature_flag);
    HDassert(sect);

    /* Check if aggregator would get too large and should be absorbed into section */
    if((aggr->size + sect->sect_info.size) >= aggr->alloc_size && allow_sect_absorb) {
        /* Check if the section adjoins the beginning or end of the aggregator */
        if(H5F_addr_eq((sect->sect_info.addr + sect->sect_info.size), aggr->addr)) {
#ifdef H5MF_AGGR_DBEUG
HDfprintf(stderr, "%s: aggr {%a, %Hu} adjoins front of section = {%a, %Hu}\n", "H5MF_aggr_absorb", aggr->addr, aggr->size, sect->sect_info.addr, sect->sect_info.size);
#endif /* H5MF_AGGR_DBEUG */
            /* Absorb aggregator onto end of section */
            sect->sect_info.size += aggr->size;
        } /* end if */
        else {
            /* Sanity check */
            HDassert(H5F_addr_eq((aggr->addr + aggr->size), sect->sect_info.addr));

#ifdef H5MF_AGGR_DBEUG
HDfprintf(stderr, "%s: aggr {%a, %Hu} adjoins end of section = {%a, %Hu}\n", "H5MF_aggr_absorb", aggr->addr, aggr->size, sect->sect_info.addr, sect->sect_info.size);
#endif /* H5MF_AGGR_DBEUG */
            /* Absorb aggregator onto beginning of section */
            sect->sect_info.addr -= aggr->size;
            sect->sect_info.size += aggr->size;
        } /* end if */

        /* Reset aggregator */
        aggr->tot_size = 0;
        aggr->addr = 0;
        aggr->size = 0;
    } /* end if */
    else {
        /* Check if the section adjoins the beginning or end of the aggregator */
        if(H5F_addr_eq((sect->sect_info.addr + sect->sect_info.size), aggr->addr)) {
#ifdef H5MF_AGGR_DBEUG
HDfprintf(stderr, "%s: section {%a, %Hu} adjoins front of aggr = {%a, %Hu}\n", "H5MF_aggr_absorb", sect->sect_info.addr, sect->sect_info.size, aggr->addr, aggr->size);
#endif /* H5MF_AGGR_DBEUG */
            /* Absorb section onto front of aggregator */
            aggr->addr -= sect->sect_info.size;
            aggr->size += sect->sect_info.size;

            /* Sections absorbed onto front of aggregator count against the total
             * amount of space aggregated together.
             */
            aggr->tot_size -= MIN(aggr->tot_size, sect->sect_info.size);
        } /* end if */
        else {
            /* Sanity check */
            HDassert(H5F_addr_eq((aggr->addr + aggr->size), sect->sect_info.addr));

#ifdef H5MF_AGGR_DBEUG
HDfprintf(stderr, "%s: section {%a, %Hu} adjoins end of aggr = {%a, %Hu}\n", "H5MF_aggr_absorb", sect->sect_info.addr, sect->sect_info.size, aggr->addr, aggr->size);
#endif /* H5MF_AGGR_DBEUG */
            /* Absorb section onto end of aggregator */
            aggr->size += sect->sect_info.size;
        } /* end if */
        /* Sanity check */
        HDassert(!allow_sect_absorb || (aggr->size < aggr->alloc_size));
    } /* end else */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5MF_aggr_absorb() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_aggr_query
 *
 * Purpose:     Query a block aggregator's current address & size info
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Thursday, December 13, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MF_aggr_query(const H5F_t *f, const H5F_blk_aggr_t *aggr, haddr_t *addr,
    hsize_t *size)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5MF_aggr_query)

    /* Check args */
    HDassert(f);
    HDassert(aggr);
    HDassert(aggr->feature_flag == H5FD_FEAT_AGGREGATE_METADATA || aggr->feature_flag == H5FD_FEAT_AGGREGATE_SMALLDATA);

    /* Check if this aggregator is active */
    if(f->shared->feature_flags & aggr->feature_flag) {
        *addr = aggr->addr;
        *size = aggr->size;
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5MF_aggr_query() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_aggr_reset
 *
 * Purpose:     Reset a block aggregator, returning any space back to file
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Thursday, December 13, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5MF_aggr_reset(H5F_t *f, hid_t dxpl_id, H5F_blk_aggr_t *aggr)
{
    H5FD_mem_t alloc_type;      /* Type of file memory to work with */
    herr_t ret_value = SUCCEED;	/* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5MF_aggr_reset)

    /* Check args */
    HDassert(f);
    HDassert(aggr);
    HDassert(aggr->feature_flag == H5FD_FEAT_AGGREGATE_METADATA || aggr->feature_flag == H5FD_FEAT_AGGREGATE_SMALLDATA);

    /* Set the type of memory in the file */
    alloc_type = (aggr->feature_flag == H5FD_FEAT_AGGREGATE_METADATA ? H5FD_MEM_DEFAULT : H5FD_MEM_DRAW);      /* Type of file memory to work with */

    /* Check if this aggregator is active */
    if(f->shared->feature_flags & aggr->feature_flag) {
        haddr_t tmp_addr;       /* Temporary holder for aggregator address */
        hsize_t tmp_size;       /* Temporary holder for aggregator size */

        /* Retain aggregator info */
        tmp_addr = aggr->addr;
        tmp_size = aggr->size;
#ifdef H5MF_AGGR_DBEUG
HDfprintf(stderr, "%s: tmp_addr = %a, tmp_size = %Hu\n", FUNC, tmp_addr, tmp_size);
#endif /* H5MF_AGGR_DBEUG */

        /* Reset aggregator block information */
        aggr->tot_size = 0;
        aggr->addr = 0;
        aggr->size = 0;

        /* Return the unused portion of the metadata block to the file */
        if(tmp_size > 0 && (H5F_INTENT(f) & H5F_ACC_RDWR))
            if(H5MF_xfree(f, alloc_type, dxpl_id, tmp_addr, tmp_size) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, FAIL, "can't release aggregator's free space")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_aggr_reset() */


/*-------------------------------------------------------------------------
 * Function:    H5MF_free_aggrs
 *
 * Purpose:     Reset a metadata & small block aggregators, returning any space
 *		back to file
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Vailin Choi
 *	        July 1st, 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MF_free_aggrs(H5F_t *f, hid_t dxpl_id)
{
    H5F_blk_aggr_t *first_aggr;         /* First aggregator to reset */
    H5F_blk_aggr_t *second_aggr;        /* Second aggregator to reset */
    haddr_t ma_addr = HADDR_UNDEF;      /* Base "metadata aggregator" address */
    hsize_t ma_size = 0;                /* Size of "metadata aggregator" */
    haddr_t sda_addr = HADDR_UNDEF;     /* Base "small data aggregator" address */
    hsize_t sda_size = 0;               /* Size of "small data aggregator" */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5MF_free_aggrs, FAIL)

    /* Check args */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->lf);

    /* Retrieve metadata aggregator info, if available */
    if(H5MF_aggr_query(f, &(f->shared->meta_aggr), &ma_addr, &ma_size) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "can't query metadata aggregator stats")

    /* Retrieve 'small data' aggregator info, if available */
    if(H5MF_aggr_query(f, &(f->shared->sdata_aggr), &sda_addr, &sda_size) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "can't query small data aggregator stats")

    /* Make certain we release the aggregator that's later in the file first */
    /* (so the file shrinks properly) */
    if(H5F_addr_defined(ma_addr) && H5F_addr_defined(sda_addr)) {
        if(H5F_addr_lt(ma_addr, sda_addr)) {
            first_aggr = &(f->shared->sdata_aggr);
            second_aggr = &(f->shared->meta_aggr);
        } /* end if */
        else {
            first_aggr = &(f->shared->meta_aggr);
            second_aggr = &(f->shared->sdata_aggr);
        } /* end else */
    } /* end if */
    else {
        first_aggr = &(f->shared->meta_aggr);
        second_aggr = &(f->shared->sdata_aggr);
    } /* end else */

     /* Release the unused portion of the metadata and "small data" blocks back
      * to the free lists in the file.
      */
    if(H5MF_aggr_reset(f, dxpl_id, first_aggr) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTFREE, FAIL, "can't reset metadata block")
    if(H5MF_aggr_reset(f, dxpl_id, second_aggr) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTFREE, FAIL, "can't reset 'small data' block")
done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_free_aggrs() */

