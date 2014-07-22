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
 * Created:		H5Aint.c
 *			Dec 18 2006
 *			Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:		Internal routines for managing attributes.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5A_PACKAGE		/*suppress error about including H5Apkg  */
#define H5O_PACKAGE		/*suppress error about including H5Opkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Apkg.h"		/* Attributes	  			*/
#include "H5Dprivate.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Opkg.h"             /* Object headers			*/
#include "H5SMprivate.h"	/* Shared Object Header Messages	*/


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/

/* Data exchange structure to use when building table of compact attributes for an object */
typedef struct {
    H5F_t       *f;             /* Pointer to file that fractal heap is in */
    hid_t       dxpl_id;        /* DXPL for operation                */
    H5A_attr_table_t *atable;   /* Pointer to attribute table to build */
    size_t curr_attr;           /* Current attribute to operate on */
    hbool_t bogus_crt_idx;      /* Whether bogus creation index values need to be set */
} H5A_compact_bt_ud_t;

/* Data exchange structure to use when building table of dense attributes for an object */
typedef struct {
    H5A_attr_table_t *atable;   /* Pointer to attribute table to build */
    size_t curr_attr;           /* Current attribute to operate on */
} H5A_dense_bt_ud_t;

/* Data exchange structure to use when copying an attribute from _SRC to _DST */
typedef struct {
    const H5O_ainfo_t *ainfo;   /* dense information    */
    H5F_t *file;                /* file                 */
    hbool_t *recompute_size;    /* Flag to indicate if size changed */
    H5O_copy_t *cpy_info;       /* Information on copying options   */
    hid_t dxpl_id;              /* DXPL for operation               */
    const H5O_loc_t *oloc_src;
    H5O_loc_t *oloc_dst;
} H5A_dense_file_cp_ud_t;


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

static herr_t H5A__compact_build_table_cb(H5O_t *oh, H5O_mesg_t *mesg/*in,out*/,
    unsigned sequence, unsigned *oh_flags_ptr, void *_udata/*in,out*/);
static herr_t H5A_dense_build_table_cb(const H5A_t *attr, void *_udata);
static int H5A__attr_cmp_name_inc(const void *attr1, const void *attr2);
static int H5A__attr_cmp_name_dec(const void *attr1, const void *attr2);
static int H5A__attr_cmp_corder_inc(const void *attr1, const void *attr2);
static int H5A__attr_cmp_corder_dec(const void *attr1, const void *attr2);
static herr_t H5A__attr_sort_table(H5A_attr_table_t *atable, H5_index_t idx_type,
    H5_iter_order_t order);

/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/

typedef H5A_t*	H5A_t_ptr;
H5FL_SEQ_DEFINE(H5A_t_ptr);


/*-------------------------------------------------------------------------
 * Function:	H5A__compact_build_table_cb
 *
 * Purpose:	Object header iterator callback routine to copy attribute
 *              into table.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Dec 18 2006
 *
 * Modification:Raymond Lu
 *              24 June 2008
 *              Changed the table of attribute objects to be the table of
 *              pointers to attribute objects for the ease of operation.
 *
 *	Vailin Choi; September 2011
 *	Change "oh_modified" from boolean to unsigned
 *	(See H5Oprivate.h for possible flags)
 *-------------------------------------------------------------------------
 */
static herr_t
H5A__compact_build_table_cb(H5O_t UNUSED *oh, H5O_mesg_t *mesg/*in,out*/,
    unsigned sequence, unsigned UNUSED *oh_modified, void *_udata/*in,out*/)
{
    H5A_compact_bt_ud_t *udata = (H5A_compact_bt_ud_t *)_udata;   /* Operator user data */
    herr_t ret_value = H5_ITER_CONT;    /* Return value */

    FUNC_ENTER_STATIC

    /* check args */
    HDassert(mesg);

    /* Re-allocate the table if necessary */
    if(udata->curr_attr == udata->atable->nattrs) {
        H5A_t **new_table;          /* New table for attributes */
        size_t new_table_size;      /* Number of attributes in new table */

        /* Allocate larger table */
        new_table_size = MAX(1, 2 * udata->atable->nattrs);
        if(NULL == (new_table = (H5A_t **)H5FL_SEQ_REALLOC(H5A_t_ptr, udata->atable->attrs, new_table_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, H5_ITER_ERROR, "unable to extend attribute table")

        /* Update table information in user data */
        udata->atable->attrs = new_table;
        udata->atable->nattrs = new_table_size;
    } /* end if */

    /* Copy attribute into table */
    if(NULL == (udata->atable->attrs[udata->curr_attr] = H5A_copy(NULL, (const H5A_t *)mesg->native)))
        HGOTO_ERROR(H5E_ATTR, H5E_CANTCOPY, H5_ITER_ERROR, "can't copy attribute")

    /* Assign [somewhat arbitrary] creation order value, if requested */
    if(udata->bogus_crt_idx)
        ((udata->atable->attrs[udata->curr_attr])->shared)->crt_idx = sequence;

    /* Increment current attribute */
    udata->curr_attr++;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5A__compact_build_table_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5A_compact_build_table
 *
 * Purpose:     Builds a table containing a sorted list of attributes for
 *              an object
 *
 * Note:	Used for building table of attributes in non-native iteration
 *              order for an index
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *	        Dec 18, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5A_compact_build_table(H5F_t *f, hid_t dxpl_id, H5O_t *oh, H5_index_t idx_type,
    H5_iter_order_t order, H5A_attr_table_t *atable)
{
    H5A_compact_bt_ud_t udata;                  /* User data for iteration callback */
    H5O_mesg_operator_t op;             /* Wrapper for operator */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(f);
    HDassert(oh);
    HDassert(atable);

    /* Initialize table */
    atable->attrs = NULL;
    atable->nattrs = 0;

    /* Set up user data for iteration */
    udata.f = f;
    udata.dxpl_id = dxpl_id;
    udata.atable = atable;
    udata.curr_attr = 0;
    udata.bogus_crt_idx = (hbool_t)((oh->version == H5O_VERSION_1 ||
            !(oh->flags & H5O_HDR_ATTR_CRT_ORDER_TRACKED)) ? TRUE : FALSE);

    /* Iterate over existing attributes, checking for attribute with same name */
    op.op_type = H5O_MESG_OP_LIB;
    op.u.lib_op = H5A__compact_build_table_cb;
    if(H5O_msg_iterate_real(f, oh, H5O_MSG_ATTR, &op, &udata, dxpl_id) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_BADITER, FAIL, "error building attribute table")

    /* Correct # of attributes in table */
    atable->nattrs = udata.curr_attr;

    /* Don't sort an empty table. */
    if(atable->nattrs > 0) {
        /* Sort attribute table in correct iteration order */
        if(H5A__attr_sort_table(atable, idx_type, order) < 0)
            HGOTO_ERROR(H5E_ATTR, H5E_CANTSORT, FAIL, "error sorting attribute table")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5A_compact_build_table() */


/*-------------------------------------------------------------------------
 * Function:	H5A_dense_build_table_cb
 *
 * Purpose:	Callback routine for building table of attributes from dense
 *              attribute storage.
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Dec 11 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5A_dense_build_table_cb(const H5A_t *attr, void *_udata)
{
    H5A_dense_bt_ud_t *udata = (H5A_dense_bt_ud_t *)_udata;     /* 'User data' passed in */
    herr_t ret_value = H5_ITER_CONT;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check arguments */
    HDassert(attr);
    HDassert(udata);
    HDassert(udata->curr_attr < udata->atable->nattrs);

    /* Allocate attribute for entry in the table */
    if(NULL == (udata->atable->attrs[udata->curr_attr] = H5FL_CALLOC(H5A_t)))
        HGOTO_ERROR(H5E_ATTR, H5E_CANTALLOC, H5_ITER_ERROR, "can't allocate attribute")

    /* Copy attribute information.  Share the attribute object in copying. */
    if(NULL == H5A_copy(udata->atable->attrs[udata->curr_attr], attr))
        HGOTO_ERROR(H5E_ATTR, H5E_CANTCOPY, H5_ITER_ERROR, "can't copy attribute")

    /* Increment number of attributes stored */
    udata->curr_attr++;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5A_dense_build_table_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5A_dense_build_table
 *
 * Purpose:     Builds a table containing a sorted list of attributes for
 *              an object
 *
 * Note:	Used for building table of attributes in non-native iteration
 *              order for an index.  Uses the "name" index to retrieve records,
 *		but the 'idx_type' index for sorting them.
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *	        Dec 11, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5A_dense_build_table(H5F_t *f, hid_t dxpl_id, const H5O_ainfo_t *ainfo,
    H5_index_t idx_type, H5_iter_order_t order, H5A_attr_table_t *atable)
{
    H5B2_t *bt2_name = NULL;            /* v2 B-tree handle for name index */
    hsize_t nrec;                       /* # of records in v2 B-tree */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(f);
    HDassert(ainfo);
    HDassert(H5F_addr_defined(ainfo->fheap_addr));
    HDassert(H5F_addr_defined(ainfo->name_bt2_addr));
    HDassert(atable);

    /* Open the name index v2 B-tree */
    if(NULL == (bt2_name = H5B2_open(f, dxpl_id, ainfo->name_bt2_addr, NULL)))
        HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, FAIL, "unable to open v2 B-tree for name index")

    /* Retrieve # of records in "name" B-tree */
    /* (should be same # of records in all indices) */
    if(H5B2_get_nrec(bt2_name, &nrec) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTGET, FAIL, "can't retrieve # of records in index")

    /* Set size of table */
    H5_CHECK_OVERFLOW(nrec, /* From: */ hsize_t, /* To: */ size_t);
    atable->nattrs = (size_t)nrec;

    /* Allocate space for the table entries */
    if(atable->nattrs > 0) {
        H5A_dense_bt_ud_t udata;       /* User data for iteration callback */
        H5A_attr_iter_op_t attr_op;    /* Attribute operator */

        /* Allocate the table to store the attributes */
        if((atable->attrs = (H5A_t **)H5FL_SEQ_CALLOC(H5A_t_ptr, atable->nattrs)) == NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

        /* Set up user data for iteration */
        udata.atable = atable;
        udata.curr_attr = 0;

        /* Build iterator operator */
        attr_op.op_type = H5A_ATTR_OP_LIB;
        attr_op.u.lib_op = H5A_dense_build_table_cb;

        /* Iterate over the links in the group, building a table of the link messages */
        if(H5A_dense_iterate(f, dxpl_id, (hid_t)0, ainfo, H5_INDEX_NAME,
                H5_ITER_NATIVE, (hsize_t)0, NULL, &attr_op, &udata) < 0)
            HGOTO_ERROR(H5E_ATTR, H5E_CANTINIT, FAIL, "error building attribute table")

        /* Sort attribute table in correct iteration order */
        if(H5A__attr_sort_table(atable, idx_type, order) < 0)
            HGOTO_ERROR(H5E_ATTR, H5E_CANTSORT, FAIL, "error sorting attribute table")
    } /* end if */
    else
        atable->attrs = NULL;

done:
    /* Release resources */
    if(bt2_name && H5B2_close(bt2_name, dxpl_id) < 0)
        HDONE_ERROR(H5E_ATTR, H5E_CLOSEERROR, FAIL, "can't close v2 B-tree for name index")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5A_dense_build_table() */


/*-------------------------------------------------------------------------
 * Function:	H5A__attr_cmp_name_inc
 *
 * Purpose:	Callback routine for comparing two attribute names, in
 *              increasing alphabetic order
 *
 * Return:	An integer less than, equal to, or greater than zero if the
 *              first argument is considered to be respectively less than,
 *              equal to, or greater than the second.  If two members compare
 *              as equal, their order in the sorted array is undefined.
 *              (i.e. same as strcmp())
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Dec 11 2006
 *
 *-------------------------------------------------------------------------
 */
static int
H5A__attr_cmp_name_inc(const void *attr1, const void *attr2)
{
    FUNC_ENTER_STATIC_NOERR

    FUNC_LEAVE_NOAPI(HDstrcmp((*(const H5A_t * const *)attr1)->shared->name,
            (*(const H5A_t * const *)attr2)->shared->name))
} /* end H5A__attr_cmp_name_inc() */


/*-------------------------------------------------------------------------
 * Function:	H5A__attr_cmp_name_dec
 *
 * Purpose:	Callback routine for comparing two attribute names, in
 *              decreasing alphabetic order
 *
 * Return:	An integer less than, equal to, or greater than zero if the
 *              second argument is considered to be respectively less than,
 *              equal to, or greater than the first.  If two members compare
 *              as equal, their order in the sorted array is undefined.
 *              (i.e. opposite of strcmp())
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Feb  8 2007
 *
 *-------------------------------------------------------------------------
 */
static int
H5A__attr_cmp_name_dec(const void *attr1, const void *attr2)
{
    FUNC_ENTER_STATIC_NOERR

    FUNC_LEAVE_NOAPI(HDstrcmp((*(const H5A_t * const *)attr2)->shared->name,
            (*(const H5A_t * const *)attr1)->shared->name))
} /* end H5A__attr_cmp_name_dec() */


/*-------------------------------------------------------------------------
 * Function:	H5A__attr_cmp_corder_inc
 *
 * Purpose:	Callback routine for comparing two attributes, in
 *              increasing creation order
 *
 * Return:	An integer less than, equal to, or greater than zero if the
 *              first argument is considered to be respectively less than,
 *              equal to, or greater than the second.  If two members compare
 *              as equal, their order in the sorted array is undefined.
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Feb  8 2007
 *
 *-------------------------------------------------------------------------
 */
static int
H5A__attr_cmp_corder_inc(const void *attr1, const void *attr2)
{
    int ret_value;              /* Return value */

    FUNC_ENTER_STATIC_NOERR

    if((*(const H5A_t * const *)attr1)->shared->crt_idx < (*(const H5A_t * const *)attr2)->shared->crt_idx)
        ret_value = -1;
    else if((*(const H5A_t * const *)attr1)->shared->crt_idx > (*(const H5A_t * const *)attr2)->shared->crt_idx)
        ret_value = 1;
    else
        ret_value = 0;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5A__attr_cmp_corder_inc() */


/*-------------------------------------------------------------------------
 * Function:	H5A__attr_cmp_corder_dec
 *
 * Purpose:	Callback routine for comparing two attributes, in
 *              decreasing creation order
 *
 * Return:	An integer less than, equal to, or greater than zero if the
 *              second argument is considered to be respectively less than,
 *              equal to, or greater than the first.  If two members compare
 *              as equal, their order in the sorted array is undefined.
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Feb  8 2007
 *
 *-------------------------------------------------------------------------
 */
static int
H5A__attr_cmp_corder_dec(const void *attr1, const void *attr2)
{
    int ret_value;              /* Return value */

    FUNC_ENTER_STATIC_NOERR

    if((*(const H5A_t * const *)attr1)->shared->crt_idx < (*(const H5A_t * const *)attr2)->shared->crt_idx)
        ret_value = 1;
    else if((*(const H5A_t * const *)attr1)->shared->crt_idx > (*(const H5A_t * const *)attr2)->shared->crt_idx)
        ret_value = -1;
    else
        ret_value = 0;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5A__attr_cmp_corder_dec() */


/*-------------------------------------------------------------------------
 * Function:	H5A__attr_sort_table
 *
 * Purpose:     Sort table containing a list of attributes for an object
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *	        Dec 11, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5A__attr_sort_table(H5A_attr_table_t *atable, H5_index_t idx_type,
    H5_iter_order_t order)
{
    FUNC_ENTER_STATIC_NOERR

    /* Sanity check */
    HDassert(atable);

    /* Pick appropriate comparison routine */
    if(idx_type == H5_INDEX_NAME) {
        if(order == H5_ITER_INC)
            HDqsort(atable->attrs, atable->nattrs, sizeof(H5A_t*), H5A__attr_cmp_name_inc);
        else if(order == H5_ITER_DEC)
            HDqsort(atable->attrs, atable->nattrs, sizeof(H5A_t*), H5A__attr_cmp_name_dec);
        else
            HDassert(order == H5_ITER_NATIVE);
    } /* end if */
    else {
        HDassert(idx_type == H5_INDEX_CRT_ORDER);
        if(order == H5_ITER_INC)
            HDqsort(atable->attrs, atable->nattrs, sizeof(H5A_t*), H5A__attr_cmp_corder_inc);
        else if(order == H5_ITER_DEC)
            HDqsort(atable->attrs, atable->nattrs, sizeof(H5A_t*), H5A__attr_cmp_corder_dec);
        else
            HDassert(order == H5_ITER_NATIVE);
    } /* end else */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5A__attr_sort_table() */


/*-------------------------------------------------------------------------
 * Function:	H5A_attr_iterate_table
 *
 * Purpose:     Iterate over table containing a list of attributes for an object,
 *              making appropriate callbacks
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *	        Dec 18, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5A_attr_iterate_table(const H5A_attr_table_t *atable, hsize_t skip,
    hsize_t *last_attr, hid_t loc_id, const H5A_attr_iter_op_t *attr_op,
    void *op_data)
{
    size_t u;                           /* Local index variable */
    herr_t ret_value = H5_ITER_CONT;   /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(atable);
    HDassert(attr_op);

    /* Skip over attributes, if requested */
    if(last_attr)
        *last_attr = skip;

    /* Iterate over attribute messages */
    H5_ASSIGN_OVERFLOW(/* To: */ u, /* From: */ skip, /* From: */ hsize_t, /* To: */ size_t)
    for(; u < atable->nattrs && !ret_value; u++) {
        /* Check which type of callback to make */
        switch(attr_op->op_type) {
            case H5A_ATTR_OP_APP2:
            {
                H5A_info_t ainfo;               /* Info for attribute */

                /* Get the attribute information */
                if(H5A_get_info(atable->attrs[u], &ainfo) < 0)
                    HGOTO_ERROR(H5E_ATTR, H5E_CANTGET, H5_ITER_ERROR, "unable to get attribute info")

                /* Make the application callback */
                ret_value = (attr_op->u.app_op2)(loc_id, ((atable->attrs[u])->shared)->name, &ainfo, op_data);
                break;
            }

#ifndef H5_NO_DEPRECATED_SYMBOLS
            case H5A_ATTR_OP_APP:
                /* Make the application callback */
                ret_value = (attr_op->u.app_op)(loc_id, ((atable->attrs[u])->shared)->name, op_data);
                break;
#endif /* H5_NO_DEPRECATED_SYMBOLS */

            case H5A_ATTR_OP_LIB:
                /* Call the library's callback */
                ret_value = (attr_op->u.lib_op)((atable->attrs[u]), op_data);
                break;

            default:
                HDassert("unknown attribute op type" && 0);
#ifdef NDEBUG
                HGOTO_ERROR(H5E_ATTR, H5E_UNSUPPORTED, FAIL, "unsupported attribute op type")
#endif /* NDEBUG */
        } /* end switch */

        /* Increment the number of entries passed through */
        if(last_attr)
            (*last_attr)++;
    } /* end for */

    /* Check for callback failure and pass along return value */
    if(ret_value < 0)
        HERROR(H5E_ATTR, H5E_CANTNEXT, "iteration operator failed");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5A_attr_iterate_table() */


/*-------------------------------------------------------------------------
 * Function:	  H5A_attr_release_table
 *
 * Purpose:       Release table containing a list of attributes for an object
 *
 * Return:	  Success:        Non-negative
 *		  Failure:	Negative
 *
 * Programmer:	  Quincey Koziol
 *	          Dec 11, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5A_attr_release_table(H5A_attr_table_t *atable)
{
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(atable);

    /* Release attribute info, if any. */
    if(atable->nattrs > 0) {
        size_t      u;               /* Local index variable */

        /* Free attribute message information */
        for(u = 0; u < atable->nattrs; u++)
            if(atable->attrs[u] && H5A_close(atable->attrs[u]) < 0)
                HGOTO_ERROR(H5E_ATTR, H5E_CANTFREE, FAIL, "unable to release attribute")
    } /* end if */
    else
        HDassert(atable->attrs == NULL);

    atable->attrs = (H5A_t **)H5FL_SEQ_FREE(H5A_t_ptr, atable->attrs);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5A_attr_release_table() */


/*-------------------------------------------------------------------------
 * Function:    H5A_get_ainfo
 *
 * Purpose:     Retrieves the "attribute info" message for an object.  Also
 *              sets the number of attributes correctly, if it isn't set up yet.
 *
 * Return:	Success:	TRUE/FALSE whether message was found & retrieved
 *              Failure:        FAIL if error occurred
 *
 * Programmer:  Quincey Koziol
 *              koziol@hdfgroup.org
 *              Mar 11 2007
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5A_get_ainfo(H5F_t *f, hid_t dxpl_id, H5O_t *oh, H5O_ainfo_t *ainfo)
{
    H5B2_t *bt2_name = NULL;            /* v2 B-tree handle for name index */
    htri_t ret_value;   /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check arguments */
    HDassert(f);
    HDassert(oh);
    HDassert(ainfo);

    /* Check if the "attribute info" message exists */
    if((ret_value = H5O_msg_exists_oh(oh, H5O_AINFO_ID)) < 0)
	HGOTO_ERROR(H5E_ATTR, H5E_NOTFOUND, FAIL, "unable to check object header")
    if(ret_value > 0) {
        /* Retrieve the "attribute info" structure */
        if(NULL == H5O_msg_read_oh(f, dxpl_id, oh, H5O_AINFO_ID, ainfo))
	    HGOTO_ERROR(H5E_ATTR, H5E_CANTGET, FAIL, "can't read AINFO message")

        /* Check if we don't know how many attributes there are */
        if(ainfo->nattrs == HSIZET_MAX) {
            /* Check if we are using "dense" attribute storage */
            if(H5F_addr_defined(ainfo->fheap_addr)) {
                /* Open the name index v2 B-tree */
                if(NULL == (bt2_name = H5B2_open(f, dxpl_id, ainfo->name_bt2_addr, NULL)))
                    HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, FAIL, "unable to open v2 B-tree for name index")

                /* Retrieve # of records in "name" B-tree */
                /* (should be same # of records in all indices) */
                if(H5B2_get_nrec(bt2_name, &ainfo->nattrs) < 0)
                    HGOTO_ERROR(H5E_ATTR, H5E_CANTGET, FAIL, "can't retrieve # of records in index")
            } /* end if */
            else
                /* Retrieve # of attributes from object header */
                ainfo->nattrs = oh->attr_msgs_seen;
        } /* end if */
    } /* end if */

done:
    /* Release resources */
    if(bt2_name && H5B2_close(bt2_name, dxpl_id) < 0)
        HDONE_ERROR(H5E_ATTR, H5E_CLOSEERROR, FAIL, "can't close v2 B-tree for name index")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5A_get_ainfo() */


/*-------------------------------------------------------------------------
 * Function:    H5A_set_version
 *
 * Purpose:     Sets the correct version to encode attribute with.
 *              Chooses the oldest version possible, unless the "use the
 *              latest format" flag is set.
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:  Quincey Koziol
 *              koziol@hdfgroup.org
 *              Jul 17 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5A_set_version(const H5F_t *f, H5A_t *attr)
{
    hbool_t type_shared, space_shared;  /* Flags to indicate that shared messages are used for this attribute */
    hbool_t use_latest_format;          /* Flag indicating the newest file format should be used */

    FUNC_ENTER_NOAPI_NOERR

    /* check arguments */
    HDassert(f);
    HDassert(attr);

    /* Get the file's 'use the latest version of the format' flag */
    use_latest_format = H5F_USE_LATEST_FORMAT(f);

    /* Check whether datatype and dataspace are shared */
    if(H5O_msg_is_shared(H5O_DTYPE_ID, attr->shared->dt) > 0)
        type_shared = TRUE;
    else
        type_shared = FALSE;

    if(H5O_msg_is_shared(H5O_SDSPACE_ID, attr->shared->ds) > 0)
        space_shared = TRUE;
    else
        space_shared = FALSE;

    /* Check which version to encode attribute with */
    if(use_latest_format)
        attr->shared->version = H5O_ATTR_VERSION_LATEST;      /* Write out latest version of format */
    else if(attr->shared->encoding != H5T_CSET_ASCII)
        attr->shared->version = H5O_ATTR_VERSION_3;   /* Write version which includes the character encoding */
    else if(type_shared || space_shared)
        attr->shared->version = H5O_ATTR_VERSION_2;   /* Write out version with flag for indicating shared datatype or dataspace */
    else
        attr->shared->version = H5O_ATTR_VERSION_1;   /* Write out basic version */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5A_set_version() */


/*-------------------------------------------------------------------------
 * Function:    H5A_attr_copy_file
 *
 * Purpose:     Copies a message from _MESG to _DEST in file
 *
 *              Note that this function assumes that it is copying *all*
 *              the attributes in the object, specifically when it copies
 *              the creation order from source to destination.  If this is
 *              to be used to copy only a single attribute, then the
 *              creation order must be handled differently.  -NAF
 *
 * Return:      Success:        Ptr to _DEST
 *
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              November 1, 2005
 *
 *-------------------------------------------------------------------------
 */
H5A_t *
H5A_attr_copy_file(const H5A_t *attr_src, H5F_t *file_dst, hbool_t *recompute_size,
    H5O_copy_t *cpy_info, hid_t dxpl_id)
{
    H5A_t        *attr_dst = NULL;

    /* for dataype conversion */
    hid_t       tid_src = -1;           /* Datatype ID for source datatype */
    hid_t       tid_dst = -1;           /* Datatype ID for destination datatype */
    hid_t       tid_mem = -1;           /* Datatype ID for memory datatype */
    void       *buf = NULL;             /* Buffer for copying data */
    void       *reclaim_buf = NULL;     /* Buffer for reclaiming data */
    hid_t       buf_sid = -1;           /* ID for buffer dataspace */

    H5A_t       *ret_value;             /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(attr_src);
    HDassert(file_dst);
    HDassert(cpy_info);
    HDassert(!cpy_info->copy_without_attr);

    /* Allocate space for the destination message */
    if(NULL == (attr_dst = H5FL_CALLOC(H5A_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Copy the top level of the attribute */
    *attr_dst = *attr_src;

    if(NULL == (attr_dst->shared = H5FL_CALLOC(H5A_shared_t)))
        HGOTO_ERROR(H5E_FILE, H5E_NOSPACE, NULL, "can't allocate shared attr structure")

    /* Don't have an opened group location for copy */
    H5O_loc_reset(&(attr_dst->oloc));
    H5G_name_reset(&(attr_dst->path));
    attr_dst->obj_opened = FALSE;

    /* Reference count for the header message in the cache */
    attr_dst->shared->nrefs = 1;

    /* Copy attribute's name */
    attr_dst->shared->name = H5MM_strdup(attr_src->shared->name);
    HDassert(attr_dst->shared->name);
    attr_dst->shared->encoding = attr_src->shared->encoding;

    /* Copy attribute's datatype */
    /* If source is named, we will keep dst as named, but we will not actually
     * copy the target and update the message until post copy */
    if(NULL == (attr_dst->shared->dt = H5T_copy(attr_src->shared->dt, H5T_COPY_ALL)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, NULL, "cannot copy datatype")

    /* Set the location of the destination datatype */
    if(H5T_set_loc(attr_dst->shared->dt, file_dst, H5T_LOC_DISK) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "cannot mark datatype on disk")

    if(!H5T_committed(attr_src->shared->dt)) {
        /* If the datatype is not named, it may have been shared in the
         * source file's heap.  Un-share it for now. We'll try to shared
         * it in the destination file below.
         */
        if(H5O_msg_reset_share(H5O_DTYPE_ID, attr_dst->shared->dt) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "unable to reset datatype sharing")
    } /* end if */

    /* Copy the dataspace for the attribute. Make sure the maximal dimension is also copied.
     * Otherwise the comparison in the test may complain about it. SLU 2011/4/12 */
    attr_dst->shared->ds = H5S_copy(attr_src->shared->ds, FALSE, TRUE);
    HDassert(attr_dst->shared->ds);

    /* Reset the dataspace's sharing in the source file before trying to share
     * it in the destination.
     */
    if(H5O_msg_reset_share(H5O_SDSPACE_ID, attr_dst->shared->ds) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "unable to reset dataspace sharing")

    /* Simulate trying to share both the datatype and dataset, to determine the
     * final size of the messages.  This does nothing if the datatype is
     * committed or sharing is disabled.
     */
    if(H5SM_try_share(file_dst, dxpl_id, NULL, H5SM_DEFER, H5O_DTYPE_ID, attr_dst->shared->dt, NULL) < 0)
	HGOTO_ERROR(H5E_OHDR, H5E_WRITEERROR, NULL, "can't share attribute datatype")
    if(H5SM_try_share(file_dst, dxpl_id, NULL, H5SM_DEFER, H5O_SDSPACE_ID, attr_dst->shared->ds, NULL) < 0)
	HGOTO_ERROR(H5E_OHDR, H5E_WRITEERROR, NULL, "can't share attribute dataspace")

    /* Compute the sizes of the datatype and dataspace. This is their raw
     * size unless they're shared.
     */
    attr_dst->shared->dt_size = H5O_msg_raw_size(file_dst, H5O_DTYPE_ID, FALSE, attr_dst->shared->dt);
    HDassert(attr_dst->shared->dt_size > 0);
    attr_dst->shared->ds_size = H5O_msg_raw_size(file_dst, H5O_SDSPACE_ID, FALSE, attr_dst->shared->ds);
    HDassert(attr_dst->shared->ds_size > 0);

    /* Check whether to recompute the size of the attribute */
    /* (happens when the datatype or dataspace changes sharing status) */
    if(attr_dst->shared->dt_size != attr_src->shared->dt_size || attr_dst->shared->ds_size != attr_src->shared->ds_size)
        *recompute_size = TRUE;

    /* Compute the size of the data */
    H5_ASSIGN_OVERFLOW(attr_dst->shared->data_size, H5S_GET_EXTENT_NPOINTS(attr_dst->shared->ds) * H5T_get_size(attr_dst->shared->dt), hssize_t, size_t);

    /* Copy (& convert) the data, if necessary */
    if(attr_src->shared->data) {
        if(NULL == (attr_dst->shared->data = H5FL_BLK_MALLOC(attr_buf, attr_dst->shared->data_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

        /* Check if we need to convert data */
        if(H5T_detect_class(attr_src->shared->dt, H5T_VLEN, FALSE) > 0) {
            H5T_path_t  *tpath_src_mem, *tpath_mem_dst;   /* Datatype conversion paths */
            H5T_t *dt_mem;              /* Memory datatype */
            size_t src_dt_size;         /* Source datatype size */
            size_t tmp_dt_size;         /* Temp. datatype size */
            size_t max_dt_size;         /* Max atatype size */
            H5S_t *buf_space;           /* Dataspace describing buffer */
            hsize_t buf_dim;            /* Dimension for buffer */
            size_t nelmts;              /* Number of elements in buffer */
            size_t buf_size;            /* Size of copy buffer */

            /* Create datatype ID for src datatype */
            if((tid_src = H5I_register(H5I_DATATYPE, attr_src->shared->dt, FALSE)) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, NULL, "unable to register source file datatype")

            /* create a memory copy of the variable-length datatype */
            if(NULL == (dt_mem = H5T_copy(attr_src->shared->dt, H5T_COPY_TRANSIENT)))
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to copy")
            if((tid_mem = H5I_register(H5I_DATATYPE, dt_mem, FALSE)) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, NULL, "unable to register memory datatype")

            /* create variable-length datatype at the destinaton file */
            if((tid_dst = H5I_register(H5I_DATATYPE, attr_dst->shared->dt, FALSE)) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, NULL, "unable to register destination file datatype")

            /* Set up the conversion functions */
            if(NULL == (tpath_src_mem = H5T_path_find(attr_src->shared->dt, dt_mem, NULL, NULL, dxpl_id, FALSE)))
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to convert between src and mem datatypes")
            if(NULL == (tpath_mem_dst = H5T_path_find(dt_mem, attr_dst->shared->dt, NULL, NULL, dxpl_id, FALSE)))
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to convert between mem and dst datatypes")

            /* Determine largest datatype size */
            if(0 == (src_dt_size = H5T_get_size(attr_src->shared->dt)))
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to determine datatype size")
            if(0 == (tmp_dt_size = H5T_get_size(dt_mem)))
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to determine datatype size")
            max_dt_size = MAX(src_dt_size, tmp_dt_size);
            if(0 == (tmp_dt_size = H5T_get_size(attr_dst->shared->dt)))
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to determine datatype size")
            max_dt_size = MAX(max_dt_size, tmp_dt_size);

            /* Set number of whole elements that fit in buffer */
            if(0 == (nelmts = attr_src->shared->data_size / src_dt_size))
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "element size too large")

            /* Set up number of bytes to copy, and initial buffer size */
            buf_size = nelmts * max_dt_size;

            /* Create dataspace for number of elements in buffer */
            buf_dim = nelmts;

            /* Create the space and set the initial extent */
            if(NULL == (buf_space = H5S_create_simple((unsigned)1, &buf_dim, NULL)))
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCREATE, NULL, "can't create simple dataspace")

            /* Atomize */
            if((buf_sid = H5I_register(H5I_DATASPACE, buf_space, FALSE)) < 0) {
                H5S_close(buf_space);
                HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, NULL, "unable to register dataspace ID")
            } /* end if */

            /* Allocate memory for recclaim buf */
            if(NULL == (reclaim_buf = H5FL_BLK_MALLOC(attr_buf, buf_size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation NULLed for raw data chunk")

            /* Allocate memory for copying the chunk */
            if(NULL == (buf = H5FL_BLK_MALLOC(attr_buf, buf_size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation NULLed for raw data chunk")

            HDmemcpy(buf, attr_src->shared->data, attr_src->shared->data_size);

            /* Convert from source file to memory */
            if(H5T_convert(tpath_src_mem, tid_src, tid_mem, nelmts, (size_t)0, (size_t)0, buf, NULL, dxpl_id) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "datatype conversion NULLed")

            HDmemcpy(reclaim_buf, buf, buf_size);

            /* Convert from memory to destination file */
            if(H5T_convert(tpath_mem_dst, tid_mem, tid_dst, nelmts, (size_t)0, (size_t)0, buf, NULL, dxpl_id) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "datatype conversion NULLed")

            HDmemcpy(attr_dst->shared->data, buf, attr_dst->shared->data_size);

            if(H5D_vlen_reclaim(tid_mem, buf_space, H5P_DATASET_XFER_DEFAULT, reclaim_buf) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_BADITER, NULL, "unable to reclaim variable-length data")
        }  /* end if */
        else {
            HDassert(attr_dst->shared->data_size == attr_src->shared->data_size);
            HDmemcpy(attr_dst->shared->data, attr_src->shared->data, attr_src->shared->data_size);
        } /* end else */
    } /* end if(attr_src->shared->data) */

    /* Copy the creation order */
    attr_dst->shared->crt_idx = attr_src->shared->crt_idx;

    /* Recompute the version to encode the destination attribute */
    if(H5A_set_version(file_dst, attr_dst) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTSET, NULL, "unable to update attribute version")

    /* Recompute the destination attribute's size, if it's a different version */
    if(attr_src->shared->version != attr_dst->shared->version)
        *recompute_size = TRUE;

    /* Set return value */
    ret_value = attr_dst;

done:
    if(buf_sid > 0 && H5I_dec_ref(buf_sid) < 0)
        HDONE_ERROR(H5E_ATTR, H5E_CANTFREE, NULL, "Can't decrement temporary dataspace ID")
    if(tid_src > 0)
        /* Don't decrement ID, we want to keep underlying datatype */
        if(NULL == H5I_remove(tid_src))
            HDONE_ERROR(H5E_ATTR, H5E_CANTFREE, NULL, "Can't decrement temporary datatype ID")
    if(tid_dst > 0)
        /* Don't decrement ID, we want to keep underlying datatype */
        if(NULL == H5I_remove(tid_dst))
            HDONE_ERROR(H5E_ATTR, H5E_CANTFREE, NULL, "Can't decrement temporary datatype ID")
    if(tid_mem > 0)
        /* Decrement the memory datatype ID, it's transient */
        if(H5I_dec_ref(tid_mem) < 0)
            HDONE_ERROR(H5E_ATTR, H5E_CANTFREE, NULL, "Can't decrement temporary datatype ID")
    if(buf)
        buf = H5FL_BLK_FREE(attr_buf, buf);
    if(reclaim_buf)
        reclaim_buf = H5FL_BLK_FREE(attr_buf, reclaim_buf);

    /* Release destination attribute information on failure */
    if(!ret_value && attr_dst && H5A_close(attr_dst) < 0)
        HDONE_ERROR(H5E_ATTR, H5E_CANTFREE, NULL, "can't close attribute")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5A_attr_copy_file() */


/*-------------------------------------------------------------------------
 * Function:    H5A_attr_post_copy_file
 *
 * Purpose:     Finish copying a message from between files.
 *              We have to copy the values of a reference attribute in the
 *              post copy because H5O_post_copy_file() fails at the case that
 *              an object may have a reference attribute that points to the
 *              object itself.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Peter Cao
 *              March 6, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5A_attr_post_copy_file(const H5O_loc_t *src_oloc, const H5A_t *attr_src,
    H5O_loc_t *dst_oloc, const H5A_t *attr_dst, hid_t dxpl_id, H5O_copy_t *cpy_info)
{
    H5F_t  *file_src, *file_dst;
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(src_oloc);
    HDassert(dst_oloc);
    HDassert(attr_dst);
    HDassert(attr_src);

    file_src = src_oloc->file;
    file_dst = dst_oloc->file;

    HDassert(file_src);
    HDassert(file_dst);

    if(H5T_committed(attr_src->shared->dt)) {
        H5O_loc_t         *src_oloc_dt;           /* Pointer to source datatype's object location */
        H5O_loc_t         *dst_oloc_dt;           /* Pointer to dest. datatype's object location */

        /* Get group entries for source & destination */
        src_oloc_dt = H5T_oloc(attr_src->shared->dt);
        HDassert(src_oloc_dt);
        dst_oloc_dt = H5T_oloc(attr_dst->shared->dt);
        HDassert(dst_oloc_dt);

        /* Reset object location for new object */
        H5O_loc_reset(dst_oloc_dt);
        dst_oloc_dt->file = file_dst;

        /* Copy the shared object from source to destination */
        if(H5O_copy_header_map(src_oloc_dt, dst_oloc_dt, dxpl_id, cpy_info, FALSE, NULL, NULL) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to copy object")

        /* Update shared message info from named datatype info */
        H5T_update_shared(attr_dst->shared->dt);
    } /* end if */

    /* Try to share both the datatype and dataset.  This does nothing if the
     * datatype is committed or sharing is disabled.
     */
    if(H5SM_try_share(file_dst, dxpl_id, NULL, H5SM_WAS_DEFERRED, H5O_DTYPE_ID, attr_dst->shared->dt, NULL) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_WRITEERROR, FAIL, "can't share attribute datatype")
    if(H5SM_try_share(file_dst, dxpl_id, NULL, H5SM_WAS_DEFERRED, H5O_SDSPACE_ID, attr_dst->shared->ds, NULL) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_WRITEERROR, FAIL, "can't share attribute dataspace")

    /* Only need to fix reference attribute with real data being copied to
     *  another file.
     */
    if((NULL != attr_dst->shared->data) && (H5T_get_class(attr_dst->shared->dt, FALSE) == H5T_REFERENCE) ) {

        /* copy object pointed by reference. The current implementation does not
         *  deal with nested reference such as reference in a compound structure
         */

        /* Check for expanding references */
        if(cpy_info->expand_ref) {
            size_t ref_count;

            /* Determine # of reference elements to copy */
            ref_count = attr_dst->shared->data_size / H5T_get_size(attr_dst->shared->dt);

            /* Copy objects referenced in source buffer to destination file and set destination elements */
            if(H5O_copy_expand_ref(file_src, attr_dst->shared->data, dxpl_id,
                    file_dst, attr_dst->shared->data, ref_count, H5T_get_ref_type(attr_dst->shared->dt), cpy_info) < 0)
                HGOTO_ERROR(H5E_ATTR, H5E_CANTCOPY, FAIL, "unable to copy reference attribute")
        } /* end if */
        else
            /* Reset value to zero */
            HDmemset(attr_dst->shared->data, 0, attr_dst->shared->data_size);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5A_attr_post_copy_file() */


/*-------------------------------------------------------------------------
 * Function:    H5A_dense_post_copy_file_cb
 *
 * Purpose:     Callback routine for copying a dense attribute from SRC to DST.
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Peter Cao
 *              xcao@hdfgroup.org
 *              July 20, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5A_dense_post_copy_file_cb(const H5A_t *attr_src, void *_udata)
{
    H5A_dense_file_cp_ud_t *udata = (H5A_dense_file_cp_ud_t *)_udata;
    H5A_t *attr_dst = NULL;
    herr_t ret_value = H5_ITER_CONT;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check arguments */
    HDassert(attr_src);
    HDassert(udata);
    HDassert(udata->ainfo);
    HDassert(udata->file);
    HDassert(udata->cpy_info);

    if(NULL == (attr_dst = H5A_attr_copy_file(attr_src, udata->file,
            udata->recompute_size, udata->cpy_info,  udata->dxpl_id)))
        HGOTO_ERROR(H5E_ATTR, H5E_CANTCOPY, H5_ITER_ERROR, "can't copy attribute")

    if(H5A_attr_post_copy_file(udata->oloc_src, attr_src, udata->oloc_dst, attr_dst,
            udata->dxpl_id, udata->cpy_info) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTCOPY, H5_ITER_ERROR, "can't copy attribute")

    /* Reset shared location information */
    if(H5O_msg_reset_share(H5O_ATTR_ID, attr_dst) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to reset attribute sharing")

    /* Insert attribute into dense storage */
    if(H5A_dense_insert(udata->file, udata->dxpl_id, udata->ainfo, attr_dst) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, H5_ITER_ERROR, "unable to add to dense storage")

done:
    if(attr_dst && H5A_close(attr_dst) < 0)
        HDONE_ERROR(H5E_ATTR, H5E_CLOSEERROR, FAIL, "can't close destination attribute")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5A_dense_post_copy_file_cb() */


/*-------------------------------------------------------------------------
 * Function:    H5A_dense_post_copy_file_all
 *
 * Purpose:     Copy all dense attributes from SRC to DST.
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Peter Cao
 *              xcao@hdfgroup.org
 *              July 20, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5A_dense_post_copy_file_all(const H5O_loc_t *src_oloc, const H5O_ainfo_t *ainfo_src,
    H5O_loc_t *dst_oloc, H5O_ainfo_t *ainfo_dst, hid_t dxpl_id, H5O_copy_t *cpy_info)
{
    H5A_dense_file_cp_ud_t udata;       /* User data for iteration callback */
    H5A_attr_iter_op_t attr_op;         /* Attribute operator */
    hbool_t recompute_size = FALSE;     /* recompute the size */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check arguments */
    HDassert(ainfo_src);
    HDassert(ainfo_dst);

    udata.ainfo = ainfo_dst;                  /* Destination dense information    */
    udata.file = dst_oloc->file;              /* Destination file                 */
    udata.recompute_size = &recompute_size;   /* Flag to indicate if size changed */
    udata.cpy_info = cpy_info;                /* Information on copying options   */
    udata.dxpl_id = dxpl_id;                  /* DXPL for operation               */
    udata.oloc_src = src_oloc;
    udata.oloc_dst = dst_oloc;

    attr_op.op_type = H5A_ATTR_OP_LIB;
    attr_op.u.lib_op = H5A_dense_post_copy_file_cb;


     if(H5A_dense_iterate(src_oloc->file, dxpl_id, (hid_t)0, ainfo_src, H5_INDEX_NAME,
            H5_ITER_NATIVE, (hsize_t)0, NULL, &attr_op, &udata) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTINIT, FAIL, "error building attribute table")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5A_dense_post_copy_file_all */

