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
#include "H5VMprivate.h"		/* Vector and array functions		*/

#ifdef H5_HAVE_PARALLEL

static herr_t H5S_mpio_all_type(const H5S_t *space, size_t elmt_size,
    MPI_Datatype *new_type, int *count, hbool_t *is_derived_type);
static herr_t H5S_mpio_none_type(MPI_Datatype *new_type, int *count,
    hbool_t *is_derived_type);
static herr_t H5S_mpio_create_point_datatype(size_t elmt_size, hsize_t num_points, 
    MPI_Aint *disp, MPI_Datatype *new_type);
static herr_t H5S_mpio_point_type(const H5S_t *space, size_t elmt_size,
    MPI_Datatype *new_type, int *count, hbool_t *is_derived_type,
    hbool_t do_permute, hsize_t **permute_map, hbool_t *is_permuted);
static herr_t H5S_mpio_permute_type(const H5S_t *space, size_t elmt_size, 
    hsize_t **permute_map, MPI_Datatype *new_type, int *count,
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

    FUNC_ENTER_NOAPI_NOINIT

    /* Check args */
    HDassert(space);

    /* Just treat the entire extent as a block of bytes */
    if((snelmts = (hssize_t)H5S_GET_EXTENT_NPOINTS(space)) < 0)
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
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* fill in the return values */
    *new_type = MPI_BYTE;
    *count = 0;
    *is_derived_type = FALSE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5S_mpio_none_type() */


/*-------------------------------------------------------------------------
 * Function:	H5S_mpio_create_point_datatype
 *
 * Purpose:	Create a derived datatype for point selections.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Outputs:	*new_type	  the MPI type corresponding to the selection
 *
 * Programmer:	Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5S_mpio_create_point_datatype (size_t elmt_size, hsize_t num_points,
    MPI_Aint *disp, MPI_Datatype *new_type) 
{
    MPI_Datatype   elmt_type;           /* MPI datatype for individual element */
    hbool_t        elmt_type_created = FALSE;   /* Whether the element MPI datatype was created */
    int            mpi_code;            /* MPI error code */
    int            *blocks = NULL;      /* Array of block sizes for MPI hindexed create call */
    hsize_t        u;                   /* Local index variable */
    herr_t	   ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Create an MPI datatype for an element */
    if(MPI_SUCCESS != (mpi_code = MPI_Type_contiguous((int)elmt_size, MPI_BYTE, &elmt_type)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_contiguous failed", mpi_code)
    elmt_type_created = TRUE;
    
    /* Allocate block sizes for MPI datatype call */
    if(NULL == (blocks = (int *)H5MM_malloc(sizeof(int) * num_points)))
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate array of blocks")

    /* Would be nice to have Create_Hindexed_block to avoid this array of all ones */
    for(u = 0; u < num_points; u++)
        blocks[u] = 1;

    /* Create an MPI datatype for the whole point selection */
    if(MPI_SUCCESS != (mpi_code = MPI_Type_create_hindexed((int)num_points, blocks, disp, elmt_type, new_type)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_create_indexed_block failed", mpi_code)

    /* Commit MPI datatype for later use */
    if(MPI_SUCCESS != (mpi_code = MPI_Type_commit(new_type)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code)

done:
    if(elmt_type_created)
        MPI_Type_free(&elmt_type);
    if(blocks)
        H5MM_free(blocks);

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5S_mpio_create_point_datatype() */


/*-------------------------------------------------------------------------
 * Function:	H5S_mpio_point_type
 *
 * Purpose:	Translate an HDF5 "point" selection into an MPI type.
 *              Create a permutation array to handle out-of-order point selections.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Outputs:	*new_type	  the MPI type corresponding to the selection
 *		*count		  how many objects of the new_type in selection
 *				  (useful if this is the buffer type for xfer)
 *		*is_derived_type  0 if MPI primitive type, 1 if derived
 *              *permute_map      the permutation of the displacements to create
 *                                the MPI_Datatype
 *              *is_permuted      0 if the displacements are permuted, 1 if not
 *
 * Programmer:	Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_mpio_point_type(const H5S_t *space, size_t elmt_size, MPI_Datatype *new_type, 
    int *count, hbool_t *is_derived_type, hbool_t do_permute, hsize_t **permute,
    hbool_t *is_permuted)
{
    MPI_Aint *disp = NULL;      /* Datatype displacement for each point*/
    H5S_pnt_node_t *curr = NULL; /* Current point being operated on in from the selection */
    hssize_t snum_points;       /* Signed number of elements in selection */
    hsize_t num_points;         /* Sumber of points in the selection */
    hsize_t u;                  /* Local index variable */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check args */
    HDassert(space);

    /* Get the total number of points selected */
    if((snum_points = (hssize_t)H5S_GET_SELECT_NPOINTS(space)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOUNT, FAIL, "can't get number of elements selected")
    num_points = (hsize_t)snum_points;

    /* Allocate array for element displacements */
    if(NULL == (disp = (MPI_Aint *)H5MM_malloc(sizeof(MPI_Aint) * num_points)))
         HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate array of displacements")

    /* Allocate array for element permutation - returned to caller */
    if(do_permute)
        if(NULL == (*permute = (hsize_t *)H5MM_malloc(sizeof(hsize_t) * num_points)))
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate permutation array")

    /* Iterate through list of elements */
    curr = space->select.sel_info.pnt_lst->head;
    for(u = 0 ; u < num_points ; u++) {
        /* calculate the displacement of the current point */
        disp[u] = H5VM_array_offset(space->extent.rank, space->extent.size, curr->pnt);
        disp[u] *= elmt_size;

        /* This is a File Space used to set the file view, so adjust the displacements 
         * to have them monotonically non-decreasing.
         * Generate the permutation array by indicating at each point being selected, 
         * the position it will shifted in the new displacement. Example: 
         * Suppose 4 points with corresponding are selected 
         * Pt 1: disp=6 ; Pt 2: disp=3 ; Pt 3: disp=0 ; Pt 4: disp=4 
         * The permute map to sort the displacements in order will be:
         * point 1: map[0] = L, indicating that this point is not moved (1st point selected)
         * point 2: map[1] = 0, indicating that this point is moved to the first position, 
         *                      since disp_pt1(6) > disp_pt2(3)
         * point 3: map[2] = 0, move to position 0, bec it has the lowest disp between 
         *                      the points selected so far.
         * point 4: map[3] = 2, move the 2nd position since point 1 has a higher disp, 
         *                      but points 2 and 3 have lower displacements.
         */
        if(do_permute) {
            if(u > 0 && disp[u] < disp[u - 1]) {
                unsigned s = 0, l = u, m = u / 2;

                *is_permuted = TRUE;
                do {
                    if(disp[u] > disp[m])
                        s = m + 1;
                    else if(disp[u] < disp[m])
                        l = m;
                    else
                        break;
                    m = s + ((l - s) / 2);
                } while(s < l);

                if(m < u) {
                    MPI_Aint temp;

                    temp = disp[u];
                    HDmemmove(disp + m + 1, disp + m, (u - m) * sizeof(MPI_Aint));
                    disp[m] = temp;
                } /* end if */
                (*permute)[u] = m;                
            } /* end if */
            else
                (*permute)[u] = num_points;
        } /* end if */
        /* this is a memory space, and no permutation is necessary to create
           the derived datatype */
        else {
            ;/* do nothing */
        } /* end else */

        /* get the next point */
        curr = curr->next;
    } /* end for */

    /* Create the MPI datatype for the set of element displacements */
    if(H5S_mpio_create_point_datatype(elmt_size, num_points, disp, new_type) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL, "couldn't create an MPI Datatype from point selection")

    /* Set values about MPI datatype created */
    *count = 1;
    *is_derived_type = TRUE;

done:
    if(NULL != disp)
        H5MM_free(disp);

    /* Release the permutation buffer, if it wasn't used */
    if(!(*is_permuted) && (*permute)) {
        H5MM_free(*permute);
        *permute = NULL;
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5S_mpio_point_type() */


/*-------------------------------------------------------------------------
 * Function:	H5S_mpio_permute_type
 *
 * Purpose:	Translate an HDF5 "all/hyper/point" selection into an MPI type,
 *              while applying the permutation map. This function is called if
 *              the file space selection is permuted due to out-of-order point
 *              selection and so the memory datatype has to be permuted using the
 *              permutation map created by the file selection.
 *
 * Note:	This routine is called from H5S_mpio_space_type(), which is
 *              called first for the file dataspace and creates
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Outputs:	*new_type	  the MPI type corresponding to the selection
 *		*count		  how many objects of the new_type in selection
 *				  (useful if this is the buffer type for xfer)
 *		*is_derived_type  0 if MPI primitive type, 1 if derived
 *
 * Programmer:	Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_mpio_permute_type(const H5S_t *space, size_t elmt_size, hsize_t **permute, 
    MPI_Datatype *new_type, int *count, hbool_t *is_derived_type)
{
    MPI_Aint *disp = NULL;      /* Datatype displacement for each point*/
    H5S_sel_iter_t sel_iter;    /* Selection iteration info */
    hbool_t sel_iter_init = FALSE;      /* Selection iteration info has been initialized */
    hsize_t off[H5D_IO_VECTOR_SIZE];    /* Array to store sequence offsets */
    size_t len[H5D_IO_VECTOR_SIZE];     /* Array to store sequence lengths */
    hssize_t snum_points;       /* Signed number of elements in selection */
    hsize_t num_points;         /* Number of points in the selection */
    size_t max_elem;            /* Maximum number of elements allowed in sequences */
    hsize_t u;                  /* Local index variable */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check args */
    HDassert(space);

    /* Get the total number of points selected */
    if((snum_points = (hssize_t)H5S_GET_SELECT_NPOINTS(space)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOUNT, FAIL, "can't get number of elements selected")
    num_points = (hsize_t)snum_points;

    /* Allocate array to store point displacements */
    if(NULL == (disp = (MPI_Aint *)H5MM_malloc(sizeof(MPI_Aint) * num_points)))
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate array of displacements")

    /* Initialize selection iterator */
    if(H5S_select_iter_init(&sel_iter, space, elmt_size) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator")
    sel_iter_init = TRUE;	/* Selection iteration info has been initialized */

    /* Set the number of elements to iterate over */
    H5_ASSIGN_OVERFLOW(max_elem, num_points, hsize_t, size_t);

    /* Loop, while elements left in selection */
    u = 0;
    while(max_elem > 0) {
        hsize_t off[H5D_IO_VECTOR_SIZE];        /* Array to store sequence offsets */
        size_t len[H5D_IO_VECTOR_SIZE];         /* Array to store sequence lengths */
        size_t nelem;               /* Number of elements used in sequences */
        size_t nseq;                /* Number of sequences generated */
        size_t curr_seq;            /* Current sequence being worked on */

        /* Get the sequences of bytes */
        if(H5S_SELECT_GET_SEQ_LIST(space, 0, &sel_iter, (size_t)H5D_IO_VECTOR_SIZE, max_elem, &nseq, &nelem, off, len) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, FAIL, "sequence length generation failed")

        /* Loop, while sequences left to process */
        for(curr_seq = 0; curr_seq < nseq; curr_seq++) {
            hsize_t curr_off;           /* Current offset within sequence */
            size_t curr_len;            /* Length of bytes left to process in sequence */

            /* Get the current offset */
            curr_off = off[curr_seq];

            /* Get the number of bytes in sequence */
            curr_len = len[curr_seq];

            /* Loop, while bytes left in sequence */
            while(curr_len > 0) {
                /* Set the displacement of the current point */
                disp[u] = curr_off;

                /* This is a memory displacement, so for each point selected, 
                 * apply the map that was generated by the file selection */
                if((*permute)[u] != num_points) {
                    MPI_Aint temp = disp[u];

                    HDmemmove(disp + (*permute)[u] + 1, disp + (*permute)[u], 
                             (u - (*permute)[u]) * sizeof(MPI_Aint));
                    disp[(*permute)[u]] = temp;
                } /* end if */

                /* Advance to next element */
                u++;

                /* Increment offset in dataspace */
                curr_off += elmt_size;

                /* Decrement number of bytes left in sequence */
                curr_len -= elmt_size;
            } /* end while */
        } /* end for */

        /* Decrement number of elements left to process */
        max_elem -= nelem;
    } /* end while */

    /* Create the MPI datatype for the set of element displacements */
    if(H5S_mpio_create_point_datatype(elmt_size, num_points, disp, new_type) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL, "couldn't create an MPI Datatype from point selection")

    /* Set values about MPI datatype created */
    *count = 1;
    *is_derived_type = TRUE;

done:
    /* Release selection iterator */
    if(sel_iter_init)
        if(H5S_SELECT_ITER_RELEASE(&sel_iter) < 0)
            HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator")

    /* Free memory */
    if(disp)
        H5MM_free(disp);
    if(*permute) {
        H5MM_free(*permute);
        *permute = NULL;
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5S_mpio_permute_type() */


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

    hsize_t		offset[H5S_MAX_RANK];
    hsize_t		max_xtent[H5S_MAX_RANK];
    H5S_hyper_dim_t	*diminfo;		/* [rank] */
    unsigned		rank;
    int			block_length[3];
    MPI_Datatype	inner_type, outer_type, old_types[3];
    MPI_Aint            extent_len, displacement[3];
    unsigned		u;			/* Local index variable */
    int			i;			/* Local index variable */
    int                 mpi_code;               /* MPI return code */
    herr_t		ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

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
        HDassert(rank <= H5S_MAX_RANK);	/* within array bounds */
#ifdef H5S_DEBUG
  if(H5DEBUG(S))
            HDfprintf(H5DEBUG(S), "%s: Flattened selection\n",FUNC);
#endif
        for(u = 0; u < rank; ++u) {
            H5_CHECK_OVERFLOW(diminfo[u].start, hsize_t, hssize_t)
            d[u].start = (hssize_t)diminfo[u].start + sel_iter.u.hyp.sel_off[u];
            d[u].strid = diminfo[u].stride;
            d[u].block = diminfo[u].block;
            d[u].count = diminfo[u].count;
            d[u].xtent = sel_iter.u.hyp.size[u];
#ifdef H5S_DEBUG
       if(H5DEBUG(S)){
            HDfprintf(H5DEBUG(S), "%s: start=%Hd  stride=%Hu  count=%Hu  block=%Hu  xtent=%Hu",
                FUNC, d[u].start, d[u].strid, d[u].count, d[u].block, d[u].xtent );
            if (u==0)
                HDfprintf(H5DEBUG(S), "  rank=%u\n", rank );
            else
                HDfprintf(H5DEBUG(S), "\n" );
      }
#endif
            if(0 == d[u].block)
                goto empty;
            if(0 == d[u].count)
                goto empty;
            if(0 == d[u].xtent)
                goto empty;
        } /* end for */
    } /* end if */
    else {
        /* Non-flattened selection */
        rank = space->extent.rank;
        HDassert(rank <= H5S_MAX_RANK);	/* within array bounds */
        if(0 == rank)
            goto empty;
#ifdef H5S_DEBUG
  if(H5DEBUG(S))
            HDfprintf(H5DEBUG(S),"%s: Non-flattened selection\n",FUNC);
#endif
        for(u = 0; u < rank; ++u) {
            H5_CHECK_OVERFLOW(diminfo[u].start, hsize_t, hssize_t)
            d[u].start = (hssize_t)diminfo[u].start + space->select.offset[u];
            d[u].strid = diminfo[u].stride;
            d[u].block = diminfo[u].block;
            d[u].count = diminfo[u].count;
            d[u].xtent = space->extent.size[u];
#ifdef H5S_DEBUG
  if(H5DEBUG(S)){
    HDfprintf(H5DEBUG(S), "%s: start=%Hd  stride=%Hu  count=%Hu  block=%Hu  xtent=%Hu",
              FUNC, d[u].start, d[u].strid, d[u].count, d[u].block, d[u].xtent );
    if (u==0)
        HDfprintf(H5DEBUG(S), "  rank=%u\n", rank );
    else
        HDfprintf(H5DEBUG(S), "\n" );
  }
#endif
            if(0 == d[u].block)
                goto empty;
            if(0 == d[u].count)
                goto empty;
            if(0 == d[u].xtent)
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
     i = ((int)rank) - 1;
     HDfprintf(H5DEBUG(S), " offset[%2d]=%Hu; max_xtent[%2d]=%Hu\n",
                          i, offset[i], i, max_xtent[i]);
  }
#endif
    for(i = ((int)rank) - 2; i >= 0; --i) {
        offset[i] = offset[i + 1] * d[i + 1].xtent;
        max_xtent[i] = max_xtent[i + 1] * d[i].xtent;
#ifdef H5S_DEBUG
  if(H5DEBUG(S))
    HDfprintf(H5DEBUG(S), " offset[%2d]=%Hu; max_xtent[%2d]=%Hu\n",
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
    for(i = ((int)rank) - 1; i >= 0; --i)
        HDfprintf(H5DEBUG(S), "d[%d].xtent=%Hu \n", i, d[i].xtent);
  }
#endif
    if(MPI_SUCCESS != (mpi_code = MPI_Type_contiguous((int)elmt_size, MPI_BYTE, &inner_type)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_contiguous failed", mpi_code)

/*******************************************************
*  Construct the type by walking the hyperslab dims
*  from the inside out:
*******************************************************/
    for(i = ((int)rank) - 1; i >= 0; --i) {
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

    FUNC_ENTER_NOAPI_NOINIT

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
    if(H5VM_array_down(space->extent.rank, space->extent.size, down) < 0)
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
 * Function:	H5S_obtain_datatype
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

    FUNC_ENTER_NOAPI_NOINIT

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
            H5_CHECK_OVERFLOW(tspan->nelem, hsize_t, int)
            blocklen[outercount]  = (int)tspan->nelem;

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
                inner_type = tmp_inner_type;
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
H5S_mpio_space_type(const H5S_t *space, size_t elmt_size, MPI_Datatype *new_type, 
    int *count, hbool_t *is_derived_type, hbool_t do_permute, hsize_t **permute_map,
    hbool_t *is_permuted)
{
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check args */
    HDassert(space);
    HDassert(elmt_size);

    /* Create MPI type based on the kind of selection */
    switch(H5S_GET_EXTENT_TYPE(space)) {
        case H5S_NULL:
        case H5S_SCALAR:
        case H5S_SIMPLE:
            /* If the file space has been permuted previously due to
             * out-of-order point selection, then permute this selection which
             * should be a memory selection to match the file space permutation.
             */
            if(TRUE == *is_permuted) { 
                switch(H5S_GET_SELECT_TYPE(space)) {
                    case H5S_SEL_NONE:
                        if(H5S_mpio_none_type(new_type, count, is_derived_type) < 0)
                            HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL, "couldn't convert 'none' selection to MPI type")
                        break;

                    case H5S_SEL_ALL:
                    case H5S_SEL_POINTS:
                    case H5S_SEL_HYPERSLABS:
                        /* Sanity check */
                        HDassert(!do_permute);

                        if(H5S_mpio_permute_type(space, elmt_size, permute_map, new_type, count, is_derived_type) < 0)
                            HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL, "couldn't convert 'all' selection to MPI type")
                        break;

                    case H5S_SEL_ERROR:
                    case H5S_SEL_N:
                    default:
                        HDassert("unknown selection type" && 0);
                        break;
                } /* end switch */
            } /* end if */
            /* the file space is not permuted, so do a regular selection */
            else {
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
                        if(H5S_mpio_point_type(space, elmt_size, new_type, count, is_derived_type, do_permute, permute_map, is_permuted) < 0)
                           HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL, "couldn't convert 'point' selection to MPI type")
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

                    case H5S_SEL_ERROR:
                    case H5S_SEL_N:
                    default:
                        HDassert("unknown selection type" && 0);
                        break;
                } /* end switch */
            } /* end else */
            break;

        case H5S_NO_CLASS:
        default:
            HDassert("unknown data space type" && 0);
            break;
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5S_mpio_space_type() */
#endif  /* H5_HAVE_PARALLEL */

