/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the LICENSE file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Purpose:     The Virtual Object Layer as described in documentation.
 *              The purpose is to provide an abstraction on how to access the
 *              underlying HDF5 container, whether in a local file with
 *              a specific file format, or remotely on other machines, etc...
 */

/****************/
/* Module Setup */
/****************/

#include "H5VLmodule.h" /* This source code file is part of the H5VL module */

/***********/
/* Headers */
/***********/

#include "H5private.h"   /* Generic Functions                    */
#include "H5Aprivate.h"  /* Attributes                           */
#include "H5CXprivate.h" /* API Contexts                         */
#include "H5Dprivate.h"  /* Datasets                             */
#include "H5Eprivate.h"  /* Error handling                       */
#include "H5ESprivate.h" /* Event sets                           */
#include "H5Fprivate.h"  /* Files                                */
#include "H5FLprivate.h" /* Free lists                           */
#include "H5Gprivate.h"  /* Groups                               */
#include "H5Iprivate.h"  /* IDs                                  */
#include "H5Mprivate.h"  /* Maps                                 */
#include "H5MMprivate.h" /* Memory management                    */
#include "H5PLprivate.h" /* Plugins                              */
#include "H5Tprivate.h"  /* Datatypes                            */
#include "H5VLpkg.h"     /* Virtual Object Layer                 */

/* VOL connectors */
#include "H5VLnative_private.h"   /* Native VOL connector                 */
#include "H5VLpassthru_private.h" /* Pass-through VOL connector           */

/****************/
/* Local Macros */
/****************/

/******************/
/* Local Typedefs */
/******************/

/* Object wrapping context info for passthrough VOL connectors.
 * Passthrough VOL connectors must wrap objects returned from lower level(s) of the VOL connector stack
 * so that they may be passed back up the stack to the library, and must unwrap objects
 * passed down the stack before providing them to the next lower VOL connector.
 * This is generally done individually within each VOL object callback. However, the library sometimes
 * needs to wrap objects from places that don't pass through the VOL layer.
 * In this case, the wrap callbacks defined in H5VL_wrap_class_t are used, and the VOL-defined wrap context
 * (obj_wrap_ctx) provides necessary information - at a minimum, the object originally returned by the lower
 * VOL connector, and the ID of the next VOL connector.
 */
typedef struct H5VL_wrap_ctx_t {
    unsigned          rc;           /* Ref. count for the # of times the context was set / reset */
    H5VL_connector_t *connector;    /* VOL connector for "outermost" class to start wrap */
    void             *obj_wrap_ctx; /* "wrap context" for outermost connector */
} H5VL_wrap_ctx_t;

/* Information needed for iterating over the registered VOL connector hid_t IDs.
 * The name or value of the new VOL connector that is being registered is
 * stored in the name (or value) field and the found_id field is initialized to
 * H5I_INVALID_HID (-1).  If we find a VOL connector with the same name / value,
 * we set the found_id field to the existing ID for return to the function.
 */
typedef struct {
    /* IN */
    H5PL_vol_key_t key;

    /* OUT */
    hid_t found_id; /* The connector ID, if we found a match */
} H5VL_get_connector_ud_t;

/********************/
/* Package Typedefs */
/********************/

/********************/
/* Local Prototypes */
/********************/
static herr_t            H5VL__free_cls(H5VL_class_t *cls);
static void             *H5VL__wrap_obj(void *obj, H5I_type_t obj_type);
static H5VL_connector_t *H5VL__conn_create(H5VL_class_t *cls);
static herr_t            H5VL__conn_find(H5PL_vol_key_t *key, H5VL_connector_t **connector);
static herr_t            H5VL__conn_free(H5VL_connector_t *connector);
static herr_t            H5VL__conn_free_id(H5VL_connector_t *connector, void H5_ATTR_UNUSED **request);
static void             *H5VL__object(hid_t id, H5I_type_t obj_type);
static herr_t            H5VL__free_vol_wrapper(H5VL_wrap_ctx_t *vol_wrap_ctx);

/*********************/
/* Package Variables */
/*********************/

/* Package initialization variable */
bool H5_PKG_INIT_VAR = false;

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/

/* VOL ID class */
static const H5I_class_t H5I_VOL_CLS[1] = {{
    H5I_VOL,                       /* ID class value */
    0,                             /* Class flags */
    0,                             /* # of reserved IDs for class */
    (H5I_free_t)H5VL__conn_free_id /* Callback routine for closing objects of this class */
}};

/* Declare a free list to manage the H5VL_class_t struct */
H5FL_DEFINE_STATIC(H5VL_class_t);

/* Declare a free list to manage the H5VL_connector_t struct */
H5FL_DEFINE_STATIC(H5VL_connector_t);

/* Declare a free list to manage the H5VL_object_t struct */
H5FL_DEFINE(H5VL_object_t);

/* Declare a free list to manage the H5VL_wrap_ctx_t struct */
H5FL_DEFINE_STATIC(H5VL_wrap_ctx_t);

/* List of currently active VOL connectors */
static H5VL_connector_t *H5VL_conn_list_head_g = NULL;

/* Default VOL connector */
static H5VL_connector_prop_t H5VL_def_conn_s = {NULL, NULL};

/*-------------------------------------------------------------------------
 * Function:    H5VL_init_phase1
 *
 * Purpose:     Initialize the interface from some other package.  This should
 *		be followed with a call to H5VL_init_phase2 after the H5P
 *		interface is completely set up, finish setting up the H5VL
 *		information.
 *
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_init_phase1(void)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* FUNC_ENTER() does all the work */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_init_phase1() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_init_phase2
 *
 * Purpose:     Finish initializing the interface from some other package.
 *
 * Note:	This is broken out as a separate routine to avoid a circular
 *		reference with the H5P package.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_init_phase2(void)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Initialize all packages for VOL-managed objects */
    if (H5T_init() < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTINIT, FAIL, "unable to initialize datatype interface");
    if (H5D_init() < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTINIT, FAIL, "unable to initialize dataset interface");
    if (H5F_init() < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTINIT, FAIL, "unable to initialize file interface");
    if (H5G_init() < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTINIT, FAIL, "unable to initialize group interface");
    if (H5A_init() < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTINIT, FAIL, "unable to initialize attribute interface");
    if (H5M_init() < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTINIT, FAIL, "unable to initialize map interface");

    /* Register internal VOL connectors */
    if (H5VL__native_register() < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTINIT, FAIL, "unable to register native VOL connector");
    if (H5VL__passthru_register() < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTINIT, FAIL, "unable to register passthru VOL connector");

    /* Sanity check default VOL connector */
    assert(H5VL_def_conn_s.connector == NULL);
    assert(H5VL_def_conn_s.connector_info == NULL);

    /* Set up the default VOL connector in the default FAPL */
    if (H5VL__set_def_conn() < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "unable to set default VOL connector");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_init_phase2() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__init_package
 *
 * Purpose:     Initialize interface-specific information
 *
 * Return:      Success:    Non-negative
 *
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__init_package(void)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Initialize the ID group for the VL IDs */
    if (H5I_register_type(H5I_VOL_CLS) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTINIT, FAIL, "unable to initialize H5VL interface");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__init_package() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_term_package
 *
 * Purpose:     Terminate various H5VL objects
 *
 * Return:      Success:    Positive if anything was done that might
 *                          affect other interfaces; zero otherwise.
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
int
H5VL_term_package(void)
{
    int n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if (H5_PKG_INIT_VAR) {
        if (H5VL_def_conn_s.connector) {
            /* Release the default VOL connector */
            (void)H5VL_conn_prop_free(&H5VL_def_conn_s);
            H5VL_def_conn_s.connector      = NULL;
            H5VL_def_conn_s.connector_info = NULL;
            n++;
        } /* end if */
        else {
            if (H5I_nmembers(H5I_VOL) > 0) {
                /* Unregister all VOL connectors */
                (void)H5I_clear_type(H5I_VOL, true, false);

                /* Reset internal VOL connectors' global vars */
                (void)H5VL__native_unregister();
                (void)H5VL__passthru_unregister();

                n++;
            } /* end if */
            else {
                if (H5VL__num_opt_operation() > 0) {
                    /* Unregister all dynamically registered optional operations */
                    (void)H5VL__term_opt_operation();
                    n++;
                } /* end if */
                else {
                    /* Destroy the VOL connector ID group */
                    n += (H5I_dec_type_ref(H5I_VOL) > 0);

                    /* Mark interface as closed */
                    if (0 == n)
                        H5_PKG_INIT_VAR = false;
                } /* end else */
            }     /* end else */
        }         /* end else */
    }             /* end if */

    FUNC_LEAVE_NOAPI(n)
} /* end H5VL_term_package() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__free_cls
 *
 * Purpose:     Frees a file VOL class struct and returns an indication of
 *              success. This function is used as the free callback for the
 *              virtual object layer object identifiers
 *              (c.f.: H5VL_init_interface).
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__free_cls(H5VL_class_t *cls)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    assert(cls);

    /* Shut down the VOL connector */
    if (cls->terminate) {
        /* Prepare & restore library for user callback */
        H5_BEFORE_USER_CB(FAIL)
            {
                ret_value = cls->terminate();
            }
        H5_AFTER_USER_CB(FAIL)
        if (ret_value < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTCLOSEOBJ, FAIL, "VOL connector did not terminate cleanly");
    }

    /* Release the class */
    H5MM_xfree_const(cls->name);
    H5FL_FREE(H5VL_class_t, cls);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__free_cls() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__set_def_conn
 *
 * Purpose:     Parses a string that contains the default VOL connector for
 *              the library.
 *
 * Note:	Usually from the environment variable "HDF5_VOL_CONNECTOR",
 *		but could be from elsewhere.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__set_def_conn(void)
{
    H5P_genplist_t   *def_fapl;            /* Default file access property list */
    H5P_genclass_t   *def_fapclass;        /* Default file access property class */
    const char       *env_var;             /* Environment variable for default VOL connector */
    char             *buf       = NULL;    /* Buffer for tokenizing string */
    H5VL_connector_t *connector = NULL;    /* VOL connector */
    void             *vol_info  = NULL;    /* VOL connector info */
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Reset default VOL connector, if it's set already */
    /* (Can happen during testing -QAK) */
    if (H5VL_def_conn_s.connector) {
        /* Release the default VOL connector */
        (void)H5VL_conn_prop_free(&H5VL_def_conn_s);
        H5VL_def_conn_s.connector      = NULL;
        H5VL_def_conn_s.connector_info = NULL;
    } /* end if */

    /* Check for environment variable set */
    env_var = getenv(HDF5_VOL_CONNECTOR);

    /* Only parse the string if it's set */
    if (env_var && *env_var) {
        char       *lasts = NULL;            /* Context pointer for strtok_r() call */
        const char *tok   = NULL;            /* Token from strtok_r call */
        htri_t      connector_is_registered; /* Whether connector is already registered */

        /* Duplicate the string to parse, as it is modified as we go */
        if (NULL == (buf = H5MM_strdup(env_var)))
            HGOTO_ERROR(H5E_VOL, H5E_CANTALLOC, FAIL,
                        "can't allocate memory for environment variable string");

        /* Get the first 'word' of the environment variable.
         * If it's nothing (environment variable was whitespace) return error.
         */
        if (NULL == (tok = HDstrtok_r(buf, " \t\n\r", &lasts)))
            HGOTO_ERROR(H5E_VOL, H5E_BADVALUE, FAIL, "VOL connector environment variable set empty?");

        /* First, check to see if the connector is already registered */
        if ((connector_is_registered = H5VL__is_connector_registered_by_name(tok)) < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't check if VOL connector already registered");
        else if (connector_is_registered) {
            /* Retrieve the ID of the already-registered VOL connector */
            if (NULL == (connector = H5VL__get_connector_by_name(tok)))
                HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't get VOL connector ID");
        } /* end else-if */
        else {
            /* Check for VOL connectors that ship with the library */
            if (!strcmp(tok, "native")) {
                connector = H5VL_NATIVE_conn_g;

                /* Inc. refcount on connector object, so it can be uniformly released */
                H5VL_conn_inc_rc(connector);
            } /* end if */
            else if (!strcmp(tok, "pass_through")) {
                connector = H5VL_PASSTHRU_conn_g;

                /* Inc. refcount on connector object, so it can be uniformly released */
                H5VL_conn_inc_rc(connector);
            } /* end else-if */
            else {
                /* Register the VOL connector */
                /* (NOTE: No provisions for vipl_id currently) */
                if (NULL == (connector = H5VL__register_connector_by_name(tok, H5P_VOL_INITIALIZE_DEFAULT)))
                    HGOTO_ERROR(H5E_VOL, H5E_CANTREGISTER, FAIL, "can't register connector");
            } /* end else */
        }     /* end else */

        /* Was there any connector info specified in the environment variable? */
        if (NULL != (tok = HDstrtok_r(NULL, "\n\r", &lasts)))
            if (H5VL__connector_str_to_info(tok, connector, &vol_info) < 0)
                HGOTO_ERROR(H5E_VOL, H5E_CANTDECODE, FAIL, "can't deserialize connector info");

        /* Set the default VOL connector */
        H5VL_def_conn_s.connector      = connector;
        H5VL_def_conn_s.connector_info = vol_info;
    } /* end if */
    else {
        /* Set the default VOL connector */
        H5VL_def_conn_s.connector      = H5_DEFAULT_VOL;
        H5VL_def_conn_s.connector_info = NULL;

        /* Increment the ref count on the default connector */
        H5VL_conn_inc_rc(H5VL_def_conn_s.connector);
    } /* end else */

    /* Get default file access pclass */
    if (NULL == (def_fapclass = H5I_object(H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_VOL, H5E_BADID, FAIL, "can't find object for default file access property class ID");

    /* Change the default VOL for the default file access pclass */
    if (H5P_reset_vol_class(def_fapclass, &H5VL_def_conn_s) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL,
                    "can't set default VOL connector for default file access property class");

    /* Get default file access plist */
    if (NULL == (def_fapl = H5I_object(H5P_FILE_ACCESS_DEFAULT)))
        HGOTO_ERROR(H5E_VOL, H5E_BADID, FAIL, "can't find object for default fapl ID");

    /* Change the default VOL for the default FAPL */
    if (H5P_set_vol(def_fapl, H5VL_def_conn_s.connector, H5VL_def_conn_s.connector_info) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set default VOL connector for default FAPL");

done:
    /* Clean up on error */
    if (ret_value < 0) {
        if (vol_info)
            if (H5VL_free_connector_info(connector, vol_info) < 0)
                HDONE_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "can't free VOL connector info");
        if (connector)
            /* The H5VL_connector_t struct will be freed by this function */
            if (H5VL_conn_dec_rc(connector) < 0)
                HDONE_ERROR(H5E_VOL, H5E_CANTDEC, FAIL, "unable to unregister VOL connector");
    } /* end if */

    /* Clean up */
    H5MM_xfree(buf);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__set_def_conn() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__wrap_obj
 *
 * Purpose:     Wraps a library object with possible VOL connector wrappers, to
 *		match the VOL connector stack for the file.
 *
 * Return:      Success:        Wrapped object pointer
 *              Failure:        NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL__wrap_obj(void *obj, H5I_type_t obj_type)
{
    H5VL_wrap_ctx_t *vol_wrap_ctx = NULL; /* Object wrapping context */
    void            *ret_value    = NULL; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check arguments */
    assert(obj);

    /* Retrieve the VOL object wrapping context */
    if (H5CX_get_vol_wrap_ctx((void **)&vol_wrap_ctx) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, NULL, "can't get VOL object wrap context");

    /* If there is a VOL object wrapping context, wrap the object */
    if (vol_wrap_ctx) {
        /* Wrap object, using the VOL callback */
        if (NULL == (ret_value = H5VL_wrap_object(vol_wrap_ctx->connector->cls, vol_wrap_ctx->obj_wrap_ctx,
                                                  obj, obj_type)))
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, NULL, "can't wrap object");
    } /* end if */
    else
        ret_value = obj;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__wrap_obj() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_new_vol_obj
 *
 * Purpose:     Creates a new VOL object, to use when registering an ID.
 *
 * Return:      Success:        VOL object pointer
 *              Failure:        NULL
 *
 *-------------------------------------------------------------------------
 */
H5VL_object_t *
H5VL_new_vol_obj(H5I_type_t type, void *object, H5VL_connector_t *connector, bool wrap_obj)
{
    H5VL_object_t *new_vol_obj  = NULL;  /* Pointer to new VOL object                    */
    bool           conn_rc_incr = false; /* Whether the VOL connector refcount has been incremented */
    H5VL_object_t *ret_value    = NULL;  /* Return value                                 */

    FUNC_ENTER_NOAPI(NULL)

    /* Check arguments */
    assert(object);
    assert(connector);

    /* Make sure type number is valid */
    if (type != H5I_ATTR && type != H5I_DATASET && type != H5I_DATATYPE && type != H5I_FILE &&
        type != H5I_GROUP && type != H5I_MAP)
        HGOTO_ERROR(H5E_VOL, H5E_BADVALUE, NULL, "invalid type number");

    /* Create the new VOL object */
    if (NULL == (new_vol_obj = H5FL_CALLOC(H5VL_object_t)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTALLOC, NULL, "can't allocate memory for VOL object");
    new_vol_obj->connector = connector;
    if (wrap_obj) {
        if (NULL == (new_vol_obj->data = H5VL__wrap_obj(object, type)))
            HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, NULL, "can't wrap library object");
    } /* end if */
    else
        new_vol_obj->data = object;
    new_vol_obj->rc = 1;

    /* Bump the reference count on the VOL connector */
    H5VL_conn_inc_rc(connector);
    conn_rc_incr = true;

    /* If this is a datatype, we have to hide the VOL object under the H5T_t pointer */
    if (H5I_DATATYPE == type) {
        if (NULL == (ret_value = (H5VL_object_t *)H5T_construct_datatype(new_vol_obj)))
            HGOTO_ERROR(H5E_VOL, H5E_CANTINIT, NULL, "can't construct datatype object");
    } /* end if */
    else
        ret_value = (H5VL_object_t *)new_vol_obj;

done:
    /* Cleanup on error */
    if (NULL == ret_value) {
        if (conn_rc_incr && H5VL_conn_dec_rc(connector) < 0)
            HDONE_ERROR(H5E_VOL, H5E_CANTDEC, NULL, "unable to decrement ref count on VOL connector");

        if (new_vol_obj) {
            if (wrap_obj && new_vol_obj->data)
                (void)H5VL_object_unwrap(new_vol_obj);
            (void)H5FL_FREE(H5VL_object_t, new_vol_obj);
        }
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_new_vol_obj() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_conn_prop_copy
 *
 * Purpose:     Copy VOL connector property.
 *
 * Note:        This is an "in-place" copy.
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_conn_prop_copy(H5VL_connector_prop_t *connector_prop)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if (connector_prop) {
        /* Copy the connector ID & info, if there is one */
        if (connector_prop->connector) {
            /* Increment the reference count on connector */
            H5VL_conn_inc_rc(connector_prop->connector);

            /* Copy connector info, if it exists */
            if (connector_prop->connector_info) {
                void *new_connector_info = NULL; /* Copy of connector info */

                /* Allocate and copy connector info */
                if (H5VL_copy_connector_info(connector_prop->connector, &new_connector_info,
                                             connector_prop->connector_info) < 0)
                    HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, FAIL, "connector info copy failed");

                /* Set the connector info to the copy */
                connector_prop->connector_info = new_connector_info;
            } /* end if */
        }     /* end if */
    }         /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_conn_prop_copy() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_conn_prop_cmp
 *
 * Purpose:     Compare two VOL connector properties.
 *
 * Return:      positive if PROP1 is greater than PROP2, negative if PROP2
 *              is greater than PROP1 and zero if PROP1 and PROP2 are equal.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_conn_prop_cmp(int *cmp_value, const H5VL_connector_prop_t *prop1, const H5VL_connector_prop_t *prop2)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments */
    assert(cmp_value);
    assert(prop1);
    assert(prop2);

    /* Fast check */
    if (prop1 == prop2)
        /* Set output comparison value */
        *cmp_value = 0;
    else {
        H5VL_connector_t *conn1, *conn2;     /* Connector for each property */
        int               tmp_cmp_value = 0; /* Value from comparison */

        /* Compare connectors' classes */
        conn1 = prop1->connector;
        conn2 = prop2->connector;
        if (H5VL_cmp_connector_cls(&tmp_cmp_value, conn1->cls, conn2->cls) < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTCOMPARE, FAIL, "can't compare connector classes");
        if (tmp_cmp_value != 0)
            /* Set output comparison value */
            *cmp_value = tmp_cmp_value;
        else {
            /* At this point, we should be able to assume that we are dealing with
             * the same connector class struct (or a copies of the same class struct)
             */

            /* Use one of the classes (cls1) info comparison routines to compare the
             * info objects
             */
            assert(conn1->cls->info_cls.cmp == conn2->cls->info_cls.cmp);
            tmp_cmp_value = 0;
            if (H5VL_cmp_connector_info(conn1, &tmp_cmp_value, prop1->connector_info, prop2->connector_info) <
                0)
                HGOTO_ERROR(H5E_VOL, H5E_CANTCOMPARE, FAIL, "can't compare connector class info");

            /* Set output comparison value */
            *cmp_value = tmp_cmp_value;
        }
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_conn_prop_same() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_conn_prop_free
 *
 * Purpose:     Free VOL connector property
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_conn_prop_free(const H5VL_connector_prop_t *connector_prop)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if (connector_prop) {
        /* Free the connector info (if it exists) and decrement the ID */
        if (connector_prop->connector) {
            if (connector_prop->connector_info)
                /* Free the connector info */
                if (H5VL_free_connector_info(connector_prop->connector, connector_prop->connector_info) < 0)
                    HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL,
                                "unable to release VOL connector info object");

            /* Decrement reference count for connector ID */
            if (H5VL_conn_dec_rc(connector_prop->connector) < 0)
                HGOTO_ERROR(H5E_VOL, H5E_CANTDEC, FAIL, "can't decrement reference count for connector");
        }
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_conn_prop_free() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_register
 *
 * Purpose:     VOL-aware version of H5I_register. Constructs an H5VL_object_t
 *              from the passed-in object and registers that. Does the right
 *              thing with datatypes, which are complicated under the VOL.
 *
 * Return:      Success:    A valid HDF5 ID
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5VL_register(H5I_type_t type, void *object, H5VL_connector_t *vol_connector, bool app_ref)
{
    H5VL_object_t *vol_obj   = NULL;            /* VOL object wrapper for library object */
    hid_t          ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_NOAPI(H5I_INVALID_HID)

    /* Check arguments */
    assert(object);
    assert(vol_connector);

    /* Set up VOL object for the passed-in data */
    /* (Does not wrap object, since it's from a VOL callback) */
    if (NULL == (vol_obj = H5VL_new_vol_obj(type, object, vol_connector, false)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, H5I_INVALID_HID, "can't create VOL object");

    /* Register VOL object as _object_ type, for future object API calls */
    if ((ret_value = H5I_register(type, vol_obj, app_ref)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to register handle");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_register() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_register_using_existing_id
 *
 * Purpose:     Registers an OBJECT in a TYPE with the supplied ID for it.
 *              This routine will check to ensure the supplied ID is not already
 *              in use, and ensure that it is a valid ID for the given type,
 *              but will NOT check to ensure the OBJECT is not already
 *              registered (thus, it is possible to register one object under
 *              multiple IDs).
 *
 * NOTE:        Intended for use in refresh calls, where we have to close
 *              and re-open the underlying data, then hook the VOL object back
 *              up to the original ID.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_register_using_existing_id(H5I_type_t type, void *object, H5VL_connector_t *vol_connector, bool app_ref,
                                hid_t existing_id)
{
    H5VL_object_t *new_vol_obj = NULL;    /* Pointer to new VOL object                    */
    herr_t         ret_value   = SUCCEED; /* Return value                                 */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments */
    assert(object);
    assert(vol_connector);

    /* Set up VOL object for the passed-in data */
    /* (Wraps object, since it's a library object) */
    if (NULL == (new_vol_obj = H5VL_new_vol_obj(type, object, vol_connector, true)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, FAIL, "can't create VOL object");

    /* Call the underlying H5I function to complete the registration */
    if (H5I_register_using_existing_id(type, new_vol_obj, app_ref, existing_id) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTREGISTER, FAIL, "can't register object under existing ID");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_register_using_existing_id() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_create_object
 *
 * Purpose:     Similar to H5VL_register but does not create an ID.
 *              Creates a new VOL object for the provided generic object
 *              using the provided vol connector.  Should only be used for
 *              internal objects returned from the connector such as
 *              requests.
 *
 * Return:      Success:    A valid VOL object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
H5VL_object_t *
H5VL_create_object(void *object, H5VL_connector_t *vol_connector)
{
    H5VL_object_t *ret_value = NULL; /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Check arguments */
    assert(object);
    assert(vol_connector);

    /* Set up VOL object for the passed-in data */
    /* (Does not wrap object, since it's from a VOL callback) */
    if (NULL == (ret_value = H5FL_CALLOC(H5VL_object_t)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTALLOC, NULL, "can't allocate memory for VOL object");
    ret_value->connector = vol_connector;
    ret_value->data      = object;
    ret_value->rc        = 1;

    /* Bump the reference count on the VOL connector */
    H5VL_conn_inc_rc(vol_connector);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_create_object() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__conn_create
 *
 * Purpose:     Utility function to create a connector around a class
 *
 * Return:      Success:    Pointer to a new connector object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static H5VL_connector_t *
H5VL__conn_create(H5VL_class_t *cls)
{
    H5VL_connector_t *connector = NULL; /* New VOL connector struct */
    H5VL_connector_t *ret_value = NULL; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    assert(cls);

    /* Setup VOL info struct */
    if (NULL == (connector = H5FL_CALLOC(H5VL_connector_t)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTALLOC, NULL, "can't allocate VOL connector struct");
    connector->cls = cls;

    /* Add connector to list of active VOL connectors */
    if (H5VL_conn_list_head_g) {
        connector->next             = H5VL_conn_list_head_g;
        H5VL_conn_list_head_g->prev = connector;
    }
    H5VL_conn_list_head_g = connector;

    /* Set return value */
    ret_value = connector;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__conn_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_conn_register
 *
 * Purpose:     Registers an existing VOL connector with a new ID
 *
 * Return:      Success:    VOL connector ID
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5VL_conn_register(H5VL_connector_t *connector)
{
    hid_t ret_value = H5I_INVALID_HID;

    FUNC_ENTER_NOAPI(H5I_INVALID_HID)

    /* Check arguments */
    assert(connector);

    /* Create a ID for the connector */
    if ((ret_value = H5I_register(H5I_VOL, connector, true)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to register VOL connector ID");

    /* ID is holding a reference to the connector */
    H5VL_conn_inc_rc(connector);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_conn_register() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__conn_find
 *
 * Purpose:     Find a matching connector
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__conn_find(H5PL_vol_key_t *key, H5VL_connector_t **connector)
{
    H5VL_connector_t *node; /* Current node in linked list */

    FUNC_ENTER_PACKAGE_NOERR

    /* Check arguments */
    assert(key);
    assert(connector);

    /* Iterate over linked list of active connectors */
    node = H5VL_conn_list_head_g;
    while (node) {
        if (H5VL_GET_CONNECTOR_BY_NAME == key->kind) {
            if (0 == strcmp(node->cls->name, key->u.name)) {
                *connector = node;
                break;
            } /* end if */
        }     /* end if */
        else {
            assert(H5VL_GET_CONNECTOR_BY_VALUE == key->kind);
            if (node->cls->value == key->u.value) {
                *connector = node;
                break;
            } /* end if */
        }     /* end else */

        /* Advance to next node */
        node = node->next;
    }

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5VL__conn_find() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_conn_inc_rc
 *
 * Purpose:     Wrapper to increment the ref. count on a connector.
 *
 * Return:      Current ref. count (can't fail)
 *
 *-------------------------------------------------------------------------
 */
int64_t
H5VL_conn_inc_rc(H5VL_connector_t *connector)
{
    int64_t ret_value = -1;

    FUNC_ENTER_NOAPI(-1)

    /* Check arguments */
    assert(connector);

    /* Increment refcount for connector */
    connector->nrefs++;

    /* Set return value */
    ret_value = connector->nrefs;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_conn_inc_rc() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_conn_dec_rc
 *
 * Purpose:     Wrapper to decrement the ref. count on a connector.
 *
 * Return:      Current ref. count (>=0) on success, <0 on failure
 *
 *-------------------------------------------------------------------------
 */
int64_t
H5VL_conn_dec_rc(H5VL_connector_t *connector)
{
    int64_t ret_value = -1; /* Return value */

    FUNC_ENTER_NOAPI(-1)

    /* Check arguments */
    assert(connector);

    /* Decrement refcount for connector */
    connector->nrefs--;

    /* Set return value */
    ret_value = connector->nrefs;

    /* Check for last reference */
    if (0 == connector->nrefs)
        if (H5VL__conn_free(connector) < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "unable to free VOL connector");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_conn_dec_rc() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_conn_same_class
 *
 * Purpose:     Determine if two connectors point to the same VOL class
 *
 * Return:      true/false/FAIL
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5VL_conn_same_class(const H5VL_connector_t *conn1, const H5VL_connector_t *conn2)
{
    htri_t ret_value = FAIL; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments */
    assert(conn1);
    assert(conn2);

    /* Fast check */
    if (conn1 == conn2)
        HGOTO_DONE(true);
    else {
        int cmp_value = 0; /* Comparison result */

        /* Compare connector classes */
        if (H5VL_cmp_connector_cls(&cmp_value, conn1->cls, conn2->cls) < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTCOMPARE, FAIL, "can't compare connector classes");
        ret_value = (0 == cmp_value);
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_conn_same_class() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__conn_free
 *
 * Purpose:     Free a connector object
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__conn_free(H5VL_connector_t *connector)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check arguments */
    assert(connector);
    assert(0 == connector->nrefs);

    /* Remove connector from list of active VOL connectors */
    if (H5VL_conn_list_head_g == connector) {
        H5VL_conn_list_head_g = H5VL_conn_list_head_g->next;
        if (H5VL_conn_list_head_g)
            H5VL_conn_list_head_g->prev = NULL;
    }
    else {
        if (connector->prev)
            connector->prev->next = connector->next;
        if (connector->next)
            connector->next->prev = connector->prev;
    }

    if (H5VL__free_cls(connector->cls) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "can't free VOL class");

    H5FL_FREE(H5VL_connector_t, connector);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__conn_free() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__conn_free_id
 *
 * Purpose:     Shim for freeing connector ID
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__conn_free_id(H5VL_connector_t *connector, void H5_ATTR_UNUSED **request)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check arguments */
    assert(connector);

    /* Decrement refcount on connector */
    if (H5VL_conn_dec_rc(connector) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTDEC, FAIL, "unable to decrement ref count on VOL connector");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__conn_free_id() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_object_inc_rc
 *
 * Purpose:     Wrapper to increment the ref count on a VOL object.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
hsize_t
H5VL_object_inc_rc(H5VL_object_t *vol_obj)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check arguments */
    assert(vol_obj);

    /* Increment refcount for object and return */
    FUNC_LEAVE_NOAPI(++vol_obj->rc)
} /* end H5VL_object_inc_rc() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_free_object
 *
 * Purpose:     Wrapper to unregister an object ID with a VOL aux struct
 *              and decrement ref count on VOL connector ID
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_free_object(H5VL_object_t *vol_obj)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments */
    assert(vol_obj);

    if (--vol_obj->rc == 0) {
        /* Decrement refcount on connector */
        if (H5VL_conn_dec_rc(vol_obj->connector) < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTDEC, FAIL, "unable to decrement ref count on VOL connector");

        vol_obj = H5FL_FREE(H5VL_object_t, vol_obj);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_free_object() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_object_is_native
 *
 * Purpose:     Query if an object is (if it's a file object) / is in (if its
 *              an object) a native connector's file.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_object_is_native(const H5VL_object_t *obj, bool *is_native)
{
    const H5VL_class_t *cls;                 /* VOL connector class structs for object */
    H5VL_connector_t   *native;              /* Native VOL connector */
    int                 cmp_value = 0;       /* Comparison result */
    herr_t              ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments */
    assert(obj);
    assert(is_native);

    /* Retrieve the terminal connector class for the object */
    cls = NULL;
    if (H5VL_introspect_get_conn_cls(obj, H5VL_GET_CONN_LVL_TERM, &cls) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't get VOL connector class");

    /* Retrieve the native connector class */
    native = H5VL_NATIVE_conn_g;

    /* Compare connector classes */
    if (H5VL_cmp_connector_cls(&cmp_value, cls, native->cls) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCOMPARE, FAIL, "can't compare connector classes");

    /* If classes compare equal, then the object is / is in a native connector's file */
    *is_native = (cmp_value == 0);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_object_is_native() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_file_is_same
 *
 * Purpose:     Query if two files are the same.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_file_is_same(const H5VL_object_t *vol_obj1, const H5VL_object_t *vol_obj2, bool *same_file)
{
    const H5VL_class_t *cls1;                /* VOL connector class struct for first object */
    const H5VL_class_t *cls2;                /* VOL connector class struct for second object */
    int                 cmp_value = 0;       /* Comparison result */
    herr_t              ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments */
    assert(vol_obj1);
    assert(vol_obj2);
    assert(same_file);

    /* Retrieve the terminal connectors for each object */
    cls1 = NULL;
    if (H5VL_introspect_get_conn_cls(vol_obj1, H5VL_GET_CONN_LVL_TERM, &cls1) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't get VOL connector class");
    cls2 = NULL;
    if (H5VL_introspect_get_conn_cls(vol_obj2, H5VL_GET_CONN_LVL_TERM, &cls2) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't get VOL connector class");

    /* Compare connector classes */
    if (H5VL_cmp_connector_cls(&cmp_value, cls1, cls2) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCOMPARE, FAIL, "can't compare connector classes");

    /* If the connector classes are different, the files are different */
    if (cmp_value)
        *same_file = false;
    else {
        void                     *obj2;        /* Terminal object for second file */
        H5VL_file_specific_args_t vol_cb_args; /* Arguments to VOL callback */

        /* Get unwrapped (terminal) object for vol_obj2 */
        if (NULL == (obj2 = H5VL_object_data(vol_obj2)))
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't get unwrapped object");

        /* Set up VOL callback arguments */
        vol_cb_args.op_type                 = H5VL_FILE_IS_EQUAL;
        vol_cb_args.args.is_equal.obj2      = obj2;
        vol_cb_args.args.is_equal.same_file = same_file;

        /* Make 'are files equal' callback */
        if (H5VL_file_specific(vol_obj1, &vol_cb_args, H5P_DATASET_XFER_DEFAULT, NULL) < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "file specific failed");
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_file_is_same() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__register_connector
 *
 * Purpose:     Registers a new VOL connector as a member of the virtual object
 *              layer class.
 *
 * Return:      Success:    A pointer to a VOL connector
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
H5VL_connector_t *
H5VL__register_connector(const H5VL_class_t *cls, hid_t vipl_id)
{
    H5VL_connector_t *connector = NULL;
    H5VL_class_t     *saved     = NULL;
    bool              init_done = false;
    H5VL_connector_t *ret_value = NULL;

    FUNC_ENTER_PACKAGE

    /* Check arguments */
    assert(cls);

    /* Copy the class structure so the caller can reuse or free it */
    if (NULL == (saved = H5FL_MALLOC(H5VL_class_t)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTALLOC, NULL, "memory allocation failed for VOL connector class struct");
    H5MM_memcpy(saved, cls, sizeof(H5VL_class_t));
    if (NULL == (saved->name = H5MM_strdup(cls->name)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTALLOC, NULL, "memory allocation failed for VOL connector name");

    /* Initialize the VOL connector */
    if (cls->initialize) {
        herr_t status;

        /* Prepare & restore library for user callback */
        H5_BEFORE_USER_CB(NULL)
            {
                status = cls->initialize(vipl_id);
            }
        H5_AFTER_USER_CB(NULL)
        if (status < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTINIT, NULL, "unable to init VOL connector");
    }
    init_done = true;

    /* Create new connector for the class */
    if (NULL == (connector = H5VL__conn_create(saved)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, NULL, "unable to create VOL connector");

    /* Set return value */
    ret_value = connector;

done:
    if (NULL == ret_value) {
        if (connector) {
            if (H5VL__conn_free(connector) < 0)
                HDONE_ERROR(H5E_VOL, H5E_CANTRELEASE, NULL, "can't free VOL connector");
        }
        else if (init_done) {
            if (H5VL__free_cls(saved) < 0)
                HDONE_ERROR(H5E_VOL, H5E_CANTRELEASE, NULL, "can't free VOL class");
        }
        else if (saved) {
            if (saved->name)
                H5MM_xfree_const(saved->name);
            H5FL_FREE(H5VL_class_t, saved);
        }
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__register_connector() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__register_connector_by_class
 *
 * Purpose:     Registers a new VOL connector as a member of the virtual object
 *              layer class.
 *
 * Return:      Success:    A pointer to a VOL connector
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
H5VL_connector_t *
H5VL__register_connector_by_class(const H5VL_class_t *cls, hid_t vipl_id)
{
    H5VL_connector_t *connector = NULL; /* Connector for class */
    H5PL_vol_key_t    key;              /* Info for connector search */
    H5VL_connector_t *ret_value = NULL; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check arguments */
    if (!cls)
        HGOTO_ERROR(H5E_ARGS, H5E_UNINITIALIZED, NULL, "VOL connector class pointer cannot be NULL");
    if (H5VL_VERSION != cls->version)
        HGOTO_ERROR(H5E_VOL, H5E_CANTREGISTER, NULL, "VOL connector has incompatible version");
    if (!cls->name)
        HGOTO_ERROR(H5E_VOL, H5E_CANTREGISTER, NULL, "VOL connector class name cannot be the NULL pointer");
    if (0 == strlen(cls->name))
        HGOTO_ERROR(H5E_VOL, H5E_CANTREGISTER, NULL, "VOL connector class name cannot be the empty string");
    if (cls->info_cls.copy && !cls->info_cls.free)
        HGOTO_ERROR(
            H5E_VOL, H5E_CANTREGISTER, NULL,
            "VOL connector must provide free callback for VOL info objects when a copy callback is provided");
    if (cls->wrap_cls.get_wrap_ctx && !cls->wrap_cls.free_wrap_ctx)
        HGOTO_ERROR(H5E_VOL, H5E_CANTREGISTER, NULL,
                    "VOL connector must provide free callback for object wrapping contexts when a get "
                    "callback is provided");

    /* Set up data for find */
    key.kind   = H5VL_GET_CONNECTOR_BY_NAME;
    key.u.name = cls->name;

    /* Check if connector is already registered */
    if (H5VL__conn_find(&key, &connector) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTFIND, NULL, "can't search VOL connectors");

    /* If not found, create a new connector */
    if (NULL == connector)
        if (NULL == (connector = H5VL__register_connector(cls, vipl_id)))
            HGOTO_ERROR(H5E_VOL, H5E_CANTREGISTER, NULL, "unable to register VOL connector");

    /* Inc. refcount on connector object, so it can be uniformly released */
    H5VL_conn_inc_rc(connector);

    /* Set return value */
    ret_value = connector;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__register_connector_by_class() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__register_connector_by_name
 *
 * Purpose:     Registers a new VOL connector as a member of the virtual object
 *              layer class.
 *
 * Return:      Success:    A pointer to a VOL connector
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
H5VL_connector_t *
H5VL__register_connector_by_name(const char *name, hid_t vipl_id)
{
    H5VL_connector_t *connector = NULL; /* Connector for class */
    H5PL_vol_key_t    key;              /* Info for connector search */
    H5VL_connector_t *ret_value = NULL; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Set up data for find */
    key.kind   = H5VL_GET_CONNECTOR_BY_NAME;
    key.u.name = name;

    /* Check if connector is already registered */
    if (H5VL__conn_find(&key, &connector) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTFIND, NULL, "can't search VOL connectors");

    /* If not found, create a new connector */
    if (NULL == connector) {
        H5PL_key_t          plugin_key;
        const H5VL_class_t *cls;

        /* Try loading the connector */
        plugin_key.vol.kind   = H5VL_GET_CONNECTOR_BY_NAME;
        plugin_key.vol.u.name = name;
        if (NULL == (cls = H5PL_load(H5PL_TYPE_VOL, &plugin_key)))
            HGOTO_ERROR(H5E_VOL, H5E_CANTINIT, NULL, "unable to load VOL connector");

        /* Create a connector for the class we loaded */
        if (NULL == (connector = H5VL__register_connector(cls, vipl_id)))
            HGOTO_ERROR(H5E_VOL, H5E_CANTREGISTER, NULL, "unable to register VOL connector");
    } /* end if */

    /* Inc. refcount on connector object, so it can be uniformly released */
    H5VL_conn_inc_rc(connector);

    /* Set return value */
    ret_value = connector;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__register_connector_by_name() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__register_connector_by_value
 *
 * Purpose:     Registers a new VOL connector as a member of the virtual object
 *              layer class.
 *
 * Return:      Success:    A pointer to a VOL connector
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
H5VL_connector_t *
H5VL__register_connector_by_value(H5VL_class_value_t value, hid_t vipl_id)
{
    H5VL_connector_t *connector = NULL; /* Connector for class */
    H5PL_vol_key_t    key;              /* Info for connector search */
    H5VL_connector_t *ret_value = NULL; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Set up data for find */
    key.kind    = H5VL_GET_CONNECTOR_BY_VALUE;
    key.u.value = value;

    /* Check if connector is already registered */
    if (H5VL__conn_find(&key, &connector) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTFIND, NULL, "can't search VOL connectors");

    /* If not found, create a new connector */
    if (NULL == connector) {
        H5PL_key_t          plugin_key;
        const H5VL_class_t *cls;

        /* Try loading the connector */
        plugin_key.vol.kind    = H5VL_GET_CONNECTOR_BY_VALUE;
        plugin_key.vol.u.value = value;
        if (NULL == (cls = H5PL_load(H5PL_TYPE_VOL, &plugin_key)))
            HGOTO_ERROR(H5E_VOL, H5E_CANTINIT, NULL, "unable to load VOL connector");

        /* Create a connector for the class we loaded */
        if (NULL == (connector = H5VL__register_connector(cls, vipl_id)))
            HGOTO_ERROR(H5E_VOL, H5E_CANTREGISTER, NULL, "unable to register VOL connector ID");
    } /* end if */

    /* Inc. refcount on connector object, so it can be uniformly released */
    H5VL_conn_inc_rc(connector);

    /* Set return value */
    ret_value = connector;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__register_connector_by_value() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__is_connector_registered_by_name
 *
 * Purpose:     Checks if a connector with a particular name is registered.
 *
 * Return:      >0 if a VOL connector with that name has been registered
 *              0 if a VOL connector with that name has NOT been registered
 *              <0 on errors
 *
 *-------------------------------------------------------------------------
 */
H5_ATTR_PURE htri_t
H5VL__is_connector_registered_by_name(const char *name)
{
    H5VL_connector_t *connector = NULL;  /* Connector for class */
    H5PL_vol_key_t    key;               /* Info for connector search */
    htri_t            ret_value = false; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Set up data for find */
    key.kind   = H5VL_GET_CONNECTOR_BY_NAME;
    key.u.name = name;

    /* Find connector with name */
    if (H5VL__conn_find(&key, &connector) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTFIND, FAIL, "can't search VOL connectors");

    /* Found a connector with that name */
    if (connector)
        ret_value = true;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__is_connector_registered_by_name() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__is_connector_registered_by_value
 *
 * Purpose:     Checks if a connector with a particular value (ID) is
 *              registered.
 *
 * Return:      >0 if a VOL connector with that value has been registered
 *              0 if a VOL connector with that value hasn't been registered
 *              <0 on errors
 *
 *-------------------------------------------------------------------------
 */
H5_ATTR_PURE htri_t
H5VL__is_connector_registered_by_value(H5VL_class_value_t value)
{
    H5VL_connector_t *connector = NULL;  /* Connector for class */
    H5PL_vol_key_t    key;               /* Info for connector search */
    htri_t            ret_value = false; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Set up data for find */
    key.kind    = H5VL_GET_CONNECTOR_BY_VALUE;
    key.u.value = value;

    /* Find connector with value */
    if (H5VL__conn_find(&key, &connector) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTFIND, FAIL, "can't search VOL connectors");

    /* Found a connector with that value */
    if (connector)
        ret_value = true;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__is_connector_registered_by_value() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__get_connector_by_name
 *
 * Purpose:     Looks up a connector by its class name.
 *
 * Return:      Pointer to the connector if the VOL class has been registered
 *              NULL on error (if the class is not a valid class or not registered)
 *
 *-------------------------------------------------------------------------
 */
H5VL_connector_t *
H5VL__get_connector_by_name(const char *name)
{
    H5VL_connector_t *connector = NULL; /* Connector for class */
    H5PL_vol_key_t    key;              /* Info for connector search */
    H5VL_connector_t *ret_value = NULL; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Set up data for find */
    key.kind   = H5VL_GET_CONNECTOR_BY_NAME;
    key.u.name = name;

    /* Find connector with name */
    if (H5VL__conn_find(&key, &connector) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_BADITER, NULL, "can't find VOL connector");

    if (connector)
        /* Inc. refcount on connector object, so it can be uniformly released */
        H5VL_conn_inc_rc(connector);

    /* Set return value */
    ret_value = connector;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__get_connector_by_name() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__get_connector_by_value
 *
 * Purpose:     Looks up a connector by its class value.
 *
 * Return:      Pointer to the connector if the VOL class has been registered
 *              NULL on error (if the class is not a valid class or not registered)
 *
 *-------------------------------------------------------------------------
 */
H5VL_connector_t *
H5VL__get_connector_by_value(H5VL_class_value_t value)
{
    H5VL_connector_t *connector = NULL; /* Connector for class */
    H5PL_vol_key_t    key;              /* Info for connector search */
    H5VL_connector_t *ret_value = NULL; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Set up data for find */
    key.kind    = H5VL_GET_CONNECTOR_BY_VALUE;
    key.u.value = value;

    /* Find connector with name */
    if (H5VL__conn_find(&key, &connector) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_BADITER, NULL, "can't find VOL connector");

    if (connector)
        /* Inc. refcount on connector object, so it can be uniformly released */
        H5VL_conn_inc_rc(connector);

    /* Set return value */
    ret_value = connector;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__get_connector_by_value() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__get_connector_name
 *
 * Purpose:     Retrieve name of connector
 *
 * Return:      Success:        The length of the connector name
 *              Failure:        Can't fail
 *
 *-------------------------------------------------------------------------
 */
size_t
H5VL__get_connector_name(const H5VL_connector_t *connector, char *name /*out*/, size_t size)
{
    size_t len = 0;

    FUNC_ENTER_PACKAGE_NOERR

    /* Sanity check */
    assert(connector);

    len = strlen(connector->cls->name);
    if (name) {
        strncpy(name, connector->cls->name, size);
        if (len >= size)
            name[size - 1] = '\0';
    } /* end if */

    FUNC_LEAVE_NOAPI(len)
} /* end H5VL__get_connector_name() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_vol_object
 *
 * Purpose:     Utility function to return the object pointer associated with
 *              a hid_t. This routine is the same as H5I_object for all types
 *              except for named datatypes, where the vol_obj is returned that
 *              is attached to the H5T_t struct.
 *
 * Return:      Success:        object pointer
 *              Failure:        NULL
 *
 *-------------------------------------------------------------------------
 */
H5VL_object_t *
H5VL_vol_object(hid_t id)
{
    H5VL_object_t *ret_value = NULL;

    FUNC_ENTER_NOAPI(NULL)

    /* Get the underlying object */
    if (NULL == (ret_value = H5VL_vol_object_verify(id, H5I_get_type(id))))
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, NULL, "can't retrieve object for ID");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_vol_object() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_vol_object_verify
 *
 * Purpose:     Utility function to return the object pointer associated with
 *              an ID of the specified type. This routine is the same as
 *              H5VL_vol_object except it takes the additional argument
 *              obj_type to verify the ID's type against.
 *
 * Return:      Success:        object pointer
 *              Failure:        NULL
 *
 *-------------------------------------------------------------------------
 */
H5VL_object_t *
H5VL_vol_object_verify(hid_t id, H5I_type_t obj_type)
{
    void          *obj       = NULL;
    H5VL_object_t *ret_value = NULL;

    FUNC_ENTER_NOAPI(NULL)

    if (H5I_FILE == obj_type || H5I_GROUP == obj_type || H5I_ATTR == obj_type || H5I_DATASET == obj_type ||
        H5I_DATATYPE == obj_type || H5I_MAP == obj_type) {
        /* Get the object */
        if (NULL == (obj = H5I_object_verify(id, obj_type)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "identifier is not of specified type");

        /* If this is a datatype, get the VOL object attached to the H5T_t struct */
        if (H5I_DATATYPE == obj_type)
            if (NULL == (obj = H5T_get_named_type((H5T_t *)obj)))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a named datatype");
    } /* end if */
    else
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "invalid identifier type to function");

    ret_value = (H5VL_object_t *)obj;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5VL_object_data
 *
 * Purpose:     Correctly retrieve the 'data' field for a VOL object (H5VL_object),
 *              even for nested / stacked VOL connectors.
 *
 * Return:      Success:        object pointer
 *              Failure:        NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_object_data(const H5VL_object_t *vol_obj)
{
    void *ret_value = NULL;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check for 'get_object' callback in connector */
    if (vol_obj->connector->cls->wrap_cls.get_object) {
        /* Prepare & restore library for user callback */
        H5_BEFORE_USER_CB_NOERR(NULL)
            {
                ret_value = (vol_obj->connector->cls->wrap_cls.get_object)(vol_obj->data);
            }
        H5_AFTER_USER_CB_NOERR(NULL)
    }
    else
        ret_value = vol_obj->data;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_object_data() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_object_unwrap
 *
 * Purpose:     Correctly unwrap the 'data' field for a VOL object (H5VL_object),
 *              even for nested / stacked VOL connectors.
 *
 * Return:      Success:        Object pointer
 *              Failure:        NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_object_unwrap(const H5VL_object_t *vol_obj)
{
    void *ret_value = NULL;

    FUNC_ENTER_NOAPI(NULL)

    if (NULL == (ret_value = H5VL_unwrap_object(vol_obj->connector->cls, vol_obj->data)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, NULL, "can't unwrap object");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_object_unwrap() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__object
 *
 * Purpose:     Internal function to return the VOL object pointer associated
 *              with an hid_t.
 *
 * Return:      Success:    object pointer
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL__object(hid_t id, H5I_type_t obj_type)
{
    H5VL_object_t *vol_obj   = NULL;
    void          *ret_value = NULL;

    FUNC_ENTER_PACKAGE

    /* Get the underlying object */
    switch (obj_type) {
        case H5I_GROUP:
        case H5I_DATASET:
        case H5I_FILE:
        case H5I_ATTR:
        case H5I_MAP:
            /* get the object */
            if (NULL == (vol_obj = H5I_object(id)))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "invalid identifier");
            break;

        case H5I_DATATYPE: {
            H5T_t *dt = NULL;

            /* get the object */
            if (NULL == (dt = H5I_object(id)))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "invalid identifier");

            /* Get the actual datatype object that should be the vol_obj */
            if (NULL == (vol_obj = H5T_get_named_type(dt)))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a named datatype");
            break;
        }

        case H5I_UNINIT:
        case H5I_BADID:
        case H5I_DATASPACE:
        case H5I_VFL:
        case H5I_VOL:
        case H5I_GENPROP_CLS:
        case H5I_GENPROP_LST:
        case H5I_ERROR_CLASS:
        case H5I_ERROR_MSG:
        case H5I_ERROR_STACK:
        case H5I_SPACE_SEL_ITER:
        case H5I_EVENTSET:
        case H5I_NTYPES:
        default:
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "unknown data object type");
    } /* end switch */

    /* Set the return value */
    ret_value = H5VL_object_data(vol_obj);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__object() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_object
 *
 * Purpose:     Utility function to return the VOL object pointer associated with
 *              a hid_t.
 *
 * Return:      Success:    object pointer
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_object(hid_t id)
{
    void *ret_value = NULL;

    FUNC_ENTER_NOAPI(NULL)

    /* Get the underlying object */
    if (NULL == (ret_value = H5VL__object(id, H5I_get_type(id))))
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, NULL, "can't retrieve object for ID");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_object() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_object_verify
 *
 * Purpose:     Utility function to return the VOL object pointer associated
 *              with an identifier.
 *
 * Return:      Success:    object pointer
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_object_verify(hid_t id, H5I_type_t obj_type)
{
    void *ret_value = NULL;

    FUNC_ENTER_NOAPI(NULL)

    /* Check of ID of correct type */
    if (obj_type != H5I_get_type(id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "invalid identifier");

    /* Get the underlying object */
    if (NULL == (ret_value = H5VL__object(id, obj_type)))
        HGOTO_ERROR(H5E_ARGS, H5E_CANTGET, NULL, "can't retrieve object for ID");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_object_verify() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_cmp_connector_cls
 *
 * Purpose:     Compare VOL class for a connector
 *
 * Note:        Sets *cmp_value positive if VALUE1 is greater than VALUE2,
 *		negative if VALUE2 is greater than VALUE1, and zero if VALUE1
 *              and VALUE2 are equal (like strcmp).
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_cmp_connector_cls(int *cmp_value, const H5VL_class_t *cls1, const H5VL_class_t *cls2)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    assert(cls1);
    assert(cls2);

    /* If the pointers are the same the classes are the same */
    if (cls1 == cls2) {
        *cmp_value = 0;
        HGOTO_DONE(SUCCEED);
    } /* end if */

    /* Compare connector "values" */
    if (cls1->value < cls2->value) {
        *cmp_value = -1;
        HGOTO_DONE(SUCCEED);
    } /* end if */
    if (cls1->value > cls2->value) {
        *cmp_value = 1;
        HGOTO_DONE(SUCCEED);
    } /* end if */
    assert(cls1->value == cls2->value);

    /* Compare connector names */
    if (cls1->name == NULL && cls2->name != NULL) {
        *cmp_value = -1;
        HGOTO_DONE(SUCCEED);
    } /* end if */
    if (cls1->name != NULL && cls2->name == NULL) {
        *cmp_value = 1;
        HGOTO_DONE(SUCCEED);
    } /* end if */
    if (0 != (*cmp_value = strcmp(cls1->name, cls2->name)))
        HGOTO_DONE(SUCCEED);

    /* Compare connector VOL API versions */
    if (cls1->version < cls2->version) {
        *cmp_value = -1;
        HGOTO_DONE(SUCCEED);
    } /* end if */
    if (cls1->version > cls2->version) {
        *cmp_value = 1;
        HGOTO_DONE(SUCCEED);
    } /* end if */
    assert(cls1->version == cls2->version);

    /* Compare connector info */
    if (cls1->info_cls.size < cls2->info_cls.size) {
        *cmp_value = -1;
        HGOTO_DONE(SUCCEED);
    } /* end if */
    if (cls1->info_cls.size > cls2->info_cls.size) {
        *cmp_value = 1;
        HGOTO_DONE(SUCCEED);
    } /* end if */
    assert(cls1->info_cls.size == cls2->info_cls.size);

    /* Set comparison value to 'equal' */
    *cmp_value = 0;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_cmp_connector_cls() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_retrieve_lib_state
 *
 * Purpose:     Retrieve the state of the library.
 *
 * Note:        Currently just retrieves the API context state, but could be
 *		expanded in the future.
 *
 * Return:      Success:    Non-negative, *state set
 *              Failure:    Negative, *state unset
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_retrieve_lib_state(void **state)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    assert(state);

    /* Retrieve the API context state */
    if (H5CX_retrieve_state((H5CX_state_t **)state) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't get API context state");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_retrieve_lib_state() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_start_lib_state
 *
 * Purpose:     Opens a new internal state for the HDF5 library.
 *
 * Note:        Currently just pushes a new API context, but could be
 *		expanded in the future.
 *
 * Return:      Success:    Non-negative, *context set
 *              Failure:    Negative, *context unset
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_start_lib_state(void **context)
{
    H5CX_node_t *cnode     = NULL;    /* API context */
    herr_t       ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    assert(context);

    /* Allocate & clear a new API context */
    if (NULL == (cnode = H5MM_calloc(sizeof(H5CX_node_t))))
        HGOTO_ERROR(H5E_VOL, H5E_CANTALLOC, FAIL, "can't allocate library context");

    /* Push a new API context on the stack */
    if (H5CX_push(cnode) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't push API context");

    /* Set output parameter */
    *context = cnode;

done:
    if (ret_value < 0)
        if (cnode)
            H5MM_xfree(cnode);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_start_lib_state() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_restore_lib_state
 *
 * Purpose:     Restore the state of the library.
 *
 * Note:        Currently just restores the API context state, but could be
 *		expanded in the future.
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_restore_lib_state(const void *state)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    assert(state);

    /* Restore the API context state */
    if (H5CX_restore_state((const H5CX_state_t *)state) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set API context state");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_restore_lib_state() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_finish_lib_state
 *
 * Purpose:     Closes the state of the library, undoing affects of
 *		H5VL_start_lib_state.
 *
 * Note:        Currently just resets the API context state, but could be
 *		expanded in the future.
 *
 * Note:	This routine must be called as a "pair" with
 * 		H5VL_start_lib_state.  It can be called before / after /
 * 		independently of H5VL_free_lib_state.
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_finish_lib_state(void *context)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    assert(context);

    /* Pop the API context off the stack */
    if (H5CX_pop(false) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't pop API context");

    /* Release library context */
    H5MM_xfree(context);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_finish_lib_state() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_free_lib_state
 *
 * Purpose:     Free a library state.
 *
 * Note:	This routine must be called as a "pair" with
 * 		H5VL_retrieve_lib_state.
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_free_lib_state(void *state)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    assert(state);

    /* Free the API context state */
    if (H5CX_free_state((H5CX_state_t *)state) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "can't free API context state");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_free_lib_state() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__free_vol_wrapper
 *
 * Purpose:     Free object wrapping context for VOL connector
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__free_vol_wrapper(H5VL_wrap_ctx_t *vol_wrap_ctx)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    assert(vol_wrap_ctx);
    assert(0 == vol_wrap_ctx->rc);
    assert(vol_wrap_ctx->connector);
    assert(vol_wrap_ctx->connector->cls);

    /* If there is a VOL connector object wrapping context, release it */
    if (vol_wrap_ctx->obj_wrap_ctx) {
        /* Prepare & restore library for user callback */
        H5_BEFORE_USER_CB(FAIL)
            {
                /* Release the VOL connector's object wrapping context */
                ret_value =
                    (*vol_wrap_ctx->connector->cls->wrap_cls.free_wrap_ctx)(vol_wrap_ctx->obj_wrap_ctx);
            }
        H5_AFTER_USER_CB(FAIL)
        if (ret_value < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL,
                        "unable to release connector's object wrapping context");
    }

    /* Decrement refcount on connector */
    if (H5VL_conn_dec_rc(vol_wrap_ctx->connector) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTDEC, FAIL, "unable to decrement ref count on VOL connector");

    /* Release object wrapping context */
    H5FL_FREE(H5VL_wrap_ctx_t, vol_wrap_ctx);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__free_vol_wrapper() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_set_vol_wrapper
 *
 * Purpose:     Set up object wrapping context for current VOL connector
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_set_vol_wrapper(const H5VL_object_t *vol_obj)
{
    H5VL_wrap_ctx_t *vol_wrap_ctx = NULL;    /* Object wrapping context */
    herr_t           ret_value    = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    assert(vol_obj);

    /* Retrieve the VOL object wrap context */
    if (H5CX_get_vol_wrap_ctx((void **)&vol_wrap_ctx) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't get VOL object wrap context");

    /* Check for existing wrapping context */
    if (NULL == vol_wrap_ctx) {
        void *obj_wrap_ctx = NULL; /* VOL connector's wrapping context */

        /* Sanity checks */
        assert(vol_obj->data);
        assert(vol_obj->connector);

        /* Check if the connector can create a wrap context */
        if (vol_obj->connector->cls->wrap_cls.get_wrap_ctx) {
            /* Sanity check */
            assert(vol_obj->connector->cls->wrap_cls.free_wrap_ctx);

            /* Prepare & restore library for user callback */
            H5_BEFORE_USER_CB(FAIL)
                {
                    /* Get the wrap context from the connector */
                    ret_value =
                        (vol_obj->connector->cls->wrap_cls.get_wrap_ctx)(vol_obj->data, &obj_wrap_ctx);
                }
            H5_AFTER_USER_CB(FAIL)
            if (ret_value < 0)
                HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't retrieve VOL connector's object wrap context");
        } /* end if */

        /* Allocate VOL object wrapper context */
        if (NULL == (vol_wrap_ctx = H5FL_MALLOC(H5VL_wrap_ctx_t)))
            HGOTO_ERROR(H5E_VOL, H5E_CANTALLOC, FAIL, "can't allocate VOL wrap context");

        /* Increment the outstanding objects that are using the connector */
        H5VL_conn_inc_rc(vol_obj->connector);

        /* Set up VOL object wrapper context */
        vol_wrap_ctx->rc           = 1;
        vol_wrap_ctx->connector    = vol_obj->connector;
        vol_wrap_ctx->obj_wrap_ctx = obj_wrap_ctx;
    } /* end if */
    else
        /* Increment ref count on existing wrapper context */
        vol_wrap_ctx->rc++;

    /* Save the wrapper context */
    if (H5CX_set_vol_wrap_ctx(vol_wrap_ctx) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL object wrap context");

done:
    if (ret_value < 0 && vol_wrap_ctx)
        /* Release object wrapping context */
        H5FL_FREE(H5VL_wrap_ctx_t, vol_wrap_ctx);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_set_vol_wrapper() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_inc_vol_wrapper
 *
 * Purpose:     Increment refcount on object wrapping context
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_inc_vol_wrapper(void *_vol_wrap_ctx)
{
    H5VL_wrap_ctx_t *vol_wrap_ctx = (H5VL_wrap_ctx_t *)_vol_wrap_ctx; /* VOL object wrapping context */
    herr_t           ret_value    = SUCCEED;                          /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check for valid, active VOL object wrap context */
    if (NULL == vol_wrap_ctx)
        HGOTO_ERROR(H5E_VOL, H5E_BADVALUE, FAIL, "no VOL object wrap context?");
    if (0 == vol_wrap_ctx->rc)
        HGOTO_ERROR(H5E_VOL, H5E_BADVALUE, FAIL, "bad VOL object wrap context refcount?");

    /* Increment ref count on wrapping context */
    vol_wrap_ctx->rc++;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_inc_vol_wrapper() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dec_vol_wrapper
 *
 * Purpose:     Decrement refcount on object wrapping context, releasing it
 *		if the refcount drops to zero.
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_dec_vol_wrapper(void *_vol_wrap_ctx)
{
    H5VL_wrap_ctx_t *vol_wrap_ctx = (H5VL_wrap_ctx_t *)_vol_wrap_ctx; /* VOL object wrapping context */
    herr_t           ret_value    = SUCCEED;                          /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check for valid, active VOL object wrap context */
    if (NULL == vol_wrap_ctx)
        HGOTO_ERROR(H5E_VOL, H5E_BADVALUE, FAIL, "no VOL object wrap context?");
    if (0 == vol_wrap_ctx->rc)
        HGOTO_ERROR(H5E_VOL, H5E_BADVALUE, FAIL, "bad VOL object wrap context refcount?");

    /* Decrement ref count on wrapping context */
    vol_wrap_ctx->rc--;

    /* Release context if the ref count drops to zero */
    if (0 == vol_wrap_ctx->rc)
        if (H5VL__free_vol_wrapper(vol_wrap_ctx) < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "unable to release VOL object wrapping context");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_dec_vol_wrapper() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_reset_vol_wrapper
 *
 * Purpose:     Reset object wrapping context for current VOL connector
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_reset_vol_wrapper(void)
{
    H5VL_wrap_ctx_t *vol_wrap_ctx = NULL;    /* Object wrapping context */
    herr_t           ret_value    = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Retrieve the VOL object wrap context */
    if (H5CX_get_vol_wrap_ctx((void **)&vol_wrap_ctx) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't get VOL object wrap context");

    /* Check for VOL object wrap context */
    if (NULL == vol_wrap_ctx)
        HGOTO_ERROR(H5E_VOL, H5E_BADVALUE, FAIL, "no VOL object wrap context?");

    /* Decrement ref count on wrapping context */
    vol_wrap_ctx->rc--;

    /* Release context if the ref count drops to zero */
    if (0 == vol_wrap_ctx->rc) {
        /* Release object wrapping context */
        if (H5VL__free_vol_wrapper(vol_wrap_ctx) < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "unable to release VOL object wrapping context");

        /* Reset the wrapper context */
        if (H5CX_set_vol_wrap_ctx(NULL) < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL object wrap context");
    } /* end if */
    else
        /* Save the updated wrapper context */
        if (H5CX_set_vol_wrap_ctx(vol_wrap_ctx) < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL object wrap context");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_reset_vol_wrapper() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_wrap_register
 *
 * Purpose:     Wrap an object and register an ID for it
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5VL_wrap_register(H5I_type_t type, void *obj, bool app_ref)
{
    H5VL_wrap_ctx_t *vol_wrap_ctx = NULL;         /* Object wrapping context */
    void            *new_obj;                     /* Newly wrapped object */
    hid_t            ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_NOAPI(H5I_INVALID_HID)

    /* Sanity check */
    assert(obj);

    /* Retrieve the VOL object wrapping context */
    if (H5CX_get_vol_wrap_ctx((void **)&vol_wrap_ctx) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, H5I_INVALID_HID, "can't get VOL object wrap context");
    if (NULL == vol_wrap_ctx || NULL == vol_wrap_ctx->connector)
        HGOTO_ERROR(H5E_VOL, H5E_BADVALUE, H5I_INVALID_HID,
                    "VOL object wrap context or its connector is NULL???");

    /* If the datatype is already VOL-managed, the datatype's vol_obj
     * field will get clobbered later, so disallow this.
     */
    if (type == H5I_DATATYPE)
        if (vol_wrap_ctx->connector == H5VL_NATIVE_conn_g)
            if (true == H5T_already_vol_managed((const H5T_t *)obj))
                HGOTO_ERROR(H5E_VOL, H5E_BADTYPE, H5I_INVALID_HID, "can't wrap an uncommitted datatype");

    /* Wrap the object with VOL connector info */
    if (NULL == (new_obj = H5VL__wrap_obj(obj, type)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, H5I_INVALID_HID, "can't wrap library object");

    /* Get an ID for the object */
    if ((ret_value = H5VL_register(type, new_obj, vol_wrap_ctx->connector, app_ref)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to get an ID for the object");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_wrap_register() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_check_plugin_load
 *
 * Purpose:     Check if a VOL connector matches the search criteria, and
 *              can be loaded.
 *
 * Note:        Matching the connector's name / value, but the connector
 *              having an incompatible version is not an error, but means
 *              that the connector isn't a "match".  Setting the SUCCEED
 *              value to false and not failing for that case allows the
 *              plugin framework to keep looking for other DLLs that match
 *              and have a compatible version.
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_check_plugin_load(const H5VL_class_t *cls, const H5PL_key_t *key, bool *success)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    assert(cls);
    assert(key);
    assert(success);

    /* Which kind of key are we looking for? */
    if (key->vol.kind == H5VL_GET_CONNECTOR_BY_NAME) {
        /* Check if plugin name matches VOL connector class name */
        if (cls->name && !strcmp(cls->name, key->vol.u.name))
            *success = true;
    } /* end if */
    else {
        /* Sanity check */
        assert(key->vol.kind == H5VL_GET_CONNECTOR_BY_VALUE);

        /* Check if plugin value matches VOL connector class value */
        if (cls->value == key->vol.u.value)
            *success = true;
    } /* end else */

    /* Connector is a match, but might not be a compatible version */
    if (*success && cls->version != H5VL_VERSION)
        *success = false;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_check_plugin_load() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__is_default_conn
 *
 * Purpose:     Check if the default connector will be used for a container.
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
void
H5VL__is_default_conn(hid_t fapl_id, const H5VL_connector_t *connector, bool *is_default)
{
    FUNC_ENTER_PACKAGE_NOERR

    /* Sanity checks */
    assert(is_default);

    /* Determine if the default VOL connector will be used, based on non-default
     * values in the FAPL, connector ID, or the HDF5_VOL_CONNECTOR environment
     * variable being set.
     */
    *is_default = (H5VL_def_conn_s.connector == H5_DEFAULT_VOL) &&
                  (H5P_FILE_ACCESS_DEFAULT == fapl_id || connector == H5_DEFAULT_VOL);

    FUNC_LEAVE_NOAPI_VOID
} /* end H5VL__is_default_conn() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_setup_args
 *
 * Purpose:     Set up arguments to access an object
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_setup_args(hid_t loc_id, H5I_type_t id_type, H5VL_object_t **vol_obj)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    assert(vol_obj);

    /* Get attribute pointer */
    if (NULL == (*vol_obj = H5I_object_verify(loc_id, id_type)))
        HGOTO_ERROR(H5E_VOL, H5E_BADTYPE, FAIL, "not the correct type of ID");

    /* Set up collective metadata (if appropriate) */
    if (H5CX_set_loc(loc_id) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set collective metadata read");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_setup_args() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_setup_loc_args
 *
 * Purpose:     Set up arguments to access an object
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_setup_loc_args(hid_t loc_id, H5VL_object_t **vol_obj, H5VL_loc_params_t *loc_params)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    assert(vol_obj);
    assert(loc_params);

    /* Get the location object */
    if (NULL == (*vol_obj = (H5VL_object_t *)H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_VOL, H5E_BADTYPE, FAIL, "not the correct type of ID");

    /* Set up collective metadata (if appropriate */
    if (H5CX_set_loc(loc_id) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set collective metadata read");

    /* Set location parameters */
    loc_params->type     = H5VL_OBJECT_BY_SELF;
    loc_params->obj_type = H5I_get_type(loc_id);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_setup_loc_args() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_setup_acc_args
 *
 * Purpose:     Set up arguments to access an object
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_setup_acc_args(hid_t loc_id, const H5P_libclass_t *libclass, bool is_collective, hid_t *acspl_id,
                    H5VL_object_t **vol_obj, H5VL_loc_params_t *loc_params)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    assert(libclass);
    assert(acspl_id);
    assert(vol_obj);
    assert(loc_params);

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(acspl_id, libclass, loc_id, is_collective) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set access property list info");

    /* Get the location object */
    if (NULL == (*vol_obj = (H5VL_object_t *)H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier");

    /* Set location parameters */
    loc_params->type     = H5VL_OBJECT_BY_SELF;
    loc_params->obj_type = H5I_get_type(loc_id);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_setup_acc_args() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_setup_self_args
 *
 * Purpose:     Set up arguments to access an object "by self"
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_setup_self_args(hid_t loc_id, H5VL_object_t **vol_obj, H5VL_loc_params_t *loc_params)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    assert(vol_obj);
    assert(loc_params);

    /* Get the location object */
    if (NULL == (*vol_obj = (H5VL_object_t *)H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier");

    /* Set location parameters */
    loc_params->type     = H5VL_OBJECT_BY_SELF;
    loc_params->obj_type = H5I_get_type(loc_id);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_setup_self_args() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_setup_name_args
 *
 * Purpose:     Set up arguments to access an object "by name"
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_setup_name_args(hid_t loc_id, const char *name, bool is_collective, hid_t lapl_id,
                     H5VL_object_t **vol_obj, H5VL_loc_params_t *loc_params)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    assert(vol_obj);
    assert(loc_params);

    /* Check args */
    if (!name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "name parameter cannot be NULL");
    if (!*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "name parameter cannot be an empty string");

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, is_collective) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set access property list info");

    /* Get the location object */
    if (NULL == (*vol_obj = (H5VL_object_t *)H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier");

    /* Set up location parameters */
    loc_params->type                         = H5VL_OBJECT_BY_NAME;
    loc_params->loc_data.loc_by_name.name    = name;
    loc_params->loc_data.loc_by_name.lapl_id = lapl_id;
    loc_params->obj_type                     = H5I_get_type(loc_id);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_setup_name_args() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_setup_idx_args
 *
 * Purpose:     Set up arguments to access an object "by idx"
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_setup_idx_args(hid_t loc_id, const char *name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n,
                    bool is_collective, hid_t lapl_id, H5VL_object_t **vol_obj, H5VL_loc_params_t *loc_params)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    assert(vol_obj);
    assert(loc_params);

    /* Check args */
    if (!name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "name parameter cannot be NULL");
    if (!*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "name parameter cannot be an empty string");
    if (idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified");
    if (order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified");

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, is_collective) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set access property list info");

    /* Get the location object */
    if (NULL == (*vol_obj = (H5VL_object_t *)H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier");

    /* Set location parameters */
    loc_params->type                         = H5VL_OBJECT_BY_IDX;
    loc_params->loc_data.loc_by_idx.name     = name;
    loc_params->loc_data.loc_by_idx.idx_type = idx_type;
    loc_params->loc_data.loc_by_idx.order    = order;
    loc_params->loc_data.loc_by_idx.n        = n;
    loc_params->loc_data.loc_by_idx.lapl_id  = lapl_id;
    loc_params->obj_type                     = H5I_get_type(loc_id);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_setup_idx_args() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_setup_token_args
 *
 * Purpose:     Set up arguments to access an object by token
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_setup_token_args(hid_t loc_id, H5O_token_t *obj_token, H5VL_object_t **vol_obj,
                      H5VL_loc_params_t *loc_params)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    assert(vol_obj);
    assert(loc_params);

    /* Get the location object */
    if (NULL == (*vol_obj = (H5VL_object_t *)H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier");

    /* Set location parameters */
    loc_params->type                        = H5VL_OBJECT_BY_TOKEN;
    loc_params->loc_data.loc_by_token.token = obj_token;
    loc_params->obj_type                    = H5I_get_type(loc_id);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_setup_token_args() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_conn_prop_get_cap_flags
 *
 * Purpose:     Query capability flags for connector property.
 *
 * Note:        VOL connector set with HDF5_VOL_CONNECTOR overrides the
 *              property passed in.
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_conn_prop_get_cap_flags(const H5VL_connector_prop_t *connector_prop, uint64_t *cap_flags)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    assert(connector_prop);

    /* Copy the connector ID & info, if there is one */
    if (connector_prop->connector) {
        /* Query the connector's capability flags */
        if (H5VL_introspect_get_cap_flags(connector_prop->connector_info, connector_prop->connector->cls,
                                          cap_flags) < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't query connector's capability flags");
    } /* end if */
    else
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "connector ID not set?");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_conn_prop_get_cap_flags() */
