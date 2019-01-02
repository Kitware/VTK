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

/* Programmer:  Quincey Koziol <koziol@ncsa.uiuc.ued>
 *              Friday, May 29, 1998
 *
 * Purpose:	Dataspace selection functions.
 */

#include "H5Smodule.h"          /* This source code file is part of the H5S module */


#include "H5private.h"		/* Generic Functions			*/
#include "H5Dprivate.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free Lists                           */
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"        /* Memory management                    */
#include "H5Spkg.h"		/* Dataspaces 				*/
#include "H5VMprivate.h"		/* Vector and array functions		*/
#include "H5WBprivate.h"        /* Wrapped Buffers                      */

/* Local functions */
#ifdef LATER
static herr_t H5S_select_iter_block(const H5S_sel_iter_t *iter, hsize_t *start, hsize_t *end);
static htri_t H5S_select_iter_has_next_block(const H5S_sel_iter_t *iter);
static herr_t H5S_select_iter_next_block(H5S_sel_iter_t *iter);
#endif /* LATER */

/* Declare a free list to manage the H5S_sel_iter_t struct */
H5FL_DEFINE(H5S_sel_iter_t);

/* Declare extern free list to manage sequences of size_t */
H5FL_SEQ_EXTERN(size_t);

/* Declare extern free list to manage sequences of hsize_t */
H5FL_SEQ_EXTERN(hsize_t);



/*--------------------------------------------------------------------------
 NAME
    H5S_select_offset
 PURPOSE
    Set the selection offset for a datapace
 USAGE
    herr_t H5S_select_offset(space, offset)
        H5S_t *space;	        IN/OUT: Dataspace object to set selection offset
        const hssize_t *offset; IN: Offset to position the selection at
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Sets the selection offset for the dataspace
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    Only works for simple dataspaces currently
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_offset(H5S_t *space, const hssize_t *offset)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);
    HDassert(0 < space->extent.rank && space->extent.rank <= H5S_MAX_RANK);
    HDassert(offset);

    /* Copy the offset over */
    HDmemcpy(space->select.offset, offset, sizeof(hssize_t)*space->extent.rank);

    /* Indicate that the offset was changed */
    space->select.offset_changed = TRUE;

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_select_offset() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_copy
 PURPOSE
    Copy a selection from one dataspace to another
 USAGE
    herr_t H5S_select_copy(dst, src)
        H5S_t *dst;  OUT: Pointer to the destination dataspace
        H5S_t *src;  IN: Pointer to the source dataspace
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Copies all the selection information (include offset) from the source
    dataspace to the destination dataspace.

    If the SHARE_SELECTION flag is set, then the selection can be shared
    between the source and destination dataspaces.  (This should only occur in
    situations where the destination dataspace will immediately change to a new
    selection)
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_copy(H5S_t *dst, const H5S_t *src, hbool_t share_selection)
{
    herr_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(dst);
    HDassert(src);

    /* Copy regular fields */
    dst->select = src->select;

    /* Perform correct type of copy based on the type of selection */
    if((ret_value = (*src->select.type->copy)(dst,src,share_selection)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "can't copy selection specific information")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_copy() */


/*-------------------------------------------------------------------------
 * Function:	H5S_select_release
 *
 * Purpose:	Releases all memory associated with a dataspace selection.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Friday, May 30, 2003
 *
 * Note: This routine participates in the "Inlining C function pointers"
 *      pattern, don't call it directly, use the appropriate macro
 *      defined in H5Sprivate.h.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_select_release(H5S_t *ds)
{
    herr_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(ds);

    /* Call the selection type's release function */
    if((ds->select.type) && ((ret_value = (*ds->select.type->release)(ds)) < 0))
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5S_select_release() */


/*-------------------------------------------------------------------------
 * Function:	H5S_select_get_seq_list
 *
 * Purpose:	Retrieves the next sequence of offset/length pairs for an
 *              iterator on a dataspace
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, May 18, 2004
 *
 * Note: This routine participates in the "Inlining C function pointers"
 *      pattern, don't call it directly, use the appropriate macro
 *      defined in H5Sprivate.h.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_select_get_seq_list(const H5S_t *space, unsigned flags,
    H5S_sel_iter_t *iter, size_t maxseq, size_t maxbytes,
    size_t *nseq, size_t *nbytes, hsize_t *off, size_t *len)
{
    herr_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(space);

    /* Call the selection type's get_seq_list function */
    if((ret_value = (*space->select.type->get_seq_list)(space, flags, iter, maxseq, maxbytes, nseq, nbytes, off, len)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "unable to get selection sequence list")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5S_select_get_seq_list() */


/*-------------------------------------------------------------------------
 * Function:	H5S_select_serial_size
 *
 * Purpose:	Determines the number of bytes required to store the current
 *              selection
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, May 18, 2004
 *
 * Note: This routine participates in the "Inlining C function pointers"
 *      pattern, don't call it directly, use the appropriate macro
 *      defined in H5Sprivate.h.
 *
 *-------------------------------------------------------------------------
 */
hssize_t
H5S_select_serial_size(const H5S_t *space, H5F_t *f)
{
    hssize_t ret_value = -1;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);

    /* Call the selection type's serial_size function */
    ret_value=(*space->select.type->serial_size)(space, f);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5S_select_serial_size() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_serialize
 PURPOSE
    Serialize the selection for a dataspace into a buffer
 USAGE
    herr_t H5S_select_serialize(space, p)
        const H5S_t *space;     IN: Dataspace with selection to serialize
        uint8_t **p;            OUT: Pointer to buffer to put serialized
                                selection.  Will be advanced to end of
                                serialized selection.
        H5F_t *f;               IN: File pointer
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Calls the appropriate dataspace selection callback to serialize the
    current selection into a buffer.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This routine participates in the "Inlining C function pointers"
        pattern, don't call it directly, use the appropriate macro
        defined in H5Sprivate.h.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_serialize(const H5S_t *space, uint8_t **p, H5F_t *f)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);
    HDassert(p);

    /* Call the selection type's serialize function */
    ret_value=(*space->select.type->serialize)(space,p,f);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5S_select_serialize() */


/*--------------------------------------------------------------------------
 NAME
    H5Sget_select_npoints
 PURPOSE
    Get the number of elements in current selection
 USAGE
    hssize_t H5Sget_select_npoints(dsid)
        hid_t dsid;             IN: Dataspace ID of selection to query
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Returns the number of elements in current selection for dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hssize_t
H5Sget_select_npoints(hid_t spaceid)
{
    H5S_t *space;               /* Dataspace to modify selection of */
    hssize_t ret_value;         /* return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("Hs", "i", spaceid);

    /* Check args */
    if(NULL == (space = (H5S_t *)H5I_object_verify(spaceid, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace")

    ret_value = (hssize_t)H5S_GET_SELECT_NPOINTS(space);

done:
    FUNC_LEAVE_API(ret_value)
}   /* H5Sget_select_npoints() */


/*--------------------------------------------------------------------------
 NAME
    H5S_get_select_npoints
 PURPOSE
    Get the number of elements in current selection
 USAGE
    hssize_t H5Sget_select_npoints(space)
        H5S_t *space;             IN: Dataspace of selection to query
 RETURNS
    The number of elements in selection on success, 0 on failure
 DESCRIPTION
    Returns the number of elements in current selection for dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
     This routine participates in the "Inlining C function pointers"
        pattern, don't call it directly, use the appropriate macro
        defined in H5Sprivate.h.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hssize_t
H5S_get_select_npoints(const H5S_t *space)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);

    FUNC_LEAVE_NOAPI((hssize_t)space->select.num_elem)
}   /* H5S_get_select_npoints() */


/*--------------------------------------------------------------------------
 NAME
    H5Sselect_valid
 PURPOSE
    Check whether the selection fits within the extent, with the current
    offset defined.
 USAGE
    htri_t H5Sselect_void(dsid)
        hid_t dsid;             IN: Dataspace ID to query
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
    Christian Chilan 01/17/2007
    Changed the error return value from 0 to FAIL.
--------------------------------------------------------------------------*/
htri_t
H5Sselect_valid(hid_t spaceid)
{
    H5S_t *space;       /* Dataspace to modify selection of */
    htri_t ret_value;   /* return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("t", "i", spaceid);

    /* Check args */
    if(NULL == (space = (H5S_t *)H5I_object_verify(spaceid, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace")

    ret_value = H5S_SELECT_VALID(space);

done:
    FUNC_LEAVE_API(ret_value)
}   /* H5Sselect_valid() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_valid
 PURPOSE
    Check whether the selection fits within the extent, with the current
    offset defined.
 USAGE
    htri_t H5S_select_void(space)
        H5S_t *space;           IN: Dataspace to query
 RETURNS
    TRUE if the selection fits within the extent, FALSE if it does not and
        Negative on an error.
 DESCRIPTION
    Determines if the current selection at the current offset fits within the
    extent for the dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
     This routine participates in the "Inlining C function pointers"
        pattern, don't call it directly, use the appropriate macro
        defined in H5Sprivate.h.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5S_select_valid(const H5S_t *space)
{
    htri_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);

    ret_value = (*space->select.type->is_valid)(space);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_valid() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_deserialize
 PURPOSE
    Deserialize the current selection from a user-provided buffer into a real
        selection in the dataspace.
 USAGE
    herr_t H5S_select_deserialize(space, p)
        H5S_t **space;          IN/OUT: Dataspace pointer to place
                                selection into.  Will be allocated if not
                                provided.
        uint8 **p;              OUT: Pointer to buffer holding serialized
                                selection.  Will be advanced to end of
                                serialized selection.
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Deserializes the current selection into a buffer.  (Primarily for retrieving
    from disk).  This routine just hands off to the appropriate routine for each
    type of selection.  The format of the serialized information is shown in
    the H5S_select_serialize() header.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_deserialize(H5S_t **space, const uint8_t **p)
{
    H5S_t *tmp_space = NULL;    /* Pointer to actual dataspace to use, either
                                 *space or a newly allocated one */
    uint32_t sel_type;          /* Pointer to the selection type */
    uint32_t version;           /* Version number */
    uint8_t flags = 0;          /* Flags */
    herr_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(space);

    /* Allocate space if not provided */
    if(!*space) {
        if(NULL == (tmp_space = H5S_create(H5S_SIMPLE)))
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCREATE, FAIL, "can't create dataspace")
    } /* end if */
    else
        tmp_space = *space;

    /* Decode selection type */
    UINT32DECODE(*p, sel_type);

    /* Decode version */
    UINT32DECODE(*p, version);

    if(version >= (uint32_t)2) {
        /* Decode flags */
        flags = *(*p)++;

        /* Check for unknown flags */
        if(flags & ~H5S_SELECT_FLAG_BITS)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTLOAD, FAIL, "unknown flag for selection")

        /* Skip over the remainder of the header */
        *p += 4;
    } /* end if */
    else
        /* Skip over the remainder of the header */
        *p += 8;

    /* Decode and check or patch rank for point and hyperslab selections */
    if((sel_type == H5S_SEL_POINTS) || (sel_type == H5S_SEL_HYPERSLABS)) {
        uint32_t rank;              /* Rank of dataspace */

        /* Decode the rank of the point selection */
        UINT32DECODE(*p,rank);

        if(!*space) {
            hsize_t dims[H5S_MAX_RANK];

            /* Patch the rank of the allocated dataspace */
            (void)HDmemset(dims, 0, (size_t)rank * sizeof(dims[0]));
            if(H5S_set_extent_simple(tmp_space, rank, dims, NULL) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "can't set dimensions")
        } /* end if */
        else
            /* Verify the rank of the provided dataspace */
            if(rank != tmp_space->extent.rank)
                HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "rank of serialized selection does not match dataspace")
    } /* end if */

    /* Make routine for selection type */
    switch(sel_type) {
        case H5S_SEL_POINTS:         /* Sequence of points selected */
            ret_value = (*H5S_sel_point->deserialize)(tmp_space, version, flags, p);
            break;

        case H5S_SEL_HYPERSLABS:     /* Hyperslab selection defined */
            ret_value = (*H5S_sel_hyper->deserialize)(tmp_space, version, flags, p);
            break;

        case H5S_SEL_ALL:            /* Entire extent selected */
            ret_value = (*H5S_sel_all->deserialize)(tmp_space, version, flags, p);
            break;

        case H5S_SEL_NONE:           /* Nothing selected */
            ret_value = (*H5S_sel_none->deserialize)(tmp_space, version, flags, p);
            break;

        default:
            break;
    }
    if(ret_value < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTLOAD, FAIL, "can't deserialize selection")

    /* Return space to the caller if allocated */
    if(!*space)
        *space = tmp_space;

done:
    /* Free temporary space if not passed to caller (only happens on error) */
    if(!*space && tmp_space)
        if(H5S_close(tmp_space) < 0)
            HDONE_ERROR(H5E_DATASPACE, H5E_CANTFREE, FAIL, "can't close dataspace")

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_deserialize() */


/*--------------------------------------------------------------------------
 NAME
    H5Sget_select_bounds
 PURPOSE
    Gets the bounding box containing the selection.
 USAGE
    herr_t H5S_get_select_bounds(space, start, end)
        hid_t dsid;             IN: Dataspace ID of selection to query
        hsize_t start[];        OUT: Starting coordinate of bounding box
        hsize_t end[];          OUT: Opposite coordinate of bounding box
 RETURNS
    Non-negative on success, negative on failure
 DESCRIPTION
    Retrieves the bounding box containing the current selection and places
    it into the user's buffers.  The start and end buffers must be large
    enough to hold the dataspace rank number of coordinates.  The bounding box
    exactly contains the selection, ie. if a 2-D element selection is currently
    defined with the following points: (4,5), (6,8) (10,7), the bounding box
    with be (4, 5), (10, 8).  Calling this function on a "none" selection
    returns fail.
        The bounding box calculations _does_ include the current offset of the
    selection within the dataspace extent.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This routine participates in the "Inlining C function pointers"
        pattern, don't call it directly, use the appropriate macro
        defined in H5Sprivate.h.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Sget_select_bounds(hid_t spaceid, hsize_t start[], hsize_t end[])
{
    H5S_t *space;       /* Dataspace to modify selection of */
    herr_t ret_value;   /* return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "i*h*h", spaceid, start, end);

    /* Check args */
    if(start == NULL || end == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid pointer")
    if(NULL == (space = (H5S_t *)H5I_object_verify(spaceid, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace")

    ret_value = H5S_SELECT_BOUNDS(space, start, end);

done:
    FUNC_LEAVE_API(ret_value)
}   /* H5Sget_select_bounds() */


/*--------------------------------------------------------------------------
 NAME
    H5S_get_select_bounds
 PURPOSE
    Gets the bounding box containing the selection.
 USAGE
    herr_t H5S_get_select_bounds(space, start, end)
        H5S_t *space;           IN: Dataspace ID of selection to query
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
    with be (4, 5), (10, 8).  Calling this function on a "none" selection
    returns fail.
        The bounding box calculations _does_ include the current offset of the
    selection within the dataspace extent.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_get_select_bounds(const H5S_t *space, hsize_t *start, hsize_t *end)
{
    herr_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);
    HDassert(start);
    HDassert(end);

    ret_value = (*space->select.type->bounds)(space,start,end);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_get_select_bounds() */


/*--------------------------------------------------------------------------
 NAME
    H5S_get_select_offset
 PURPOSE
    Gets the linear offset of the first element for the selection.
 USAGE
    herr_t H5S_get_select_offset(space, offset)
        const H5S_t *space;     IN: Dataspace pointer of selection to query
        hsize_t *offset;        OUT: Linear offset of first element in selection
 RETURNS
    Non-negative on success, negative on failure
 DESCRIPTION
    Retrieves the linear offset (in "units" of elements) of the first element
    selected within the dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
        The offset calculation _does_ include the current offset of the
    selection within the dataspace extent.

        Calling this function on a "none" selection returns fail.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_get_select_offset(const H5S_t *space, hsize_t *offset)
{
    herr_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);
    HDassert(offset);

    ret_value = (*space->select.type->offset)(space, offset);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_get_select_offset() */


/*--------------------------------------------------------------------------
 NAME
    H5S_get_select_unlim_dim
 PURPOSE
    Gets the unlimited dimension in the selection, or -1 if there is no
    unlimited dimension.
 USAGE
    int H5S_get_select_unlim_dim(space)
        const H5S_t *space;     IN: Dataspace pointer of selection to query
 RETURNS
    Unlimited dimension in the selection, or -1 if there is no unlimited
    dimension (never fails)
 DESCRIPTION
    Gets the unlimited dimension in the selection, or -1 if there is no
    unlimited dimension.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
        Currently only implemented for hyperslab selections, all others
        simply return -1.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5S_get_select_unlim_dim(const H5S_t *space)
{
    herr_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);

    ret_value = (*space->select.type->unlim_dim)(space);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_get_select_unlim_dim() */


/*--------------------------------------------------------------------------
 NAME
    H5S_get_select_num_elem_non_unlim
 PURPOSE
    Gets the number of elements in the non-unlimited dimensions
 USAGE
    herr_t H5S_get_select_num_elem_non_unlim(space,num_elem_non_unlim)
        H5S_t *space;           IN: Dataspace pointer to check
        hsize_t *num_elem_non_unlim; OUT: Number of elements in the non-unlimited dimensions
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Returns the number of elements in a slice through the non-unlimited
    dimensions of the selection.  Fails if the selection has no unlimited
    dimension.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_get_select_num_elem_non_unlim(const H5S_t *space,
    hsize_t *num_elem_non_unlim)
{
    herr_t ret_value = SUCCEED; /* return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(space);
    HDassert(num_elem_non_unlim);

    /* Check for selection callback */
    if(!space->select.type->num_elem_non_unlim)
        HGOTO_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, FAIL, "selection type has no num_elem_non_unlim callback")

    /* Make selection callback */
    if((*space->select.type->num_elem_non_unlim)(space, num_elem_non_unlim) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOUNT, FAIL, "can't get number of elements in non-unlimited dimension")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_get_select_unlim_dim() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_is_contiguous
 PURPOSE
    Determines if a selection is contiguous in the dataspace
 USAGE
    htri_t H5S_select_is_contiguous(space)
        const H5S_t *space;             IN: Dataspace of selection to query
 RETURNS
    Non-negative (TRUE/FALSE) on success, negative on failure
 DESCRIPTION
    Checks the selection to determine if the points to iterated over will be
    contiguous in the particular dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This routine participates in the "Inlining C function pointers"
        pattern, don't call it directly, use the appropriate macro
        defined in H5Sprivate.h.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5S_select_is_contiguous(const H5S_t *space)
{
    herr_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);

    ret_value = (*space->select.type->is_contiguous)(space);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_is_contiguous() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_is_single
 PURPOSE
    Determines if a selection is a single block in the dataspace
 USAGE
    htri_t H5S_select_is_single(space)
        const H5S_t *space;             IN: Dataspace of selection to query
 RETURNS
    Non-negative (TRUE/FALSE) on success, negative on failure
 DESCRIPTION
    Checks the selection to determine if it occupies a single block in the
    particular dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This routine participates in the "Inlining C function pointers"
        pattern, don't call it directly, use the appropriate macro
        defined in H5Sprivate.h.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5S_select_is_single(const H5S_t *space)
{
    herr_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);

    ret_value = (*space->select.type->is_single)(space);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_is_single() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_is_regular
 PURPOSE
    Determines if a selection is "regular"  in the dataspace
 USAGE
    htri_t H5S_select_is_regular(space)
        const H5S_t *space;             IN: Dataspace of selection to query
 RETURNS
    Non-negative (TRUE/FALSE) on success, negative on failure
 DESCRIPTION
    Checks the selection to determine if it is "regular" (i.e. a single
    block or a strided pattern) in the particular dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This routine participates in the "Inlining C function pointers"
        pattern, don't call it directly, use the appropriate macro
        defined in H5Sprivate.h.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5S_select_is_regular(const H5S_t *space)
{
    herr_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);

    ret_value = (*space->select.type->is_regular)(space);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_is_regular() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_adjust_u
 PURPOSE
    Adjust a selection by subtracting an offset
 USAGE
    void H5S_select_adjust_u(space, offset)
        H5S_t *space;           IN/OUT: Pointer to dataspace to adjust
        const hsize_t *offset; IN: Offset to subtract
 RETURNS
    None
 DESCRIPTION
    Moves a selection by subtracting an offset from it.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This routine participates in the "Inlining C function pointers"
        pattern, don't call it directly, use the appropriate macro
        defined in H5Sprivate.h.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
void
H5S_select_adjust_u(H5S_t *space, const hsize_t *offset)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);
    HDassert(offset);

    (*space->select.type->adjust_u)(space, offset);

    FUNC_LEAVE_NOAPI_VOID
}   /* H5S_select_adjust_u() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_project_scalar
 PURPOSE
    Project a single element selection for a scalar dataspace
 USAGE
    herr_t H5S_select_project_scalar(space, offset)
        const H5S_t *space;             IN: Pointer to dataspace to project
        hsize_t *offset;                IN/OUT: Offset of projected point
 RETURNS
    Non-negative on success, negative on failure
 DESCRIPTION
    Projects a selection of a single element into a scalar dataspace, computing
    the offset of the element in the original selection.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This routine participates in the "Inlining C function pointers"
        pattern, don't call it directly, use the appropriate macro
        defined in H5Sprivate.h.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_project_scalar(const H5S_t *space, hsize_t *offset)
{
    herr_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);
    HDassert(offset);

    ret_value = (*space->select.type->project_scalar)(space, offset);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_project_scalar() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_project_simple
 PURPOSE
    Project a selection onto/into a dataspace of different rank
 USAGE
    herr_t H5S_select_project_simple(space, new_space, offset)
        const H5S_t *space;             IN: Pointer to dataspace to project
        H5S_t *new_space;               IN/OUT: Pointer to dataspace projected onto
        hsize_t *offset;                IN/OUT: Offset of projected point
 RETURNS
    Non-negative on success, negative on failure
 DESCRIPTION
    Projects a selection onto/into a simple dataspace, computing
    the offset of the first element in the original selection.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This routine participates in the "Inlining C function pointers"
        pattern, don't call it directly, use the appropriate macro
        defined in H5Sprivate.h.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_project_simple(const H5S_t *space, H5S_t *new_space, hsize_t *offset)
{
    herr_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);
    HDassert(new_space);
    HDassert(offset);

    ret_value = (*space->select.type->project_simple)(space, new_space, offset);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_project_simple() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_iter_init
 PURPOSE
    Initializes iteration information for a selection.
 USAGE
    herr_t H5S_select_iter_init(sel_iter, space, elmt_size)
        H5S_sel_iter_t *sel_iter; OUT: Selection iterator to initialize.
        H5S_t *space;           IN: Dataspace object containing selection to
                                    iterate over
        size_t elmt_size;       IN: Size of elements in the selection
 RETURNS
     Non-negative on success, negative on failure.
 DESCRIPTION
    Initialize the selection iterator object to point to the first element
    in the dataspace's selection.
--------------------------------------------------------------------------*/
herr_t
H5S_select_iter_init(H5S_sel_iter_t *sel_iter, const H5S_t *space, size_t elmt_size)
{
    herr_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(sel_iter);
    HDassert(space);

    /* Initialize common information */

    /* Save the dataspace's rank */
    sel_iter->rank = space->extent.rank;

    /* Point to the dataspace dimensions, if there are any */
    if(sel_iter->rank > 0)
        sel_iter->dims = space->extent.size;
    else
        sel_iter->dims = NULL;

    /* Save the element size */
    sel_iter->elmt_size = elmt_size;

    /* Call initialization routine for selection type */
    ret_value = (*space->select.type->iter_init)(sel_iter, space);
    HDassert(sel_iter->type);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_iter_init() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_iter_coords
 PURPOSE
    Get the coordinates of the current iterator position
 USAGE
    herr_t H5S_select_iter_coords(sel_iter,coords)
        H5S_sel_iter_t *sel_iter; IN: Selection iterator to query
        hsize_t *coords;         OUT: Array to place iterator coordinates in
 RETURNS
    Non-negative on success, negative on failure.
 DESCRIPTION
    The current location of the iterator within the selection is placed in
    the COORDS array.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This routine participates in the "Inlining C function pointers"
        pattern, don't call it directly, use the appropriate macro
        defined in H5Sprivate.h.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_iter_coords(const H5S_sel_iter_t *sel_iter, hsize_t *coords)
{
    herr_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(sel_iter);
    HDassert(coords);

    /* Call iter_coords routine for selection type */
    ret_value = (*sel_iter->type->iter_coords)(sel_iter,coords);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_iter_coords() */

#ifdef LATER

/*--------------------------------------------------------------------------
 NAME
    H5S_select_iter_block
 PURPOSE
    Get the block of the current iterator position
 USAGE
    herr_t H5S_select_iter_block(sel_iter,start,end)
        const H5S_sel_iter_t *sel_iter; IN: Selection iterator to query
        hsize_t *start;    OUT: Array to place iterator start block coordinates
        hsize_t *end;      OUT: Array to place iterator end block coordinates
 RETURNS
    Non-negative on success, negative on failure.
 DESCRIPTION
    The current location of the iterator within the selection is placed in
    the COORDS array.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This routine participates in the "Inlining C function pointers"
        pattern, don't call it directly, use the appropriate macro
        defined in H5Sprivate.h.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_select_iter_block(const H5S_sel_iter_t *iter, hsize_t *start, hsize_t *end)
{
    herr_t ret_value;         /* return value */

    FUNC_ENTER_NOAPI_NOINIT_NOINIT_NOERR

    /* Check args */
    HDassert(iter);
    HDassert(start);
    HDassert(end);

    /* Call iter_block routine for selection type */
    ret_value = (*iter->type->iter_block)(iter,start,end);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_iter_block() */
#endif /* LATER */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_iter_nelmts
 PURPOSE
    Get the number of elements left to iterate over in selection
 USAGE
    hssize_t H5S_select_iter_nelmts(sel_iter)
        H5S_sel_iter_t *sel_iter; IN: Selection iterator to query
 RETURNS
    The number of elements in selection on success, 0 on failure
 DESCRIPTION
    Returns the number of elements in current selection for dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This routine participates in the "Inlining C function pointers"
        pattern, don't call it directly, use the appropriate macro
        defined in H5Sprivate.h.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hsize_t
H5S_select_iter_nelmts(const H5S_sel_iter_t *sel_iter)
{
    hsize_t ret_value = 0;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(sel_iter);

    /* Call iter_nelmts routine for selection type */
    ret_value = (*sel_iter->type->iter_nelmts)(sel_iter);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_iter_nelmts() */

#ifdef LATER

/*--------------------------------------------------------------------------
 NAME
    H5S_select_iter_has_next_block
 PURPOSE
    Check if there is another block available in the selection iterator
 USAGE
    htri_t H5S_select_iter_has_next_block(sel_iter)
        const H5S_sel_iter_t *sel_iter; IN: Selection iterator to query
 RETURNS
    Non-negative on success, negative on failure.
 DESCRIPTION
    Check if there is another block available to advance to in the selection
    iterator.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This routine participates in the "Inlining C function pointers"
        pattern, don't call it directly, use the appropriate macro
        defined in H5Sprivate.h.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static htri_t
H5S_select_iter_has_next_block(const H5S_sel_iter_t *iter)
{
    herr_t ret_value;         /* return value */

    FUNC_ENTER_NOAPI_NOINIT_NOINIT_NOERR

    /* Check args */
    HDassert(iter);

    /* Call iter_has_next_block routine for selection type */
    ret_value = (*iter->type->iter_has_next_block)(iter);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_iter_has_next_block() */
#endif /* LATER */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_iter_next
 PURPOSE
    Advance selection iterator to next element
 USAGE
    herr_t H5S_select_iter_next(iter, nelem)
        H5S_sel_iter_t *iter;   IN/OUT: Selection iterator to change
        hsize_t nelem;          IN: Number of elements to advance by
 RETURNS
    Non-negative on success, negative on failure.
 DESCRIPTION
    Move the current element for the selection iterator to the NELEM'th next
    element in the selection.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This routine participates in the "Inlining C function pointers"
        pattern, don't call it directly, use the appropriate macro
        defined in H5Sprivate.h.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_iter_next(H5S_sel_iter_t *iter, hsize_t nelem)
{
    herr_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(iter);
    HDassert(nelem>0);

    /* Call iter_next routine for selection type */
    ret_value = (*iter->type->iter_next)(iter,nelem);

    /* Decrement the number of elements left in selection */
    iter->elmt_left-=nelem;

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_iter_next() */

#ifdef LATER

/*--------------------------------------------------------------------------
 NAME
    H5S_select_iter_next_block
 PURPOSE
    Advance selection iterator to next block
 USAGE
    herr_t H5S_select_iter_next_block(iter)
        H5S_sel_iter_t *iter;   IN/OUT: Selection iterator to change
 RETURNS
    Non-negative on success, negative on failure.
 DESCRIPTION
    Move the current element for the selection iterator to the next
    block in the selection.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    Doesn't maintain the 'elmt_left' field of the selection iterator.

    This routine participates in the "Inlining C function pointers"
        pattern, don't call it directly, use the appropriate macro
        defined in H5Sprivate.h.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_select_iter_next_block(H5S_sel_iter_t *iter)
{
    herr_t ret_value;         /* return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(iter);

    /* Call iter_next_block routine for selection type */
    ret_value = (*iter->type->iter_next_block)(iter);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_iter_next_block() */
#endif /* LATER */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_iter_release
 PURPOSE
    Release a selection iterator's resources.
 USAGE
    hssize_t H5S_select_iter_release(sel_iter)
        H5S_sel_iter_t *sel_iter; IN: Selection iterator to query
 RETURNS
    The number of elements in selection on success, 0 on failure
 DESCRIPTION
    Returns the number of elements in current selection for dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This routine participates in the "Inlining C function pointers"
        pattern, don't call it directly, use the appropriate macro
        defined in H5Sprivate.h.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_iter_release(H5S_sel_iter_t *sel_iter)
{
    herr_t ret_value = FAIL;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(sel_iter);

    /* Call selection type-specific release routine */
    ret_value = (*sel_iter->type->iter_release)(sel_iter);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_iter_release() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_iterate
 PURPOSE
    Iterate over the selected elements in a memory buffer.
 USAGE
    herr_t H5S_select_iterate(buf, type, space, operator, operator_data)
        void *buf;      IN/OUT: Buffer containing elements to iterate over
        H5T_t *type;    IN: Datatype of BUF array.
        H5S_t *space;   IN: Dataspace object containing selection to iterate over
        H5D_operator_t op; IN: Function pointer to the routine to be
                                called for each element in BUF iterated over.
        void *operator_data;    IN/OUT: Pointer to any user-defined data
                                associated with the operation.
 RETURNS
    Returns the return value of the last operator if it was non-zero, or zero
    if all elements were processed. Otherwise returns a negative value.
 DESCRIPTION
    Iterates over the selected elements in a memory buffer, calling the user's
    callback function for each element.  The selection in the dataspace is
    modified so that any elements already iterated over are removed from the
    selection if the iteration is interrupted (by the H5D_operator_t function
    returning non-zero) in the "middle" of the iteration and may be re-started
    by the user where it left off.

    NOTE: Until "subtracting" elements from a selection is implemented,
        the selection is not modified.
--------------------------------------------------------------------------*/
herr_t
H5S_select_iterate(void *buf, const H5T_t *type, const H5S_t *space,
    const H5S_sel_iter_op_t *op, void *op_data)
{
    H5S_sel_iter_t *iter = NULL; /* Selection iteration info */
    hbool_t iter_init = FALSE;  /* Selection iteration info has been initialized */
    hsize_t *off = NULL;        /* Array to store sequence offsets */
    size_t *len = NULL;         /* Array to store sequence lengths */
    hssize_t nelmts;            /* Number of elements in selection */
    hsize_t space_size[H5O_LAYOUT_NDIMS]; /* Dataspace size */
    size_t max_elem;            /* Maximum number of elements allowed in sequences */
    size_t elmt_size;           /* Datatype size */
    unsigned ndims;             /* Number of dimensions in dataspace */
    herr_t user_ret = 0;        /* User's return value */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(buf);
    HDassert(type);
    HDassert(space);
    HDassert(op);

    /* Get the datatype size */
    if(0 == (elmt_size = H5T_get_size(type)))
        HGOTO_ERROR(H5E_DATATYPE, H5E_BADSIZE, FAIL, "datatype size invalid")

    /* Allocate the selection iterator */
    if(NULL == (iter = H5FL_MALLOC(H5S_sel_iter_t)))
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate selection iterator")

    /* Initialize iterator */
    if(H5S_select_iter_init(iter, space, elmt_size) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator")
    iter_init = TRUE;	/* Selection iteration info has been initialized */

    /* Get the number of elements in selection */
    if((nelmts = (hssize_t)H5S_GET_SELECT_NPOINTS(space)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOUNT, FAIL, "can't get number of elements selected")

    /* Get the rank of the dataspace */
    ndims = space->extent.rank;

    if(ndims > 0) {
	/* Copy the size of the space */
	HDassert(space->extent.size);
	HDmemcpy(space_size, space->extent.size, ndims * sizeof(hsize_t));
    } /* end if */
    space_size[ndims] = elmt_size;

    /* Compute the maximum number of bytes required */
    H5_CHECKED_ASSIGN(max_elem, size_t, nelmts, hssize_t);

    /* Allocate the offset & length arrays */
    if(NULL == (len = H5FL_SEQ_MALLOC(size_t, H5D_IO_VECTOR_SIZE)))
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate length vector array")
    if(NULL == (off = H5FL_SEQ_MALLOC(hsize_t, H5D_IO_VECTOR_SIZE)))
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate offset vector array")

    /* Loop, while elements left in selection */
    while(max_elem > 0 && user_ret == 0) {
        size_t nelem;               /* Number of elements used in sequences */
        size_t nseq;                /* Number of sequences generated */
        size_t curr_seq;            /* Current sequence being worked on */

        /* Get the sequences of bytes */
        if(H5S_SELECT_GET_SEQ_LIST(space, 0, iter, (size_t)H5D_IO_VECTOR_SIZE, max_elem, &nseq, &nelem, off, len) < 0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_UNSUPPORTED, FAIL, "sequence length generation failed")

        /* Loop, while sequences left to process */
        for(curr_seq = 0; curr_seq < nseq && user_ret == 0; curr_seq++) {
            hsize_t curr_off;           /* Current offset within sequence */
            size_t curr_len;            /* Length of bytes left to process in sequence */

            /* Get the current offset */
            curr_off = off[curr_seq];

            /* Get the number of bytes in sequence */
            curr_len = len[curr_seq];

            /* Loop, while bytes left in sequence */
            while(curr_len > 0 && user_ret == 0) {
                hsize_t coords[H5O_LAYOUT_NDIMS];  /* Coordinates of element in dataspace */
                hsize_t tmp_off;        /* Temporary offset within sequence */
                uint8_t *loc;           /* Current element location in buffer */
                int i;			/* Local Index variable */

                /* Compute the coordinate from the offset */
                for(i = (int)ndims, tmp_off = curr_off; i >= 0; i--) {
                    coords[i] = tmp_off % space_size[i];
                    tmp_off /= space_size[i];
                } /* end for */

                /* Get the location within the user's buffer */
                loc = (unsigned char *)buf + curr_off;

                /* Check which type of callback to make */
                switch(op->op_type) {
                    case H5S_SEL_ITER_OP_APP:
                        /* Make the application callback */
                        user_ret = (op->u.app_op.op)(loc, op->u.app_op.type_id, ndims, coords, op_data);
                    break;
                    case H5S_SEL_ITER_OP_LIB:
                        /* Call the library's callback */
                        user_ret = (op->u.lib_op)(loc, type, ndims, coords, op_data);
                    break;
                    default:
                        HGOTO_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, FAIL, "unsupported op type")
                } /* end switch */

                /* Increment offset in dataspace */
                curr_off += elmt_size;

                /* Decrement number of bytes left in sequence */
                curr_len -= elmt_size;
            } /* end while */
        } /* end for */

        /* Decrement number of elements left to process */
        max_elem -= nelem;
    } /* end while */

    /* Set return value */
    ret_value = user_ret;

done:
    /* Release resources, if allocated */
    if(len)
        len = H5FL_SEQ_FREE(size_t, len);
    if(off)
        off = H5FL_SEQ_FREE(hsize_t, off);

    /* Release selection iterator */
    if(iter_init && H5S_SELECT_ITER_RELEASE(iter) < 0)
        HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator")
    if(iter)
        iter = H5FL_FREE(H5S_sel_iter_t, iter);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5S_select_iterate() */


/*--------------------------------------------------------------------------
 NAME
    H5Sget_select_type
 PURPOSE
    Retrieve the type of selection in a dataspace
 USAGE
    H5S_sel_type H5Sget_select_type(space_id)
        hid_t space_id;	        IN: Dataspace object to query
 RETURNS
    Non-negative on success/Negative on failure.  Return value is from the
    set of values in the H5S_sel_type enumerated type.
 DESCRIPTION
	This function retrieves the type of selection currently defined for
    a dataspace.
--------------------------------------------------------------------------*/
H5S_sel_type
H5Sget_select_type(hid_t space_id)
{
    H5S_t *space;	        /* dataspace to modify */
    H5S_sel_type ret_value;     /* Return value */

    FUNC_ENTER_API(H5S_SEL_ERROR)
    H5TRACE1("St", "i", space_id);

    /* Check args */
    if(NULL == (space = (H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, H5S_SEL_ERROR, "not a dataspace")

    /* Set return value */
    ret_value = H5S_GET_SELECT_TYPE(space);

done:
    FUNC_LEAVE_API(ret_value)
}   /* end H5Sget_select_type() */


/*--------------------------------------------------------------------------
 NAME
    H5S_get_select_type
 PURPOSE
    Retrieve the type of selection in a dataspace
 USAGE
    H5S_sel_type H5Sget_select_type(space)
        const H5S_t *space;	        IN: Dataspace object to query
 RETURNS
    Non-negative on success/Negative on failure.  Return value is from the
    set of values in the H5S_sel_type enumerated type.
 DESCRIPTION
	This function retrieves the type of selection currently defined for
    a dataspace.
 COMMENTS
     This routine participates in the "Inlining C function pointers"
        pattern, don't call it directly, use the appropriate macro
        defined in H5Sprivate.h.
--------------------------------------------------------------------------*/
H5S_sel_type
H5S_get_select_type(const H5S_t *space)
{
    H5S_sel_type        ret_value = H5S_SEL_ERROR;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);

    /* Set return value */
    ret_value=H5S_GET_SELECT_TYPE(space);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5S_get_select_type() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_shape_same
 PURPOSE
    Check if two selections are the same shape
 USAGE
    htri_t H5S_select_shape_same(space1, space2)
        const H5S_t *space1;         IN: 1st Dataspace pointer to compare
        const H5S_t *space2;         IN: 2nd Dataspace pointer to compare
 RETURNS
    TRUE/FALSE/FAIL
 DESCRIPTION
    Checks to see if the current selection in the dataspaces are the same
    dimensionality and shape.
    This is primarily used for reading the entire selection in one swoop.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    Assumes that there is only a single "block" for hyperslab selections.
 EXAMPLES
 REVISION LOG
    Modified function to view identical shapes with different dimensions 
    as being the same under some circumstances.
--------------------------------------------------------------------------*/
htri_t
H5S_select_shape_same(const H5S_t *space1, const H5S_t *space2)
{
    H5S_sel_iter_t *iter_a = NULL;  /* Selection a iteration info */
    H5S_sel_iter_t *iter_b = NULL;  /* Selection b iteration info */
    hbool_t iter_a_init = FALSE;  /* Selection a iteration info has been initialized */
    hbool_t iter_b_init = FALSE;  /* Selection b iteration info has been initialized */
    htri_t ret_value = TRUE; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(space1);
    HDassert(space2);

    /* Check for different number of elements selected */
    if(H5S_GET_SELECT_NPOINTS(space1) != H5S_GET_SELECT_NPOINTS(space2))
        HGOTO_DONE(FALSE)

    /* Check special cases if both dataspaces aren't scalar */
    /* (If only one is, the number of selected points check is sufficient) */
    if(space1->extent.rank > 0 && space2->extent.rank > 0) {
        const H5S_t *space_a;           /* Dataspace with larger rank */
        const H5S_t *space_b;           /* Dataspace with smaller rank */
        unsigned space_a_rank;          /* Number of dimensions of dataspace A */
        unsigned space_b_rank;          /* Number of dimensions of dataspace B */

        /* need to be able to handle spaces of different rank:
         *
         * To simplify logic, let space_a point to the element of the set
         * {space1, space2} with the largest rank or space1 if the ranks 
         * are identical.
         *
         * Similarly, let space_b point to the element of {space1, space2}
         * with the smallest rank, or space2 if they are identical.
         *
         * Let:  space_a_rank be the rank of space_a, 
         *       space_b_rank be the rank of space_b,
         *       delta_rank = space_a_rank - space_b_rank.
         *
         * Set all this up below.
         */
        if(space1->extent.rank >= space2->extent.rank) {
            space_a = space1;
            space_a_rank = space_a->extent.rank;

            space_b = space2;
            space_b_rank = space_b->extent.rank;
        } /* end if */
        else {
            space_a = space2;
            space_a_rank = space_a->extent.rank;

            space_b = space1;
            space_b_rank = space_b->extent.rank;
        } /* end else */
        HDassert(space_a_rank >= space_b_rank);
        HDassert(space_b_rank > 0);

        /* Check for different number of elements selected */
        if(H5S_GET_SELECT_NPOINTS(space_a) != H5S_GET_SELECT_NPOINTS(space_b))
            HGOTO_DONE(FALSE)

        /* Check for "easy" cases before getting into generalized block iteration code */
        if((H5S_GET_SELECT_TYPE(space_a) == H5S_SEL_ALL) && (H5S_GET_SELECT_TYPE(space_b) == H5S_SEL_ALL)) {
            hsize_t dims1[H5O_LAYOUT_NDIMS];    /* End point of selection block in dataspace #1 */
            hsize_t dims2[H5O_LAYOUT_NDIMS];    /* End point of selection block in dataspace #2 */
            int space_a_dim;                /* Current dimension in dataspace A */
            int space_b_dim;                /* Current dimension in dataspace B */

            if(H5S_get_simple_extent_dims(space_a, dims1, NULL) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "unable to get dimensionality")
            if(H5S_get_simple_extent_dims(space_b, dims2, NULL) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "unable to get dimensionality")

            space_a_dim = (int)space_a_rank - 1;
            space_b_dim = (int)space_b_rank - 1;

            /* recall that space_a_rank >= space_b_rank. 
             *
             * In the following while loop, we test to see if space_a and space_b
             * have identical size in all dimensions they have in common.
             */
            while(space_b_dim >= 0) {
                if(dims1[space_a_dim] != dims2[space_b_dim])
                    HGOTO_DONE(FALSE)

                space_a_dim--;
                space_b_dim--;
            } /* end while */

            /* Since we are selecting the entire space, we must also verify that space_a 
             * has size 1 in all dimensions that it does not share with space_b.
             */
            while(space_a_dim >= 0) {
                if(dims1[space_a_dim] != 1)
                    HGOTO_DONE(FALSE)

                space_a_dim--;
            } /* end while */
        } /* end if */
        else if((H5S_GET_SELECT_TYPE(space1) == H5S_SEL_NONE) || (H5S_GET_SELECT_TYPE(space2) == H5S_SEL_NONE)) {
            /* (Both must be, at this point, if one is) */
            HGOTO_DONE(TRUE)
        } /* end if */
        else if((H5S_GET_SELECT_TYPE(space_a) == H5S_SEL_HYPERSLABS && space_a->select.sel_info.hslab->diminfo_valid)
                && (H5S_GET_SELECT_TYPE(space_b) == H5S_SEL_HYPERSLABS && space_b->select.sel_info.hslab->diminfo_valid)) {
            int space_a_dim;                /* Current dimension in dataspace A */
            int space_b_dim;                /* Current dimension in dataspace B */

            space_a_dim = (int)space_a_rank - 1;
            space_b_dim = (int)space_b_rank - 1;

            /* check that the shapes are the same in the common dimensions, and that
             * block == 1 in all dimensions that appear only in space_a.
             */
            while(space_b_dim >= 0) {
                if(space_a->select.sel_info.hslab->opt_diminfo[space_a_dim].stride != 
                        space_b->select.sel_info.hslab->opt_diminfo[space_b_dim].stride)
                    HGOTO_DONE(FALSE)

                if(space_a->select.sel_info.hslab->opt_diminfo[space_a_dim].count !=
                        space_b->select.sel_info.hslab->opt_diminfo[space_b_dim].count)
                    HGOTO_DONE(FALSE)

                if(space_a->select.sel_info.hslab->opt_diminfo[space_a_dim].block != 
                        space_b->select.sel_info.hslab->opt_diminfo[space_b_dim].block)
                    HGOTO_DONE(FALSE)

                space_a_dim--;
                space_b_dim--;
            } /* end while */

            while(space_a_dim >= 0) {
                if(space_a->select.sel_info.hslab->opt_diminfo[space_a_dim].block != 1)
                    HGOTO_DONE(FALSE)

                space_a_dim--;
            } /* end while */
        } /* end if */
        /* Iterate through all the blocks in the selection */
        else {
            hsize_t start_a[H5S_MAX_RANK];      /* Start point of selection block in dataspace a */
            hsize_t start_b[H5S_MAX_RANK];      /* Start point of selection block in dataspace b */
            hsize_t end_a[H5S_MAX_RANK];        /* End point of selection block in dataspace a */
            hsize_t end_b[H5S_MAX_RANK];        /* End point of selection block in dataspace b */
            hsize_t off_a[H5S_MAX_RANK];        /* Offset of selection a blocks */
            hsize_t off_b[H5S_MAX_RANK];        /* Offset of selection b blocks */
            hbool_t first_block = TRUE;         /* Flag to indicate the first block */

            /* Allocate the selection iterators */
            if(NULL == (iter_a = H5FL_MALLOC(H5S_sel_iter_t)))
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate selection iterator")
            if(NULL == (iter_b = H5FL_MALLOC(H5S_sel_iter_t)))
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate selection iterator")

            /* Initialize iterator for each dataspace selection
             * Use '0' for element size instead of actual element size to indicate
             * that the selection iterator shouldn't be "flattened", since we
             * aren't actually going to be doing I/O with the iterators.
             */
            if(H5S_select_iter_init(iter_a, space_a, (size_t)0) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator a")
            iter_a_init = TRUE;
            if(H5S_select_iter_init(iter_b, space_b, (size_t)0) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator b")
            iter_b_init = TRUE;

            /* Iterate over all the blocks in each selection */
            while(1) {
                int space_a_dim;                /* Current dimension in dataspace A */
                int space_b_dim;                /* Current dimension in dataspace B */
                htri_t status_a, status_b;      /* Status from next block checks */

                /* Get the current block for each selection iterator */
                if(H5S_SELECT_ITER_BLOCK(iter_a, start_a, end_a) < 0)
                    HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "unable to get iterator block a")
                if(H5S_SELECT_ITER_BLOCK(iter_b, start_b, end_b) < 0)
                    HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "unable to get iterator block b")

                space_a_dim = (int)space_a_rank - 1;
                space_b_dim = (int)space_b_rank - 1;

                /* The first block only compares the sizes and sets the 
                 * relative offsets for later blocks 
                 */
                if(first_block) {
                    /* If the block sizes in the common dimensions from 
                     * each selection don't match, get out
                     */
                    while(space_b_dim >= 0) {
                        if((end_a[space_a_dim] - start_a[space_a_dim]) !=
                                (end_b[space_b_dim] - start_b[space_b_dim]))
                            HGOTO_DONE(FALSE)

                        /* Set the relative locations of the selections */
                        off_a[space_a_dim] = start_a[space_a_dim];
                        off_b[space_b_dim] = start_b[space_b_dim];

                        space_a_dim--;
                        space_b_dim--;
                    } /* end while */

                    /* similarly, if the block size in any dimension that appears only
                     * in space_a is not equal to 1, get out.
                     */
                    while(space_a_dim >= 0) {
                        if((end_a[space_a_dim] - start_a[space_a_dim]) != 0)
                            HGOTO_DONE(FALSE)

                        /* Set the relative locations of the selections */
                        off_a[space_a_dim] = start_a[space_a_dim];

                        space_a_dim--;
                    } /* end while */

                    /* Reset "first block" flag */
                    first_block = FALSE;
                } /* end if */
                /* Check over the blocks for each selection */
                else {
                    /* for dimensions that space_a and space_b have in common: */
                    while(space_b_dim >= 0) {
                        /* Check if the blocks are in the same relative location */
                        if((start_a[space_a_dim] - off_a[space_a_dim]) !=
                                (start_b[space_b_dim] - off_b[space_b_dim]))
                            HGOTO_DONE(FALSE)

                        /* If the block sizes from each selection doesn't match, get out */
                        if((end_a[space_a_dim] - start_a[space_a_dim]) !=
                                (end_b[space_b_dim] - start_b[space_b_dim]))
                            HGOTO_DONE(FALSE)

                        space_a_dim--;
                        space_b_dim--;
                    } /* end while */

                    /* For dimensions that appear only in space_a: */
                    while(space_a_dim >= 0) {
                        /* If the block size isn't 1, get out */
                        if((end_a[space_a_dim] - start_a[space_a_dim]) != 0)
                            HGOTO_DONE(FALSE)

                        space_a_dim--;
                    } /* end while */
                } /* end else */

                /* Check if we are able to advance to the next selection block */
                if((status_a = H5S_SELECT_ITER_HAS_NEXT_BLOCK(iter_a)) < 0)
                    HGOTO_ERROR(H5E_DATASPACE, H5E_CANTNEXT, FAIL, "unable to check iterator block a")

                if((status_b = H5S_SELECT_ITER_HAS_NEXT_BLOCK(iter_b)) < 0)
                    HGOTO_ERROR(H5E_DATASPACE, H5E_CANTNEXT, FAIL, "unable to check iterator block b")

                /* Did we run out of blocks at the same time? */
                if((status_a == FALSE) && (status_b == FALSE))
                    break;
                else if(status_a != status_b)
                    HGOTO_DONE(FALSE)
                else {
                    /* Advance to next block in selection iterators */
                    if(H5S_SELECT_ITER_NEXT_BLOCK(iter_a) < 0)
                        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTNEXT, FAIL, "unable to advance to next iterator block a")

                    if(H5S_SELECT_ITER_NEXT_BLOCK(iter_b) < 0)
                        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTNEXT, FAIL, "unable to advance to next iterator block b")
                } /* end else */
            } /* end while */
        } /* end else */
    } /* end if */

done:
    if(iter_a_init && H5S_SELECT_ITER_RELEASE(iter_a) < 0)
        HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator a")
    if(iter_a)
        iter_a = H5FL_FREE(H5S_sel_iter_t, iter_a);
    if(iter_b_init && H5S_SELECT_ITER_RELEASE(iter_b) < 0)
        HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator b")
    if(iter_b)
        iter_b = H5FL_FREE(H5S_sel_iter_t, iter_b);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_shape_same() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_construct_projection

 PURPOSE
    Given a dataspace a of rank n with some selection, construct a new
    dataspace b of rank m (m != n), with the selection in a being 
    topologically identical to that in b (as verified by 
    H5S_select_shape_same().

    This function exists, as some I/O code chokes on topologically 
    identical selections with different ranks.  At least to begin 
    with, we will deal with the issue by constructing projections
    of the memory dataspace with ranks equaling those of the file 
    dataspace.

    Note that if m > n, it is possible that the starting point in the 
    buffer associated with the memory dataspace will have to be 
    adjusted to match the projected dataspace.  If the buf parameter
    is not NULL, the function must return an adjusted buffer base
    address in *adj_buf_ptr.

 USAGE
    htri_t H5S_select_construct_projection(base_space, 
                                           new_space_ptr,
                                           new_space_rank,
                                           buf,
                                           adj_buf_ptr)
        const H5S_t *base_space;     IN: Ptr to Dataspace to project
        H5S_t ** new_space_ptr;     OUT: Ptr to location in which to return
					 the address of the projected space
        int new_space_rank;	     IN: Rank of the projected space.
        const void * buf;            IN: Base address of the buffer 
					 associated with the base space.
					 May be NULL.
        void ** adj_buf_ptr;        OUT: If buf != NULL, store the base
					 address of the section of buf 
					 that is described by *new_space_ptr
					 in *adj_buf_ptr.
					 
 RETURNS
    Non-negative on success/Negative on failure.

 DESCRIPTION
    Construct a new dataspace and associated selection which is a 
    projection of the supplied dataspace and associated selection into 
    the specified rank.  Return it in *new_space_ptr.

    If buf is supplied, computes the base address of the projected 
    selection in buf, and stores the base address in *adj_buf_ptr.
    
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    The selection in the supplied base_space has thickness 1 in all 
    dimensions greater than new_space_rank.  Note that here we count
    dimensions from the fastest changing coordinate to the slowest 
    changing changing coordinate.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_construct_projection(const H5S_t *base_space, H5S_t **new_space_ptr,
    unsigned new_space_rank, const void *buf, void const **adj_buf_ptr, hsize_t element_size)
{
    H5S_t * new_space = NULL;           /* New dataspace constructed */
    hsize_t base_space_dims[H5S_MAX_RANK];      /* Current dimensions of base dataspace */
    hsize_t base_space_maxdims[H5S_MAX_RANK];   /* Maximum dimensions of base dataspace */
    int sbase_space_rank;               /* Signed # of dimensions of base dataspace */
    unsigned base_space_rank;           /* # of dimensions of base dataspace */
    hsize_t projected_space_element_offset = 0; /* Offset of selected element in projected buffer */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(base_space != NULL);
    HDassert((H5S_GET_EXTENT_TYPE(base_space) == H5S_SCALAR) || (H5S_GET_EXTENT_TYPE(base_space) == H5S_SIMPLE));
    HDassert(new_space_ptr != NULL);
    HDassert((new_space_rank != 0) || (H5S_GET_SELECT_NPOINTS(base_space) <= 1));
    HDassert(new_space_rank <= H5S_MAX_RANK);
    HDassert((buf == NULL) || (adj_buf_ptr != NULL));
    HDassert(element_size > 0 );

    /* Get the extent info for the base dataspace */
    if((sbase_space_rank = H5S_get_simple_extent_dims(base_space, base_space_dims, base_space_maxdims)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "unable to get dimensionality of base space")
    base_space_rank = (unsigned)sbase_space_rank;
    HDassert(base_space_rank != new_space_rank);

    /* Check if projected space is scalar */
    if(new_space_rank == 0) {
        hssize_t npoints;               /* Number of points selected */

        /* Retreve the number of elements selected */
        if((npoints = (hssize_t)H5S_GET_SELECT_NPOINTS(base_space)) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "unable to get number of points selected")
        HDassert(npoints <= 1);

        /* Create new scalar dataspace */
        if(NULL == (new_space = H5S_create(H5S_SCALAR)))
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCREATE, FAIL, "unable to create scalar dataspace")
    
        /* No need to register the dataspace(i.e. get an ID) as
         * we will just be discarding it shortly.
         */

        /* Selection for the new space will be either all or 
         * none, depending on whether the base space has 0 or
         * 1 elements selected.
         *
         * Observe that the base space can't have more than 
         * one selected element, since its selection has the
         * same shape as the file dataspace, and that data 
         * space is scalar.
         */
        if(1 == npoints) {
            /* Assuming that the selection in the base dataspace is not
             * empty, we must compute the offset of the selected item in 
             * the buffer associated with the base dataspace.
             *
             * Since the new space rank is zero, we know that the 
             * the base space must have rank at least 1 -- and 
             * hence it is a simple dataspace.  However, the 
             * selection, may be either point, hyperspace, or all.
             *
             */
            if(H5S_SELECT_PROJECT_SCALAR(base_space, &projected_space_element_offset) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSET, FAIL, "unable to project scalar selection")
        } /* end if */
        else {
            HDassert(0 == npoints);

            if(H5S_select_none(new_space) < 0)
                 HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't delete default selection")
        } /* end else */
    } /* end if */
    else { /* projected space must be simple */
        hsize_t new_space_dims[H5S_MAX_RANK];   /* Current dimensions for new dataspace */
        hsize_t new_space_maxdims[H5S_MAX_RANK];/* Maximum dimensions for new dataspace */
        unsigned rank_diff;             /* Difference in ranks */
    
        /* Set up the dimensions of the new, projected dataspace.
         *
         * How we do this depends on whether we are projecting up into 
         * increased dimensions, or down into a reduced number of 
         * dimensions.
         *
         * If we are projecting up (the first half of the following 
         * if statement), we copy the dimensions of the base data 
         * space into the fastest changing dimensions of the new 
         * projected dataspace, and set the remaining dimensions to
         * one.
         *
         * If we are projecting down (the second half of the following
         * if statement), we just copy the dimensions with the most 
         * quickly changing dimensions into the dims for the projected
         * data set.
         *
         * This works, because H5S_select_shape_same() will return 
         * true on selections of different rank iff:
         *
         * 1) the selection in the lower rank dataspace matches that
         *    in the dimensions with the fastest changing indicies in
         *    the larger rank dataspace, and
         *
         * 2) the selection has thickness 1 in all ranks that appear
         *    only in the higher rank dataspace (i.e. those with 
         *    more slowly changing indicies).
         */ 
        if(new_space_rank > base_space_rank) {
            hsize_t tmp_dim_size = 1;   /* Temporary dimension value, for filling arrays */

            /* we must copy the dimensions of the base space into 
             * the fastest changing dimensions of the new space,
             * and set the remaining dimensions to 1
             */
            rank_diff = new_space_rank - base_space_rank;
            H5VM_array_fill(new_space_dims, &tmp_dim_size, sizeof(tmp_dim_size), rank_diff);
            H5VM_array_fill(new_space_maxdims, &tmp_dim_size, sizeof(tmp_dim_size), rank_diff);
            HDmemcpy(&new_space_dims[rank_diff], base_space_dims, sizeof(new_space_dims[0]) * base_space_rank);
            HDmemcpy(&new_space_maxdims[rank_diff], base_space_maxdims, sizeof(new_space_maxdims[0]) * base_space_rank);
        } /* end if */
        else { /* new_space_rank < base_space_rank */
            /* we must copy the fastest changing dimension of the 
             * base space into the dimensions of the new space.
             */
            rank_diff = base_space_rank - new_space_rank;
            HDmemcpy(new_space_dims, &base_space_dims[rank_diff], sizeof(new_space_dims[0]) * new_space_rank);
            HDmemcpy(new_space_maxdims, &base_space_maxdims[rank_diff], sizeof(new_space_maxdims[0]) * new_space_rank);
        } /* end else */

        /* now have the new space rank and dimensions set up -- 
         * so we can create the new simple dataspace.
         */
        if(NULL == (new_space = H5S_create_simple(new_space_rank, new_space_dims, new_space_maxdims)))
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCREATE, FAIL, "can't create simple dataspace")
    
        /* No need to register the dataspace(i.e. get an ID) as
         * we will just be discarding it shortly.
         */

        /* If we get this far, we have successfully created the projected
         * dataspace.  We must now project the selection in the base
         * dataspace into the projected dataspace.
         */
        if(H5S_SELECT_PROJECT_SIMPLE(base_space, new_space, &projected_space_element_offset) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSET, FAIL, "unable to project simple selection")
    
        /* If we get this far, we have created the new dataspace, and projected
         * the selection in the base dataspace into the new dataspace.
         *
         * If the base dataspace is simple, check to see if the 
         * offset_changed flag on the base selection has been set -- if so, 
         * project the offset into the new dataspace and set the 
         * offset_changed flag.
         */
        if(H5S_GET_EXTENT_TYPE(base_space) == H5S_SIMPLE && base_space->select.offset_changed) {
            if(new_space_rank > base_space_rank) {
                HDmemset(new_space->select.offset, 0, sizeof(new_space->select.offset[0]) * rank_diff);
                HDmemcpy(&new_space->select.offset[rank_diff], base_space->select.offset, sizeof(new_space->select.offset[0]) * base_space_rank);
            } /* end if */
            else
                HDmemcpy(new_space->select.offset, &base_space->select.offset[rank_diff], sizeof(new_space->select.offset[0]) * new_space_rank);

            /* Propagate the offset changed flag into the new dataspace. */
            new_space->select.offset_changed = TRUE;
        } /* end if */
    } /* end else */

    /* If we have done the projection correctly, the following assertion
     * should hold.
     */
    HDassert(TRUE == H5S_select_shape_same(base_space, new_space));

    /* load the address of the new space into *new_space_ptr */
    *new_space_ptr = new_space;

    /* now adjust the buffer if required */
    if(buf != NULL) {
        if(new_space_rank < base_space_rank) {
            /* a bit of pointer magic here:
             *
             * Since we can't do pointer arithmetic on void pointers, we first
             * cast buf to a pointer to byte -- i.e. uint8_t.
             *
             * We then multiply the projected space element offset we 
             * calculated earlier by the supplied element size, add this 
             * value to the type cast buf pointer, cast the result back 
             * to a pointer to void, and assign the result to *adj_buf_ptr.
             */
            *adj_buf_ptr = (const void *)(((const uint8_t *)buf) + 
                    ((size_t)(projected_space_element_offset * element_size)));
        } /* end if */
        else
            /* No adjustment necessary */
            *adj_buf_ptr = buf;
    } /* end if */

done:
    /* Cleanup on error */
    if(ret_value < 0)
        if(new_space && H5S_close(new_space) < 0)
            HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release dataspace")

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_construct_projection() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_fill
 PURPOSE
    Fill a selection in memory with a value
 USAGE
    herr_t H5S_select_fill(fill,fill_size,space,buf)
        const void *fill;       IN: Pointer to fill value to use
        size_t fill_size;       IN: Size of elements in memory buffer & size of
                                    fill value
        H5S_t *space;           IN: Dataspace describing memory buffer &
                                    containing selection to use.
        void *buf;              IN/OUT: Memory buffer to fill selection in
 RETURNS
    Non-negative on success/Negative on failure.
 DESCRIPTION
    Use the selection in the dataspace to fill elements in a memory buffer.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    The memory buffer elements are assumed to have the same datatype as the
    fill value being placed into them.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_fill(const void *fill, size_t fill_size, const H5S_t *space, void *_buf)
{
    H5S_sel_iter_t *iter = NULL; /* Selection iteration info */
    hbool_t iter_init = 0;      /* Selection iteration info has been initialized */
    hsize_t *off = NULL;        /* Array to store sequence offsets */
    size_t *len = NULL;         /* Array to store sequence lengths */
    hssize_t nelmts;            /* Number of elements in selection */
    size_t max_elem;            /* Total number of elements in selection */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(fill);
    HDassert(fill_size > 0);
    HDassert(space);
    HDassert(_buf);

    /* Allocate the selection iterator */
    if(NULL == (iter = H5FL_MALLOC(H5S_sel_iter_t)))
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate selection iterator")

    /* Initialize iterator */
    if(H5S_select_iter_init(iter, space, fill_size) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator")
    iter_init = 1;	/* Selection iteration info has been initialized */

    /* Get the number of elements in selection */
    if((nelmts = (hssize_t)H5S_GET_SELECT_NPOINTS(space)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOUNT, FAIL, "can't get number of elements selected")

    /* Compute the number of bytes to process */
    H5_CHECKED_ASSIGN(max_elem, size_t, nelmts, hssize_t);

    /* Allocate the offset & length arrays */
    if(NULL == (len = H5FL_SEQ_MALLOC(size_t, H5D_IO_VECTOR_SIZE)))
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate length vector array")
    if(NULL == (off = H5FL_SEQ_MALLOC(hsize_t, H5D_IO_VECTOR_SIZE)))
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTALLOC, FAIL, "can't allocate offset vector array")

    /* Loop, while elements left in selection */
    while(max_elem > 0) {
        size_t nseq;                /* Number of sequences generated */
        size_t curr_seq;            /* Current sequnce being worked on */
        size_t nelem;               /* Number of elements used in sequences */

        /* Get the sequences of bytes */
        if(H5S_SELECT_GET_SEQ_LIST(space, 0, iter, (size_t)H5D_IO_VECTOR_SIZE, max_elem, &nseq, &nelem, off, len) < 0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_UNSUPPORTED, FAIL, "sequence length generation failed")

        /* Loop over sequences */
        for(curr_seq = 0; curr_seq < nseq; curr_seq++) {
            uint8_t *buf;               /* Current location in buffer */

            /* Get offset in memory buffer */
            buf = (uint8_t *)_buf + off[curr_seq];

            /* Fill each sequence in memory with fill value */
            HDassert((len[curr_seq] % fill_size) == 0);
            H5VM_array_fill(buf, fill, fill_size, (len[curr_seq] / fill_size));
        } /* end for */

        /* Decrement number of elements left to process */
        max_elem -= nelem;
    } /* end while */

done:
    /* Release resources, if allocated */
    if(len)
        len = H5FL_SEQ_FREE(size_t, len);
    if(off)
        off = H5FL_SEQ_FREE(hsize_t, off);

    /* Release selection iterator */
    if(iter_init && H5S_SELECT_ITER_RELEASE(iter) < 0)
        HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator")
    if(iter)
        iter = H5FL_FREE(H5S_sel_iter_t, iter);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_fill() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_project_intersection

 PURPOSE
    Projects the intersection of of the selections of src_space and
    src_intersect_space within the selection of src_space as a selection
    within the selection of dst_space

 USAGE
    herr_t H5S_select_project_intersection(src_space,dst_space,src_intersect_space,proj_space)
        H5S_t *src_space;       IN: Selection that is mapped to dst_space, and intersected with src_intersect_space
        H5S_t *dst_space;       IN: Selection that is mapped to src_space, and which contains the result
        H5S_t *src_intersect_space; IN: Selection whose intersection with src_space is projected to dst_space to obtain the result
        H5S_t **new_space_ptr;  OUT: Will contain the result (intersection of src_intersect_space and src_space projected from src_space to dst_space) after the operation

 RETURNS
    Non-negative on success/Negative on failure.

 DESCRIPTION
    Projects the intersection of of the selections of src_space and
    src_intersect_space within the selection of src_space as a selection
    within the selection of dst_space.  The result is placed in the
    selection of new_space_ptr.
    
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_project_intersection(const H5S_t *src_space, const H5S_t *dst_space,
    const H5S_t *src_intersect_space, H5S_t **new_space_ptr)
{
    H5S_t *new_space = NULL;           /* New dataspace constructed */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(src_space);
    HDassert(dst_space);
    HDassert(src_intersect_space);
    HDassert(new_space_ptr);

    /* Create new space, using dst extent.  Start with "all" selection. */
    if(NULL == (new_space = H5S_create(H5S_SIMPLE)))
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCREATE, FAIL, "unable to create output dataspace")
    if(H5S_extent_copy_real(&new_space->extent, &dst_space->extent, TRUE) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "unable to copy destination space extent")

    /* If the intersecting space is "all", the intersection must be equal to the
     * source space and the projection must be equal to the destination space */
    if(src_intersect_space->select.type->type == H5S_SEL_ALL) {
        /* Copy the destination selection. */
        if(H5S_select_copy(new_space, dst_space, FALSE) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "can't copy destination space selection")
    } /* end if */
    /* If any of the spaces are "none", the projection must also be "none" */
    else if((src_intersect_space->select.type->type == H5S_SEL_NONE)
            || (src_space->select.type->type == H5S_SEL_NONE)
            || (dst_space->select.type->type == H5S_SEL_NONE)) {
        /* Change to "none" selection */
        if(H5S_select_none(new_space) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't change selection")
    } /* end if */
    /* If any of the spaces use point selection, fall back to general algorithm
     */
    else if((src_intersect_space->select.type->type == H5S_SEL_POINTS)
            || (src_space->select.type->type == H5S_SEL_POINTS)
            || (dst_space->select.type->type == H5S_SEL_POINTS))
        HGOTO_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, FAIL, "point selections not currently supported")
    else {
        HDassert(src_intersect_space->select.type->type == H5S_SEL_HYPERSLABS);
        /* Intersecting space is hyperslab selection.  Call the hyperslab
         * routine to project to another hyperslab selection. */
        if(H5S__hyper_project_intersection(src_space, dst_space, src_intersect_space, new_space) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCLIP, FAIL, "can't project hyperslab ondot destination selection")
    } /* end else */

    /* load the address of the new space into *new_space_ptr */
    *new_space_ptr = new_space;

done:
    /* Cleanup on error */
    if(ret_value < 0)
        if(new_space && H5S_close(new_space) < 0)
            HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release dataspace")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5S_select_project_intersection() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_subtract

 PURPOSE
    Subtract one selection from another

 USAGE
    herr_t H5S_select_subtract(space,subtract_space)
        H5S_t *space;           IN/OUT: Selection to be operated on
        H5S_t *subtract_space;  IN: Selection that will be subtracted from space

 RETURNS
    Non-negative on success/Negative on failure.

 DESCRIPTION
    Removes any and all portions of space that are also present in
    subtract_space.  In essence, performs an A_NOT_B operation with the
    two selections.
    
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_subtract(H5S_t *space, H5S_t *subtract_space)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(space);
    HDassert(subtract_space);

    /* If either space is using the none selection, then we do not need to do
     * anything */
    if((space->select.type->type != H5S_SEL_NONE)
            && (subtract_space->select.type->type != H5S_SEL_NONE)) {
        /* If subtract_space is using the all selection, set space to none */
        if(subtract_space->select.type->type == H5S_SEL_ALL) {
            /* Change to "none" selection */
            if(H5S_select_none(space) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't change selection")
        } /* end if */
        else {
            /* Check for point selection in subtract_space, convert to hyperslab */
            if(subtract_space->select.type->type == H5S_SEL_POINTS)
                HGOTO_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, FAIL, "point selections not currently supported")

            /* Check for point or all selection in space, convert to hyperslab */
            if(space->select.type->type == H5S_SEL_ALL) {
                /* Convert current "all" selection to "real" hyperslab selection */
                /* Then allow operation to proceed */
                hsize_t tmp_start[H5S_MAX_RANK];        /* Temporary start information */
                hsize_t tmp_stride[H5S_MAX_RANK];       /* Temporary stride information */
                hsize_t tmp_count[H5S_MAX_RANK];        /* Temporary count information */
                hsize_t tmp_block[H5S_MAX_RANK];        /* Temporary block information */
                unsigned u;                             /* Local index variable */

                /* Fill in temporary information for the dimensions */
                for(u = 0; u < space->extent.rank; u++) {
                    tmp_start[u] = 0;
                    tmp_stride[u] = 1;
                    tmp_count[u] = 1;
                    tmp_block[u] = space->extent.size[u];
                } /* end for */

                /* Convert to hyperslab selection */
                if(H5S_select_hyperslab(space, H5S_SELECT_SET, tmp_start, tmp_stride, tmp_count, tmp_block) < 0)
                    HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSELECT, FAIL, "can't convert selection")
            } /* end if */
            else if(space->select.type->type == H5S_SEL_POINTS)
                HGOTO_ERROR(H5E_DATASPACE, H5E_UNSUPPORTED, FAIL, "point selections not currently supported")

            HDassert(space->select.type->type == H5S_SEL_HYPERSLABS);
            HDassert(subtract_space->select.type->type == H5S_SEL_HYPERSLABS);

            /* Both spaces are now hyperslabs, perform the operation */
            if(H5S__hyper_subtract(space, subtract_space) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCLIP, FAIL, "can't subtract hyperslab")
        } /* end else */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5S_select_subtract() */

