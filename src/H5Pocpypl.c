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
 * Created:		H5Pocpypl.c
 *			Mar 13 2006
 *			Peter Cao <xcao@ncsa.uiuc.edu>
 *
 * Purpose:		Object copying property list class routines
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
#include "H5FLprivate.h"        /* Free Lists                           */
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"        /* Memory management                    */
#include "H5Ppkg.h"		/* Property lists		  	*/


/****************/
/* Local Macros */
/****************/

/* ========= Object Copy properties ============ */
/* Definitions for copy options */
#define H5O_CPY_OPTION_SIZE			sizeof(unsigned)
#define H5O_CPY_OPTION_DEF			0
/* Definitions for merge committed dtype list */
#define H5O_CPY_MERGE_COMM_DT_LIST_SIZE        sizeof(char *)
#define H5O_CPY_MERGE_COMM_DT_LIST_DEF         NULL
#define H5O_CPY_MERGE_COMM_DT_LIST_COPY        H5P__ocpy_merge_comm_dt_list_copy
#define H5O_CPY_MERGE_COMM_DT_LIST_CMP         H5P__ocpy_merge_comm_dt_list_cmp
#define H5O_CPY_MERGE_COMM_DT_LIST_CLOSE       H5P__ocpy_merge_comm_dt_list_close
/* Definitions for callback function when completing the search for a matching committed datatype from the committed dtype list */
#define H5O_CPY_MCDT_SEARCH_CB_SIZE           sizeof(H5O_mcdt_cb_info_t)
#define H5O_CPY_MCDT_SEARCH_CB_DEF            {NULL,NULL} 


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

/* General routines */
static H5O_copy_dtype_merge_list_t *H5P__free_merge_comm_dtype_list(H5O_copy_dtype_merge_list_t *dt_list);

/* Property class callbacks */
static herr_t H5P__ocpy_reg_prop(H5P_genclass_t *pclass);

/* Property callbacks */
static herr_t H5P__ocpy_merge_comm_dt_list_copy(const char* name, size_t size, void* value);
static int H5P__ocpy_merge_comm_dt_list_cmp(const void *value1, const void *value2, size_t size);
static herr_t H5P__ocpy_merge_comm_dt_list_close(const char* name, size_t size, void* value);


/*********************/
/* Package Variables */
/*********************/

/* Object copy property list class library initialization object */
const H5P_libclass_t H5P_CLS_OCPY[1] = {{
    "object copy",		/* Class name for debugging     */
    H5P_TYPE_OBJECT_COPY,       /* Class type                   */
    &H5P_CLS_ROOT_g,		/* Parent class ID              */
    &H5P_CLS_OBJECT_COPY_g,	/* Pointer to class ID          */
    &H5P_LST_OBJECT_COPY_g,	/* Pointer to default property list ID */
    H5P__ocpy_reg_prop,		/* Default property registration routine */
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

/* Declare a free list to manage the H5O_copy_dtype_merge_list_t struct */
H5FL_DEFINE(H5O_copy_dtype_merge_list_t);



/*-------------------------------------------------------------------------
 * Function:    H5P__ocpy_reg_prop
 *
 * Purpose:     Initialize the object copy property list class
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              October 31, 2006
 *-------------------------------------------------------------------------
 */
static herr_t
H5P__ocpy_reg_prop(H5P_genclass_t *pclass)
{
    unsigned ocpy_option = H5O_CPY_OPTION_DEF;  /* Default object copy flags */
    H5O_copy_dtype_merge_list_t *merge_comm_dtype_list = H5O_CPY_MERGE_COMM_DT_LIST_DEF; /* Default merge committed dtype list */
    H5O_mcdt_cb_info_t mcdt_cb = H5O_CPY_MCDT_SEARCH_CB_DEF; /* Default callback before searching the global list of committed datatypes at destination */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

    /* Register copy options property */
    if(H5P_register_real(pclass, H5O_CPY_OPTION_NAME, H5O_CPY_OPTION_SIZE, &ocpy_option, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register merge named dtype list property */
    if(H5P_register_real(pclass, H5O_CPY_MERGE_COMM_DT_LIST_NAME, H5O_CPY_MERGE_COMM_DT_LIST_SIZE, &merge_comm_dtype_list, NULL, NULL, NULL, NULL, H5O_CPY_MERGE_COMM_DT_LIST_COPY, H5O_CPY_MERGE_COMM_DT_LIST_CMP, H5O_CPY_MERGE_COMM_DT_LIST_CLOSE) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register property for callback when completing the search for a matching named datatype from the named dtype list */
    if(H5P_register_real(pclass, H5O_CPY_MCDT_SEARCH_CB_NAME, H5O_CPY_MCDT_SEARCH_CB_SIZE, &mcdt_cb, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P__ocpy_reg_prop() */


/*-------------------------------------------------------------------------
 * Function:    H5P__free_merge_comm_dtype_list
 *
 * Purpose:     Frees the provided merge named dtype list
 *
 * Return:      NULL
 *
 * Programmer:  Neil Fortner
 *              October 27, 2011
 *-------------------------------------------------------------------------
 */
static H5O_copy_dtype_merge_list_t *
H5P__free_merge_comm_dtype_list(H5O_copy_dtype_merge_list_t *dt_list)
{
    H5O_copy_dtype_merge_list_t *tmp_node;

    FUNC_ENTER_STATIC_NOERR

    /* Free the list */
    while(dt_list) {
        tmp_node = dt_list->next;
        (void)H5MM_xfree(dt_list->path);
        (void)H5FL_FREE(H5O_copy_dtype_merge_list_t, dt_list);
        dt_list = tmp_node;
    } /* end while */

    FUNC_LEAVE_NOAPI(NULL);
} /* H5P__free_merge_comm_dtype_list */


/*--------------------------------------------------------------------------
 * Function:	H5P__ocpy_merge_comm_dt_list_copy
 *
 * Purpose:	Copy the merge committed datatype list
 *
 * Return:	Success:	Non-negative
 * 		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		Friday, August 31, 2012
 *
 *--------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5P__ocpy_merge_comm_dt_list_copy(const char UNUSED *name, size_t UNUSED size,
    void *value)
{
    const H5O_copy_dtype_merge_list_t *src_dt_list;     /* Source merge named datatype lists */
    H5O_copy_dtype_merge_list_t *dst_dt_list = NULL;    /* Destination merge named datatype lists */
    H5O_copy_dtype_merge_list_t *dst_dt_list_tail = NULL, *tmp_dt_list = NULL; /* temporary merge named datatype lists */
    herr_t         ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    HDassert(value);

    /* Make copy of merge committed dtype list */
    src_dt_list = *(const H5O_copy_dtype_merge_list_t **)value;
    while(src_dt_list) {
        /* Copy src_dt_list */
        if(NULL == (tmp_dt_list = H5FL_CALLOC(H5O_copy_dtype_merge_list_t)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
        if(NULL == (tmp_dt_list->path = H5MM_strdup(src_dt_list->path)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

        /* Add copied node to dest dtype list */
        if(dst_dt_list_tail) {
            dst_dt_list_tail->next = tmp_dt_list;
            dst_dt_list_tail = tmp_dt_list;
        } /* end if */
        else {
            dst_dt_list = tmp_dt_list;
            dst_dt_list_tail = tmp_dt_list;
        } /* end else */
        tmp_dt_list = NULL;

        /* Advance src_dt_list pointer */
        src_dt_list = src_dt_list->next;
    } /* end while */

    /* Set the merge named dtype list property for the destination property list */
    *(H5O_copy_dtype_merge_list_t **)value = dst_dt_list;

done:
    if(ret_value < 0) {
        dst_dt_list = H5P__free_merge_comm_dtype_list(dst_dt_list);
        if(tmp_dt_list) {
            tmp_dt_list->path = (char *)H5MM_xfree(tmp_dt_list->path);
            tmp_dt_list = H5FL_FREE(H5O_copy_dtype_merge_list_t, tmp_dt_list);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P__ocpy_merge_comm_dt_list_copy() */


/*-------------------------------------------------------------------------
 * Function:       H5P__ocpy_merge_comm_dt_list_cmp
 *
 * Purpose:        Callback routine which is called whenever the merge
 *                 named dtype property in the object copy property list
 *                 is compared.
 *
 * Return:         positive if VALUE1 is greater than VALUE2, negative if
 *                      VALUE2 is greater than VALUE1 and zero if VALUE1 and
 *                      VALUE2 are equal.
 *
 * Programmer:     Neil Fortner
 *                 Friday, October 28, 2011
 *
 *-------------------------------------------------------------------------
 */
static int
H5P__ocpy_merge_comm_dt_list_cmp(const void *_dt_list1, const void *_dt_list2,
    size_t UNUSED size)
{
    const H5O_copy_dtype_merge_list_t *dt_list1 = *(H5O_copy_dtype_merge_list_t * const *)_dt_list1,     /* Create local aliases for values */
        *dt_list2 = *(H5O_copy_dtype_merge_list_t * const *)_dt_list2;
    herr_t ret_value = 0;       /* Return value */

    FUNC_ENTER_STATIC_NOERR

    /* Sanity check */
    HDassert(_dt_list1);
    HDassert(_dt_list2);
    HDassert(size == sizeof(H5O_copy_dtype_merge_list_t *));

    /* Walk through the lists, comparing each path.  For the lists to be the
     * same, the paths must be in the same order. */
    while(dt_list1 && dt_list2) {
        /* Compare paths */
        ret_value = HDstrcmp(dt_list1->path, dt_list2->path);
        if(ret_value != 0) HGOTO_DONE(ret_value)

        /* Advance to next node */
        dt_list1 = dt_list1->next;
        dt_list2 = dt_list2->next;
    } /* end while */

    /* Check if one list is longer than the other */
    if(dt_list1) HGOTO_DONE(1)
    if(dt_list2) HGOTO_DONE(-1)

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P__ocpy_merge_comm_dt_list_cmp() */


/*--------------------------------------------------------------------------
 * Function:	H5P__ocpy_merge_comm_dt_list_close
 *
 * Purpose:	Close the merge common datatype list property
 *
 * Return:	Success:	Non-negative
 * 		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		Friday, August 31, 2012
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5P__ocpy_merge_comm_dt_list_close(const char UNUSED *name, size_t UNUSED size, void *value)
{
    FUNC_ENTER_STATIC_NOERR

    HDassert(value);

    /* Free the merge named dtype list */
    H5P__free_merge_comm_dtype_list(*(H5O_copy_dtype_merge_list_t **)value);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5P__ocpy_merge_comm_dt_list_close() */


/*-------------------------------------------------------------------------
 * Function:    H5Pset_copy_object
 *
 * Purpose:     Set properties when copying an object (group, dataset, and datatype)
 *              from one location to another
 *
 * Usage:       H5Pset_copy_group(plist_id, cpy_option)
 *              hid_t plist_id;			IN: Property list to copy object
 *              unsigned cpy_option; 		IN: Options to copy object such as
 *                  H5O_COPY_SHALLOW_HIERARCHY_FLAG    -- Copy only immediate members
 *                  H5O_COPY_EXPAND_SOFT_LINK_FLAG     -- Expand soft links into new objects/
 *                  H5O_COPY_EXPAND_EXT_LINK_FLAG      -- Expand external links into new objects
 *                  H5O_COPY_EXPAND_REFERENCE_FLAG -- Copy objects that are pointed by references
 *                  H5O_COPY_WITHOUT_ATTR_FLAG         -- Copy object without copying attributes
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Peter Cao
 *              March 13, 2006
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_copy_object(hid_t plist_id, unsigned cpy_option)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "iIu", plist_id, cpy_option);

    /* Check parameters */
    if(cpy_option & ~H5O_COPY_ALL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unknown option specified")

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_OBJECT_COPY)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Set value */
    if(H5P_set(plist, H5O_CPY_OPTION_NAME, &cpy_option) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set copy object flag")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_copy_object() */


/*-------------------------------------------------------------------------
 * Function:    H5Pget_copy_object
 *
 * Purpose:     Returns the cpy_option, which is set for H5Ocopy(hid_t loc_id,
 *              const char* name, ... ) for copying objects
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Peter Cao
 *              March 13, 2006
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_copy_object(hid_t plist_id, unsigned *cpy_option /*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value = SUCCEED; /* return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "ix", plist_id, cpy_option);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_OBJECT_COPY)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Get values */
    if(cpy_option)
        if(H5P_get(plist, H5O_CPY_OPTION_NAME, cpy_option) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get object copy flag")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_copy_object() */


/*-------------------------------------------------------------------------
 * Function:    H5Padd_merge_committed_dtype_path
 *
 * Purpose:     Adds path to the list of paths to search first in the
 *              target file when merging committed datatypes during H5Ocopy
 *              (i.e. when using the H5O_COPY_MERGE_COMMITTED_DTYPE_FLAG flag
 *              as set by H5Pset_copy_object).  If the source named
 *              dataype is not found in the list of paths created by this
 *              function, the entire file will be searched.
 *
 * Usage:       H5Padd_merge_committed_dtype_path(plist_id, path)
 *              hid_t plist_id;                 IN: Property list to copy object
 *              const char *path;               IN: Path to add to list
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Neil Fortner
 *              October 27, 2011
 *-------------------------------------------------------------------------
 */
herr_t
H5Padd_merge_committed_dtype_path(hid_t plist_id, const char *path)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    H5O_copy_dtype_merge_list_t *old_list; 	/* Merge committed dtype list currently present */
    H5O_copy_dtype_merge_list_t *new_obj = NULL; /* New object to add to list */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*s", plist_id, path);

    /* Check parameters */
    if(!path)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no path specified")

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_OBJECT_COPY)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Get dtype list */
    if(H5P_get(plist, H5O_CPY_MERGE_COMM_DT_LIST_NAME, &old_list) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get merge named dtype list")

    /* Add the new path to the list */
    if(NULL == (new_obj = H5FL_CALLOC(H5O_copy_dtype_merge_list_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
    if(NULL == (new_obj->path = H5MM_strdup(path)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
    new_obj->next = old_list;

    /* Update the list stored in the property list */
    if(H5P_set(plist, H5O_CPY_MERGE_COMM_DT_LIST_NAME, &new_obj) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set merge named dtype list")

done:
    if(ret_value < 0)
        if(new_obj) {
            new_obj->path = (char *)H5MM_xfree(new_obj->path);
            new_obj = H5FL_FREE(H5O_copy_dtype_merge_list_t, new_obj);
        } /* end if */

    FUNC_LEAVE_API(ret_value)
} /* end H5Padd_merge_committed_dtype_path() */


/*-------------------------------------------------------------------------
 * Function:    H5Pfree_merge_committed_dtype_paths
 *
 * Purpose:     Frees and clears the list of paths created by
 *              H5Padd_merge_committed_dtype_path.  A new list may then be
 *              created by calling H5Padd_merge_committed_dtype_path again.
 *
 * Usage:       H5Pfree_merge_committed_dtype_paths(plist_id)
 *              hid_t plist_id;                 IN: Property list to copy object
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Neil Fortner
 *              October 27, 2011
 *-------------------------------------------------------------------------
 */
herr_t
H5Pfree_merge_committed_dtype_paths(hid_t plist_id)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    H5O_copy_dtype_merge_list_t *dt_list; /* Merge committed dtype list currently present */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", plist_id);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_OBJECT_COPY)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Get dtype list */
    if(H5P_get(plist, H5O_CPY_MERGE_COMM_DT_LIST_NAME, &dt_list) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get merge committed dtype list")

    /* Free dtype list */
    dt_list = H5P__free_merge_comm_dtype_list(dt_list);

    /* Update the list stored in the property list (to NULL) */
    if(H5P_set(plist, H5O_CPY_MERGE_COMM_DT_LIST_NAME, &dt_list) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set merge committed dtype list")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pfree_merge_committed_dtype_paths() */


/*-------------------------------------------------------------------------
 * Function:    H5Pset_mcdt_search_cb
 *
 * Purpose:     Set the callback function when a matching committed datatype is not found
 *		from the list of paths stored in the object copy property list.
 * 		H5Ocopy will invoke this callback before searching all committed datatypes
 *		at destination.
 *
 * Usage:       H5Pset_mcdt_search_cb(plist_id, H5O_mcdt_search_cb_t func, void *op_data)
 *              hid_t plist_id;                 IN: Property list to copy object
 *              H5O_mcdt_search_cb_t func;      IN: The callback function
 *              void *op_data;      		IN: The user data
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Vailin Choi; November 28, 2011
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_mcdt_search_cb(hid_t plist_id, H5O_mcdt_search_cb_t func, void *op_data)
{
    H5P_genplist_t *plist;      	/* Property list pointer */
    H5O_mcdt_cb_info_t cb_info;		/* Callback info struct */
    herr_t ret_value = SUCCEED; 	/* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "ix*x", plist_id, func, op_data);

    /* Check if the callback function is NULL and the user data is non-NULL.
     * This is almost certainly an error as the user data will not be used. */
    if(!func && op_data)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "callback is NULL while user data is not")

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_OBJECT_COPY)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

     /* Populate the callback info struct */
    cb_info.func = func;
    cb_info.user_data = op_data;

    /* Set callback info */
    if(H5P_set(plist, H5O_CPY_MCDT_SEARCH_CB_NAME, &cb_info) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set callback info")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_mcdt_search_cb() */


/*-------------------------------------------------------------------------
 * Function:    H5Pget_mcdt_search_cb
 *
 * Purpose:     Retrieves the callback function and user data from the specified 
 *		object copy property list.
 *
 * Usage:       H5Pget_mcdt_search_cb(plist_id, H5O_mcdt_search_cb_t *func, void **op_data)
 *              hid_t plist_id;                 IN: Property list to copy object
 *		H5O_mcdt_search_cb_t *func;	OUT: The callback function
 *		void **op_data;			OUT: The user data
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Vailin Choi; November 29, 2011
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_mcdt_search_cb(hid_t plist_id, H5O_mcdt_search_cb_t *func, void **op_data)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    H5O_mcdt_cb_info_t cb_info;	/* Callback info struct */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "i*x**x", plist_id, func, op_data);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_OBJECT_COPY)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Get callback info */
    if(H5P_get(plist, H5O_CPY_MCDT_SEARCH_CB_NAME, &cb_info) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get callback info")
    
    if(func)
	*func = cb_info.func;

    if(op_data)
	*op_data = cb_info.user_data;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_mcdt_search_cb() */

