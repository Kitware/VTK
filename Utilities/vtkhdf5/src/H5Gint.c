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
 * Created:	H5Gint.c
 *		April 5 2007
 *		Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:	General use, "internal" routines for groups.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5G_PACKAGE		/*suppress error about including H5Gpkg   */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5G_init_int_interface


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Gpkg.h"		/* Groups		  		*/
#include "H5MMprivate.h"	/* Memory management			*/


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



/*--------------------------------------------------------------------------
NAME
   H5G_init_int_interface -- Initialize interface-specific information
USAGE
    herr_t H5G_init_int_interface()
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5G_init() currently).

--------------------------------------------------------------------------*/
static herr_t
H5G_init_int_interface(void)
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5G_init_int_interface, FAIL)

    /* Funnel all work to H5G_init() */
    if(H5G_init() < 0)
        HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, FAIL, "interface initialization failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5G_init_int_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5G_component
 *
 * Purpose:	Returns the pointer to the first component of the
 *		specified name by skipping leading slashes.  Returns
 *		the size in characters of the component through SIZE_P not
 *		counting leading slashes or the null terminator.
 *
 * Return:	Success:	Ptr into NAME.
 *
 *		Failure:	Ptr to the null terminator of NAME.
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug 11 1997
 *
 *-------------------------------------------------------------------------
 */
const char *
H5G_component(const char *name, size_t *size_p)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5G_component)

    assert(name);

    while ('/' == *name)
        name++;
    if (size_p)
        *size_p = HDstrcspn(name, "/");

    FUNC_LEAVE_NOAPI(name)
} /* end H5G_component() */


/*-------------------------------------------------------------------------
 * Function:	H5G_normalize
 *
 * Purpose:	Returns a pointer to a new string which has duplicate and
 *              trailing slashes removed from it.
 *
 * Return:	Success:	Ptr to normalized name.
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Saturday, August 16, 2003
 *
 *-------------------------------------------------------------------------
 */
char *
H5G_normalize(const char *name)
{
    char *norm;         /* Pointer to the normalized string */
    size_t	s,d;    /* Positions within the strings */
    unsigned    last_slash;     /* Flag to indicate last character was a slash */
    char *ret_value;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5G_normalize)

    /* Sanity check */
    HDassert(name);

    /* Duplicate the name, to return */
    if(NULL == (norm = H5MM_strdup(name)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for normalized string")

    /* Walk through the characters, omitting duplicated '/'s */
    s = d = 0;
    last_slash = 0;
    while(name[s] != '\0') {
        if(name[s] == '/')
            if(last_slash)
                ;
            else {
                norm[d++] = name[s];
                last_slash = 1;
            } /* end else */
        else {
            norm[d++] = name[s];
            last_slash = 0;
        } /* end else */
        s++;
    } /* end while */

    /* Terminate normalized string */
    norm[d] = '\0';

    /* Check for final '/' on normalized name & eliminate it */
    if(d > 1 && last_slash)
        norm[d - 1] = '\0';

    /* Set return value */
    ret_value = norm;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_normalize() */

