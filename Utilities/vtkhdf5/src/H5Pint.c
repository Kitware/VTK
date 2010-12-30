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
 *
 * Purpose:	Generic Property Functions
 */

/****************/
/* Module Setup */
/****************/

#define H5P_PACKAGE		/*suppress error about including H5Ppkg	  */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5P_init_interface


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free lists                           */
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Ppkg.h"		/* Property lists		  	*/

/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/

/* Typedef for checking for duplicate class names in parent class */
typedef struct {
    const H5P_genclass_t *parent;       /* Pointer to parent class */
    const char *name;                   /* Pointer to name to check */
} H5P_check_class_t;


/********************/
/* Local Prototypes */
/********************/

/* General helper routines */
static H5P_genprop_t *H5P_dup_prop(H5P_genprop_t *oprop, H5P_prop_within_t type);
static herr_t H5P_free_prop(H5P_genprop_t *prop);
static int H5P_cmp_prop(const H5P_genprop_t *prop1, const H5P_genprop_t *prop2);


/*********************/
/* Package Variables */
/*********************/

/*
 * Predefined property list classes. These are initialized at runtime by
 * H5P_init_interface() in this source file.
 */
hid_t H5P_CLS_ROOT_g                = FAIL;
hid_t H5P_CLS_OBJECT_CREATE_g       = FAIL;
hid_t H5P_CLS_FILE_CREATE_g         = FAIL;
hid_t H5P_CLS_FILE_ACCESS_g         = FAIL;
hid_t H5P_CLS_DATASET_CREATE_g      = FAIL;
hid_t H5P_CLS_DATASET_ACCESS_g      = FAIL;
hid_t H5P_CLS_DATASET_XFER_g        = FAIL;
hid_t H5P_CLS_FILE_MOUNT_g          = FAIL;
hid_t H5P_CLS_GROUP_CREATE_g        = FAIL;
hid_t H5P_CLS_GROUP_ACCESS_g        = FAIL;
hid_t H5P_CLS_DATATYPE_CREATE_g     = FAIL;
hid_t H5P_CLS_DATATYPE_ACCESS_g     = FAIL;
hid_t H5P_CLS_ATTRIBUTE_CREATE_g    = FAIL;
hid_t H5P_CLS_OBJECT_COPY_g         = FAIL;
hid_t H5P_CLS_LINK_CREATE_g         = FAIL;
hid_t H5P_CLS_LINK_ACCESS_g         = FAIL;
hid_t H5P_CLS_STRING_CREATE_g         = FAIL;

/*
 * Predefined property lists for each predefined class. These are initialized
 * at runtime by H5P_init_interface() in this source file.
 */
hid_t H5P_LST_FILE_CREATE_g         = FAIL;
hid_t H5P_LST_FILE_ACCESS_g         = FAIL;
hid_t H5P_LST_DATASET_CREATE_g      = FAIL;
hid_t H5P_LST_DATASET_ACCESS_g      = FAIL;
hid_t H5P_LST_DATASET_XFER_g        = FAIL;
hid_t H5P_LST_FILE_MOUNT_g          = FAIL;
hid_t H5P_LST_GROUP_CREATE_g        = FAIL;
hid_t H5P_LST_GROUP_ACCESS_g        = FAIL;
hid_t H5P_LST_DATATYPE_CREATE_g     = FAIL;
hid_t H5P_LST_DATATYPE_ACCESS_g     = FAIL;
hid_t H5P_LST_ATTRIBUTE_CREATE_g    = FAIL;
hid_t H5P_LST_OBJECT_COPY_g         = FAIL;
hid_t H5P_LST_LINK_CREATE_g         = FAIL;
hid_t H5P_LST_LINK_ACCESS_g         = FAIL;

/* Root property list class library initialization object */
const H5P_libclass_t H5P_CLS_ROOT[1] = {{
    "root",			/* Class name for debugging     */
    NULL,			/* Parent class ID              */
    &H5P_CLS_ROOT_g,		/* Pointer to class ID          */
    NULL,			/* Pointer to default property list ID */
    NULL,			/* Default property registration routine */
    NULL,		        /* Class creation callback      */
    NULL,		        /* Class creation callback info */
    NULL,			/* Class copy callback          */
    NULL,		        /* Class copy callback info     */
    NULL,			/* Class close callback         */
    NULL 		        /* Class close callback info    */
}};

/* Group access property list class library initialization object */
/* (move to proper source code file when used for real) */
const H5P_libclass_t H5P_CLS_GACC[1] = {{
    "group access",		/* Class name for debugging     */
    &H5P_CLS_LINK_ACCESS_g,	/* Parent class ID              */
    &H5P_CLS_GROUP_ACCESS_g,	/* Pointer to class ID          */
    &H5P_LST_GROUP_ACCESS_g,	/* Pointer to default property list ID */
    NULL,			/* Default property registration routine */
    NULL,		        /* Class creation callback      */
    NULL,		        /* Class creation callback info */
    NULL,			/* Class copy callback          */
    NULL,		        /* Class copy callback info     */
    NULL,			/* Class close callback         */
    NULL 		        /* Class close callback info    */
}};

/* Datatype creation property list class library initialization object */
/* (move to proper source code file when used for real) */
const H5P_libclass_t H5P_CLS_TCRT[1] = {{
    "datatype create",		/* Class name for debugging     */
    &H5P_CLS_OBJECT_CREATE_g,	/* Parent class ID              */
    &H5P_CLS_DATATYPE_CREATE_g,	/* Pointer to class ID          */
    &H5P_LST_DATATYPE_CREATE_g,	/* Pointer to default property list ID */
    NULL,			/* Default property registration routine */
    NULL,		        /* Class creation callback      */
    NULL,		        /* Class creation callback info */
    NULL,			/* Class copy callback          */
    NULL,		        /* Class copy callback info     */
    NULL,			/* Class close callback         */
    NULL 		        /* Class close callback info    */
}};

/* Datatype access property list class library initialization object */
/* (move to proper source code file when used for real) */
const H5P_libclass_t H5P_CLS_TACC[1] = {{
    "datatype access",		/* Class name for debugging     */
    &H5P_CLS_LINK_ACCESS_g,	/* Parent class ID              */
    &H5P_CLS_DATATYPE_ACCESS_g,	/* Pointer to class ID          */
    &H5P_LST_DATATYPE_ACCESS_g,	/* Pointer to default property list ID */
    NULL,			/* Default property registration routine */
    NULL,		        /* Class creation callback      */
    NULL,		        /* Class creation callback info */
    NULL,			/* Class copy callback          */
    NULL,		        /* Class copy callback info     */
    NULL,			/* Class close callback         */
    NULL 		        /* Class close callback info    */
}};


/* Library property list classes defined in other code modules */
H5_DLLVAR const H5P_libclass_t H5P_CLS_OCRT[1];         /* Object creation */
H5_DLLVAR const H5P_libclass_t H5P_CLS_STRCRT[1];       /* String create */
H5_DLLVAR const H5P_libclass_t H5P_CLS_LACC[1];         /* Link access */
H5_DLLVAR const H5P_libclass_t H5P_CLS_GCRT[1];         /* Group create */
H5_DLLVAR const H5P_libclass_t H5P_CLS_OCPY[1];         /* Object copy */
H5_DLLVAR const H5P_libclass_t H5P_CLS_FCRT[1];         /* File creation */
H5_DLLVAR const H5P_libclass_t H5P_CLS_FACC[1];         /* File access */
H5_DLLVAR const H5P_libclass_t H5P_CLS_DCRT[1];         /* Dataset creation */
H5_DLLVAR const H5P_libclass_t H5P_CLS_DACC[1];         /* Dataset access */
H5_DLLVAR const H5P_libclass_t H5P_CLS_DXFR[1];         /* Data transfer */
H5_DLLVAR const H5P_libclass_t H5P_CLS_FMNT[1];         /* File mount */
H5_DLLVAR const H5P_libclass_t H5P_CLS_ACRT[1];         /* Attribute creation */
H5_DLLVAR const H5P_libclass_t H5P_CLS_LCRT[1];         /* Link creation */


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/

/* Track the revision count of a class, to make comparisons faster */
static unsigned H5P_next_rev = 0;
#define H5P_GET_NEXT_REV        (H5P_next_rev++)

/* List of all property list classes in the library */
/* (order here is not important, they will be initialized in the proper
 *      order according to their parent class dependencies)
 */
static H5P_libclass_t const * const init_class[] = {
    H5P_CLS_ROOT,       /* Root */
    H5P_CLS_OCRT,       /* Object create */
    H5P_CLS_STRCRT,     /* String create */
    H5P_CLS_LACC,       /* Link access */
    H5P_CLS_GCRT,       /* Group create */
    H5P_CLS_OCPY,       /* Object copy */
    H5P_CLS_GACC,       /* Group access */
    H5P_CLS_FCRT,       /* File creation */
    H5P_CLS_FACC,       /* File access */
    H5P_CLS_DCRT,       /* Dataset creation */
    H5P_CLS_DACC,       /* Dataset access */
    H5P_CLS_DXFR,       /* Data transfer */
    H5P_CLS_FMNT,       /* File mount */
    H5P_CLS_TCRT,       /* Datatype creation */
    H5P_CLS_TACC,       /* Datatype access */
    H5P_CLS_ACRT,       /* Attribute creation */
    H5P_CLS_LCRT        /* Link creation */
};

/* Declare a free list to manage the H5P_genclass_t struct */
H5FL_DEFINE_STATIC(H5P_genclass_t);

/* Declare a free list to manage the H5P_genprop_t struct */
H5FL_DEFINE_STATIC(H5P_genprop_t);

/* Declare a free list to manage the H5P_genplist_t struct */
H5FL_DEFINE_STATIC(H5P_genplist_t);


/*--------------------------------------------------------------------------
 NAME
    H5P_do_prop_cb1
 PURPOSE
    Internal routine to call a property list callback routine and update
    the property list accordingly.
 USAGE
    herr_t H5P_do_prop_cb1(slist,prop,cb)
        H5SL_t *slist;          IN/OUT: Skip list to hold changed properties
        H5P_genprop_t *prop;    IN: Property to call callback for
        H5P_prp_cb1_t *cb;      IN: Callback routine to call
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Calls the callback routine passed in.  If the callback routine changes
    the property value, then the property is duplicated and added to skip list.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5P_do_prop_cb1(H5SL_t *slist, H5P_genprop_t *prop, H5P_prp_cb1_t cb)
{
    void *tmp_value=NULL;       /* Temporary value buffer */
    H5P_genprop_t *pcopy=NULL;  /* Copy of property to insert into skip list */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_do_prop_cb1)

    /* Allocate space for a temporary copy of the property value */
    if(NULL == (tmp_value = H5MM_malloc(prop->size)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for temporary property value")
    HDmemcpy(tmp_value,prop->value,prop->size);

    /* Call "type 1" callback ('create', 'copy' or 'close') */
    if(cb(prop->name,prop->size,tmp_value) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL,"Property callback failed")

    /* Check if the property value changed */
    if((prop->cmp)(tmp_value,prop->value,prop->size)) {
        /* Make a copy of the class's property */
        if((pcopy=H5P_dup_prop(prop,H5P_PROP_WITHIN_LIST)) == NULL)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, FAIL,"Can't copy property")

        /* Copy the changed value into the new property */
        HDmemcpy(pcopy->value,tmp_value,prop->size);

        /* Insert the changed property into the property list */
        if(H5P_add_prop(slist,pcopy) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL,"Can't insert property into skip list")
    } /* end if */

done:
    /* Release the temporary value buffer */
    if(tmp_value!=NULL)
        H5MM_xfree(tmp_value);

    /* Cleanup on failure */
    if(ret_value<0) {
        if(pcopy!=NULL)
            H5P_free_prop(pcopy);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P_do_prop_cb1() */


/*-------------------------------------------------------------------------
 * Function:	H5P_init
 *
 * Purpose:	Initialize the interface from some other layer.
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Saturday, March 4, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5P_init(void)
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5P_init, FAIL)
    /* FUNC_ENTER() does all the work */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P_init() */


/*--------------------------------------------------------------------------
NAME
   H5P_init_interface -- Initialize interface-specific information
USAGE
    herr_t H5P_init_interface()
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.
--------------------------------------------------------------------------*/
static herr_t
H5P_init_interface(void)
{
    size_t tot_init;                    /* Total # of classes initialized */
    size_t pass_init;                   /* # of classes initialized in each pass */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_init_interface)

    /*
     * Initialize the Generic Property class & object groups.
     */
    if(H5I_register_type(H5I_GENPROP_CLS, (size_t)H5I_GENPROPCLS_HASHSIZE, 0, (H5I_free_t)H5P_close_class) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTINIT, FAIL, "unable to initialize ID group")
    if(H5I_register_type(H5I_GENPROP_LST, (size_t)H5I_GENPROPOBJ_HASHSIZE, 0, (H5I_free_t)H5P_close) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTINIT, FAIL, "unable to initialize ID group")

    /* Repeatedly pass over the list of property list classes for the library,
     *  initializing each class if it's parent class is initialized, until no
     *  more progress is made.
     */
    tot_init = 0;
    do {
        size_t u;                           /* Local index variable */

        /* Reset pass initialization counter */
        pass_init = 0;

        /* Make a pass over all the library's property list classes */
        for(u = 0; u < NELMTS(init_class); u++) {
            H5P_libclass_t const *lib_class = init_class[u]; /* Current class to operate on */

            /* Check if the current class hasn't been initialized and can be now */
            HDassert(lib_class->class_id);
            if(*lib_class->class_id == (-1) && (lib_class->par_class_id == NULL
                        || *lib_class->par_class_id > 0)) {
                H5P_genclass_t *par_pclass = NULL;   /* Parent class of new class */
                H5P_genclass_t *new_pclass;          /* New property list class created */

                /* Sanity check - only the root class is not allowed to have a parent class */
                HDassert(lib_class->par_class_id || lib_class == H5P_CLS_ROOT);

                /* Check for parent class */
                if(lib_class->par_class_id) {
                    /* Get the pointer to the parent class */
                    if(NULL == (par_pclass = (H5P_genclass_t *)H5I_object(*lib_class->par_class_id)))
                        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list class")
                } /* end if */

                /* Allocate the new class */
                if(NULL == (new_pclass = H5P_create_class(par_pclass, lib_class->name, 1, lib_class->create_func, lib_class->create_data, lib_class->copy_func, lib_class->copy_data, lib_class->close_func, lib_class->close_data)))
                    HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "class initialization failed")

                /* Call routine to register properties for class */
                if(lib_class->reg_prop_func && (*lib_class->reg_prop_func)(new_pclass) < 0)
                    HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "can't register properties")

                /* Register the new class */
                if((*lib_class->class_id = H5I_register(H5I_GENPROP_CLS, new_pclass, FALSE)) < 0)
                    HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "can't register property list class")

                /* Only register the default property list if it hasn't been created yet */
                if(lib_class->def_plist_id && *lib_class->def_plist_id == (-1)) {
                    /* Register the default property list for the new class*/
                    if((*lib_class->def_plist_id = H5P_create_id(new_pclass, FALSE)) < 0)
                         HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "can't register default property list for class")
                } /* end if */

                /* Increment class initialization counters */
                pass_init++;
                tot_init++;
            } /* end if */
        } /* end for */
    } while(pass_init > 0);

    /* Verify that all classes were initialized */
    HDassert(tot_init == NELMTS(init_class));

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P_init_interface() */


/*--------------------------------------------------------------------------
 NAME
    H5P_term_interface
 PURPOSE
    Terminate various H5P objects
 USAGE
    void H5P_term_interface()
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Release the atom group and any other resources allocated.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
     Can't report errors...
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5P_term_interface(void)
{
    int	nlist=0;
    int	nclass=0;
    int	n=0;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_term_interface)

    if(H5_interface_initialize_g) {
        /* Destroy HDF5 library property classes & lists */

        /* Check if there are any open property list classes or lists */
        nclass = H5I_nmembers(H5I_GENPROP_CLS);
        nlist = H5I_nmembers(H5I_GENPROP_LST);
        n=nclass+nlist;

        /* If there are any open classes or groups, attempt to get rid of them. */
        if(n) {
            /* Clear the lists */
            if(nlist>0) {
                H5I_clear_type(H5I_GENPROP_LST, FALSE, FALSE);

                /* Reset the default property lists, if they've been closed */
                if(H5I_nmembers(H5I_GENPROP_LST)==0) {
                    H5P_LST_FILE_CREATE_g =
                        H5P_LST_FILE_ACCESS_g =
                        H5P_LST_DATASET_CREATE_g =
                        H5P_LST_DATASET_ACCESS_g =
                        H5P_LST_DATASET_XFER_g =
                        H5P_LST_GROUP_CREATE_g =
                        H5P_LST_GROUP_ACCESS_g =
                        H5P_LST_DATATYPE_CREATE_g =
                        H5P_LST_DATATYPE_ACCESS_g =
                        H5P_LST_ATTRIBUTE_CREATE_g =
                        H5P_LST_OBJECT_COPY_g =
                        H5P_LST_LINK_CREATE_g =
                        H5P_LST_LINK_ACCESS_g =
                        H5P_LST_FILE_MOUNT_g = (-1);
                } /* end if */
            } /* end if */

            /* Only attempt to close the classes after all the lists are closed */
            if(nlist==0 && nclass>0) {
                H5I_clear_type(H5I_GENPROP_CLS, FALSE, FALSE);

                /* Reset the default property lists, if they've been closed */
                if(H5I_nmembers(H5I_GENPROP_CLS)==0) {
                        H5P_CLS_ROOT_g =
                        H5P_CLS_OBJECT_CREATE_g =
                        H5P_CLS_FILE_CREATE_g =
                        H5P_CLS_FILE_ACCESS_g =
                        H5P_CLS_DATASET_CREATE_g =
                        H5P_CLS_DATASET_ACCESS_g =
                        H5P_CLS_DATASET_XFER_g =
                        H5P_CLS_GROUP_CREATE_g =
                        H5P_CLS_GROUP_ACCESS_g =
                        H5P_CLS_DATATYPE_CREATE_g =
                        H5P_CLS_DATATYPE_ACCESS_g =
                        H5P_CLS_STRING_CREATE_g =
                        H5P_CLS_ATTRIBUTE_CREATE_g =
                        H5P_CLS_OBJECT_COPY_g =
                        H5P_CLS_LINK_CREATE_g =
                        H5P_CLS_LINK_ACCESS_g =
                        H5P_CLS_FILE_MOUNT_g = (-1);
                } /* end if */
            } /* end if */
        } else {
            H5I_dec_type_ref(H5I_GENPROP_LST);
            n++; /*H5I*/
            H5I_dec_type_ref(H5I_GENPROP_CLS);
            n++; /*H5I*/

            H5_interface_initialize_g = 0;
        }
    }
    FUNC_LEAVE_NOAPI(n)
}


/*--------------------------------------------------------------------------
 NAME
    H5P_copy_pclass
 PURPOSE
    Internal routine to copy a generic property class
 USAGE
    hid_t H5P_copy_pclass(pclass)
        H5P_genclass_t *pclass;      IN: Property class to copy
 RETURNS
    Success: valid property class ID on success (non-negative)
    Failure: negative
 DESCRIPTION
    Copy a property class and return the ID.  This routine does not make
    any callbacks.  (They are only make when operating on property lists).

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5P_genclass_t *
H5P_copy_pclass(H5P_genclass_t *pclass)
{
    H5P_genclass_t *new_pclass = NULL;  /* Property list class copied */
    H5P_genprop_t *pcopy;               /* Copy of property to insert into class */
    H5P_genclass_t *ret_value=NULL;     /* return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_copy_pclass)

    HDassert(pclass);

    /*
     * Create new property class object
     */

    /* Create the new property list class */
    if(NULL==(new_pclass=H5P_create_class(pclass->parent, pclass->name, 0, pclass->create_func, pclass->create_data, pclass->copy_func, pclass->copy_data, pclass->close_func, pclass->close_data)))
        HGOTO_ERROR(H5E_PLIST, H5E_CANTCREATE, NULL, "unable to create property list class")

    /* Copy the properties registered for this class */
    if(pclass->nprops>0) {
        H5SL_node_t *curr_node;   /* Current node in skip list */

        /* Walk through the properties in the old class */
        curr_node=H5SL_first(pclass->props);
        while(curr_node!=NULL) {
            /* Make a copy of the class's property */
            if(NULL == (pcopy = H5P_dup_prop((H5P_genprop_t *)H5SL_item(curr_node), H5P_PROP_WITHIN_CLASS)))
                HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, NULL,"Can't copy property")

            /* Insert the initialized property into the property list */
            if(H5P_add_prop(new_pclass->props,pcopy) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, NULL,"Can't insert property into class")

            /* Increment property count for class */
            new_pclass->nprops++;

            /* Get the next property node in the list */
            curr_node=H5SL_next(curr_node);
        } /* end while */
    } /* end if */

    /* Set the return value */
    ret_value=new_pclass;

done:
    if(ret_value==NULL && new_pclass)
        H5P_close_class(new_pclass);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_copy_pclass() */


/*--------------------------------------------------------------------------
 NAME
    H5P_copy_plist
 PURPOSE
    Internal routine to copy a generic property list
 USAGE
        hid_t H5P_copy_plist(old_plist_id)
            hid_t old_plist_id;             IN: Property list ID to copy
 RETURNS
    Success: valid property list ID on success (non-negative)
    Failure: negative
 DESCRIPTION
    Copy a property list and return the ID.  This routine calls the
    class 'copy' callback after any property 'copy' callbacks are called
    (assuming all property 'copy' callbacks return successfully).

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hid_t
H5P_copy_plist(const H5P_genplist_t *old_plist, hbool_t app_ref)
{
    H5P_genclass_t *tclass;     /* Temporary class pointer */
    H5P_genplist_t *new_plist=NULL;  /* New property list generated from copy */
    H5P_genprop_t *tmp;         /* Temporary pointer to properties */
    H5P_genprop_t *new_prop;    /* New property created for copy */
    hid_t new_plist_id;         /* Property list ID of new list created */
    H5SL_node_t *curr_node;     /* Current node in skip list */
    H5SL_t *seen=NULL;          /* Skip list containing properties already seen */
    size_t nseen;               /* Number of items 'seen' */
    hbool_t has_parent_class;   /* Flag to indicate that this property list's class has a parent */
    hid_t ret_value=FAIL;       /* return value */

    FUNC_ENTER_NOAPI(H5P_copy_plist, FAIL)

    HDassert(old_plist);

    /*
     * Create new property list object
     */

    /* Allocate room for the property list */
    if(NULL==(new_plist = H5FL_CALLOC(H5P_genplist_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,"memory allocation failed")

    /* Set class state */
    new_plist->pclass = old_plist->pclass;
    new_plist->nprops = 0;      /* Initially the plist has the same number of properties as the class */
    new_plist->class_init = FALSE;  /* Initially, wait until the class callback finishes to set */

    /* Initialize the skip list to hold the changed properties */
    if((new_plist->props = H5SL_create(H5SL_TYPE_STR)) == NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTCREATE,FAIL,"can't create skip list for changed properties")

    /* Create the skip list for deleted properties */
    if((new_plist->del = H5SL_create(H5SL_TYPE_STR)) == NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTCREATE,FAIL,"can't create skip list for deleted properties")

    /* Create the skip list to hold names of properties already seen
     * (This prevents a property in the class hierarchy from having it's
     * 'create' callback called, if a property in the class hierarchy has
     * already been seen)
     */
    if((seen = H5SL_create(H5SL_TYPE_STR))== NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTCREATE,FAIL,"can't create skip list for seen properties")
    nseen = 0;

    /* Cycle through the deleted properties & copy them into the new list's deleted section */
    if(H5SL_count(old_plist->del)>0) {
        curr_node=H5SL_first(old_plist->del);
        while(curr_node) {
            char *new_name;   /* Pointer to new name */

            /* Duplicate string for insertion into new deleted property skip list */
            if((new_name=H5MM_xstrdup((char *)H5SL_item(curr_node))) == NULL)
                HGOTO_ERROR(H5E_RESOURCE,H5E_NOSPACE,FAIL,"memory allocation failed")

            /* Insert property name into deleted list */
            if(H5SL_insert(new_plist->del,new_name,new_name) < 0)
                HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into deleted skip list")

            /* Add property name to "seen" list */
            if(H5SL_insert(seen,new_name,new_name) < 0)
                HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into seen skip list")
            nseen++;

            /* Get the next property node in the skip list */
            curr_node=H5SL_next(curr_node);
        } /* end while */
    } /* end if */

    /* Cycle through the properties and copy them also */
    if(H5SL_count(old_plist->props)>0) {
        curr_node=H5SL_first(old_plist->props);
        while(curr_node) {
            /* Get a pointer to the node's property */
            tmp = (H5P_genprop_t *)H5SL_item(curr_node);

            /* Make a copy of the list's property */
            if(NULL == (new_prop = H5P_dup_prop(tmp, H5P_PROP_WITHIN_LIST)))
                HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, FAIL,"Can't copy property")

            /* Call property copy callback, if it exists */
            if(new_prop->copy) {
                if((new_prop->copy)(new_prop->name,new_prop->size,new_prop->value) < 0) {
                    H5P_free_prop(new_prop);
                    HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, FAIL,"Can't copy property")
                } /* end if */
            } /* end if */

            /* Insert the initialized property into the property list */
            if(H5P_add_prop(new_plist->props,new_prop) < 0) {
                H5P_free_prop(new_prop);
                HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL,"Can't insert property into list")
            } /* end if */

            /* Add property name to "seen" list */
            if(H5SL_insert(seen,new_prop->name,new_prop->name) < 0)
                HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into seen skip list")
            nseen++;

            /* Increment the number of properties in list */
            new_plist->nprops++;

            /* Get the next property node in the skip list */
            curr_node=H5SL_next(curr_node);
        } /* end while */
    } /* end if */

    /*
     * Check for copying class properties (up through list of parent classes also),
     * initialize each with default value & make property 'copy' callback.
     */
    tclass=old_plist->pclass;
    has_parent_class = (hbool_t)(tclass != NULL && tclass->parent != NULL && tclass->parent->nprops > 0);
    while(tclass!=NULL) {
        if(tclass->nprops>0) {
            /* Walk through the properties in the old class */
            curr_node=H5SL_first(tclass->props);
            while(curr_node!=NULL) {
                /* Get pointer to property from node */
                tmp = (H5P_genprop_t *)H5SL_item(curr_node);

                /* Only "copy" properties we haven't seen before */
                if(nseen==0 || H5SL_search(seen,tmp->name) == NULL) {
                    /* Call property creation callback, if it exists */
                    if(tmp->copy) {
                        /* Call the callback & insert changed value into skip list (if necessary) */
                        if(H5P_do_prop_cb1(new_plist->props,tmp,tmp->copy) < 0)
                            HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, FAIL,"Can't create property")
                    } /* end if */

                    /* Add property name to "seen" list, if we have other classes to work on */
                    if(has_parent_class) {
                        if(H5SL_insert(seen,tmp->name,tmp->name) < 0)
                            HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into seen skip list")
                        nseen++;
                    } /* end if */

                    /* Increment the number of properties in list */
                    new_plist->nprops++;
                } /* end if */

                /* Get the next property node in the skip list */
                curr_node=H5SL_next(curr_node);
            } /* end while */
        } /* end if */

        /* Go up to parent class */
        tclass=tclass->parent;
    } /* end while */

    /* Increment the number of property lists derived from class */
    if(H5P_access_class(new_plist->pclass, H5P_MOD_INC_LST) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "Can't increment class ref count")

    /* Get an atom for the property list */
    if((new_plist_id = H5I_register(H5I_GENPROP_LST, new_plist, app_ref)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to atomize property list")

    /* Save the property list ID in the property list struct, for use in the property class's 'close' callback */
    new_plist->plist_id=new_plist_id;

    /* Call the class callback (if it exists) now that we have the property list ID
     * (up through chain of parent classes also)
     */
    tclass = new_plist->pclass;
    while(NULL != tclass) {
        if(NULL != tclass->copy_func) {
            if((tclass->copy_func)(new_plist_id, old_plist->plist_id, old_plist->pclass->copy_data) < 0) {
                /* Delete ID, ignore return value */
                H5I_remove(new_plist_id);
                HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL,"Can't initialize property")
            } /* end if */
        } /* end if */

        /* Go up to parent class */
        tclass = tclass->parent;
    } /* end while */

    /* Set the class initialization flag */
    new_plist->class_init = TRUE;

    /* Set the return value */
    ret_value=new_plist_id;

done:
    /* Release the list of 'seen' properties */
    if(seen!=NULL)
        H5SL_close(seen);

    if(ret_value<0 && new_plist)
        H5P_close(new_plist);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_copy_plist() */


/*--------------------------------------------------------------------------
 NAME
    H5P_dup_prop
 PURPOSE
    Internal routine to duplicate a property
 USAGE
    H5P_genprop_t *H5P_dup_prop(oprop)
        H5P_genprop_t *oprop;   IN: Pointer to property to copy
        H5P_prop_within_t type; IN: Type of object the property will be inserted into
 RETURNS
    Returns a pointer to the newly created duplicate of a property on success,
        NULL on failure.
 DESCRIPTION
    Allocates memory and copies property information into a new property object.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static H5P_genprop_t *
H5P_dup_prop(H5P_genprop_t *oprop, H5P_prop_within_t type)
{
    H5P_genprop_t *prop = NULL;      /* Pointer to new property copied */
    H5P_genprop_t *ret_value;        /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_dup_prop)

    HDassert(oprop);
    HDassert(type != H5P_PROP_WITHIN_UNKNOWN);

    /* Allocate the new property */
    if(NULL == (prop = H5FL_MALLOC(H5P_genprop_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Copy basic property information */
    HDmemcpy(prop, oprop, sizeof(H5P_genprop_t));

    /* Check if we should duplicate the name or share it */

    /* Duplicating property for a class */
    if(type == H5P_PROP_WITHIN_CLASS) {
        HDassert(oprop->type == H5P_PROP_WITHIN_CLASS);
        HDassert(oprop->shared_name == FALSE);

        /* Duplicate name */
        prop->name = H5MM_xstrdup(oprop->name);
    } /* end if */
    /* Duplicating property for a list */
    else {
        /* Check if we are duplicating a property from a list or a class */

        /* Duplicating a property from a list */
        if(oprop->type == H5P_PROP_WITHIN_LIST) {
            /* If the old property's name wasn't shared, we have to copy it here also */
            if(!oprop->shared_name)
                prop->name = H5MM_xstrdup(oprop->name);
        } /* end if */
        /* Duplicating a property from a class */
        else {
            HDassert(oprop->type == H5P_PROP_WITHIN_CLASS);
            HDassert(oprop->shared_name == FALSE);

            /* Share the name */
            prop->shared_name = TRUE;

            /* Set the type */
            prop->type = type;
        } /* end else */
    } /* end else */

    /* Duplicate current value, if it exists */
    if(oprop->value != NULL) {
        HDassert(prop->size > 0);
        if(NULL == (prop->value = H5MM_malloc(prop->size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
        HDmemcpy(prop->value, oprop->value, prop->size);
    } /* end if */

    /* Set return value */
    ret_value = prop;

done:
    /* Free any resources allocated */
    if(ret_value == NULL) {
        if(prop != NULL) {
            if(prop->name != NULL)
                H5MM_xfree(prop->name);
            if(prop->value != NULL)
                H5MM_xfree(prop->value);
            prop = H5FL_FREE(H5P_genprop_t, prop);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_dup_prop() */


/*--------------------------------------------------------------------------
 NAME
    H5P_create_prop
 PURPOSE
    Internal routine to create a new property
 USAGE
    H5P_genprop_t *H5P_create_prop(name,size,type,value,prp_create,prp_set,
            prp_get,prp_delete,prp_close)
        const char *name;       IN: Name of property to register
        size_t size;            IN: Size of property in bytes
        H5P_prop_within_t type; IN: Type of object the property will be inserted into
        void *value;            IN: Pointer to buffer containing value for property
        H5P_prp_create_func_t prp_create;   IN: Function pointer to property
                                    creation callback
        H5P_prp_set_func_t prp_set; IN: Function pointer to property set callback
        H5P_prp_get_func_t prp_get; IN: Function pointer to property get callback
        H5P_prp_delete_func_t prp_delete; IN: Function pointer to property delete callback
        H5P_prp_copy_func_t prp_copy; IN: Function pointer to property copy callback
        H5P_prp_compare_func_t prp_cmp; IN: Function pointer to property compare callback
        H5P_prp_close_func_t prp_close; IN: Function pointer to property close
                                    callback
 RETURNS
    Returns a pointer to the newly created property on success,
        NULL on failure.
 DESCRIPTION
    Allocates memory and copies property information into a new property object.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static H5P_genprop_t *
H5P_create_prop(const char *name, size_t size, H5P_prop_within_t type,
    const void *value,
    H5P_prp_create_func_t prp_create, H5P_prp_set_func_t prp_set,
    H5P_prp_get_func_t prp_get, H5P_prp_delete_func_t prp_delete,
    H5P_prp_copy_func_t prp_copy, H5P_prp_compare_func_t prp_cmp,
    H5P_prp_close_func_t prp_close)
{
    H5P_genprop_t *prop=NULL;        /* Pointer to new property copied */
    H5P_genprop_t *ret_value;        /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_create_prop)

    HDassert(name);
    HDassert((size>0 && value!=NULL) || (size==0));
    HDassert(type!=H5P_PROP_WITHIN_UNKNOWN);

    /* Allocate the new property */
    if(NULL==(prop = H5FL_MALLOC (H5P_genprop_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Set the property initial values */
    prop->name = H5MM_xstrdup(name); /* Duplicate name */
    prop->shared_name = FALSE;
    prop->size = size;
    prop->type = type;

    /* Duplicate value, if it exists */
    if(value!=NULL) {
        if(NULL==(prop->value = H5MM_malloc (prop->size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
        HDmemcpy(prop->value,value,prop->size);
    } /* end if */
    else
        prop->value=NULL;

    /* Set the function pointers */
    prop->create=prp_create;
    prop->set=prp_set;
    prop->get=prp_get;
    prop->del=prp_delete;
    prop->copy=prp_copy;
    /* Use custom comparison routine if available, otherwise default to memcmp() */
    if(prp_cmp!=NULL)
        prop->cmp=prp_cmp;
    else
        prop->cmp=&memcmp;
    prop->close=prp_close;

    /* Set return value */
    ret_value=prop;

done:
    /* Free any resources allocated */
    if(ret_value==NULL) {
        if(prop!=NULL) {
            if(prop->name!=NULL)
                H5MM_xfree(prop->name);
            if(prop->value!=NULL)
                H5MM_xfree(prop->value);
            prop = H5FL_FREE(H5P_genprop_t, prop);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_create_prop() */


/*--------------------------------------------------------------------------
 NAME
    H5P_add_prop
 PURPOSE
    Internal routine to insert a property into a property skip list
 USAGE
    herr_t H5P_add_prop(slist, prop)
        H5SL_t *slist;          IN/OUT: Pointer to skip list of properties
        H5P_genprop_t *prop;    IN: Pointer to property to insert
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Inserts a property into a skip list of properties.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_add_prop(H5SL_t *slist, H5P_genprop_t *prop)
{
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5P_add_prop, FAIL)

    HDassert(slist);
    HDassert(prop);
    HDassert(prop->type != H5P_PROP_WITHIN_UNKNOWN);

    /* Insert property into skip list */
    if(H5SL_insert(slist, prop, prop->name) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into skip list")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5P_add_prop() */


/*--------------------------------------------------------------------------
 NAME
    H5P_find_prop_plist
 PURPOSE
    Internal routine to check for a property in a property list's skip list
 USAGE
    H5P_genprop_t *H5P_find_prop(plist, name)
        H5P_genplist_t *plist;  IN: Pointer to property list to check
        const char *name;       IN: Name of property to check for
 RETURNS
    Returns pointer to property on success, NULL on failure.
 DESCRIPTION
    Checks for a property in a property list's skip list of properties.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static H5P_genprop_t *
H5P_find_prop_plist(H5P_genplist_t *plist, const char *name)
{
    H5P_genprop_t *ret_value;   /* Property pointer return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_find_prop_plist)

    HDassert(plist);
    HDassert(name);

    /* Check if the property has been deleted from list */
    if(H5SL_search(plist->del,name) != NULL) {
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, NULL, "can't find property in skip list")
    } /* end if */
    else {
        /* Get the property data from the skip list */
        if(NULL == (ret_value = (H5P_genprop_t *)H5SL_search(plist->props, name))) {
            H5P_genclass_t *tclass;     /* Temporary class pointer */

            /* Couldn't find property in list itself, start searching through class info */
            tclass = plist->pclass;
            while(tclass != NULL) {
                /* Find the property in the class */
                if(NULL != (ret_value = (H5P_genprop_t *)H5SL_search(tclass->props, name)))
                    /* Got pointer to property - leave now */
                    break;

                /* Go up to parent class */
                tclass = tclass->parent;
            } /* end while */

            /* Check if we haven't found the property */
            if(ret_value == NULL)
                HGOTO_ERROR(H5E_PLIST,H5E_NOTFOUND,NULL,"can't find property in skip list")
        } /* end else */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_find_prop_plist() */


/*--------------------------------------------------------------------------
 NAME
    H5P_find_prop_pclass
 PURPOSE
    Internal routine to check for a property in a class skip list
 USAGE
    H5P_genprop_t *H5P_find_prop_class(pclass, name)
        H5P_genclass *pclass;   IN: Pointer generic property class to check
        const char *name;       IN: Name of property to check for
 RETURNS
    Returns pointer to property on success, NULL on failure.
 DESCRIPTION
    Checks for a property in a class's skip list of properties.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static H5P_genprop_t *
H5P_find_prop_pclass(H5P_genclass_t *pclass, const char *name)
{
    H5P_genprop_t *ret_value;   /* Property pointer return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_find_prop_pclass)

    HDassert(pclass);
    HDassert(name);

    /* Get the property from the skip list */
    if(NULL == (ret_value = (H5P_genprop_t *)H5SL_search(pclass->props, name)))
        HGOTO_ERROR(H5E_PLIST,H5E_NOTFOUND,NULL,"can't find property in skip list")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_find_prop_pclass() */


/*--------------------------------------------------------------------------
 NAME
    H5P_free_prop
 PURPOSE
    Internal routine to destroy a property node
 USAGE
    herr_t H5P_free_prop(prop)
        H5P_genprop_t *prop;    IN: Pointer to property to destroy
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Releases all the memory for a property list.  Does _not_ call the
    properties 'close' callback, that should already have been done.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5P_free_prop(H5P_genprop_t *prop)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_free_prop)

    HDassert(prop);

    /* Release the property value if it exists */
    if(prop->value)
        H5MM_xfree(prop->value);

    /* Only free the name if we own it */
    if(!prop->shared_name)
        H5MM_xfree(prop->name);

    prop = H5FL_FREE(H5P_genprop_t, prop);

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5P_free_prop() */


/*--------------------------------------------------------------------------
 NAME
    H5P_free_prop_cb
 PURPOSE
    Internal routine to properties from a property skip list
 USAGE
    herr_t H5P_free_prop_cb(item, key, op_data)
        void *item;             IN/OUT: Pointer to property
        void *key;              IN/OUT: Pointer to property key
        void *_make_cb;         IN: Whether to make property callbacks or not
 RETURNS
    Returns zero on success, negative on failure.
 DESCRIPTION
        Calls the property 'close' callback for a property & frees property
    info.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5P_free_prop_cb(void *item, void UNUSED *key, void *op_data)
{
    H5P_genprop_t *tprop=(H5P_genprop_t *)item; /* Temporary pointer to property */
    hbool_t make_cb = *(hbool_t *)op_data;      /* Whether to make property 'close' callback */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_free_prop_cb)

    HDassert(tprop);

    /* Call the close callback and ignore the return value, there's nothing we can do about it */
    if(make_cb && tprop->close != NULL)
        (tprop->close)(tprop->name, tprop->size, tprop->value);

    /* Free the property, ignoring return value, nothing we can do */
    H5P_free_prop(tprop);

    FUNC_LEAVE_NOAPI(0)
}   /* H5P_free_prop_cb() */


/*--------------------------------------------------------------------------
 NAME
    H5P_free_del_name_cb
 PURPOSE
    Internal routine to free 'deleted' property name
 USAGE
    herr_t H5P_free_del_name_cb(item, key, op_data)
        void *item;             IN/OUT: Pointer to deleted name
        void *key;              IN/OUT: Pointer to key
        void *op_data;          IN: Operator callback data (unused)
 RETURNS
    Returns zero on success, negative on failure.
 DESCRIPTION
    Frees the deleted property name
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5P_free_del_name_cb(void *item, void UNUSED *key, void UNUSED *op_data)
{
    char *del_name=(char *)item;       /* Temporary pointer to deleted name */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_free_del_name_cb)

    HDassert(del_name);

    /* Free the name */
    H5MM_xfree(del_name);

    FUNC_LEAVE_NOAPI(0)
}   /* H5P_free_del_name_cb() */


/*--------------------------------------------------------------------------
 NAME
    H5P_access_class
 PURPOSE
    Internal routine to increment or decrement list & class dependancies on a
        property list class
 USAGE
    herr_t H5P_access_class(pclass,mod)
        H5P_genclass_t *pclass;     IN: Pointer to class to modify
        H5P_class_mod_t mod;        IN: Type of modification to class
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Increment/Decrement the class or list dependancies for a given class.
    This routine is the final arbiter on decisions about actually releasing a
    class in memory, such action is only taken when the reference counts for
    both dependent classes & lists reach zero.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_access_class(H5P_genclass_t *pclass, H5P_class_mod_t mod)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_access_class)

    HDassert(pclass);
    HDassert(mod > H5P_MOD_ERR && mod < H5P_MOD_MAX);

    switch(mod) {
        case H5P_MOD_INC_CLS:        /* Increment the dependant class count*/
            pclass->classes++;
            break;

        case H5P_MOD_DEC_CLS:        /* Decrement the dependant class count*/
            pclass->classes--;
            break;

        case H5P_MOD_INC_LST:        /* Increment the dependant list count*/
            pclass->plists++;
            break;

        case H5P_MOD_DEC_LST:        /* Decrement the dependant list count*/
            pclass->plists--;
            break;

        case H5P_MOD_INC_REF:        /* Increment the ID reference count*/
            /* Reset the deleted flag if incrementing the reference count */
            if(pclass->deleted)
                pclass->deleted = FALSE;
            pclass->ref_count++;
            break;

        case H5P_MOD_DEC_REF:        /* Decrement the ID reference count*/
            pclass->ref_count--;

            /* Mark the class object as deleted if reference count drops to zero */
            if(pclass->ref_count == 0)
                pclass->deleted = TRUE;
            break;

        case H5P_MOD_ERR:
        case H5P_MOD_MAX:
        default:
            HDassert(0 && "Invalid H5P class modification");
    } /* end switch */

    /* Check if we can release the class information now */
    if(pclass->deleted && pclass->plists == 0 && pclass->classes == 0) {
        H5P_genclass_t *par_class = pclass->parent;       /* Pointer to class's parent */

        HDassert(pclass->name);
        H5MM_xfree(pclass->name);

        /* Free the class properties without making callbacks */
        if(pclass->props) {
            hbool_t make_cb = FALSE;

            H5SL_destroy(pclass->props, H5P_free_prop_cb, &make_cb);
        } /* end if */

        pclass = H5FL_FREE(H5P_genclass_t, pclass);

        /* Reduce the number of dependent classes on parent class also */
        if(par_class != NULL)
            H5P_access_class(par_class, H5P_MOD_DEC_CLS);
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5P_access_class() */


/*--------------------------------------------------------------------------
 NAME
    H5P_check_class
 PURPOSE
    Internal callback routine to check for duplicated names in parent class.
 USAGE
    int H5P_check_class(obj, id, key)
        H5P_genclass_t *obj;    IN: Pointer to class
        hid_t id;               IN: ID of object being looked at
        const void *key;        IN: Pointer to information used to compare
                                    classes.
 RETURNS
    Returns >0 on match, 0 on no match and <0 on failure.
 DESCRIPTION
    Checks whether a property list class has the same parent and name as a
    new class being created.  This is a callback routine for H5I_search()
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static int
H5P_check_class(void *_obj, hid_t UNUSED id, void *_key)
{
    H5P_genclass_t *obj=(H5P_genclass_t *)_obj; /* Pointer to the class for this ID */
    const H5P_check_class_t *key=(const H5P_check_class_t *)_key; /* Pointer to key information for comparison */
    int ret_value=0;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_check_class)

    HDassert(obj);
    HDassert(H5I_GENPROP_CLS==H5I_get_type(id));
    HDassert(key);

    /* Check if the class object has the same parent as the new class */
    if(obj->parent==key->parent) {
        /* Check if they have the same name */
        if(HDstrcmp(obj->name,key->name)==0)
            ret_value=1;        /* Indicate a match */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P_check_class() */


/*--------------------------------------------------------------------------
 NAME
    H5P_create_class
 PURPOSE
    Internal routine to create a new property list class.
 USAGE
    H5P_genclass_t H5P_create_class(par_class, name, internal,
                cls_create, create_data, cls_close, close_data)
        H5P_genclass_t *par_class;  IN: Pointer to parent class
        const char *name;       IN: Name of class we are creating
        hbool_t internal;       IN: Whether this is an internal class or not
        H5P_cls_create_func_t;  IN: The callback function to call when each
                                    property list in this class is created.
        void *create_data;      IN: Pointer to user data to pass along to class
                                    creation callback.
        H5P_cls_copy_func_t;    IN: The callback function to call when each
                                    property list in this class is copied.
        void *copy_data;        IN: Pointer to user data to pass along to class
                                    copy callback.
        H5P_cls_close_func_t;   IN: The callback function to call when each
                                    property list in this class is closed.
        void *close_data;       IN: Pointer to user data to pass along to class
                                    close callback.
 RETURNS
    Returns a pointer to the newly created property list class on success,
        NULL on failure.
 DESCRIPTION
    Allocates memory and attaches a class to the property list class hierarchy.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5P_genclass_t *
H5P_create_class(H5P_genclass_t *par_class, const char *name, hbool_t internal,
    H5P_cls_create_func_t cls_create, void *create_data,
    H5P_cls_copy_func_t cls_copy, void *copy_data,
    H5P_cls_close_func_t cls_close, void *close_data)
{
    H5P_genclass_t *pclass=NULL;   /* Property list class created */
    H5P_genclass_t *ret_value;     /* return value */

    FUNC_ENTER_NOAPI(H5P_create_class, NULL)

    HDassert(name);
    /* Allow internal classes to break some rules */
    /* (This allows the root of the tree to be created with this routine -QAK) */
    if(!internal)
        HDassert(par_class);

    /* Allocate room for the class */
    if(NULL == (pclass = H5FL_CALLOC(H5P_genclass_t)))
        HGOTO_ERROR(H5E_PLIST, H5E_CANTALLOC, NULL, "propery list class allocation failed")

    /* Set class state */
    pclass->parent = par_class;
    if(NULL == (pclass->name = H5MM_xstrdup(name)))
        HGOTO_ERROR(H5E_PLIST, H5E_CANTALLOC, NULL, "propery list class name allocation failed")
    pclass->nprops = 0;     /* Classes are created without properties initially */
    pclass->plists = 0;     /* No properties lists of this class yet */
    pclass->classes = 0;    /* No classes derived from this class yet */
    pclass->ref_count = 1;  /* This is the first reference to the new class */
    pclass->internal = internal;
    pclass->deleted = FALSE;    /* Not deleted yet... :-) */
    pclass->revision = H5P_GET_NEXT_REV;        /* Get a revision number for the class */

    /* Create the skip list for properties */
    if(NULL == (pclass->props = H5SL_create(H5SL_TYPE_STR)))
        HGOTO_ERROR(H5E_PLIST, H5E_CANTCREATE, NULL, "can't create skip list for properties")

    /* Set callback functions and pass-along data */
    pclass->create_func = cls_create;
    pclass->create_data = create_data;
    pclass->copy_func = cls_copy;
    pclass->copy_data = copy_data;
    pclass->close_func = cls_close;
    pclass->close_data = close_data;

    /* Increment parent class's derived class value */
    if(par_class != NULL) {
        if(H5P_access_class(par_class, H5P_MOD_INC_CLS) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, NULL, "Can't increment parent class ref count")
    } /* end if */

    /* Set return value */
    ret_value = pclass;

done:
    /* Free any resources allocated */
    if(ret_value == NULL)
        if(pclass) {
            if(pclass->name)
                H5MM_xfree(pclass->name);
            if(pclass->props) {
                hbool_t make_cb = FALSE;

                H5SL_destroy(pclass->props, H5P_free_prop_cb, &make_cb);
            } /* end if */
            pclass = H5FL_FREE(H5P_genclass_t, pclass);
        } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_create_class() */


/*--------------------------------------------------------------------------
 NAME
    H5P_create
 PURPOSE
    Internal routine to create a new property list of a property list class.
 USAGE
    H5P_genplist_t *H5P_create(class)
        H5P_genclass_t *class;  IN: Property list class create list from
 RETURNS
    Returns a pointer to the newly created property list on success,
        NULL on failure.
 DESCRIPTION
        Creates a property list of a given class.  If a 'create' callback
    exists for the property list class, it is called before the
    property list is passed back to the user.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
        If this routine is called from a library routine other than
    H5P_c, the calling routine is responsible for getting an ID for
    the property list and calling the class 'create' callback (if one exists)
    and also setting the "class_init" flag.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static H5P_genplist_t *
H5P_create(H5P_genclass_t *pclass)
{
    H5P_genclass_t *tclass;         /* Temporary class pointer */
    H5P_genplist_t *plist=NULL;     /* New property list created */
    H5P_genprop_t *tmp;             /* Temporary pointer to parent class properties */
    H5SL_t *seen=NULL;              /* Skip list to hold names of properties already seen */
    H5P_genplist_t *ret_value;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_create)

    HDassert(pclass);

    /*
     * Create new property list object
     */

    /* Allocate room for the property list */
    if(NULL==(plist = H5FL_CALLOC(H5P_genplist_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL,"memory allocation failed")

    /* Set class state */
    plist->pclass = pclass;
    plist->nprops = 0;      /* Initially the plist has the same number of properties as the class */
    plist->class_init = FALSE;  /* Initially, wait until the class callback finishes to set */

    /* Create the skip list for changed properties */
    if((plist->props = H5SL_create(H5SL_TYPE_STR)) == NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTCREATE,NULL,"can't create skip list for changed properties")

    /* Create the skip list for deleted properties */
    if((plist->del = H5SL_create(H5SL_TYPE_STR)) == NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTCREATE,NULL,"can't create skip list for deleted properties")

    /* Create the skip list to hold names of properties already seen
     * (This prevents a property in the class hierarchy from having it's
     * 'create' callback called, if a property in the class hierarchy has
     * already been seen)
     */
    if((seen = H5SL_create(H5SL_TYPE_STR)) == NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTCREATE,NULL,"can't create skip list for seen properties")

    /*
     * Check if we should copy class properties (up through list of parent classes also),
     * initialize each with default value & make property 'create' callback.
     */
    tclass=pclass;
    while(tclass!=NULL) {
        if(tclass->nprops>0) {
            H5SL_node_t *curr_node;   /* Current node in skip list */

            /* Walk through the properties in the old class */
            curr_node=H5SL_first(tclass->props);
            while(curr_node!=NULL) {
                /* Get pointer to property from node */
                tmp = (H5P_genprop_t *)H5SL_item(curr_node);

                /* Only "create" properties we haven't seen before */
                if(H5SL_search(seen,tmp->name) == NULL) {
                    /* Call property creation callback, if it exists */
                    if(tmp->create) {
                        /* Call the callback & insert changed value into skip list (if necessary) */
                        if(H5P_do_prop_cb1(plist->props,tmp,tmp->create) < 0)
                            HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, NULL,"Can't create property")
                    } /* end if */

                    /* Add property name to "seen" list */
                    if(H5SL_insert(seen,tmp->name,tmp->name) < 0)
                        HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,NULL,"can't insert property into seen skip list")

                    /* Increment the number of properties in list */
                    plist->nprops++;
                } /* end if */

                /* Get the next property node in the skip list */
                curr_node=H5SL_next(curr_node);
            } /* end while */
        } /* end if */

        /* Go up to parent class */
        tclass=tclass->parent;
    } /* end while */

    /* Increment the number of property lists derived from class */
    if(H5P_access_class(plist->pclass,H5P_MOD_INC_LST) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, NULL,"Can't increment class ref count")

    /* Set return value */
    ret_value=plist;

done:
    /* Release the skip list of 'seen' properties */
    if(seen!=NULL)
        H5SL_close(seen);

    /* Release resources allocated on failure */
    if(ret_value==NULL) {
        if(plist!=NULL) {
            /* Close & free any changed properties */
            if(plist->props) {
                unsigned make_cb=1;

                H5SL_destroy(plist->props,H5P_free_prop_cb,&make_cb);
            } /* end if */

            /* Close the deleted property skip list */
            if(plist->del)
                H5SL_close(plist->del);

            /* Release the property list itself */
            plist = H5FL_FREE(H5P_genplist_t, plist);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_create() */


/*--------------------------------------------------------------------------
 NAME
    H5P_create_id
 PURPOSE
    Internal routine to create a new property list of a property list class.
 USAGE
    hid_t H5P_create_id(pclass)
        H5P_genclass_t *pclass;       IN: Property list class create list from
 RETURNS
    Returns a valid property list ID on success, FAIL on failure.
 DESCRIPTION
        Creates a property list of a given class.  If a 'create' callback
    exists for the property list class, it is called before the
    property list is passed back to the user.  If 'create' callbacks exist for
    any individual properties in the property list, they are called before the
    class 'create' callback.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hid_t
H5P_create_id(H5P_genclass_t *pclass, hbool_t app_ref)
{
    H5P_genclass_t *tclass;         /* Temporary class pointer */
    H5P_genplist_t *plist = NULL;   /* Property list created */
    hid_t plist_id = FAIL;      /* Property list ID */
    hid_t ret_value;            /* return value */

    FUNC_ENTER_NOAPI(H5P_create_id, FAIL)

    HDassert(pclass);

    /* Create the new property list */
    if((plist=H5P_create(pclass)) == NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTCREATE, FAIL, "unable to create property list")

    /* Get an atom for the property list */
    if((plist_id = H5I_register(H5I_GENPROP_LST, plist, app_ref)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to atomize property list")

    /* Save the property list ID in the property list struct, for use in the property class's 'close' callback */
    plist->plist_id=plist_id;

    /* Call the class callback (if it exists) now that we have the property list ID
     * (up through chain of parent classes also)
     */
    tclass = plist->pclass;
    while(NULL != tclass) {
        if(NULL != tclass->create_func) {
            if((tclass->create_func)(plist_id, tclass->create_data) < 0) {
                /* Delete ID, ignore return value */
                H5I_remove(plist_id);
                HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL,"Can't initialize property")
            } /* end if */
        } /* end if */

        /* Go up to parent class */
        tclass = tclass->parent;
    } /* end while */

    /* Set the class initialization flag */
    plist->class_init = TRUE;

    /* Set the return value */
    ret_value=plist_id;

done:
    if(ret_value<0 && plist)
        H5P_close(plist);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_create_id() */


/*--------------------------------------------------------------------------
 NAME
    H5P_register_real
 PURPOSE
    Internal routine to register a new property in a property list class.
 USAGE
    herr_t H5P_register_real(class, name, size, default, prp_create, prp_set, prp_get, prp_close)
        H5P_genclass_t *class;  IN: Property list class to modify
        const char *name;       IN: Name of property to register
        size_t size;            IN: Size of property in bytes
        void *def_value;        IN: Pointer to buffer containing default value
                                    for property in newly created property lists
        H5P_prp_create_func_t prp_create;   IN: Function pointer to property
                                    creation callback
        H5P_prp_set_func_t prp_set; IN: Function pointer to property set callback
        H5P_prp_get_func_t prp_get; IN: Function pointer to property get callback
        H5P_prp_delete_func_t prp_delete; IN: Function pointer to property delete callback
        H5P_prp_copy_func_t prp_copy; IN: Function pointer to property copy callback
        H5P_prp_compare_func_t prp_cmp; IN: Function pointer to property compare callback
        H5P_prp_close_func_t prp_close; IN: Function pointer to property close
                                    callback
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Registers a new property with a property list class.  The property will
    exist in all property list objects of that class after this routine is
    finished.  The name of the property must not already exist.  The default
    property value must be provided and all new property lists created with this
    property will have the property value set to the default provided.  Any of
    the callback routines may be set to NULL if they are not needed.

        Zero-sized properties are allowed and do not store any data in the
    property list.  These may be used as flags to indicate the presence or
    absence of a particular piece of information.  The 'default' pointer for a
    zero-sized property may be set to NULL.  The property 'create' & 'close'
    callbacks are called for zero-sized properties, but the 'set' and 'get'
    callbacks are never called.

        The 'create' callback is called when a new property list with this
    property is being created.  H5P_prp_create_func_t is defined as:
        typedef herr_t (*H5P_prp_create_func_t)(hid_t prop_id, const char *name,
                size_t size, void *initial_value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list being created.
        const char *name;   IN: The name of the property being modified.
        size_t size;        IN: The size of the property value
        void *initial_value; IN/OUT: The initial value for the property being created.
                                (The 'default' value passed to H5Pregister2)
    The 'create' routine may modify the value to be set and those changes will
    be stored as the initial value of the property.  If the 'create' routine
    returns a negative value, the new property value is not copied into the
    property and the property list creation routine returns an error value.

        The 'set' callback is called before a new value is copied into the
    property.  H5P_prp_set_func_t is defined as:
        typedef herr_t (*H5P_prp_set_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list being modified.
        const char *name;   IN: The name of the property being modified.
        size_t size;        IN: The size of the property value
        void *new_value;    IN/OUT: The value being set for the property.
    The 'set' routine may modify the value to be set and those changes will be
    stored as the value of the property.  If the 'set' routine returns a
    negative value, the new property value is not copied into the property and
    the property list set routine returns an error value.

        The 'get' callback is called before a value is retrieved from the
    property.  H5P_prp_get_func_t is defined as:
        typedef herr_t (*H5P_prp_get_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list being queried.
        const char *name;   IN: The name of the property being queried.
        size_t size;        IN: The size of the property value
        void *value;        IN/OUT: The value being retrieved for the property.
    The 'get' routine may modify the value to be retrieved and those changes
    will be returned to the calling function.  If the 'get' routine returns a
    negative value, the property value is returned and the property list get
    routine returns an error value.

        The 'delete' callback is called when a property is deleted from a
    property list.  H5P_prp_del_func_t is defined as:
        typedef herr_t (*H5P_prp_del_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list the property is deleted from.
        const char *name;   IN: The name of the property being deleted.
        size_t size;        IN: The size of the property value
        void *value;        IN/OUT: The value of the property being deleted.
    The 'delete' routine may modify the value passed in, but the value is not
    used by the library when the 'delete' routine returns.  If the
    'delete' routine returns a negative value, the property list deletion
    routine returns an error value but the property is still deleted.

        The 'copy' callback is called when a property list with this
    property is copied.  H5P_prp_copy_func_t is defined as:
        typedef herr_t (*H5P_prp_copy_func_t)(const char *name, size_t size,
            void *value);
    where the parameters to the callback function are:
        const char *name;   IN: The name of the property being copied.
        size_t size;        IN: The size of the property value
        void *value;        IN: The value of the property being copied.
    The 'copy' routine may modify the value to be copied and those changes will be
    stored as the value of the property.  If the 'copy' routine returns a
    negative value, the new property value is not copied into the property and
    the property list copy routine returns an error value.

        The 'compare' callback is called when a property list with this
    property is compared to another property list.  H5P_prp_compare_func_t is
    defined as:
        typedef int (*H5P_prp_compare_func_t)( void *value1, void *value2,
            size_t size);
    where the parameters to the callback function are:
        const void *value1; IN: The value of the first property being compared.
        const void *value2; IN: The value of the second property being compared.
        size_t size;        IN: The size of the property value
    The 'compare' routine may not modify the values to be compared.  The
    'compare' routine should return a positive value if VALUE1 is greater than
    VALUE2, a negative value if VALUE2 is greater than VALUE1 and zero if VALUE1
    and VALUE2 are equal.

        The 'close' callback is called when a property list with this
    property is being destroyed.  H5P_prp_close_func_t is defined as:
        typedef herr_t (*H5P_prp_close_func_t)(const char *name, size_t size,
            void *value);
    where the parameters to the callback function are:
        const char *name;   IN: The name of the property being closed.
        size_t size;        IN: The size of the property value
        void *value;        IN: The value of the property being closed.
    The 'close' routine may modify the value passed in, but the value is not
    used by the library when the 'close' routine returns.  If the
    'close' routine returns a negative value, the property list close
    routine returns an error value but the property list is still closed.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
        The 'set' callback function may be useful to range check the value being
    set for the property or may perform some tranformation/translation of the
    value set.  The 'get' callback would then [probably] reverse the
    transformation, etc.  A single 'get' or 'set' callback could handle
    multiple properties by performing different actions based on the property
    name or other properties in the property list.

        I would like to say "the property list is not closed" when a 'close'
    routine fails, but I don't think that's possible due to other properties in
    the list being successfully closed & removed from the property list.  I
    suppose that it would be possible to just remove the properties which have
    successful 'close' callbacks, but I'm not happy with the ramifications
    of a mangled, un-closable property list hanging around...  Any comments? -QAK

 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_register_real(H5P_genclass_t *pclass, const char *name, size_t size,
    const void *def_value, H5P_prp_create_func_t prp_create, H5P_prp_set_func_t prp_set,
    H5P_prp_get_func_t prp_get, H5P_prp_delete_func_t prp_delete,
    H5P_prp_copy_func_t prp_copy, H5P_prp_compare_func_t prp_cmp,
    H5P_prp_close_func_t prp_close)
{
    H5P_genprop_t *new_prop = NULL;     /* Temporary property pointer */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5P_register_real, FAIL)

    HDassert(pclass);
    HDassert(0 == pclass->plists);
    HDassert(0 == pclass->classes);
    HDassert(name);
    HDassert((size > 0 && def_value != NULL) || (size == 0));

    /* Check for duplicate named properties */
    if(NULL != H5SL_search(pclass->props, name))
        HGOTO_ERROR(H5E_PLIST, H5E_EXISTS, FAIL, "property already exists")

    /* Create property object from parameters */
    if(NULL == (new_prop = H5P_create_prop(name, size, H5P_PROP_WITHIN_CLASS, def_value, prp_create, prp_set, prp_get, prp_delete, prp_copy, prp_cmp, prp_close)))
        HGOTO_ERROR(H5E_PLIST, H5E_CANTCREATE, FAIL,"Can't create property")

    /* Insert property into property list class */
    if(H5P_add_prop(pclass->props, new_prop) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL,"Can't insert property into class")

    /* Increment property count for class */
    pclass->nprops++;

    /* Update the revision for the class */
    pclass->revision = H5P_GET_NEXT_REV;

done:
    if(ret_value < 0)
        if(new_prop && H5P_free_prop(new_prop) < 0)
            HDONE_ERROR(H5E_PLIST, H5E_CANTRELEASE, FAIL, "unable to close property")

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_register_real() */


/*--------------------------------------------------------------------------
 NAME
    H5P_register
 PURPOSE
    Internal routine to register a new property in a property list class.
 USAGE
    herr_t H5P_register(class, name, size, default, prp_create, prp_set, prp_get, prp_close)
        H5P_genclass_t **class; IN: Property list class to modify
        const char *name;       IN: Name of property to register
        size_t size;            IN: Size of property in bytes
        void *def_value;        IN: Pointer to buffer containing default value
                                    for property in newly created property lists
        H5P_prp_create_func_t prp_create;   IN: Function pointer to property
                                    creation callback
        H5P_prp_set_func_t prp_set; IN: Function pointer to property set callback
        H5P_prp_get_func_t prp_get; IN: Function pointer to property get callback
        H5P_prp_delete_func_t prp_delete; IN: Function pointer to property delete callback
        H5P_prp_copy_func_t prp_copy; IN: Function pointer to property copy callback
        H5P_prp_compare_func_t prp_cmp; IN: Function pointer to property compare callback
        H5P_prp_close_func_t prp_close; IN: Function pointer to property close
                                    callback
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Registers a new property with a property list class.  The property will
    exist in all property list objects of that class after this routine is
    finished.  The name of the property must not already exist.  The default
    property value must be provided and all new property lists created with this
    property will have the property value set to the default provided.  Any of
    the callback routines may be set to NULL if they are not needed.

        Zero-sized properties are allowed and do not store any data in the
    property list.  These may be used as flags to indicate the presence or
    absence of a particular piece of information.  The 'default' pointer for a
    zero-sized property may be set to NULL.  The property 'create' & 'close'
    callbacks are called for zero-sized properties, but the 'set' and 'get'
    callbacks are never called.

        The 'create' callback is called when a new property list with this
    property is being created.  H5P_prp_create_func_t is defined as:
        typedef herr_t (*H5P_prp_create_func_t)(hid_t prop_id, const char *name,
                size_t size, void *initial_value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list being created.
        const char *name;   IN: The name of the property being modified.
        size_t size;        IN: The size of the property value
        void *initial_value; IN/OUT: The initial value for the property being created.
                                (The 'default' value passed to H5Pregister2)
    The 'create' routine may modify the value to be set and those changes will
    be stored as the initial value of the property.  If the 'create' routine
    returns a negative value, the new property value is not copied into the
    property and the property list creation routine returns an error value.

        The 'set' callback is called before a new value is copied into the
    property.  H5P_prp_set_func_t is defined as:
        typedef herr_t (*H5P_prp_set_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list being modified.
        const char *name;   IN: The name of the property being modified.
        size_t size;        IN: The size of the property value
        void *new_value;    IN/OUT: The value being set for the property.
    The 'set' routine may modify the value to be set and those changes will be
    stored as the value of the property.  If the 'set' routine returns a
    negative value, the new property value is not copied into the property and
    the property list set routine returns an error value.

        The 'get' callback is called before a value is retrieved from the
    property.  H5P_prp_get_func_t is defined as:
        typedef herr_t (*H5P_prp_get_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list being queried.
        const char *name;   IN: The name of the property being queried.
        size_t size;        IN: The size of the property value
        void *value;        IN/OUT: The value being retrieved for the property.
    The 'get' routine may modify the value to be retrieved and those changes
    will be returned to the calling function.  If the 'get' routine returns a
    negative value, the property value is returned and the property list get
    routine returns an error value.

        The 'delete' callback is called when a property is deleted from a
    property list.  H5P_prp_del_func_t is defined as:
        typedef herr_t (*H5P_prp_del_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list the property is deleted from.
        const char *name;   IN: The name of the property being deleted.
        size_t size;        IN: The size of the property value
        void *value;        IN/OUT: The value of the property being deleted.
    The 'delete' routine may modify the value passed in, but the value is not
    used by the library when the 'delete' routine returns.  If the
    'delete' routine returns a negative value, the property list deletion
    routine returns an error value but the property is still deleted.

        The 'copy' callback is called when a property list with this
    property is copied.  H5P_prp_copy_func_t is defined as:
        typedef herr_t (*H5P_prp_copy_func_t)(const char *name, size_t size,
            void *value);
    where the parameters to the callback function are:
        const char *name;   IN: The name of the property being copied.
        size_t size;        IN: The size of the property value
        void *value;        IN: The value of the property being copied.
    The 'copy' routine may modify the value to be copied and those changes will be
    stored as the value of the property.  If the 'copy' routine returns a
    negative value, the new property value is not copied into the property and
    the property list copy routine returns an error value.

        The 'compare' callback is called when a property list with this
    property is compared to another property list.  H5P_prp_compare_func_t is
    defined as:
        typedef int (*H5P_prp_compare_func_t)( void *value1, void *value2,
            size_t size);
    where the parameters to the callback function are:
        const void *value1; IN: The value of the first property being compared.
        const void *value2; IN: The value of the second property being compared.
        size_t size;        IN: The size of the property value
    The 'compare' routine may not modify the values to be compared.  The
    'compare' routine should return a positive value if VALUE1 is greater than
    VALUE2, a negative value if VALUE2 is greater than VALUE1 and zero if VALUE1
    and VALUE2 are equal.

        The 'close' callback is called when a property list with this
    property is being destroyed.  H5P_prp_close_func_t is defined as:
        typedef herr_t (*H5P_prp_close_func_t)(const char *name, size_t size,
            void *value);
    where the parameters to the callback function are:
        const char *name;   IN: The name of the property being closed.
        size_t size;        IN: The size of the property value
        void *value;        IN: The value of the property being closed.
    The 'close' routine may modify the value passed in, but the value is not
    used by the library when the 'close' routine returns.  If the
    'close' routine returns a negative value, the property list close
    routine returns an error value but the property list is still closed.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
        The 'set' callback function may be useful to range check the value being
    set for the property or may perform some tranformation/translation of the
    value set.  The 'get' callback would then [probably] reverse the
    transformation, etc.  A single 'get' or 'set' callback could handle
    multiple properties by performing different actions based on the property
    name or other properties in the property list.

        I would like to say "the property list is not closed" when a 'close'
    routine fails, but I don't think that's possible due to other properties in
    the list being successfully closed & removed from the property list.  I
    suppose that it would be possible to just remove the properties which have
    successful 'close' callbacks, but I'm not happy with the ramifications
    of a mangled, un-closable property list hanging around...  Any comments? -QAK

 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_register(H5P_genclass_t **ppclass, const char *name, size_t size,
    const void *def_value, H5P_prp_create_func_t prp_create, H5P_prp_set_func_t prp_set,
    H5P_prp_get_func_t prp_get, H5P_prp_delete_func_t prp_delete,
    H5P_prp_copy_func_t prp_copy, H5P_prp_compare_func_t prp_cmp,
    H5P_prp_close_func_t prp_close)
{
    H5P_genclass_t *pclass = *ppclass;  /* Pointer to class to modify */
    H5P_genclass_t *new_class = NULL;   /* New class pointer */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5P_register, FAIL)

    /* Sanity check */
    HDassert(ppclass);
    HDassert(pclass);

    /* Check if class needs to be split because property lists or classes have
     *  been created since the last modification was made to the class.
     */
    if(pclass->plists > 0 || pclass->classes > 0) {
        if(NULL == (new_class = H5P_create_class(pclass->parent, pclass->name,
                pclass->internal, pclass->create_func, pclass->create_data,
                pclass->copy_func, pclass->copy_data,
                pclass->close_func, pclass->close_data)))
            HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, FAIL, "can't copy class")

        /* Walk through the skip list of the old class and copy properties */
        if(pclass->nprops > 0) {
            H5SL_node_t *curr_node;   /* Current node in skip list */

            /* Walk through the properties in the old class */
            curr_node = H5SL_first(pclass->props);
            while(curr_node != NULL) {
                H5P_genprop_t *pcopy;               /* Property copy */

                /* Make a copy of the class's property */
                if(NULL == (pcopy = H5P_dup_prop((H5P_genprop_t *)H5SL_item(curr_node), H5P_PROP_WITHIN_CLASS)))
                    HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, FAIL, "Can't copy property")

                /* Insert the initialized property into the property class */
                if(H5P_add_prop(new_class->props, pcopy) < 0)
                    HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL,"Can't insert property into class")

                /* Increment property count for class */
                new_class->nprops++;

                /* Get the next property node in the skip list */
                curr_node = H5SL_next(curr_node);
            } /* end while */
        } /* end if */

        /* Use the new class instead of the old one */
        pclass = new_class;
    } /* end if */

    /* Really register the property in the class */
    if(H5P_register_real(pclass, name, size, def_value, prp_create, prp_set, prp_get, prp_delete, prp_copy, prp_cmp, prp_close) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTCREATE, FAIL, "can't register property")

    /* Update pointer to pointer to class, if a new one was generated */
    if(new_class)
        *ppclass = pclass;

done:
    if(ret_value < 0)
        if(new_class && H5P_close_class(new_class) < 0)
            HDONE_ERROR(H5E_PLIST, H5E_CANTRELEASE, FAIL, "unable to close new property class")

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_register() */


/*--------------------------------------------------------------------------
 NAME
    H5P_insert
 PURPOSE
    Internal routine to insert a new property in a property list.
 USAGE
    herr_t H5P_insert(plist, name, size, value, prp_set, prp_get, prp_close)
        H5P_genplist_t *plist;  IN: Property list to add property to
        const char *name;       IN: Name of property to add
        size_t size;            IN: Size of property in bytes
        void *value;            IN: Pointer to the value for the property
        H5P_prp_set_func_t prp_set; IN: Function pointer to property set callback
        H5P_prp_get_func_t prp_get; IN: Function pointer to property get callback
        H5P_prp_delete_func_t prp_delete; IN: Function pointer to property delete callback
        H5P_prp_copy_func_t prp_copy; IN: Function pointer to property copy callback
        H5P_prp_compare_func_t prp_cmp; IN: Function pointer to property compare callback
        H5P_prp_close_func_t prp_close; IN: Function pointer to property close
                                    callback
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Inserts a temporary property into a property list.  The property will
    exist only in this property list object.  The name of the property must not
    already exist.  The value must be provided unless the property is zero-
    sized.  Any of the callback routines may be set to NULL if they are not
    needed.

        Zero-sized properties are allowed and do not store any data in the
    property list.  These may be used as flags to indicate the presence or
    absence of a particular piece of information.  The 'value' pointer for a
    zero-sized property may be set to NULL.  The property 'close' callback is
    called for zero-sized properties, but the 'set' and 'get' callbacks are
    never called.

        The 'set' callback is called before a new value is copied into the
    property.  H5P_prp_set_func_t is defined as:
        typedef herr_t (*H5P_prp_set_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list being modified.
        const char *name;   IN: The name of the property being modified.
        size_t size;        IN: The size of the property value
        void *new_value;    IN/OUT: The value being set for the property.
    The 'set' routine may modify the value to be set and those changes will be
    stored as the value of the property.  If the 'set' routine returns a
    negative value, the new property value is not copied into the property and
    the property list set routine returns an error value.

        The 'get' callback is called before a value is retrieved from the
    property.  H5P_prp_get_func_t is defined as:
        typedef herr_t (*H5P_prp_get_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list being queried.
        const char *name;   IN: The name of the property being queried.
        size_t size;        IN: The size of the property value
        void *value;        IN/OUT: The value being retrieved for the property.
    The 'get' routine may modify the value to be retrieved and those changes
    will be returned to the calling function.  If the 'get' routine returns a
    negative value, the property value is returned and the property list get
    routine returns an error value.

        The 'delete' callback is called when a property is deleted from a
    property list.  H5P_prp_del_func_t is defined as:
        typedef herr_t (*H5P_prp_del_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list the property is deleted from.
        const char *name;   IN: The name of the property being deleted.
        size_t size;        IN: The size of the property value
        void *value;        IN/OUT: The value of the property being deleted.
    The 'delete' routine may modify the value passed in, but the value is not
    used by the library when the 'delete' routine returns.  If the
    'delete' routine returns a negative value, the property list deletion
    routine returns an error value but the property is still deleted.

        The 'copy' callback is called when a property list with this
    property is copied.  H5P_prp_copy_func_t is defined as:
        typedef herr_t (*H5P_prp_copy_func_t)(const char *name, size_t size,
            void *value);
    where the parameters to the callback function are:
        const char *name;   IN: The name of the property being copied.
        size_t size;        IN: The size of the property value
        void *value;        IN: The value of the property being copied.
    The 'copy' routine may modify the value to be copied and those changes will be
    stored as the value of the property.  If the 'copy' routine returns a
    negative value, the new property value is not copied into the property and
    the property list copy routine returns an error value.

        The 'compare' callback is called when a property list with this
    property is compared to another property list.  H5P_prp_compare_func_t is
    defined as:
        typedef int (*H5P_prp_compare_func_t)( void *value1, void *value2,
            size_t size);
    where the parameters to the callback function are:
        const void *value1; IN: The value of the first property being compared.
        const void *value2; IN: The value of the second property being compared.
        size_t size;        IN: The size of the property value
    The 'compare' routine may not modify the values to be compared.  The
    'compare' routine should return a positive value if VALUE1 is greater than
    VALUE2, a negative value if VALUE2 is greater than VALUE1 and zero if VALUE1
    and VALUE2 are equal.

        The 'close' callback is called when a property list with this
    property is being destroyed.  H5P_prp_close_func_t is defined as:
        typedef herr_t (*H5P_prp_close_func_t)(const char *name, size_t size,
            void *value);
    where the parameters to the callback function are:
        const char *name;   IN: The name of the property being closed.
        size_t size;        IN: The size of the property value
        void *value;        IN: The value of the property being closed.
    The 'close' routine may modify the value passed in, but the value is not
    used by the library when the 'close' routine returns.  If the
    'close' routine returns a negative value, the property list close
    routine returns an error value but the property list is still closed.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
        The 'set' callback function may be useful to range check the value being
    set for the property or may perform some tranformation/translation of the
    value set.  The 'get' callback would then [probably] reverse the
    transformation, etc.  A single 'get' or 'set' callback could handle
    multiple properties by performing different actions based on the property
    name or other properties in the property list.

        There is no 'create' callback routine for temporary property list
    objects, the initial value is assumed to have any necessary setup already
    performed on it.

        I would like to say "the property list is not closed" when a 'close'
    routine fails, but I don't think that's possible due to other properties in
    the list being successfully closed & removed from the property list.  I
    suppose that it would be possible to just remove the properties which have
    successful 'close' callbacks, but I'm not happy with the ramifications
    of a mangled, un-closable property list hanging around...  Any comments? -QAK

 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_insert(H5P_genplist_t *plist, const char *name, size_t size,
    void *value, H5P_prp_set_func_t prp_set, H5P_prp_get_func_t prp_get,
    H5P_prp_delete_func_t prp_delete, H5P_prp_copy_func_t prp_copy,
    H5P_prp_compare_func_t prp_cmp, H5P_prp_close_func_t prp_close)
{
    H5P_genprop_t *new_prop = NULL;     /* Temporary property pointer */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_insert)

    HDassert(plist);
    HDassert(name);
    HDassert((size > 0 && value != NULL) || (size == 0));

    /* Check for duplicate named properties */
    if(NULL != H5SL_search(plist->props, name))
        HGOTO_ERROR(H5E_PLIST, H5E_EXISTS, FAIL, "property already exists")

    /* Check if the property has been deleted */
    if(NULL != H5SL_search(plist->del, name)) {
        /* Remove the property name from the deleted property skip list */
        if(NULL == H5SL_remove(plist->del, name))
            HGOTO_ERROR(H5E_PLIST,H5E_CANTDELETE,FAIL,"can't remove property from deleted skip list")
    } /* end if */
    else {
        H5P_genclass_t *tclass;     /* Temporary class pointer */

        /* Check if the property is already in the class hierarchy */
        tclass = plist->pclass;
        while(tclass) {
            if(tclass->nprops > 0) {
                /* Find the property in the class */
                if(NULL != H5SL_search(tclass->props, name))
                    HGOTO_ERROR(H5E_PLIST, H5E_EXISTS, FAIL, "property already exists")
            } /* end if */

            /* Go up to parent class */
            tclass = tclass->parent;
        } /* end while */
    } /* end else */

    /* Ok to add to property list */

    /* Create property object from parameters */
    if(NULL == (new_prop = H5P_create_prop(name, size, H5P_PROP_WITHIN_LIST, value, NULL, prp_set, prp_get, prp_delete, prp_copy, prp_cmp, prp_close)))
        HGOTO_ERROR(H5E_PLIST, H5E_CANTCREATE, FAIL, "Can't create property")

    /* Insert property into property list class */
    if(H5P_add_prop(plist->props, new_prop) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "Can't insert property into class")

    /* Increment property count for class */
    plist->nprops++;

done:
    if(ret_value < 0)
        if(new_prop && H5P_free_prop(new_prop) < 0)
            HDONE_ERROR(H5E_PLIST, H5E_CANTRELEASE, FAIL, "unable to close property")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5P_insert() */


/*--------------------------------------------------------------------------
 NAME
    H5P_set
 PURPOSE
    Internal routine to set a property's value in a property list.
 USAGE
    herr_t H5P_set(plist, name, value)
        H5P_genplist_t *plist;  IN: Property list to find property in
        const char *name;       IN: Name of property to set
        void *value;            IN: Pointer to the value for the property
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Sets a new value for a property in a property list.  The property name
    must exist or this routine will fail.  If there is a 'set' callback routine
    registered for this property, the 'value' will be passed to that routine and
    any changes to the 'value' will be used when setting the property value.
    The information pointed at by the 'value' pointer (possibly modified by the
    'set' callback) is copied into the property list value and may be changed
    by the application making the H5Pset call without affecting the property
    value.

        If the 'set' callback routine returns an error, the property value will
    not be modified.  This routine may not be called for zero-sized properties
    and will return an error in that case.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_set(H5P_genplist_t *plist, const char *name, const void *value)
{
    H5P_genclass_t *tclass;     /* Temporary class pointer */
    H5P_genprop_t *prop;        /* Temporary property pointer */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5P_set, FAIL)

    HDassert(plist);
    HDassert(name);
    HDassert(value);

    /* Check if the property has been deleted */
    if(H5SL_search(plist->del,name)!=NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "property doesn't exist")

    /* Find property in changed list */
    if(NULL != (prop = (H5P_genprop_t *)H5SL_search(plist->props, name))) {
        /* Check for property size >0 */
        if(prop->size==0)
            HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "property has zero size")

        /* Make a copy of the value and pass to 'set' callback */
        if(prop->set!=NULL) {
            void *tmp_value;            /* Temporary value for property */

            /* Make a copy of the current value, in case the callback fails */
            if(NULL==(tmp_value=H5MM_malloc(prop->size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed temporary property value")
            HDmemcpy(tmp_value,value,prop->size);

            /* Call user's callback */
            if((*(prop->set))(plist->plist_id,name,prop->size,tmp_value) < 0) {
                H5MM_xfree(tmp_value);
                HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't set property value")
            } /* end if */

            /* Copy new [possibly unchanged] value into property value */
            HDmemcpy(prop->value,tmp_value,prop->size);

            /* Free the temporary value buffer */
            H5MM_xfree(tmp_value);
        } /* end if */
        /* No 'set' callback, just copy value */
        else
            HDmemcpy(prop->value,value,prop->size);
    } /* end if */
    else {
        /*
         * Check if we should set class properties (up through list of parent classes also),
         * & make property 'set' callback.
         */
        tclass=plist->pclass;
        while(tclass!=NULL) {
            if(tclass->nprops>0) {
                /* Find the property in the class */
                if((prop = (H5P_genprop_t *)H5SL_search(tclass->props,name))!=NULL) {
                    H5P_genprop_t *pcopy;  /* Copy of property to insert into skip list */

                    /* Check for property size >0 */
                    if(prop->size==0)
                        HGOTO_ERROR(H5E_PLIST,H5E_BADVALUE,FAIL,"property has zero size")

                    /* Make a copy of the value and pass to 'set' callback */
                    if(prop->set!=NULL) {
                        void *tmp_value;            /* Temporary value for property */

                        /* Make a copy of the current value, in case the callback fails */
                        if(NULL==(tmp_value=H5MM_malloc(prop->size)))
                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed temporary property value")
                        HDmemcpy(tmp_value,value,prop->size);

                        /* Call user's callback */
                        if((*(prop->set))(plist->plist_id,name,prop->size,tmp_value) < 0) {
                            H5MM_xfree(tmp_value);
                            HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't set property value")
                        } /* end if */

                        if((prop->cmp)(tmp_value,prop->value,prop->size)) {
                            /* Make a copy of the class's property */
                            if((pcopy=H5P_dup_prop(prop,H5P_PROP_WITHIN_LIST)) == NULL)
                                HGOTO_ERROR(H5E_PLIST,H5E_CANTCOPY,FAIL,"Can't copy property")

                            /* Copy new value into property value */
                            HDmemcpy(pcopy->value,tmp_value,pcopy->size);

                            /* Insert the changed property into the property list */
                            if(H5P_add_prop(plist->props,pcopy) < 0)
                                HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL,"Can't insert changed property into skip list")
                        } /* end if */

                        /* Free the temporary value buffer */
                        H5MM_xfree(tmp_value);
                    } /* end if */
                    /* No 'set' callback, just copy value */
                    else {
                        if((prop->cmp)(value,prop->value,prop->size)) {
                            /* Make a copy of the class's property */
                            if((pcopy=H5P_dup_prop(prop,H5P_PROP_WITHIN_LIST)) == NULL)
                                HGOTO_ERROR(H5E_PLIST,H5E_CANTCOPY,FAIL,"Can't copy property")

                            HDmemcpy(pcopy->value,value,pcopy->size);

                            /* Insert the changed property into the property list */
                            if(H5P_add_prop(plist->props,pcopy) < 0)
                                HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL,"Can't insert changed property into skip list")
                        } /* end if */
                    } /* end else */

                    /* Leave */
                    HGOTO_DONE(SUCCEED);
                } /* end while */
            } /* end if */

            /* Go up to parent class */
            tclass=tclass->parent;
        } /* end while */

        /* If we get this far, then it wasn't in the list of changed properties,
         * nor in the properties in the class hierarchy, indicate an error
         */
        HGOTO_ERROR(H5E_PLIST,H5E_NOTFOUND,FAIL,"can't find property in skip list")
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_set() */


/*--------------------------------------------------------------------------
 NAME
    H5P_exist_plist
 PURPOSE
    Internal routine to query the existance of a property in a property list.
 USAGE
    herr_t H5P_exist_plist(plist, name)
        H5P_genplist_t *plist;  IN: Property list to check
        const char *name;       IN: Name of property to check for
 RETURNS
    Success: Positive if the property exists in the property list, zero
            if the property does not exist.
    Failure: negative value
 DESCRIPTION
        This routine checks if a property exists within a property list.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5P_exist_plist(H5P_genplist_t *plist, const char *name)
{
    htri_t ret_value = FAIL;     /* return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_exist_plist)

    HDassert(plist);
    HDassert(name);

    /* Check for property in deleted property list */
    if(H5SL_search(plist->del, name) != NULL)
        ret_value = FALSE;
    else {
        /* Check for property in changed property list */
        if(H5SL_search(plist->props, name) != NULL)
            ret_value = TRUE;
        else {
            H5P_genclass_t *tclass;     /* Temporary class pointer */

            tclass = plist->pclass;
            while(tclass != NULL) {
                if(H5SL_search(tclass->props, name) != NULL)
                    HGOTO_DONE(TRUE)

                /* Go up to parent class */
                tclass = tclass->parent;
            } /* end while */

            /* If we've reached here, we couldn't find the property */
            ret_value = FALSE;
        } /* end else */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_exist_plist() */


/*--------------------------------------------------------------------------
 NAME
    H5P_exist_pclass
 PURPOSE
    Internal routine to query the existance of a property in a property class.
 USAGE
    herr_t H5P_exist_pclass(pclass, name)
        H5P_genclass_t *pclass;  IN: Property class to check
        const char *name;       IN: Name of property to check for
 RETURNS
    Success: Positive if the property exists in the property class, zero
            if the property does not exist.
    Failure: negative value
 DESCRIPTION
        This routine checks if a property exists within a property class.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5P_exist_pclass(H5P_genclass_t *pclass, const char *name)
{
    htri_t ret_value = FAIL;     /* return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_exist_pclass)

    HDassert(pclass);
    HDassert(name);

    /* Check for property in property list */
    if(H5SL_search(pclass->props, name) != NULL)
        ret_value = TRUE;
    else {
        H5P_genclass_t *tclass;     /* Temporary class pointer */

        tclass = pclass->parent;
        while(tclass != NULL) {
            if(H5SL_search(tclass->props, name) != NULL)
                HGOTO_DONE(TRUE)

            /* Go up to parent class */
            tclass = tclass->parent;
        } /* end while */

        /* If we've reached here, we couldn't find the property */
        ret_value = FALSE;
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_exist_pclass() */


/*--------------------------------------------------------------------------
 NAME
    H5P_get_size_plist
 PURPOSE
    Internal routine to query the size of a property in a property list.
 USAGE
    herr_t H5P_get_size_plist(plist, name)
        H5P_genplist_t *plist;  IN: Property list to check
        const char *name;       IN: Name of property to query
        size_t *size;           OUT: Size of property
 RETURNS
    Success: non-negative value
    Failure: negative value
 DESCRIPTION
        This routine retrieves the size of a property's value in bytes.  Zero-
    sized properties are allowed and return a value of 0.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_get_size_plist(H5P_genplist_t *plist, const char *name, size_t *size)
{
    H5P_genprop_t *prop;        /* Temporary property pointer */
    herr_t ret_value=SUCCEED;      /* return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_get_size_plist)

    HDassert(plist);
    HDassert(name);
    HDassert(size);

    /* Find property */
    if((prop=H5P_find_prop_plist(plist,name)) == NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "property doesn't exist")

    /* Get property size */
    *size=prop->size;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_get_size_plist() */


/*--------------------------------------------------------------------------
 NAME
    H5P_get_size_pclass
 PURPOSE
    Internal routine to query the size of a property in a property class.
 USAGE
    herr_t H5P_get_size_pclass(pclass, name)
        H5P_genclass_t *pclass; IN: Property class to check
        const char *name;       IN: Name of property to query
        size_t *size;           OUT: Size of property
 RETURNS
    Success: non-negative value
    Failure: negative value
 DESCRIPTION
        This routine retrieves the size of a property's value in bytes.  Zero-
    sized properties are allowed and return a value of 0.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_get_size_pclass(H5P_genclass_t *pclass, const char *name, size_t *size)
{
    H5P_genprop_t *prop;        /* Temporary property pointer */
    herr_t ret_value=SUCCEED;      /* return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_get_size_pclass)

    HDassert(pclass);
    HDassert(name);
    HDassert(size);

    /* Find property */
    if((prop=H5P_find_prop_pclass(pclass,name)) == NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "property doesn't exist")

    /* Get property size */
    *size=prop->size;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_get_size_pclass() */


/*--------------------------------------------------------------------------
 NAME
    H5P_get_class
 PURPOSE
    Internal routine to query the class of a generic property list
 USAGE
    H5P_genclass_t *H5P_get_class(plist)
        H5P_genplist_t *plist;    IN: Property list to check
 RETURNS
    Success: Pointer to the class for a property list
    Failure: NULL
 DESCRIPTION
    This routine retrieves a pointer to the class for a property list.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5P_genclass_t *
H5P_get_class(const H5P_genplist_t *plist)
{
    H5P_genclass_t *ret_value;      /* return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_get_class)

    HDassert(plist);

    /* Get property size */
    ret_value = plist->pclass;

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_get_class() */


/*--------------------------------------------------------------------------
 NAME
    H5P_get_nprops_plist
 PURPOSE
    Internal routine to query the number of properties in a property list
 USAGE
    herr_t H5P_get_nprops_plist(plist, nprops)
        H5P_genplist_t *plist;  IN: Property list to check
        size_t *nprops;         OUT: Number of properties in the property list
 RETURNS
    Success: non-negative value
    Failure: negative value
 DESCRIPTION
        This routine retrieves the number of a properties in a property list.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_get_nprops_plist(const H5P_genplist_t *plist, size_t *nprops)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_get_nprops_plist)

    HDassert(plist);
    HDassert(nprops);

    /* Get property size */
    *nprops = plist->nprops;

    FUNC_LEAVE_NOAPI(SUCCEED)
}   /* H5P_get_nprops_plist() */


/*--------------------------------------------------------------------------
 NAME
    H5P_get_nprops_pclass
 PURPOSE
    Internal routine to query the number of properties in a property class
 USAGE
    herr_t H5P_get_nprops_pclass(pclass, nprops)
        H5P_genclass_t *pclass;  IN: Property class to check
        size_t *nprops;         OUT: Number of properties in the property list
        hbool_t recurse;        IN: Include properties in parent class(es) also
 RETURNS
    Success: non-negative value (can't fail)
    Failure: negative value
 DESCRIPTION
    This routine retrieves the number of a properties in a property class.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_get_nprops_pclass(const H5P_genclass_t *pclass, size_t *nprops, hbool_t recurse)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5P_get_nprops_pclass, FAIL)

    HDassert(pclass);
    HDassert(nprops);

    /* Get number of properties */
    *nprops = pclass->nprops;

    /* Check if the class is derived, and walk up the chain, if so */
    if(recurse)
	while(pclass->parent != NULL) {
	    pclass = pclass->parent;
	    *nprops += pclass->nprops;
	} /* end while */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_get_nprops_pclass() */


/*--------------------------------------------------------------------------
 NAME
    H5P_cmp_prop
 PURPOSE
    Internal routine to compare two generic properties
 USAGE
    int H5P_cmp_prop(prop1, prop2)
        H5P_genprop_t *prop1;    IN: 1st property to compare
        H5P_genprop_t *prop1;    IN: 2nd property to compare
 RETURNS
    Success: negative if prop1 "less" than prop2, positive if prop1 "greater"
        than prop2, zero if prop1 is "equal" to prop2
    Failure: can't fail
 DESCRIPTION
        This function compares two generic properties together to see if
    they are the same property.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static int
H5P_cmp_prop(const H5P_genprop_t *prop1, const H5P_genprop_t *prop2)
{
    int cmp_value;             /* Value from comparison */
    int ret_value = 0;         /* return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_cmp_prop)

    HDassert(prop1);
    HDassert(prop2);

    /* Check the name */
    if((cmp_value = HDstrcmp(prop1->name, prop2->name)) != 0)
        HGOTO_DONE(cmp_value);

    /* Check the size of properties */
    if(prop1->size < prop2->size) HGOTO_DONE(-1);
    if(prop1->size > prop2->size) HGOTO_DONE(1);

    /* Check if they both have the same 'create' callback */
    if(prop1->create == NULL && prop2->create != NULL) HGOTO_DONE(-1);
    if(prop1->create != NULL && prop2->create == NULL) HGOTO_DONE(1);
    if(prop1->create != prop2->create) HGOTO_DONE(-1);

    /* Check if they both have the same 'set' callback */
    if(prop1->set == NULL && prop2->set != NULL) HGOTO_DONE(-1);
    if(prop1->set != NULL && prop2->set == NULL) HGOTO_DONE(1);
    if(prop1->set != prop2->set) HGOTO_DONE(-1);

    /* Check if they both have the same 'get' callback */
    if(prop1->get == NULL && prop2->get != NULL) HGOTO_DONE(-1);
    if(prop1->get != NULL && prop2->get == NULL) HGOTO_DONE(1);
    if(prop1->get != prop2->get) HGOTO_DONE(-1);

    /* Check if they both have the same 'delete' callback */
    if(prop1->del == NULL && prop2->del != NULL) HGOTO_DONE(-1);
    if(prop1->del != NULL && prop2->del == NULL) HGOTO_DONE(1);
    if(prop1->del != prop2->del) HGOTO_DONE(-1);

    /* Check if they both have the same 'copy' callback */
    if(prop1->copy == NULL && prop2->copy != NULL) HGOTO_DONE(-1);
    if(prop1->copy != NULL && prop2->copy == NULL) HGOTO_DONE(1);
    if(prop1->copy != prop2->copy) HGOTO_DONE(-1);

    /* Check if they both have the same 'compare' callback */
    if(prop1->cmp == NULL && prop2->cmp != NULL) HGOTO_DONE(-1);
    if(prop1->cmp != NULL && prop2->cmp == NULL) HGOTO_DONE(1);
    if(prop1->cmp != prop2->cmp) HGOTO_DONE(-1);

    /* Check if they both have the same 'close' callback */
    if(prop1->close == NULL && prop2->close != NULL) HGOTO_DONE(-1);
    if(prop1->close != NULL && prop2->close == NULL) HGOTO_DONE(1);
    if(prop1->close != prop2->close) HGOTO_DONE(-1);

    /* Check if they both have values allocated (or not allocated) */
    if(prop1->value == NULL && prop2->value != NULL) HGOTO_DONE(-1);
    if(prop1->value != NULL && prop2->value == NULL) HGOTO_DONE(1);
    if(prop1->value != NULL) {
        /* Call comparison routine */
        if((cmp_value = prop1->cmp(prop1->value, prop2->value, prop1->size)) != 0)
            HGOTO_DONE(cmp_value);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_cmp_prop() */


/*--------------------------------------------------------------------------
 NAME
    H5P_cmp_class
 PURPOSE
    Internal routine to compare two generic property classes
 USAGE
    int H5P_cmp_class(pclass1, pclass2)
        H5P_genclass_t *pclass1;    IN: 1st property class to compare
        H5P_genclass_t *pclass2;    IN: 2nd property class to compare
 RETURNS
    Success: negative if class1 "less" than class2, positive if class1 "greater"
        than class2, zero if class1 is "equal" to class2
    Failure: can't fail
 DESCRIPTION
        This function compares two generic property classes together to see if
    they are the same class.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5P_cmp_class(const H5P_genclass_t *pclass1, const H5P_genclass_t *pclass2)
{
    H5SL_node_t *tnode1, *tnode2;   /* Temporary pointer to property nodes */
    int cmp_value;                  /* Value from comparison */
    int ret_value = 0;              /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_cmp_class)

    HDassert(pclass1);
    HDassert(pclass2);

    /* Use the revision number to quickly check for identical classes */
    if(pclass1->revision == pclass2->revision)
        HGOTO_DONE(0);

    /* Check the name */
    if((cmp_value = HDstrcmp(pclass1->name, pclass2->name)) != 0)
        HGOTO_DONE(cmp_value);

    /* Check the number of properties */
    if(pclass1->nprops < pclass2->nprops) HGOTO_DONE(-1);
    if(pclass1->nprops > pclass2->nprops) HGOTO_DONE(1);

    /* Check the number of property lists created from the class */
    if(pclass1->plists < pclass2->plists) HGOTO_DONE(-1);
    if(pclass1->plists > pclass2->plists) HGOTO_DONE(1);

    /* Check the number of classes derived from the class */
    if(pclass1->classes < pclass2->classes) HGOTO_DONE(-1);
    if(pclass1->classes > pclass2->classes) HGOTO_DONE(1);

    /* Check the number of ID references open on the class */
    if(pclass1->ref_count < pclass2->ref_count) HGOTO_DONE(-1);
    if(pclass1->ref_count > pclass2->ref_count) HGOTO_DONE(1);

    /* Check whether they are internal or not */
    if(pclass1->internal < pclass2->internal) HGOTO_DONE(-1);
    if(pclass1->internal > pclass2->internal) HGOTO_DONE(1);

    /* Check whether they are deleted or not */
    if(pclass1->deleted < pclass2->deleted) HGOTO_DONE(-1);
    if(pclass1->deleted > pclass2->deleted) HGOTO_DONE(1);

    /* Check whether they have creation callback functions & data */
    if(pclass1->create_func == NULL && pclass2->create_func != NULL) HGOTO_DONE(-1);
    if(pclass1->create_func != NULL && pclass2->create_func == NULL) HGOTO_DONE(1);
    if(pclass1->create_func != pclass2->create_func) HGOTO_DONE(-1);
    if(pclass1->create_data < pclass2->create_data) HGOTO_DONE(-1);
    if(pclass1->create_data > pclass2->create_data) HGOTO_DONE(1);

    /* Check whether they have close callback functions & data */
    if(pclass1->close_func == NULL && pclass2->close_func != NULL) HGOTO_DONE(-1);
    if(pclass1->close_func != NULL && pclass2->close_func == NULL) HGOTO_DONE(1);
    if(pclass1->close_func != pclass2->close_func) HGOTO_DONE(-1);
    if(pclass1->close_data < pclass2->close_data) HGOTO_DONE(-1);
    if(pclass1->close_data > pclass2->close_data) HGOTO_DONE(1);

    /* Cycle through the properties and compare them also */
    tnode1 = H5SL_first(pclass1->props);
    tnode2 = H5SL_first(pclass2->props);
    while(tnode1 || tnode2) {
        H5P_genprop_t *prop1, *prop2;   /* Property for node */

        /* Check if they both have properties in this skip list node */
        if(tnode1 == NULL && tnode2 != NULL) HGOTO_DONE(-1);
        if(tnode1 != NULL && tnode2 == NULL) HGOTO_DONE(1);

        /* Compare the two properties */
        prop1 = (H5P_genprop_t *)H5SL_item(tnode1);
        prop2 = (H5P_genprop_t *)H5SL_item(tnode2);
        if((cmp_value = H5P_cmp_prop(prop1, prop2)) != 0)
            HGOTO_DONE(cmp_value);

        /* Advance the pointers */
        tnode1 = H5SL_next(tnode1);
        tnode2 = H5SL_next(tnode2);
    } /* end while */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_cmp_class() */


/*--------------------------------------------------------------------------
 NAME
    H5P_cmp_plist
 PURPOSE
    Internal routine to compare two generic property lists
 USAGE
    int H5P_cmp_plist(plist1, plist2)
        H5P_genplist_t *plist1;    IN: 1st property list to compare
        H5P_genplist_t *plist2;    IN: 2nd property list to compare
 RETURNS
    Success: negative if list1 "less" than list2, positive if list1 "greater"
        than list2, zero if list1 is "equal" to list2
    Failure: can't fail
 DESCRIPTION
        This function compares two generic property lists together to see if
    they are the same list.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5P_cmp_plist(const H5P_genplist_t *plist1, const H5P_genplist_t *plist2)
{
    H5SL_node_t *tnode1, *tnode2;       /* Temporary pointer to property nodes */
    int cmp_value;                      /* Value from comparison */
    int ret_value = 0;                  /* return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_cmp_plist)

    HDassert(plist1);
    HDassert(plist2);

    /* Check the number of properties */
    if(plist1->nprops < plist2->nprops) HGOTO_DONE(-1);
    if(plist1->nprops > plist2->nprops) HGOTO_DONE(1);

    /* Check whether they've been initialized */
    if(plist1->class_init < plist2->class_init) HGOTO_DONE(-1);
    if(plist1->class_init > plist2->class_init) HGOTO_DONE(1);

    /* Check for identical deleted properties */
    if(H5SL_count(plist1->del) > 0) {
        /* Check for no deleted properties in plist2 */
        if(H5SL_count(plist2->del) == 0) HGOTO_DONE(1);

        tnode1 = H5SL_first(plist1->del);
        tnode2 = H5SL_first(plist2->del);
        while(tnode1 || tnode2) {
            const char *name1, *name2;   /* Name for node */

            /* Check if they both have properties in this node */
            if(tnode1 == NULL && tnode2 != NULL) HGOTO_DONE(-1);
            if(tnode1 != NULL && tnode2 == NULL) HGOTO_DONE(1);

            /* Compare the two deleted properties */
            name1 = (const char *)H5SL_item(tnode1);
            name2 = (const char *)H5SL_item(tnode2);
            if((cmp_value = HDstrcmp(name1, name2)) != 0)
                HGOTO_DONE(cmp_value);

            /* Advance the pointers */
            tnode1 = H5SL_next(tnode1);
            tnode2 = H5SL_next(tnode2);
        } /* end while */
    } /* end if */
    else
        if(H5SL_count(plist2->del) > 0) HGOTO_DONE (-1);

    /* Cycle through the changed properties and compare them also */
    if(H5SL_count(plist1->props) > 0) {
        /* Check for no changed properties in plist2 */
        if(H5SL_count(plist2->props) == 0) HGOTO_DONE(1);

        tnode1 = H5SL_first(plist1->props);
        tnode2 = H5SL_first(plist2->props);
        while(tnode1 || tnode2) {
            H5P_genprop_t *prop1, *prop2;   /* Property for node */

            /* Check if they both have properties in this node */
            if(tnode1 == NULL && tnode2 != NULL) HGOTO_DONE(-1);
            if(tnode1 != NULL && tnode2 == NULL) HGOTO_DONE(1);

            /* Compare the two properties */
            prop1 = (H5P_genprop_t *)H5SL_item(tnode1);
            prop2 = (H5P_genprop_t *)H5SL_item(tnode2);
            if((cmp_value = H5P_cmp_prop(prop1, prop2)) != 0)
                HGOTO_DONE(cmp_value);

            /* Advance the pointers */
            tnode1 = H5SL_next(tnode1);
            tnode2 = H5SL_next(tnode2);
        } /* end while */
    } /* end if */
    else
        if(H5SL_count(plist2->props)>0) HGOTO_DONE (-1);

    /* Check the parent classes */
    if((cmp_value = H5P_cmp_class(plist1->pclass, plist2->pclass)) != 0)
        HGOTO_DONE(cmp_value);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_cmp_plist() */


/*--------------------------------------------------------------------------
 NAME
    H5P_isa_class_real
 PURPOSE
    Internal routine to query whether a property class is the same as another
    class.
 USAGE
    htri_t H5P_isa_class_real(pclass1, pclass2)
        H5P_genclass_t *pclass1;   IN: Property class to check
        H5P_genclass_t *pclass2;   IN: Property class to compare with
 RETURNS
    Success: TRUE (1) or FALSE (0)
    Failure: negative value
 DESCRIPTION
    This routine queries whether a property class is the same as another class,
    and walks up the hierarchy of derived classes, checking if the first class
    is derived from the second class also.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static htri_t
H5P_isa_class_real(H5P_genclass_t *pclass1, H5P_genclass_t *pclass2)
{
    htri_t ret_value;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_isa_class_real)

    HDassert(pclass1);
    HDassert(pclass2);

    /* Compare property classes */
    if(H5P_cmp_class(pclass1, pclass2) == 0) {
        HGOTO_DONE(TRUE);
    } else {
        /* Check if the class is derived, and walk up the chain, if so */
        if(pclass1->parent != NULL)
            ret_value = H5P_isa_class_real(pclass1->parent, pclass2);
        else
            HGOTO_DONE(FALSE);
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_isa_class_real() */


/*--------------------------------------------------------------------------
 NAME
    H5P_isa_class
 PURPOSE
    Internal routine to query whether a property list is a certain class
 USAGE
    hid_t H5P_isa_class(plist_id, pclass_id)
        hid_t plist_id;         IN: Property list to query
        hid_t pclass_id;        IN: Property class to query
 RETURNS
    Success: TRUE (1) or FALSE (0)
    Failure: negative
 DESCRIPTION
    This routine queries whether a property list is a member of the property
    list class.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This function is special in that it is an internal library function, but
    accepts hid_t's as parameters.  Since it is used in basically the same way
    as the H5I functions, this should be OK.  Don't make more library functions
    which accept hid_t's without thorough discussion. -QAK
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5P_isa_class(hid_t plist_id, hid_t pclass_id)
{
    H5P_genplist_t	*plist;         /* Property list to query */
    H5P_genclass_t	*pclass;        /* Property list class */
    htri_t ret_value;                   /* return value */

    FUNC_ENTER_NOAPI(H5P_isa_class, FAIL)

    /* Check arguments. */
    if(NULL == (plist = (H5P_genplist_t *)H5I_object_verify(plist_id, H5I_GENPROP_LST)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")
    if(NULL == (pclass = (H5P_genclass_t *)H5I_object_verify(pclass_id, H5I_GENPROP_CLS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property class")

    /* Compare the property list's class against the other class */
    if((ret_value = H5P_isa_class_real(plist->pclass, pclass)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to compare property list classes")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_isa_class() */


/*--------------------------------------------------------------------------
 NAME
    H5P_object_verify
 PURPOSE
    Internal routine to query whether a property list is a certain class and
        retrieve the property list object associated with it.
 USAGE
    void *H5P_object_verify(plist_id, pclass_id)
        hid_t plist_id;         IN: Property list to query
        hid_t pclass_id;        IN: Property class to query
 RETURNS
    Success: valid pointer to a property list object
    Failure: NULL
 DESCRIPTION
    This routine queries whether a property list is member of a certain class
    and retrieves the property list object associated with it.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This function is special in that it is an internal library function, but
    accepts hid_t's as parameters.  Since it is used in basically the same way
    as the H5I functions, this should be OK.  Don't make more library functions
    which accept hid_t's without thorough discussion. -QAK

    This function is similar (in spirit) to H5I_object_verify()
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5P_genplist_t *
H5P_object_verify(hid_t plist_id, hid_t pclass_id)
{
    H5P_genplist_t *ret_value;                   /* return value */

    FUNC_ENTER_NOAPI(H5P_object_verify, NULL)

    /* Compare the property list's class against the other class */
    if(H5P_isa_class(plist_id, pclass_id) != TRUE)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, NULL, "property list is not a member of the class")

    /* Get the plist structure */
    if(NULL == (ret_value = (H5P_genplist_t *)H5I_object(plist_id)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, NULL, "can't find object for ID")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_object_verify() */


/*--------------------------------------------------------------------------
 NAME
    H5P_iterate_plist
 PURPOSE
    Internal routine to iterate over the properties in a property list
 USAGE
    herr_t H5P_iterate_plist(plist_id, idx, iter_func, iter_data)
        hid_t plist_id;             IN: ID of property list to iterate over
        int *idx;                   IN/OUT: Index of the property to begin with
        H5P_iterate_t iter_func;    IN: Function pointer to function to be
                                        called with each property iterated over.
        void *iter_data;            IN/OUT: Pointer to iteration data from user
 RETURNS
    Success: Returns the return value of the last call to ITER_FUNC if it was
                non-zero, or zero if all properties have been processed.
    Failure: negative value
 DESCRIPTION
    This routine iterates over the properties in the property object specified
with PLIST_ID.  For each property in the object, the ITER_DATA and some
additional information, specified below, are passed to the ITER_FUNC function.
The iteration begins with the IDX property in the object and the next element
to be processed by the operator is returned in IDX.  If IDX is NULL, then the
iterator starts at the first property; since no stopping point is returned in
this case, the iterator cannot be restarted if one of the calls to its operator
returns non-zero.

The prototype for H5P_iterate_t is:
    typedef herr_t (*H5P_iterate_t)(hid_t id, const char *name, void *iter_data);
The operation receives the property list or class identifier for the object
being iterated over, ID, the name of the current property within the object,
NAME, and the pointer to the operator data passed in to H5Piterate, ITER_DATA.

The return values from an operator are:
    Zero causes the iterator to continue, returning zero when all properties
        have been processed.
    Positive causes the iterator to immediately return that positive value,
        indicating short-circuit success. The iterator can be restarted at the
        index of the next property.
    Negative causes the iterator to immediately return that value, indicating
        failure. The iterator can be restarted at the index of the next
        property.

H5Piterate assumes that the properties in the object identified by ID remains
unchanged through the iteration.  If the membership changes during the
iteration, the function's behavior is undefined.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5P_iterate_plist(hid_t plist_id, int *idx, H5P_iterate_t iter_func, void *iter_data)
{
    H5P_genclass_t *tclass;     /* Temporary class pointer */
    H5P_genplist_t *plist;      /* Property list pointer */
    H5P_genprop_t *tmp;         /* Temporary pointer to properties */
    H5SL_t *seen = NULL;        /* Skip list to hold names of properties already seen */
    H5SL_node_t *curr_node;     /* Current node in skip list */
    int curr_idx = 0;           /* Current iteration index */
    int ret_value = FAIL;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_iterate_plist)

    HDassert(idx);
    HDassert(iter_func);

    /* Get the property list object */
    if(NULL == (plist = (H5P_genplist_t *)H5I_object_verify(plist_id, H5I_GENPROP_LST)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")

    /* Create the skip list to hold names of properties already seen */
    if((seen = H5SL_create(H5SL_TYPE_STR)) == NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTCREATE,FAIL,"can't create skip list for seen properties")

    /* Walk through the changed properties in the list */
    if(H5SL_count(plist->props) > 0) {
        curr_node = H5SL_first(plist->props);
        while(curr_node != NULL) {
            /* Get pointer to property from node */
            tmp = (H5P_genprop_t *)H5SL_item(curr_node);

            /* Check if we've found the correctly indexed property */
            if(curr_idx>=*idx) {
                /* Call the callback function */
                ret_value=(*iter_func)(plist_id,tmp->name,iter_data);

                if(ret_value!=0)
                    HGOTO_DONE(ret_value);
            } /* end if */

            /* Increment the current index */
            curr_idx++;

            /* Add property name to "seen" list */
            if(H5SL_insert(seen,tmp->name,tmp->name) < 0)
                HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into seen skip list")

            /* Get the next property node in the skip list */
            curr_node=H5SL_next(curr_node);
        } /* end while */
    } /* end if */

    /* Walk up the class hiearchy */
    tclass=plist->pclass;
    while(tclass!=NULL) {
        if(tclass->nprops>0) {
            /* Walk through the properties in the class */
            curr_node=H5SL_first(tclass->props);
            while(curr_node!=NULL) {
                /* Get pointer to property from node */
                tmp = (H5P_genprop_t *)H5SL_item(curr_node);

                /* Only call iterator callback for properties we haven't seen
                 * before and that haven't been deleted
                 */
                if(H5SL_search(seen,tmp->name) == NULL &&
                        H5SL_search(plist->del,tmp->name) == NULL) {


                    /* Check if we've found the correctly indexed property */
                    if(curr_idx>=*idx) {
                        /* Call the callback function */
                        ret_value=(*iter_func)(plist_id,tmp->name,iter_data);

                        if(ret_value!=0)
                            HGOTO_DONE(ret_value);
                    } /* end if */

                    /* Increment the current index */
                    curr_idx++;

                    /* Add property name to "seen" list */
                    if(H5SL_insert(seen,tmp->name,tmp->name) < 0)
                        HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into seen skip list")
                } /* end if */

                /* Get the next property node in the skip list */
                curr_node=H5SL_next(curr_node);
            } /* end while */
        } /* end if */

        /* Go up to parent class */
        tclass=tclass->parent;
    } /* end while */

done:
    /* Set the index we stopped at */
    *idx=curr_idx;

    /* Release the skip list of 'seen' properties */
    if(seen!=NULL)
        H5SL_close(seen);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_iterate_plist() */


/*--------------------------------------------------------------------------
 NAME
    H5P_iterate_pclass
 PURPOSE
    Internal routine to iterate over the properties in a property class
 USAGE
    herr_t H5P_iterate_pclass(pclass_id, idx, iter_func, iter_data)
        hid_t pclass_id;            IN: ID of property class to iterate over
        int *idx;                   IN/OUT: Index of the property to begin with
        H5P_iterate_t iter_func;    IN: Function pointer to function to be
                                        called with each property iterated over.
        void *iter_data;            IN/OUT: Pointer to iteration data from user
 RETURNS
    Success: Returns the return value of the last call to ITER_FUNC if it was
                non-zero, or zero if all properties have been processed.
    Failure: negative value
 DESCRIPTION
    This routine iterates over the properties in the property object specified
with PCLASS_ID.  For each property in the object, the ITER_DATA and some
additional information, specified below, are passed to the ITER_FUNC function.
The iteration begins with the IDX property in the object and the next element
to be processed by the operator is returned in IDX.  If IDX is NULL, then the
iterator starts at the first property; since no stopping point is returned in
this case, the iterator cannot be restarted if one of the calls to its operator
returns non-zero.

The prototype for H5P_iterate_t is:
    typedef herr_t (*H5P_iterate_t)(hid_t id, const char *name, void *iter_data);
The operation receives the property list or class identifier for the object
being iterated over, ID, the name of the current property within the object,
NAME, and the pointer to the operator data passed in to H5Piterate, ITER_DATA.

The return values from an operator are:
    Zero causes the iterator to continue, returning zero when all properties
        have been processed.
    Positive causes the iterator to immediately return that positive value,
        indicating short-circuit success. The iterator can be restarted at the
        index of the next property.
    Negative causes the iterator to immediately return that value, indicating
        failure. The iterator can be restarted at the index of the next
        property.

H5Piterate assumes that the properties in the object identified by ID remains
unchanged through the iteration.  If the membership changes during the
iteration, the function's behavior is undefined.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5P_iterate_pclass(hid_t pclass_id, int *idx, H5P_iterate_t iter_func, void *iter_data)
{
    H5P_genclass_t *pclass;     /* Property list pointer */
    H5SL_node_t *curr_node;     /* Current node in skip list */
    H5P_genprop_t *prop;        /* Temporary property pointer */
    int curr_idx=0;             /* Current iteration index */
    int ret_value=FAIL;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_iterate_pclass)

    HDassert(idx);
    HDassert(iter_func);

    /* Get the property list object */
    if(NULL == (pclass = (H5P_genclass_t *)H5I_object_verify(pclass_id, H5I_GENPROP_CLS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property class")

    /* Cycle through the properties and call the callback */
    curr_idx=0;
    curr_node=H5SL_first(pclass->props);
    while(curr_node!=NULL) {
        if(curr_idx>=*idx) {
            /* Get the property for the node */
            prop = (H5P_genprop_t *)H5SL_item(curr_node);

            /* Call the callback function */
            ret_value=(*iter_func)(pclass_id,prop->name,iter_data);

            /* Check if iteration function succeeded */
            if(ret_value!=0)
                HGOTO_DONE(ret_value);
        } /* end if */

        /* Increment the iteration index */
        curr_idx++;

        /* Get the next property node in the skip list */
        curr_node=H5SL_next(curr_node);
    } /* end while */

done:
    /* Set the index we stopped at */
    *idx=curr_idx;

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_iterate_pclass() */


/*--------------------------------------------------------------------------
 NAME
    H5P_peek_unsigned
 PURPOSE
    Internal routine to quickly retrieve the value of a property in a property list.
 USAGE
    int H5P_peek_unsigned(plist, name)
        H5P_genplist_t *plist;  IN: Property list to check
        const char *name;       IN: Name of property to query
 RETURNS
    Directly returns the value of the property in the list
 DESCRIPTION
        This function directly returns the value of a property in a property
    list.  Because this function is only able to just copy a particular property
    value to the return value, there is no way to check for errors.  We attempt
    to make certain that bad things don't happen by validating that the size of
    the property is the same as the size of the return type, but that can't
    catch all errors.
        This function does call the user's 'get' callback routine still.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    No error checking!
    Use with caution!
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
unsigned
H5P_peek_unsigned(H5P_genplist_t *plist, const char *name)
{
    unsigned ret_value;            /* return value */

    FUNC_ENTER_NOAPI(H5P_peek_unsigned, UFAIL)

    HDassert(plist);
    HDassert(name);

    /* Get the value to return, don't worry about the return value, we can't return it */
    H5P_get(plist,name,&ret_value);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_peek_unsigned() */


/*--------------------------------------------------------------------------
 NAME
    H5P_peek_hid_t
 PURPOSE
    Internal routine to quickly retrieve the value of a property in a property list.
 USAGE
    hid_t H5P_peek_hid_t(plist, name)
        H5P_genplist_t *plist;  IN: Property list to check
        const char *name;       IN: Name of property to query
 RETURNS
    Directly returns the value of the property in the list
 DESCRIPTION
        This function directly returns the value of a property in a property
    list.  Because this function is only able to just copy a particular property
    value to the return value, there is no way to check for errors.  We attempt
    to make certain that bad things don't happen by validating that the size of
    the property is the same as the size of the return type, but that can't
    catch all errors.
        This function does call the user's 'get' callback routine still.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    No error checking!
    Use with caution!
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hid_t
H5P_peek_hid_t(H5P_genplist_t *plist, const char *name)
{
    hid_t ret_value;            /* return value */

    FUNC_ENTER_NOAPI(H5P_peek_hid_t, FAIL)

    HDassert(plist);
    HDassert(name);

    /* Get the value to return, don't worry about the return value, we can't return it */
    H5P_get(plist,name,&ret_value);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_peek_hid_t() */


/*--------------------------------------------------------------------------
 NAME
    H5P_peek_voidp
 PURPOSE
    Internal routine to quickly retrieve the value of a property in a property list.
 USAGE
    void *H5P_peek_voidp(plist, name)
        H5P_genplist_t *plist;  IN: Property list to check
        const char *name;       IN: Name of property to query
 RETURNS
    Directly returns the value of the property in the list
 DESCRIPTION
        This function directly returns the value of a property in a property
    list.  Because this function is only able to just copy a particular property
    value to the return value, there is no way to check for errors.  We attempt
    to make certain that bad things don't happen by validating that the size of
    the property is the same as the size of the return type, but that can't
    catch all errors.
        This function does call the user's 'get' callback routine still.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    No error checking!
    Use with caution!
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
void *
H5P_peek_voidp(H5P_genplist_t *plist, const char *name)
{
    void * ret_value;            /* return value */

    FUNC_ENTER_NOAPI(H5P_peek_voidp, NULL)

    HDassert(plist);
    HDassert(name);

    /* Get the value to return, don't worry about the return value, we can't return it */
    H5P_get(plist,name,&ret_value);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_peek_voidp() */


/*--------------------------------------------------------------------------
 NAME
    H5P_peek_size_t
 PURPOSE
    Internal routine to quickly retrieve the value of a property in a property list.
 USAGE
    hsize_t H5P_peek_size_t(plist, name)
        H5P_genplist_t *plist;  IN: Property list to check
        const char *name;       IN: Name of property to query
 RETURNS
    Directly returns the value of the property in the list
 DESCRIPTION
        This function directly returns the value of a property in a property
    list.  Because this function is only able to just copy a particular property
    value to the return value, there is no way to check for errors.  We attempt
    to make certain that bad things don't happen by validating that the size of
    the property is the same as the size of the return type, but that can't
    catch all errors.
        This function does call the user's 'get' callback routine still.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    No error checking!
    Use with caution!
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
size_t
H5P_peek_size_t(H5P_genplist_t *plist, const char *name)
{
    size_t ret_value;            /* return value */

    FUNC_ENTER_NOAPI(H5P_peek_size_t, UFAIL)

    HDassert(plist);
    HDassert(name);

    /* Get the value to return, don't worry about the return value, we can't return it */
    H5P_get(plist,name,&ret_value);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_peek_size_t() */


/*--------------------------------------------------------------------------
 NAME
    H5P_get
 PURPOSE
    Internal routine to query the value of a property in a property list.
 USAGE
    herr_t H5P_get(plist, name, value)
        H5P_genplist_t *plist;  IN: Property list to check
        const char *name;       IN: Name of property to query
        void *value;            OUT: Pointer to the buffer for the property value
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Retrieves a copy of the value for a property in a property list.  The
    property name must exist or this routine will fail.  If there is a
    'get' callback routine registered for this property, the copy of the
    value of the property will first be passed to that routine and any changes
    to the copy of the value will be used when returning the property value
    from this routine.
        If the 'get' callback routine returns an error, 'value' will not be
    modified and this routine will return an error.  This routine may not be
    called for zero-sized properties.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_get(const H5P_genplist_t *plist, const char *name, void *value)
{
    H5P_genclass_t *tclass;     /* Temporary class pointer */
    H5P_genprop_t *prop;        /* Temporary property pointer */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5P_get, FAIL)

    HDassert(plist);
    HDassert(name);
    HDassert(value);

    /* Check if the property has been deleted */
    if(H5SL_search(plist->del,name)!=NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "property doesn't exist")

    /* Find property */
    if((prop = (H5P_genprop_t *)H5SL_search(plist->props,name))!=NULL) {
        /* Check for property size >0 */
        if(prop->size==0)
            HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "property has zero size")

        /* Make a copy of the value and pass to 'get' callback */
        if(prop->get!=NULL) {
            void *tmp_value;            /* Temporary value for property */

            /* Make a copy of the current value, in case the callback fails */
            if(NULL==(tmp_value=H5MM_malloc(prop->size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed temporary property value")
            HDmemcpy(tmp_value,prop->value,prop->size);

            /* Call user's callback */
            if((*(prop->get))(plist->plist_id,name,prop->size,tmp_value) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't get property value")

            /* Copy new [possibly unchanged] value into return value */
            HDmemcpy(value,tmp_value,prop->size);

            /* Free the temporary value buffer */
            H5MM_xfree(tmp_value);
        } /* end if */
        /* No 'get' callback, just copy value */
        else
            HDmemcpy(value,prop->value,prop->size);
    } /* end if */
    else {
        /*
         * Check if we should get class properties (up through list of parent classes also),
         * & make property 'get' callback.
         */
        tclass=plist->pclass;
        while(tclass!=NULL) {
            if(tclass->nprops>0) {
                /* Find the property in the class */
                if((prop = (H5P_genprop_t *)H5SL_search(tclass->props,name))!=NULL) {
                    /* Check for property size >0 */
                    if(prop->size==0)
                        HGOTO_ERROR(H5E_PLIST,H5E_BADVALUE,FAIL,"property has zero size")

                    /* Call the 'get' callback, if there is one */
                    if(prop->get!=NULL) {
                        void *tmp_value;            /* Temporary value for property */

                        /* Make a copy of the current value, in case the callback fails */
                        if(NULL==(tmp_value=H5MM_malloc(prop->size)))
                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed temporary property value")
                        HDmemcpy(tmp_value,prop->value,prop->size);

                        /* Call user's callback */
                        if((*(prop->get))(plist->plist_id,name,prop->size,tmp_value) < 0) {
                            H5MM_xfree(tmp_value);
                            HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't set property value")
                        } /* end if */

                        if((prop->cmp)(tmp_value,prop->value,prop->size)) {
                            H5P_genprop_t *pcopy;  /* Copy of property to insert into skip list */

                            /* Make a copy of the class's property */
                            if((pcopy=H5P_dup_prop(prop,H5P_PROP_WITHIN_LIST)) == NULL)
                                HGOTO_ERROR(H5E_PLIST,H5E_CANTCOPY,FAIL,"Can't copy property")

                            /* Copy new value into property value */
                            HDmemcpy(pcopy->value,tmp_value,prop->size);

                            /* Insert the changed property into the property list */
                            if(H5P_add_prop(plist->props,pcopy) < 0)
                                HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL,"Can't insert changed property into skip list")
                        } /* end if */

                        /* Copy new [possibly unchanged] value into return value */
                        HDmemcpy(value,tmp_value,prop->size);

                        /* Free the temporary value buffer */
                        H5MM_xfree(tmp_value);
                    } /* end if */
                    /* No 'get' callback, just copy value */
                    else
                        HDmemcpy(value,prop->value,prop->size);

                    /* Leave */
                    HGOTO_DONE(SUCCEED);
                } /* end while */
            } /* end if */

            /* Go up to parent class */
            tclass=tclass->parent;
        } /* end while */

        /* If we get this far, then it wasn't in the list of changed properties,
         * nor in the properties in the class hierarchy, indicate an error
         */
        HGOTO_ERROR(H5E_PLIST,H5E_NOTFOUND,FAIL,"can't find property in skip list")
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_get() */


/*--------------------------------------------------------------------------
 NAME
    H5P_remove
 PURPOSE
    Internal routine to remove a property from a property list.
 USAGE
    herr_t H5P_remove(plist, name)
        H5P_genplist_t *plist;  IN: Property list to modify
        const char *name;       IN: Name of property to remove
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Removes a property from a property list.  Both properties which were
    in existance when the property list was created (i.e. properties registered
    with H5Pregister2) and properties added to the list after it was created
    (i.e. added with H5Pinsert2) may be removed from a property list.
    Properties do not need to be removed a property list before the list itself
    is closed, they will be released automatically when H5Pclose is called.
    The 'close' callback for this property is called before the property is
    release, if the callback exists.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_remove(hid_t plist_id, H5P_genplist_t *plist, const char *name)
{
    H5P_genclass_t *tclass;     /* Temporary class pointer */
    H5P_genprop_t *prop;        /* Temporary property pointer */
    char *del_name;             /* Pointer to deleted name */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5P_remove, FAIL)

    HDassert(plist);
    HDassert(name);

    /* Indicate that the property isn't in the list if it has been deleted already */
    if(H5SL_search(plist->del,name)!=NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_NOTFOUND,FAIL,"can't find property in skip list")

    /* Get the property node from the changed property skip list */
    if((prop = (H5P_genprop_t *)H5SL_search(plist->props,name))!=NULL) {
        /* Pass value to 'close' callback, if it exists */
        if(prop->del!=NULL) {
            /* Call user's callback */
            if((*(prop->del))(plist_id,name,prop->size,prop->value) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't close property value")
        } /* end if */

        /* Duplicate string for insertion into new deleted property skip list */
        if((del_name=H5MM_xstrdup(name)) == NULL)
            HGOTO_ERROR(H5E_RESOURCE,H5E_NOSPACE,FAIL,"memory allocation failed")

        /* Insert property name into deleted list */
        if(H5SL_insert(plist->del,del_name,del_name) < 0)
            HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into deleted skip list")

        /* Remove the property from the skip list */
        if(H5SL_remove(plist->props,prop->name) == NULL)
            HGOTO_ERROR(H5E_PLIST,H5E_CANTDELETE,FAIL,"can't remove property from skip list")

        /* Free the property, ignoring return value, nothing we can do */
        H5P_free_prop(prop);

        /* Decrement the number of properties in list */
        plist->nprops--;
    } /* end if */
    /* Walk through all the properties in the class hierarchy, looking for the property */
    else {
        /*
         * Check if we should delete class properties (up through list of parent classes also),
         * & make property 'delete' callback.
         */
        tclass=plist->pclass;
        while(tclass!=NULL) {
            if(tclass->nprops>0) {
                /* Find the property in the class */
                if((prop=H5P_find_prop_pclass(tclass,name))!=NULL) {
                    /* Pass value to 'del' callback, if it exists */
                    if(prop->del!=NULL) {
                        void *tmp_value;       /* Temporary value buffer */

                        /* Allocate space for a temporary copy of the property value */
                        if(NULL==(tmp_value=H5MM_malloc(prop->size)))
                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for temporary property value")
                        HDmemcpy(tmp_value,prop->value,prop->size);

                        /* Call user's callback */
                        if((*(prop->del))(plist_id,name,prop->size,tmp_value) < 0) {
                            H5MM_xfree(tmp_value);
                            HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't close property value")
                        } /* end if */

                        /* Release the temporary value buffer */
                        H5MM_xfree(tmp_value);
                    } /* end if */

                    /* Duplicate string for insertion into new deleted property skip list */
                    if((del_name=H5MM_xstrdup(name)) == NULL)
                        HGOTO_ERROR(H5E_RESOURCE,H5E_NOSPACE,FAIL,"memory allocation failed")

                    /* Insert property name into deleted list */
                    if(H5SL_insert(plist->del,del_name,del_name) < 0)
                        HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into deleted skip list")

                    /* Decrement the number of properties in list */
                    plist->nprops--;

                    /* Leave */
                    HGOTO_DONE(SUCCEED);
                } /* end while */
            } /* end if */

            /* Go up to parent class */
            tclass=tclass->parent;
        } /* end while */

        /* If we get this far, then it wasn't in the list of changed properties,
         * nor in the properties in the class hierarchy, indicate an error
         */
        HGOTO_ERROR(H5E_PLIST,H5E_NOTFOUND,FAIL,"can't find property in skip list")
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_remove() */


/*--------------------------------------------------------------------------
 NAME
    H5P_copy_prop_plist
 PURPOSE
    Internal routine to copy a property from one list to another
 USAGE
    herr_t H5P_copy_prop_plist(dst_plist, src_plist, name)
        hid_t dst_id;               IN: ID of destination property list or class
        hid_t src_id;               IN: ID of source property list or class
        const char *name;           IN: Name of property to copy
 RETURNS
    Success: non-negative value.
    Failure: negative value.
 DESCRIPTION
    Copies a property from one property list to another.

    If a property is copied from one list to another, the property will be
    first deleted from the destination list (generating a call to the 'close'
    callback for the property, if one exists) and then the property is copied
    from the source list to the destination list (generating a call to the
    'copy' callback for the property, if one exists).

    If the property does not exist in the destination list, this call is
    equivalent to calling H5Pinsert2 and the 'create' callback will be called
    (if such a callback exists for the property).

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_copy_prop_plist(hid_t dst_id, hid_t src_id, const char *name)
{
    H5P_genplist_t *dst_plist;      /* Pointer to destination property list */
    H5P_genplist_t *src_plist;      /* Pointer to source property list */
    H5P_genprop_t *prop;            /* Temporary property pointer */
    H5P_genprop_t *new_prop=NULL;   /* Pointer to new property */
    herr_t ret_value=SUCCEED;       /* return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_copy_prop_plist)

    HDassert(name);

    /* Get the objects to operate on */
    if(NULL == (src_plist = (H5P_genplist_t *)H5I_object(src_id)) || NULL == (dst_plist = (H5P_genplist_t *)H5I_object(dst_id)))
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "property object doesn't exist")

    /* If the property exists in the destination alread */
    if(H5P_find_prop_plist(dst_plist,name)!=NULL) {
        /* Delete the property from the destination list, calling the 'close' callback if necessary */
        if(H5P_remove(dst_id,dst_plist,name) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTDELETE, FAIL, "unable to remove property")

        /* Get the pointer to the source property */
        prop=H5P_find_prop_plist(src_plist,name);

        /* Make a copy of the source property */
        if((new_prop=H5P_dup_prop(prop,H5P_PROP_WITHIN_LIST)) == NULL)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, FAIL,"Can't copy property")

        /* Call property copy callback, if it exists */
        if(new_prop->copy) {
            if((new_prop->copy)(new_prop->name,new_prop->size,new_prop->value) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, FAIL,"Can't copy property")
        } /* end if */

        /* Insert the initialized property into the property list */
        if(H5P_add_prop(dst_plist->props,new_prop) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL,"Can't insert property into list")

        /* Increment the number of properties in list */
        dst_plist->nprops++;
    } /* end if */
    /* If not, get the information required to do an H5Pinsert2 with the property into the destination list */
    else {
        /* Get the pointer to the source property */
        prop=H5P_find_prop_plist(src_plist,name);

        /* Create property object from parameters */
        if((new_prop=H5P_create_prop(prop->name,prop->size,H5P_PROP_WITHIN_LIST,prop->value,prop->create,prop->set,prop->get,prop->del,prop->copy,prop->cmp,prop->close)) == NULL)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTCREATE, FAIL,"Can't create property")

        /* Call property creation callback, if it exists */
        if(new_prop->create) {
            if((new_prop->create)(new_prop->name,new_prop->size,new_prop->value) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL,"Can't initialize property")
        } /* end if */

        /* Insert property into property list class */
        if(H5P_add_prop(dst_plist->props,new_prop) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL,"Can't insert property into class")

        /* Increment property count for class */
        dst_plist->nprops++;

    } /* end else */

done:
    /* Cleanup, if necessary */
    if(ret_value<0) {
        if(new_prop!=NULL)
            H5P_free_prop(new_prop);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_copy_prop_plist() */


/*--------------------------------------------------------------------------
 NAME
    H5P_copy_prop_pclass
 PURPOSE
    Internal routine to copy a property from one class to another
 USAGE
    herr_t H5P_copy_prop_pclass(dst_pclass, src_pclass, name)
        H5P_genclass_t	*dst_pclass;    IN: Pointer to destination class
        H5P_genclass_t	*src_pclass;    IN: Pointer to source class
        const char *name;               IN: Name of property to copy
 RETURNS
    Success: non-negative value.
    Failure: negative value.
 DESCRIPTION
    Copies a property from one property class to another.

    If a property is copied from one class to another, all the property
    information will be first deleted from the destination class and then the
    property information will be copied from the source class into the
    destination class.

    If the property does not exist in the destination class or list, this call
    is equivalent to calling H5Pregister2.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_copy_prop_pclass(hid_t dst_id, hid_t src_id, const char *name)
{
    H5P_genclass_t *src_pclass;     /* Source property class, containing property to copy */
    H5P_genclass_t *dst_pclass;     /* Destination property class */
    H5P_genclass_t *orig_dst_pclass; /* Original destination property class */
    H5P_genprop_t *prop;            /* Temporary property pointer */
    herr_t ret_value = SUCCEED;     /* return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_copy_prop_pclass)

    /* Sanity check */
    HDassert(name);

    /* Get propery list classes */
    if(NULL == (src_pclass = (H5P_genclass_t *)H5I_object(src_id)))
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "source property class object doesn't exist")
    if(NULL == (dst_pclass = (H5P_genclass_t *)H5I_object(dst_id)))
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "destination property class object doesn't exist")

    /* Get the property from the source */
    if(NULL == (prop = H5P_find_prop_pclass(src_pclass, name)))
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "unable to locate property")

    /* If the property exists in the destination already */
    if(H5P_exist_pclass(dst_pclass, name)) {
        /* Delete the old property from the destination class */
        if(H5P_unregister(dst_pclass, name) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTDELETE, FAIL, "unable to remove property")
    } /* end if */

    /* Register the property into the destination */
    orig_dst_pclass = dst_pclass;
    if(H5P_register(&dst_pclass, name, prop->size, prop->value, prop->create, prop->set, prop->get, prop->del, prop->copy, prop->cmp, prop->close) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTDELETE, FAIL, "unable to remove property")

    /* Check if the property class changed and needs to be substituted in the ID */
    if(dst_pclass != orig_dst_pclass) {
        H5P_genclass_t *old_dst_pclass;     /* Old destination property class */

        /* Substitute the new destination property class in the ID */
        if(NULL == (old_dst_pclass = (H5P_genclass_t *)H5I_subst(dst_id, dst_pclass)))
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to substitute property class in ID")
        HDassert(old_dst_pclass == orig_dst_pclass);

        /* Close the previous class */
        if(H5P_close_class(orig_dst_pclass) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTCLOSEOBJ, FAIL, "unable to close original property class after substitution")
    } /* end if */

done:
    /* Cleanup, if necessary */

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_copy_prop_pclass() */


/*--------------------------------------------------------------------------
 NAME
    H5P_unregister
 PURPOSE
    Internal routine to remove a property from a property list class.
 USAGE
    herr_t H5P_unregister(pclass, name)
        H5P_genclass_t *pclass; IN: Property list class to modify
        const char *name;       IN: Name of property to remove
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Removes a property from a property list class.  Future property lists
    created of that class will not contain this property.  Existing property
    lists containing this property are not affected.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_unregister(H5P_genclass_t *pclass, const char *name)
{
    H5P_genprop_t *prop;        /* Temporary property pointer */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_unregister)

    HDassert(pclass);
    HDassert(name);

    /* Get the property node from the skip list */
    if((prop = (H5P_genprop_t *)H5SL_search(pclass->props,name)) == NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_NOTFOUND,FAIL,"can't find property in skip list")

    /* Remove the property from the skip list */
    if(H5SL_remove(pclass->props,prop->name) == NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTDELETE,FAIL,"can't remove property from skip list")

    /* Free the property, ignoring return value, nothing we can do */
    H5P_free_prop(prop);

    /* Decrement the number of registered properties in class */
    pclass->nprops--;

    /* Update the revision for the class */
    pclass->revision = H5P_GET_NEXT_REV;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_unregister() */


/*--------------------------------------------------------------------------
 NAME
    H5P_close
 PURPOSE
    Internal routine to close a property list.
 USAGE
    herr_t H5P_close(plist)
        H5P_genplist_t *plist;  IN: Property list to close
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Closes a property list.  If a 'close' callback exists for the property
    list class, it is called before the property list is destroyed.  If 'close'
    callbacks exist for any individual properties in the property list, they are
    called after the class 'close' callback.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
        The property list class 'close' callback routine is not called from
    here, it must have been check for and called properly prior to this routine
    being called
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_close(void *_plist)
{
    H5P_genclass_t *tclass;         /* Temporary class pointer */
    H5P_genplist_t *plist=(H5P_genplist_t *)_plist;
    H5SL_t *seen=NULL;              /* Skip list to hold names of properties already seen */
    size_t nseen;                   /* Number of items 'seen' */
    hbool_t has_parent_class;       /* Flag to indicate that this property list's class has a parent */
    size_t ndel;                    /* Number of items deleted */
    H5SL_node_t *curr_node;         /* Current node in skip list */
    H5P_genprop_t *tmp;             /* Temporary pointer to properties */
    unsigned make_cb=0;             /* Operator data for property free callback */
    herr_t ret_value=SUCCEED;       /* return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_close)

    HDassert(plist);

    /* Make call to property list class close callback, if needed
     * (up through chain of parent classes also)
     */
    if(plist->class_init) {
        tclass = plist->pclass;
        while(NULL != tclass) {
            if(NULL != tclass->close_func) {
                /* Call user's "close" callback function, ignoring return value */
                (tclass->close_func)(plist->plist_id, tclass->close_data);
            } /* end if */

            /* Go up to parent class */
            tclass = tclass->parent;
        } /* end while */
    } /* end if */

    /* Create the skip list to hold names of properties already seen
     * (This prevents a property in the class hierarchy from having it's
     * 'close' callback called, if a property in the class hierarchy has
     * already been seen)
     */
    if((seen = H5SL_create(H5SL_TYPE_STR)) == NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTCREATE,FAIL,"can't create skip list for seen properties")
    nseen = 0;

    /* Walk through the changed properties in the list */
    if(H5SL_count(plist->props)>0) {
        curr_node=H5SL_first(plist->props);
        while(curr_node!=NULL) {
            /* Get pointer to property from node */
            tmp = (H5P_genprop_t *)H5SL_item(curr_node);

            /* Call property close callback, if it exists */
            if(tmp->close) {
                /* Call the 'close' callback */
                (tmp->close)(tmp->name,tmp->size,tmp->value);
            } /* end if */

            /* Add property name to "seen" list */
            if(H5SL_insert(seen,tmp->name,tmp->name) < 0)
                HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into seen skip list")
            nseen++;

            /* Get the next property node in the skip list */
            curr_node=H5SL_next(curr_node);
        } /* end while */
    } /* end if */

    /* Determine number of deleted items from property list */
    ndel=H5SL_count(plist->del);

    /*
     * Check if we should remove class properties (up through list of parent classes also),
     * initialize each with default value & make property 'remove' callback.
     */
    tclass=plist->pclass;
    has_parent_class = (hbool_t)(tclass != NULL && tclass->parent != NULL && tclass->parent->nprops > 0);
    while(tclass!=NULL) {
        if(tclass->nprops>0) {
            /* Walk through the properties in the class */
            curr_node=H5SL_first(tclass->props);
            while(curr_node!=NULL) {
                /* Get pointer to property from node */
                tmp = (H5P_genprop_t *)H5SL_item(curr_node);

                /* Only "delete" properties we haven't seen before
                 * and that haven't already been deleted
                 */
                if((nseen==0 || H5SL_search(seen,tmp->name) == NULL) &&
                        (ndel==0 || H5SL_search(plist->del,tmp->name) == NULL)) {

                    /* Call property close callback, if it exists */
                    if(tmp->close) {
                        void *tmp_value;       /* Temporary value buffer */

                        /* Allocate space for a temporary copy of the property value */
                        if(NULL==(tmp_value=H5MM_malloc(tmp->size)))
                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for temporary property value")
                        HDmemcpy(tmp_value,tmp->value,tmp->size);

                        /* Call the 'close' callback */
                        (tmp->close)(tmp->name,tmp->size,tmp_value);

                        /* Release the temporary value buffer */
                        H5MM_xfree(tmp_value);
                    } /* end if */

                    /* Add property name to "seen" list, if we have other classes to work on */
                    if(has_parent_class) {
                        if(H5SL_insert(seen,tmp->name,tmp->name) < 0)
                            HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into seen skip list")
                        nseen++;
                    } /* end if */
                } /* end if */

                /* Get the next property node in the skip list */
                curr_node=H5SL_next(curr_node);
            } /* end while */
        } /* end if */

        /* Go up to parent class */
        tclass=tclass->parent;
    } /* end while */

    /* Decrement class's dependant property list value! */
    if(H5P_access_class(plist->pclass,H5P_MOD_DEC_LST) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "Can't decrement class ref count")

    /* Free the list of 'seen' properties */
    H5SL_close(seen);
    seen=NULL;

    /* Free the list of deleted property names */
    H5SL_destroy(plist->del,H5P_free_del_name_cb,NULL);

    /* Free the properties */
    H5SL_destroy(plist->props,H5P_free_prop_cb,&make_cb);

    /* Destroy property list object */
    plist = H5FL_FREE(H5P_genplist_t, plist);

done:
    /* Release the skip list of 'seen' properties */
    if(seen != NULL)
        H5SL_close(seen);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_close() */


/*--------------------------------------------------------------------------
 NAME
    H5P_get_class_name
 PURPOSE
    Internal routine to query the name of a generic property list class
 USAGE
    char *H5P_get_class_name(pclass)
        H5P_genclass_t *pclass;    IN: Property list class to check
 RETURNS
    Success: Pointer to a malloc'ed string containing the class name
    Failure: NULL
 DESCRIPTION
        This routine retrieves the name of a generic property list class.
    The pointer to the name must be free'd by the user for successful calls.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
char *
H5P_get_class_name(H5P_genclass_t *pclass)
{
    char *ret_value;      /* return value */

    FUNC_ENTER_NOAPI(H5P_get_class_name, NULL)

    HDassert(pclass);

    /* Get class name */
    ret_value=H5MM_xstrdup(pclass->name);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_get_class_name() */


/*--------------------------------------------------------------------------
 NAME
    H5P_get_class_path
 PURPOSE
    Internal routine to query the full path of a generic property list class
 USAGE
    char *H5P_get_class_name(pclass)
        H5P_genclass_t *pclass;    IN: Property list class to check
 RETURNS
    Success: Pointer to a malloc'ed string containing the full path of class
    Failure: NULL
 DESCRIPTION
        This routine retrieves the full path name of a generic property list
    class, starting with the root of the class hierarchy.
    The pointer to the name must be free'd by the user for successful calls.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
char *
H5P_get_class_path(H5P_genclass_t *pclass)
{
    char *par_path;     /* Parent class's full path */
    size_t par_path_len;/* Parent class's full path's length */
    size_t my_path_len; /* This class's name's length */
    char *ret_value;    /* return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_get_class_path)

    HDassert(pclass);

    /* Recursively build the full path */
    if(pclass->parent!=NULL) {
        /* Get the parent class's path */
        par_path=H5P_get_class_path(pclass->parent);
        if(par_path!=NULL) {
            /* Get the string lengths we need to allocate space */
            par_path_len=HDstrlen(par_path);
            my_path_len=HDstrlen(pclass->name);

            /* Allocate enough space for the parent class's path, plus the '/'
             * separator, this class's name and the string terminator
             */
            if(NULL == (ret_value = (char *)H5MM_malloc(par_path_len + 1 + my_path_len + 1)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for class name")

            /* Build the full path for this class */
            HDstrcpy(ret_value,par_path);
            HDstrcat(ret_value,"/");
            HDstrcat(ret_value,pclass->name);

            /* Free the parent class's path */
            H5MM_xfree(par_path);
        } /* end if */
        else
            ret_value=H5MM_xstrdup(pclass->name);
    } /* end if */
    else
        ret_value=H5MM_xstrdup(pclass->name);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_get_class_path() */


/*--------------------------------------------------------------------------
 NAME
    H5P_open_class_path
 PURPOSE
    Internal routine to open [a copy of] a class with its full path name
 USAGE
    H5P_genclass_t *H5P_open_class_path(path)
        const char *path;       IN: Full path name of class to open [copy of]
 RETURNS
    Success: Pointer to a generic property class object
    Failure: NULL
 DESCRIPTION
    This routine opens [a copy] of the class indicated by the full path.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5P_genclass_t *
H5P_open_class_path(const char *path)
{
    char *tmp_path = NULL;      /* Temporary copy of the path */
    char *curr_name;            /* Pointer to current component of path name */
    char *delimit;              /* Pointer to path delimiter during traversal */
    H5P_genclass_t *curr_class; /* Pointer to class during path traversal */
    H5P_genclass_t *ret_value;  /* Return value */
    H5P_check_class_t check_info;   /* Structure to hold the information for checking duplicate names */

    FUNC_ENTER_NOAPI_NOINIT(H5P_open_class_path)

    HDassert(path);

    /* Duplicate the path to use */
    tmp_path = H5MM_xstrdup(path);
    HDassert(tmp_path);

    /* Find the generic property class with this full path */
    curr_name = tmp_path;
    curr_class = NULL;
    while((delimit=HDstrchr(curr_name,'/'))!=NULL) {
        /* Change the delimiter to terminate the string */
        *delimit='\0';

        /* Set up the search structure */
        check_info.parent=curr_class;
        check_info.name=curr_name;

        /* Find the class with this name & parent by iterating over the open classes */
        if(NULL == (curr_class = (H5P_genclass_t *)H5I_search(H5I_GENPROP_CLS, H5P_check_class, &check_info, FALSE)))
            HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, NULL, "can't locate class")

        /* Advance the pointer in the path to the start of the next component */
        curr_name=delimit+1;
    } /* end while */

    /* Should be pointing to the last component in the path name now... */

    /* Set up the search structure */
    check_info.parent = curr_class;
    check_info.name = curr_name;

    /* Find the class with this name & parent by iterating over the open classes */
    if(NULL == (curr_class = (H5P_genclass_t *)H5I_search(H5I_GENPROP_CLS, H5P_check_class, &check_info, FALSE)))
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, NULL, "can't locate class")

    /* Copy it */
    if(NULL == (ret_value = H5P_copy_pclass(curr_class)))
        HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, NULL, "can't copy property class")

done:
    /* Free the duplicated path */
    H5MM_xfree(tmp_path);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_open_class_path() */


/*--------------------------------------------------------------------------
 NAME
    H5P_get_class_parent
 PURPOSE
    Internal routine to query the parent class of a generic property class
 USAGE
    H5P_genclass_t *H5P_get_class_parent(pclass)
        H5P_genclass_t *pclass;    IN: Property class to check
 RETURNS
    Success: Pointer to the parent class of a property class
    Failure: NULL
 DESCRIPTION
    This routine retrieves a pointer to the parent class for a property class.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5P_genclass_t *
H5P_get_class_parent(const H5P_genclass_t *pclass)
{
    H5P_genclass_t *ret_value;      /* return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_get_class_parent)

    HDassert(pclass);

    /* Get property size */
    ret_value = pclass->parent;

    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_get_class_parent() */


/*--------------------------------------------------------------------------
 NAME
    H5P_close_class
 PURPOSE
    Internal routine to close a property list class.
 USAGE
    herr_t H5P_close_class(class)
        H5P_genclass_t *class;  IN: Property list class to close
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Releases memory and de-attach a class from the property list class hierarchy.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_close_class(void *_pclass)
{
    H5P_genclass_t *pclass = (H5P_genclass_t *)_pclass;
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_close_class)

    HDassert(pclass);

    /* Decrement the reference count & check if the object should go away */
    if(H5P_access_class(pclass, H5P_MOD_DEC_REF) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "can't decrement ID ref count")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_close_class() */

