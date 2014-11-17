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

/*-------------------------------------------------------------------------
 *
 * Created:		H5dbg.c
 *			Mar  4 2006
 *			Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:		Generic debugging routines
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/

/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


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
 * Function:	H5_buffer_dump
 *
 * Purpose:	Dumps a buffer of memory in an octal dump form
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Mar  4 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5_buffer_dump(FILE *stream, int indent, const uint8_t *buf,
    const uint8_t *marker, size_t buf_offset, size_t buf_size)
{
    size_t	u, v;                   /* Local index variable */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /*
     * Check arguments.
     */
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(buf);
    HDassert(marker);
    HDassert(buf_size > 0);

    /*
     * Print the buffer in a VMS-style octal dump.
     */
    HDfprintf(stream, "%*sData follows (`__' indicates free region)...\n",
	    indent, "");
    for(u = 0; u < buf_size; u += 16) {
        uint8_t		c;

	HDfprintf(stream, "%*s %8d: ", indent, "", u + buf_offset);

        /* Print the hex values */
	for(v = 0; v < 16; v++) {
	    if(u + v < buf_size) {
		if(marker[u + v])
		    HDfprintf(stream, "__ ");
		else {
		    c = buf[buf_offset + u + v];
		    HDfprintf(stream, "%02x ", c);
		} /* end else */
	    } /* end if */
            else
		fprintf(stream, "   ");
	    if(7 == v)
		HDfputc(' ', stream);
	} /* end for */
        HDfputc(' ', stream);

        /* Print the character values */
	for(v = 0; v < 16; v++) {
	    if(u + v < buf_size) {
		if(marker[u + v])
		    HDfputc(' ', stream);
		else {
		    c = buf[buf_offset + u + v];
		    if(HDisprint(c))
			HDfputc(c, stream);
		    else
			HDfputc('.', stream);
		} /* end else */
	    } /* end if */
	    if(7 == v)
		HDfputc(' ', stream);
	} /* end for */

	HDfputc('\n', stream);
    } /* end for */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5_buffer_dump() */

