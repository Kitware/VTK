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
#include "H5Oprivate.h"		/* Object headers		  	*/
#include "H5Pprivate.h"         /* Property lists                       */
#include "H5Spkg.h"		/* Dataspaces 				*/

#ifdef H5_HAVE_PARALLEL

static herr_t
H5S_mpio_all_type( const H5S_t *space, size_t elmt_size,
		     /* out: */
		     MPI_Datatype *new_type,
		     size_t *count,
		     hsize_t *extra_offset,
		     hbool_t *is_derived_type );
static herr_t
H5S_mpio_none_type( const H5S_t *space, size_t elmt_size,
		     /* out: */
		     MPI_Datatype *new_type,
		     size_t *count,
		     hsize_t *extra_offset,
		     hbool_t *is_derived_type );
static herr_t
H5S_mpio_hyper_type( const H5S_t *space, size_t elmt_size,
		     /* out: */
		     MPI_Datatype *new_type,
		     size_t *count,
		     hsize_t *extra_offset,
		     hbool_t *is_derived_type );

static herr_t
H5S_mpio_span_hyper_type( const H5S_t *space, size_t elmt_size,
			  /* out: */
			  MPI_Datatype *new_type,
			  size_t *count,
			  hsize_t *extra_offset,
			  hbool_t *is_derived_type );

static herr_t H5S_obtain_datatype(const hsize_t size[],
                              H5S_hyper_span_t* span,MPI_Datatype *span_type,
                              size_t elmt_size,int dimindex);


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
 *		*extra_offset     Number of bytes of offset within dataset
 *		*is_derived_type  0 if MPI primitive type, 1 if derived
 *
 * Programmer:	rky 980813
 *
 * Modifications:
 *
 *      Quincey Koziol, June 18, 2002
 *      Added 'extra_offset' parameter
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_mpio_all_type( const H5S_t *space, size_t elmt_size,
		     /* out: */
		     MPI_Datatype *new_type,
		     size_t *count,
		     hsize_t *extra_offset,
		     hbool_t *is_derived_type )
{
    hsize_t	total_bytes;
    hssize_t	snelmts;                /*total number of elmts	(signed) */
    hsize_t	nelmts;                 /*total number of elmts	*/
    herr_t		ret_value = SUCCEED;

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
    H5_ASSIGN_OVERFLOW(*count, total_bytes, hsize_t, size_t);
    *extra_offset = 0;
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
 *		*extra_offset     Number of bytes of offset within dataset
 *		*is_derived_type  0 if MPI primitive type, 1 if derived
 *
 * Programmer:	Quincey Koziol, October 29, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_mpio_none_type( const H5S_t UNUSED *space, size_t UNUSED elmt_size,
		     /* out: */
		     MPI_Datatype *new_type,
		     size_t *count,
		     hsize_t *extra_offset,
		     hbool_t *is_derived_type )
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5S_mpio_none_type);

    /* fill in the return values */
    *new_type = MPI_BYTE;
    *count = 0;
    *extra_offset = 0;
    *is_derived_type = FALSE;

    FUNC_LEAVE_NOAPI(SUCCEED);
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
 *		*extra_offset     Number of bytes of offset within dataset
 *		*is_derived_type  0 if MPI primitive type, 1 if derived
 *
 * Programmer:	rky 980813
 *
 * Modifications:  ppw 990401
 *		rky, ppw 2000-09-26 Freed old type after creating struct type.
 *		rky 2000-10-05 Changed displacements to be MPI_Aint.
 *		rky 2000-10-06 Added code for cases of empty hyperslab.
 *		akc, rky 2000-11-16 Replaced hard coded dimension size with
 *		    H5S_MAX_RANK.
 *
 *      	Quincey Koziol, June 18, 2002
 *      	Added 'extra_offset' parameter.  Also accomodate selection
 *      	offset in MPI type built.
 *
 *      	Albert Cheng, August 4, 2004
 *      	Reimplemented the algorithm of forming the outer_type by
 *      	defining it as (start, vector, extent) in one call.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_mpio_hyper_type( const H5S_t *space, size_t elmt_size,
		     /* out: */
		     MPI_Datatype *new_type,
		     size_t *count,
		     hsize_t *extra_offset,
		     hbool_t *is_derived_type )
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

    FUNC_ENTER_NOAPI_NOINIT(H5S_mpio_hyper_type);

    /* Check args */
    HDassert(space);
    HDassert(sizeof(MPI_Aint) >= sizeof(elmt_size));
    if(0 == elmt_size)
        goto empty;

    /* Initialize selection iterator */
    if(H5S_select_iter_init(&sel_iter, space, elmt_size) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator")
    sel_iter_init = 1;	/* Selection iteration info has been initialized */

    /* Abbreviate args */
    diminfo = sel_iter.u.hyp.diminfo;
    HDassert(diminfo);

    /* make a local copy of the dimension info so we can operate with them */

    /* Check if this is a "flattened" regular hyperslab selection */
    if(sel_iter.u.hyp.iter_rank!=0 && sel_iter.u.hyp.iter_rank<space->extent.rank) {
        /* Flattened selection */
        rank = sel_iter.u.hyp.iter_rank;
        HDassert(rank >= 0 && rank <= H5S_MAX_RANK);	/* within array bounds */
        if (0==rank)
            goto empty;
#ifdef H5S_DEBUG
  if(H5DEBUG(S))
            HDfprintf(H5DEBUG(S), "%s: Flattened selection\n",FUNC);
#endif
        for ( i=0; i<rank; ++i) {
            d[i].start = diminfo[i].start+sel_iter.u.hyp.sel_off[i];
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
            if (0==d[i].block)
                goto empty;
            if (0==d[i].count)
                goto empty;
            if (0==d[i].xtent)
                goto empty;
        }
    } /* end if */
    else {
        /* Non-flattened selection */
        rank = space->extent.rank;
        HDassert(rank >= 0 && rank<=H5S_MAX_RANK);	/* within array bounds */
        if (0==rank)
            goto empty;
#ifdef H5S_DEBUG
  if(H5DEBUG(S))
            HDfprintf(H5DEBUG(S),"%s: Non-flattened selection\n",FUNC);
#endif
        for ( i=0; i<rank; ++i) {
            d[i].start = diminfo[i].start+space->select.offset[i];
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
            if (0==d[i].block)
                goto empty;
            if (0==d[i].count)
                goto empty;
            if (0==d[i].xtent)
                goto empty;
        }
    } /* end else */

/**********************************************************************
    Compute array "offset[rank]" which gives the offsets for a multi-
    dimensional array with dimensions "d[i].xtent" (i=0,1,...,rank-1).
**********************************************************************/
    offset[rank-1] = 1;
    max_xtent[rank-1] = d[rank-1].xtent;
/*#ifdef H5Smpi_DEBUG   */ /* leave the old way */
#ifdef H5S_DEBUG
  if(H5DEBUG(S)){
     i=rank-1;
    HDfprintf(H5DEBUG(S), " offset[%2d]=%d; max_xtent[%2d]=%d\n",
                          i, offset[i], i, max_xtent[i]);
  }
#endif
    for (i=rank-2; i>=0; --i) {
        offset[i] = offset[i+1]*d[i+1].xtent;
        max_xtent[i] = max_xtent[i+1]*d[i].xtent;
#ifdef H5S_DEBUG
  if(H5DEBUG(S)){
    HDfprintf(H5DEBUG(S), " offset[%2d]=%d; max_xtent[%2d]=%d\n",
                          i, offset[i], i, max_xtent[i]);
  }
#endif

    }

    /*  Create a type covering the selected hyperslab.
     *  Multidimensional dataspaces are stored in row-major order.
     *  The type is built from the inside out, going from the
     *  fastest-changing (i.e., inner) dimension * to the slowest (outer). */

/*******************************************************
*  Construct contig type for inner contig dims:
*******************************************************/
#ifdef H5S_DEBUG
  if(H5DEBUG(S)) {
    HDfprintf(H5DEBUG(S), "%s: Making contig type %d MPI_BYTEs\n", FUNC,elmt_size );
    for (i=rank-1; i>=0; --i)
        HDfprintf(H5DEBUG(S), "d[%d].xtent=%Hu \n", i, d[i].xtent);
  }
#endif
    if (MPI_SUCCESS != (mpi_code= MPI_Type_contiguous( (int)elmt_size, MPI_BYTE, &inner_type )))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_contiguous failed", mpi_code);

/*******************************************************
*  Construct the type by walking the hyperslab dims
*  from the inside out:
*******************************************************/
    for ( i=rank-1; i>=0; --i) {
#ifdef H5S_DEBUG
     if(H5DEBUG(S)) {
        HDfprintf(H5DEBUG(S), "%s: Dimension i=%d \n"
            "start=%Hd count=%Hu block=%Hu stride=%Hu, xtent=%Hu max_xtent=%d\n",
            FUNC, i, d[i].start, d[i].count, d[i].block, d[i].strid, d[i].xtent, max_xtent[i]);
     }
#endif

#ifdef H5S_DEBUG
  if(H5DEBUG(S))
            HDfprintf(H5DEBUG(S), "%s: i=%d  Making vector-type \n", FUNC,i);
#endif
       /****************************************
       *  Build vector type of the selection.
       ****************************************/
	mpi_code =MPI_Type_vector((int)(d[i].count),        /* count */
				  (int)(d[i].block),        /* blocklength */
				  (int)(d[i].strid),   	    /* stride */
				  inner_type,	            /* old type */
				  &outer_type);            /* new type */

        MPI_Type_free( &inner_type );
        if (mpi_code!=MPI_SUCCESS)
            HMPI_GOTO_ERROR(FAIL, "couldn't create MPI vector type", mpi_code);

       /****************************************
       *  Then build the dimension type as (start, vector type, xtent).
       ****************************************/
       /* calculate start and extent values of this dimension */
	displacement[1] = d[i].start * offset[i] * elmt_size;
        displacement[2] = (MPI_Aint)elmt_size * max_xtent[i];
        if(MPI_SUCCESS != (mpi_code = MPI_Type_extent(outer_type, &extent_len)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Type_extent failed", mpi_code);

       /*************************************************
       *  Restructure this datatype ("outer_type")
       *  so that it still starts at 0, but its extent
       *  is the full extent in this dimension.
       *************************************************/
        if (displacement[1] > 0 || (int)extent_len < displacement[2]) {

            block_length[0] = 1;
            block_length[1] = 1;
            block_length[2] = 1;

            displacement[0] = 0;

            old_types[0] = MPI_LB;
            old_types[1] = outer_type;
            old_types[2] = MPI_UB;
#ifdef H5S_DEBUG
       if(H5DEBUG(S)){
            HDfprintf(H5DEBUG(S), "%s: i=%d Extending struct type\n"
                "***displacements: %d, %d, %d\n",
		FUNC, i, displacement[0], displacement[1], displacement[2]);
       }
#endif

            mpi_code = MPI_Type_struct ( 3,               /* count */
                                    block_length,    /* blocklengths */
                                    displacement,    /* displacements */
                                    old_types,       /* old types */
                                    &inner_type);    /* new type */

            MPI_Type_free (&outer_type);
    	    if (mpi_code!=MPI_SUCCESS)
                HMPI_GOTO_ERROR(FAIL, "couldn't resize MPI vector type", mpi_code);
        }
        else {
            inner_type = outer_type;
        }
    } /* end for */
/***************************
*  End of loop, walking
*  thru dimensions.
***************************/


    /* At this point inner_type is actually the outermost type, even for 0-trip loop */

    *new_type = inner_type;
    if (MPI_SUCCESS != (mpi_code= MPI_Type_commit( new_type )))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code);

    /* fill in the remaining return values */
    *count = 1;			/* only have to move one of these suckers! */
    *extra_offset = 0;
    *is_derived_type = TRUE;
    HGOTO_DONE(SUCCEED);

empty:
    /* special case: empty hyperslab */
    *new_type = MPI_BYTE;
    *count = 0;
    *extra_offset = 0;
    *is_derived_type = FALSE;

done:
    /* Release selection iterator */
    if(sel_iter_init) {
        if (H5S_SELECT_ITER_RELEASE(&sel_iter)<0)
            HDONE_ERROR (H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator");
    } /* end if */

#ifdef H5S_DEBUG
  if(H5DEBUG(S)){
    HDfprintf(H5DEBUG(S), "Leave %s, count=%ld  is_derived_type=%t\n",
		FUNC, *count, *is_derived_type );
  }
#endif
    FUNC_LEAVE_NOAPI(ret_value);
}


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
 *		*extra_offset     Number of bytes of offset within dataset
 *		*is_derived_type  0 if MPI primitive type, 1 if derived
 *
 * Programmer:  kyang
 *
 */
static herr_t
H5S_mpio_span_hyper_type( const           H5S_t *space,
			  size_t          elmt_size,
			  MPI_Datatype    *new_type,/* out: */
			  size_t          *count,
			  hsize_t         *extra_offset,
			  hbool_t         *is_derived_type )
{
    MPI_Datatype          span_type;
    H5S_hyper_span_t      *ospan;
    H5S_hyper_span_info_t *odown;
    hsize_t               *size;
    int                   mpi_code;
    herr_t                ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5S_mpio_span_hyper_type)

    /* Check args */
    HDassert(space);

    if(0 == elmt_size)
        goto empty;
    size = space->extent.size;
    if(0 == size)
        goto empty;

    odown = space->select.sel_info.hslab->span_lst;
    if(NULL == odown)
        goto empty;
    ospan = odown->head;
    if(NULL == ospan)
        goto empty;

    /* obtain derived data type */
    if(FAIL == H5S_obtain_datatype(space->extent.size, ospan, &span_type, elmt_size, space->extent.rank))
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't obtain  MPI derived data type")

    if(MPI_SUCCESS != (mpi_code = MPI_Type_commit(&span_type)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code);

    *new_type = span_type;
    /* fill in the remaining return values */
    *count = 1;
    *extra_offset = 0;
    *is_derived_type = TRUE;

    HGOTO_DONE(SUCCEED)

empty:
    /* special case: empty hyperslab */
    *new_type        = MPI_BYTE;
    *count           = 0;
    *extra_offset    = 0;
    *is_derived_type = FALSE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5S_mpio_span_hyper_type() */


/*-------------------------------------------------------------------------
 * Function:	H5S_obtain datatype
 *
 * Purpose:	Obtain an MPI derived datatype based on span-tree
                implementation
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Outputs:	*span_type	 the MPI type corresponding to the selection
 *
 * Programmer:  kyang
 *
 */
static herr_t
H5S_obtain_datatype(const hsize_t size[],
			      H5S_hyper_span_t* span,
			      MPI_Datatype *span_type,
			      size_t elmt_size,
			      int dimindex)
{
    int                   innercount, outercount;
    MPI_Datatype          bas_type;
    MPI_Datatype          temp_type;
    MPI_Datatype          tempinner_type;
    MPI_Datatype          *inner_type = NULL;
    int                   *blocklen = NULL;
    MPI_Aint              *disp = NULL;
    MPI_Aint              stride;
    H5S_hyper_span_info_t *down;
    H5S_hyper_span_t      *tspan;
#ifdef H5_HAVE_MPI2
    MPI_Aint              sizeaint, sizedtype;
#endif /* H5_HAVE_MPI2 */
    hsize_t               total_lowd, total_lowd1;
    int                   i;
    int                   mpi_code;
    herr_t                ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5S_obtain_datatype)

    HDassert(span);

    inner_type = NULL;
    down       = NULL;
    tspan      = NULL;
    down       = span->down;
    tspan      = span;

    /* Obtain the number of span tree nodes for this dimension */
    outercount = 0;
    while(tspan) {
        tspan = tspan->next;
        outercount++;
    } /* end while */
    if(outercount == 0)
        HGOTO_DONE(SUCCEED)

/* MPI2 hasn't been widely acccepted, adding H5_HAVE_MPI2 for the future use */
#ifdef H5_HAVE_MPI2
    MPI_Type_extent(MPI_Aint, &sizeaint);
    MPI_Type_extent(MPI_Datatype, &sizedtype);

    blocklen  = (int *)HDcalloc((size_t)outercount, sizeof(int));
    disp = (MPI_Aint *)HDcalloc((size_t)outercount, sizeaint);
    inner_type   = (MPI_Datatype *)HDcalloc((size_t)outercount, sizedtype);
#else
    blocklen     = (int *)HDcalloc((size_t)outercount, sizeof(int));
    disp         = (MPI_Aint *)HDcalloc((size_t)outercount, sizeof(MPI_Aint));
    inner_type   = (MPI_Datatype *)HDcalloc((size_t)outercount, sizeof(MPI_Datatype));
#endif

    tspan      = span;
    outercount = 0;

    /* if this is the fastest changing dimension, it is the base case for derived datatype. */
    if(down == NULL) {

        HDassert(dimindex <= 1);

        if(MPI_SUCCESS != (mpi_code = MPI_Type_contiguous((int)elmt_size, MPI_BYTE, &bas_type)))
              HMPI_GOTO_ERROR(FAIL, "MPI_Type_contiguous failed", mpi_code);

        if(MPI_SUCCESS != (mpi_code = MPI_Type_commit(&bas_type)))
              HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code);

        while(tspan) {
            disp[outercount]      = (MPI_Aint)elmt_size * tspan->low;
            blocklen[outercount]  = tspan->nelem;
            tspan                 = tspan->next;
            outercount++;
        } /* end while */

        if(MPI_SUCCESS != (mpi_code = MPI_Type_hindexed(outercount, blocklen, disp, bas_type, span_type)))
              HMPI_GOTO_ERROR(FAIL, "MPI_Type_hindexed failed", mpi_code);
    } /* end if */
    else {      /* dimindex is the rank of the dimension */

        HDassert(dimindex > 1);

        /* Calculate the total bytes of the lower dimensions */
        total_lowd  = 1;  /* one dimension down */
        total_lowd1 = 1;  /* two dimensions down */

        for(i = dimindex - 1; i > 0; i--)
            total_lowd = total_lowd * size[i];

        for(i = dimindex - 1; i > 1; i--)
            total_lowd1 = total_lowd1 * size[i];

        while(tspan) {

            /* Displacement should be in byte and should have dimension information */
            /* First using MPI Type vector to build derived data type for this span only */
            /* Need to calculate the disp in byte for this dimension. */
            /* Calculate the total bytes of the lower dimension */

            disp[outercount]      = tspan->low * total_lowd * elmt_size;
            blocklen[outercount]  = 1;

            /* generating inner derived datatype by using MPI_Type_hvector */
            if(FAIL == H5S_obtain_datatype(size, tspan->down->head, &temp_type, elmt_size, dimindex - 1))
                HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't obtain  MPI derived data type")

            if(MPI_SUCCESS != (mpi_code = MPI_Type_commit(&temp_type)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code);

            /* building the inner vector datatype */
            stride     = total_lowd * elmt_size;
            innercount = tspan->nelem;

            if(MPI_SUCCESS != (mpi_code = MPI_Type_hvector(innercount, 1, stride, temp_type, &tempinner_type)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Type_hvector failed", mpi_code);

            if(MPI_SUCCESS != (mpi_code = MPI_Type_commit(&tempinner_type)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code);

            if(MPI_SUCCESS != (mpi_code = MPI_Type_free(&temp_type)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Type_free failed", mpi_code);

            inner_type[outercount] = tempinner_type;
            outercount ++;
            tspan = tspan->next;
         } /* end while */

        /* building the whole vector datatype */
        if(MPI_SUCCESS != (mpi_code = MPI_Type_struct(outercount, blocklen, disp, inner_type, span_type)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Type_struct failed", mpi_code);
    } /* end else */

    if(inner_type != NULL && down != NULL) {
    } /* end if */

done:
    if(inner_type != NULL) {
        if(down != NULL) {
            for(i = 0; i < outercount; i++)
                if(MPI_SUCCESS != (mpi_code = MPI_Type_free(&inner_type[i])))
                    HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code);
        } /* end if */

        HDfree(inner_type);
    } /* end if */
    if(blocklen != NULL)
        HDfree(blocklen);
    if(disp != NULL)
        HDfree(disp);

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
 *		*extra_offset     Number of bytes of offset within dataset
 *		*is_derived_type  0 if MPI primitive type, 1 if derived
 *
 * Programmer:	rky 980813
 *
 * Modifications:
 *
 *      Quincey Koziol, June 18, 2002
 *      Added 'extra_offset' parameter
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_mpio_space_type( const H5S_t *space, size_t elmt_size,
		     /* out: */
		     MPI_Datatype *new_type,
		     size_t *count,
		     hsize_t *extra_offset,
		     hbool_t *is_derived_type )
{
    herr_t	ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5S_mpio_space_type);

    /* Check args */
    HDassert(space);

    /* Creat MPI type based on the kind of selection */
    switch (H5S_GET_EXTENT_TYPE(space)) {
        case H5S_NULL:
        case H5S_SCALAR:
        case H5S_SIMPLE:
            switch(H5S_GET_SELECT_TYPE(space)) {
                case H5S_SEL_NONE:
                    if ( H5S_mpio_none_type( space, elmt_size,
                        /* out: */ new_type, count, extra_offset, is_derived_type ) <0)
                        HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't convert \"all\" selection to MPI type");
                    break;

                case H5S_SEL_ALL:
                    if ( H5S_mpio_all_type( space, elmt_size,
                        /* out: */ new_type, count, extra_offset, is_derived_type ) <0)
                        HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't convert \"all\" selection to MPI type");
                    break;

                case H5S_SEL_POINTS:
                    /* not yet implemented */
                    ret_value = FAIL;
                    break;

                case H5S_SEL_HYPERSLABS:
		  if((H5S_SELECT_IS_REGULAR(space) == TRUE)) {
		    if(H5S_mpio_hyper_type( space, elmt_size,
                            /* out: */ new_type, count, extra_offset, is_derived_type )<0)
                        HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't convert \"all\" selection to MPI type");
		    }
		     else {
		       if(H5S_mpio_span_hyper_type( space, elmt_size,
                            /* out: */ new_type, count, extra_offset, is_derived_type )<0)
                        HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't convert \"all\" selection to MPI type");
		     }
                    break;

                default:
                    HDassert("unknown selection type" && 0);
                    break;
            } /* end switch */
            break;

        default:
            HDassert("unknown data space type" && 0);
            break;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}

#endif  /* H5_HAVE_PARALLEL */

