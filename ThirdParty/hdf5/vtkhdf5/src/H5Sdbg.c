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

/*-------------------------------------------------------------------------
 *
 * Created:        H5Sdbg.c
 *            Jul 24 2007
 *            Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:        Dump debugging information about a dataspace
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#include "H5Smodule.h"          /* This source code file is part of the H5S module */


/***********/
/* Headers */
/***********/
#include "H5private.h"        /* Generic Functions            */
#include "H5Eprivate.h"        /* Error handling              */
#include "H5Spkg.h"        /* Dataspaces                     */


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
 * Function:    H5S_debug
 *
 * Purpose:    Prints debugging information about a dataspace.
 *
 * Return:    Non-negative on success/Negative on failure
 *
 * Programmer:    Robb Matzke
 *              Tuesday, July 21, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5S_debug(H5F_t *f, const void *_mesg, FILE *stream, int indent, int fwidth)
{
    const H5S_t    *mesg = (const H5S_t*)_mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    switch(H5S_GET_EXTENT_TYPE(mesg)) {
        case H5S_NULL:
            HDfprintf(stream, "%*s%-*s H5S_NULL\n", indent, "", fwidth,
                    "Space class:");
            break;

        case H5S_SCALAR:
            HDfprintf(stream, "%*s%-*s H5S_SCALAR\n", indent, "", fwidth,
                    "Space class:");
            break;

        case H5S_SIMPLE:
            HDfprintf(stream, "%*s%-*s H5S_SIMPLE\n", indent, "", fwidth,
                    "Space class:");
            H5O_debug_id(H5O_SDSPACE_ID, f, &(mesg->extent), stream, indent + 3, MAX(0, fwidth - 3));
            break;

        case H5S_NO_CLASS:
        default:
            HDfprintf(stream, "%*s%-*s **UNKNOWN-%ld**\n", indent, "", fwidth,
                    "Space class:", (long)(H5S_GET_EXTENT_TYPE(mesg)));
            break;
    } /* end switch */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5S_debug() */

