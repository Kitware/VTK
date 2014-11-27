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
 * Module Info: This module contains most of the "core" functionality of
 *      the H5T interface, including the API initialization code, etc.
 *      Many routines that are infrequently used, or are specialized for
 *      one particular datatype class are in another module.
 */

/*-------------------------------------------------------------------------
 *
 * Created:		H5Tvisit.c
 *			Jul 19 2007
 *			Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:		Visit all the components of a datatype
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5T_PACKAGE		/*suppress error about including H5Tpkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Tpkg.h"		/* Datatypes         			*/


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
 * Function:    H5T__visit
 *
 * Purpose:     Visit a datatype and all it's members and/or parents, making
 *              a callback for each.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, July 19, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__visit(H5T_t *dt, unsigned visit_flags, H5T_operator_t op, void *op_value)
{
    hbool_t is_complex;                 /* Flag indicating current datatype is "complex" */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    HDassert(dt);
    HDassert(op);

    /* Check for complex datatype */
    is_complex  = H5T_IS_COMPLEX(dt->shared->type);

    /* If the callback is to be made on the datatype first, do that */
    if(is_complex && (visit_flags & H5T_VISIT_COMPLEX_FIRST))
        if(op(dt, op_value) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_BADITER, FAIL, "operator callback failed")

    /* Make callback for each member/child, if requested */
    switch(dt->shared->type) {
        case H5T_COMPOUND:
            {
                unsigned u;             /* Local index variable */

                /* Visit each member of the compound datatype */
                for(u = 0; u < dt->shared->u.compnd.nmembs; u++)
                    if(H5T__visit(dt->shared->u.compnd.memb[u].type, visit_flags, op, op_value) < 0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_BADITER, FAIL, "can't visit member datatype")
            } /* end case */
            break;

        case H5T_ARRAY:
        case H5T_VLEN:
        case H5T_ENUM:
            /* Visit parent type */
            if(H5T__visit(dt->shared->parent, visit_flags, op, op_value) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADITER, FAIL, "can't visit parent datatype")
            break;

        default:
            /* Visit "simple" datatypes here */
            if(visit_flags & H5T_VISIT_SIMPLE)
                if(op(dt, op_value) < 0)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_BADITER, FAIL, "operator callback failed")
            break;
    } /* end switch */

    /* If the callback is to be made on the datatype last, do that */
    if(is_complex && (visit_flags & H5T_VISIT_COMPLEX_LAST))
        if(op(dt, op_value) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_BADITER, FAIL, "operator callback failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__visit() */

