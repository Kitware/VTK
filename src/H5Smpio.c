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
 * Programmer:  rky 980813
 *
 * Purpose:	Functions to read/write directly between app buffer and file.
 *
 * 		Beware of the ifdef'ed print statements.
 *		I didn't make them portable.
 */

#define H5S_PACKAGE		/*suppress error about including H5Spkg	  */


#include "H5private.h"		/* Generic Functions			*/
#include "H5Dprivate.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"		/* File access				*/
#include "H5FDprivate.h"	/* File drivers				*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"        /* Memory management                    */
#include "H5Oprivate.h"		/* Object headers		  	*/
#include "H5Pprivate.h"         /* Property lists                       */
#include "H5Spkg.h"		/* Dataspaces 				*/

#ifdef H5_HAVE_PARALLEL

static herr_t H5S_mpio_all_type(const H5S_t *space, size_t elmt_size,
    MPI_Datatype *new_type, int *count, hbool_t *is_derived_type);
static herr_t H5S_mpio_none_type(MPI_Datatype *new_type, int *count,
    hbool_t *is_derived_type);
static herr_t H5S_mpio_hyper_type(const H5S_t *space, size_t elmt_size,
    MPI_Datatype *new_type, int *count, hbool_t *is_derived_type);
static herr_t H5S_mpio_span_hyper_type(const H5S_t *space, size_t elmt_size,
    MPI_Datatype *new_type, int *count, hbool_t *is_derived_type);
static herr_t H5S_obtain_datatype(const hsize_t down[], H5S_hyper_span_t* span,
    const MPI_Datatype *elmt_type, MPI_Datatype *span_type, size_t elmt_size);

#define H5S_MPIO_INITIAL_ALLOC_COUNT    256


/*-------------------------------------------------------------------------
 * Function:	H5S_mpio_all_type
 *
 * Purpose:	Translate an HDF5 "all" selection into an MPI type.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Outputs:	*new_type	  the MPI type corresponding to the selection
 *		*count		  how many objects of the new_type in selection
 *				  (useful if this is the buffer type for xfer)
 *		*is_derived_type  0 if MPI primitive type, 1 if derived
 *
 * Programmer:	rky 980813
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_mpio_all_type(const H5S_t *space, size_t elmt_size,
    MPI_Datatype *new_type, int *count, hbool_t *is_derived_type)
{
    hsize_t	total_bytes;
    hssize_t	snelmts;                /* Total number of elmts	(signed) */
    hsize_t	nelmts;                 /* Total number of elmts	*/
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5S_mpio_all_type)

    /* Check args */
    HDassert(space);

    /* Just treat the entire extent as a block of bytes */
    if((snelmts = H5S_GET_EXTENT_NPOINTS(space)) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "src dataspace has invalid selection")
    H5_ASSIGN_OVERFLOW(nelmts, snelmts, hssize_t, hsize_t);

    total_bytes = (hsize_t)elmt_size * nelmts;

    /* fill in the return values */
    *new_type = MPI_BYTE;
    H5_ASSIGN_OVERFLOW(*count, total_bytes, hsize_t, int);
    *is_derived_type = FALSE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5S_mpio_all_type() */


/*-------------------------------------------------------------------------
 * Function:	H5S_mpio_none_type
 *
 * Purpose:	Translate an HDF5 "none" selection into an MPI type.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Outputs:	*new_type	  the MPI type corresponding to the selection
 *		*count		  how many objects of the new_type in selection
 *				  (useful if this is the buffer type for xfer)
 *		*is_derived_type  0 if MPI primitive type, 1 if derived
 *
 * Programmer:	Quincey Koziol, October 29, 2002
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_mpio_none_type(MPI_Datatype *new_type, int *count, hbool_t *is_derived_type)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5S_mpio_none_type)

    /* fill in the return values */
    *new_type = MPI_BYTE;
    *count = 0;
    *is_derived_type = FALSE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5S_mpio_none_type() */


/*-------------------------------------------------------------------------
 * Function:	H5S_mpio_hyper_type
 *
 * Purpose:	Translate an HDF5 hyperslab selection into an MPI type.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Outputs:	*new_type	  the MPI type corresponding to the selection
 *		*count		  how many objects of the new_type in selection
 *				  (useful if this is the buffer type for xfer)
 *		*is_derived_type  0 if MPI primitive type, 1 if derived
 *
 * Programmer:	rky 980813
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_mpio_hyper_type(const H5S_t *space, size_t elmt_size,
    MPI_Datatype *new_type, int *count, hbool_t *is_derived_type)
{
    H5S_sel_iter_t sel_iter;    /* Selection iteration info */
    hbool_t sel_iter_init = FALSE;    /* Selection iteration info has been initialized */

    struct dim {	/* less hassle than malloc/free & ilk */
        hssize_t start;
        hsize_t strid;
        hsize_t block;
        hsize_t xtent;
        hsize_t count;
    } d[H5S_MAX_RANK];

    int			i;
    int			offset[H5S_MAX_RANK];
    int			max_xtent[H5S_MAX_RANK];
    H5S_hyper_dim_t	*diminfo;		/* [rank] */
    int		rank;
    int			block_length[3];
    MPI_Datatype	inner_type, outer_type, old_types[3];
    MPI_Aint            extent_len, displacement[3];
    int                 mpi_code;               /* MPI return code */
    herr_t		ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5S_mpio_hyper_type)

    /* Check args */
    HDassert(space);
    HDassert(sizeof(MPI_Aint) >= sizeof(elmt_size));

    /* Initialize selection iterator */
    if(H5S_select_iter_init(&sel_iter, space, elmt_size) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator")
    sel_iter_init = TRUE;	/* Selection iteration info has been initialized */

    /* Abbreviate args */
    diminfo = sel_iter.u.hyp.diminfo;
    HDassert(diminfo);

    /* make a local copy of the dimension info so we can operate with them */

    /* Check if this is a "flattened" regular hyperslab selection */
    if(sel_iter.u.hyp.iter_rank != 0 && sel_iter.u.hyp.iter_rank < space->extent.rank) {
        /* Flattened selection */
        rank = sel_iter.u.hyp.iter_rank;
        HDassert(rank >= 0 && rank <= H5S_MAX_RANK);	/* within array bounds */
#ifdef H5S_DEBUG
  if(H5DEBUG(S))
            HDfprintf(H5DEBUG(S), "%s: Flattened selection\n",FUNC);
#endif
        for(i = 0; i < rank; ++i) {
            d[i].start = diminfo[i].start + sel_iter.u.hyp.sel_off[i];
            d[i].strid = diminfo[i].stride;
            d[i].block = diminfo[i].block;
            d[i].count = diminfo[i].count;
            d[i].xtent = sel_iter.u.hyp.size[i];
#ifdef H5S_DEBUG
       if(H5DEBUG(S)){
            HDfprintf(H5DEBUG(S), "%s: start=%Hd  stride=%Hu  count=%Hu  block=%Hu  xtent=%Hu",
                FUNC, d[i].start, d[i].strid, d[i].count, d[i].block, d[i].xtent );
            if (i==0)
                HDfprintf(H5DEBUG(S), "  rank=%d\n", rank );
            else
                HDfprintf(H5DEBUG(S), "\n" );
      }
#endif
            if(0 == d[i].block)
                goto empty;
            if(0 == d[i].count)
                goto empty;
            if(0 == d[i].xtent)
                goto empty;
        } /* end for */
    } /* end if */
    else {
        /* Non-flattened selection */
        rank = space->extent.rank;
        HDassert(rank >= 0 && rank <= H5S_MAX_RANK);	/* within array bounds */
        if(0 == rank)
            goto empty;
#ifdef H5S_DEBUG
  if(H5DEBUG(S))
            HDfprintf(H5DEBUG(S),"%s: Non-flattened selection\n",FUNC);
#endif
        for(i = 0; i < rank; ++i) {
            d[i].start = diminfo[i].start + space->select.offset[i];
            d[i].strid = diminfo[i].stride;
            d[i].block = diminfo[i].block;
            d[i].count = diminfo[i].count;
            d[i].xtent = space->extent.size[i];
#ifdef H5S_DEBUG
  if(H5DEBUG(S)){
    HDfprintf(H5DEBUG(S), "%s: start=%Hd  stride=%Hu  count=%Hu  block=%Hu  xtent=%Hu",
              FUNC, d[i].start, d[i].strid, d[i].count, d[i].block, d[i].xtent );
    if (i==0)
        HDfprintf(H5DEBUG(S), "  rank=%d\n", rank );
    else
        HDfprintf(H5DEBUG(S), "\n" );
  }
#endif
            if(0 == d[i].block)
                goto empty;
            if(0 == d[i].count)
                goto empty;
            if(0 == d[i].xtent)
                goto empty;
        } /* end for */
    } /* end else */

/**********************************************************************
    Compute array "offset[rank]" which gives the offsets for a multi-
    dimensional array with dimensions "d[i].xtent" (i=0,1,...,rank-1).
**********************************************************************/
    offset[rank - 1] = 1;
    max_xtent[rank - 1] = d[rank - 1].xtent;
#ifdef H5S_DEBUG
  if(H5DEBUG(S)) {
     i=rank-1;
     HDfprintf(H5DEBUG(S), " offset[%2d]=%d; max_xtent[%2d]=%d\n",
                          i, offset[i], i, max_xtent[i]);
  }
#endif
    for(i = rank - 2; i >= 0; --i) {
        offset[i] = offset[i + 1] * d[i + 1].xtent;
        max_xtent[i] = max_xtent[i + 1] * d[i].xtent;
#ifdef H5S_DEBUG
  if(H5DEBUG(S))
    HDfprintf(H5DEBUG(S), " offset[%2d]=%d; max_xtent[%2d]=%d\n",
                          i, offset[i], i, max_xtent[i]);
#endif
    } /* end for */

    /*  Create a type covering the selected hyperslab.
     *  Multidimensional dataspaces are stored in row-major order.
     *  The type is built from the inside out, going from the
     *  fastest-changing (i.e., inner) dimension * to the slowest (outer). */

/*******************************************************
*  Construct contig type for inner contig dims:
*******************************************************/
#ifdef H5S_DEBUG
  if(H5DEBUG(S)) {
    HDfprintf(H5DEBUG(S), "%s: Making contig type %Zu MPI_BYTEs\n", FUNC, elmt_size);
    for (i=rank-1; i>=0; --i)
        HDfprintf(H5DEBUG(S), "d[%d].xtent=%Hu \n", i, d[i].xtent);
  }
#endif
    if(MPI_SUCCESS != (mpi_code = MPI_Type_contiguous((int)elmt_size, MPI_BYTE, &inner_type)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_contiguous failed", mpi_code)

/*******************************************************
*  Construct the type by walking the hyperslab dims
*  from the inside out:
*******************************************************/
    for(i = rank - 1; i >= 0; --i) {
#ifdef H5S_DEBUG
  if(H5DEBUG(S))
    HDfprintf(H5DEBUG(S), "%s: Dimension i=%d \n"
            "start=%Hd count=%Hu block=%Hu stride=%Hu, xtent=%Hu max_xtent=%d\n",
            FUNC, i, d[i].start, d[i].count, d[i].block, d[i].strid, d[i].xtent, max_xtent[i]);
#endif

#ifdef H5S_DEBUG
  if(H5DEBUG(S))
    HDfprintf(H5DEBUG(S), "%s: i=%d  Making vector-type \n", FUNC,i);
#endif
       /****************************************
       *  Build vector type of the selection.
       ****************************************/
	mpi_code = MPI_Type_vector((int)(d[i].count),       /* count */
				   (int)(d[i].block),       /* blocklength */
				   (int)(d[i].strid),       /* stride */
				   inner_type,	            /* old type */
				   &outer_type);            /* new type */

        MPI_Type_free(&inner_type);
        if(mpi_code != MPI_SUCCESS)
            HMPI_GOTO_ERROR(FAIL, "couldn't create MPI vector type", mpi_code)

        /****************************************
        *  Then build the dimension type as (start, vector type, xtent).
        ****************************************/
        /* calculate start and extent values of this dimension */
	displacement[1] = d[i].start * offset[i] * elmt_size;
        displacement[2] = (MPI_Aint)elmt_size * max_xtent[i];
        if(MPI_SUCCESS != (mpi_code = MPI_Type_extent(outer_type, &extent_len)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Type_extent failed", mpi_code)

        /*************************************************
        *  Restructure this datatype ("outer_type")
        *  so that it still starts at 0, but its extent
        *  is the full extent in this dimension.
        *************************************************/
        if(displacement[1] > 0 || (int)extent_len < displacement[2]) {

            block_length[0] = 1;
            block_length[1] = 1;
            block_length[2] = 1;

            displacement[0] = 0;

            old_types[0] = MPI_LB;
            old_types[1] = outer_type;
            old_types[2] = MPI_UB;
#ifdef H5S_DEBUG
  if(H5DEBUG(S))
    HDfprintf(H5DEBUG(S), "%s: i=%d Extending struct type\n"
        "***displacements: %ld, %ld, %ld\n",
        FUNC, i, (long)displacement[0], (long)displacement[1], (long)displacement[2]);
#endif

            mpi_code = MPI_Type_struct(3,               /* count */
                                       block_length,    /* blocklengths */
                                       displacement,    /* displacements */
                                       old_types,       /* old types */
                                       &inner_type);    /* new type */

            MPI_Type_free(&outer_type);
    	    if(mpi_code != MPI_SUCCESS)
                HMPI_GOTO_ERROR(FAIL, "couldn't resize MPI vector type", mpi_code)
        } /* end if */
        else
            inner_type = outer_type;
    } /* end for */
/***************************
*  End of loop, walking
*  thru dimensions.
***************************/

    /* At this point inner_type is actually the outermost type, even for 0-trip loop */
    *new_type = inner_type;
    if(MPI_SUCCESS != (mpi_code = MPI_Type_commit(new_type)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code)

    /* fill in the remaining return values */
    *count = 1;			/* only have to move one of these suckers! */
    *is_derived_type = TRUE;
    HGOTO_DONE(SUCCEED);

empty:
    /* special case: empty hyperslab */
    *new_type = MPI_BYTE;
    *count = 0;
    *is_derived_type = FALSE;

done:
    /* Release selection iterator */
    if(sel_iter_init)
        if(H5S_SELECT_ITER_RELEASE(&sel_iter) < 0)
            HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator")

#ifdef H5S_DEBUG
  if(H5DEBUG(S))
    HDfprintf(H5DEBUG(S), "Leave %s, count=%ld  is_derived_type=%t\n",
		FUNC, *count, *is_derived_type );
#endif
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5S_mpio_hyper_type() */


/*-------------------------------------------------------------------------
 * Function:	H5S_mpio_span_hyper_type
 *
 * Purpose:	Translate an HDF5 irregular hyperslab selection into an
                MPI type.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Outputs:	*new_type	  the MPI type corresponding to the selection
 *		*count		  how many objects of the new_type in selection
 *				  (useful if this is the buffer type for xfer)
 *		*is_derived_type  0 if MPI primitive type, 1 if derived
 *
 * Programmer:  kyang
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_mpio_span_hyper_type(const H5S_t *space, size_t elmt_size,
    MPI_Datatype *new_type, int *count, hbool_t *is_derived_type)
{
    MPI_Datatype  elmt_type;            /* MPI datatype for an element */
    hbool_t elmt_type_is_derived = FALSE;       /* Whether the element type has been created */
    MPI_Datatype  span_type;            /* MPI datatype for overall span tree */
    hsize_t       down[H5S_MAX_RANK];   /* 'down' sizes for each dimension */
    int           mpi_code;             /* MPI return code */
    herr_t        ret_value = SUCCEED;  /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5S_mpio_span_hyper_type)

    /* Check args */
    HDassert(space);
    HDassert(space->extent.size);
    HDassert(space->select.sel_info.hslab->span_lst);
    HDassert(space->select.sel_info.hslab->span_lst->head);

    /* Create the base type for an element */
    if(MPI_SUCCESS != (mpi_code = MPI_Type_contiguous((int)elmt_size, MPI_BYTE, &elmt_type)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_contiguous failed", mpi_code)
    elmt_type_is_derived = TRUE;

    /* Compute 'down' sizes for each dimension */
    if(H5V_array_down(space->extent.rank, space->extent.size, down) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGETSIZE, FAIL, "couldn't compute 'down' dimension sizes")

    /* Obtain derived data type */
    if(H5S_obtain_datatype(down, space->select.sel_info.hslab->span_lst->head, &elmt_type, &span_type, elmt_size) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL, "couldn't obtain  MPI derived data type")
    if(MPI_SUCCESS != (mpi_code = MPI_Type_commit(&span_type)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code)
    *new_type = span_type;

    /* fill in the remaining return values */
    *count = 1;
    *is_derived_type = TRUE;

done:
    /* Release resources */
    if(elmt_type_is_derived)
        if(MPI_SUCCESS != (mpi_code = MPI_Type_free(&elmt_type)))
            HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5S_mpio_span_hyper_type() */


/*-------------------------------------------------------------------------
 * Function:	H5S_obtain datatype
 *
 * Purpose:	Obtain an MPI derived datatype based on span-tree
 *              implementation
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Outputs:	*span_type	 the MPI type corresponding to the selection
 *
 * Programmer:  kyang
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_obtain_datatype(const hsize_t *down, H5S_hyper_span_t *span,
    const MPI_Datatype *elmt_type, MPI_Datatype *span_type, size_t elmt_size)
{
    size_t                alloc_count;          /* Number of span tree nodes allocated at this level */
    size_t                outercount;           /* Number of span tree nodes at this level */
    MPI_Datatype          *inner_type = NULL;
    hbool_t inner_types_freed = FALSE;          /* Whether the inner_type MPI datatypes have been freed */
    hbool_t span_type_valid = FALSE;            /* Whether the span_type MPI datatypes is valid */
    int                   *blocklen = NULL;
    MPI_Aint              *disp = NULL;
    H5S_hyper_span_t      *tspan;               /* Temporary pointer to span tree node */
    int                   mpi_code;             /* MPI return status code */
    herr_t                ret_value = SUCCEED;  /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5S_obtain_datatype)

    /* Sanity check */
    HDassert(span);

    /* Allocate the initial displacement & block length buffers */
    alloc_count = H5S_MPIO_INITIAL_ALLOC_COUNT;
    if(NULL == (disp = (MPI_Aint *)H5MM_malloc(alloc_count * sizeof(MPI_Aint))))
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate array of displacements")
    if(NULL == (blocklen = (int *)H5MM_malloc(alloc_count * sizeof(int))))
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate array of block lengths")

    /* if this is the fastest changing dimension, it is the base case for derived datatype. */
    if(NULL == span->down) {
        tspan = span;
        outercount = 0;
        while(tspan) {
            /* Check if we need to increase the size of the buffers */
            if(outercount >= alloc_count) {
                MPI_Aint     *tmp_disp;         /* Temporary pointer to new displacement buffer */
                int          *tmp_blocklen;     /* Temporary pointer to new block length buffer */

                /* Double the allocation count */
                alloc_count *= 2;

                /* Re-allocate the buffers */
                if(NULL == (tmp_disp = (MPI_Aint *)H5MM_realloc(disp, alloc_count * sizeof(MPI_Aint))))
                    HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate array of displacements")
                disp = tmp_disp;
                if(NULL == (tmp_blocklen = (int *)H5MM_realloc(blocklen, alloc_count * sizeof(int))))
                    HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate array of block lengths")
                blocklen = tmp_blocklen;
            } /* end if */

            /* Store displacement & block length */
            disp[outercount]      = (MPI_Aint)elmt_size * tspan->low;
            blocklen[outercount]  = tspan->nelem;

            tspan                 = tspan->next;
            outercount++;
        } /* end while */

        if(MPI_SUCCESS != (mpi_code = MPI_Type_hindexed((int)outercount, blocklen, disp, *elmt_type, span_type)))
              HMPI_GOTO_ERROR(FAIL, "MPI_Type_hindexed failed", mpi_code)
        span_type_valid = TRUE;
    } /* end if */
    else {
        size_t u;               /* Local index variable */

        if(NULL == (inner_type = (MPI_Datatype *)H5MM_malloc(alloc_count * sizeof(MPI_Datatype))))
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate array of inner MPI datatypes")

        tspan = span;
        outercount = 0;
        while(tspan) {
            MPI_Datatype down_type;     /* Temporary MPI datatype for a span tree node's children */
            MPI_Aint stride;            /* Distance between inner MPI datatypes */

            /* Check if we need to increase the size of the buffers */
            if(outercount >= alloc_count) {
                MPI_Aint     *tmp_disp;         /* Temporary pointer to new displacement buffer */
                int          *tmp_blocklen;     /* Temporary pointer to new block length buffer */
                MPI_Datatype *tmp_inner_type;   /* Temporary pointer to inner MPI datatype buffer */

                /* Double the allocation count */
                alloc_count *= 2;

                /* Re-allocate the buffers */
                if(NULL == (tmp_disp = (MPI_Aint *)H5MM_realloc(disp, alloc_count * sizeof(MPI_Aint))))
                    HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate array of displacements")
                disp = tmp_disp;
                if(NULL == (tmp_blocklen = (int *)H5MM_realloc(blocklen, alloc_count * sizeof(int))))
                    HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate array of block lengths")
                blocklen = tmp_blocklen;
                if(NULL == (tmp_inner_type = (MPI_Datatype *)H5MM_realloc(inner_type, alloc_count * sizeof(MPI_Datatype))))
                    HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate array of inner MPI datatypes")
            } /* end if */

            /* Displacement should be in byte and should have dimension information */
            /* First using MPI Type vector to build derived data type for this span only */
            /* Need to calculate the disp in byte for this dimension. */
            /* Calculate the total bytes of the lower dimension */
            disp[outercount]      = tspan->low * (*down) * elmt_size;
            blocklen[outercount]  = 1;

            /* Generate MPI datatype for next dimension down */
            if(H5S_obtain_datatype(down + 1, tspan->down->head, elmt_type, &down_type, elmt_size) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL, "couldn't obtain  MPI derived data type")

            /* Build the MPI datatype for this node */
            stride = (*down) * elmt_size;
            H5_CHECK_OVERFLOW(tspan->nelem, hsize_t, int)
            if(MPI_SUCCESS != (mpi_code = MPI_Type_hvector((int)tspan->nelem, 1, stride, down_type, &inner_type[outercount]))) {
                MPI_Type_free(&down_type);
                HMPI_GOTO_ERROR(FAIL, "MPI_Type_hvector failed", mpi_code)
            } /* end if */

            /* Release MPI datatype for next dimension down */
            if(MPI_SUCCESS != (mpi_code = MPI_Type_free(&down_type)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Type_free failed", mpi_code)

            tspan = tspan->next;
            outercount++;
         } /* end while */

        /* building the whole vector datatype */
        H5_CHECK_OVERFLOW(outercount, size_t, int)
        if(MPI_SUCCESS != (mpi_code = MPI_Type_struct((int)outercount, blocklen, disp, inner_type, span_type)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Type_struct failed", mpi_code)
        span_type_valid = TRUE;

        /* Release inner node types */
        for(u = 0; u < outercount; u++)
            if(MPI_SUCCESS != (mpi_code = MPI_Type_free(&inner_type[u])))
                HMPI_GOTO_ERROR(FAIL, "MPI_Type_free failed", mpi_code)
        inner_types_freed = TRUE;
    } /* end else */

done:
    /* General cleanup */
    if(inner_type != NULL) {
        if(!inner_types_freed) {
            size_t u;          /* Local index variable */

            for(u = 0; u < outercount; u++)
                if(MPI_SUCCESS != (mpi_code = MPI_Type_free(&inner_type[u])))
                    HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)
        } /* end if */

        H5MM_free(inner_type);
    } /* end if */
    if(blocklen != NULL)
        H5MM_free(blocklen);
    if(disp != NULL)
        H5MM_free(disp);

    /* Error cleanup */
    if(ret_value < 0) {
        if(span_type_valid)
            if(MPI_SUCCESS != (mpi_code = MPI_Type_free(span_type)))
                HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)
    } /* end if */

  FUNC_LEAVE_NOAPI(ret_value)
} /* end H5S_obtain_datatype() */


/*-------------------------------------------------------------------------
 * Function:	H5S_mpio_space_type
 *
 * Purpose:	Translate an HDF5 dataspace selection into an MPI type.
 *		Currently handle only hyperslab and "all" selections.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Outputs:	*new_type	  the MPI type corresponding to the selection
 *		*count		  how many objects of the new_type in selection
 *				  (useful if this is the buffer type for xfer)
 *		*is_derived_type  0 if MPI primitive type, 1 if derived
 *
 * Programmer:	rky 980813
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_mpio_space_type(const H5S_t *space, size_t elmt_size,
    MPI_Datatype *new_type, int *count, hbool_t *is_derived_type)
{
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5S_mpio_space_type)

    /* Check args */
    HDassert(space);
    HDassert(elmt_size);

    /* Creat MPI type based on the kind of selection */
    switch(H5S_GET_EXTENT_TYPE(space)) {
        case H5S_NULL:
        case H5S_SCALAR:
        case H5S_SIMPLE:
            switch(H5S_GET_SELECT_TYPE(space)) {
                case H5S_SEL_NONE:
                    if(H5S_mpio_none_type(new_type, count, is_derived_type) < 0)
                        HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't convert 'none' selection to MPI type")
                    break;

                case H5S_SEL_ALL:
                    if(H5S_mpio_all_type(space, elmt_size, new_type, count, is_derived_type) < 0)
                        HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't convert 'all' selection to MPI type")
                    break;

                case H5S_SEL_POINTS:
                    /* not yet implemented */
                    ret_value = FAIL;
                    break;

                case H5S_SEL_HYPERSLABS:
                    if((H5S_SELECT_IS_REGULAR(space) == TRUE)) {
                        if(H5S_mpio_hyper_type(space, elmt_size, new_type, count, is_derived_type) < 0)
                            HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't convert regular 'hyperslab' selection to MPI type")
		    } /* end if */
                    else {
                        if(H5S_mpio_span_hyper_type(space, elmt_size, new_type, count, is_derived_type) < 0)
                            HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't convert irregular 'hyperslab' selection to MPI type")
                    } /* end else */
                    break;

                default:
                    HDassert("unknown selection type" && 0);
                    break;
            } /* end switch */
            break;

        default:
            HDassert("unknown data space type" && 0);
            break;
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5S_mpio_space_type() */
#endif  /* H5_HAVE_PARALLEL */

