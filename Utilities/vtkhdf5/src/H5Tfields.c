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
 * Module Info: This module contains commond functionality for fields in
 *      enumerated & compound datatypes in the H5T interface.
 */

#define H5T_PACKAGE		/*suppress error about including H5Tpkg	  */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5T_init_fields_interface


#include "H5private.h"		/*generic functions			  */
#include "H5Eprivate.h"		/*error handling			  */
#include "H5Iprivate.h"		/*ID functions		   		  */
#include "H5MMprivate.h"	/*memory management			  */
#include "H5Tpkg.h"		/*data-type functions			  */


/*--------------------------------------------------------------------------
NAME
   H5T_init_fields_interface -- Initialize interface-specific information
USAGE
    herr_t H5T_init_fields_interface()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5T_init_iterface currently).

--------------------------------------------------------------------------*/
static herr_t
H5T_init_fields_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_init_fields_interface)

    FUNC_LEAVE_NOAPI(H5T_init())
} /* H5T_init_fields_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5Tget_nmembers
 *
 * Purpose:	Determines how many members TYPE_ID has.  The type must be
 *		either a compound datatype or an enumeration datatype.
 *
 * Return:	Success:	Number of members defined in the datatype.
 *
 *		Failure:	Negative
 *
 * Errors:
 *
 * Programmer:	Robb Matzke
 *		Monday, December  8, 1997
 *
 * Modifications:
 *	Robb Matzke, 22 Dec 1998
 *	Also works with enumeration datatypes.
 *-------------------------------------------------------------------------
 */
int
H5Tget_nmembers(hid_t type_id)
{
    H5T_t *dt;          /* Datatype to query */
    int	ret_value;      /* Return value */

    FUNC_ENTER_API(H5Tget_nmembers, FAIL)
    H5TRACE1("Is", "i", type_id);

    /* Check args */
    if(NULL == (dt = H5I_object_verify(type_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")

    if((ret_value = H5T_get_nmembers(dt)) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "cannot return member number")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tget_nmembers() */


/*-------------------------------------------------------------------------
 * Function:	H5T_get_nmembers
 *
 * Purpose:	Private function for H5Tget_nmembers.  Determines how many
 *              members DTYPE has.  The type must be either a compound data
 *              type or an enumeration datatype.
 *
 * Return:	Success:	Number of members defined in the datatype.
 *
 *		Failure:	Negative
 *
 * Errors:
 *
 * Programmer:  Raymond Lu
 *	        October 8, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5T_get_nmembers(const H5T_t *dt)
{
    int	ret_value;

    FUNC_ENTER_NOAPI(H5T_get_nmembers, FAIL)

    HDassert(dt);

    if(H5T_COMPOUND == dt->shared->type)
	ret_value = (int)dt->shared->u.compnd.nmembs;
    else if(H5T_ENUM == dt->shared->type)
	ret_value = (int)dt->shared->u.enumer.nmembs;
    else
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "operation not supported for type class")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_get_nmembers() */


/*-------------------------------------------------------------------------
 * Function:	H5Tget_member_name
 *
 * Purpose:	Returns the name of a member of a compound or enumeration
 *		datatype. Members are stored in no particular order with
 *		numbers 0 through N-1 where N is the value returned by
 *		H5Tget_nmembers().
 *
 * Return:	Success:	Ptr to a string allocated with malloc().  The
 *				caller is responsible for freeing the string.
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *		Wednesday, January  7, 1998
 *
 * Modifications:
 *	Robb Matzke, 22 Dec 1998
 *	Also works with enumeration datatypes.
 *-------------------------------------------------------------------------
 */
char *
H5Tget_member_name(hid_t type_id, unsigned membno)
{
    H5T_t	*dt = NULL;
    char	*ret_value;

    FUNC_ENTER_API(H5Tget_member_name, NULL)

    /* Check args */
    if (NULL == (dt = H5I_object_verify(type_id,H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a datatype")

    if((ret_value = H5T_get_member_name(dt, membno))==NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "unable to get member name")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5T_get_member_name
 *
 * Purpose:	Private function for H5Tget_member_name.  Returns the name
 *              of a member of a compound or enumeration datatype. Members
 *              are stored in no particular order with numbers 0 through
 *              N-1 where N is the value returned by H5Tget_nmembers().
 *
 * Return:	Success:	Ptr to a string allocated with malloc().  The
 *				caller is responsible for freeing the string.
 *
 *		Failure:	NULL
 *
 * Programmer:	Raymond Lu
 *              October 9, 2002
 *
 * Modifications:
 *-------------------------------------------------------------------------
 */
char *
H5T_get_member_name(H5T_t const *dt, unsigned membno)
{
    char	*ret_value;

    FUNC_ENTER_NOAPI(H5T_get_member_name, NULL)

    assert(dt);

    switch (dt->shared->type) {
        case H5T_COMPOUND:
            if (membno>=dt->shared->u.compnd.nmembs)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid member number")
            ret_value = H5MM_xstrdup(dt->shared->u.compnd.memb[membno].name);
            break;

        case H5T_ENUM:
            if (membno>=dt->shared->u.enumer.nmembs)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid member number")
            ret_value = H5MM_xstrdup(dt->shared->u.enumer.name[membno]);
            break;

        default:
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "operation not supported for type class")
    } /*lint !e788 All appropriate cases are covered */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_member_index
 *
 * Purpose:     Returns the index of a member in a compound or enumeration
 *              datatype by given name.Members are stored in no particular
 *              order with numbers 0 through N-1 where N is the value
 *              returned by H5Tget_nmembers().
 *
 * Return:      Success:        index of the member if exists.
 *              Failure:        -1.
 *
 * Programmer:  Raymond Lu
 *              Thursday, April 4, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5Tget_member_index(hid_t type_id, const char *name)
{
    H5T_t       *dt = NULL;
    int         ret_value=FAIL;
    unsigned    i;

    FUNC_ENTER_API(H5Tget_member_index, FAIL)
    H5TRACE2("Is", "i*s", type_id, name);

    /* Check arguments */
    assert(name);
    if(NULL==(dt=H5I_object_verify(type_id,H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")

    /* Locate member by name */
    switch (dt->shared->type) {
        case H5T_COMPOUND:
            for(i=0; i< dt->shared->u.compnd.nmembs; i++) {
                if(!HDstrcmp(dt->shared->u.compnd.memb[i].name, name))
                    HGOTO_DONE((int)i)
            }
            break;
        case H5T_ENUM:
            for(i=0; i< dt->shared->u.enumer.nmembs; i++) {
                if(!HDstrcmp(dt->shared->u.enumer.name[i], name))
                    HGOTO_DONE((int)i)
            }
            break;
        default:
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "operation not supported for this type")
    } /*lint !e788 All appropriate cases are covered */

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5T_sort_value
 *
 * Purpose:	Sorts the members of a compound datatype by their offsets;
 *		sorts the members of an enum type by their values. This even
 *		works for locked datatypes since it doesn't change the value
 *		of the type.  MAP is an optional parallel integer array which
 *		is also swapped along with members of DT.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Wednesday, January  7, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_sort_value(const H5T_t *dt, int *map)
{
    unsigned	nmembs;                 /* Number of members for datatype */
    size_t	size;
    hbool_t	swapped;                /* Whether we've swapped fields */
    uint8_t	tbuf[32];
    unsigned	i, j;                   /* Local index variables */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5T_sort_value, FAIL)

    /* Check args */
    HDassert(dt);
    HDassert(H5T_COMPOUND == dt->shared->type || H5T_ENUM == dt->shared->type);

    /* Use a bubble sort because we can short circuit */
    if(H5T_COMPOUND == dt->shared->type) {
	if(H5T_SORT_VALUE != dt->shared->u.compnd.sorted) {
	    dt->shared->u.compnd.sorted = H5T_SORT_VALUE;
	    nmembs = dt->shared->u.compnd.nmembs;
	    for(i = nmembs - 1, swapped = TRUE; i > 0 && swapped; --i) {
		for(j = 0, swapped = FALSE; j < i; j++) {
		    if(dt->shared->u.compnd.memb[j].offset > dt->shared->u.compnd.memb[j + 1].offset) {
                        H5T_cmemb_t tmp = dt->shared->u.compnd.memb[j];
			dt->shared->u.compnd.memb[j] = dt->shared->u.compnd.memb[j + 1];
			dt->shared->u.compnd.memb[j + 1] = tmp;
			if(map) {
			    int x = map[j];

			    map[j] = map[j + 1];
			    map[j + 1] = x;
			} /* end if */
			swapped = TRUE;
		    } /* end if */
		} /* end for */
	    } /* end for */
#ifndef NDEBUG
	    /* I never trust a sort :-) -RPM */
	    for(i = 0; i < (nmembs - 1); i++)
		HDassert(dt->shared->u.compnd.memb[i].offset < dt->shared->u.compnd.memb[i + 1].offset);
#endif
	} /* end if */
    } else if(H5T_ENUM == dt->shared->type) {
	if(H5T_SORT_VALUE != dt->shared->u.enumer.sorted) {
	    dt->shared->u.enumer.sorted = H5T_SORT_VALUE;
	    nmembs = dt->shared->u.enumer.nmembs;
	    size = dt->shared->size;
	    HDassert(size <= sizeof(tbuf));
	    for(i = (nmembs - 1), swapped = TRUE; i > 0 && swapped; --i) {
		for(j = 0, swapped = FALSE; j < i; j++) {
		    if(HDmemcmp(dt->shared->u.enumer.value + (j * size), dt->shared->u.enumer.value + ((j + 1) * size), size) > 0) {
			/* Swap names */
			char *tmp = dt->shared->u.enumer.name[j];
			dt->shared->u.enumer.name[j] = dt->shared->u.enumer.name[j + 1];
			dt->shared->u.enumer.name[j + 1] = tmp;

			/* Swap values */
			HDmemcpy(tbuf, dt->shared->u.enumer.value + (j * size), size);
			HDmemcpy(dt->shared->u.enumer.value + (j * size),
				 dt->shared->u.enumer.value + ((j + 1) * size), size);
			HDmemcpy(dt->shared->u.enumer.value + ((j + 1) * size), tbuf, size);

			/* Swap map */
			if(map) {
			    int x = map[j];

			    map[j] = map[j + 1];
			    map[j + 1] = x;
			} /* end if */

			swapped = TRUE;
		    } /* end if */
		} /* end for */
	    } /* end for */
#ifndef NDEBUG
	    /* I never trust a sort :-) -RPM */
	    for(i = 0; i < (nmembs - 1); i++)
		HDassert(HDmemcmp(dt->shared->u.enumer.value + (i * size), dt->shared->u.enumer.value + ((i + 1) * size), size) < 0);
#endif
	} /* end if */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_sort_value() */


/*-------------------------------------------------------------------------
 * Function:	H5T_sort_name
 *
 * Purpose:	Sorts members of a compound or enumeration datatype by their
 *		names. This even works for locked datatypes since it doesn't
 *		change the value of the types.
 *
 * Return:	Success:	Non-negative
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Monday, January  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_sort_name(const H5T_t *dt, int *map)
{
    unsigned	i, j, nmembs;
    size_t	size;
    hbool_t	swapped;
    uint8_t	tbuf[32];
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5T_sort_name, FAIL)

    /* Check args */
    assert(dt);
    assert(H5T_COMPOUND==dt->shared->type || H5T_ENUM==dt->shared->type);

    /* Use a bubble sort because we can short circuit */
    if (H5T_COMPOUND==dt->shared->type) {
	if (H5T_SORT_NAME!=dt->shared->u.compnd.sorted) {
	    dt->shared->u.compnd.sorted = H5T_SORT_NAME;
	    nmembs = dt->shared->u.compnd.nmembs;
	    for (i=nmembs-1, swapped=TRUE; i>0 && swapped; --i) {
		for (j=0, swapped=FALSE; j<i; j++) {
		    if (HDstrcmp(dt->shared->u.compnd.memb[j].name,
				 dt->shared->u.compnd.memb[j+1].name)>0) {
			H5T_cmemb_t tmp = dt->shared->u.compnd.memb[j];
			dt->shared->u.compnd.memb[j] = dt->shared->u.compnd.memb[j+1];
			dt->shared->u.compnd.memb[j+1] = tmp;
			swapped = TRUE;
			if (map) {
			    int x = map[j];
			    map[j] = map[j+1];
			    map[j+1] = x;
			}
		    }
		}
	    }
#ifndef NDEBUG
	    /* I never trust a sort :-) -RPM */
	    for (i=0; i<nmembs-1; i++) {
		assert(HDstrcmp(dt->shared->u.compnd.memb[i].name,
				dt->shared->u.compnd.memb[i+1].name)<0);
	    }
#endif
	}
    } else if (H5T_ENUM==dt->shared->type) {
	if (H5T_SORT_NAME!=dt->shared->u.enumer.sorted) {
	    dt->shared->u.enumer.sorted = H5T_SORT_NAME;
	    nmembs = dt->shared->u.enumer.nmembs;
	    size = dt->shared->size;
	    assert(size<=sizeof(tbuf));
	    for (i=nmembs-1, swapped=TRUE; i>0 && swapped; --i) {
		for (j=0, swapped=FALSE; j<i; j++) {
		    if (HDstrcmp(dt->shared->u.enumer.name[j],
				 dt->shared->u.enumer.name[j+1])>0) {
			/* Swap names */
			char *tmp = dt->shared->u.enumer.name[j];
			dt->shared->u.enumer.name[j] = dt->shared->u.enumer.name[j+1];
			dt->shared->u.enumer.name[j+1] = tmp;

			/* Swap values */
			HDmemcpy(tbuf, dt->shared->u.enumer.value+j*size, size);
			HDmemcpy(dt->shared->u.enumer.value+j*size,
				 dt->shared->u.enumer.value+(j+1)*size, size);
			HDmemcpy(dt->shared->u.enumer.value+(j+1)*size, tbuf, size);

			/* Swap map */
			if (map) {
			    int x = map[j];
			    map[j] = map[j+1];
			    map[j+1] = x;
			}

			swapped = TRUE;
		    }
		}
	    }
#ifndef NDEBUG
	    /* I never trust a sort :-) -RPM */
	    for (i=0; i<nmembs-1; i++)
		assert(HDstrcmp(dt->shared->u.enumer.name[i], dt->shared->u.enumer.name[i+1])<0);
#endif
	}
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

