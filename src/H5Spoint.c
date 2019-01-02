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

/*
 * Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              Tuesday, June 16, 1998
 *
 * Purpose:	Point selection dataspace I/O functions.
 */

#include "H5Smodule.h"          /* This source code file is part of the H5S module */


#include "H5private.h"		/* Generic Functions			  */
#include "H5Eprivate.h"		/* Error handling		  */
#include "H5FLprivate.h"	/* Free Lists	  */
#include "H5Iprivate.h"		/* ID Functions		  */
#include "H5MMprivate.h"	/* Memory Management functions		  */
#include "H5Spkg.h"		/* Dataspace functions			  */
#include "H5VMprivate.h"         /* Vector functions */

/* Static function prototypes */

/* Selection callbacks */
static herr_t H5S_point_copy(H5S_t *dst, const H5S_t *src, hbool_t share_selection);
static herr_t H5S_point_get_seq_list(const H5S_t *space, unsigned flags,
    H5S_sel_iter_t *iter, size_t maxseq, size_t maxbytes,
    size_t *nseq, size_t *nbytes, hsize_t *off, size_t *len);
static herr_t H5S_point_release(H5S_t *space);
static htri_t H5S_point_is_valid(const H5S_t *space);
static hssize_t H5S_point_serial_size(const H5S_t *space, H5F_t *f);
static herr_t H5S_point_serialize(const H5S_t *space, uint8_t **p, H5F_t *f);
static herr_t H5S_point_deserialize(H5S_t *space, uint32_t version, uint8_t flags,
    const uint8_t **p);
static herr_t H5S_point_bounds(const H5S_t *space, hsize_t *start, hsize_t *end);
static herr_t H5S_point_offset(const H5S_t *space, hsize_t *off);
static int H5S__point_unlim_dim(const H5S_t *space);
static htri_t H5S_point_is_contiguous(const H5S_t *space);
static htri_t H5S_point_is_single(const H5S_t *space);
static htri_t H5S_point_is_regular(const H5S_t *space);
static void H5S_point_adjust_u(H5S_t *space, const hsize_t *offset);
static herr_t H5S_point_project_scalar(const H5S_t *space, hsize_t *offset);
static herr_t H5S_point_project_simple(const H5S_t *space, H5S_t *new_space, hsize_t *offset);
static herr_t H5S_point_iter_init(H5S_sel_iter_t *iter, const H5S_t *space);

/* Selection iteration callbacks */
static herr_t H5S_point_iter_coords(const H5S_sel_iter_t *iter, hsize_t *coords);
static herr_t H5S_point_iter_block(const H5S_sel_iter_t *iter, hsize_t *start, hsize_t *end);
static hsize_t H5S_point_iter_nelmts(const H5S_sel_iter_t *iter);
static htri_t H5S_point_iter_has_next_block(const H5S_sel_iter_t *iter);
static herr_t H5S_point_iter_next(H5S_sel_iter_t *sel_iter, hsize_t nelem);
static herr_t H5S_point_iter_next_block(H5S_sel_iter_t *sel_iter);
static herr_t H5S_point_iter_release(H5S_sel_iter_t *sel_iter);

/* Selection properties for point selections */
const H5S_select_class_t H5S_sel_point[1] = {{
    H5S_SEL_POINTS,

    /* Methods on selection */
    H5S_point_copy,
    H5S_point_get_seq_list,
    H5S_point_release,
    H5S_point_is_valid,
    H5S_point_serial_size,
    H5S_point_serialize,
    H5S_point_deserialize,
    H5S_point_bounds,
    H5S_point_offset,
    H5S__point_unlim_dim,
    NULL,
    H5S_point_is_contiguous,
    H5S_point_is_single,
    H5S_point_is_regular,
    H5S_point_adjust_u,
    H5S_point_project_scalar,
    H5S_point_project_simple,
    H5S_point_iter_init,
}};

/* Iteration properties for point selections */
static const H5S_sel_iter_class_t H5S_sel_iter_point[1] = {{
    H5S_SEL_POINTS,

    /* Methods on selection iterator */
    H5S_point_iter_coords,
    H5S_point_iter_block,
    H5S_point_iter_nelmts,
    H5S_point_iter_has_next_block,
    H5S_point_iter_next,
    H5S_point_iter_next_block,
    H5S_point_iter_release,
}};

/* Declare a free list to manage the H5S_pnt_node_t struct */
H5FL_DEFINE_STATIC(H5S_pnt_node_t);

/* Declare a free list to manage the H5S_pnt_list_t struct */
H5FL_DEFINE_STATIC(H5S_pnt_list_t);


/*-------------------------------------------------------------------------
 * Function:	H5S_point_iter_init
 *
 * Purpose:	Initializes iteration information for point selection.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, June 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_point_iter_init(H5S_sel_iter_t *iter, const H5S_t *space)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space && H5S_SEL_POINTS==H5S_GET_SELECT_TYPE(space));
    HDassert(iter);

    /* Initialize the number of points to iterate over */
    iter->elmt_left=space->select.num_elem;

    /* Start at the head of the list of points */
    iter->u.pnt.curr=space->select.sel_info.pnt_lst->head;

    /* Initialize type of selection iterator */
    iter->type=H5S_sel_iter_point;

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_point_iter_init() */


/*-------------------------------------------------------------------------
 * Function:	H5S_point_iter_coords
 *
 * Purpose:	Retrieve the current coordinates of iterator for current
 *              selection
 *
 * Return:	non-negative on success, negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, April 22, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_point_iter_coords (const H5S_sel_iter_t *iter, hsize_t *coords)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(iter);
    HDassert(coords);

    /* Copy the offset of the current point */
    HDmemcpy(coords,iter->u.pnt.curr->pnt,sizeof(hsize_t)*iter->rank);

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_point_iter_coords() */


/*-------------------------------------------------------------------------
 * Function:	H5S_point_iter_block
 *
 * Purpose:	Retrieve the current block of iterator for current
 *              selection
 *
 * Return:	non-negative on success, negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, June 2, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_point_iter_block (const H5S_sel_iter_t *iter, hsize_t *start, hsize_t *end)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(iter);
    HDassert(start);
    HDassert(end);

    /* Copy the current point as a block */
    HDmemcpy(start,iter->u.pnt.curr->pnt,sizeof(hsize_t)*iter->rank);
    HDmemcpy(end,iter->u.pnt.curr->pnt,sizeof(hsize_t)*iter->rank);

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_point_iter_block() */


/*-------------------------------------------------------------------------
 * Function:	H5S_point_iter_nelmts
 *
 * Purpose:	Return number of elements left to process in iterator
 *
 * Return:	non-negative number of elements on success, zero on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, June 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static hsize_t
H5S_point_iter_nelmts (const H5S_sel_iter_t *iter)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(iter);

    FUNC_LEAVE_NOAPI(iter->elmt_left)
}   /* H5S_point_iter_nelmts() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_iter_has_next_block
 PURPOSE
    Check if there is another block left in the current iterator
 USAGE
    htri_t H5S_point_iter_has_next_block(iter)
        const H5S_sel_iter_t *iter;       IN: Pointer to selection iterator
 RETURNS
    Non-negative (TRUE/FALSE) on success/Negative on failure
 DESCRIPTION
    Check if there is another block available in the selection iterator.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static htri_t
H5S_point_iter_has_next_block(const H5S_sel_iter_t *iter)
{
    htri_t ret_value=TRUE;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(iter);

    /* Check if there is another point in the list */
    if(iter->u.pnt.curr->next==NULL)
        HGOTO_DONE(FALSE);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_point_iter_has_next_block() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_iter_next
 PURPOSE
    Increment selection iterator
 USAGE
    herr_t H5S_point_iter_next(iter, nelem)
        H5S_sel_iter_t *iter;       IN: Pointer to selection iterator
        hsize_t nelem;              IN: Number of elements to advance by
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Advance selection iterator to the NELEM'th next element in the selection.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_point_iter_next(H5S_sel_iter_t *iter, hsize_t nelem)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(iter);
    HDassert(nelem>0);

    /* Increment the iterator */
    while(nelem>0) {
        iter->u.pnt.curr=iter->u.pnt.curr->next;
        nelem--;
    } /* end while */

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_point_iter_next() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_iter_next_block
 PURPOSE
    Increment selection iterator to next block
 USAGE
    herr_t H5S_point_iter_next_block(iter)
        H5S_sel_iter_t *iter;       IN: Pointer to selection iterator
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Advance selection iterator to the next block in the selection.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_point_iter_next_block(H5S_sel_iter_t *iter)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(iter);

    /* Increment the iterator */
    iter->u.pnt.curr=iter->u.pnt.curr->next;

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_point_iter_next_block() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_iter_release
 PURPOSE
    Release point selection iterator information for a dataspace
 USAGE
    herr_t H5S_point_iter_release(iter)
        H5S_sel_iter_t *iter;       IN: Pointer to selection iterator
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Releases all information for a dataspace point selection iterator
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_point_iter_release (H5S_sel_iter_t H5_ATTR_UNUSED * iter)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(iter);

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_point_iter_release() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_add
 PURPOSE
    Add a series of elements to a point selection
 USAGE
    herr_t H5S_point_add(space, num_elem, coord)
        H5S_t *space;           IN: Dataspace of selection to modify
        hsize_t num_elem;       IN: Number of elements in COORD array.
        const hsize_t *coord[];    IN: The location of each element selected
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    This function adds elements to the current point selection for a dataspace
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_point_add(H5S_t *space, H5S_seloper_t op, hsize_t num_elem, const hsize_t *coord)
{
    H5S_pnt_node_t *top = NULL, *curr = NULL, *new_node = NULL; /* Point selection nodes */
    unsigned u;                         /* Counter */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(space);
    HDassert(num_elem > 0);
    HDassert(coord);
    HDassert(op == H5S_SELECT_SET || op == H5S_SELECT_APPEND || op == H5S_SELECT_PREPEND);

    for(u = 0; u < num_elem; u++) {
        /* Allocate space for the new node */
        if(NULL == (new_node = H5FL_MALLOC(H5S_pnt_node_t)))
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate point node")

        /* Initialize fields in node */
        new_node->next = NULL;
        if(NULL == (new_node->pnt = (hsize_t *)H5MM_malloc(space->extent.rank * sizeof(hsize_t))))
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate coordinate information")

        /* Copy over the coordinates */
        HDmemcpy(new_node->pnt, coord + (u * space->extent.rank), (space->extent.rank * sizeof(hsize_t)));

        /* Link into list */
        if(top == NULL)
            top = new_node;
        else
            curr->next = new_node;
        curr = new_node;
    } /* end for */
    new_node = NULL;

    /* Insert the list of points selected in the proper place */
    if(op == H5S_SELECT_SET || op == H5S_SELECT_PREPEND) {
        /* Append current list, if there is one */
        if(NULL != space->select.sel_info.pnt_lst->head)
            curr->next = space->select.sel_info.pnt_lst->head;

        /* Put new list in point selection */
        space->select.sel_info.pnt_lst->head = top;
    } /* end if */
    else {  /* op==H5S_SELECT_APPEND */
        H5S_pnt_node_t *tmp_node;       /* Temporary point selection node */

        tmp_node = space->select.sel_info.pnt_lst->head;
        if(tmp_node != NULL) {
            while(tmp_node->next != NULL)
                tmp_node = tmp_node->next;

            /* Append new list to point selection */
            tmp_node->next = top;
        } /* end if */
        else
            space->select.sel_info.pnt_lst->head = top;
    } /* end else */

    /* Set the number of elements in the new selection */
    if(op == H5S_SELECT_SET)
        space->select.num_elem = num_elem;
    else
        space->select.num_elem += num_elem;

done:
    if(ret_value < 0) {
        /* Release possibly partially initialized new node */
        if(new_node)
            new_node = H5FL_FREE(H5S_pnt_node_t, new_node);

        /* Release possible linked list of nodes */
        while(top) {
            curr = top->next; 
            H5MM_xfree(top->pnt);
            top = H5FL_FREE(H5S_pnt_node_t, top);
            top = curr;
        } /* end while */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_point_add() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_release
 PURPOSE
    Release point selection information for a dataspace
 USAGE
    herr_t H5S_point_release(space)
        H5S_t *space;       IN: Pointer to dataspace
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Releases all point selection information for a dataspace
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_point_release (H5S_t *space)
{
    H5S_pnt_node_t *curr, *next;        /* Point selection nodes */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);

    /* Delete all the nodes from the list */
    curr = space->select.sel_info.pnt_lst->head;
    while(curr != NULL) {
        next = curr->next;
        H5MM_xfree(curr->pnt);
        curr = H5FL_FREE(H5S_pnt_node_t, curr);
        curr = next;
    } /* end while */

    /* Free & reset the point list header */
    space->select.sel_info.pnt_lst = H5FL_FREE(H5S_pnt_list_t, space->select.sel_info.pnt_lst);

    /* Reset the number of elements in the selection */
    space->select.num_elem = 0;

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_point_release() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_elements
 PURPOSE
    Specify a series of elements in the dataspace to select
 USAGE
    herr_t H5S_select_elements(dsid, op, num_elem, coord)
        hid_t dsid;             IN: Dataspace ID of selection to modify
        H5S_seloper_t op;       IN: Operation to perform on current selection
        hsize_t num_elem;       IN: Number of elements in COORD array.
        const hsize_t *coord;   IN: The location of each element selected
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    This function selects array elements to be included in the selection for
    the dataspace.  The COORD array is a 2-D array of size <dataspace rank>
    by NUM_ELEM (ie. a list of coordinates in the dataspace).  The order of
    the element coordinates in the COORD array specifies the order that the
    array elements are iterated through when I/O is performed.  Duplicate
    coordinates are not checked for.  The selection operator, OP, determines
    how the new selection is to be combined with the existing selection for
    the dataspace.  Currently, only H5S_SELECT_SET is supported, which replaces
    the existing selection with the one defined in this call.  When operators
    other than H5S_SELECT_SET are used to combine a new selection with an
    existing selection, the selection ordering is reset to 'C' array ordering.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_elements(H5S_t *space, H5S_seloper_t op, hsize_t num_elem,
    const hsize_t *coord)
{
    herr_t ret_value = SUCCEED;  /* return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check args */
    HDassert(space);
    HDassert(num_elem);
    HDassert(coord);
    HDassert(op == H5S_SELECT_SET || op == H5S_SELECT_APPEND || op == H5S_SELECT_PREPEND);

    /* If we are setting a new selection, remove current selection first */
    if(op == H5S_SELECT_SET || H5S_GET_SELECT_TYPE(space) != H5S_SEL_POINTS)
        if(H5S_SELECT_RELEASE(space) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't release point selection")

    /* Allocate space for the point selection information if necessary */
    if(H5S_GET_SELECT_TYPE(space) != H5S_SEL_POINTS || space->select.sel_info.pnt_lst == NULL)
        if(NULL == (space->select.sel_info.pnt_lst = H5FL_CALLOC(H5S_pnt_list_t)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate element information")

    /* Add points to selection */
    if(H5S_point_add(space, op, num_elem, coord) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINSERT, FAIL, "can't insert elements")

    /* Set selection type */
    space->select.type = H5S_sel_point;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_elements() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_copy
 PURPOSE
    Copy a selection from one dataspace to another
 USAGE
    herr_t H5S_point_copy(dst, src)
        H5S_t *dst;  OUT: Pointer to the destination dataspace
        H5S_t *src;  IN: Pointer to the source dataspace
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Copies all the point selection information from the source
    dataspace to the destination dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_point_copy(H5S_t *dst, const H5S_t *src, hbool_t H5_ATTR_UNUSED share_selection)
{
    H5S_pnt_node_t *curr, *new_node, *new_tail;    /* Point information nodes */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(src);
    HDassert(dst);

    /* Allocate room for the head of the point list */
    if(NULL == (dst->select.sel_info.pnt_lst = H5FL_MALLOC(H5S_pnt_list_t)))
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate point list node")

    curr = src->select.sel_info.pnt_lst->head;
    new_tail = NULL;
    while(curr) {
        /* Create new point */
        if(NULL == (new_node = H5FL_MALLOC(H5S_pnt_node_t)))
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate point node")
        new_node->next = NULL;
        if(NULL == (new_node->pnt = (hsize_t *)H5MM_malloc(src->extent.rank * sizeof(hsize_t)))) {
            new_node = H5FL_FREE(H5S_pnt_node_t, new_node);
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate coordinate information")
        } /* end if */

        /* Copy over the point's coordinates */
        HDmemcpy(new_node->pnt, curr->pnt, (src->extent.rank * sizeof(hsize_t)));

        /* Keep the order the same when copying */
        if(NULL == new_tail)
            new_tail = dst->select.sel_info.pnt_lst->head = new_node;
        else {
            new_tail->next = new_node;
            new_tail = new_node;
        } /* end else */

        curr = curr->next;
    } /* end while */

done:
    if(ret_value < 0 && dst->select.sel_info.pnt_lst) {
        /* Traverse the (incomplete?) dst list, freeing all memory */
        curr = dst->select.sel_info.pnt_lst->head;
        while(curr) {
            H5S_pnt_node_t *tmp_node = curr;

            curr->pnt = (hsize_t *)H5MM_xfree(curr->pnt);
            curr = curr->next;
            tmp_node = H5FL_FREE(H5S_pnt_node_t, tmp_node);
        } /* end while */

        dst->select.sel_info.pnt_lst = H5FL_FREE(H5S_pnt_list_t, dst->select.sel_info.pnt_lst);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5S_point_copy() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_is_valid
 PURPOSE
    Check whether the selection fits within the extent, with the current
    offset defined.
 USAGE
    htri_t H5S_point_is_valid(space);
        H5S_t *space;             IN: Dataspace pointer to query
 RETURNS
    TRUE if the selection fits within the extent, FALSE if it does not and
        Negative on an error.
 DESCRIPTION
    Determines if the current selection at the current offset fits within the
    extent for the dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static htri_t
H5S_point_is_valid (const H5S_t *space)
{
    H5S_pnt_node_t *curr;      /* Point information nodes */
    unsigned u;                 /* Counter */
    htri_t ret_value = TRUE;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);

    /* Check each point to determine whether selection+offset is within extent */
    curr = space->select.sel_info.pnt_lst->head;
    while(curr != NULL) {
        /* Check each dimension */
        for(u = 0; u < space->extent.rank; u++) {
            /* Check if an offset has been defined */
            /* Bounds check the selected point + offset against the extent */
            if(((curr->pnt[u] + (hsize_t)space->select.offset[u]) > space->extent.size[u])
                    || (((hssize_t)curr->pnt[u] + space->select.offset[u]) < 0))
                HGOTO_DONE(FALSE)
        } /* end for */

        curr = curr->next;
    } /* end while */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5S_point_is_valid() */


/*--------------------------------------------------------------------------
 NAME
    H5Sget_select_elem_npoints
 PURPOSE
    Get the number of points in current element selection
 USAGE
    hssize_t H5Sget_select_elem_npoints(dsid)
        hid_t dsid;             IN: Dataspace ID of selection to query
 RETURNS
    The number of element points in selection on success, negative on failure
 DESCRIPTION
    Returns the number of element points in current selection for dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hssize_t
H5Sget_select_elem_npoints(hid_t spaceid)
{
    H5S_t *space;               /* Dataspace to modify selection of */
    hssize_t ret_value;         /* return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("Hs", "i", spaceid);

    /* Check args */
    if(NULL == (space = (H5S_t *)H5I_object_verify(spaceid, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace")
    if(H5S_GET_SELECT_TYPE(space) != H5S_SEL_POINTS)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an element selection")

    ret_value = (hssize_t)H5S_GET_SELECT_NPOINTS(space);

done:
    FUNC_LEAVE_API(ret_value)
}   /* H5Sget_select_elem_npoints() */

/*--------------------------------------------------------------------------
 NAME
    H5S_point_set_version
 PURPOSE
    Determine the version to use for encoding points selection info
 USAGE
    hssize_t H5S_point_set_version(space, bounds_end, f, version)
        const H5S_t *space;  IN: The dataspace
        hsize_t bounds_end:  IN: The selection high bounds
        H5F_t *f:            IN: The file pointer
        uint32_t *version:   OUT: The version to use for encoding
 RETURNS
    The version to use
 DESCRIPTION
    Determine the version to use for encoding points selection info:
        For 1.10, return 1

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_point_set_version(const H5S_t *space, hsize_t bounds_end[], H5F_t *f, uint32_t *version)
{
    hbool_t exceed = FALSE;
    unsigned u;
    herr_t ret_value = SUCCEED; /* return value */

    FUNC_ENTER_NOAPI_NOINIT

    *version = H5S_POINT_VERSION_1;

    /* Determine whether the number of points or the high bounds in the selection exceed (2^32 - 1) */
    for(u = 0; u < space->extent.rank; u++)
        if(bounds_end[u] > H5S_UINT32_MAX) {
            exceed = TRUE;
            break;
       }

    if(space->select.num_elem > H5S_UINT32_MAX)
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADVALUE, FAIL, "The number of points in point selection exceeds 2^32")
    else if(exceed)
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADVALUE, FAIL, "The end of bounding box in point selection exceeds 2^32")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5S_point_set_version() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_set_info_size
 PURPOSE
    Determine the size of point info to use for encoding selection info
 USAGE
    hssize_t H5S_point_set_info_size(space, bounds_end, version, point_size)
        const H5S_t *space:         IN: Dataspace ID of selection to query
        hsize_t bounds_end[]:       IN: The selection high bounds
        uint32_t version:           IN: The version used for encoding
        uint8_t *point_size:        OUT: The size of point info
 RETURNS
    The size of the points selection info
 DESCRIPTION
    Determine the size for encoding points selection info:
        For 1.10, return 4

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_point_set_info_size(const H5S_t *space, hsize_t H5_ATTR_UNUSED bounds_end[], uint32_t H5_ATTR_UNUSED version, uint8_t H5_ATTR_UNUSED *point_size)
{
    hsize_t max_size = 0;
    unsigned u;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(version == H5S_POINT_VERSION_1);

    *point_size = H5S_INFO_SIZE_4;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5S_point_set_info_size() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_serial_size
 PURPOSE
    Determine the number of bytes needed to store the serialized point selection
    information.
 USAGE
    hssize_t H5S_point_serial_size(space, f)
        H5S_t *space;             IN: Dataspace pointer to query
        H5F_t *f;                 IN: File pointer
 RETURNS
    The number of bytes required on success, negative on an error.
 DESCRIPTION
    Determines the number of bytes required to serialize the current point
    selection information for storage on disk.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static hssize_t
H5S_point_serial_size (const H5S_t *space, H5F_t *f)
{
    H5S_pnt_node_t *curr;       /* Point information nodes */
    hsize_t bounds_start[H5S_MAX_RANK];
    hsize_t bounds_end[H5S_MAX_RANK];
    uint32_t version;           /* Version number */
    uint8_t point_size;         /* Size of point info */
    hssize_t ret_value = -1;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(space);

    /* Get bounding box for the selection */
    HDmemset(bounds_end, 0, sizeof(bounds_end));
    if(H5S_point_bounds(space, bounds_start, bounds_end) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't get selection bounds")

    /* Determine the version */
    if(H5S_point_set_version(space, bounds_end, f, &version) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't determine hyper version")

    /* Determine the size of point info */
    if(H5S_point_set_info_size(space, bounds_end, version, &point_size) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't determine hyper version")

    HDassert(version == H5S_POINT_VERSION_1);
    HDassert(point_size == H5S_INFO_SIZE_4);

    /* Basic number of bytes required to serialize point selection: */
    /*
     *  <type (4 bytes)> + <version (4 bytes)> + <padding (4 bytes)> +
     *  <length (4 bytes)> + <rank (4 bytes)>
     */
    ret_value=20;

    /* <num points (depend on point_size)> */
    ret_value += point_size;

    /* Count points in selection */
    curr=space->select.sel_info.pnt_lst->head;
    while(curr!=NULL) {
        /* Add <point_size> bytes times the rank for each element selected */
        ret_value += point_size * space->extent.rank;
        curr = curr->next;
    } /* end while */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5S_point_serial_size() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_serialize
 PURPOSE
    Serialize the current selection into a user-provided buffer.
 USAGE
    herr_t H5S_point_serialize(space, p, f)
        const H5S_t *space;     IN: Dataspace with selection to serialize
        uint8_t **p;            OUT: Pointer to buffer to put serialized
                                selection.  Will be advanced to end of
                                serialized selection.
        H5F_t *f;               IN: File pointer
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Serializes the current element selection into a buffer.  (Primarily for
    storing on disk).
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_point_serialize (const H5S_t *space, uint8_t **p, H5F_t *f)
{
    H5S_pnt_node_t *curr;       /* Point information nodes */
    uint8_t *pp = (*p);         /* Local pointer for decoding */
    uint8_t *lenp;              /* pointer to length location for later storage */
    uint32_t len=0;             /* number of bytes used */
    unsigned u;                 /* local counting variable */
    uint32_t version;           /* Version number */
    uint8_t point_size;         /* Size of point info */
    hsize_t bounds_start[H5S_MAX_RANK];
    hsize_t bounds_end[H5S_MAX_RANK];
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check args */
    HDassert(space);
    HDassert(p);
    HDassert(pp);

    /* Get bounding box for the selection */
    HDmemset(bounds_end, 0, sizeof(bounds_end));
    if(H5S_point_bounds(space, bounds_start, bounds_end) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't get selection bounds")

    /* Determine the version */
    if(H5S_point_set_version(space, bounds_end, f, &version) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't determine hyper version")

    /* Determine the size of point info */
    if(H5S_point_set_info_size(space, bounds_end, version, &point_size) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't determine hyper version")

    HDassert(point_size == H5S_INFO_SIZE_4);
    HDassert(version == H5S_POINT_VERSION_1);

    /* Store the preamble information */
    UINT32ENCODE(pp, (uint32_t)H5S_GET_SELECT_TYPE(space));  /* Store the type of selection */
    UINT32ENCODE(pp, version);  /* Store the version number */

    UINT32ENCODE(pp, (uint32_t)0);  /* Store the un-used padding */
    lenp = pp;  /* Keep the pointer to the length location for later */
    pp += 4;    /* Skip over space for length */
    len += 8;   /* Add in advance # of bytes for num of dimensions and num elements  */

    /* Encode number of dimensions */
    UINT32ENCODE(pp, (uint32_t)space->extent.rank);


    /* Encode number of elements */
    UINT32ENCODE(pp, (uint32_t)space->select.num_elem);

    /* Encode each point in selection */
    curr=space->select.sel_info.pnt_lst->head;
    while(curr!=NULL) {
        /* Add 4 bytes times the rank for each element selected */
        len += 4 * space->extent.rank;

        /* Encode each point */
        for(u=0; u<space->extent.rank; u++)
            UINT32ENCODE(pp, (uint32_t)curr->pnt[u]);

        curr=curr->next;
    } /* end while */

    /* Encode length */
    UINT32ENCODE(lenp, (uint32_t)len);  /* Store the length of the extra information */

    /* Update encoding pointer */
    *p = pp;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_point_serialize() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_deserialize
 PURPOSE
    Deserialize the current selection from a user-provided buffer.
 USAGE
    herr_t H5S_point_deserialize(space, p)
        H5S_t *space;           IN/OUT: Dataspace pointer to place
                                selection into
        uint32_t version        IN: Selection version
        uint8_t flags           IN: Selection flags
        uint8 **p;              OUT: Pointer to buffer holding serialized
                                selection.  Will be advanced to end of
                                serialized selection.
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Deserializes the current selection into a buffer.  (Primarily for retrieving
    from disk).
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_point_deserialize(H5S_t *space, uint32_t H5_ATTR_UNUSED version, uint8_t H5_ATTR_UNUSED flags,
    const uint8_t **p)
{
    H5S_seloper_t op = H5S_SELECT_SET;  /* Selection operation */
    hsize_t *coord = NULL, *tcoord;     /* Pointer to array of elements */
    const uint8_t *pp = (*p);   /* Local pointer for decoding */
    hsize_t num_elem = 0;       /* Number of elements in selection */
    unsigned rank;              /* Rank of points */
    unsigned i, j;              /* local counting variables */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check args */
    HDassert(space);
    HDassert(p);
    HDassert(pp);

    /* Deserialize points to select */
    /* (The header and rank have already beed decoded) */
    rank = space->extent.rank;  /* Retrieve rank from space */
    UINT32DECODE(pp, num_elem); /* decode the number of points */

    /* Allocate space for the coordinates */
    if(NULL == (coord = (hsize_t *)H5MM_malloc(num_elem * rank * sizeof(hsize_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate coordinate information")

    /* Retrieve the coordinates from the buffer */
    for(tcoord = coord, i = 0; i < num_elem; i++)
        for(j = 0; j < (unsigned)rank; j++, tcoord++)
            UINT32DECODE(pp, *tcoord);

    /* Select points */
    if(H5S_select_elements(space, op, num_elem, (const hsize_t *)coord) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't change selection")

    /* Update decoding pointer */
    *p = pp;

done:
    /* Free the coordinate array if necessary */
    if(coord != NULL)
        H5MM_xfree(coord);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_point_deserialize() */


/*--------------------------------------------------------------------------
 NAME
    H5S_get_select_elem_pointlist
 PURPOSE
    Get the list of element points currently selected
 USAGE
    herr_t H5S_get_select_elem_pointlist(space, hsize_t *buf)
        H5S_t *space;           IN: Dataspace pointer of selection to query
        hsize_t startpoint;     IN: Element point to start with
        hsize_t numpoints;      IN: Number of element points to get
        hsize_t *buf;           OUT: List of element points selected
 RETURNS
    Non-negative on success, negative on failure
 DESCRIPTION
        Puts a list of the element points into the user's buffer.  The points
    start with the 'startpoint'th block in the list of points and put
    'numpoints' number of points into the user's buffer (or until the end of
    the list of points, whichever happen first)
        The point coordinates have the same dimensionality (rank) as the
    dataspace they are located within.  The list of points is formatted as
    follows: <coordinate> followed by the next coordinate, etc. until all the
    point information in the selection have been put into the user's buffer.
        The points are returned in the order they will be interated through
    when a selection is read/written from/to disk.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_get_select_elem_pointlist(H5S_t *space, hsize_t startpoint, hsize_t numpoints, hsize_t *buf)
{
    H5S_pnt_node_t *node;       /* Point node */
    unsigned rank;              /* Dataspace rank */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);
    HDassert(buf);

    /* Get the dataspace extent rank */
    rank = space->extent.rank;

    /* Get the head of the point list */
    node = space->select.sel_info.pnt_lst->head;

    /* Iterate to the first point to return */
    while(node != NULL && startpoint > 0) {
        startpoint--;
        node = node->next;
      } /* end while */

    /* Iterate through the node, copying each point's information */
    while(node != NULL && numpoints > 0) {
        HDmemcpy(buf, node->pnt, sizeof(hsize_t) * rank);
        buf += rank;
        numpoints--;
        node = node->next;
      } /* end while */

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_get_select_elem_pointlist() */


/*--------------------------------------------------------------------------
 NAME
    H5Sget_select_elem_pointlist
 PURPOSE
    Get the list of element points currently selected
 USAGE
    herr_t H5Sget_select_elem_pointlist(dsid, hsize_t *buf)
        hid_t dsid;             IN: Dataspace ID of selection to query
        hsize_t startpoint;     IN: Element point to start with
        hsize_t numpoints;      IN: Number of element points to get
        hsize_t buf[];          OUT: List of element points selected
 RETURNS
    Non-negative on success, negative on failure
 DESCRIPTION
        Puts a list of the element points into the user's buffer.  The points
    start with the 'startpoint'th block in the list of points and put
    'numpoints' number of points into the user's buffer (or until the end of
    the list of points, whichever happen first)
        The point coordinates have the same dimensionality (rank) as the
    dataspace they are located within.  The list of points is formatted as
    follows: <coordinate> followed by the next coordinate, etc. until all the
    point information in the selection have been put into the user's buffer.
        The points are returned in the order they will be interated through
    when a selection is read/written from/to disk.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Sget_select_elem_pointlist(hid_t spaceid, hsize_t startpoint,
    hsize_t numpoints, hsize_t buf[/*numpoints*/])
{
    H5S_t *space;               /* Dataspace to modify selection of */
    herr_t ret_value;           /* return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "ihh*[a2]h", spaceid, startpoint, numpoints, buf);

    /* Check args */
    if(NULL == buf)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid pointer")
    if(NULL == (space = (H5S_t *)H5I_object_verify(spaceid, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace")
    if(H5S_GET_SELECT_TYPE(space) != H5S_SEL_POINTS)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a point selection")

    ret_value = H5S_get_select_elem_pointlist(space, startpoint, numpoints, buf);

done:
    FUNC_LEAVE_API(ret_value)
}   /* H5Sget_select_elem_pointlist() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_bounds
 PURPOSE
    Gets the bounding box containing the selection.
 USAGE
    herr_t H5S_point_bounds(space, start, end)
        H5S_t *space;           IN: Dataspace pointer of selection to query
        hsize_t *start;         OUT: Starting coordinate of bounding box
        hsize_t *end;           OUT: Opposite coordinate of bounding box
 RETURNS
    Non-negative on success, negative on failure
 DESCRIPTION
    Retrieves the bounding box containing the current selection and places
    it into the user's buffers.  The start and end buffers must be large
    enough to hold the dataspace rank number of coordinates.  The bounding box
    exactly contains the selection, ie. if a 2-D element selection is currently
    defined with the following points: (4,5), (6,8) (10,7), the bounding box
    with be (4, 5), (10, 8).
        The bounding box calculations _does_ include the current offset of the
    selection within the dataspace extent.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_point_bounds(const H5S_t *space, hsize_t *start, hsize_t *end)
{
    H5S_pnt_node_t *node;       /* Point node */
    unsigned rank;              /* Dataspace rank */
    unsigned u;                 /* Local index variable */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(space);
    HDassert(start);
    HDassert(end);

    /* Get the dataspace extent rank */
    rank = space->extent.rank;

    /* Set the start and end arrays up */
    for(u = 0; u < rank; u++) {
        start[u] = HSIZET_MAX;
        end[u] = 0;
    } /* end for */

    /* Iterate through the node, checking the bounds on each element */
    node = space->select.sel_info.pnt_lst->head;
    while(node != NULL) {
        for(u = 0; u < rank; u++) {
            /* Check for offset moving selection negative */
            if(((hssize_t)node->pnt[u] + space->select.offset[u]) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "offset moves selection out of bounds")

            if(start[u] > (hsize_t)((hssize_t)node->pnt[u] + space->select.offset[u]))
                start[u] = (hsize_t)((hssize_t)node->pnt[u] + space->select.offset[u]);
            if(end[u] < (hsize_t)((hssize_t)node->pnt[u] + space->select.offset[u]))
                end[u] = (hsize_t)((hssize_t)node->pnt[u] + space->select.offset[u]);
        } /* end for */
        node = node->next;
      } /* end while */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_point_bounds() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_offset
 PURPOSE
    Gets the linear offset of the first element for the selection.
 USAGE
    herr_t H5S_point_offset(space, offset)
        const H5S_t *space;     IN: Dataspace pointer of selection to query
        hsize_t *offset;        OUT: Linear offset of first element in selection
 RETURNS
    Non-negative on success, negative on failure
 DESCRIPTION
    Retrieves the linear offset (in "units" of elements) of the first element
    selected within the dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    Calling this function on a "none" selection returns fail.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_point_offset(const H5S_t *space, hsize_t *offset)
{
    const hsize_t *pnt;         /* Pointer to a selected point's coordinates */
    const hssize_t *sel_offset; /* Pointer to the selection's offset */
    const hsize_t *dim_size;    /* Pointer to a dataspace's extent */
    hsize_t accum;              /* Accumulator for dimension sizes */
    int i;                      /* index variable */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(space);
    HDassert(offset);

    /* Start at linear offset 0 */
    *offset = 0;

    /* Set up pointers to arrays of values */
    pnt = space->select.sel_info.pnt_lst->head->pnt;
    sel_offset = space->select.offset;
    dim_size = space->extent.size;

    /* Loop through coordinates, calculating the linear offset */
    accum = 1;
    for(i = (int)space->extent.rank - 1; i >= 0; i--) {
        hssize_t pnt_offset = (hssize_t)pnt[i] + sel_offset[i]; /* Point's offset in this dimension */

        /* Check for offset moving selection out of the dataspace */
        if(pnt_offset < 0 || (hsize_t)pnt_offset >= dim_size[i])
            HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "offset moves selection out of bounds")

        /* Add the point's offset in this dimension to the total linear offset */
        *offset += (hsize_t)pnt_offset * accum;

        /* Increase the accumulator */
        accum *= dim_size[i];
    } /* end for */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_point_offset() */


/*--------------------------------------------------------------------------
 NAME
    H5S__point_unlim_dim
 PURPOSE
    Return unlimited dimension of selection, or -1 if none
 USAGE
    int H5S__point_unlim_dim(space)
        H5S_t *space;           IN: Dataspace pointer to check
 RETURNS
    Unlimited dimension of selection, or -1 if none (never fails).
 DESCRIPTION
    Returns the index of the unlimited dimension in this selection, or -1
    if the selection has no unlimited dimension.  Currently point
    selections cannot have an unlimited dimension, so this function always
    returns -1.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static int
H5S__point_unlim_dim(const H5S_t H5_ATTR_UNUSED *space)
{
    FUNC_ENTER_STATIC_NOERR

    FUNC_LEAVE_NOAPI(-1)
} /* end H5S__point_unlim_dim() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_is_contiguous
 PURPOSE
    Check if a point selection is contiguous within the dataspace extent.
 USAGE
    htri_t H5S_point_is_contiguous(space)
        H5S_t *space;           IN: Dataspace pointer to check
 RETURNS
    TRUE/FALSE/FAIL
 DESCRIPTION
    Checks to see if the current selection in the dataspace is contiguous.
    This is primarily used for reading the entire selection in one swoop.
    This code currently doesn't properly check for contiguousness when there is
    more than one point, as that would take a lot of extra coding that we
    don't need now.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static htri_t
H5S_point_is_contiguous(const H5S_t *space)
{
    htri_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);

    /* One point is definitely contiguous */
    if(space->select.num_elem==1)
    	ret_value=TRUE;
    else	/* More than one point might be contiguous, but it's complex to check and we don't need it right now */
    	ret_value=FALSE;

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_point_is_contiguous() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_is_single
 PURPOSE
    Check if a point selection is single within the dataspace extent.
 USAGE
    htri_t H5S_point_is_single(space)
        H5S_t *space;           IN: Dataspace pointer to check
 RETURNS
    TRUE/FALSE/FAIL
 DESCRIPTION
    Checks to see if the current selection in the dataspace is a single block.
    This is primarily used for reading the entire selection in one swoop.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static htri_t
H5S_point_is_single(const H5S_t *space)
{
    htri_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);

    /* One point is definitely 'single' :-) */
    if(space->select.num_elem==1)
    	ret_value=TRUE;
    else
    	ret_value=FALSE;

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_point_is_single() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_is_regular
 PURPOSE
    Check if a point selection is "regular"
 USAGE
    htri_t H5S_point_is_regular(space)
        const H5S_t *space;     IN: Dataspace pointer to check
 RETURNS
    TRUE/FALSE/FAIL
 DESCRIPTION
    Checks to see if the current selection in a dataspace is the a regular
    pattern.
    This is primarily used for reading the entire selection in one swoop.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    Doesn't check for points selected to be next to one another in a regular
    pattern yet.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static htri_t
H5S_point_is_regular(const H5S_t *space)
{
    htri_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);

    /* Only simple check for regular points for now... */
    if(space->select.num_elem==1)
    	ret_value=TRUE;
    else
    	ret_value=FALSE;

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_point_is_regular() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_adjust_u
 PURPOSE
    Adjust a "point" selection by subtracting an offset
 USAGE
    void H5S_point_adjust_u(space, offset)
        H5S_t *space;           IN/OUT: Pointer to dataspace to adjust
        const hsize_t *offset; IN: Offset to subtract
 RETURNS
    None
 DESCRIPTION
    Moves a point selection by subtracting an offset from it.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static void
H5S_point_adjust_u(H5S_t *space, const hsize_t *offset)
{
    H5S_pnt_node_t *node;               /* Point node */
    unsigned rank;                      /* Dataspace rank */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);
    HDassert(offset);

    /* Iterate through the nodes, checking the bounds on each element */
    node = space->select.sel_info.pnt_lst->head;
    rank = space->extent.rank;
    while(node) {
        unsigned u;                         /* Local index variable */

        /* Adjust each coordinate for point node */
        for(u = 0; u < rank; u++) {
            /* Check for offset moving selection negative */
            HDassert(node->pnt[u] >= offset[u]);

            /* Adjust node's coordinate location */
            node->pnt[u] -= offset[u];
        } /* end for */

        /* Advance to next point node in selection */
        node = node->next;
    } /* end while */

    FUNC_LEAVE_NOAPI_VOID
}   /* H5S_point_adjust_u() */


/*-------------------------------------------------------------------------
 * Function:	H5S_point_project_scalar
 *
 * Purpose:	Projects a single element point selection into a scalar
 *              dataspace
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Programmer:	Quincey Koziol
 *              Sunday, July 18, 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_point_project_scalar(const H5S_t *space, hsize_t *offset)
{
    const H5S_pnt_node_t *node;         /* Point node */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check args */
    HDassert(space && H5S_SEL_POINTS == H5S_GET_SELECT_TYPE(space));
    HDassert(offset);

    /* Get the head of the point list */
    node = space->select.sel_info.pnt_lst->head;

    /* Check for more than one point selected */
    if(node->next)
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "point selection of one element has more than one node!")

    /* Calculate offset of selection in projected buffer */
    *offset = H5VM_array_offset(space->extent.rank, space->extent.size, node->pnt); 

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_point_project_scalar() */


/*-------------------------------------------------------------------------
 * Function:	H5S_point_project_simple
 *
 * Purpose:	Projects a point selection onto/into a simple dataspace
 *              of a different rank
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Programmer:	Quincey Koziol
 *              Sunday, July 18, 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_point_project_simple(const H5S_t *base_space, H5S_t *new_space, hsize_t *offset)
{
    const H5S_pnt_node_t *base_node;    /* Point node in base space */
    H5S_pnt_node_t *new_node;           /* Point node in new space */
    H5S_pnt_node_t *prev_node;          /* Previous point node in new space */
    unsigned rank_diff;                 /* Difference in ranks between spaces */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check args */
    HDassert(base_space && H5S_SEL_POINTS == H5S_GET_SELECT_TYPE(base_space));
    HDassert(new_space);
    HDassert(offset);

    /* We are setting a new selection, remove any current selection in new dataspace */
    if(H5S_SELECT_RELEASE(new_space) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't release selection")

    /* Allocate room for the head of the point list */
    if(NULL == (new_space->select.sel_info.pnt_lst = H5FL_MALLOC(H5S_pnt_list_t)))
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate point list node")

    /* Check if the new space's rank is < or > base space's rank */
    if(new_space->extent.rank < base_space->extent.rank) {
        hsize_t block[H5S_MAX_RANK];     /* Block selected in base dataspace */

        /* Compute the difference in ranks */
        rank_diff = base_space->extent.rank - new_space->extent.rank;

        /* Calculate offset of selection in projected buffer */
        HDmemset(block, 0, sizeof(block));
        HDmemcpy(block, base_space->select.sel_info.pnt_lst->head->pnt, sizeof(hsize_t) * rank_diff);
        *offset = H5VM_array_offset(base_space->extent.rank, base_space->extent.size, block); 

        /* Iterate through base space's point nodes, copying the point information */
        base_node = base_space->select.sel_info.pnt_lst->head;
        prev_node = NULL;
        while(base_node) {
            /* Create new point */
            if(NULL == (new_node = H5FL_MALLOC(H5S_pnt_node_t)))
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate point node")
            new_node->next = NULL;
            if(NULL == (new_node->pnt = (hsize_t *)H5MM_malloc(new_space->extent.rank * sizeof(hsize_t)))) {
                new_node = H5FL_FREE(H5S_pnt_node_t, new_node);
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate coordinate information")
            } /* end if */

            /* Copy over the point's coordinates */
            HDmemcpy(new_node->pnt, &base_node->pnt[rank_diff], (new_space->extent.rank * sizeof(hsize_t)));

            /* Keep the order the same when copying */
            if(NULL == prev_node)
                prev_node = new_space->select.sel_info.pnt_lst->head = new_node;
            else {
                prev_node->next = new_node;
                prev_node = new_node;
            } /* end else */

            /* Advance to next node */
            base_node = base_node->next;
        } /* end while */
    } /* end if */
    else {
        HDassert(new_space->extent.rank > base_space->extent.rank);

        /* Compute the difference in ranks */
        rank_diff = new_space->extent.rank - base_space->extent.rank;

        /* The offset is zero when projected into higher dimensions */
        *offset = 0;

        /* Iterate through base space's point nodes, copying the point information */
        base_node = base_space->select.sel_info.pnt_lst->head;
        prev_node = NULL;
        while(base_node) {
            /* Create new point */
            if(NULL == (new_node = H5FL_MALLOC(H5S_pnt_node_t)))
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate point node")
            new_node->next = NULL;
            if(NULL == (new_node->pnt = (hsize_t *)H5MM_malloc(new_space->extent.rank * sizeof(hsize_t)))) {
                new_node = H5FL_FREE(H5S_pnt_node_t, new_node);
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate coordinate information")
            } /* end if */

            /* Copy over the point's coordinates */
            HDmemset(new_node->pnt, 0, sizeof(hsize_t) * rank_diff);
            HDmemcpy(&new_node->pnt[rank_diff], base_node->pnt, (new_space->extent.rank * sizeof(hsize_t)));

            /* Keep the order the same when copying */
            if(NULL == prev_node)
                prev_node = new_space->select.sel_info.pnt_lst->head = new_node;
            else {
                prev_node->next = new_node;
                prev_node = new_node;
            } /* end else */

            /* Advance to next node */
            base_node = base_node->next;
        } /* end while */
    } /* end else */

    /* Number of elements selected will be the same */
    new_space->select.num_elem = base_space->select.num_elem;

    /* Set selection type */
    new_space->select.type = H5S_sel_point;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_point_project_simple() */


/*--------------------------------------------------------------------------
 NAME
    H5Sselect_elements
 PURPOSE
    Specify a series of elements in the dataspace to select
 USAGE
    herr_t H5Sselect_elements(dsid, op, num_elem, coord)
        hid_t dsid;             IN: Dataspace ID of selection to modify
        H5S_seloper_t op;       IN: Operation to perform on current selection
        size_t num_elem;        IN: Number of elements in COORD array.
        const hsize_t *coord;   IN: The location of each element selected
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    This function selects array elements to be included in the selection for
    the dataspace.  The COORD array is a 2-D array of size <dataspace rank>
    by NUM_ELEM (ie. a list of coordinates in the dataspace).  The order of
    the element coordinates in the COORD array specifies the order that the
    array elements are iterated through when I/O is performed.  Duplicate
    coordinates are not checked for.  The selection operator, OP, determines
    how the new selection is to be combined with the existing selection for
    the dataspace.  Currently, only H5S_SELECT_SET is supported, which replaces
    the existing selection with the one defined in this call.  When operators
    other than H5S_SELECT_SET are used to combine a new selection with an
    existing selection, the selection ordering is reset to 'C' array ordering.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Sselect_elements(hid_t spaceid, H5S_seloper_t op, size_t num_elem,
    const hsize_t *coord)
{
    H5S_t    *space;               /* Dataspace to modify selection of */
    herr_t   ret_value;            /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "iSsz*h", spaceid, op, num_elem, coord);

    /* Check args */
    if(NULL == (space = (H5S_t *)H5I_object_verify(spaceid, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace")
    if(H5S_SCALAR == H5S_GET_EXTENT_TYPE(space))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "point doesn't support H5S_SCALAR space")
    if(H5S_NULL == H5S_GET_EXTENT_TYPE(space))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "point doesn't support H5S_NULL space")
    if(coord == NULL || num_elem == 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "elements not specified")
    if(!(op == H5S_SELECT_SET || op == H5S_SELECT_APPEND || op == H5S_SELECT_PREPEND))
        HGOTO_ERROR(H5E_ARGS, H5E_UNSUPPORTED, FAIL, "unsupported operation attempted")

    /* Call the real element selection routine */
    if((ret_value = H5S_select_elements(space, op, (hsize_t)num_elem, coord)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't select elements")

done:
    FUNC_LEAVE_API(ret_value)
}   /* H5Sselect_elements() */


/*--------------------------------------------------------------------------
 NAME
    H5S_point_get_seq_list
 PURPOSE
    Create a list of offsets & lengths for a selection
 USAGE
    herr_t H5S_point_get_seq_list(space,flags,iter,maxseq,maxelem,nseq,nelem,off,len)
        H5S_t *space;           IN: Dataspace containing selection to use.
        unsigned flags;         IN: Flags for extra information about operation
        H5S_sel_iter_t *iter;   IN/OUT: Selection iterator describing last
                                    position of interest in selection.
        size_t maxseq;          IN: Maximum number of sequences to generate
        size_t maxelem;         IN: Maximum number of elements to include in the
                                    generated sequences
        size_t *nseq;           OUT: Actual number of sequences generated
        size_t *nelem;          OUT: Actual number of elements in sequences generated
        hsize_t *off;           OUT: Array of offsets
        size_t *len;            OUT: Array of lengths
 RETURNS
    Non-negative on success/Negative on failure.
 DESCRIPTION
    Use the selection in the dataspace to generate a list of byte offsets and
    lengths for the region(s) selected.  Start/Restart from the position in the
    ITER parameter.  The number of sequences generated is limited by the MAXSEQ
    parameter and the number of sequences actually generated is stored in the
    NSEQ parameter.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_point_get_seq_list(const H5S_t *space, unsigned flags, H5S_sel_iter_t *iter,
    size_t maxseq, size_t maxelem, size_t *nseq, size_t *nelem,
    hsize_t *off, size_t *len)
{
    size_t io_left;             /* The number of bytes left in the selection */
    size_t start_io_left;       /* The initial number of bytes left in the selection */
    H5S_pnt_node_t *node;       /* Point node */
    hsize_t dims[H5O_LAYOUT_NDIMS];     /* Total size of memory buf */
    int	ndims;                  /* Dimensionality of space*/
    hsize_t	acc;            /* Coordinate accumulator */
    hsize_t	loc;            /* Coordinate offset */
    size_t      curr_seq;       /* Current sequence being operated on */
    int         i;              /* Local index variable */
    herr_t ret_value=SUCCEED;      /* return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check args */
    HDassert(space);
    HDassert(iter);
    HDassert(maxseq > 0);
    HDassert(maxelem > 0);
    HDassert(nseq);
    HDassert(nelem);
    HDassert(off);
    HDassert(len);

    /* Choose the minimum number of bytes to sequence through */
    H5_CHECK_OVERFLOW(iter->elmt_left, hsize_t, size_t);
    start_io_left = io_left = (size_t)MIN(iter->elmt_left, maxelem);

    /* Get the dataspace dimensions */
    if((ndims = H5S_get_simple_extent_dims (space, dims, NULL)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to retrieve dataspace dimensions")

    /* Walk through the points in the selection, starting at the current */
    /*  location in the iterator */
    node = iter->u.pnt.curr;
    curr_seq = 0;
    while(NULL != node) {
        /* Compute the offset of each selected point in the buffer */
        for(i = ndims - 1, acc = iter->elmt_size, loc = 0; i >= 0; i--) {
            loc += (hsize_t)((hssize_t)node->pnt[i] + space->select.offset[i]) * acc;
            acc *= dims[i];
        } /* end for */

        /* Check if this is a later point in the selection */
        if(curr_seq>0) {
            /* If a sorted sequence is requested, make certain we don't go backwards in the offset */
            if((flags&H5S_GET_SEQ_LIST_SORTED) && loc<off[curr_seq-1])
                break;

            /* Check if this point extends the previous sequence */
            /* (Unlikely, but possible) */
            if(loc==(off[curr_seq-1]+len[curr_seq-1])) {
                /* Extend the previous sequence */
                len[curr_seq-1]+=iter->elmt_size;
            } /* end if */
            else {
                /* Add a new sequence */
                off[curr_seq]=loc;
                len[curr_seq]=iter->elmt_size;

                /* Increment sequence count */
                curr_seq++;
            } /* end else */
        } /* end if */
        else {
            /* Add a new sequence */
            off[curr_seq]=loc;
            len[curr_seq]=iter->elmt_size;

            /* Increment sequence count */
            curr_seq++;
        } /* end else */

        /* Decrement number of elements left to process */
        io_left--;

        /* Move the iterator */
        iter->u.pnt.curr=node->next;
        iter->elmt_left--;

        /* Check if we're finished with all sequences */
        if(curr_seq==maxseq)
            break;

        /* Check if we're finished with all the elements available */
        if(io_left==0)
            break;

        /* Advance to the next point */
        node=node->next;
      } /* end while */

    /* Set the number of sequences generated */
    *nseq=curr_seq;

    /* Set the number of elements used */
    *nelem=start_io_left-io_left;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5S_point_get_seq_list() */

