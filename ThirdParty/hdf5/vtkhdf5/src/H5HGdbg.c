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

/* Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              Wednesday, July 9, 2003
 *
 * Purpose:	Global Heap object debugging functions.
 */
#define H5HG_PACKAGE		/*suppress error about including H5HGpkg	  */


#include "H5private.h"		/* Generic Functions			*/
#include "H5ACprivate.h"	/* Metadata cache			*/
#include "H5Eprivate.h"		/* Error handling		        */
#include "H5HGpkg.h"		/* Global heaps				*/
#include "H5Iprivate.h"		/* ID Functions		                */


/*-------------------------------------------------------------------------
 * Function:	H5HG_debug
 *
 * Purpose:	Prints debugging information about a global heap collection.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Mar 27, 1998
 *
 * Modifications:
 *		Robb Matzke, 1999-07-28
 *		The ADDR argument is passed by value.
 *
 *              Robb Matzke, LLNL, 2003-06-05
 *              The size does not include the object header, just the data.
 *
 *              John Mainzer, 6/17/05
 *              Modified the function to use the new dirtied parameter of
 *              of H5AC_unprotect() instead of modifying the is_dirty
 *              field of the cache info.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HG_debug(H5F_t *f, hid_t dxpl_id, haddr_t addr, FILE *stream, int indent,
	  int fwidth)
{
    unsigned		u, nused, maxobj;
    unsigned		j, k;
    H5HG_heap_t		*h = NULL;
    char		buf[64];
    uint8_t		*p = NULL;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5HG_debug, FAIL);

    /* check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined (addr));
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);

    if(NULL == (h = H5HG_protect(f, dxpl_id, addr, H5AC_READ)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTPROTECT, FAIL, "unable to protect global heap collection");

    fprintf(stream, "%*sGlobal Heap Collection...\n", indent, "");
    fprintf(stream, "%*s%-*s %d\n", indent, "", fwidth,
	    "Dirty:",
	    (int)(h->cache_info.is_dirty));
    fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
	    "Total collection size in file:",
	    (unsigned long)(h->size));

    for (u=1, nused=0, maxobj=0; u<h->nused; u++) {
	if (h->obj[u].begin) {
	    nused++;
	    if (u>maxobj) maxobj = u;
	}
    }
    fprintf (stream, "%*s%-*s %u/%lu/", indent, "", fwidth,
	     "Objects defined/allocated/max:",
	     nused, (unsigned long)h->nalloc);
    if(nused)
        fprintf(stream, "%u\n", maxobj);
    else
        fprintf(stream, "NA\n");

    fprintf (stream, "%*s%-*s %lu\n", indent, "", fwidth,
	     "Free space:",
	     (unsigned long)(h->obj[0].size));

    for (u=1; u<h->nused; u++) {
	if (h->obj[u].begin) {
	    sprintf (buf, "Object %u", u);
	    fprintf (stream, "%*s%s\n", indent, "", buf);
	    fprintf (stream, "%*s%-*s %d\n", indent+3, "", MIN(fwidth-3, 0),
		     "Reference count:",
		     h->obj[u].nrefs);
	    fprintf (stream, "%*s%-*s %lu/%lu\n", indent+3, "",
		     MIN(fwidth-3, 0),
		     "Size of object body:",
		     (unsigned long)(h->obj[u].size),
		     (unsigned long)H5HG_ALIGN(h->obj[u].size));
	    p = h->obj[u].begin + H5HG_SIZEOF_OBJHDR (f);
	    for (j=0; j<h->obj[u].size; j+=16) {
		fprintf (stream, "%*s%04u: ", indent+6, "", j);
		for (k=0; k<16; k++) {
		    if (8==k) fprintf (stream, " ");
		    if (j+k<h->obj[u].size) {
			fprintf (stream, "%02x ", p[j+k]);
		    } else {
			HDfputs("   ", stream);
		    }
		}
		for (k=0; k<16 && j+k<h->obj[u].size; k++) {
		    if (8==k) fprintf (stream, " ");
		    HDfputc(p[j+k]>' ' && p[j+k]<='~' ? p[j+k] : '.', stream);
		}
		fprintf (stream, "\n");
	    }
	}
    }

done:
    if (h && H5AC_unprotect(f, dxpl_id, H5AC_GHEAP, addr, h, H5AC__NO_FLAGS_SET) < 0)
        HDONE_ERROR(H5E_HEAP, H5E_PROTECT, FAIL, "unable to release object header");

    FUNC_LEAVE_NOAPI(ret_value);
}
