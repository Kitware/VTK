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
 * FILE:	H5I.c - Internal storage routines for handling "IDs"
 *
 * REMARKS:	ID's which allow objects (void *'s currently) to be bundled
 *		into "types" for more general storage.
 *
 * DESIGN:	The types are stored in an array of pointers to store each
 *		type in an element. Each "type" node contains a link to a
 *		hash table to manage the IDs in each type.  Allowed types are
 *		values within the range 1 to H5I_MAX_NUM_TYPES and are given out
 *		at run-time.  Types used by the library are stored in global
 *		variables defined in H5Ipublic.h.
 *
 * AUTHOR:	Quincey Koziol
 *
 * MODIFICATIONS:
 *	1/3/96	- Starting writing specs & coding prototype
 *	1/7/96	- Finished coding prototype
 *	6/10/97 - Moved into HDF5 library
 *	5/18/04 - Expanded to allow registration of new types at run-time
 */

#define H5I_PACKAGE		/*suppress error about including H5Ipkg	  */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5I_init_interface


#include "H5private.h"		/* Generic Functions			*/
#include "H5ACprivate.h"        /* Metadata cache                       */
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free Lists                           */
#include "H5Ipkg.h"		/* IDs			  		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Oprivate.h"		/* Object headers		  	*/
#include "H5SLprivate.h"	/* Skip Lists                           */

/* Define this to compile in support for dumping ID information */
/* #define H5I_DEBUG_OUTPUT */
#ifndef H5I_DEBUG_OUTPUT
#include "H5Gprivate.h"		/* Groups				*/
#else /* H5I_DEBUG_OUTPUT */
#define H5G_PACKAGE /*suppress error message about including H5Gpkg.h */
#include "H5Gpkg.h"		/* Groups		  		*/
#include "H5Dprivate.h"		/* Datasets				*/
#include "H5Tprivate.h"		/* Datatypes				*/
#endif /* H5I_DEBUG_OUTPUT */

/* Local Macros */

/* Define the maximum number of returned ID structures to keep around
   for re-use. */
#define MAX_FREE_ID_STRUCTS 1000

/* Combine a Type number and an atom index into an atom */
#define H5I_MAKE(g,i)	((((hid_t)(g)&TYPE_MASK)<<ID_BITS)|	  \
			     ((hid_t)(i)&ID_MASK))

/* Local typedefs */

/* Atom information structure used */
typedef struct H5I_id_info_t {
    hid_t	id;		/* ID for this info			    */
    unsigned	count;		/* ref. count for this atom		    */
    unsigned    app_count;      /* ref. count of application visible atoms  */
    const void	*obj_ptr;	/* pointer associated with the atom	    */
} H5I_id_info_t;

/* ID type structure used */
typedef struct {
    const H5I_class_t *cls;     /* Pointer to ID class                      */
    unsigned	init_count;	/* # of times this type has been initialized*/
    hbool_t	wrapped;	/* Whether the id count has wrapped around  */
    unsigned	id_count;	/* Current number of IDs held		    */
    hid_t	nextid;		/* ID to use for the next atom		    */
    H5SL_t      *ids;           /* Pointer to skip list that stores IDs     */

    /* Fields for holding available IDs */
    unsigned    avail_count;    /* # of available ID structures awaiting recycling */
    H5SL_t      *avail_ids;     /* pointer to skip list of available IDs */
} H5I_id_type_t;

typedef struct {
    H5I_search_func_t app_cb;   /* Application's callback routine */
    void *app_key;              /* Application's "key" (user data) */
    void *ret_obj;              /* Object to return */
} H5I_search_ud_t;

/* User data for iterator callback when IDs have wrapped */
typedef struct {
    hid_t nextid;               /* Next ID to expect */
} H5I_wrap_ud_t;

/* User data for iterator callback for ID iteration */
typedef struct {
    H5I_search_func_t user_func;        /* 'User' function to invoke */
    void *user_udata;                   /* User data to pass to 'user' function */
    hbool_t app_ref;                    /* Whether this is an appl. ref. call */
} H5I_iterate_ud_t;

/*-------------------- Locally scoped variables -----------------------------*/

/* Array of pointers to atomic types */
static H5I_id_type_t *H5I_id_type_list_g[H5I_MAX_NUM_TYPES];

/* Variable to keep track of the number of types allocated.  Its value is the */
/* next type ID to be handed out, so it is always one greater than the number */
/* of types. */
/* Starts at 1 instead of 0 because it makes trace output look nicer.  If more */
/* types (or IDs within a type) are needed, adjust TYPE_BITS in H5Ipkg.h       */
/* and/or increase size of hid_t */
static H5I_type_t H5I_next_type = (H5I_type_t) H5I_NTYPES;

/* Declare a free list to manage the H5I_id_info_t struct */
H5FL_DEFINE_STATIC(H5I_id_info_t);

/* Declare a free list to manage the H5I_id_type_t struct */
H5FL_DEFINE_STATIC(H5I_id_type_t);

/* Declare a free list to manage the H5I_class_t struct */
H5FL_DEFINE_STATIC(H5I_class_t);

/*--------------------- Local function prototypes ---------------------------*/
static herr_t H5I__free_cb(void *_item, void *_key, void *_udata);
static int H5I__destroy_type(H5I_type_t type);
static void *H5I__remove_verify(hid_t id, H5I_type_t id_type);
static void *H5I__remove_common(H5I_id_type_t *type_ptr, hid_t id);
static int H5I__inc_type_ref(H5I_type_t type);
static int H5I__get_type_ref(H5I_type_t type);
static H5I_id_info_t *H5I__find_id(hid_t id);
#ifdef H5I_DEBUG_OUTPUT
static herr_t H5I__debug(H5I_type_t type);
#endif /* H5I_DEBUG_OUTPUT */


/*--------------------------------------------------------------------------
NAME
   H5I_init_interface -- Initialize interface-specific information
USAGE
    herr_t H5I_init_interface()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.

--------------------------------------------------------------------------*/
static herr_t
H5I_init_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5I_init_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5I_term_interface
 *
 * Purpose:	Terminate the H5I interface: release all memory, reset all
 *		global variables to initial values. This only happens if all
 *		types have been destroyed from other interfaces.
 *
 * Return:	Success:	Positive if any action was taken that might
 *				affect some other interface; zero otherwise.
 *
 * 		Failure:	Negative.
 *
 * Programmer:	Unknown
 *
 *-------------------------------------------------------------------------
 */
int
H5I_term_interface(void)
{
    H5I_id_type_t	*type_ptr;
    H5I_type_t		type;
    int		n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(H5_interface_initialize_g) {
        /* How many types are still being used? */
        for(type = (H5I_type_t)0; type < H5I_next_type; H5_INC_ENUM(H5I_type_t, type)) {
            if((type_ptr = H5I_id_type_list_g[type]) && type_ptr->ids)
                n++;
        } /* end for */

        /* If no types are used then clean up */
        if(0 == n) {
            for(type = (H5I_type_t)0; type < H5I_next_type; H5_INC_ENUM(H5I_type_t,type)) {
                type_ptr = H5I_id_type_list_g[type];
                if(type_ptr) {
                    HDassert(NULL == type_ptr->ids);
                    type_ptr = H5FL_FREE(H5I_id_type_t, type_ptr);
                    H5I_id_type_list_g[type] = NULL;
                } /* end if */
            } /* end for */
        } /* end if */

        /* Mark interface closed */
        H5_interface_initialize_g = 0;
    } /* end if */

    FUNC_LEAVE_NOAPI(n)
} /* end H5I_term_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5Iregister_type
 *
 * Purpose:	Public interface to H5I_register_type.  Creates a new type
 *		of ID's to give out.  A specific number (RESERVED) of type
 *		entries may be reserved to enable "constant" values to be handed
 *		out which are valid IDs in the type, but which do not map to any
 *		data structures and are not allocated dynamically later. HASH_SIZE is
 *		the minimum hash table size to use for the type. FREE_FUNC is
 *		called with an object pointer when the object is removed from
 *		the type.
 *
 * Return:	Success:	Type ID of the new type
 *		Failure:	H5I_BADID
 *
 * Programmers:	Nathaniel Furrer
 *		James Laird
 *		Friday, April 30, 2004
 *
 *-------------------------------------------------------------------------
 */
H5I_type_t
H5Iregister_type(size_t hash_size, unsigned reserved, H5I_free_t free_func)
{
    H5I_class_t *cls = NULL;    /* New ID class */
    H5I_type_t new_type;        /* New ID type value */
    H5I_type_t ret_value;       /* Return value */

    FUNC_ENTER_API(H5I_BADID)
    H5TRACE3("It", "zIux", hash_size, reserved, free_func);

    /* Generate a new H5I_type_t value */

    /* Increment the number of types*/
    if(H5I_next_type < H5I_MAX_NUM_TYPES) {
        new_type = H5I_next_type;
        H5_INC_ENUM(H5I_type_t, H5I_next_type);
    } /* end if */
    else {
        hbool_t done;       /* Indicate that search was successful */
        int i;              /* Local index variable */

        /* Look for a free type to give out */
        done = FALSE;
        for(i = H5I_NTYPES; i < H5I_MAX_NUM_TYPES && done == FALSE; i++) {
            if(NULL == H5I_id_type_list_g[i]) {
                /* Found a free type ID */
                new_type = (H5I_type_t)i;
                done = TRUE;
            } /* end if */
        } /* end for */

        /* Verify that we found a type to give out */
        if(done == FALSE)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, H5I_BADID, "Maximum number of ID types exceeded.")
    } /* end else */

    /* Allocate new ID class */
    if(NULL == (cls = H5FL_MALLOC(H5I_class_t)))
        HGOTO_ERROR(H5E_ATOM, H5E_CANTALLOC, H5I_BADID, "ID class allocation failed")

    /* Initialize class fields */
    cls->type_id = new_type;
    cls->flags = H5I_CLASS_IS_APPLICATION;
    cls->reserved = reserved;
    cls->free_func = free_func;

    /* Register the new ID class */
    if(H5I_register_type(cls) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTINIT, H5I_BADID, "can't initialize ID class")

    /* Set return value */
    ret_value = new_type;

done:
    /* Clean up on error */
    if(ret_value < 0) {
        if(cls)
            cls = H5FL_FREE(H5I_class_t, cls);
    } /* end if */

    FUNC_LEAVE_API(ret_value)
} /* end H5Iregister_type() */


/*-------------------------------------------------------------------------
 * Function:	H5I_register_type
 *
 * Purpose:	Creates a new type of ID's to give out.
 *		The class is initialized or its reference count is incremented
 *              (if it is already initialized).
 *
 * Return:	Success:	Type ID of the new type
 *		Failure:	H5I_BADID
 *
 * Programmers:	Nathaniel Furrer
 *		James Laird
 *		Friday, April 30, 2004
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5I_register_type(const H5I_class_t *cls)
{
    H5I_id_type_t *type_ptr = NULL;	/* Ptr to the atomic type*/
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(cls);
    HDassert(cls->type_id > 0 && cls->type_id < H5I_MAX_NUM_TYPES);

    /* Initialize the type */
    if(NULL == H5I_id_type_list_g[cls->type_id]) {
        /* Allocate the type information for new type */
        if(NULL == (type_ptr = (H5I_id_type_t *)H5FL_CALLOC(H5I_id_type_t)))
            HGOTO_ERROR(H5E_ATOM, H5E_CANTALLOC, FAIL, "ID type allocation failed")
        H5I_id_type_list_g[cls->type_id] = type_ptr;
    } /* end if */
    else {
        /* Get the pointer to the existing type */
        type_ptr = H5I_id_type_list_g[cls->type_id];
    } /* end else */

    /* Initialize the ID type structure for new types */
    if(type_ptr->init_count == 0) {
        type_ptr->cls = cls;
        type_ptr->wrapped = FALSE;
        type_ptr->id_count = 0;
        type_ptr->nextid = (hid_t)cls->reserved;
        if(NULL == (type_ptr->ids = H5SL_create(H5SL_TYPE_HID, NULL)))
            HGOTO_ERROR(H5E_ATOM, H5E_CANTCREATE, FAIL, "skip list creation failed")
        type_ptr->avail_count = 0;
        if(NULL == (type_ptr->avail_ids = H5SL_create(H5SL_TYPE_HID, NULL)))
            HGOTO_ERROR(H5E_ATOM, H5E_CANTCREATE, FAIL, "skip list creation failed")
    } /* end if */

    /* Increment the count of the times this type has been initialized */
    type_ptr->init_count++;

done:
    if(ret_value < 0) {	/* Clean up on error */
        if(type_ptr) {
            if(type_ptr->ids)
                H5SL_close(type_ptr->ids);
            if(type_ptr->avail_ids)
                H5SL_close(type_ptr->avail_ids);
            (void)H5FL_FREE(H5I_id_type_t, type_ptr);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I_register_type() */


/*-------------------------------------------------------------------------
 * Function:	H5Itype_exists
 *
 * Purpose:     Query function to inform the user if a given type is
 *              currently registered with the library.
 *
 * Return:	Success:        1 if the type is registered, 0 if it is not
 *		Failure:	Negative
 *
 * Programmer:	James Laird
 *		Nathaniel Furrer
 *              Tuesday, June 29, 2004
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Itype_exists(H5I_type_t type)
{
    htri_t ret_value = TRUE;                      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("t", "It", type);

    if(type <= H5I_BADID || type >= H5I_next_type)
	HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "invalid type number")

    if(NULL == H5I_id_type_list_g[type])
        ret_value = FALSE;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Itype_exists() */


/*-------------------------------------------------------------------------
 * Function:	H5Inmembers
 *
 * Purpose:	Returns the number of members in a type.  Public interface to
 *		H5I_nmembers.  The public interface throws an error if the
 *              supplied type does not exist.  This is different than the
 *              private interface, which will just return 0.
 *
 * Return:	Success:	Zero
 *		Failure:	Negative
 *
 * Programmer:	James Laird
 *		Nathaniel Furrer
 *              Friday, April 23, 2004
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Inmembers(H5I_type_t type, hsize_t *num_members)
{
    int ret_value = SUCCEED;                      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "It*h", type, num_members);

    if(H5I_IS_LIB_TYPE(type))
        HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, FAIL, "cannot call public function on library type")

    /* Validate parameters.  This needs to be done here, instead of letting
     * the private interface handle it, because the public interface throws
     * an error when the supplied type does not exist.
     */
    if(type <= H5I_BADID || type >= H5I_next_type)
	HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "invalid type number")
    if(NULL == H5I_id_type_list_g[type])
	HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "supplied type does not exist")

    if(num_members) {
        int members;

        if((members = H5I_nmembers(type)) < 0)
            HGOTO_ERROR(H5E_ATOM, H5E_CANTCOUNT, FAIL, "can't compute number of members")

        *num_members = (hsize_t)members;
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Inmembers() */


/*-------------------------------------------------------------------------
 * Function:	H5I_nmembers
 *
 * Purpose:	Returns the number of members in a type.
 *
 * Return:	Success:	Number of members; zero if the type is empty
 *				or has been deleted.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Wednesday, March 24, 1999
 *
 *-------------------------------------------------------------------------
 */
int
H5I_nmembers(H5I_type_t type)
{
    H5I_id_type_t	*type_ptr = NULL;
    int		ret_value;

    FUNC_ENTER_NOAPI(FAIL)

    if(type <= H5I_BADID || type >= H5I_next_type)
	HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "invalid type number")
    if(NULL == (type_ptr = H5I_id_type_list_g[type]) || type_ptr->init_count <= 0)
	HGOTO_DONE(0);

    /* Set return value */
    H5_ASSIGN_OVERFLOW(ret_value, type_ptr->id_count, unsigned, int);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I_nmembers() */


/*-------------------------------------------------------------------------
 * Function:	H5Iclear_type
 *
 * Purpose:	Removes all objects from the type, calling the free
 *		function for each object regardless of the reference count.
 *		Public interface to H5I_clear_type.
 *
 * Return:	Success:	Non-negative
 *		Failure:	negative
 *
 * Programmer:	James Laird
 *		Nathaniel Furrer
 *              Friday, April 23, 2004
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Iclear_type(H5I_type_t type, hbool_t force)
{
    herr_t ret_value;                      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "Itb", type, force);

    if(H5I_IS_LIB_TYPE(type))
        HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, FAIL, "cannot call public function on library type")

    ret_value = H5I_clear_type(type, force, TRUE);

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Iclear_type() */


/*-------------------------------------------------------------------------
 * Function:	H5I__free_cb
 *
 * Purpose:	Callback for freeing ID nodes in H5I__remove_common
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Friday, October 4, 2013
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5I__free_cb(void *_item, void UNUSED *_key, void UNUSED *_udata)
{
    H5I_id_info_t *item = (H5I_id_info_t *)_item;       /* Pointer to the ID node */

    FUNC_ENTER_STATIC_NOERR

    /* Sanity check */
    HDassert(item);

    item = H5FL_FREE(H5I_id_info_t, item);

    FUNC_LEAVE_NOAPI(H5_ITER_CONT)
} /* end H5I__free_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5I_clear_type
 *
 * Purpose:	Removes all objects from the type, calling the free
 *		function for each object regardless of the reference count.
 *
 * Return:	Success:	Non-negative
 *		Failure:	negative
 *
 * Programmer:	Robb Matzke
 *              Wednesday, March 24, 1999
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5I_clear_type(H5I_type_t type, hbool_t force, hbool_t app_ref)
{
    H5I_id_type_t *type_ptr;	        /* ptr to the atomic type */
    H5SL_node_t *curr_node;             /* Current skip list node ptr */
    H5SL_node_t *next_node;             /* Next skip list node ptr */
    int		ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if(type <= H5I_BADID || type >= H5I_next_type)
	HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "invalid type number")

    type_ptr = H5I_id_type_list_g[type];
    if(type_ptr == NULL || type_ptr->init_count <= 0)
	HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, FAIL, "invalid type")

    /*
     * Call free method for all objects in type regardless of their reference
     * counts. Ignore the return value from from the free method and remove
     * object from type regardless if FORCE is non-zero.
     */
    for(curr_node = H5SL_first(type_ptr->ids); curr_node; curr_node = next_node) {
        H5I_id_info_t *cur;         /* Current ID being worked with */
        hbool_t    delete_node;     /* Flag to indicate node should be removed from linked list */

        /* Get ID for this node */
        if(NULL == (cur = (H5I_id_info_t *)H5SL_item(curr_node)))
            HGOTO_ERROR(H5E_ATOM, H5E_CANTGET, FAIL, "can't get ID info for node")

        /*
         * Do nothing to the object if the reference count is larger than
         * one and forcing is off.
         */
        if(!force && (cur->count - (!app_ref * cur->app_count)) > 1)
            delete_node = FALSE;
        else {
            /* Check for a 'free' function and call it, if it exists */
            /* (Casting away const OK -QAK) */
            if(type_ptr->cls->free_func && (type_ptr->cls->free_func)((void *)cur->obj_ptr) < 0) {
                if(force) {
#ifdef H5I_DEBUG
                    if(H5DEBUG(I)) {
                        fprintf(H5DEBUG(I), "H5I: free type=%d obj=0x%08lx "
                            "failure ignored\n", (int)type,
                            (unsigned long)(cur->obj_ptr));
                    } /* end if */
#endif /*H5I_DEBUG*/

                    /* Indicate node should be removed from list */
                    delete_node = TRUE;
                } /* end if */
                else {
                    /* Indicate node should _NOT_ be removed from list */
                    delete_node = FALSE;
                } /* end else */
            } /* end if */
            else {
                /* Indicate node should be removed from list */
                delete_node = TRUE;
            } /* end else */
        } /* end else */

        /* Get the next node in the list */
        next_node = H5SL_next(curr_node);

        /* Check if we should delete this node or not */
        if(delete_node) {
            /* Decrement the number of IDs in the type */
            (type_ptr->id_count)--;

            /* Remove the node from the list */
            if(NULL == H5SL_remove(type_ptr->ids, &cur->id))
                HGOTO_ERROR(H5E_ATOM, H5E_CANTDELETE, FAIL, "can't remove ID node from skip list")

            /* Free the node */
            cur = H5FL_FREE(H5I_id_info_t, cur);
        } /* end if */
    } /* end for */

    /* Also free any ID structures being retained for potential re-use */
    if(type_ptr->avail_count > 0) {
        if(H5SL_free(type_ptr->avail_ids, H5I__free_cb, NULL) < 0)
            HGOTO_ERROR(H5E_ATOM, H5E_CANTREMOVE, FAIL, "can't release available ID nodes")
        type_ptr->avail_count = 0;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I_clear_type() */


/*-------------------------------------------------------------------------
 * Function:	H5Idestroy_type
 *
 * Purpose:	Destroys a type along with all atoms in that type
 *		regardless of their reference counts. Destroying IDs
 *		involves calling the free-func for each ID's object and
 *		then adding the ID struct to the ID free list.  Public
 *		interface to H5I__destroy_type.
 *
 * Return:	Zero on success/Negative on failure
 *
 * Programmer:	Nathaniel Furrer
 *		James Laird
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Idestroy_type(H5I_type_t type)
{
    herr_t ret_value;           /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "It", type);

    if(H5I_IS_LIB_TYPE(type))
        HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, FAIL, "cannot call public function on library type")

    ret_value = H5I__destroy_type(type);

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Idestroy_type() */


/*-------------------------------------------------------------------------
 * Function:	H5I__destroy_type
 *
 * Purpose:	Destroys a type along with all atoms in that type
 *		regardless of their reference counts. Destroying IDs
 *		involves calling the free-func for each ID's object and
 *		then adding the ID struct to the ID free list.
 *
 * Return:	Zero on success/Negative on failure
 *
 * Programmer:	Nathaniel Furrer
 *		James Laird
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5I__destroy_type(H5I_type_t type)
{
    H5I_id_type_t *type_ptr;	/* ptr to the atomic type */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    if(type <= H5I_BADID || type >= H5I_next_type)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "invalid type number")

    type_ptr = H5I_id_type_list_g[type];
    if(type_ptr == NULL || type_ptr->init_count <= 0)
        HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, FAIL, "invalid type")

    /* Close/clear/destroy all IDs for this type */
    H5E_BEGIN_TRY {
        H5I_clear_type(type, TRUE, FALSE);
    } H5E_END_TRY       /*don't care about errors*/

    /* Check if we should release the ID class */
    if(type_ptr->cls->flags & H5I_CLASS_IS_APPLICATION)
        type_ptr->cls = H5FL_FREE(H5I_class_t, (void *)type_ptr->cls);

    if(H5SL_close(type_ptr->avail_ids) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTCLOSEOBJ, FAIL, "can't close skip list")
    type_ptr->avail_ids = NULL;

    if(H5SL_close(type_ptr->ids) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTCLOSEOBJ, FAIL, "can't close skip list")
    type_ptr->ids = NULL;

    type_ptr = H5FL_FREE(H5I_id_type_t, type_ptr);
    H5I_id_type_list_g[type] = NULL;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I__destroy_type() */


/*-------------------------------------------------------------------------
 * Function:	H5Iregister
 *
 * Purpose:	Public interface to H5I_register.
 *
 * Return:	Success:	New object id.
 *		Failure:	Negative
 *
 * Programmer:	Nathaniel Furrer
 *		James Laird
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Iregister(H5I_type_t type, const void *object)
{
    hid_t ret_value;                      /* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE2("i", "It*x", type, object);

    if(H5I_IS_LIB_TYPE(type))
        HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, FAIL, "cannot call public function on library type")

    ret_value = H5I_register(type, object, TRUE);

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Iregister() */


/*-------------------------------------------------------------------------
 * Function:	H5I__wrapped_cb
 *
 * Purpose:	Callback for searching for next free ID, when IDs have wrapped
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Thursday, October 3, 2013
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5I__wrapped_cb(void *_item, void UNUSED *_key, void *_udata)
{
    H5I_id_info_t *item = (H5I_id_info_t *)_item;       /* Pointer to the ID node */
    H5I_wrap_ud_t *udata = (H5I_wrap_ud_t *)_udata;     /* Pointer to user data */
    int ret_value = H5_ITER_CONT;                       /* Return value */

    FUNC_ENTER_STATIC_NOERR

    /* Sanity check */
    HDassert(item);
    HDassert(udata);

    /* Break out if we see a free ID */
    if(udata->nextid != item->id) {
        /* Sanity check */
        HDassert(item->id > udata->nextid);

        ret_value = H5_ITER_STOP;
    } /* end if */
    else
        /* Increment to expect the next ID */
        udata->nextid++;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I__wrapped_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5I_register
 *
 * Purpose:	Registers an OBJECT in a TYPE and returns an ID for it.
 *		This routine does _not_ check for unique-ness of the objects,
 *		if you register an object twice, you will get two different
 *		IDs for it.  This routine does make certain that each ID in a
 *		type is unique.  IDs are created by getting a unique number
 *		for the type the ID is in and incorporating the type into
 *		the ID which is returned to the user.
 *
 * Return:	Success:	New object id.
 *		Failure:	Negative
 *
 * Programmer:	Unknown
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5I_register(H5I_type_t type, const void *object, hbool_t app_ref)
{
    H5I_id_type_t	*type_ptr;	/*ptr to the type		*/
    H5I_id_info_t	*id_ptr;	/*ptr to the new ID information */
    hid_t		ret_value = SUCCEED; /*return value		*/

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments */
    if(type <= H5I_BADID || type >= H5I_next_type)
	HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "invalid type number")
    type_ptr = H5I_id_type_list_g[type];
    if(NULL == type_ptr || type_ptr->init_count <= 0)
	HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, FAIL, "invalid type")

    /* If there is an available ID structure, use it. */
    if(type_ptr->avail_count > 0) {
        /* Use existing available ID struct */
        if(NULL == (id_ptr = (H5I_id_info_t *)H5SL_remove_first(type_ptr->avail_ids)))
            HGOTO_ERROR(H5E_ATOM, H5E_CANTREMOVE, FAIL, "can't remove ID from available ID list")
    
        /* Decrease count of available ID structures */
        type_ptr->avail_count--;
    } /* end if */
    /* If no available ID structure, then create a new id for use, and
     * allocate a new struct to house it. */
    else {
        /* Allocate new ID struct */
        if(NULL == (id_ptr = H5FL_MALLOC(H5I_id_info_t)))
            HGOTO_ERROR(H5E_ATOM, H5E_NOSPACE, FAIL, "memory allocation failed")

        /* Make a new ID */
        id_ptr->id = H5I_MAKE(type, type_ptr->nextid);

        /* Increment nextid value */
        type_ptr->nextid++;
    } /* end if */

    /* Fill in remaining fields of ID struct */
    id_ptr->count = 1; /*initial reference count*/
    id_ptr->app_count = !!app_ref;
    id_ptr->obj_ptr = object;

    /* Insert into the type */
    if(H5SL_insert(type_ptr->ids, id_ptr, &id_ptr->id) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTINSERT, FAIL, "can't insert ID node into skip list")
    type_ptr->id_count++;

    /*
     * This next section of code checks for the 'nextid' getting too large and
     * wrapping around, thus necessitating checking for duplicate IDs being
     * handed out.
     */
    if(type_ptr->nextid > (hid_t)ID_MASK)
	type_ptr->wrapped = TRUE;

    /*
     * If we've wrapped around then we need to check for duplicate id's being
     * handed out.
     */
    if(type_ptr->wrapped) {
        H5I_wrap_ud_t udata;    /* User data for iteration */
        herr_t iter_status;     /* Iteration status */

        /* Set up user data for iteration */
        udata.nextid = (hid_t)type_ptr->cls->reserved;

        /* Iterate over all the ID nodes, looking for a gap in the ID sequence */
        if((iter_status = H5SL_iterate(type_ptr->ids, H5I__wrapped_cb, &udata)) < 0)
            HGOTO_ERROR(H5E_ATOM, H5E_BADITER, FAIL, "ID iteration failed")

        /* If we didn't break out of the iteration and we're at the max. ID, we've used all the IDs */
        if(0 == iter_status && udata.nextid >= ID_MASK)
            HGOTO_ERROR(H5E_ATOM, H5E_NOIDS, FAIL, "no IDs available in type")

        /* Sanity check */
        HDassert(udata.nextid < ID_MASK);

        /* Retain the next ID for the class */
        type_ptr->nextid = udata.nextid;
    } /* end if */

    /* Set return value */
    ret_value = id_ptr->id;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I_register() */


/*-------------------------------------------------------------------------
 * Function:	H5I_subst
 *
 * Purpose:	Substitute a new object pointer for the specified ID.
 *
 * Return:	Success:	Non-null previous object pointer associated
 *				with the specified ID.
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *		Saturday, February 27, 2010
 *
 *-------------------------------------------------------------------------
 */
void *
H5I_subst(hid_t id, const void *new_object)
{
    H5I_id_info_t	*id_ptr;	/* Ptr to the atom	*/
    void		*ret_value;	/* Return value		*/

    FUNC_ENTER_NOAPI(NULL)

    /* General lookup of the ID */
    if(NULL == (id_ptr = H5I__find_id(id)))
        HGOTO_ERROR(H5E_ATOM, H5E_NOTFOUND, NULL, "can't get ID ref count")

    /* Get the old object pointer to return */
    /* (Casting away const OK -QAK) */
    ret_value = (void *)id_ptr->obj_ptr;

    /* Set the new object pointer for the ID */
    id_ptr->obj_ptr = new_object;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end if */


/*-------------------------------------------------------------------------
 * Function:	H5I_object
 *
 * Purpose:	Find an object pointer for the specified ID.
 *
 * Return:	Success:	Non-null object pointer associated with the
 *				specified ID.
 *		Failure:	NULL
 *
 * Programmer:	Unknown
 *
 *-------------------------------------------------------------------------
 */
void *
H5I_object(hid_t id)
{
    H5I_id_info_t	*id_ptr;		/*ptr to the new atom	*/
    void		*ret_value = NULL;	/*return value		*/

    FUNC_ENTER_NOAPI(NULL)

    /* General lookup of the ID */
    if(NULL != (id_ptr = H5I__find_id(id))) {
        /* Get the object pointer to return */
        /* (Casting away const OK -QAK) */
        ret_value = (void *)id_ptr->obj_ptr;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end if */


/*-------------------------------------------------------------------------
 * Function:	H5Iobject_verify
 *
 * Purpose:	Find an object pointer for the specified ID, verifying that
 *                  its in a particular type.  Public interface to
 *					H5I_object_verify.
 *
 * Return:	Success:	Non-null object pointer associated with the
 *				specified ID.
 *		Failure:	NULL
 *
 * Programmer:	Nathaniel Furrer
 *		James Laird
 *		Friday, April 23, 2004
 *
 *-------------------------------------------------------------------------
 */
void *
H5Iobject_verify(hid_t id, H5I_type_t id_type)
{
    void * ret_value;                      /* Return value */

    FUNC_ENTER_API(NULL)

    if(H5I_IS_LIB_TYPE(id_type))
        HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, NULL, "cannot call public function on library type")

    if(id_type < 1 || id_type >= H5I_next_type)
        HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, NULL, "identifier has invalid type")

    ret_value = H5I_object_verify(id, id_type);

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Iobject_verify() */


/*-------------------------------------------------------------------------
 * Function:	H5I_object_verify
 *
 * Purpose:	Find an object pointer for the specified ID, verifying that
 *                  its in a particular type.
 *
 * Return:	Success:	Non-null object pointer associated with the
 *				specified ID.
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, July 31, 2002
 *
 *-------------------------------------------------------------------------
 */
void *
H5I_object_verify(hid_t id, H5I_type_t id_type)
{
    H5I_id_info_t	*id_ptr = NULL;		/*ptr to the new atom	*/
    void		*ret_value = NULL;	/*return value		*/

    FUNC_ENTER_NOAPI(NULL)

    HDassert(id_type >= 1 && id_type < H5I_next_type);

    /* Verify that the type of the ID is correct & lookup the ID */
    if(id_type == H5I_TYPE(id) && NULL != (id_ptr = H5I__find_id(id))) {
        /* Get the object pointer to return */
        /* (Casting away const OK -QAK) */
        ret_value = (void *)id_ptr->obj_ptr;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5I_object_verify() */


/*-------------------------------------------------------------------------
 * Function:	H5I_get_type
 *
 * Purpose:	Given an object ID return the type to which it
 *		belongs.  The ID need not be the ID of an object which
 *		currently exists because the type number is encoded
 *		in the object ID.
 *
 * Return:	Success:	A valid type number
 *		Failure:	H5I_BADID, a negative value.
 *
 * Programmer:	Robb Matzke
 *		Friday, February 19, 1999
 *
 *-------------------------------------------------------------------------
 */
H5I_type_t
H5I_get_type(hid_t id)
{
    H5I_type_t		ret_value = H5I_BADID;

    FUNC_ENTER_NOAPI(H5I_BADID)

    if(id > 0)
        ret_value = H5I_TYPE(id);

    HDassert(ret_value >= H5I_BADID && ret_value < H5I_next_type);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I_get_type() */


/*-------------------------------------------------------------------------
 * Function:	H5Iget_type
 *
 * Purpose:	The public version of H5I_get_type(), obtains a type number
 *		when given an ID.  The ID need not be the ID of an
 *		object which currently exists because the type number is
 *		encoded as part of the ID.
 *
 * Return:	Success:	Type number
 *		Failure:	H5I_BADID, a negative value
 *
 * Programmer:	Unknown
 *
 *-------------------------------------------------------------------------
 */
H5I_type_t
H5Iget_type(hid_t id)
{
    H5I_type_t		ret_value = H5I_BADID;          /* Return value */

    FUNC_ENTER_API(H5I_BADID)
    H5TRACE1("It", "i", id);

    ret_value = H5I_get_type(id);

    if(ret_value <= H5I_BADID || ret_value >= H5I_next_type || NULL == H5I_object(id))
	HGOTO_DONE(H5I_BADID);

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Iget_type() */


/*-------------------------------------------------------------------------
 * Function:	H5Iremove_verify
 *
 * Purpose:	Removes the specified ID from its type, first checking that the
 *			type of the ID and the type type are the same.  Public interface to
 *			H5I__remove_verify.
 *
 * Return:	Success:	A pointer to the object that was removed, the
 *				same pointer which would have been found by
 *				calling H5I_object().
 *		Failure:	NULL
 *
 * Programmer:	James Laird
 *		Nathaniel Furrer
 *
 *-------------------------------------------------------------------------
 */
void *
H5Iremove_verify(hid_t id, H5I_type_t id_type)
{
    void * ret_value;                      /* Return value */

    FUNC_ENTER_API(NULL)

    if(H5I_IS_LIB_TYPE(id_type))
        HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, NULL, "cannot call public function on library type")

    /* Remove the id */
    ret_value = H5I__remove_verify(id, id_type);

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Iremove_verify() */


/*-------------------------------------------------------------------------
 * Function:	H5I__remove_verify
 *
 * Purpose:	Removes the specified ID from its type, first checking that
 *			the ID's type is the same as the ID type supplied as an argument
 *
 * Return:	Success:	A pointer to the object that was removed, the
 *				same pointer which would have been found by
 *				calling H5I_object().
 *		Failure:	NULL
 *
 * Programmer:	James Laird
 *		Nat Furrer
 *
 *-------------------------------------------------------------------------
 */
void *
H5I__remove_verify(hid_t id, H5I_type_t id_type)
{
    void * ret_value = NULL;	/*return value			*/

    FUNC_ENTER_STATIC_NOERR

    /* Argument checking will be performed by H5I_remove() */

    /* Verify that the type of the ID is correct */
    if(id_type == H5I_TYPE(id))
        ret_value = H5I_remove(id);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I__remove_verify() */


/*-------------------------------------------------------------------------
 * Function:	H5I__remove_common
 *
 * Purpose:	Common code to remove a specified ID from its type.
 *
 * Return:	Success:	A pointer to the object that was removed, the
 *				same pointer which would have been found by
 *				calling H5I_object().
 *		Failure:	NULL
 *
 * Programmer:  Quincey Koziol
 *              October 3, 2013
 *
 *-------------------------------------------------------------------------
 */
static void *
H5I__remove_common(H5I_id_type_t *type_ptr, hid_t id)
{
    H5I_id_info_t	*curr_id;	/*ptr to the current atom	*/
    void *	        ret_value;	/*return value			*/

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(type_ptr);

    /* Get the ID node for the ID */
    if(NULL == (curr_id = (H5I_id_info_t *)H5SL_remove(type_ptr->ids, &id)))
        HGOTO_ERROR(H5E_ATOM, H5E_CANTDELETE, NULL, "can't remove ID node from skip list")

    /* (Casting away const OK -QAK) */
    ret_value = (void *)curr_id->obj_ptr;

    /* If there's room, and we can save IDs of this type, then 
       save the struct (and its ID) for future re-use */
    if((type_ptr->cls->flags & H5I_CLASS_REUSE_IDS)
            && (type_ptr->avail_count < MAX_FREE_ID_STRUCTS)) {
        if(H5SL_insert(type_ptr->avail_ids, curr_id, &curr_id->id) < 0)
            HGOTO_ERROR(H5E_ATOM, H5E_CANTINSERT, NULL, "can't insert available ID node into skip list")
        type_ptr->avail_count++;
    } /* end if */
    /* Otherwise, just toss it. */
    else
        curr_id = H5FL_FREE(H5I_id_info_t, curr_id);

    /* Decrement the number of IDs in the type */
    (type_ptr->id_count)--;

    /* If there are no more IDs of this type, then we can free all available
       ID strutures, and reset starting typeid and wrapped status. */
    if(0 == type_ptr->id_count) {
        if(H5SL_free(type_ptr->avail_ids, H5I__free_cb, NULL) < 0)
            HGOTO_ERROR(H5E_ATOM, H5E_CANTREMOVE, NULL, "can't release available ID nodes")
        type_ptr->avail_count = 0;

        type_ptr->nextid = (hid_t)type_ptr->cls->reserved;
        type_ptr->wrapped = FALSE;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I__remove_common() */


/*-------------------------------------------------------------------------
 * Function:	H5I_remove
 *
 * Purpose:	Removes the specified ID from its type.
 *
 * Return:	Success:	A pointer to the object that was removed, the
 *				same pointer which would have been found by
 *				calling H5I_object().
 *		Failure:	NULL
 *
 * Programmer:	Unknown
 *
 *-------------------------------------------------------------------------
 */
void *
H5I_remove(hid_t id)
{
    H5I_id_type_t	*type_ptr;	/*ptr to the atomic type	*/
    H5I_type_t		type;		/*atom's atomic type		*/
    void *	        ret_value;	/*return value			*/

    FUNC_ENTER_NOAPI(NULL)

    /* Check arguments */
    type = H5I_TYPE(id);
    if(type <= H5I_BADID || type >= H5I_next_type)
	HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, NULL, "invalid type number")
    type_ptr = H5I_id_type_list_g[type];
    if(type_ptr == NULL || type_ptr->init_count <= 0)
	HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, NULL, "invalid type")

    /* Remove the node from the type */
    if(NULL == (ret_value = H5I__remove_common(type_ptr, id)))
        HGOTO_ERROR(H5E_ATOM, H5E_CANTDELETE, NULL, "can't remove ID node")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I_remove() */


/*-------------------------------------------------------------------------
 * Function:	H5Idec_ref
 *
 * Purpose:	Decrements the number of references outstanding for an ID.
 *              If the reference count for an ID reaches zero, the object
 *              will be closed.
 *
 * Return:	Success:	New reference count
 *		Failure:	Negative
 *
 * Programmer:  Quincey Koziol
 *              Dec  7, 2003
 *
 *-------------------------------------------------------------------------
 */
int
H5Idec_ref(hid_t id)
{
    int ret_value;                      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("Is", "i", id);

    /* Check arguments */
    if(id < 0)
	HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "invalid ID")

    /* Do actual decrement operation */
    if((ret_value = H5I_dec_app_ref(id)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTDEC, FAIL, "can't decrement ID ref count")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Idec_ref() */


/*-------------------------------------------------------------------------
 * Function:	H5I_dec_ref
 *
 * Purpose:	Decrements the number of references outstanding for an ID.
 *		This will fail if the type is not a reference counted type.
 *		The ID type's 'free' function will be called for the ID
 *		if the reference count for the ID reaches 0 and a free
 *		function has been defined at type creation time.
 *
 * Return:	Success:	New reference count.
 *
 *		Failure:	Negative
 *
 * Programmer:	Unknown
 *
 *-------------------------------------------------------------------------
 */
int
H5I_dec_ref(hid_t id)
{
    H5I_type_t		type;		/*type the object is in*/
    H5I_id_type_t	*type_ptr;	/*ptr to the type	*/
    H5I_id_info_t	*id_ptr;	/*ptr to the new ID	*/
    int ret_value;                      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(id >= 0);

    /* Check arguments */
    type = H5I_TYPE(id);
    if(type <= H5I_BADID || type >= H5I_next_type)
	HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "invalid type number")
    type_ptr = H5I_id_type_list_g[type];
    if(NULL == type_ptr || type_ptr->init_count <= 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "invalid type number")

    /* General lookup of the ID */
    if(NULL == (id_ptr = (H5I_id_info_t *)H5SL_search(type_ptr->ids, &id)))
	HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't locate ID")

    /*
     * If this is the last reference to the object then invoke the type's
     * free method on the object. If the free method is undefined or
     * successful then remove the object from the type; otherwise leave
     * the object in the type without decrementing the reference
     * count. If the reference count is more than one then decrement the
     * reference count without calling the free method.
     *
     * Beware: the free method may call other H5I functions.
     * 
     * If an object is closing, we can remove the ID even though the free 
     * method might fail.  This can happen when a mandatory filter fails to
     * write when a dataset is closed and the chunk cache is flushed to the 
     * file.  We have to close the dataset anyway. (SLU - 2010/9/7)
     */
    if(1 == id_ptr->count) {
        /* (Casting away const OK -QAK) */
        if(!type_ptr->cls->free_func || (type_ptr->cls->free_func)((void *)id_ptr->obj_ptr) >= 0) {
            /* Remove the node from the type */
            if(NULL == H5I__remove_common(type_ptr, id))
                HGOTO_ERROR(H5E_ATOM, H5E_CANTDELETE, FAIL, "can't remove ID node")
            ret_value = 0;
        } /* end if */
        else
            ret_value = FAIL;
    } /* end if */
    else {
        --(id_ptr->count);
        ret_value = (int)id_ptr->count;
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I_dec_ref() */


/*-------------------------------------------------------------------------
 * Function:	H5I_dec_app_ref
 *
 * Purpose:	H5I_dec_ref wrapper for case of modifying the application ref.
 *		count for an ID as well as normal reference count.
 *
 * Return:	Success:	New app. reference count.
 *		Failure:	Negative
 *
 * Programmer:  Quincey Koziol
 *              Sept 16, 2010
 *
 *-------------------------------------------------------------------------
 */
int
H5I_dec_app_ref(hid_t id)
{
    H5I_id_info_t	*id_ptr;	/*ptr to the new ID	*/
    int ret_value;                      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(id >= 0);

    /* Call regular decrement reference count routine */
    if((ret_value = H5I_dec_ref(id)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTDEC, FAIL, "can't decrement ID ref count")

    /* Check if the ID still exists */
    if(ret_value > 0) {
        /* General lookup of the ID */
        if(NULL == (id_ptr = H5I__find_id(id)))
            HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't locate ID")

        /* Adjust app_ref */
        --(id_ptr->app_count);
        HDassert(id_ptr->count >= id_ptr->app_count);

        /* Set return value */
        ret_value = (int)id_ptr->app_count;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I_dec_app_ref() */


/*-------------------------------------------------------------------------
 * Function:	H5I_dec_app_ref_always_close
 *
 * Purpose:	H5I_dec_app_ref wrapper for case of always closing the ID,
 *		even when the free routine fails
 *
 * Return:	Success:	New app. reference count.
 *		Failure:	Negative
 *
 * Programmer:  Quincey Koziol
 *              Sept 16, 2010
 *
 *-------------------------------------------------------------------------
 */
int
H5I_dec_app_ref_always_close(hid_t id)
{
    int ret_value;                      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(id >= 0);

    /* Call application decrement reference count routine */
    ret_value = H5I_dec_app_ref(id);

    /* Check for failure */
    if(ret_value < 0) {
        /*
         * If an object is closing, we can remove the ID even though the free 
         * method might fail.  This can happen when a mandatory filter fails to
         * write when a dataset is closed and the chunk cache is flushed to the 
         * file.  We have to close the dataset anyway. (SLU - 2010/9/7)
         */
        H5I_remove(id);

        HGOTO_ERROR(H5E_ATOM, H5E_CANTDEC, FAIL, "can't decrement ID ref count")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I_dec_app_ref_always_close() */


/*-------------------------------------------------------------------------
 * Function:	H5Iinc_ref
 *
 * Purpose:	Increments the number of references outstanding for an ID.
 *
 * Return:	Success:	New reference count
 *		Failure:	Negative
 *
 * Programmer:  Quincey Koziol
 *              Dec  7, 2003
 *
 *-------------------------------------------------------------------------
 */
int
H5Iinc_ref(hid_t id)
{
    int ret_value;                      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("Is", "i", id);

    /* Check arguments */
    if(id < 0)
	HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "invalid ID")

    /* Do actual increment operation */
    if((ret_value = H5I_inc_ref(id, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTINC, FAIL, "can't increment ID ref count")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Iinc_ref() */


/*-------------------------------------------------------------------------
 * Function:	H5I_inc_ref
 *
 * Purpose:	Increment the reference count for an object.
 *
 * Return:	Success:	The new reference count.
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Thursday, July 29, 1999
 *
 *-------------------------------------------------------------------------
 */
int
H5I_inc_ref(hid_t id, hbool_t app_ref)
{
    H5I_type_t		type;		/*type the object is in*/
    H5I_id_type_t	*type_ptr;	/*ptr to the type	*/
    H5I_id_info_t	*id_ptr;	/*ptr to the ID		*/
    int ret_value;                      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(id >= 0);

    /* Check arguments */
    type = H5I_TYPE(id);
    if(type <= H5I_BADID || type >= H5I_next_type)
	HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "invalid type number")
    type_ptr = H5I_id_type_list_g[type];
    if(!type_ptr || type_ptr->init_count <= 0)
	HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, FAIL, "invalid type")

    /* General lookup of the ID */
    if(NULL == (id_ptr = (H5I_id_info_t *)H5SL_search(type_ptr->ids, &id)))
	HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't locate ID")

    /* Adjust reference counts */
    ++(id_ptr->count);
    if (app_ref)
        ++(id_ptr->app_count);

    /* Set return value */
    ret_value = (int)(app_ref ? id_ptr->app_count : id_ptr->count);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I_inc_ref() */


/*-------------------------------------------------------------------------
 * Function:	H5Iget_ref
 *
 * Purpose:	Retrieves the number of references outstanding for an ID.
 *
 * Return:	Success:	Reference count
 *		Failure:	Negative
 *
 * Programmer:  Quincey Koziol
 *              Dec  7, 2003
 *
 *-------------------------------------------------------------------------
 */
int
H5Iget_ref(hid_t id)
{
    int ret_value;                      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("Is", "i", id);

    /* Check arguments */
    if(id < 0)
	HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "invalid ID")

    /* Do actual retrieve operation */
    if((ret_value = H5I_get_ref(id, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTGET, FAIL, "can't get ID ref count")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Iget_ref() */


/*-------------------------------------------------------------------------
 * Function:	H5I_get_ref
 *
 * Purpose:	Retrieve the reference count for an object.
 *
 * Return:	Success:	The reference count.
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Saturday, Decemeber  6, 2003
 *
 *-------------------------------------------------------------------------
 */
int
H5I_get_ref(hid_t id, hbool_t app_ref)
{
    H5I_type_t		type;		/*type the object is in*/
    H5I_id_type_t	*type_ptr;	/*ptr to the type	*/
    H5I_id_info_t	*id_ptr;	/*ptr to the ID		*/
    int ret_value;                      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(id >= 0);

    /* Check arguments */
    type = H5I_TYPE(id);
    if(type <= H5I_BADID || type >= H5I_next_type)
	HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "invalid type number")
    type_ptr = H5I_id_type_list_g[type];
    if(!type_ptr || type_ptr->init_count <= 0)
	HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, FAIL, "invalid type")

    /* General lookup of the ID */
    if(NULL == (id_ptr = (H5I_id_info_t *)H5SL_search(type_ptr->ids, &id)))
	HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't locate ID")

    /* Set return value */
    ret_value = (int)(app_ref ? id_ptr->app_count : id_ptr->count);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I_get_ref() */


/*-------------------------------------------------------------------------
 * Function:	H5Iinc_type_ref
 *
 * Purpose:	Increments the number of references outstanding for an ID type.
 *
 * Return:	Success:	New reference count
 *		Failure:	Negative
 *
 * Programmer:  Nat Furrer
 *		James Laird
 *              April 30, 2004
 *
 *-------------------------------------------------------------------------
 */
int
H5Iinc_type_ref(H5I_type_t type)
{
    int ret_value;                      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("Is", "It", type);

    /* Check arguments */
    if(type <= 0 || type >= H5I_next_type)
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "invalid ID type")

    if(H5I_IS_LIB_TYPE(type))
        HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, FAIL, "cannot call public function on library type")

    /* Do actual increment operation */
    if((ret_value = H5I__inc_type_ref(type)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTINC, FAIL, "can't increment ID type ref count")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Iinc_ref() */


/*-------------------------------------------------------------------------
 * Function:	H5I__inc_type_ref
 *
 * Purpose:	Increment the reference count for an ID type.
 *
 * Return:	Success:	The new reference count.
 *		Failure:	Negative
 *
 * Programmer:	James Laird
 *		Nat Furrer
 *              Friday, April 30, 2004
 *
 *-------------------------------------------------------------------------
 */
static int
H5I__inc_type_ref(H5I_type_t type)
{
    H5I_id_type_t	*type_ptr;	/* ptr to the type	*/
    int ret_value;                      /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(type > 0 && type < H5I_next_type);

    /* Check arguments */
    type_ptr = H5I_id_type_list_g[type];
    if(!type_ptr)
	HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, FAIL, "invalid type")

    /* Set return value */
    ret_value = (int)(++(type_ptr->init_count));

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I__inc_type_ref() */


/*-------------------------------------------------------------------------
 * Function:	H5Idec_type_ref
 *
 * Purpose:	Decrements the reference count on an entire type of IDs.
 *		If the type reference count becomes zero then the type is
 *		destroyed along with all atoms in that type regardless of
 *		their reference counts.	 Destroying IDs involves calling
 *		the free-func for each ID's object and then adding the ID
 *		struct to the ID free list.  Public interface to
 *		H5I_dec_type_ref.
 *		Returns the number of references to the type on success; a
 *		return value of 0 means that the type will have to be
 *		re-initialized before it can be used again (and should probably
 *		be set to H5I_UNINIT).
 *
 * Return:	Number of references to type on success/Negative on failure
 *
 * Programmer:	Nathaniel Furrer
 *		James Laird
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Idec_type_ref(H5I_type_t type)
{
    herr_t ret_value;           /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "It", type);

    if(H5I_IS_LIB_TYPE(type))
        HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, FAIL, "cannot call public function on library type")

    ret_value = H5I_dec_type_ref(type);

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Idec_type_ref() */


/*-------------------------------------------------------------------------
 * Function:	H5I_dec_type_ref
 *
 * Purpose:	Decrements the reference count on an entire type of IDs.
 *		If the type reference count becomes zero then the type is
 *		destroyed along with all atoms in that type regardless of
 *		their reference counts.	 Destroying IDs involves calling
 *		the free-func for each ID's object and then adding the ID
 *		struct to the ID free list.
 *		Returns the number of references to the type on success; a
 *		return value of 0 means that the type will have to be
 *		re-initialized before it can be used again (and should probably
 *		be set to H5I_UNINIT).
 *
 * Return:	Number of references to type on success/Negative on failure
 *
 * Programmer:	Unknown
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5I_dec_type_ref(H5I_type_t type)
{
    H5I_id_type_t	*type_ptr;      /* Pointer to the ID type */
    herr_t		ret_value;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if(type <= H5I_BADID || type >= H5I_next_type)
	HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "invalid type number")

    type_ptr = H5I_id_type_list_g[type];
    if(type_ptr == NULL || type_ptr->init_count <= 0)
	HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, FAIL, "invalid type")

    /*
     * Decrement the number of users of the atomic type.  If this is the
     * last user of the type then release all atoms from the type and
     * free all memory it used.  The free function is invoked for each atom
     * being freed.
     */
    if(1 == type_ptr->init_count) {
        H5I__destroy_type(type);
        ret_value = 0;
    } /* end if */
    else {
        --(type_ptr->init_count);
        ret_value = (herr_t)type_ptr->init_count;
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I_dec_type_ref() */


/*-------------------------------------------------------------------------
 * Function:	H5Iget_type_ref
 *
 * Purpose:	Retrieves the number of references outstanding for a type.
 *
 * Return:	Success:	Reference count
 *		Failure:	Negative
 *
 * Programmer:  Nat Furrer
 *		James Laird
 *              April 30, 2004
 *
 *-------------------------------------------------------------------------
 */
int
H5Iget_type_ref(H5I_type_t type)
{
    int ret_value;                      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("Is", "It", type);

    /* Check arguments */
    if(type <= 0 || type >= H5I_next_type)
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "invalid ID type")

    if(H5I_IS_LIB_TYPE(type))
        HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, FAIL, "cannot call public function on library type")

    /* Do actual retrieve operation */
    if((ret_value = H5I__get_type_ref(type)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTGET, FAIL, "can't get ID type ref count")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Iget_ref() */


/*-------------------------------------------------------------------------
 * Function:	H5I__get_type_ref
 *
 * Purpose:	Retrieve the reference count for an ID type.
 *
 * Return:	Success:	The reference count.
 *
 *		Failure:	Negative
 *
 * Programmer:  Nat Furrer
 *		James Laird
 *              April 30, 2004
 *
 *-------------------------------------------------------------------------
 */
static int
H5I__get_type_ref(H5I_type_t type)
{
    H5I_id_type_t	*type_ptr;	/*ptr to the type	*/
    int ret_value;                      /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(type >= 0);

    /* Check arguments */
    type_ptr = H5I_id_type_list_g[type];
    if(!type_ptr)
        HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, FAIL, "invalid type")

    /* Set return value */
    ret_value = (int)type_ptr->init_count;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I__get_type_ref() */


/*-------------------------------------------------------------------------
 * Function:	H5Iis_valid
 *
 * Purpose:	Check if the given id is valid.  An id is valid if it is in
 *              use and has an application reference count of at least 1.
 *
 * Return:	Success:        TRUE if the id is valid, FALSE otherwise.
 *
 *              Failure:	Negative (never fails currently)
 *
 * Programmer:  Neil Fortner
 *              Friday, October 31, 2008 (boo)
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Iis_valid(hid_t id)
{
    H5I_id_info_t   *id_ptr;            /* ptr to the ID */
    htri_t          ret_value = TRUE;   /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("t", "i", id);

    /* Find the ID */
    if (NULL == (id_ptr = H5I__find_id(id)))
        ret_value = FALSE;

    /* Check if the found id is an internal id */
    else if (!id_ptr->app_count)
        ret_value = FALSE;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Iis_valid() */


/*-------------------------------------------------------------------------
 * Function:	H5I__search_cb
 *
 * Purpose:	Callback routine for H5Isearch, when it calls H5I_iterate.
 *		Calls "user" callback search function, and then sets return
 *		value, based on the result of that callback.
 *
 * Return:	Success:	The first object in the type for which FUNC
 *				returns non-zero. NULL if FUNC returned zero
 *				for every object in the type.
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *		Friday, March 30, 2012
 *
 *-------------------------------------------------------------------------
 */
static int
H5I__search_cb(void *obj, hid_t id, void *_udata)
{
    H5I_search_ud_t *udata = (H5I_search_ud_t *)_udata; /* User data for callback */
    int ret_value;     /* Callback return value */

    FUNC_ENTER_STATIC_NOERR

    ret_value = (*udata->app_cb)(obj, id, udata->app_key);
    if(ret_value > 0)
        udata->ret_obj = obj;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I__search_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5Isearch
 *
 * Purpose:	Apply function FUNC to each member of type TYPE and return a
 *		pointer to the first object for which FUNC returns non-zero.
 *		The FUNC should take a pointer to the object and the KEY as
 *		arguments and return non-zero to terminate the search (zero
 *		to continue).  Public interface to H5I_search.
 *
 * Limitation:	Currently there is no way to start searching from where a
 *		previous search left off.
 *
 * Return:	Success:	The first object in the type for which FUNC
 *				returns non-zero. NULL if FUNC returned zero
 *				for every object in the type.
 *
 *		Failure:	NULL
 *
 * Programmer:	James Laird
 *		Nathaniel Furrer
 *		Friday, April 23, 2004
 *
 *-------------------------------------------------------------------------
 */
void *
H5Isearch(H5I_type_t type, H5I_search_func_t func, void *key)
{
    H5I_search_ud_t udata;      /* Context for iteration */
    void *ret_value;            /* Return value */

    FUNC_ENTER_API(NULL)

    /* Check arguments */
    if(H5I_IS_LIB_TYPE(type))
        HGOTO_ERROR(H5E_ATOM, H5E_BADGROUP, NULL, "cannot call public function on library type")

    /* Set up udata struct */
    udata.app_cb = func;
    udata.app_key = key;
    udata.ret_obj = NULL;

    /* Note that H5I_iterate returns an error code.  We ignore it 
     * here, as we can't do anything with it without revising the API.
     */
    (void)H5I_iterate(type, H5I__search_cb, &udata, TRUE);

    /* Set return value */
    ret_value = udata.ret_obj;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Isearch() */


/*-------------------------------------------------------------------------
 * Function:	H5I__iterate_cb
 *
 * Purpose:	Callback routine for H5I_iterate, invokes "user" callback
 *              function, and then sets return value, based on the result of
 *              that callback.
 *
 * Return:	Success:	Non-negative on success
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		Thursday, October 3, 2013
 *
 *-------------------------------------------------------------------------
 */
static int
H5I__iterate_cb(void *_item, void UNUSED *_key, void *_udata)
{
    H5I_id_info_t *item = (H5I_id_info_t *)_item;       /* Pointer to the ID node */
    H5I_iterate_ud_t *udata = (H5I_iterate_ud_t *)_udata; /* User data for callback */
    int ret_value = H5_ITER_CONT;     /* Callback return value */

    FUNC_ENTER_STATIC_NOERR

    /* Don't make callback if app_ref is set and the appl. ref count is 0 */
    if((!udata->app_ref) || (item->app_count > 0)) {
        herr_t cb_ret_val;

        /* (Casting away const OK) */
        cb_ret_val = (*udata->user_func)((void *)item->obj_ptr, item->id, udata->user_udata);
        if(cb_ret_val > 0)
            ret_value = H5_ITER_STOP;	/* terminate iteration early */
        else if(cb_ret_val < 0)
            ret_value = H5_ITER_ERROR;  /* indicate failure (which terminates iteration) */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I__iterate_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5I_iterate
 *
 * Purpose:	Apply function FUNC to each member of type TYPE (with 
 *		non-zero application reference count if app_ref is TRUE).  
 *		Stop if FUNC returns a non zero value (i.e. anything 
 *		other than H5_ITER_CONT).  
 *
 *		If FUNC returns a positive value (i.e. H5_ITER_STOP), 
 *		return SUCCEED.
 *
 *		If FUNC returns a negative value (i.e. H5_ITER_ERROR), 
 *		return FAIL.
 *		
 *		The FUNC should take a pointer to the object and the 
 *		udata as arguments and return non-zero to terminate 
 *		siteration, and zero to continue.
 *
 * Limitation:	Currently there is no way to start the iteration from 
 *		where a previous iteration left off.
 *
 * Return:	Success:	SUCCEED
 *		Failure:	FAIL
 *
 * Programmer:	John Mainzer
 *		Monday, December 6, 2011
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5I_iterate(H5I_type_t type, H5I_search_func_t func, void *udata, hbool_t app_ref)
{
    H5I_id_type_t *type_ptr;		/*ptr to the type	*/
    herr_t	   ret_value = SUCCEED;	/*return value		*/

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments */
    if(type <= H5I_BADID || type >= H5I_next_type)
	HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "invalid type number")
    type_ptr = H5I_id_type_list_g[type];

    /* Only iterate through ID list if it is initialized and there are IDs in type */
    if(type_ptr && type_ptr->init_count > 0 && type_ptr->id_count > 0) {
        H5I_iterate_ud_t iter_udata;    /* User data for iteration callback */
        herr_t iter_status;             /* Iteration status */

        /* Set up iterator user data */
        iter_udata.user_func = func;
        iter_udata.user_udata = udata;
        iter_udata.app_ref = app_ref;

        /* Iterate over IDs */
        if((iter_status = H5SL_iterate(type_ptr->ids, H5I__iterate_cb, &iter_udata)) < 0)
            HGOTO_ERROR(H5E_ATOM, H5E_BADITER, FAIL, "iteration failed")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I_iterate() */


/*-------------------------------------------------------------------------
 * Function:	H5I__find_id
 *
 * Purpose:	Given an object ID find the info struct that describes the
 *		object.
 *
 * Return:	Success:	Ptr to the object's info struct.
 *
 *		Failure:	NULL
 *
 * Programmer:	Unknown
 *
 *-------------------------------------------------------------------------
 */
static H5I_id_info_t *
H5I__find_id(hid_t id)
{
    H5I_id_type_t	*type_ptr;		/*ptr to the type	*/
    H5I_type_t		type;			/*ID's type		*/
    H5I_id_info_t	*ret_value;		/*return value		*/

    FUNC_ENTER_STATIC_NOERR

    /* Check arguments */
    type = H5I_TYPE(id);
    if (type <= H5I_BADID || type >= H5I_next_type)
        HGOTO_DONE(NULL);

    type_ptr = H5I_id_type_list_g[type];
    if (!type_ptr || type_ptr->init_count <= 0)
        HGOTO_DONE(NULL);

    /* Locate the ID node for the ID */
    ret_value = (H5I_id_info_t *)H5SL_search(type_ptr->ids, &id);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I__find_id() */


/*-------------------------------------------------------------------------
 * Function: H5Iget_name
 *
 * Purpose: Gets a name of an object from its ID.
 *
 * Return: Success: The length of name.
 *
 *         Failure: -1
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: July 26, 2002
 *
 * Comments: Public function
 *  If `name' is non-NULL then write up to `size' bytes into that
 *  buffer and always return the length of the entry name.
 *  Otherwise `size' is ignored and the function does not store the name,
 *  just returning the number of characters required to store the name.
 *  If an error occurs then the buffer pointed to by `name' (NULL or non-NULL)
 *  is unchanged and the function returns a negative value.
 *  If a zero is returned for the name's length, then there is no name
 *  associated with the ID.
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5Iget_name(hid_t id, char *name/*out*/, size_t size)
{
    H5G_loc_t     loc;          /* Object location */
    ssize_t       ret_value;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("Zs", "ixz", id, name, size);

    /* Get object location */
    if(H5G_loc(id, &loc) < 0)
	HGOTO_ERROR(H5E_ATOM, H5E_CANTGET, FAIL, "can't retrieve object location")

    /* Call internal group routine to retrieve object's name */
    if((ret_value = H5G_get_name(&loc, name, size, NULL, H5P_DEFAULT, H5AC_ind_dxpl_id)) < 0)
	HGOTO_ERROR(H5E_ATOM, H5E_CANTGET, FAIL, "can't retrieve object name")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Iget_name() */


/*-------------------------------------------------------------------------
 * Function:	H5Iget_file_id
 *
 * Purpose:	The public version of H5I_get_file_id(), obtains the file
 *              ID given an object ID.  User has to close this ID.
 *
 * Return:	Success:	file ID
 *
 *		Failure:	a negative value
 *
 * Programmer:  Raymond Lu
 *              Oct 27, 2003
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Iget_file_id(hid_t obj_id)
{
    hid_t ret_value;            /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("i", "i", obj_id);

    if((ret_value = H5I_get_file_id(obj_id, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTGET, FAIL, "can't retrieve file ID")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Iget_file_id() */


/*-------------------------------------------------------------------------
 * Function:	H5I_get_file_id
 *
 * Purpose:	The private version of H5Iget_file_id(), obtains the file
 *              ID given an object ID.
 *
 * Return:	Success:	file ID
 *		Failure:	a negative value
 *
 * Programmer:  Raymond Lu
 *              Oct 27, 2003
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5I_get_file_id(hid_t obj_id, hbool_t app_ref)
{
    H5I_type_t type;            /* ID type */
    hid_t ret_value;            /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Get object type */
    type = H5I_TYPE(obj_id);
    if(type == H5I_FILE) {
        /* Increment reference count on file ID */
        if(H5I_inc_ref(obj_id, app_ref) < 0)
            HGOTO_ERROR(H5E_ATOM, H5E_CANTSET, FAIL, "incrementing file ID failed")

        /* Set return value */
        ret_value = obj_id;
    } /* end if */
    else if(type == H5I_DATATYPE || type == H5I_GROUP || type == H5I_DATASET || type == H5I_ATTR) {
        H5G_loc_t loc;              /* Location of object */

        /* Get the object location information */
        if(H5G_loc(obj_id, &loc) < 0)
            HGOTO_ERROR(H5E_ATOM, H5E_CANTGET, FAIL, "can't get object location")

        /* Get the file ID for the object */
        if((ret_value = H5F_get_id(loc.oloc->file, app_ref)) < 0)
            HGOTO_ERROR(H5E_ATOM, H5E_CANTGET, FAIL, "can't get file ID")
    } /* end if */
    else
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "invalid object ID")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I_get_file_id() */

#ifdef H5I_DEBUG_OUTPUT

/*-------------------------------------------------------------------------
 * Function:	H5I__debug_cb
 *
 * Purpose:	Dump the contents of an ID to stderr for debugging.
 *
 * Return:	Success: Non-negative
 *		Failure: Negative
 *
 * Programmer:	Robb Matzke
 *		Friday, February 19, 1999
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5I__debug_cb(void *_item, void UNUSED *_key, void *_udata)
{
    H5I_id_info_t *item = (H5I_id_info_t *)_item;       /* Pointer to the ID node */
    H5I_type_t type = *(H5I_type_t *)_udata;            /* User data */
    H5G_name_t *path = NULL;
    int ret_value = H5_ITER_CONT;                       /* Return value */

    FUNC_ENTER_STATIC_NOERR

    fprintf(stderr, "		 id = %lu\n", (unsigned long)(item->id));
    fprintf(stderr, "		 count = %u\n", item->count);
    fprintf(stderr, "		 obj   = 0x%08lx\n", (unsigned long)(item->obj_ptr));

    /* Get the group location, so we get get the name */
    switch(type) {
        case H5I_GROUP:
            path = H5G_nameof((H5G_t*)item->obj_ptr);
            break;

        case H5I_DATASET:
            path = H5D_nameof((H5D_t*)item->obj_ptr);
            break;

        case H5I_DATATYPE:
            path = H5T_nameof((H5T_t*)item->obj_ptr);
            break;

        default:
            break;   /* Other types of IDs are not stored in files */
    } /* end switch*/

    if(path) {
        if(path->user_path_r)
            fprintf(stderr, "                user_path = %s\n", H5RS_get_str(path->user_path_r));
        if(path->full_path_r)
            fprintf(stderr, "                full_path = %s\n", H5RS_get_str(path->full_path_r));
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5I__debug_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5I__debug
 *
 * Purpose:	Dump the contents of a type to stderr for debugging.
 *
 * Return:	Success: Non-negative
 *		Failure: Negative
 *
 * Programmer:	Robb Matzke
 *		Friday, February 19, 1999
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5I__debug(H5I_type_t type)
{
    H5I_id_type_t *type_ptr;

    FUNC_ENTER_STATIC_NOERR

    fprintf(stderr, "Dumping ID type %d\n", (int)type);
    type_ptr = H5I_id_type_list_g[type];

    /* Header */
    fprintf(stderr, "	 init_count = %u\n", type_ptr->init_count);
    fprintf(stderr, "	 reserved   = %u\n", type_ptr->cls->reserved);
    fprintf(stderr, "	 wrapped    = %u\n", type_ptr->wrapped);
    fprintf(stderr, "	 id_count   = %u\n", type_ptr->id_count);
    fprintf(stderr, "	 nextid	    = %u\n", type_ptr->nextid);

    /* List */
    fprintf(stderr, "	 List:\n");
    H5SL_iterate(type_ptr->ids, H5I__debug_cb, &type);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5I__debug() */
#endif /* H5I_DEBUG_OUTPUT */

