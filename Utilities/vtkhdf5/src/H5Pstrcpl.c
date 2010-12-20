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
 * Created:		H5Pstrcpl.c
 *			October 26 2005
 *			James Laird <jlaird@ncsa.uiuc.edu>
 *
 * Purpose:		String creation property list class routines
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/
#define H5P_PACKAGE		/*suppress error about including H5Ppkg	  */

/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5Ppkg.h"		/* Property lists		  	*/


/****************/
/* Local Macros */
/****************/

/* ========  String creation properties ======== */
/* Definitions for character set encoding property */
#define H5P_STRCRT_CHAR_ENCODING_SIZE  sizeof(H5T_cset_t)
#define H5P_STRCRT_CHAR_ENCODING_DEF   H5F_DEFAULT_CSET


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

/* Property class callbacks */
static herr_t H5P_strcrt_reg_prop(H5P_genclass_t *pclass);


/*********************/
/* Package Variables */
/*********************/

/* String creation property list class library initialization object */
const H5P_libclass_t H5P_CLS_STRCRT[1] = {{
    "string create",		/* Class name for debugging     */
    &H5P_CLS_ROOT_g,		/* Parent class ID              */
    &H5P_CLS_STRING_CREATE_g,	/* Pointer to class ID          */
    NULL,			/* Pointer to default property list ID */
    H5P_strcrt_reg_prop,	/* Default property registration routine */
    NULL,		        /* Class creation callback      */
    NULL,		        /* Class creation callback info */
    NULL,			/* Class copy callback          */
    NULL,		        /* Class copy callback info     */
    NULL,			/* Class close callback         */
    NULL 		        /* Class close callback info    */
}};


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:    H5P_strcrt_reg_prop
 *
 * Purpose:     Register the dataset creation property list class's properties
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              October 31, 2006
 *-------------------------------------------------------------------------
 */
herr_t
H5P_strcrt_reg_prop(H5P_genclass_t *pclass)
{
    H5T_cset_t char_encoding = H5P_STRCRT_CHAR_ENCODING_DEF;  /* Default character set encoding */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5P_strcrt_reg_prop, FAIL)

    /* Register character encoding */
    if(H5P_register_real(pclass, H5P_STRCRT_CHAR_ENCODING_NAME, H5P_STRCRT_CHAR_ENCODING_SIZE, &char_encoding, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P_strcrt_reg_prop() */


/*-------------------------------------------------------------------------
 * Function:  H5Pset_char_encoding
 *
 * Purpose:   Sets the character encoding of the string.
 *
 * Return:    Non-negative on success/Negative on failure
 *
 * Programmer:        James Laird
 *            Wednesday, October 26, 2005
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_char_encoding(hid_t plist_id, H5T_cset_t encoding)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_char_encoding, FAIL);
    H5TRACE2("e", "iTc", plist_id, encoding);

    /* Check arguments */
    if (encoding <= H5T_CSET_ERROR || encoding >= H5T_NCSET)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "character encoding is not valid")

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_STRING_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Set the character encoding */
    if(H5P_set(plist, H5P_STRCRT_CHAR_ENCODING_NAME, &encoding) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set character encoding")

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5P_set_char_encoding() */


/*-------------------------------------------------------------------------
 * Function:    H5Pget_char_encoding
 *
 * Purpose:     Gets the character encoding of the string.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  James Laird
 *              November 1, 2005
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_char_encoding(hid_t plist_id, H5T_cset_t *encoding /*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value = SUCCEED; /* return value */

    FUNC_ENTER_API(H5Pget_char_encoding, FAIL);
    H5TRACE2("e", "ix", plist_id, encoding);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_STRING_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Get value */
    if(encoding)
        if(H5P_get(plist, H5P_STRCRT_CHAR_ENCODING_NAME, encoding) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get character encoding flag")

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pget_char_encoding() */

