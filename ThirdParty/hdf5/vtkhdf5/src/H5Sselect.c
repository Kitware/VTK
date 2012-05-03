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

/* Programmer:  Quincey Koziol <koziol@ncsa.uiuc.ued>
 *              Friday, May 29, 1998
 *
 * Purpose:	Dataspace selection functions.
 */

#define H5S_PACKAGE		/*suppress error about including H5Spkg	  */


#include "H5private.h"		/* Generic Functions			*/
#include "H5Dprivate.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free Lists                           */
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5Spkg.h"		/* Dataspaces 				*/
#include "H5Vprivate.h"		/* Vector and array functions		*/
#include "H5WBprivate.h"        /* Wrapped Buffers                      */

/* Local functions */
#ifdef LATER
static herr_t H5S_select_iter_block (const H5S_sel_iter_t *iter, hsize_t *start, hsize_t *end);
static htri_t H5S_select_iter_has_next_block (const H5S_sel_iter_t *iter);
static herr_t H5S_select_iter_next_block(H5S_sel_iter_t *iter);
#endif /* LATER */



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
    FUNC_ENTER_NOAPI_NOFUNC(H5S_select_offset)

    /* Check args */
    HDassert(space);
    HDassert(space->extent.rank);
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
H5S_select_copy (H5S_t *dst, const H5S_t *src, hbool_t share_selection)
{
    herr_t ret_value;     /* return value */

    FUNC_ENTER_NOAPI(H5S_select_copy, FAIL);

    /* Check args */
    assert(dst);
    assert(src);

    /* Copy regular fields */
    dst->select=src->select;

    /* Perform correct type of copy based on the type of selection */
    if((ret_value=(*src->select.type->copy)(dst,src,share_selection))<0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "can't copy selection specific information");

done:
    FUNC_LEAVE_NOAPI(ret_value);
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
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_select_release(H5S_t *ds)
{
    herr_t ret_value;   /* Return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5S_select_release);

    assert(ds);

    /* Call the selection type's release function */
    ret_value=(*ds->select.type->release)(ds);

    FUNC_LEAVE_NOAPI(ret_value);
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
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_select_get_seq_list(const H5S_t *space, unsigned flags,
    H5S_sel_iter_t *iter, size_t maxseq, size_t maxbytes,
    size_t *nseq, size_t *nbytes, hsize_t *off, size_t *len)
{
    herr_t ret_value;   /* Return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5S_select_get_seq_list);

    assert(space);

    /* Call the selection type's get_seq_list function */
    ret_value=(*space->select.type->get_seq_list)(space,flags,iter,maxseq,maxbytes,nseq,nbytes,off,len);

    FUNC_LEAVE_NOAPI(ret_value);
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
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hssize_t
H5S_select_serial_size(const H5S_t *space)
{
    hssize_t ret_value;   /* Return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5S_select_serial_size);

    assert(space);

    /* Call the selection type's serial_size function */
    ret_value=(*space->select.type->serial_size)(space);

    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5S_select_serial_size() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_serialize
 PURPOSE
    Serialize the selection for a dataspace into a buffer
 USAGE
    herr_t H5S_select_serialize(space, buf)
        const H5S_t *space;     IN: Dataspace with selection to serialize
        uint8_t *buf;           OUT: Buffer to put serialized selection
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
H5S_select_serialize(const H5S_t *space, uint8_t *buf)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5S_select_serialize);

    assert(space);
    assert(buf);

    /* Call the selection type's serialize function */
    ret_value=(*space->select.type->serialize)(space,buf);

    FUNC_LEAVE_NOAPI(ret_value);
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

    FUNC_ENTER_API(H5Sget_select_npoints, FAIL)
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
    FUNC_ENTER_NOAPI_NOFUNC(H5S_get_select_npoints)

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
    Determines if the current selection at the current offet fits within the
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

    FUNC_ENTER_API(H5Sselect_valid, FAIL)
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
    Determines if the current selection at the current offet fits within the
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
    htri_t ret_value;   /* Return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5S_select_valid);

    assert(space);

    ret_value = (*space->select.type->is_valid)(space);

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5S_select_valid() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_deserialize
 PURPOSE
    Deserialize the current selection from a user-provided buffer into a real
        selection in the dataspace.
 USAGE
    herr_t H5S_select_deserialize(space, buf)
        H5S_t *space;           IN/OUT: Dataspace pointer to place selection into
        uint8 *buf;             IN: Buffer to retrieve serialized selection from
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
H5S_select_deserialize (H5S_t *space, const uint8_t *buf)
{
    const uint8_t *tbuf;    /* Temporary pointer to the selection type */
    uint32_t sel_type;       /* Pointer to the selection type */
    herr_t ret_value=FAIL;  /* return value */

    FUNC_ENTER_NOAPI(H5S_select_deserialize, FAIL);

    assert(space);

    tbuf=buf;
    UINT32DECODE(tbuf, sel_type);
    switch(sel_type) {
        case H5S_SEL_POINTS:         /* Sequence of points selected */
            ret_value=(*H5S_sel_point->deserialize)(space,buf);
            break;

        case H5S_SEL_HYPERSLABS:     /* Hyperslab selection defined */
            ret_value=(*H5S_sel_hyper->deserialize)(space,buf);
            break;

        case H5S_SEL_ALL:            /* Entire extent selected */
            ret_value=(*H5S_sel_all->deserialize)(space,buf);
            break;

        case H5S_SEL_NONE:           /* Nothing selected */
            ret_value=(*H5S_sel_none->deserialize)(space,buf);
            break;

        default:
            break;
    }
    if(ret_value<0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTLOAD, FAIL, "can't deserialize selection");

done:
    FUNC_LEAVE_NOAPI(ret_value);
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

    FUNC_ENTER_API(H5Sget_select_bounds, FAIL)
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
    herr_t ret_value;        /* return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5S_get_select_bounds);

    /* Check args */
    assert(space);
    assert(start);
    assert(end);

    ret_value = (*space->select.type->bounds)(space,start,end);

    FUNC_LEAVE_NOAPI(ret_value);
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
    herr_t ret_value;        /* return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5S_get_select_offset)

    /* Check args */
    HDassert(space);
    HDassert(offset);

    ret_value = (*space->select.type->offset)(space, offset);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_get_select_offset() */


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
    herr_t ret_value;        /* return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5S_select_is_contiguous);

    /* Check args */
    assert(space);

    ret_value = (*space->select.type->is_contiguous)(space);

    FUNC_LEAVE_NOAPI(ret_value);
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
    herr_t ret_value;        /* return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5S_select_is_single);

    /* Check args */
    assert(space);

    ret_value = (*space->select.type->is_single)(space);

    FUNC_LEAVE_NOAPI(ret_value);
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
    herr_t ret_value;        /* return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5S_select_is_regular);

    /* Check args */
    assert(space);

    ret_value = (*space->select.type->is_regular)(space);

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5S_select_is_regular() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_adjust_u
 PURPOSE
    Adjust a selection by subtracting an offset
 USAGE
    herr_t H5S_select_adjust_u(space, offset)
        H5S_t *space;           IN/OUT: Pointer to dataspace to adjust
        const hsize_t *offset; IN: Offset to subtract
 RETURNS
    Non-negative on success, negative on failure
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
herr_t
H5S_select_adjust_u(H5S_t *space, const hsize_t *offset)
{
    herr_t ret_value;        /* return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5S_select_adjust_u)

    /* Check args */
    HDassert(space);

    ret_value = (*space->select.type->adjust_u)(space, offset);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_adjust_u() */


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
    herr_t ret_value;   /* Return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5S_select_iter_init);

    /* Check args */
    assert(sel_iter);
    assert(space);

    /* Initialize common information */

    /* Save the dataspace's rank */
    sel_iter->rank=space->extent.rank;

    /* Point to the dataspace dimensions, if there are any */
    if(sel_iter->rank > 0)
        sel_iter->dims = space->extent.size;
    else
        sel_iter->dims = NULL;

    /* Save the element size */
    sel_iter->elmt_size=elmt_size;

    /* Call initialization routine for selection type */
    ret_value= (*space->select.type->iter_init)(sel_iter, space);

    FUNC_LEAVE_NOAPI(ret_value);
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
H5S_select_iter_coords (const H5S_sel_iter_t *sel_iter, hsize_t *coords)
{
    herr_t ret_value;         /* return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5S_select_iter_coords);

    /* Check args */
    assert(sel_iter);
    assert(coords);

    /* Call iter_coords routine for selection type */
    ret_value = (*sel_iter->type->iter_coords)(sel_iter,coords);

    FUNC_LEAVE_NOAPI(ret_value);
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
H5S_select_iter_block (const H5S_sel_iter_t *iter, hsize_t *start, hsize_t *end)
{
    herr_t ret_value;         /* return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5S_select_iter_block);

    /* Check args */
    assert(iter);
    assert(start);
    assert(end);

    /* Call iter_block routine for selection type */
    ret_value = (*iter->type->iter_block)(iter,start,end);

    FUNC_LEAVE_NOAPI(ret_value);
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
H5S_select_iter_nelmts (const H5S_sel_iter_t *sel_iter)
{
    hsize_t ret_value;         /* return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5S_select_iter_nelmts);

    /* Check args */
    assert(sel_iter);

    /* Call iter_nelmts routine for selection type */
    ret_value = (*sel_iter->type->iter_nelmts)(sel_iter);

    FUNC_LEAVE_NOAPI(ret_value);
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
H5S_select_iter_has_next_block (const H5S_sel_iter_t *iter)
{
    herr_t ret_value;         /* return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5S_select_iter_has_next_block);

    /* Check args */
    assert(iter);

    /* Call iter_has_next_block routine for selection type */
    ret_value = (*iter->type->iter_has_next_block)(iter);

    FUNC_LEAVE_NOAPI(ret_value);
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
        size_t nelem;           IN: Number of elements to advance by
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
H5S_select_iter_next(H5S_sel_iter_t *iter, size_t nelem)
{
    herr_t ret_value;         /* return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5S_select_iter_next);

    /* Check args */
    assert(iter);
    assert(nelem>0);

    /* Call iter_next routine for selection type */
    ret_value = (*iter->type->iter_next)(iter,nelem);

    /* Decrement the number of elements left in selection */
    iter->elmt_left-=nelem;

    FUNC_LEAVE_NOAPI(ret_value);
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

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5S_select_iter_next_block);

    /* Check args */
    assert(iter);

    /* Call iter_next_block routine for selection type */
    ret_value = (*iter->type->iter_next_block)(iter);

    FUNC_LEAVE_NOAPI(ret_value);
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
    herr_t ret_value;         /* return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5S_select_iter_release);

    /* Check args */
    assert(sel_iter);

    /* Call selection type-specific release routine */
    ret_value = (*sel_iter->type->iter_release)(sel_iter);

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5S_select_iter_release() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_iterate
 PURPOSE
    Iterate over the selected elements in a memory buffer.
 USAGE
    herr_t H5S_select_iterate(buf, type_id, space, operator, operator_data)
        void *buf;      IN/OUT: Buffer containing elements to iterate over
        hid_t type_id;  IN: Datatype ID of BUF array.
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
H5S_select_iterate(void *buf, hid_t type_id, const H5S_t *space, H5D_operator_t op,
        void *operator_data)
{
    H5T_t *dt;                  /* Datatype structure */
    H5S_sel_iter_t iter;        /* Selection iteration info */
    hbool_t iter_init = FALSE;  /* Selection iteration info has been initialized */
    uint8_t *loc;               /* Current element location in buffer */
    hsize_t coords[H5O_LAYOUT_NDIMS];  /* Coordinates of element in dataspace */
    hssize_t nelmts;            /* Number of elements in selection */
    hsize_t space_size[H5O_LAYOUT_NDIMS]; /* Dataspace size */
    hsize_t off[H5D_IO_VECTOR_SIZE];          /* Array to store sequence offsets */
    hsize_t curr_off;           /* Current offset within sequence */
    hsize_t tmp_off;            /* Temporary offset within sequence */
    size_t len[H5D_IO_VECTOR_SIZE];           /* Array to store sequence lengths */
    size_t curr_len;            /* Length of bytes left to process in sequence */
    size_t nseq;                /* Number of sequences generated */
    size_t curr_seq;            /* Current sequnce being worked on */
    size_t nelem;               /* Number of elements used in sequences */
    size_t max_elem;            /* Maximum number of elements allowed in sequences */
    size_t elmt_size;           /* Datatype size */
    unsigned ndims;             /* Number of dimensions in dataspace */
    int	i;			/* Local Index variable */
    herr_t user_ret=0;          /* User's return value */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5S_select_iterate, FAIL);

    /* Check args */
    HDassert(buf);
    HDassert(H5I_DATATYPE == H5I_get_type(type_id));
    HDassert(space);
    HDassert(op);

    /* Get the datatype size */
    if(NULL == (dt = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an valid base datatype")
    if(0 == (elmt_size = H5T_get_size(dt)))
        HGOTO_ERROR(H5E_DATATYPE, H5E_BADSIZE, FAIL, "datatype size invalid")

    /* Initialize iterator */
    if(H5S_select_iter_init(&iter, space, elmt_size) < 0)
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
    H5_ASSIGN_OVERFLOW(max_elem, nelmts, hssize_t, size_t);

    /* Loop, while elements left in selection */
    while(max_elem > 0 && user_ret == 0) {
        /* Get the sequences of bytes */
        if(H5S_SELECT_GET_SEQ_LIST(space, 0, &iter, (size_t)H5D_IO_VECTOR_SIZE, max_elem, &nseq, &nelem, off, len) < 0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_UNSUPPORTED, FAIL, "sequence length generation failed")

        /* Loop, while sequences left to process */
        for(curr_seq=0; curr_seq<nseq && user_ret==0; curr_seq++) {
            /* Get the current offset */
            curr_off = off[curr_seq];

            /* Get the number of bytes in sequence */
            curr_len = len[curr_seq];

            /* Loop, while bytes left in sequence */
            while(curr_len > 0 && user_ret == 0) {
                /* Compute the coordinate from the offset */
                for(i = (int)ndims, tmp_off = curr_off; i >= 0; i--) {
                    coords[i] = tmp_off % space_size[i];
                    tmp_off /= space_size[i];
                } /* end for */

                /* Get the location within the user's buffer */
                loc = (unsigned char *)buf + curr_off;

                /* Call user's callback routine */
                user_ret=(*op)(loc,type_id,ndims,coords,operator_data);

                /* Increment offset in dataspace */
                curr_off+=elmt_size;

                /* Decrement number of bytes left in sequence */
                curr_len-=elmt_size;
            } /* end while */
        } /* end for */

        /* Decrement number of elements left to process */
        max_elem-=nelem;
    } /* end while */

    /* Set return value */
    ret_value=user_ret;

done:
    /* Release selection iterator */
    if(iter_init && H5S_SELECT_ITER_RELEASE(&iter) < 0)
        HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator")

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

    FUNC_ENTER_API(H5Sget_select_type, H5S_SEL_ERROR)
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
    H5S_sel_type        ret_value;       /* Return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5S_get_select_type);

    /* Check args */
    assert(space);

    /* Set return value */
    ret_value=H5S_GET_SELECT_TYPE(space);

    FUNC_LEAVE_NOAPI(ret_value);
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
--------------------------------------------------------------------------*/
htri_t
H5S_select_shape_same(const H5S_t *space1, const H5S_t *space2)
{
    H5S_sel_iter_t iter1;               /* Selection #1 iteration info */
    H5S_sel_iter_t iter2;               /* Selection #2 iteration info */
    hbool_t iter1_init = 0;             /* Selection #1 iteration info has been initialized */
    hbool_t iter2_init = 0;             /* Selection #2 iteration info has been initialized */
    unsigned u;                         /* Index variable */
    htri_t ret_value = TRUE;            /* Return value */

    FUNC_ENTER_NOAPI(H5S_select_shape_same, FAIL)

    /* Check args */
    HDassert(space1);
    HDassert(space2);

    /* Special case for one or both dataspaces being scalar */
    if(space1->extent.rank == 0 || space2->extent.rank == 0) {
        /* Check for different number of elements selected */
        if(H5S_GET_SELECT_NPOINTS(space1) != H5S_GET_SELECT_NPOINTS(space2))
            HGOTO_DONE(FALSE)
    } /* end if */
    else {
        /* Check for different dimensionality */
        if(space1->extent.rank != space2->extent.rank)
            HGOTO_DONE(FALSE)

        /* Check for different number of elements selected */
        if(H5S_GET_SELECT_NPOINTS(space1) != H5S_GET_SELECT_NPOINTS(space2))
            HGOTO_DONE(FALSE)

        /* Check for "easy" cases before getting into generalized block iteration code */
        if(H5S_GET_SELECT_TYPE(space1)==H5S_SEL_ALL && H5S_GET_SELECT_TYPE(space2)==H5S_SEL_ALL) {
            hsize_t dims1[H5O_LAYOUT_NDIMS];             /* End point of selection block in dataspace #1 */
            hsize_t dims2[H5O_LAYOUT_NDIMS];             /* End point of selection block in dataspace #2 */

            if(H5S_get_simple_extent_dims(space1, dims1, NULL)<0)
                HGOTO_ERROR (H5E_DATASPACE, H5E_CANTGET, FAIL, "unable to get dimensionality");
            if(H5S_get_simple_extent_dims(space2, dims2, NULL)<0)
                HGOTO_ERROR (H5E_DATASPACE, H5E_CANTGET, FAIL, "unable to get dimensionality");

            /* Check that the sizes are the same */
            for (u=0; u<space1->extent.rank; u++)
                if(dims1[u]!=dims2[u])
                    HGOTO_DONE(FALSE);
        } /* end if */
        else if(H5S_GET_SELECT_TYPE(space1)==H5S_SEL_NONE || H5S_GET_SELECT_TYPE(space2)==H5S_SEL_NONE) {
            HGOTO_DONE(TRUE);
        } /* end if */
        else if((H5S_GET_SELECT_TYPE(space1)==H5S_SEL_HYPERSLABS && space1->select.sel_info.hslab->diminfo_valid)
                && (H5S_GET_SELECT_TYPE(space2)==H5S_SEL_HYPERSLABS && space2->select.sel_info.hslab->diminfo_valid)) {

            /* Check that the shapes are the same */
            for (u=0; u<space1->extent.rank; u++) {
                if(space1->select.sel_info.hslab->opt_diminfo[u].stride!=space2->select.sel_info.hslab->opt_diminfo[u].stride)
                    HGOTO_DONE(FALSE);
                if(space1->select.sel_info.hslab->opt_diminfo[u].count!=space2->select.sel_info.hslab->opt_diminfo[u].count)
                    HGOTO_DONE(FALSE);
                if(space1->select.sel_info.hslab->opt_diminfo[u].block!=space2->select.sel_info.hslab->opt_diminfo[u].block)
                    HGOTO_DONE(FALSE);
            } /* end for */
        } /* end if */
        /* Iterate through all the blocks in the selection */
        else {
            hsize_t start1[H5O_LAYOUT_NDIMS];      /* Start point of selection block in dataspace #1 */
            hsize_t start2[H5O_LAYOUT_NDIMS];      /* Start point of selection block in dataspace #2 */
            hsize_t end1[H5O_LAYOUT_NDIMS];        /* End point of selection block in dataspace #1 */
            hsize_t end2[H5O_LAYOUT_NDIMS];        /* End point of selection block in dataspace #2 */
            hsize_t off1[H5O_LAYOUT_NDIMS];        /* Offset of selection #1 blocks */
            hsize_t off2[H5O_LAYOUT_NDIMS];        /* Offset of selection #2 blocks */
            htri_t status1,status2;         /* Status from next block checks */
            unsigned first_block=1;         /* Flag to indicate the first block */

            /* Initialize iterator for each dataspace selection
             * Use '0' for element size instead of actual element size to indicate
             * that the selection iterator shouldn't be "flattened", since we
             * aren't actually going to be doing I/O with the iterators.
             */
            if(H5S_select_iter_init(&iter1, space1, (size_t)0) < 0)
                HGOTO_ERROR (H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator");
            iter1_init = 1;
            if(H5S_select_iter_init(&iter2, space2, (size_t)0) < 0)
                HGOTO_ERROR (H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator");
            iter2_init = 1;

            /* Iterate over all the blocks in each selection */
            while(1) {
                /* Get the current block for each selection iterator */
                if(H5S_SELECT_ITER_BLOCK(&iter1,start1,end1)<0)
                    HGOTO_ERROR (H5E_DATASPACE, H5E_CANTGET, FAIL, "unable to get iterator block");
                if(H5S_SELECT_ITER_BLOCK(&iter2,start2,end2)<0)
                    HGOTO_ERROR (H5E_DATASPACE, H5E_CANTGET, FAIL, "unable to get iterator block");

                /* The first block only compares the sizes and sets the relative offsets for later blocks */
                if(first_block) {
                    /* If the block sizes from each selection doesn't match, get out */
                    for (u=0; u<space1->extent.rank; u++) {
                        if((end1[u]-start1[u])!=(end2[u]-start2[u]))
                            HGOTO_DONE(FALSE);

                        /* Set the relative locations of the selections */
                        off1[u]=start1[u];
                        off2[u]=start2[u];
                    } /* end for */

                    /* Reset "first block" flag */
                    first_block=0;
                } /* end if */
                else {
                    /* Check over the blocks for each selection */
                    for (u=0; u<space1->extent.rank; u++) {
                        /* Check if the blocks are in the same relative location */
                        if((start1[u]-off1[u])!=(start2[u]-off2[u]))
                            HGOTO_DONE(FALSE);

                        /* If the block sizes from each selection doesn't match, get out */
                        if((end1[u]-start1[u])!=(end2[u]-start2[u]))
                            HGOTO_DONE(FALSE);
                    } /* end for */
                } /* end else */

                /* Check if we are able to advance to the next selection block */
                if((status1=H5S_SELECT_ITER_HAS_NEXT_BLOCK(&iter1))<0)
                    HGOTO_ERROR (H5E_DATASPACE, H5E_CANTNEXT, FAIL, "unable to check iterator block");
                if((status2=H5S_SELECT_ITER_HAS_NEXT_BLOCK(&iter2))<0)
                    HGOTO_ERROR (H5E_DATASPACE, H5E_CANTNEXT, FAIL, "unable to check iterator block");

                /* Did we run out of blocks at the same time? */
                if(status1==FALSE && status2==FALSE)
                    break;
                else if(status1!=status2) {
                    HGOTO_DONE(FALSE);
                } /* end if */
                else {
                    /* Advance to next block in selection iterators */
                    if(H5S_SELECT_ITER_NEXT_BLOCK(&iter1)<0)
                        HGOTO_ERROR (H5E_DATASPACE, H5E_CANTNEXT, FAIL, "unable to advance to next iterator block");
                    if(H5S_SELECT_ITER_NEXT_BLOCK(&iter2)<0)
                        HGOTO_ERROR (H5E_DATASPACE, H5E_CANTNEXT, FAIL, "unable to advance to next iterator block");
                } /* end else */
            } /* end while */
        } /* end else */
    } /* end else */

done:
    if(iter1_init) {
        if (H5S_SELECT_ITER_RELEASE(&iter1)<0)
            HDONE_ERROR (H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator");
    } /* end if */
    if(iter2_init) {
        if (H5S_SELECT_ITER_RELEASE(&iter2)<0)
            HDONE_ERROR (H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator");
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_shape_same() */


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
    H5S_sel_iter_t iter;        /* Selection iteration info */
    hbool_t iter_init = 0;      /* Selection iteration info has been initialized */
    hssize_t nelmts;            /* Number of elements in selection */
    size_t max_elem;            /* Total number of elements in selection */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(H5S_select_fill, FAIL)

    /* Check args */
    HDassert(fill);
    HDassert(fill_size > 0);
    HDassert(space);
    HDassert(_buf);

    /* Initialize iterator */
    if(H5S_select_iter_init(&iter, space, fill_size) < 0)
        HGOTO_ERROR (H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator")
    iter_init = 1;	/* Selection iteration info has been initialized */

    /* Get the number of elements in selection */
    if((nelmts = (hssize_t)H5S_GET_SELECT_NPOINTS(space)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOUNT, FAIL, "can't get number of elements selected")

    /* Compute the number of bytes to process */
    H5_ASSIGN_OVERFLOW(max_elem, nelmts, hssize_t, size_t);

    /* Loop, while elements left in selection */
    while(max_elem > 0) {
        hsize_t off[H5D_IO_VECTOR_SIZE];          /* Array to store sequence offsets */
        size_t len[H5D_IO_VECTOR_SIZE];           /* Array to store sequence lengths */
        size_t nseq;                /* Number of sequences generated */
        size_t curr_seq;            /* Current sequnce being worked on */
        size_t nelem;               /* Number of elements used in sequences */

        /* Get the sequences of bytes */
        if(H5S_SELECT_GET_SEQ_LIST(space, 0, &iter, (size_t)H5D_IO_VECTOR_SIZE, max_elem, &nseq, &nelem, off, len) < 0)
            HGOTO_ERROR (H5E_INTERNAL, H5E_UNSUPPORTED, FAIL, "sequence length generation failed")

        /* Loop over sequences */
        for(curr_seq = 0; curr_seq < nseq; curr_seq++) {
            uint8_t *buf;               /* Current location in buffer */

            /* Get offset in memory buffer */
            buf = (uint8_t *)_buf + off[curr_seq];

            /* Fill each sequence in memory with fill value */
            HDassert((len[curr_seq] % fill_size) == 0);
            H5V_array_fill(buf, fill, fill_size, (len[curr_seq] / fill_size));
        } /* end for */

        /* Decrement number of elements left to process */
        max_elem -= nelem;
    } /* end while */

done:
    /* Release resouces */
    if(iter_init && H5S_SELECT_ITER_RELEASE(&iter) < 0)
        HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator")

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_fill() */

