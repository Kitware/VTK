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
 *              Tuesday, November 10, 1998
 *
 * Purpose:	"None" selection dataspace I/O functions.
 */

#include "H5Smodule.h"          /* This source code file is part of the H5S module */


#include "H5private.h"
#include "H5Eprivate.h"
#include "H5Iprivate.h"
#include "H5Spkg.h"
#include "H5VMprivate.h"
#include "H5Dprivate.h"

/* Static function prototypes */

/* Selection callbacks */
static herr_t H5S_none_copy(H5S_t *dst, const H5S_t *src, hbool_t share_selection);
static herr_t H5S_none_get_seq_list(const H5S_t *space, unsigned flags,
    H5S_sel_iter_t *iter, size_t maxseq, size_t maxbytes,
    size_t *nseq, size_t *nbytes, hsize_t *off, size_t *len);
static herr_t H5S_none_release(H5S_t *space);
static htri_t H5S_none_is_valid(const H5S_t *space);
static hssize_t H5S_none_serial_size(const H5S_t *space, H5F_t *f);
static herr_t H5S_none_serialize(const H5S_t *space, uint8_t **p, H5F_t *f);
static herr_t H5S_none_deserialize(H5S_t *space, uint32_t version, uint8_t flags,
    const uint8_t **p);
static herr_t H5S_none_bounds(const H5S_t *space, hsize_t *start, hsize_t *end);
static herr_t H5S_none_offset(const H5S_t *space, hsize_t *off);
static int H5S__none_unlim_dim(const H5S_t *space);
static htri_t H5S_none_is_contiguous(const H5S_t *space);
static htri_t H5S_none_is_single(const H5S_t *space);
static htri_t H5S_none_is_regular(const H5S_t *space);
static void H5S_none_adjust_u(H5S_t *space, const hsize_t *offset);
static herr_t H5S_none_project_scalar(const H5S_t *space, hsize_t *offset);
static herr_t H5S_none_project_simple(const H5S_t *space, H5S_t *new_space, hsize_t *offset);
static herr_t H5S_none_iter_init(H5S_sel_iter_t *iter, const H5S_t *space);

/* Selection iteration callbacks */
static herr_t H5S_none_iter_coords(const H5S_sel_iter_t *iter, hsize_t *coords);
static herr_t H5S_none_iter_block(const H5S_sel_iter_t *iter, hsize_t *start, hsize_t *end);
static hsize_t H5S_none_iter_nelmts(const H5S_sel_iter_t *iter);
static htri_t H5S_none_iter_has_next_block(const H5S_sel_iter_t *iter);
static herr_t H5S_none_iter_next(H5S_sel_iter_t *sel_iter, hsize_t nelem);
static herr_t H5S_none_iter_next_block(H5S_sel_iter_t *sel_iter);
static herr_t H5S_none_iter_release(H5S_sel_iter_t *sel_iter);

/* Selection properties for "none" selections */
const H5S_select_class_t H5S_sel_none[1] = {{
    H5S_SEL_NONE,

    /* Methods on selection */
    H5S_none_copy,
    H5S_none_get_seq_list,
    H5S_none_release,
    H5S_none_is_valid,
    H5S_none_serial_size,
    H5S_none_serialize,
    H5S_none_deserialize,
    H5S_none_bounds,
    H5S_none_offset,
    H5S__none_unlim_dim,
    NULL,
    H5S_none_is_contiguous,
    H5S_none_is_single,
    H5S_none_is_regular,
    H5S_none_adjust_u,
    H5S_none_project_scalar,
    H5S_none_project_simple,
    H5S_none_iter_init,
}};

/* Iteration properties for "none" selections */
static const H5S_sel_iter_class_t H5S_sel_iter_none[1] = {{
    H5S_SEL_NONE,

    /* Methods on selection iterator */
    H5S_none_iter_coords,
    H5S_none_iter_block,
    H5S_none_iter_nelmts,
    H5S_none_iter_has_next_block,
    H5S_none_iter_next,
    H5S_none_iter_next_block,
    H5S_none_iter_release,
}};


/*-------------------------------------------------------------------------
 * Function:	H5S_none_iter_init
 *
 * Purpose:	Initializes iteration information for "none" selection.
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
H5S_none_iter_init(H5S_sel_iter_t *iter, const H5S_t H5_ATTR_UNUSED *space)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space && H5S_SEL_NONE==H5S_GET_SELECT_TYPE(space));
    HDassert(iter);

    /* Initialize type of selection iterator */
    iter->type = H5S_sel_iter_none;

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_none_iter_init() */


/*-------------------------------------------------------------------------
 * Function:	H5S_none_iter_coords
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
H5S_none_iter_coords(const H5S_sel_iter_t H5_ATTR_UNUSED *iter, hsize_t H5_ATTR_UNUSED *coords)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(iter);
    HDassert(coords);

    FUNC_LEAVE_NOAPI(FAIL)
}   /* H5S_none_iter_coords() */


/*-------------------------------------------------------------------------
 * Function:	H5S_none_iter_block
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
H5S_none_iter_block(const H5S_sel_iter_t H5_ATTR_UNUSED *iter, hsize_t H5_ATTR_UNUSED *start, hsize_t H5_ATTR_UNUSED *end)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(iter);
    HDassert(start);
    HDassert(end);

    FUNC_LEAVE_NOAPI(FAIL)
}   /* H5S_none_iter_block() */


/*-------------------------------------------------------------------------
 * Function:	H5S_none_iter_nelmts
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
H5S_none_iter_nelmts(const H5S_sel_iter_t H5_ATTR_UNUSED *iter)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(iter);

    FUNC_LEAVE_NOAPI(0)
}   /* H5S_none_iter_nelmts() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_iter_has_next_block
 PURPOSE
    Check if there is another block left in the current iterator
 USAGE
    htri_t H5S_none_iter_has_next_block(iter)
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
H5S_none_iter_has_next_block(const H5S_sel_iter_t H5_ATTR_UNUSED *iter)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(iter);

    FUNC_LEAVE_NOAPI(FAIL)
}   /* H5S_none_iter_has_next_block() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_iter_next
 PURPOSE
    Increment selection iterator
 USAGE
    herr_t H5S_none_iter_next(iter, nelem)
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
H5S_none_iter_next(H5S_sel_iter_t H5_ATTR_UNUSED *iter, hsize_t H5_ATTR_UNUSED nelem)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(iter);
    HDassert(nelem>0);

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_none_iter_next() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_iter_next_block
 PURPOSE
    Increment selection iterator to next block
 USAGE
    herr_t H5S_none_iter_next(iter)
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
H5S_none_iter_next_block(H5S_sel_iter_t H5_ATTR_UNUSED *iter)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(iter);

    FUNC_LEAVE_NOAPI(FAIL)
}   /* H5S_none_iter_next_block() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_iter_release
 PURPOSE
    Release "none" selection iterator information for a dataspace
 USAGE
    herr_t H5S_none_iter_release(iter)
        H5S_sel_iter_t *iter;       IN: Pointer to selection iterator
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Releases all information for a dataspace "none" selection iterator
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_none_iter_release(H5S_sel_iter_t H5_ATTR_UNUSED * iter)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(iter);

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_none_iter_release() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_release
 PURPOSE
    Release none selection information for a dataspace
 USAGE
    herr_t H5S_none_release(space)
        H5S_t *space;       IN: Pointer to dataspace
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Releases "none" selection information for a dataspace
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_none_release(H5S_t H5_ATTR_UNUSED * space)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_none_release() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_copy
 PURPOSE
    Copy a selection from one dataspace to another
 USAGE
    herr_t H5S_none_copy(dst, src)
        H5S_t *dst;  OUT: Pointer to the destination dataspace
        H5S_t *src;  IN: Pointer to the source dataspace
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Copies the 'none' selection information from the source
    dataspace to the destination dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5S_none_copy(H5S_t *dst, const H5S_t H5_ATTR_UNUSED *src, hbool_t H5_ATTR_UNUSED share_selection)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(src);
    HDassert(dst);

    /* Set number of elements in selection */
    dst->select.num_elem = 0;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5S_none_copy() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_is_valid
 PURPOSE
    Check whether the selection fits within the extent, with the current
    offset defined.
 USAGE
    htri_t H5S_none_is_valid(space);
        H5S_t *space;             IN: Dataspace pointer to query
 RETURNS
    TRUE if the selection fits within the extent, FALSE if it does not and
        Negative on an error.
 DESCRIPTION
    Determines if the current selection at the current offset fits within the
    extent for the dataspace.  Offset is irrelevant for this type of selection.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static htri_t
H5S_none_is_valid(const H5S_t H5_ATTR_UNUSED *space)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);

    FUNC_LEAVE_NOAPI(TRUE)
} /* end H5S_none_is_valid() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_serial_size
 PURPOSE
    Determine the number of bytes needed to store the serialized "none"
        selection information.
 USAGE
    hssize_t H5S_none_serial_size(space, f)
        H5S_t *space;             IN: Dataspace pointer to query
        H5F_t *f;                 IN: File pointer
 RETURNS
    The number of bytes required on success, negative on an error.
 DESCRIPTION
    Determines the number of bytes required to serialize an "none"
    selection for storage on disk.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static hssize_t
H5S_none_serial_size(const H5S_t H5_ATTR_UNUSED *space, H5F_t H5_ATTR_UNUSED *f)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);

    /* Basic number of bytes required to serialize point selection:
     *  <type (4 bytes)> + <version (4 bytes)> + <padding (4 bytes)> +
     *      <length (4 bytes)> = 16 bytes
     */
    FUNC_LEAVE_NOAPI(16)
} /* end H5S_none_serial_size() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_serialize
 PURPOSE
    Serialize the current selection into a user-provided buffer.
 USAGE
    herr_t H5S_none_serialize(space, p, f)
        const H5S_t *space;     IN: Dataspace with selection to serialize
        uint8_t **p;            OUT: Pointer to buffer to put serialized
                                selection.  Will be advanced to end of
                                serialized selection.
         H5F_t *f;              IN: File pointer
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
H5S_none_serialize(const H5S_t *space, uint8_t **p, H5F_t H5_ATTR_UNUSED *f)
{
    uint8_t *pp = (*p);         /* Local pointer for decoding */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);
    HDassert(p);
    HDassert(pp);

    /* Store the preamble information */
    UINT32ENCODE(pp, (uint32_t)H5S_GET_SELECT_TYPE(space)); /* Store the type of selection */
    UINT32ENCODE(pp, (uint32_t)H5S_NONE_VERSION_1);         /* Store the version number */
    UINT32ENCODE(pp, (uint32_t)0);  /* Store the un-used padding */
    UINT32ENCODE(pp, (uint32_t)0);  /* Store the additional information length */

    /* Update encoding pointer */
    *p = pp;

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5S_none_serialize() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_deserialize
 PURPOSE
    Deserialize the current selection from a user-provided buffer.
 USAGE
    herr_t H5S_none_deserialize(space, version, flags, p)
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
H5S_none_deserialize(H5S_t *space, uint32_t H5_ATTR_UNUSED version, uint8_t H5_ATTR_UNUSED flags,
    const uint8_t H5_ATTR_UNUSED **p)
{
    herr_t ret_value = SUCCEED;  /* return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(space);
    HDassert(p);
    HDassert(*p);

    /* Change to "none" selection */
    if(H5S_select_none(space) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't change selection")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_none_deserialize() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_bounds
 PURPOSE
    Gets the bounding box containing the selection.
 USAGE
    herr_t H5S_none_bounds(space, start, end)
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
H5S_none_bounds(const H5S_t H5_ATTR_UNUSED *space, hsize_t H5_ATTR_UNUSED *start, hsize_t H5_ATTR_UNUSED *end)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);
    HDassert(start);
    HDassert(end);

    FUNC_LEAVE_NOAPI(FAIL)
}   /* H5Sget_none_bounds() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_offset
 PURPOSE
    Gets the linear offset of the first element for the selection.
 USAGE
    herr_t H5S_none_offset(space, offset)
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
H5S_none_offset(const H5S_t H5_ATTR_UNUSED *space, hsize_t H5_ATTR_UNUSED *offset)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);
    HDassert(offset);

    FUNC_LEAVE_NOAPI(FAIL)
}   /* H5S_none_offset() */


/*--------------------------------------------------------------------------
 NAME
    H5S__none_unlim_dim
 PURPOSE
    Return unlimited dimension of selection, or -1 if none
 USAGE
    int H5S__none_unlim_dim(space)
        H5S_t *space;           IN: Dataspace pointer to check
 RETURNS
    Unlimited dimension of selection, or -1 if none (never fails).
 DESCRIPTION
    Returns the index of the unlimited dimension in this selection, or -1
    if the selection has no unlimited dimension.  "None" selections cannot
    have an unlimited dimension, so this function always returns -1.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static int
H5S__none_unlim_dim(const H5S_t H5_ATTR_UNUSED *space)
{
    FUNC_ENTER_STATIC_NOERR

    FUNC_LEAVE_NOAPI(-1)
} /* end H5S__none_unlim_dim() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_is_contiguous
 PURPOSE
    Check if a "none" selection is contiguous within the dataspace extent.
 USAGE
    htri_t H5S_none_is_contiguous(space)
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
H5S_none_is_contiguous(const H5S_t H5_ATTR_UNUSED *space)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);

    FUNC_LEAVE_NOAPI(FALSE)
}   /* H5S_none_is_contiguous() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_is_single
 PURPOSE
    Check if a "none" selection is a single block within the dataspace extent.
 USAGE
    htri_t H5S_none_is_single(space)
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
H5S_none_is_single(const H5S_t H5_ATTR_UNUSED *space)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(space);

    FUNC_LEAVE_NOAPI(FALSE)
}   /* H5S_none_is_single() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_is_regular
 PURPOSE
    Check if a "none" selection is "regular"
 USAGE
    htri_t H5S_none_is_regular(space)
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
H5S_none_is_regular(const H5S_t H5_ATTR_UNUSED *space)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);

    FUNC_LEAVE_NOAPI(TRUE)
}   /* H5S_none_is_regular() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_adjust_u
 PURPOSE
    Adjust an "none" selection by subtracting an offset
 USAGE
    void H5S_none_adjust_u(space, offset)
        H5S_t *space;           IN/OUT: Pointer to dataspace to adjust
        const hsize_t *offset; IN: Offset to subtract
 RETURNS
    None
 DESCRIPTION
    Moves selection by subtracting an offset from it.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static void
H5S_none_adjust_u(H5S_t H5_ATTR_UNUSED *space, const hsize_t H5_ATTR_UNUSED *offset)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);
    HDassert(offset);

    FUNC_LEAVE_NOAPI_VOID
}   /* H5S_none_adjust_u() */


/*-------------------------------------------------------------------------
 * Function:	H5S_none_project_scalar
 *
 * Purpose:	Projects a 'none' selection into a scalar dataspace
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Programmer:	Quincey Koziol
 *              Sunday, July 18, 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5S_none_project_scalar(const H5S_t H5_ATTR_UNUSED *space, hsize_t H5_ATTR_UNUSED *offset)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space && H5S_SEL_NONE == H5S_GET_SELECT_TYPE(space));
    HDassert(offset);

    FUNC_LEAVE_NOAPI(FAIL)
}   /* H5S_none_project_scalar() */


/*-------------------------------------------------------------------------
 * Function:	H5S_none_project_simple
 *
 * Purpose:	Projects an 'none' selection onto/into a simple dataspace
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
H5S_none_project_simple(const H5S_t *base_space, H5S_t *new_space, hsize_t *offset)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check args */
    HDassert(base_space && H5S_SEL_NONE == H5S_GET_SELECT_TYPE(base_space));
    HDassert(new_space);
    HDassert(offset);

    /* Select the entire new space */
    if(H5S_select_none(new_space) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSET, FAIL, "unable to set none selection")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_none_project_simple() */


/*--------------------------------------------------------------------------
 NAME
    H5S_select_none
 PURPOSE
    Specify that nothing is selected in the extent
 USAGE
    herr_t H5S_select_none(dsid)
        hid_t dsid;             IN: Dataspace ID of selection to modify
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    This function de-selects the entire extent for a dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5S_select_none(H5S_t *space)
{
    herr_t ret_value = SUCCEED;  /* return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(space);

    /* Remove current selection first */
    if(H5S_SELECT_RELEASE(space) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't release hyperslab")

    /* Set number of elements in selection */
    space->select.num_elem = 0;

    /* Set selection type */
    space->select.type = H5S_sel_none;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5S_select_none() */


/*--------------------------------------------------------------------------
 NAME
    H5Sselect_none
 PURPOSE
    Specify that nothing is selected in the extent
 USAGE
    herr_t H5Sselect_none(dsid)
        hid_t dsid;             IN: Dataspace ID of selection to modify
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    This function de-selects the entire extent for a dataspace.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Sselect_none(hid_t spaceid)
{
    H5S_t *space;                       /* Dataspace to modify selection of */
    herr_t ret_value = SUCCEED;         /* return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", spaceid);

    /* Check args */
    if(NULL == (space = (H5S_t *)H5I_object_verify(spaceid, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace")

    /* Change to "none" selection */
    if(H5S_select_none(space) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTDELETE, FAIL, "can't change selection")

done:
    FUNC_LEAVE_API(ret_value)
}   /* H5Sselect_none() */


/*--------------------------------------------------------------------------
 NAME
    H5S_none_get_seq_list
 PURPOSE
    Create a list of offsets & lengths for a selection
 USAGE
    herr_t H5S_none_get_seq_list(space,flags,iter,maxseq,maxelem,nseq,nelem,off,len)
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
H5S_none_get_seq_list(const H5S_t H5_ATTR_UNUSED *space, unsigned H5_ATTR_UNUSED flags, H5S_sel_iter_t H5_ATTR_UNUSED *iter,
    size_t H5_ATTR_UNUSED maxseq, size_t H5_ATTR_UNUSED maxelem, size_t *nseq, size_t *nelem,
    hsize_t H5_ATTR_UNUSED *off, size_t H5_ATTR_UNUSED *len)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(space);
    HDassert(iter);
    HDassert(maxseq > 0);
    HDassert(maxelem > 0);
    HDassert(nseq);
    HDassert(nelem);
    HDassert(off);
    HDassert(len);

    /* "none" selections don't generate sequences of bytes */
    *nseq = 0;

    /* They don't use any elements, either */
    *nelem = 0;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5S_none_get_seq_list() */

