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
 * Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              Tuesday, June 16, 1998
 *
 * Purpose:	"All" selection data space I/O functions.
 */

#define H5S_PACKAGE		/*suppress error about including H5Spkg	  */


#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		        */
#include "H5Iprivate.h"		/* ID Functions		                */
#include "H5Spkg.h"		/* Dataspace functions			*/
#include "H5VMprivate.h"         /* Vector functions                     */

/* Static function prototypes */

/* Selection callbacks */
static herr_t H5S_all_copy(H5S_t *dst, const H5S_t *src, hbool_t share_selection);
static herr_t H5S_all_get_seq_list(const H5S_t *space, unsigned flags,
    H5S_sel_iter_t *iter, size_t maxseq, size_t maxbytes,
    size_t *nseq, size_t *nbytes, hsize_t *off, size_t *len);
static herr_t H5S_all_release(H5S_t *space);
static htri_t H5S_all_is_valid(const H5S_t *space);
static hssize_t H5S_all_serial_size(const H5S_t *space);
static herr_t H5S_all_serialize(const H5S_t *space, uint8_t *buf);
static herr_t H5S_all_deserialize(H5S_t *space, const uint8_t *buf);
static herr_t H5S_all_bounds(const H5S_t *space, hsize_t *start, hsize_t *end);
static herr_t H5S_all_offset(const H5S_t *space, hsize_t *off);
static htri_t H5S_all_is_contiguous(const H5S_t *space);
static htri_t H5S_all_is_single(const H5S_t *space);
static htri_t H5S_all_is_regular(const H5S_t *space);
static herr_t H5S_all_adjust_u(H5S_t *space, const hsize_t *offset);
static herr_t H5S_all_project_scalar(const H5S_t *space, hsize_t *offset);
static herr_t H5S_all_project_simple(const H5S_t *space, H5S_t *new_space, hsize_t *offset);
static herr_t H5S_all_iter_init(H5S_sel_iter_t *iter, const H5S_t *space);

/* Selection iteration callbacks */
static herr_t H5S_all_iter_coords(const H5S_sel_iter_t *iter, hsize_t *coords);
static herr_t H5S_all_iter_block(const H5S_sel_iter_t *iter, hsize_t *start, hsize_t *end);
static hsize_t H5S_all_iter_nelmts(const H5S_sel_iter_t *iter);
static htri_t H5S_all_iter_has_next_block(const H5S_sel_iter_t *iter);
static herr_t H5S_all_iter_next(H5S_sel_iter_t *sel_iter, size_t nelem);
static herr_t H5S_all_iter_next_block(H5S_sel_iter_t *sel_iter);
static herr_t H5S_all_iter_release(H5S_sel_iter_t *sel_iter);

/* Selection properties for "all" selections */
const H5S_select_class_t H5S_sel_all[1] = {{
    H5S_SEL_ALL,

    /* Methods on selection */
    H5S_all_copy,
    H5S_all_get_seq_list,
    H5S_all_release,
    H5S_all_is_valid,
    H5S_all_serial_size,
    H5S_all_serialize,
    H5S_all_deserialize,
    H5S_all_bounds,
    H5S_all_offset,
    H5S_all_is_contiguous,
    H5S_all_is_single,
    H5S_all_is_regular,
    H5S_all_adjust_u,
    H5S_all_project_scalar,
    H5S_all_project_simple,
    H5S_all_iter_init,
}};

/* Iteration properties for "all" selections */
static const H5S_sel_iter_class_t H5S_sel_iter_all[1] = {{
    H5S_SEL_ALL,

    /* Methods on selection iterator */
    H5S_all_iter_coords,
    H5S_all_iter_block,
    H5S_all_iter_nelmts,
    H5S_all_iter_has_next_block,
    H5S_all_iter_next,
    H5S_all_iter_next_block,
    H5S_all_iter_release,
}};


/*-------------------------------------------------------------------------
 * Function:	H5S_all_iter_init
 *
 * Purpose:	Initializes iteration information for "all" selection.
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
H5S_all_iter_init (H5S_sel_iter_t *iter, const H5S_t *space)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert (space && H5S_SEL_ALL==H5S_GET_SELECT_TYPE(space));
    HDassert (iter);

    /* Initialize the number of elements to iterate over */
    iter->elmt_left=H5S_GET_SELECT_NPOINTS(space);

    /* Start at the upper left location */
    iter->u.all.elmt_offset=0;
    iter->u.all.byte_offset=0;

    /* Initialize type of selection iterator */
    iter->type=H5S_sel_iter_all;

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_all_iter_init() */


/*-------------------------------------------------------------------------
 * Function:	H5S_all_iter_coords
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
H5S_all_iter_coords (const H5S_sel_iter_t *iter, hsize_t *coords)
{
    herr_t ret_value=SUCCEED;        /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check args */
    HDassert (iter);
    HDassert (coords);

    /* Calculate the coordinates for the current iterator offset */
    if(H5VM_array_calc(iter->u.all.elmt_offset,iter->rank,iter->dims,coords)<0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't retrieve coordinates");

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_all_iter_coords() */


/*-------------------------------------------------------------------------
 * Function:	H5S_all_iter_block
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
H5S_all_iter_block (const H5S_sel_iter_t *iter, hsize_t *start, hsize_t *end)
{
    unsigned u;                 /* Local index variable */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert (iter);
    HDassert (start);
    HDassert (end);

    for(u=0; u<iter->rank; u++) {
        /* Set the start of the 'all' block */
        /* (Always '0' coordinates for now) */
        start[u]=0;

        /* Compute the end of the 'all' block */
        /* (Always size of the extent for now) */
        end[u]=iter->dims[u]-1;
    } /* end for */

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_all_iter_coords() */


/*-------------------------------------------------------------------------
 * Function:	H5S_all_iter_nelmts
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
H5S_all_iter_nelmts (const H5S_sel_iter_t *iter)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert (iter);

    FUNC_LEAVE_NOAPI(iter->elmt_left)
}   /* H5S_all_iter_nelmts() */


/*--------------------------------------------------------------------------
 NAME
    H5S_all_iter_next
 PURPOSE
    Check if there is another block left in the current iterator
 USAGE
    htri_t H5S_all_iter_has_next_block(iter)
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
H5S_all_iter_has_next_block (const H5S_sel_iter_t UNUSED *iter)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert (iter);

    FUNC_LEAVE_NOAPI(FALSE)
}   /* H5S_all_iter_has_next_block() */


/*--------------------------------------------------------------------------
 NAME
    H5S_all_iter_next
 PURPOSE
    Increment selection iterator
 USAGE
    herr_t H5S_all_iter_next(iter, nelem)
        H5S_sel_iter_t *iter;       IN: Pointer to selection iterator
        size_t nelem;               IN: Number of elements to advance by
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
H5S_all_iter_next(H5S_sel_iter_t *iter, size_t nelem)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert (iter);
    HDassert (nelem>0);

    /* Increment the iterator */
    iter->u.all.elmt_offset+=nelem;
    iter->u.all.byte_offset+=(nelem*iter->elmt_size);

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_all_iter_next() */


/*--------------------------------------------------------------------------
 NAME
    H5S_all_iter_next_block
 PURPOSE
    Increment selection iterator to next block
 USAGE
    herr_t H5S_all_iter_next_block(iter)
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
H5S_all_iter_next_block(H5S_sel_iter_t UNUSED *iter)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert (iter);

    FUNC_LEAVE_NOAPI(FAIL)
}   /* H5S_all_iter_next_block() */


/*--------------------------------------------------------------------------
 NAME
    H5S_all_iter_release
 PURPOSE
    Release "all" selection iterator information for a dataspace
 USAGE
    herr_t H5S_all_iter_release(iter)
        H5S_sel_iter_t *iter;       IN: Pointer to selection iterator
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Releases all information for a dataspace "all" selection iterator
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_all_iter_release (H5S_sel_iter_t UNUSED * iter)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert (iter);

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_all_iter_release() */


/*--------------------------------------------------------------------------
 NAME
    H5S_all_release
 PURPOSE
    Release all selection information for a dataspace
 USAGE
    herr_t H5S_all_release(space)
        H5S_t *space;       IN: Pointer to dataspace
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Releases "all" selection information for a dataspace
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_all_release(H5S_t *space)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);

    /* Reset the number of elements in the selection */
    space->select.num_elem = 0;

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_all_release() */


/*--------------------------------------------------------------------------
 NAME
    H5S_all_copy
 PURPOSE
    Copy a selection from one dataspace to another
 USAGE
    herr_t H5S_all_copy(dst, src)
        H5S_t *dst;  OUT: Pointer to the destination dataspace
        H5S_t *src;  IN: Pointer to the source dataspace
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Copies the 'all' selection information from the source
    dataspace to the destination dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_all_copy(H5S_t *dst, const H5S_t UNUSED *src, hbool_t UNUSED share_selection)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(src);
    HDassert(dst);

    /* Set number of elements in selection */
    dst->select.num_elem = (hsize_t)H5S_GET_EXTENT_NPOINTS(dst);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5S_all_copy() */


/*--------------------------------------------------------------------------
 NAME
    H5S_all_is_valid
 PURPOSE
    Check whether the selection fits within the extent, with the current
    offset defined.
 USAGE
    htri_t H5S_all_is_valid(space);
        H5S_t *space;             IN: Dataspace pointer to query
 RETURNS
    TRUE if the selection fits within the extent, FALSE if it does not and
        Negative on an error.
 DESCRIPTION
    Determines if the current selection at the current offet fits within the
    extent for the dataspace.  Offset is irrelevant for this type of selection.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static htri_t
H5S_all_is_valid (const H5S_t UNUSED *space)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);

    FUNC_LEAVE_NOAPI(TRUE)
} /* end H5S_all_is_valid() */


/*--------------------------------------------------------------------------
 NAME
    H5S_all_serial_size
 PURPOSE
    Determine the number of bytes needed to store the serialized "all"
        selection information.
 USAGE
    hssize_t H5S_all_serial_size(space)
        H5S_t *space;             IN: Dataspace pointer to query
 RETURNS
    The number of bytes required on success, negative on an error.
 DESCRIPTION
    Determines the number of bytes required to serialize an "all"
    selection for storage on disk.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static hssize_t
H5S_all_serial_size (const H5S_t UNUSED *space)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);

    /* Basic number of bytes required to serialize point selection:
     *  <type (4 bytes)> + <version (4 bytes)> + <padding (4 bytes)> +
     *      <length (4 bytes)> = 16 bytes
     */
    FUNC_LEAVE_NOAPI(16)
} /* end H5S_all_serial_size() */


/*--------------------------------------------------------------------------
 NAME
    H5S_all_serialize
 PURPOSE
    Serialize the current selection into a user-provided buffer.
 USAGE
    herr_t H5S_all_serialize(space, buf)
        H5S_t *space;           IN: Dataspace pointer of selection to serialize
        uint8 *buf;             OUT: Buffer to put serialized selection into
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
H5S_all_serialize (const H5S_t *space, uint8_t *buf)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);

    /* Store the preamble information */
    UINT32ENCODE(buf, (uint32_t)H5S_GET_SELECT_TYPE(space));  /* Store the type of selection */
    UINT32ENCODE(buf, (uint32_t)1);  /* Store the version number */
    UINT32ENCODE(buf, (uint32_t)0);  /* Store the un-used padding */
    UINT32ENCODE(buf, (uint32_t)0);  /* Store the additional information length */

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_all_serialize() */


/*--------------------------------------------------------------------------
 NAME
    H5S_all_deserialize
 PURPOSE
    Deserialize the current selection from a user-provided buffer.
 USAGE
    herr_t H5S_all_deserialize(space, buf)
        H5S_t *space;           IN/OUT: Dataspace pointer to place selection into
        uint8 *buf;             IN: Buffer to retrieve serialized selection from
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
H5S_all_deserialize(H5S_t *space, const uint8_t UNUSED *buf)
{
    herr_t ret_value;   /* return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(space);

    /* Change to "all" selection */
    if((ret_value = H5S_select_all(space, TRUE)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't change selection")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_all_deserialize() */


/*--------------------------------------------------------------------------
 NAME
    H5S_all_bounds
 PURPOSE
    Gets the bounding box containing the selection.
 USAGE
    herr_t H5S_all_bounds(space, start, end)
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
    with be (4, 5), (10, 8).  Calling this function on a "none" selection
    returns fail.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_all_bounds(const H5S_t *space, hsize_t *start, hsize_t *end)
{
    unsigned rank;                  /* Dataspace rank */
    unsigned i;                     /* index variable */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);
    HDassert(start);
    HDassert(end);

    /* Get the dataspace extent rank */
    rank = space->extent.rank;

    /* Just copy over the complete extent */
    for(i = 0; i < rank; i++) {
        start[i] = 0;
        end[i] = space->extent.size[i] - 1;
    } /* end for */

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_all_bounds() */


/*--------------------------------------------------------------------------
 NAME
    H5S_all_offset
 PURPOSE
    Gets the linear offset of the first element for the selection.
 USAGE
    herr_t H5S_all_offset(space, offset)
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
H5S_all_offset(const H5S_t UNUSED *space, hsize_t *offset)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);
    HDassert(offset);

    /* 'All' selections always start at offset 0 */
    *offset = 0;

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_all_offset() */


/*--------------------------------------------------------------------------
 NAME
    H5S_all_is_contiguous
 PURPOSE
    Check if a "all" selection is contiguous within the dataspace extent.
 USAGE
    htri_t H5S_all_is_contiguous(space)
        H5S_t *space;           IN: Dataspace pointer to check
 RETURNS
    TRUE/FALSE/FAIL
 DESCRIPTION
    Checks to see if the current selection in the dataspace is contiguous.
    This is primarily used for reading the entire selection in one swoop.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static htri_t
H5S_all_is_contiguous(const H5S_t UNUSED *space)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);

    FUNC_LEAVE_NOAPI(TRUE)
}   /* H5S_all_is_contiguous() */


/*--------------------------------------------------------------------------
 NAME
    H5S_all_is_single
 PURPOSE
    Check if a "all" selection is a single block within the dataspace extent.
 USAGE
    htri_t H5S_all_is_single(space)
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
H5S_all_is_single(const H5S_t UNUSED *space)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);

    FUNC_LEAVE_NOAPI(TRUE)
}   /* H5S_all_is_single() */


/*--------------------------------------------------------------------------
 NAME
    H5S_all_is_regular
 PURPOSE
    Check if a "all" selection is "regular"
 USAGE
    htri_t H5S_all_is_regular(space)
        const H5S_t *space;     IN: Dataspace pointer to check
 RETURNS
    TRUE/FALSE/FAIL
 DESCRIPTION
    Checks to see if the current selection in a dataspace is the a regular
    pattern.
    This is primarily used for reading the entire selection in one swoop.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static htri_t
H5S_all_is_regular(const H5S_t UNUSED *space)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);

    FUNC_LEAVE_NOAPI(TRUE)
}   /* H5S_all_is_regular() */


/*--------------------------------------------------------------------------
 NAME
    H5S_all_adjust_u
 PURPOSE
    Adjust an "all" selection by subtracting an offset
 USAGE
    herr_t H5S_all_adjust_u(space, offset)
        H5S_t *space;           IN/OUT: Pointer to dataspace to adjust
        const hsize_t *offset; IN: Offset to subtract
 RETURNS
    Non-negative on success, negative on failure
 DESCRIPTION
    Moves selection by subtracting an offset from it.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_all_adjust_u(H5S_t UNUSED *space, const hsize_t UNUSED *offset)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);
    HDassert(offset);

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_all_adjust_u() */


/*-------------------------------------------------------------------------
 * Function:	H5S_all_project_scalar
 *
 * Purpose:	Projects a single element 'all' selection into a scalar
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
H5S_all_project_scalar(const H5S_t UNUSED *space, hsize_t *offset)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space && H5S_SEL_ALL == H5S_GET_SELECT_TYPE(space));
    HDassert(offset);

    /* Set offset of selection in projected buffer */
    *offset = 0;

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_all_project_scalar() */


/*-------------------------------------------------------------------------
 * Function:	H5S_all_project_simple
 *
 * Purpose:	Projects an 'all' selection onto/into a simple dataspace
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
H5S_all_project_simple(const H5S_t *base_space, H5S_t *new_space, hsize_t *offset)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check args */
    HDassert(base_space && H5S_SEL_ALL == H5S_GET_SELECT_TYPE(base_space));
    HDassert(new_space);
    HDassert(offset);

    /* Select the entire new space */
    if(H5S_select_all(new_space, TRUE) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSET, FAIL, "unable to set all selection")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_all_project_simple() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_all
 PURPOSE
    Specify the the entire extent is selected
 USAGE
    herr_t H5S_select_all(dsid)
        hid_t dsid;             IN: Dataspace ID of selection to modify
        hbool_t rel_prev;      IN: Flag whether to release previous selection or not
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    This function selects the entire extent for a dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_all(H5S_t *space, hbool_t rel_prev)
{
    herr_t ret_value = SUCCEED;  /* return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(space);

    /* Remove current selection first */
    if(rel_prev)
        if(H5S_SELECT_RELEASE(space) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't release selection")

    /* Set number of elements in selection */
    space->select.num_elem = (hsize_t)H5S_GET_EXTENT_NPOINTS(space);

    /* Set selection type */
    space->select.type = H5S_sel_all;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_all() */


/*--------------------------------------------------------------------------
 NAME
    H5Sselect_all
 PURPOSE
    Specify the the entire extent is selected
 USAGE
    herr_t H5Sselect_all(dsid)
        hid_t dsid;             IN: Dataspace ID of selection to modify
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    This function selects the entire extent for a dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Sselect_all(hid_t spaceid)
{
    H5S_t *space;               /* Dataspace to modify selection of */
    herr_t ret_value = SUCCEED; /* return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", spaceid);

    /* Check args */
    if(NULL == (space = (H5S_t *)H5I_object_verify(spaceid, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space")

    /* Call internal routine to do the work */
    if(H5S_select_all(space, TRUE) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't change selection")

done:
    FUNC_LEAVE_API(ret_value)
}   /* H5Sselect_all() */


/*--------------------------------------------------------------------------
 NAME
    H5S_all_get_seq_list
 PURPOSE
    Create a list of offsets & lengths for a selection
 USAGE
    herr_t H5S_all_get_seq_list(space,flags,iter,maxseq,maxelem,nseq,nelem,off,len)
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
H5S_all_get_seq_list(const H5S_t UNUSED *space, unsigned UNUSED flags, H5S_sel_iter_t *iter,
    size_t UNUSED maxseq, size_t maxelem, size_t *nseq, size_t *nelem,
    hsize_t *off, size_t *len)
{
    size_t elem_used;           /* The number of elements used */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);
    HDassert(iter);
    HDassert(maxseq>0);
    HDassert(maxelem>0);
    HDassert(nseq);
    HDassert(nelem);
    HDassert(off);
    HDassert(len);

    /* Determine the actual number of elements to use */
    H5_CHECK_OVERFLOW(iter->elmt_left,hsize_t,size_t);
    elem_used=MIN(maxelem,(size_t)iter->elmt_left);

    /* Compute the offset in the dataset */
    off[0]=iter->u.all.byte_offset;
    len[0]=elem_used*iter->elmt_size;

    /* Should only need one sequence for 'all' selections */
    *nseq=1;

    /* Set the number of elements used */
    *nelem=elem_used;

    /* Update the iterator */
    iter->elmt_left-=elem_used;
    iter->u.all.elmt_offset+=elem_used;
    iter->u.all.byte_offset+=len[0];

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5S_all_get_seq_list() */

