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
 * Created:		H5MM.c
 *			Jul 10 1997
 *			Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:		Memory management functions.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */


#include "H5private.h"
#include "H5Eprivate.h"
#include "H5MMprivate.h"

#ifndef NDEBUG

/*-------------------------------------------------------------------------
 * Function:	H5MM_malloc
 *
 * Purpose:	Just like the POSIX version of malloc(3). This routine
 *		specifically checks for allocations of 0 bytes and fails
 *              in that case.  This routine is not called when NDEBUG is
 *		defined.
 *
 * Return:	Success:	Ptr to new memory
 *
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Nov  8 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void *
H5MM_malloc(size_t size)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(size);

    FUNC_LEAVE_NOAPI(HDmalloc(size));
} /* end H5MM_malloc() */


/*-------------------------------------------------------------------------
 * Function:	H5MM_calloc
 *
 * Purpose:	Similar to the POSIX version of calloc(3), except this routine
 *              just takes a 'size' parameter. This routine
 *		specifically checks for allocations of 0 bytes and fails
 *              in that case.  This routine is not called when NDEBUG is
 *		defined.
 *
 * Return:	Success:	Ptr to new memory
 *
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Nov  8 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void *
H5MM_calloc(size_t size)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(size);

    FUNC_LEAVE_NOAPI(HDcalloc(1,size));
} /* end H5MM_calloc() */
#endif /* NDEBUG */


/*-------------------------------------------------------------------------
 * Function:	H5MM_realloc
 *
 * Purpose:	Just like the POSIX version of realloc(3). Specifically, the
 *		following calls are equivalent
 *
 *		H5MM_realloc (NULL, size) <==> H5MM_malloc (size)
 *		H5MM_realloc (ptr, 0)	  <==> H5MM_xfree (ptr)
 *		H5MM_realloc (NULL, 0)	  <==> NULL
 *
 * Return:	Success:	Ptr to new memory or NULL if the memory
 *				was freed or HDrealloc couldn't allocate
 *				memory.
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jul 10 1997
 *
 *-------------------------------------------------------------------------
 */
void *
H5MM_realloc(void *mem, size_t size)
{
    void *ret_value;

    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(NULL == mem) {
	if(0 == size)
            ret_value = NULL;
        else
            ret_value = H5MM_malloc(size);
    } /* end if */
    else if(0 == size)
	ret_value = H5MM_xfree(mem);
    else
	ret_value = HDrealloc(mem, size);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MM_realloc() */


/*-------------------------------------------------------------------------
 * Function:    H5MM_xstrdup
 *
 * Purpose:     Duplicates a string, including memory allocation.
 *              NULL is an acceptable value for the input string.
 *
 * Return:      Success:    Pointer to a new string (NULL if s is NULL).
 *
 *              Failure:    abort()
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 10 1997
 *-------------------------------------------------------------------------
 */
char *
H5MM_xstrdup(const char *s)
{
    char	*ret_value = NULL;

    FUNC_ENTER_NOAPI(NULL)

    if(s) {
        if(NULL == (ret_value = (char *)H5MM_malloc(HDstrlen(s) + 1)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
        HDstrcpy(ret_value, s);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MM_xstrdup() */


/*-------------------------------------------------------------------------
 * Function:    H5MM_strdup
 *
 * Purpose:     Duplicates a string, including memory allocation.
 *              NULL is NOT an acceptable value for the input string.
 *
 *              If the string to be duplicated is the NULL pointer, then
 *              an error will be raised.
 *
 * Return:      Success:    Pointer to a new string
 *
 *              Failure:    abort()
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 10 1997
 *-------------------------------------------------------------------------
 */
char *
H5MM_strdup(const char *s)
{
    char	*ret_value;

    FUNC_ENTER_NOAPI(NULL)

    if(!s)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "null string")
    if(NULL == (ret_value = (char *)H5MM_malloc(HDstrlen(s) + 1)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
    HDstrcpy(ret_value, s);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MM_strdup() */


/*-------------------------------------------------------------------------
 * Function:	H5MM_xfree
 *
 * Purpose:	Just like free(3) except null pointers are allowed as
 *		arguments, and the return value (always NULL) can be
 *		assigned to the pointer whose memory was just freed:
 *
 *			thing = H5MM_xfree (thing);
 *
 * Return:	Success:	NULL
 *
 *		Failure:	never fails
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jul 10 1997
 *
 *-------------------------------------------------------------------------
 */
void *
H5MM_xfree(void *mem)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(mem)
        HDfree(mem);

    FUNC_LEAVE_NOAPI(NULL);
} /* end H5MM_xfree() */
