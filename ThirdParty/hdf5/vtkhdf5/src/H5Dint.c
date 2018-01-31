/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/****************/
/* Module Setup */
/****************/

#include "H5Dmodule.h"          /* This source code file is part of the H5D module */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Dpkg.h"		/* Datasets 				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free Lists                           */
#include "H5FOprivate.h"        /* File objects                         */
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5Lprivate.h"		/* Links		  		*/
#include "H5MMprivate.h"	/* Memory management			*/


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/

/* Struct for holding callback info during H5D_flush operation */
typedef struct {
    const H5F_t *f;             /* Pointer to file being flushed */
    hid_t dxpl_id;              /* DXPL for I/O operations */
} H5D_flush_ud_t;


/********************/
/* Local Prototypes */
/********************/

/* General stuff */
static herr_t H5D__get_dxpl_cache_real(hid_t dxpl_id, H5D_dxpl_cache_t *cache);
static H5D_shared_t *H5D__new(hid_t dcpl_id, hbool_t creating,
    hbool_t vl_type);
static herr_t H5D__init_type(H5F_t *file, const H5D_t *dset, hid_t type_id,
    const H5T_t *type);
static herr_t H5D__cache_dataspace_info(const H5D_t *dset);
static herr_t H5D__init_space(H5F_t *file, const H5D_t *dset, const H5S_t *space);
static herr_t H5D__update_oh_info(H5F_t *file, hid_t dxpl_id, H5D_t *dset,
    hid_t dapl_id);
static herr_t H5D_build_extfile_prefix(const H5D_t *dset, hid_t dapl_id,
    char **extfile_prefix);
static herr_t H5D__open_oid(H5D_t *dataset, hid_t dapl_id, hid_t dxpl_id);
static herr_t H5D__init_storage(const H5D_io_info_t *io_info, hbool_t full_overwrite,
    hsize_t old_dim[]);
static herr_t H5D__append_flush_setup(H5D_t *dset, hid_t dapl_id);

/*********************/
/* Package Variables */
/*********************/

/* Define a "default" dataset transfer property list cache structure to use for default DXPLs */
H5D_dxpl_cache_t H5D_def_dxpl_cache;

/* Declare a free list to manage blocks of VL data */
H5FL_BLK_DEFINE(vlen_vl_buf);

/* Declare a free list to manage other blocks of VL data */
H5FL_BLK_DEFINE(vlen_fl_buf);


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/

/* Declare a free list to manage the H5D_t and H5D_shared_t structs */
H5FL_DEFINE_STATIC(H5D_t);
H5FL_DEFINE_STATIC(H5D_shared_t);

/* Declare the external PQ free list for the sieve buffer information */
H5FL_BLK_EXTERN(sieve_buf);

/* Declare the external free list to manage the H5D_chunk_info_t struct */
H5FL_EXTERN(H5D_chunk_info_t);

/* Declare extern the free list to manage blocks of type conversion data */
H5FL_BLK_EXTERN(type_conv);

/* Define a static "default" dataset structure to use to initialize new datasets */
static H5D_shared_t H5D_def_dset;

/* Dataset ID class */
static const H5I_class_t H5I_DATASET_CLS[1] = {{
    H5I_DATASET,		/* ID class value */
    0,				/* Class flags */
    0,				/* # of reserved IDs for class */
    (H5I_free_t)H5D_close       /* Callback routine for closing objects of this class */
}};

/* Flag indicating "top" of interface has been initialized */
static hbool_t H5D_top_package_initialize_s = FALSE;



/*-------------------------------------------------------------------------
 * Function:	H5D_init
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
 *-------------------------------------------------------------------------
 */
herr_t
H5D_init(void)
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(FAIL)
    /* FUNC_ENTER() does all the work */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_init() */


/*--------------------------------------------------------------------------
NAME
   H5D__init_package -- Initialize interface-specific information
USAGE
    herr_t H5D__init_package()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.
NOTES
    Care must be taken when using the H5P functions, since they can cause
    a deadlock in the library when the library is attempting to terminate -QAK

--------------------------------------------------------------------------*/
herr_t
H5D__init_package(void)
{
    H5P_genplist_t *def_dcpl;           /* Default Dataset Creation Property list */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_PACKAGE

    /* Initialize the atom group for the dataset IDs */
    if(H5I_register_type(H5I_DATASET_CLS) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize interface")

    /* Reset the "default dataset" information */
    HDmemset(&H5D_def_dset, 0, sizeof(H5D_shared_t));

    /* Get the default dataset creation property list values and initialize the
     * default dataset with them.
     */
    if(NULL == (def_dcpl = (H5P_genplist_t *)H5I_object(H5P_LST_DATASET_CREATE_ID_g)))
        HGOTO_ERROR(H5E_DATASET, H5E_BADTYPE, FAIL, "can't get default dataset creation property list")

    /* Get the default data storage layout */
    if(H5P_get(def_dcpl, H5D_CRT_LAYOUT_NAME, &H5D_def_dset.layout) < 0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't retrieve layout")

    /* Get the default dataset creation properties */
    if(H5P_get(def_dcpl, H5D_CRT_EXT_FILE_LIST_NAME, &H5D_def_dset.dcpl_cache.efl) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't retrieve external file list")
    if(H5P_get(def_dcpl, H5D_CRT_FILL_VALUE_NAME, &H5D_def_dset.dcpl_cache.fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't retrieve fill value")
    if(H5P_get(def_dcpl, H5O_CRT_PIPELINE_NAME, &H5D_def_dset.dcpl_cache.pline) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't retrieve pipeline filter")

    /* Reset the "default DXPL cache" information */
    HDmemset(&H5D_def_dxpl_cache, 0, sizeof(H5D_dxpl_cache_t));

    /* Get the default DXPL cache information */
    if(H5D__get_dxpl_cache_real(H5P_DATASET_XFER_DEFAULT, &H5D_def_dxpl_cache) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't retrieve default DXPL info")

    /* Mark "top" of interface as initialized, too */
    H5D_top_package_initialize_s = TRUE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__init_package() */


/*-------------------------------------------------------------------------
 * Function:	H5D_top_term_package
 *
 * Purpose:	Close the "top" of the interface, releasing IDs, etc.
 *
 * Return:	Success:	Positive if anything was done that might
 *				affect other interfaces; zero otherwise.
 * 		Failure:	Negative.
 *
 * Programmer:	Quincey Koziol
 *              Sunday, September 13, 2015
 *
 *-------------------------------------------------------------------------
 */
int
H5D_top_term_package(void)
{
    int	n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(H5D_top_package_initialize_s) {
	if(H5I_nmembers(H5I_DATASET) > 0) {
            /* The dataset API uses the "force" flag set to true because it
             * is using the "file objects" (H5FO) API functions to track open
             * objects in the file.  Using the H5FO code means that dataset
             * IDs can have reference counts >1, when an existing dataset is
             * opened more than once.  However, the H5I code does not attempt
             * to close objects with reference counts>1 unless the "force" flag
             * is set to true.
             *
             * At some point (probably after the group and datatypes use the
             * the H5FO code), the H5FO code might need to be switched around
             * to storing pointers to the objects being tracked (H5D_t, H5G_t,
             * etc) and reference count those itself instead of relying on the
             * reference counting in the H5I layer.  Then, the "force" flag can
             * be put back to false.
             *
             * Setting the "force" flag to true for all the interfaces won't
             * work because the "file driver" (H5FD) APIs use the H5I reference
             * counting to avoid closing a file driver out from underneath an
             * open file...
             *
             * QAK - 5/13/03
             */
	    (void)H5I_clear_type(H5I_DATASET, TRUE, FALSE);
            n++; /*H5I*/
	} /* end if */

        /* Mark closed */
        if(0 == n)
            H5D_top_package_initialize_s = FALSE;
    } /* end if */

    FUNC_LEAVE_NOAPI(n)
} /* end H5D_top_term_package() */


/*-------------------------------------------------------------------------
 * Function:	H5D_term_package
 *
 * Purpose:	Terminate this interface.
 *
 * Note:	Finishes shutting down the interface, after
 *		H5D_top_term_package() is called
 *
 * Return:	Success:	Positive if anything was done that might
 *				affect other interfaces; zero otherwise.
 * 		Failure:	Negative.
 *
 * Programmer:	Robb Matzke
 *              Friday, November 20, 1998
 *
 *-------------------------------------------------------------------------
 */
int
H5D_term_package(void)
{
    int	n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(H5_PKG_INIT_VAR) {
        /* Sanity checks */
        HDassert(0 == H5I_nmembers(H5I_DATASET));
        HDassert(FALSE == H5D_top_package_initialize_s);

        /* Destroy the dataset object id group */
        n += (H5I_dec_type_ref(H5I_DATASET) > 0);

        /* Mark closed */
        if(0 == n)
            H5_PKG_INIT_VAR = FALSE;
    } /* end if */

    FUNC_LEAVE_NOAPI(n)
} /* end H5D_term_package() */


/*--------------------------------------------------------------------------
 NAME
    H5D__get_dxpl_cache_real
 PURPOSE
    Get all the values for the DXPL cache.
 USAGE
    herr_t H5D__get_dxpl_cache_real(dxpl_id, cache)
        hid_t dxpl_id;          IN: DXPL to query
        H5D_dxpl_cache_t *cache;IN/OUT: DXPL cache to fill with values
 RETURNS
    Non-negative on success/Negative on failure.
 DESCRIPTION
    Query all the values from a DXPL that are needed by internal routines
    within the library.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5D__get_dxpl_cache_real(hid_t dxpl_id, H5D_dxpl_cache_t *cache)
{
    H5P_genplist_t *dx_plist;   /* Data transfer property list */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check args */
    HDassert(cache);

    /* Get the dataset transfer property list */
    if(NULL == (dx_plist = (H5P_genplist_t *)H5I_object(dxpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset transfer property list")

    /* Get maximum temporary buffer size */
    if(H5P_get(dx_plist, H5D_XFER_MAX_TEMP_BUF_NAME, &cache->max_temp_buf) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "Can't retrieve maximum temporary buffer size")

    /* Get temporary buffer pointer */
    if(H5P_get(dx_plist, H5D_XFER_TCONV_BUF_NAME, &cache->tconv_buf) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "Can't retrieve temporary buffer pointer")

    /* Get background buffer pointer */
    if(H5P_get(dx_plist, H5D_XFER_BKGR_BUF_NAME, &cache->bkgr_buf) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "Can't retrieve background buffer pointer")

    /* Get background buffer type */
    if(H5P_get(dx_plist, H5D_XFER_BKGR_BUF_TYPE_NAME, &cache->bkgr_buf_type) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "Can't retrieve background buffer type")

    /* Get B-tree split ratios */
    if(H5P_get(dx_plist, H5D_XFER_BTREE_SPLIT_RATIO_NAME, &cache->btree_split_ratio) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "Can't retrieve B-tree split ratios")

    /* Get I/O vector size */
    if(H5P_get(dx_plist, H5D_XFER_HYPER_VECTOR_SIZE_NAME, &cache->vec_size) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "Can't retrieve I/O vector size")

#ifdef H5_HAVE_PARALLEL
    /* Collect Parallel I/O information for possible later use */
    if(H5P_get(dx_plist, H5D_XFER_IO_XFER_MODE_NAME, &cache->xfer_mode) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "Can't retrieve parallel transfer method")
    if(H5P_get(dx_plist, H5D_XFER_MPIO_COLLECTIVE_OPT_NAME, &cache->coll_opt_mode) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "Can't retrieve collective transfer option")
#endif /* H5_HAVE_PARALLEL */

    /* Get error detection properties */
    if(H5P_get(dx_plist, H5D_XFER_EDC_NAME, &cache->err_detect) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "Can't retrieve error detection info")

    /* Get filter callback function */
    if(H5P_get(dx_plist, H5D_XFER_FILTER_CB_NAME, &cache->filter_cb) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "Can't retrieve filter callback function")

    /* Look at the data transform property */
    /* (Note: 'peek', not 'get' - if this turns out to be a problem, we should
     *          add a H5D__free_dxpl_cache() routine. -QAK)
     */
    if(H5P_peek(dx_plist, H5D_XFER_XFORM_NAME, &cache->data_xform_prop) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "Can't retrieve data transform info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5D__get_dxpl_cache_real() */


/*--------------------------------------------------------------------------
 NAME
    H5D__get_dxpl_cache
 PURPOSE
    Get all the values for the DXPL cache.
 USAGE
    herr_t H5D__get_dxpl_cache(dxpl_id, cache)
        hid_t dxpl_id;          IN: DXPL to query
        H5D_dxpl_cache_t *cache;IN/OUT: DXPL cache to fill with values
 RETURNS
    Non-negative on success/Negative on failure.
 DESCRIPTION
    Query all the values from a DXPL that are needed by internal routines
    within the library.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    The CACHE pointer should point at already allocated memory to place
    non-default property list info.  If a default property list is used, the
    CACHE pointer will be changed to point at the default information.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5D__get_dxpl_cache(hid_t dxpl_id, H5D_dxpl_cache_t **cache)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(cache);

    /* Check for the default DXPL */
    if(dxpl_id==H5P_DATASET_XFER_DEFAULT)
        *cache=&H5D_def_dxpl_cache;
    else
        if(H5D__get_dxpl_cache_real(dxpl_id,*cache) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "Can't retrieve DXPL values")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5D__get_dxpl_cache() */


/*-------------------------------------------------------------------------
 * Function:	H5D__create_named
 *
 * Purpose:	Internal routine to create a new dataset.
 *
 * Return:	Success:	Non-NULL, pointer to new dataset object.
 *
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *		Thursday, April 5, 2007
 *
 *-------------------------------------------------------------------------
 */
H5D_t *
H5D__create_named(const H5G_loc_t *loc, const char *name, hid_t type_id,
    const H5S_t *space, hid_t lcpl_id, hid_t dcpl_id, hid_t dapl_id,
    hid_t dxpl_id)
{
    H5O_obj_create_t ocrt_info;         /* Information for object creation */
    H5D_obj_create_t dcrt_info;         /* Information for dataset creation */
    H5D_t	   *ret_value = NULL;   /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check arguments */
    HDassert(loc);
    HDassert(name && *name);
    HDassert(type_id != H5P_DEFAULT);
    HDassert(space);
    HDassert(lcpl_id != H5P_DEFAULT);
    HDassert(dcpl_id != H5P_DEFAULT);
    HDassert(dapl_id != H5P_DEFAULT);
    HDassert(dxpl_id != H5P_DEFAULT);

    /* Set up dataset creation info */
    dcrt_info.type_id = type_id;
    dcrt_info.space = space;
    dcrt_info.dcpl_id = dcpl_id;
    dcrt_info.dapl_id = dapl_id;

    /* Set up object creation information */
    ocrt_info.obj_type = H5O_TYPE_DATASET;
    ocrt_info.crt_info = &dcrt_info;
    ocrt_info.new_obj = NULL;

    /* Create the new dataset and link it to its parent group */
    if(H5L_link_object(loc, name, &ocrt_info, lcpl_id, dapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to create and link to dataset")
    HDassert(ocrt_info.new_obj);

    /* Set the return value */
    ret_value = (H5D_t *)ocrt_info.new_obj;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__create_named() */


/*-------------------------------------------------------------------------
 * Function:    H5D__get_space_status
 *
 * Purpose:     Returns the status of data space allocation.
 *
 * Return:
 *              Success:        Non-negative
 *
 *              Failture:       Negative
 *
 * Programmer:  Raymond Lu
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__get_space_status(H5D_t *dset, H5D_space_status_t *allocation, hid_t dxpl_id)
{
    hsize_t     space_allocated;    /* The number of bytes allocated for chunks */
    hssize_t    snelmts;            /* Temporary holder for number of elements in dataspace */
    hsize_t     nelmts;             /* Number of elements in dataspace */
    size_t      dt_size;            /* Size of datatype */
    hsize_t     full_size;          /* The number of bytes in the dataset when fully populated */
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    HDassert(dset);

    /* Check for chunked layout */
    if(dset->shared->layout.type == H5D_CHUNKED) {
        /* For chunked layout set the space status by the storage size */
        /* Get the dataset's dataspace */
        HDassert(dset->shared->space);

        /* Get the total number of elements in dataset's dataspace */
        if((snelmts = H5S_GET_EXTENT_NPOINTS(dset->shared->space)) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to retrieve number of elements in dataspace")
        nelmts = (hsize_t)snelmts;

        /* Get the size of the dataset's datatype */
        if(0 == (dt_size = H5T_GET_SIZE(dset->shared->type)))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to retrieve size of datatype")

        /* Compute the maximum size of the dataset in bytes */
        full_size = nelmts * dt_size;

        /* Check for overflow during multiplication */
        if(nelmts != (full_size / dt_size))
            HGOTO_ERROR(H5E_DATASET, H5E_OVERFLOW, FAIL, "size of dataset's storage overflowed")

        /* Difficult to error check, since the error value is 0 and 0 is a valid value... :-/ */
        if(H5D__get_storage_size(dset, dxpl_id, &space_allocated) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get size of dataset's storage")

        /* Decide on how much of the space is allocated */
        if(space_allocated == 0)
            *allocation = H5D_SPACE_STATUS_NOT_ALLOCATED;
        else if(space_allocated == full_size)
            *allocation = H5D_SPACE_STATUS_ALLOCATED;
        else 
            *allocation = H5D_SPACE_STATUS_PART_ALLOCATED;
    } /* end if */
    else {
        /* For non-chunked layouts set space status by result of is_space_alloc
         * function */
        if(dset->shared->layout.ops->is_space_alloc(&dset->shared->layout.storage))
            *allocation = H5D_SPACE_STATUS_ALLOCATED;
        else
            *allocation = H5D_SPACE_STATUS_NOT_ALLOCATED;
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__get_space_status() */


/*-------------------------------------------------------------------------
 * Function:	H5D__new
 *
 * Purpose:	Creates a new, empty dataset structure
 *
 * Return:	Success:	Pointer to a new dataset descriptor.
 *
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *		Monday, October 12, 1998
 *
 *-------------------------------------------------------------------------
 */
static H5D_shared_t *
H5D__new(hid_t dcpl_id, hbool_t creating, hbool_t vl_type)
{
    H5D_shared_t    *new_dset = NULL;   /* New dataset object */
    H5P_genplist_t  *plist;             /* Property list created */
    H5D_shared_t    *ret_value = NULL;  /* Return value */

    FUNC_ENTER_STATIC

    /* Allocate new shared dataset structure */
    if(NULL == (new_dset = H5FL_MALLOC(H5D_shared_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Copy the default dataset information */
    HDmemcpy(new_dset, &H5D_def_dset, sizeof(H5D_shared_t));

    /* If we are using the default dataset creation property list, during creation
     * don't bother to copy it, just increment the reference count
     */
    if(!vl_type && creating && dcpl_id == H5P_DATASET_CREATE_DEFAULT) {
        if(H5I_inc_ref(dcpl_id, FALSE) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINC, NULL, "can't increment default DCPL ID")
        new_dset->dcpl_id = dcpl_id;
    } /* end if */
    else {
        /* Get the property list */
        if(NULL == (plist = (H5P_genplist_t *)H5I_object(dcpl_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a property list")

        new_dset->dcpl_id = H5P_copy_plist(plist, FALSE);
    } /* end else */

    /* Set return value */
    ret_value = new_dset;

done:
    if(ret_value == NULL)
        if(new_dset != NULL) {
            if(new_dset->dcpl_id != 0 && H5I_dec_ref(new_dset->dcpl_id) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CANTDEC, NULL, "can't decrement temporary datatype ID")
            new_dset = H5FL_FREE(H5D_shared_t, new_dset);
        } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__new() */


/*-------------------------------------------------------------------------
 * Function:	H5D__init_type
 *
 * Purpose:	Copy a datatype for a dataset's use, performing all the
 *              necessary adjustments, etc.
 *
 * Return:	Success:    SUCCEED
 *		Failure:    FAIL
 *
 * Programmer:	Quincey Koziol
 *		Thursday, June 24, 2004
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__init_type(H5F_t *file, const H5D_t *dset, hid_t type_id, const H5T_t *type)
{
    htri_t relocatable;                 /* Flag whether the type is relocatable */
    htri_t immutable;                   /* Flag whether the type is immutable */
    hbool_t use_latest_format;          /* Flag indicating the 'latest datatype version support' is enabled */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity checking */
    HDassert(file);
    HDassert(dset);
    HDassert(type);

    /* Check whether the datatype is relocatable */
    if((relocatable = H5T_is_relocatable(type)) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "can't check datatype?")

    /* Check whether the datatype is immutable */
    if((immutable = H5T_is_immutable(type)) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "can't check datatype?")

    /* Get the file's 'use the latest datatype version support' flag */
    use_latest_format = H5F_USE_LATEST_FLAGS(file, H5F_LATEST_DATATYPE);

    /* Copy the datatype if it's a custom datatype or if it'll change when it's location is changed */
    if(!immutable || relocatable || use_latest_format) {
        /* Copy datatype for dataset */
        if((dset->shared->type = H5T_copy(type, H5T_COPY_ALL)) == NULL)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTCOPY, FAIL, "can't copy datatype")

	/* Convert a datatype (if committed) to a transient type if the committed datatype's file
	   location is different from the file location where the dataset will be created */
        if(H5T_convert_committed_datatype(dset->shared->type, file) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't get shared datatype info")

        /* Mark any datatypes as being on disk now */
        if(H5T_set_loc(dset->shared->type, file, H5T_LOC_DISK) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't set datatype location")

        /* Set the latest format, if requested */
        if(use_latest_format)
            if(H5T_set_latest_version(dset->shared->type) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set latest version of datatype")

        /* Get a datatype ID for the dataset's datatype */
	if((dset->shared->type_id = H5I_register(H5I_DATATYPE, dset->shared->type, FALSE)) < 0)
	    HGOTO_ERROR(H5E_DATASET, H5E_CANTREGISTER, FAIL, "unable to register type")
    } /* end if */
    /* Not a custom datatype, just use it directly */
    else {
        if(H5I_inc_ref(type_id, FALSE) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINC, FAIL, "Can't increment datatype ID")

        /* Use existing datatype */
        dset->shared->type_id = type_id;
        dset->shared->type = (H5T_t *)type; /* (Cast away const OK - QAK) */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__init_type() */


/*-------------------------------------------------------------------------
 * Function:	H5D__cache_dataspace_info
 *
 * Purpose:	Cache dataspace info for a dataset
 *
 * Return:	Success:    SUCCEED
 *		Failure:    FAIL
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, November 19, 2014
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__cache_dataspace_info(const H5D_t *dset)
{
    int sndims;                         /* Signed number of dimensions of dataspace rank */
    unsigned u;                         /* Local index value */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity checking */
    HDassert(dset);

    /* Cache info for dataset's dataspace */
    if((sndims = H5S_get_simple_extent_dims(dset->shared->space, dset->shared->curr_dims, dset->shared->max_dims)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't cache dataspace dimensions")
    dset->shared->ndims = (unsigned)sndims;

    /* Compute the inital 'power2up' values */
    for(u = 0; u < dset->shared->ndims; u++)
        dset->shared->curr_power2up[u] = H5VM_power2up(dset->shared->curr_dims[u]);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__cache_dataspace_info() */


/*-------------------------------------------------------------------------
 * Function:	H5D__init_space
 *
 * Purpose:	Copy a dataspace for a dataset's use, performing all the
 *              necessary adjustments, etc.
 *
 * Return:	Success:    SUCCEED
 *		Failure:    FAIL
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, July 24, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__init_space(H5F_t *file, const H5D_t *dset, const H5S_t *space)
{
    hbool_t use_latest_format;          /* Flag indicating the 'latest dataspace version support' is enabled */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity checking */
    HDassert(file);
    HDassert(dset);
    HDassert(space);

    /* Get the file's 'use the latest dataspace version support' flag */
    use_latest_format = H5F_USE_LATEST_FLAGS(file, H5F_LATEST_DATASPACE);

    /* Copy dataspace for dataset */
    if(NULL == (dset->shared->space = H5S_copy(space, FALSE, TRUE)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTCOPY, FAIL, "can't copy dataspace")

    /* Cache the dataset's dataspace info */
    if(H5D__cache_dataspace_info(dset) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTCOPY, FAIL, "can't cache dataspace info")

    /* Set the latest format, if requested */
    if(use_latest_format)
        if(H5S_set_latest_version(dset->shared->space) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set latest version of datatype")

    /* Set the dataset's dataspace to 'all' selection */
    if(H5S_select_all(dset->shared->space, TRUE) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to set all selection")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__init_space() */


/*-------------------------------------------------------------------------
 * Function:	H5D__update_oh_info
 *
 * Purpose:	Create and fill object header for dataset
 *
 * Return:	Success:    SUCCEED
 *		Failure:    FAIL
 *
 * Programmer:	Bill Wendling
 *		Thursday, October 31, 2002
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__update_oh_info(H5F_t *file, hid_t dxpl_id, H5D_t *dset, hid_t dapl_id)
{
    H5O_t              *oh = NULL;      /* Pointer to dataset's object header */
    size_t              ohdr_size = H5D_MINHDR_SIZE;    /* Size of dataset's object header */
    H5O_loc_t          *oloc = NULL;    /* Dataset's object location */
    H5O_layout_t       *layout;         /* Dataset's layout information */
    H5T_t              *type;           /* Dataset's datatype */
    H5O_fill_t		*fill_prop;     /* Pointer to dataset's fill value information */
    H5D_fill_value_t	fill_status;    /* Fill value status */
    hbool_t             fill_changed = FALSE;      /* Flag indicating the fill value was changed */
    hbool_t             layout_init = FALSE;    /* Flag to indicate that chunk information was initialized */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity checking */
    HDassert(file);
    HDassert(dset);

    /* Set some local variables, for convenience */
    oloc = &dset->oloc;
    layout = &dset->shared->layout;
    type = dset->shared->type;
    fill_prop = &dset->shared->dcpl_cache.fill;

    /* Retrieve "defined" status of fill value */
    if(H5P_is_fill_value_defined(fill_prop, &fill_status) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't tell if fill value defined")

    /* Special case handling for variable-length types */
    if(H5T_detect_class(type, H5T_VLEN, FALSE)) {
        /* If the default fill value is chosen for variable-length types, always write it */
        if(fill_prop->fill_time == H5D_FILL_TIME_IFSET && fill_status == H5D_FILL_VALUE_DEFAULT) {
            /* Update dataset creation property */
            fill_prop->fill_time = H5D_FILL_TIME_ALLOC;

            /* Note that the fill value changed */
            fill_changed = TRUE;
        } /* end if */

        /* Don't allow never writing fill values with variable-length types */
        if(fill_prop->fill_time == H5D_FILL_TIME_NEVER)
            HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "Dataset doesn't support VL datatype when fill value is not defined")
    } /* end if */

    /* Determine whether fill value is defined or not */
    if(fill_status == H5D_FILL_VALUE_DEFAULT || fill_status == H5D_FILL_VALUE_USER_DEFINED) {
        /* Convert fill value buffer to dataset's datatype */
        if(fill_prop->buf && fill_prop->size > 0 && H5O_fill_convert(fill_prop, type, &fill_changed, dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to convert fill value to dataset type")

	fill_prop->fill_defined = TRUE;
    } else if(fill_status == H5D_FILL_VALUE_UNDEFINED) {
 	fill_prop->fill_defined = FALSE;
    } else
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to determine if fill value is defined")

    /* Check for invalid fill & allocation time setting */
    if(fill_prop->fill_defined == FALSE && fill_prop->fill_time == H5D_FILL_TIME_ALLOC)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "fill value writing on allocation set, but no fill value defined")

    /* Check if the fill value info changed */
    if(fill_changed) {
        H5P_genplist_t     *dc_plist;               /* Dataset's creation property list */

        /* Get dataset's property list object */
        HDassert(dset->shared->dcpl_id != H5P_DATASET_CREATE_DEFAULT);
        if(NULL == (dc_plist = (H5P_genplist_t *)H5I_object(dset->shared->dcpl_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "can't get dataset creation property list")

        /* Update dataset creation property */
        if(H5P_set(dc_plist, H5D_CRT_FILL_VALUE_NAME, fill_prop) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set fill value info")
    } /* end if */

    /* Add the dataset's raw data size to the size of the header, if the raw data will be stored as compact */
    if(layout->type == H5D_COMPACT)
        ohdr_size += layout->storage.u.compact.size;

    /* Create an object header for the dataset */
    if(H5O_create(file, dxpl_id, ohdr_size, (size_t)1, dset->shared->dcpl_id, oloc/*out*/) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to create dataset object header")
    HDassert(file == dset->oloc.file);

    /* Pin the object header */
    if(NULL == (oh = H5O_pin(oloc, dxpl_id)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTPIN, FAIL, "unable to pin dataset object header")

    /* Write the dataspace header message */
    if(H5S_append(file, dxpl_id, oh, dset->shared->space) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update dataspace header message")

    /* Write the datatype header message */
    if(H5O_msg_append_oh(file, dxpl_id, oh, H5O_DTYPE_ID, H5O_MSG_FLAG_CONSTANT, 0, type) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update datatype header message")

    /* Write new fill value message */
    if(H5O_msg_append_oh(file, dxpl_id, oh, H5O_FILL_NEW_ID, H5O_MSG_FLAG_CONSTANT, 0, fill_prop) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update new fill value header message")

    /* If there is valid information for the old fill value struct, add it */
    /* (only if we aren't trying to write the 'latest fill message version support') */
    if(fill_prop->buf && !(H5F_USE_LATEST_FLAGS(file, H5F_LATEST_FILL_MSG))) {
        H5O_fill_t old_fill_prop;       /* Copy of fill value property, for writing as "old" fill value */

        /* Shallow copy the fill value property */
        /* (we only want to make certain that the shared component isnt' modified) */
        HDmemcpy(&old_fill_prop, fill_prop, sizeof(old_fill_prop));

        /* Reset shared component info */
        H5O_msg_reset_share(H5O_FILL_ID, &old_fill_prop);

        /* Write old fill value */
        if(H5O_msg_append_oh(file, dxpl_id, oh, H5O_FILL_ID, H5O_MSG_FLAG_CONSTANT, 0, &old_fill_prop) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update old fill value header message")
    } /* end if */

    /* Update/create the layout (and I/O pipeline & EFL) messages */
    if(H5D__layout_oh_create(file, dxpl_id, oh, dset, dapl_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update layout/pline/efl header message")

    /* Indicate that the layout information was initialized */
    layout_init = TRUE;

#ifdef H5O_ENABLE_BOGUS
{
    H5P_genplist_t     *dc_plist;               /* Dataset's creation property list */

    /* Get dataset's property list object */
    if(NULL == (dc_plist = (H5P_genplist_t *)H5I_object(dset->shared->dcpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "can't get dataset creation property list")

    /* Check whether to add a "bogus" message */
    if( (H5P_exist_plist(dc_plist, H5O_BOGUS_MSG_FLAGS_NAME) > 0) &&
        (H5P_exist_plist(dc_plist, H5O_BOGUS_MSG_ID_NAME) > 0) ) {

        uint8_t bogus_flags = 0;        /* Flags for creating "bogus" message */
	unsigned bogus_id;		/* "bogus" ID */

	/* Retrieve "bogus" message ID */
        if(H5P_get(dc_plist, H5O_BOGUS_MSG_ID_NAME, &bogus_id) < 0)
	    HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get bogus ID options")
        /* Retrieve "bogus" message flags */
        if(H5P_get(dc_plist, H5O_BOGUS_MSG_FLAGS_NAME, &bogus_flags) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get bogus message options")

        /* Add a "bogus" message (for error testing). */
	if(H5O_bogus_oh(file, dxpl_id, oh, bogus_id, (unsigned)bogus_flags) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to create 'bogus' message")
    } /* end if */
}
#endif /* H5O_ENABLE_BOGUS */

    /* Add a modification time message, if using older format. */
    /* (If using the latest 'no modification time message' version support, the modification time is part of the object
     *  header and doesn't use a separate message -QAK)
     */
    if(!(H5F_USE_LATEST_FLAGS(file, H5F_LATEST_NO_MOD_TIME_MSG)))
        if(H5O_touch_oh(file, dxpl_id, oh, TRUE) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update modification time message")

done:
    /* Release pointer to object header itself */
    if(oh != NULL)
        if(H5O_unpin(oh) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTUNPIN, FAIL, "unable to unpin dataset object header")

    /* Error cleanup */
    if(ret_value < 0)
        if(layout_init)
            /* Destroy the layout information for the dataset */
            if(dset->shared->layout.ops->dest && (dset->shared->layout.ops->dest)(dset, dxpl_id) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "unable to destroy layout info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__update_oh_info() */


/*--------------------------------------------------------------------------
 * Function:    H5D_build_extfile_prefix
 *
 * Purpose:     Determine the external file prefix to be used and store
 *              it in extfile_prefix. Stores an empty string if no prefix
 *              should be used.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Steffen Kiess
 *              October 16, 2015
 *--------------------------------------------------------------------------
 */
static herr_t
H5D_build_extfile_prefix(const H5D_t *dset, hid_t dapl_id, char **extfile_prefix /*out*/)
{
    char            *prefix = NULL;       /* prefix used to look for the file               */
    char            *extpath = NULL;      /* absolute path of directory the HDF5 file is in */
    size_t          extpath_len;          /* length of extpath                              */
    size_t          prefix_len;           /* length of prefix                               */
    size_t          extfile_prefix_len;   /* length of expanded prefix                      */
    H5P_genplist_t  *plist = NULL;        /* Property list pointer                          */
    herr_t          ret_value = SUCCEED;  /* Return value                                   */
    

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(dset);
    HDassert(dset->oloc.file);

    extpath = H5F_EXTPATH(dset->oloc.file);
    HDassert(extpath);

    /* XXX: Future thread-safety note - getenv is not required
     *      to be reentrant.
     */
    prefix = HDgetenv("HDF5_EXTFILE_PREFIX");

    if(prefix == NULL || *prefix == '\0') {
        /* Set prefix to value of H5D_ACS_EFILE_PREFIX_NAME property */
        if(NULL == (plist = H5P_object_verify(dapl_id, H5P_DATASET_ACCESS)))
            HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")
        if(H5P_peek(plist, H5D_ACS_EFILE_PREFIX_NAME, &prefix) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get external file prefix")
    } /* end if */

    /* Prefix has to be checked for NULL / empty string again because the
     * code above might have updated it.
     */
    if(prefix == NULL || *prefix == '\0' || HDstrcmp(prefix, ".") == 0) {
        /* filename is interpreted as relative to the current directory,
         * does not need to be expanded
         */
        if(NULL == (*extfile_prefix = (char *)H5MM_strdup("")))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
    } /* end if */
    else {
        if (HDstrncmp(prefix, "${ORIGIN}", HDstrlen("${ORIGIN}")) == 0) {
            /* Replace ${ORIGIN} at beginning of prefix by directory of HDF5 file */
            extpath_len = HDstrlen(extpath);
            prefix_len = HDstrlen(prefix);
            extfile_prefix_len = extpath_len + prefix_len - HDstrlen("${ORIGIN}") + 1;
        
            if(NULL == (*extfile_prefix = (char *)H5MM_malloc(extfile_prefix_len)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate buffer")
            HDsnprintf(*extfile_prefix, extfile_prefix_len, "%s%s", extpath, prefix + HDstrlen("${ORIGIN}"));
        } /* end if */
        else {
            if(NULL == (*extfile_prefix = (char *)H5MM_strdup(prefix)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
        } /* end else */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D_build_extfile_prefix() */


/*-------------------------------------------------------------------------
 * Function:	H5D__create
 *
 * Purpose:	Creates a new dataset with name NAME in file F and associates
 *		with it a datatype TYPE for each element as stored in the
 *		file, dimensionality information or dataspace SPACE, and
 *		other miscellaneous properties CREATE_PARMS.  All arguments
 *		are deep-copied before being associated with the new dataset,
 *		so the caller is free to subsequently modify them without
 *		affecting the dataset.
 *
 * Return:	Success:	Pointer to a new dataset
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *		Thursday, December  4, 1997
 *
 *-------------------------------------------------------------------------
 */
H5D_t *
H5D__create(H5F_t *file, hid_t type_id, const H5S_t *space, hid_t dcpl_id,
    hid_t dapl_id, hid_t dxpl_id)
{
    const H5T_t         *type;                  /* Datatype for dataset */
    H5D_t		*new_dset = NULL;
    H5P_genplist_t 	*dc_plist = NULL;       /* New Property list */
    hbool_t             has_vl_type = FALSE;    /* Flag to indicate a VL-type for dataset */
    hbool_t             layout_init = FALSE;    /* Flag to indicate that chunk information was initialized */
    hbool_t             layout_copied = FALSE;  /* Flag to indicate that layout message was copied */
    hbool_t             fill_copied = FALSE;    /* Flag to indicate that fill-value message was copied */
    hbool_t             pline_copied = FALSE;   /* Flag to indicate that pipeline message was copied */
    hbool_t             efl_copied = FALSE;     /* Flag to indicate that external file list message was copied */
    H5G_loc_t           dset_loc;               /* Dataset location */
    H5D_t		*ret_value = NULL;      /* Return value */

    FUNC_ENTER_PACKAGE

    /* check args */
    HDassert(file);
    HDassert(H5I_DATATYPE == H5I_get_type(type_id));
    HDassert(space);
    HDassert(H5I_GENPROP_LST == H5I_get_type(dcpl_id));
    HDassert(H5I_GENPROP_LST == H5I_get_type(dxpl_id));

    /* Get the dataset's datatype */
    if(NULL == (type = (const H5T_t *)H5I_object(type_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a datatype")

    /* Check if the datatype is "sensible" for use in a dataset */
    if(H5T_is_sensible(type) != TRUE)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "datatype is not sensible")

    /* Check if the datatype is/contains a VL-type */
    if(H5T_detect_class(type, H5T_VLEN, FALSE))
        has_vl_type = TRUE;

    /* Check if the dataspace has an extent set (or is NULL) */
    if(!H5S_has_extent(space))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "dataspace extent has not been set.")

    /* Initialize the dataset object */
    if(NULL == (new_dset = H5FL_CALLOC(H5D_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Set up & reset dataset location */
    dset_loc.oloc = &(new_dset->oloc);
    dset_loc.path = &(new_dset->path);
    H5G_loc_reset(&dset_loc);

    /* Initialize the shared dataset space */
    if(NULL == (new_dset->shared = H5D__new(dcpl_id, TRUE, has_vl_type)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Copy & initialize datatype for dataset */
    if(H5D__init_type(file, new_dset, type_id, type) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "can't copy datatype")

    /* Copy & initialize dataspace for dataset */
    if(H5D__init_space(file, new_dset, space) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "can't copy dataspace")

    /* Set the dataset's checked_filters flag to enable writing */
    new_dset->shared->checked_filters = TRUE;

    /* Check if the dataset has a non-default DCPL & get important values, if so */
    if(new_dset->shared->dcpl_id != H5P_DATASET_CREATE_DEFAULT) {
        H5O_layout_t    *layout;        /* Dataset's layout information */
        H5O_pline_t     *pline;         /* Dataset's I/O pipeline information */
        H5O_fill_t      *fill;          /* Dataset's fill value info */
        H5O_efl_t       *efl;           /* Dataset's external file list info */

        /* Check if the filters in the DCPL can be applied to this dataset */
        if(H5Z_can_apply(new_dset->shared->dcpl_id, new_dset->shared->type_id) < 0)
            HGOTO_ERROR(H5E_ARGS, H5E_CANTINIT, NULL, "I/O filters can't operate on this dataset")

        /* Make the "set local" filter callbacks for this dataset */
        if(H5Z_set_local(new_dset->shared->dcpl_id, new_dset->shared->type_id) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to set local filter parameters")

        /* Get new dataset's property list object */
        if(NULL == (dc_plist = (H5P_genplist_t *)H5I_object(new_dset->shared->dcpl_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "can't get dataset creation property list")

        /* Retrieve the properties we need */
        pline = &new_dset->shared->dcpl_cache.pline;
        if(H5P_get(dc_plist, H5O_CRT_PIPELINE_NAME, pline) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, NULL, "can't retrieve pipeline filter")
        pline_copied = TRUE;
        layout = &new_dset->shared->layout;
        if(H5P_get(dc_plist, H5D_CRT_LAYOUT_NAME, layout) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, NULL, "can't retrieve layout")
        layout_copied = TRUE;
        fill = &new_dset->shared->dcpl_cache.fill;
        if(H5P_get(dc_plist, H5D_CRT_FILL_VALUE_NAME, fill) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, NULL, "can't retrieve fill value info")
        fill_copied = TRUE;
        efl = &new_dset->shared->dcpl_cache.efl;
        if(H5P_get(dc_plist, H5D_CRT_EXT_FILE_LIST_NAME, efl) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, NULL, "can't retrieve external file list")
        efl_copied = TRUE;

        /* Check that chunked layout is used if filters are enabled */
        if(pline->nused > 0 && H5D_CHUNKED != layout->type)
            HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, NULL, "filters can only be used with chunked layout")

        /* Check if the alloc_time is the default and error out */
        if(fill->alloc_time == H5D_ALLOC_TIME_DEFAULT)
            HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, NULL, "invalid space allocation state")

        /* Don't allow compact datasets to allocate space later */
        if(layout->type == H5D_COMPACT && fill->alloc_time != H5D_ALLOC_TIME_EARLY)
            HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, NULL, "compact dataset must have early space allocation")

        /* If MPI VFD is used, no filter support yet. */
        if(H5F_HAS_FEATURE(file, H5FD_FEAT_HAS_MPI) && pline->nused > 0)
            HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, NULL, "Parallel I/O does not support filters yet")
    } /* end if */

    /* Set the latest version of the layout, pline & fill messages, if requested */
    if(H5F_USE_LATEST_FLAGS(file, H5F_LATEST_DSET_MSG_FLAGS)) {
        /* Set the latest version for the I/O pipeline message */
	if(H5F_USE_LATEST_FLAGS(file, H5F_LATEST_PLINE_MSG))
	    if(H5O_pline_set_latest_version(&new_dset->shared->dcpl_cache.pline) < 0)
		HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, NULL, "can't set latest version of I/O filter pipeline")

        /* Set the latest version for the fill message */
	if(H5F_USE_LATEST_FLAGS(file, H5F_LATEST_FILL_MSG))
	    /* Set the latest version for the fill value message */
	    if(H5O_fill_set_latest_version(&new_dset->shared->dcpl_cache.fill) < 0)
		HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, NULL, "can't set latest version of fill value")

        /* Set the latest version for the layout message */
	if(H5F_USE_LATEST_FLAGS(file, H5F_LATEST_LAYOUT_MSG))
	    /* Set the latest version for the layout message */
	    if(H5D__layout_set_latest_version(&new_dset->shared->layout, new_dset->shared->space, &new_dset->shared->dcpl_cache) < 0)
		HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, NULL, "can't set latest version of layout")
    } /* end if */
    else if(new_dset->shared->layout.version >= H5O_LAYOUT_VERSION_4) {
	/* Use latest indexing type for layout message version >= 4 */
        if(H5D__layout_set_latest_indexing(&new_dset->shared->layout, new_dset->shared->space, &new_dset->shared->dcpl_cache) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, NULL, "can't set latest indexing")
    } /* end if */

    /* Check if this dataset is going into a parallel file and set space allocation time */
    if(H5F_HAS_FEATURE(file, H5FD_FEAT_ALLOCATE_EARLY))
        new_dset->shared->dcpl_cache.fill.alloc_time = H5D_ALLOC_TIME_EARLY;

    /* Set the dataset's I/O operations */
    if(H5D__layout_set_io_ops(new_dset) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to initialize I/O operations")

    /* Create the layout information for the new dataset */
    if(new_dset->shared->layout.ops->construct && (new_dset->shared->layout.ops->construct)(file, new_dset) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to construct layout information")

    /* Update the dataset's object header info. */
    if(H5D__update_oh_info(file, dxpl_id, new_dset, dapl_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "can't update the metadata cache")

    /* Indicate that the layout information was initialized */
    layout_init = TRUE;

    /* Set up append flush parameters for the dataset */
    if(H5D__append_flush_setup(new_dset, dapl_id) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to set up flush append property")

    /* Set the external file prefix */
    if(H5D_build_extfile_prefix(new_dset, dapl_id, &new_dset->shared->extfile_prefix) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to initialize external file prefix")

    /* Add the dataset to the list of opened objects in the file */
    if(H5FO_top_incr(new_dset->oloc.file, new_dset->oloc.addr) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINC, NULL, "can't incr object ref. count")
    if(H5FO_insert(new_dset->oloc.file, new_dset->oloc.addr, new_dset->shared, TRUE) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, NULL, "can't insert dataset into list of open objects")
    new_dset->shared->fo_count = 1;

    /* Success */
    ret_value = new_dset;

done:
    if(!ret_value && new_dset && new_dset->shared) {
        if(new_dset->shared) {
            if(layout_init)
                if(new_dset->shared->layout.ops->dest && (new_dset->shared->layout.ops->dest)(new_dset, dxpl_id) < 0)
                    HDONE_ERROR(H5E_DATASET, H5E_CANTRELEASE, NULL, "unable to destroy layout info")
            if(pline_copied)
                if(H5O_msg_reset(H5O_PLINE_ID, &new_dset->shared->dcpl_cache.pline) < 0)
                    HDONE_ERROR(H5E_DATASET, H5E_CANTRESET, NULL, "unable to reset I/O pipeline info")
            if(layout_copied)
                if(H5O_msg_reset(H5O_LAYOUT_ID, &new_dset->shared->layout) < 0)
                    HDONE_ERROR(H5E_DATASET, H5E_CANTRESET, NULL, "unable to reset layout info")
            if(fill_copied)
                if(H5O_msg_reset(H5O_FILL_ID, &new_dset->shared->dcpl_cache.fill) < 0)
                    HDONE_ERROR(H5E_DATASET, H5E_CANTRESET, NULL, "unable to reset fill-value info")
            if(efl_copied)
                if(H5O_msg_reset(H5O_EFL_ID, &new_dset->shared->dcpl_cache.efl) < 0)
                    HDONE_ERROR(H5E_DATASET, H5E_CANTRESET, NULL, "unable to reset external file list info")
            if(new_dset->shared->space && H5S_close(new_dset->shared->space) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, NULL, "unable to release dataspace")
            if(new_dset->shared->type && H5I_dec_ref(new_dset->shared->type_id) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, NULL, "unable to release datatype")
            if(H5F_addr_defined(new_dset->oloc.addr)) {
                if(H5O_dec_rc_by_loc(&(new_dset->oloc), dxpl_id) < 0)
                    HDONE_ERROR(H5E_DATASET, H5E_CANTDEC, NULL, "unable to decrement refcount on newly created object")
                if(H5O_close(&(new_dset->oloc), NULL) < 0)
                    HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, NULL, "unable to release object header")
                if(file) {
                    if(H5O_delete(file, dxpl_id, new_dset->oloc.addr) < 0)
                        HDONE_ERROR(H5E_DATASET, H5E_CANTDELETE, NULL, "unable to delete object header")
                } /* end if */
            } /* end if */
            if(new_dset->shared->dcpl_id != 0 && H5I_dec_ref(new_dset->shared->dcpl_id) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CANTDEC, NULL, "unable to decrement ref count on property list")
            new_dset->shared->extfile_prefix = (char *)H5MM_xfree(new_dset->shared->extfile_prefix);
            new_dset->shared = H5FL_FREE(H5D_shared_t, new_dset->shared);
        } /* end if */
        new_dset->oloc.file = NULL;
        new_dset = H5FL_FREE(H5D_t, new_dset);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__create() */


/*-------------------------------------------------------------------------
 * Function:    H5D__open_name
 *
 * Purpose:     Opens an existing dataset by name.
 *
 * Return:      Success:        Ptr to a new dataset.
 *              Failure:        NULL
 *
 * Programmer:  Neil Fortner
 *              Friday, March 6, 2015
 *
 *-------------------------------------------------------------------------
 */
H5D_t *
H5D__open_name(const H5G_loc_t *loc, const char *name, hid_t dapl_id,
    hid_t dxpl_id)
{
    H5D_t       *dset = NULL;
    H5G_loc_t   dset_loc;               /* Object location of dataset */
    H5G_name_t  path;                   /* Dataset group hier. path */
    H5O_loc_t   oloc;                   /* Dataset object location */
    H5O_type_t  obj_type;               /* Type of object at location */
    hbool_t     loc_found = FALSE;      /* Location at 'name' found */
    H5D_t       *ret_value = NULL;      /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(loc);
    HDassert(name);

    /* Set up dataset location to fill in */
    dset_loc.oloc = &oloc;
    dset_loc.path = &path;
    H5G_loc_reset(&dset_loc);

    /* Find the dataset object */
    if(H5G_loc_find(loc, name, &dset_loc, dapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_NOTFOUND, NULL, "not found")
    loc_found = TRUE;

    /* Check that the object found is the correct type */
    if(H5O_obj_type(&oloc, &obj_type, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, NULL, "can't get object type")
    if(obj_type != H5O_TYPE_DATASET)
        HGOTO_ERROR(H5E_DATASET, H5E_BADTYPE, NULL, "not a dataset")

    /* Open the dataset */
    if(NULL == (dset = H5D_open(&dset_loc, dapl_id, dxpl_id)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "can't open dataset")

    /* Set return value */
    ret_value = dset;

done:
    if(!ret_value)
        if(loc_found && H5G_loc_free(&dset_loc) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTRELEASE, NULL, "can't free location")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__open_name() */


/*
 *-------------------------------------------------------------------------
 * Function:	H5D_open
 *
 * Purpose:	Checks if dataset is already open, or opens a dataset for
 *              access.
 *
 * Return:	Success:	Dataset ID
 *		Failure:	FAIL
 *
 * Programmer:	Quincey Koziol
 *		Friday, December 20, 2002
 *
 *-------------------------------------------------------------------------
 */
H5D_t *
H5D_open(const H5G_loc_t *loc, hid_t dapl_id, hid_t dxpl_id)
{
    H5D_shared_t    *shared_fo = NULL;
    H5D_t           *dataset = NULL;
    char            *extfile_prefix = NULL;  /* Expanded external file prefix */
    H5D_t           *ret_value = NULL;       /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* check args */
    HDassert(loc);

    /* Allocate the dataset structure */
    if(NULL == (dataset = H5FL_CALLOC(H5D_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Shallow copy (take ownership) of the object location object */
    if(H5O_loc_copy(&(dataset->oloc), loc->oloc, H5_COPY_SHALLOW) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTCOPY, NULL, "can't copy object location")

    /* Shallow copy (take ownership) of the group hier. path */
    if(H5G_name_copy(&(dataset->path), loc->path, H5_COPY_SHALLOW) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTCOPY, NULL, "can't copy path")

    /* Get the external file prefix */
    if(H5D_build_extfile_prefix(dataset, dapl_id, &extfile_prefix) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to initialize external file prefix")

    /* Check if dataset was already open */
    if(NULL == (shared_fo = (H5D_shared_t *)H5FO_opened(dataset->oloc.file, dataset->oloc.addr))) {
        /* Clear any errors from H5FO_opened() */
        H5E_clear_stack(NULL);

        /* Open the dataset object */
        if(H5D__open_oid(dataset, dapl_id, dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_NOTFOUND, NULL, "not found")

        /* Add the dataset to the list of opened objects in the file */
        if(H5FO_insert(dataset->oloc.file, dataset->oloc.addr, dataset->shared, FALSE) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, NULL, "can't insert dataset into list of open objects")

        /* Increment object count for the object in the top file */
        if(H5FO_top_incr(dataset->oloc.file, dataset->oloc.addr) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINC, NULL, "can't increment object count")

        /* We're the first dataset to use the the shared info */
        dataset->shared->fo_count = 1;

        /* Set the external file prefix */
        dataset->shared->extfile_prefix = extfile_prefix;
        /* Prevent string from being freed during done: */
        extfile_prefix = NULL;

    } /* end if */
    else {
        /* Point to shared info */
        dataset->shared = shared_fo;

        /* Increment # of datasets using shared information */
        shared_fo->fo_count++;

        /* Check whether the external file prefix of the already open dataset
         * matches the new external file prefix
         */
        if(HDstrcmp(extfile_prefix, dataset->shared->extfile_prefix) != 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTOPENOBJ, NULL, "new external file prefix does not match external file prefix of already open dataset")

        /* Check if the object has been opened through the top file yet */
        if(H5FO_top_count(dataset->oloc.file, dataset->oloc.addr) == 0) {
            /* Open the object through this top file */
            if(H5O_open(&(dataset->oloc)) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTOPENOBJ, NULL, "unable to open object header")
        } /* end if */

        /* Increment object count for the object in the top file */
        if(H5FO_top_incr(dataset->oloc.file, dataset->oloc.addr) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINC, NULL, "can't increment object count")
    } /* end else */

    /* Set the dataset to return */
    ret_value = dataset;

done:
    extfile_prefix = (char *)H5MM_xfree(extfile_prefix);

    if(ret_value == NULL) {
        /* Free the location--casting away const*/
        if(dataset) {
            if(shared_fo == NULL && dataset->shared) {   /* Need to free shared fo */
                dataset->shared->extfile_prefix = (char *)H5MM_xfree(dataset->shared->extfile_prefix);
                dataset->shared = H5FL_FREE(H5D_shared_t, dataset->shared);
            }

            H5O_loc_free(&(dataset->oloc));
            H5G_name_free(&(dataset->path));

            dataset = H5FL_FREE(H5D_t, dataset);
        } /* end if */
        if(shared_fo)
            shared_fo->fo_count--;
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_open() */


/*
 *-------------------------------------------------------------------------
 * Function:	H5D__flush_append_setup
 *
 * Purpose:	Set the append flush parameters for a dataset
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Vailin Choi
 *		Wednesday, January 8, 2014
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__append_flush_setup(H5D_t *dset, hid_t dapl_id) 
{
    herr_t ret_value = SUCCEED;         /* return value */

    FUNC_ENTER_STATIC

    /* Check args */
    HDassert(dset);
    HDassert(dset->shared);

    /* Set default append flush values */
    HDmemset(&dset->shared->append_flush,  0, sizeof(dset->shared->append_flush));

    /* If the dataset is chunked and there is a non-default DAPL */
    if(dapl_id != H5P_DATASET_ACCESS_DEFAULT && dset->shared->layout.type == H5D_CHUNKED) {
        H5P_genplist_t *dapl;               /* data access property list object pointer */

	/* Get dataset access property list */
	if(NULL == (dapl = (H5P_genplist_t *)H5I_object(dapl_id)))
	    HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for dapl ID");

	/* Check if append flush property exists */
	if(H5P_exist_plist(dapl, H5D_ACS_APPEND_FLUSH_NAME) > 0) {
            H5D_append_flush_t info;

	    /* Get append flush property */
	    if(H5P_get(dapl, H5D_ACS_APPEND_FLUSH_NAME, &info) < 0)
		HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get append flush info")
	    if(info.ndims > 0) {
                hsize_t curr_dims[H5S_MAX_RANK];	/* current dimension sizes */
                hsize_t max_dims[H5S_MAX_RANK];		/* current dimension sizes */
                int rank;                       	/* dataspace # of dimensions */
                unsigned u;     			/* local index variable */

		/* Get dataset rank */
		if((rank = H5S_get_simple_extent_dims(dset->shared->space, curr_dims, max_dims)) < 0)
		    HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get dataset dimensions")
		if(info.ndims != (unsigned)rank)
		    HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, FAIL, "boundary dimension rank does not match dataset rank")

		/* Validate boundary sizes */
		for(u = 0; u < info.ndims; u++)
		    if(info.boundary[u] != 0) /* when a non-zero boundary is set */
			/* the dimension is extendible? */
			if(max_dims[u] != H5S_UNLIMITED && max_dims[u] == curr_dims[u])
			    break;

                /* At least one boundary dimension is not extendible */
		if(u != info.ndims)
		    HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, FAIL, "boundary dimension is not valid")

                /* Copy append flush settings */
		dset->shared->append_flush.ndims = info.ndims;
		dset->shared->append_flush.func = info.func;
		dset->shared->append_flush.udata = info.udata;
		HDmemcpy(dset->shared->append_flush.boundary, info.boundary, sizeof(info.boundary));
	    } /* end if */
	} /* end if */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D__append_flush_setup() */


/*-------------------------------------------------------------------------
 * Function:	H5D__open_oid
 *
 * Purpose:	Opens a dataset for access.
 *
 * Return:	Dataset pointer on success, NULL on failure
 *
 * Programmer:	Quincey Koziol
 *		Monday, October 12, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__open_oid(H5D_t *dataset, hid_t dapl_id, hid_t dxpl_id)
{
    H5P_genplist_t *plist;              /* Property list */
    H5O_fill_t *fill_prop;              /* Pointer to dataset's fill value info */
    unsigned alloc_time_state;          /* Allocation time state */
    htri_t msg_exists;                  /* Whether a particular type of message exists */
    hbool_t layout_init = FALSE;    	/* Flag to indicate that chunk information was initialized */
    herr_t ret_value = SUCCEED;		/* Return value */

    FUNC_ENTER_STATIC_TAG(dxpl_id, dataset->oloc.addr, FAIL)

    /* check args */
    HDassert(dataset);

    /* (Set the 'vl_type' parameter to FALSE since it doesn't matter from here) */
    if(NULL == (dataset->shared = H5D__new(H5P_DATASET_CREATE_DEFAULT, FALSE, FALSE)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Open the dataset object */
    if(H5O_open(&(dataset->oloc)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTOPENOBJ, FAIL, "unable to open")

    /* Get the type and space */
    if(NULL == (dataset->shared->type = (H5T_t *)H5O_msg_read(&(dataset->oloc), H5O_DTYPE_ID, NULL, dxpl_id)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to load type info from dataset header")

    if(H5T_set_loc(dataset->shared->type, dataset->oloc.file, H5T_LOC_DISK) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "invalid datatype location")

    if(NULL == (dataset->shared->space = H5S_read(&(dataset->oloc), dxpl_id)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to load dataspace info from dataset header")

    /* Cache the dataset's dataspace info */
    if(H5D__cache_dataspace_info(dataset) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTCOPY, FAIL, "can't cache dataspace info")

    /* Get a datatype ID for the dataset's datatype */
    if((dataset->shared->type_id = H5I_register(H5I_DATATYPE, dataset->shared->type, FALSE)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTREGISTER, FAIL, "unable to register type")

    /* Get dataset creation property list object */
    if(NULL == (plist = (H5P_genplist_t *)H5I_object(dataset->shared->dcpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "can't get dataset creation property list")

    /* Get the layout/pline/efl message information */
    if(H5D__layout_oh_read(dataset, dxpl_id, dapl_id, plist) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get layout/pline/efl info")

    /* Indicate that the layout information was initialized */
    layout_init = TRUE;

    /* Set up flush append property */
    if(H5D__append_flush_setup(dataset, dapl_id))
	HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to set up flush append property")

    /* Point at dataset's copy, to cache it for later */
    fill_prop = &dataset->shared->dcpl_cache.fill;

    /* Try to get the new fill value message from the object header */
    if((msg_exists = H5O_msg_exists(&(dataset->oloc), H5O_FILL_NEW_ID, dxpl_id)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't check if message exists")
    if(msg_exists) {
        if(NULL == H5O_msg_read(&(dataset->oloc), H5O_FILL_NEW_ID, fill_prop, dxpl_id))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't retrieve message")
    } /* end if */
    else {
	/* For backward compatibility, try to retrieve the old fill value message */
        if((msg_exists = H5O_msg_exists(&(dataset->oloc), H5O_FILL_ID, dxpl_id)) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't check if message exists")
        if(msg_exists) {
            if(NULL == H5O_msg_read(&(dataset->oloc), H5O_FILL_ID, fill_prop, dxpl_id))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't retrieve message")
        } /* end if */
        else {
            /* Set the space allocation time appropriately, based on the type of dataset storage */
            switch(dataset->shared->layout.type) {
                case H5D_COMPACT:
                    fill_prop->alloc_time = H5D_ALLOC_TIME_EARLY;
                    break;

                case H5D_CONTIGUOUS:
                    fill_prop->alloc_time = H5D_ALLOC_TIME_LATE;
                    break;

                case H5D_CHUNKED:
                    fill_prop->alloc_time = H5D_ALLOC_TIME_INCR;
                    break;

                case H5D_VIRTUAL:
                    fill_prop->alloc_time = H5D_ALLOC_TIME_INCR;
                    break;

                case H5D_LAYOUT_ERROR:
                case H5D_NLAYOUTS:
                default:
                    HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "not implemented yet")
            } /* end switch */ /*lint !e788 All appropriate cases are covered */
        } /* end else */

        /* If "old" fill value size is 0 (undefined), map it to -1 */
        if(fill_prop->size == 0)
            fill_prop->size = (ssize_t)-1;
    } /* end if */
    alloc_time_state = 0;
    if((dataset->shared->layout.type == H5D_COMPACT && fill_prop->alloc_time == H5D_ALLOC_TIME_EARLY)
            || (dataset->shared->layout.type == H5D_CONTIGUOUS && fill_prop->alloc_time == H5D_ALLOC_TIME_LATE)
            || (dataset->shared->layout.type == H5D_CHUNKED && fill_prop->alloc_time == H5D_ALLOC_TIME_INCR)
            || (dataset->shared->layout.type == H5D_VIRTUAL && fill_prop->alloc_time == H5D_ALLOC_TIME_INCR))
        alloc_time_state = 1;

    /* Set revised fill value properties, if they are different from the defaults */
    if(H5P_fill_value_cmp(&H5D_def_dset.dcpl_cache.fill, fill_prop, sizeof(H5O_fill_t))) {
        if(H5P_set(plist, H5D_CRT_FILL_VALUE_NAME, fill_prop) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set fill value")
        if(H5P_set(plist, H5D_CRT_ALLOC_TIME_STATE_NAME, &alloc_time_state) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set allocation time state")
    } /* end if */

    /*
     * Make sure all storage is properly initialized.
     * This is important only for parallel I/O where the space must
     * be fully allocated before I/O can happen.
     */
    if((H5F_INTENT(dataset->oloc.file) & H5F_ACC_RDWR)
            && !(*dataset->shared->layout.ops->is_space_alloc)(&dataset->shared->layout.storage)
            && H5F_HAS_FEATURE(dataset->oloc.file, H5FD_FEAT_ALLOCATE_EARLY)) {
        H5D_io_info_t io_info;

        io_info.dset = dataset;
        io_info.raw_dxpl_id = H5AC_rawdata_dxpl_id;
        io_info.md_dxpl_id = dxpl_id;

        if(H5D__alloc_storage(&io_info, H5D_ALLOC_OPEN, FALSE, NULL) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize file storage")
    } /* end if */

done:
    if(ret_value < 0) {
        if(H5F_addr_defined(dataset->oloc.addr) && H5O_close(&(dataset->oloc), NULL) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release object header")
        if(dataset->shared) {
	    if(layout_init)
                if(dataset->shared->layout.ops->dest && (dataset->shared->layout.ops->dest)(dataset, dxpl_id) < 0)
                    HDONE_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "unable to destroy layout info")
            if(dataset->shared->space && H5S_close(dataset->shared->space) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataspace")
            if(dataset->shared->type) {
                if(dataset->shared->type_id > 0) {
                    if(H5I_dec_ref(dataset->shared->type_id) < 0)
                        HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release datatype")
                } /* end if */
                else {
                    if(H5T_close(dataset->shared->type) < 0)
                        HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release datatype")
                } /* end else */
            } /* end if */
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI_TAG(ret_value, FAIL)
} /* end H5D__open_oid() */


/*-------------------------------------------------------------------------
 * Function:	H5D_close
 *
 * Purpose:	Insures that all data has been saved to the file, closes the
 *		dataset object header, and frees all resources used by the
 *		descriptor.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Thursday, December  4, 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_close(H5D_t *dataset)
{
    hbool_t free_failed = FALSE;    /* Set if freeing sub-components failed */
    hbool_t corked;                 /* Whether the dataset is corked or not */
    hbool_t file_closed = TRUE;     /* H5O_close also closed the file?      */
    herr_t ret_value = SUCCEED;     /* Return value                         */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(dataset && dataset->oloc.file && dataset->shared);
    HDassert(dataset->shared->fo_count > 0);

    /* Dump debugging info */
#ifdef H5D_CHUNK_DEBUG
    H5D__chunk_stats(dataset, FALSE);
#endif /* H5D_CHUNK_DEBUG */

    dataset->shared->fo_count--;
    if(dataset->shared->fo_count == 0) {

        /* Flush the dataset's information.  Continue to close even if it fails. */
        if(H5D__flush_real(dataset, H5AC_ind_read_dxpl_id) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to flush cached dataset info")

        /* Set a flag to indicate the dataset is closing, before we start freeing things */
        /* (Avoids problems with flushing datasets twice, when one is holding
         *      the file open and it iterates through dataset to flush them -QAK)
         */
        dataset->shared->closing = TRUE;

        /* Free cached information for each kind of dataset */
        switch(dataset->shared->layout.type) {
            case H5D_CONTIGUOUS:
                /* Free the data sieve buffer, if it's been allocated */
                if(dataset->shared->cache.contig.sieve_buf)
                    dataset->shared->cache.contig.sieve_buf = (unsigned char *)H5FL_BLK_FREE(sieve_buf,dataset->shared->cache.contig.sieve_buf);
                break;

            case H5D_CHUNKED:
                /* Check for skip list for iterating over chunks during I/O to close */
                if(dataset->shared->cache.chunk.sel_chunks) {
                    HDassert(H5SL_count(dataset->shared->cache.chunk.sel_chunks) == 0);
                    H5SL_close(dataset->shared->cache.chunk.sel_chunks);
                    dataset->shared->cache.chunk.sel_chunks = NULL;
                } /* end if */

                /* Check for cached single chunk dataspace */
                if(dataset->shared->cache.chunk.single_space) {
                    (void)H5S_close(dataset->shared->cache.chunk.single_space);
                    dataset->shared->cache.chunk.single_space = NULL;
                } /* end if */

                /* Check for cached single element chunk info */
                if(dataset->shared->cache.chunk.single_chunk_info) {
                    dataset->shared->cache.chunk.single_chunk_info = H5FL_FREE(H5D_chunk_info_t, dataset->shared->cache.chunk.single_chunk_info);
                    dataset->shared->cache.chunk.single_chunk_info = NULL;
                } /* end if */
                break;

            case H5D_COMPACT:
                /* Nothing special to do (info freed in the layout destroy) */
                break;

            case H5D_VIRTUAL:
            {
                size_t i, j;

                HDassert(dataset->shared->layout.storage.u.virt.list || (dataset->shared->layout.storage.u.virt.list_nused == 0));

                /* Close source datasets */
                for(i = 0; i < dataset->shared->layout.storage.u.virt.list_nused; i++) {
                    /* Close source dataset */
                    if(dataset->shared->layout.storage.u.virt.list[i].source_dset.dset) {
                        HDassert(dataset->shared->layout.storage.u.virt.list[i].source_dset.dset != dataset);
                        if(H5D_close(dataset->shared->layout.storage.u.virt.list[i].source_dset.dset) < 0)
                            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to close source dataset")
                        dataset->shared->layout.storage.u.virt.list[i].source_dset.dset = NULL;
                    } /* end if */

                    /* Close sub datasets */
                    for(j = 0; j < dataset->shared->layout.storage.u.virt.list[i].sub_dset_nused; j++)
                        if(dataset->shared->layout.storage.u.virt.list[i].sub_dset[j].dset) {
                            HDassert(dataset->shared->layout.storage.u.virt.list[i].sub_dset[j].dset != dataset);
                            if(H5D_close(dataset->shared->layout.storage.u.virt.list[i].sub_dset[j].dset) < 0)
                                HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to close source dataset")
                            dataset->shared->layout.storage.u.virt.list[i].sub_dset[j].dset = NULL;
                        } /* end if */
                } /* end for */
            } /* end block */
            break;

            case H5D_LAYOUT_ERROR:
            case H5D_NLAYOUTS:
            default:
                HDassert("not implemented yet" && 0);
#ifdef NDEBUG
                HGOTO_ERROR(H5E_IO, H5E_UNSUPPORTED, FAIL, "unsupported storage layout")
#endif /* NDEBUG */
        } /* end switch */ /*lint !e788 All appropriate cases are covered */

        /* Destroy any cached layout information for the dataset */
        if(dataset->shared->layout.ops->dest && (dataset->shared->layout.ops->dest)(dataset, H5AC_ind_read_dxpl_id) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "unable to destroy layout info")

        /* Free the external file prefix */
        dataset->shared->extfile_prefix = (char *)H5MM_xfree(dataset->shared->extfile_prefix);

        /* Release layout, fill-value, efl & pipeline messages */
        if(dataset->shared->dcpl_id != H5P_DATASET_CREATE_DEFAULT)
            free_failed |= (H5O_msg_reset(H5O_PLINE_ID, &dataset->shared->dcpl_cache.pline) < 0) ||
                    (H5O_msg_reset(H5O_LAYOUT_ID, &dataset->shared->layout) < 0) ||
                    (H5O_msg_reset(H5O_FILL_ID, &dataset->shared->dcpl_cache.fill) < 0) ||
                    (H5O_msg_reset(H5O_EFL_ID, &dataset->shared->dcpl_cache.efl) < 0);

        /* Uncork cache entries with object address tag */
        if(H5AC_cork(dataset->oloc.file, dataset->oloc.addr, H5AC__GET_CORKED, &corked) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to retrieve an object's cork status")
        if(corked)
	        if(H5AC_cork(dataset->oloc.file, dataset->oloc.addr, H5AC__UNCORK, NULL) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CANTUNCORK, FAIL, "unable to uncork an object")

        /*
         * Release datatype, dataspace and creation property list -- there isn't
         * much we can do if one of these fails, so we just continue.
         */
        free_failed |= (H5I_dec_ref(dataset->shared->type_id) < 0) ||
                          (H5S_close(dataset->shared->space) < 0) ||
                          (H5I_dec_ref(dataset->shared->dcpl_id) < 0);

        /* Remove the dataset from the list of opened objects in the file */
        if(H5FO_top_decr(dataset->oloc.file, dataset->oloc.addr) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "can't decrement count for object")
        if(H5FO_delete(dataset->oloc.file, H5AC_ind_read_dxpl_id, dataset->oloc.addr) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "can't remove dataset from list of open objects")

        /* Close the dataset object */
        /* (This closes the file, if this is the last object open) */
        if(H5O_close(&(dataset->oloc), &file_closed) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release object header")

        /* Evict dataset metadata if evicting on close */
        if(!file_closed && H5F_SHARED(dataset->oloc.file) && H5F_EVICT_ON_CLOSE(dataset->oloc.file)) {
            if(H5AC_flush_tagged_metadata(dataset->oloc.file, dataset->oloc.addr, H5AC_ind_read_dxpl_id) < 0) 
                HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "unable to flush tagged metadata")
            if(H5AC_evict_tagged_metadata(dataset->oloc.file, dataset->oloc.addr, FALSE, H5AC_ind_read_dxpl_id) < 0) 
                HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "unable to evict tagged metadata")
        } /* end if */

        /*
         * Free memory.  Before freeing the memory set the file pointer to NULL.
         * We always check for a null file pointer in other H5D functions to be
         * sure we're not accessing an already freed dataset (see the HDassert()
         * above).
         */
        dataset->oloc.file = NULL;
        dataset->shared = H5FL_FREE(H5D_shared_t, dataset->shared);

    } /* end if */
    else {
        /* Decrement the ref. count for this object in the top file */
        if(H5FO_top_decr(dataset->oloc.file, dataset->oloc.addr) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "can't decrement count for object")

        /* Check reference count for this object in the top file */
        if(H5FO_top_count(dataset->oloc.file, dataset->oloc.addr) == 0) {
            if(H5O_close(&(dataset->oloc), NULL) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to close")
        } /* end if */
        else
            /* Free object location (i.e. "unhold" the file if appropriate) */
            if(H5O_loc_free(&(dataset->oloc)) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "problem attempting to free location")
    } /* end else */

    /* Release the dataset's path info */
    if(H5G_name_free(&(dataset->path)) < 0)
        free_failed = TRUE;

    /* Free the dataset's memory structure */
    dataset = H5FL_FREE(H5D_t, dataset);

    /* Check if anything failed in the middle... */
    if(free_failed)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "couldn't free a component of the dataset, but the dataset was freed anyway.")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_close() */


/*-------------------------------------------------------------------------
 * Function:	H5D_mult_refresh_close
 *
 * Purpose:	Closing down the needed information when the dataset has
 *		multiple opens.  (From H5O_refresh_metadata_close())
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Vailin Choi
 *		12/24/15
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_mult_refresh_close(hid_t dset_id, hid_t dxpl_id)
{
    H5D_t       *dataset;             	/* Dataset to refresh */
    herr_t      ret_value = SUCCEED;    /* return value */

    FUNC_ENTER_NOAPI(FAIL)

    if(NULL == (dataset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    /* check args */
    HDassert(dataset && dataset->oloc.file && dataset->shared);
    HDassert(dataset->shared->fo_count > 0);

    if(dataset->shared->fo_count > 1) {
        /* Free cached information for each kind of dataset */
        switch(dataset->shared->layout.type) {
            case H5D_CONTIGUOUS:
                /* Free the data sieve buffer, if it's been allocated */
                if(dataset->shared->cache.contig.sieve_buf)
                    dataset->shared->cache.contig.sieve_buf = (unsigned char *)H5FL_BLK_FREE(sieve_buf,dataset->shared->cache.contig.sieve_buf);
                break;

            case H5D_CHUNKED:
                /* Check for skip list for iterating over chunks during I/O to close */
                if(dataset->shared->cache.chunk.sel_chunks) {
                    HDassert(H5SL_count(dataset->shared->cache.chunk.sel_chunks) == 0);
                    H5SL_close(dataset->shared->cache.chunk.sel_chunks);
                    dataset->shared->cache.chunk.sel_chunks = NULL;
                } /* end if */

                /* Check for cached single chunk dataspace */
                if(dataset->shared->cache.chunk.single_space) {
                    (void)H5S_close(dataset->shared->cache.chunk.single_space);
                    dataset->shared->cache.chunk.single_space = NULL;
                } /* end if */

                /* Check for cached single element chunk info */
                if(dataset->shared->cache.chunk.single_chunk_info) {
                    dataset->shared->cache.chunk.single_chunk_info = H5FL_FREE(H5D_chunk_info_t, dataset->shared->cache.chunk.single_chunk_info);
                    dataset->shared->cache.chunk.single_chunk_info = NULL;
                } /* end if */
                break;

            case H5D_COMPACT:
            case H5D_VIRTUAL:
                /* Nothing special to do (info freed in the layout destroy) */
                break;

            case H5D_LAYOUT_ERROR:
            case H5D_NLAYOUTS:
            default:
                HDassert("not implemented yet" && 0);
#ifdef NDEBUG
                HGOTO_ERROR(H5E_IO, H5E_UNSUPPORTED, FAIL, "unsupported storage layout")
#endif /* NDEBUG */
        } /* end switch */ /*lint !e788 All appropriate cases are covered */

        /* Destroy any cached layout information for the dataset */
        if(dataset->shared->layout.ops->dest && (dataset->shared->layout.ops->dest)(dataset, dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "unable to destroy layout info")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_mult_refresh_close() */


/*-------------------------------------------------------------------------
 * Function:	H5D_mult_refresh_reopen
 *
 * Purpose:	Re-initialize the needed info when the dataset has multiple
 *		opens. (From H5O_refresh_metadata_reopen())
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Vailin Choi
 *		12/24/15
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_mult_refresh_reopen(H5D_t *dataset, hid_t dxpl_id)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(dataset && dataset->oloc.file && dataset->shared);
    HDassert(dataset->shared->fo_count > 0);

    if(dataset->shared->fo_count > 1) {
	/* Release dataspace info */
	if(H5S_close(dataset->shared->space) < 0)
	    HGOTO_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "unable to release dataspace")

	/* Re-load dataspace info */
	if(NULL == (dataset->shared->space = H5S_read(&(dataset->oloc), dxpl_id)))
	    HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to load dataspace info from dataset header")

	/* Cache the dataset's dataspace info */
	if(H5D__cache_dataspace_info(dataset) < 0)
	    HGOTO_ERROR(H5E_DATASET, H5E_CANTCOPY, FAIL, "can't cache dataspace info")
    
	/* Release layout info */
	if(H5O_msg_reset(H5O_LAYOUT_ID, &dataset->shared->layout) < 0)
	    HGOTO_ERROR(H5E_DATASET, H5E_CANTRESET, FAIL, "unable to reset layout info")

	/* Re-load layout message info */
	if(NULL == H5O_msg_read(&(dataset->oloc), H5O_LAYOUT_ID, &(dataset->shared->layout), dxpl_id))
	    HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to read data layout message")	
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D_mult_refresh_reopen() */


/*-------------------------------------------------------------------------
 * Function:	H5D_oloc
 *
 * Purpose:	Returns a pointer to the object location for a dataset.
 *
 * Return:	Success:	Ptr to location
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Friday, April 24, 1998
 *
 *-------------------------------------------------------------------------
 */
H5O_loc_t *
H5D_oloc(H5D_t *dataset)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    FUNC_LEAVE_NOAPI(dataset ? &(dataset->oloc) : (H5O_loc_t *)NULL)
} /* end H5D_oloc() */


/*-------------------------------------------------------------------------
 * Function:	H5D_nameof
 *
 * Purpose:	Returns a pointer to the group hier. path for a dataset.
 *
 * Return:	Success:	Ptr to entry
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Monday, September 12, 2005
 *
 *-------------------------------------------------------------------------
 */
H5G_name_t *
H5D_nameof(H5D_t *dataset)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    FUNC_LEAVE_NOAPI(dataset ? &(dataset->path) : (H5G_name_t *)NULL)
} /* end H5D_nameof() */


/*-------------------------------------------------------------------------
 * Function:	H5D_typeof
 *
 * Purpose:	Returns a pointer to the dataset's datatype.  The datatype
 *		is not copied.
 *
 * Return:	Success:	Ptr to the dataset's datatype, uncopied.
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Thursday, June  4, 1998
 *
 *-------------------------------------------------------------------------
 */
H5T_t *
H5D_typeof(const H5D_t *dset)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(dset);
    HDassert(dset->shared);
    HDassert(dset->shared->type);

    FUNC_LEAVE_NOAPI(dset->shared->type)
} /* end H5D_typeof() */


/*-------------------------------------------------------------------------
 * Function:	H5D__alloc_storage
 *
 * Purpose:	Allocate storage for the raw data of a dataset.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Friday, January 16, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__alloc_storage(const H5D_io_info_t *io_info, H5D_time_alloc_t time_alloc,
    hbool_t full_overwrite, hsize_t old_dim[])
{
    const H5D_t *dset = io_info->dset;   /* The dataset object */
    H5F_t *f = dset->oloc.file;         /* The dataset's file pointer */
    H5O_layout_t *layout;               /* The dataset's layout information */
    hbool_t must_init_space = FALSE;    /* Flag to indicate that space should be initialized */
    hbool_t addr_set = FALSE;           /* Flag to indicate that the dataset's storage address was set */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* check args */
    HDassert(dset);
    HDassert(f);

    /* If the data is stored in external files, don't set an address for the layout
     * We assume that external storage is already
     * allocated by the caller, or at least will be before I/O is performed.
     */
    if(!(H5S_NULL == H5S_GET_EXTENT_TYPE(dset->shared->space) || dset->shared->dcpl_cache.efl.nused > 0)) {
        /* Get a pointer to the dataset's layout information */
        layout = &(dset->shared->layout);

        switch(layout->type) {
            case H5D_CONTIGUOUS:
                if(!(*dset->shared->layout.ops->is_space_alloc)(&dset->shared->layout.storage)) {
                    /* Check if we have a zero-sized dataset */
                    if(layout->storage.u.contig.size > 0) {
                        /* Reserve space in the file for the entire array */
                        if(H5D__contig_alloc(f, io_info->md_dxpl_id, &layout->storage.u.contig/*out*/) < 0)
                            HGOTO_ERROR(H5E_IO, H5E_CANTINIT, FAIL, "unable to initialize contiguous storage")

                        /* Indicate that we should initialize storage space */
                        must_init_space = TRUE;
                    } /* end if */
                    else
                        layout->storage.u.contig.addr = HADDR_UNDEF;

                    /* Indicate that we set the storage addr */
                    addr_set = TRUE;
                } /* end if */
                break;

            case H5D_CHUNKED:
                if(!(*dset->shared->layout.ops->is_space_alloc)(&dset->shared->layout.storage)) {
                    /* Create the root of the index that manages chunked storage */
                    if(H5D__chunk_create(dset /*in,out*/, io_info->md_dxpl_id) < 0)
                        HGOTO_ERROR(H5E_IO, H5E_CANTINIT, FAIL, "unable to initialize chunked storage")

                    /* Indicate that we set the storage addr */
                    addr_set = TRUE;

                    /* Indicate that we should initialize storage space */
                    must_init_space = TRUE;
                } /* end if */

                /* If space allocation is set to 'early' and we are extending
		 * the dataset, indicate that space should be allocated, so the
                 * index gets expanded. -QAK
                 */
		if(dset->shared->dcpl_cache.fill.alloc_time == H5D_ALLOC_TIME_EARLY
                        && time_alloc == H5D_ALLOC_EXTEND)
		    must_init_space = TRUE;
                break;

            case H5D_COMPACT:
                /* Check if space is already allocated */
                if(NULL == layout->storage.u.compact.buf) {
                    /* Reserve space in layout header message for the entire array. 
                     * Starting from the 1.8.7 release, we allow dataspace to have 
                     * zero dimension size.  So the storage size can be zero.
                     * SLU 2011/4/4 */
                    if(layout->storage.u.compact.size > 0) {
                        if(NULL == (layout->storage.u.compact.buf = H5MM_malloc(layout->storage.u.compact.size)))
                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate memory for compact dataset")
                        if(!full_overwrite)
                            HDmemset(layout->storage.u.compact.buf, 0, layout->storage.u.compact.size);
                        layout->storage.u.compact.dirty = TRUE;

                        /* Indicate that we should initialize storage space */
                        must_init_space = TRUE;
                    } else {
                        layout->storage.u.compact.dirty = FALSE;
                        must_init_space = FALSE;
                    }
                } /* end if */
                break;

            case H5D_VIRTUAL:
                /* No-op, as the raw data is stored elsewhere and the global
                 * heap object containing the mapping information is created
                 * when the layout message is encoded.  We may wish to move the
                 * creation of the global heap object here at some point, but we
                 * will have to make sure is it always created before the
                 * dataset is closed. */
                break;

            case H5D_LAYOUT_ERROR:
            case H5D_NLAYOUTS:
            default:
                HDassert("not implemented yet" && 0);
#ifdef NDEBUG
                HGOTO_ERROR(H5E_IO, H5E_UNSUPPORTED, FAIL, "unsupported storage layout")
#endif /* NDEBUG */
        } /* end switch */ /*lint !e788 All appropriate cases are covered */

        /* Check if we need to initialize the space */
        if(must_init_space) {
            if(layout->type == H5D_CHUNKED) {
                /* If we are doing incremental allocation and the index got
                 * created during a H5Dwrite call, don't initialize the storage
                 * now, wait for the actual writes to each block and let the
                 * low-level chunking routines handle initialize the fill-values.
                 * Otherwise, pass along the space initialization call and let
                 * the low-level chunking routines sort out whether to write
                 * fill values to the chunks they allocate space for.  Yes,
                 * this is icky. -QAK
                 */
                if(!(dset->shared->dcpl_cache.fill.alloc_time == H5D_ALLOC_TIME_INCR && time_alloc == H5D_ALLOC_WRITE))
                    if(H5D__init_storage(io_info, full_overwrite, old_dim) < 0)
                        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize dataset with fill value")
            } /* end if */
            else {
                H5D_fill_value_t	fill_status;    /* The fill value status */

                /* Check the dataset's fill-value status */
                if(H5P_is_fill_value_defined(&dset->shared->dcpl_cache.fill, &fill_status) < 0)
                    HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't tell if fill value defined")

                /* If we are filling the dataset on allocation or "if set" and
                 * the fill value _is_ set, do that now */
                if(dset->shared->dcpl_cache.fill.fill_time == H5D_FILL_TIME_ALLOC ||
                        (dset->shared->dcpl_cache.fill.fill_time == H5D_FILL_TIME_IFSET && fill_status == H5D_FILL_VALUE_USER_DEFINED))
                    if(H5D__init_storage(io_info, full_overwrite, old_dim) < 0)
                        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize dataset with fill value")
            } /* end else */
        } /* end if */

        /* If we set the address (and aren't in the middle of creating the
         *      dataset), mark the layout header message for later writing to
         *      the file.  (this improves forward compatibility).
         */
        /* (The layout message is already in the dataset's object header, this
         *      operation just sets the address and makes it constant)
         */
        if(time_alloc != H5D_ALLOC_CREATE && addr_set)
            /* Mark the layout as dirty, for later writing to the file */
            if(H5D__mark(dset, io_info->md_dxpl_id, H5D_MARK_LAYOUT) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to mark dataspace as dirty")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__alloc_storage() */


/*-------------------------------------------------------------------------
 * Function:	H5D__init_storage
 *
 * Purpose:	Initialize the data for a new dataset.  If a selection is
 *		defined for SPACE then initialize only that part of the
 *		dataset.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, October  5, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__init_storage(const H5D_io_info_t *io_info, hbool_t full_overwrite, hsize_t old_dim[])
{
    const H5D_t *dset = io_info->dset;     /* dataset pointer */
    herr_t ret_value = SUCCEED;           /* Return value */

    FUNC_ENTER_STATIC

    HDassert(dset);

    switch (dset->shared->layout.type) {
        case H5D_COMPACT:
            /* If we will be immediately overwriting the values, don't bother to clear them */
            if(!full_overwrite) {
                /* Fill the compact dataset storage */
                if(H5D__compact_fill(dset, io_info->md_dxpl_id) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize compact dataset storage")
            } /* end if */
            break;

        case H5D_CONTIGUOUS:
            /* Don't write default fill values to external files */
            /* If we will be immediately overwriting the values, don't bother to clear them */
            if((dset->shared->dcpl_cache.efl.nused == 0 || dset->shared->dcpl_cache.fill.buf) && !full_overwrite)
                if(H5D__contig_fill(io_info) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to allocate all chunks of dataset")
            break;

        case H5D_CHUNKED:
            /*
             * Allocate file space
             * for all chunks now and initialize each chunk with the fill value.
             */
            {
                hsize_t             zero_dim[H5O_LAYOUT_NDIMS] = {0};

                /* Use zeros for old dimensions if not specified */
                if(old_dim == NULL)
                    old_dim = zero_dim;

                if(H5D__chunk_allocate(io_info, full_overwrite, old_dim) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to allocate all chunks of dataset")
                break;
            } /* end block */

        case H5D_VIRTUAL:
            /* No-op, as the raw data is stored elsewhere */

        case H5D_LAYOUT_ERROR:
        case H5D_NLAYOUTS:
        default:
            HDassert("not implemented yet" && 0);
#ifdef NDEBUG
            HGOTO_ERROR(H5E_IO, H5E_UNSUPPORTED, FAIL, "unsupported storage layout")
#endif /* NDEBUG */
    } /* end switch */ /*lint !e788 All appropriate cases are covered */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__init_storage() */


/*-------------------------------------------------------------------------
 * Function:	H5D__get_storage_size
 *
 * Purpose:	Determines how much space has been reserved to store the raw
 *		data of a dataset.
 *
 * Return:	Non-negative on success, negative on failure
 *
 * Programmer:	Robb Matzke
 *              Wednesday, April 21, 1999
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__get_storage_size(H5D_t *dset, hid_t dxpl_id, hsize_t *storage_size)
{
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE_TAG(dxpl_id, dset->oloc.addr, FAIL)

    switch(dset->shared->layout.type) {
        case H5D_CHUNKED:
            if((*dset->shared->layout.ops->is_space_alloc)(&dset->shared->layout.storage)) {
                if(H5D__chunk_allocated(dset, dxpl_id, storage_size) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't retrieve chunked dataset allocated size")
            } /* end if */
            else
                *storage_size = 0;
            break;

        case H5D_CONTIGUOUS:
            /* Datasets which are not allocated yet are using no space on disk */
            if((*dset->shared->layout.ops->is_space_alloc)(&dset->shared->layout.storage))
                *storage_size = dset->shared->layout.storage.u.contig.size;
            else
                *storage_size = 0;
            break;

        case H5D_COMPACT:
            *storage_size = dset->shared->layout.storage.u.compact.size;
            break;

        case H5D_VIRTUAL:
            /* Just set to 0, as virtual datasets do not actually store raw data
             */
            *storage_size = 0;
            break;

        case H5D_LAYOUT_ERROR:
        case H5D_NLAYOUTS:
        default:
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset type")
    } /*lint !e788 All appropriate cases are covered */

done:
    FUNC_LEAVE_NOAPI_TAG(ret_value, 0)
} /* end H5D__get_storage_size() */


/*-------------------------------------------------------------------------
 * Function:	H5D__get_offset
 *
 * Purpose:	Private function for H5D__get_offset.  Returns the address
 *              of dataset in file.
 *
 * Return:	Success:        the address of dataset
 *
 *		Failure:	HADDR_UNDEF
 *
 * Programmer:  Raymond Lu
 *              November 6, 2002
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5D__get_offset(const H5D_t *dset)
{
    haddr_t	ret_value = HADDR_UNDEF;

    FUNC_ENTER_PACKAGE

    HDassert(dset);

    switch(dset->shared->layout.type) {
        case H5D_VIRTUAL:
        case H5D_CHUNKED:
        case H5D_COMPACT:
            break;

        case H5D_CONTIGUOUS:
            /* If dataspace hasn't been allocated or dataset is stored in
             * an external file, the value will be HADDR_UNDEF. */
            if(dset->shared->dcpl_cache.efl.nused == 0 || H5F_addr_defined(dset->shared->layout.storage.u.contig.addr))
                /* Return the absolute dataset offset from the beginning of file. */
                ret_value = dset->shared->layout.storage.u.contig.addr + H5F_BASE_ADDR(dset->oloc.file);
            break;

        case H5D_LAYOUT_ERROR:
        case H5D_NLAYOUTS:
        default:
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, HADDR_UNDEF, "unknown dataset layout type")
    } /*lint !e788 All appropriate cases are covered */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__get_offset() */


/*-------------------------------------------------------------------------
 * Function:	H5D_vlen_reclaim
 *
 * Purpose:	Frees the buffers allocated for storing variable-length data
 *      in memory.  Only frees the VL data in the selection defined in the
 *      dataspace.  The dataset transfer property list is required to find the
 *      correct allocation/free methods for the VL data in the buffer.
 *
 * Return:	Non-negative on success, negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, November 22, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_vlen_reclaim(hid_t type_id, H5S_t *space, hid_t plist_id, void *buf)
{
    H5T_t *type;                /* Datatype */
    H5S_sel_iter_op_t dset_op;  /* Operator for iteration */
    H5T_vlen_alloc_info_t _vl_alloc_info;       /* VL allocation info buffer */
    H5T_vlen_alloc_info_t *vl_alloc_info = &_vl_alloc_info;   /* VL allocation info */
    herr_t ret_value = FAIL;            /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(H5I_DATATYPE == H5I_get_type(type_id));
    HDassert(space);
    HDassert(H5P_isa_class(plist_id, H5P_DATASET_XFER));
    HDassert(buf);

    if(NULL == (type = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an valid base datatype")

    /* Get the allocation info */
    if(H5T_vlen_get_alloc_info(plist_id,&vl_alloc_info) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "unable to retrieve VL allocation info")

    /* Call H5S_select_iterate with args, etc. */
    dset_op.op_type = H5S_SEL_ITER_OP_APP;
    dset_op.u.app_op.op = H5T_vlen_reclaim;
    dset_op.u.app_op.type_id = type_id;

    ret_value = H5S_select_iterate(buf, type, space, &dset_op, vl_alloc_info);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5D_vlen_reclaim() */


/*-------------------------------------------------------------------------
 * Function:	H5D__vlen_get_buf_size_alloc
 *
 * Purpose:	This routine makes certain there is enough space in the temporary
 *      buffer for the new data to read in.  All the VL data read in is actually
 *      placed in this buffer, overwriting the previous data.  Needless to say,
 *      this data is not actually usable.
 *
 * Return:	Non-negative on success, negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, August 17, 1999
 *
 *-------------------------------------------------------------------------
 */
void *
H5D__vlen_get_buf_size_alloc(size_t size, void *info)
{
    H5D_vlen_bufsize_t *vlen_bufsize = (H5D_vlen_bufsize_t *)info;
    void *ret_value = NULL;     /* Return value */

    FUNC_ENTER_PACKAGE_NOERR

    /* Get a temporary pointer to space for the VL data */
    if((vlen_bufsize->vl_tbuf = H5FL_BLK_REALLOC(vlen_vl_buf, vlen_bufsize->vl_tbuf, size)) != NULL)
        vlen_bufsize->size += size;

    /* Set return value */
    ret_value = vlen_bufsize->vl_tbuf;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__vlen_get_buf_size_alloc() */


/*-------------------------------------------------------------------------
 * Function:	H5D__vlen_get_buf_size
 *
 * Purpose:	This routine checks the number of bytes required to store a single
 *      element from a dataset in memory, creating a selection with just the
 *      single element selected to read in the element and using a custom memory
 *      allocator for any VL data encountered.
 *          The *size value is modified according to how many bytes are
 *      required to store the element in memory.
 *
 * Implementation: This routine actually performs the read with a custom
 *      memory manager which basically just counts the bytes requested and
 *      uses a temporary memory buffer (through the H5FL API) to make certain
 *      enough space is available to perform the read.  Then the temporary
 *      buffer is released and the number of bytes allocated is returned.
 *      Kinda kludgy, but easier than the other method of trying to figure out
 *      the sizes without actually reading the data in... - QAK
 *
 * Return:	Non-negative on success, negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, August 17, 1999
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__vlen_get_buf_size(void H5_ATTR_UNUSED *elem, hid_t type_id, unsigned H5_ATTR_UNUSED ndim, const hsize_t *point, void *op_data)
{
    H5D_vlen_bufsize_t *vlen_bufsize = (H5D_vlen_bufsize_t *)op_data;
    H5T_t *dt;                          /* Datatype for operation */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_PACKAGE

    HDassert(op_data);
    HDassert(H5I_DATATYPE == H5I_get_type(type_id));

    /* Check args */
    if(NULL == (dt = (H5T_t *)H5I_object(type_id)))
        HGOTO_ERROR(H5E_DATASET, H5E_BADTYPE, FAIL, "not a datatype")

    /* Make certain there is enough fixed-length buffer available */
    if(NULL == (vlen_bufsize->fl_tbuf = H5FL_BLK_REALLOC(vlen_fl_buf, vlen_bufsize->fl_tbuf, H5T_get_size(dt))))
        HGOTO_ERROR(H5E_DATASET, H5E_NOSPACE, FAIL, "can't resize tbuf")

    /* Select point to read in */
    if(H5S_select_elements(vlen_bufsize->fspace, H5S_SELECT_SET, (size_t)1, point) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTCREATE, FAIL, "can't select point")

    /* Read in the point (with the custom VL memory allocator) */
    if(H5D__read(vlen_bufsize->dset, type_id, vlen_bufsize->mspace, vlen_bufsize->fspace, vlen_bufsize->xfer_pid, vlen_bufsize->fl_tbuf) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "can't read point")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__vlen_get_buf_size() */


/*-------------------------------------------------------------------------
 * Function:	H5D__check_filters
 *
 * Purpose:	Check if the filters have be initialized for the dataset
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Thursday, October 11, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__check_filters(H5D_t *dataset)
{
    H5O_fill_t *fill;                   /* Dataset's fill value */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(dataset);

    /* Check if the filters in the DCPL will need to encode, and if so, can they?
     *
     * Filters need encoding if fill value is defined and a fill policy is set
     * that requires writing on an extend.
     */
    fill = &dataset->shared->dcpl_cache.fill;
    if(!dataset->shared->checked_filters) {
        H5D_fill_value_t fill_status;       /* Whether the fill value is defined */

        /* Retrieve the "defined" status of the fill value */
        if(H5P_is_fill_value_defined(fill, &fill_status) < 0)
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "Couldn't retrieve fill value from dataset.")

        /* See if we can check the filter status */
        if(fill_status == H5D_FILL_VALUE_DEFAULT || fill_status == H5D_FILL_VALUE_USER_DEFINED) {
            if(fill->fill_time == H5D_FILL_TIME_ALLOC ||
                    (fill->fill_time == H5D_FILL_TIME_IFSET && fill_status == H5D_FILL_VALUE_USER_DEFINED)) {
                /* Filters must have encoding enabled. Ensure that all filters can be applied */
                if(H5Z_can_apply(dataset->shared->dcpl_id, dataset->shared->type_id) < 0)
                    HGOTO_ERROR(H5E_PLINE, H5E_CANAPPLY, FAIL, "can't apply filters")

                dataset->shared->checked_filters = TRUE;
            } /* end if */
        } /* end if */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__check_filters() */


/*-------------------------------------------------------------------------
 * Function:	H5D__set_extent
 *
 * Purpose:	Based on H5D_extend, allows change to a lower dimension,
 *		calls H5S_set_extent and H5D__chunk_prune_by_extent instead
 *
 * Return:	Non-negative on success, negative on failure
 *
 * Programmer:  Pedro Vicente, pvn@ncsa.uiuc.edu
 *              April 9, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__set_extent(H5D_t *dset, const hsize_t *size, hid_t dxpl_id)
{
    hsize_t curr_dims[H5S_MAX_RANK];    /* Current dimension sizes */
    htri_t  changed;                    /* Whether the dataspace changed size */
    size_t  u, v;                       /* Local index variable */
    herr_t  ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_PACKAGE_TAG(dxpl_id, dset->oloc.addr, FAIL)

    /* Check args */
    HDassert(dset);
    HDassert(size);

    /* Check if we are allowed to modify this file */
    if(0 == (H5F_INTENT(dset->oloc.file) & H5F_ACC_RDWR))
        HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "no write intent on file")

    /* Check if we are allowed to modify the space; only datasets with chunked and external storage are allowed to be modified */
    if(H5D_COMPACT == dset->shared->layout.type)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "dataset has compact storage")
    if(H5D_CONTIGUOUS == dset->shared->layout.type && 0 == dset->shared->dcpl_cache.efl.nused)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "dataset has contiguous storage")

    /* Check if the filters in the DCPL will need to encode, and if so, can they? */
    if(H5D__check_filters(dset) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't apply filters")

    /* Keep the current dataspace dimensions for later */
    HDcompile_assert(sizeof(curr_dims) == sizeof(dset->shared->curr_dims));
    HDmemcpy(curr_dims, dset->shared->curr_dims, H5S_MAX_RANK * sizeof(curr_dims[0]));

    /* Modify the size of the dataspace */
    if((changed = H5S_set_extent(dset->shared->space, size)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to modify size of dataspace")

    /* Don't bother updating things, unless they've changed */
    if(changed) {
        hbool_t shrink = FALSE;         /* Flag to indicate a dimension has shrank */
        hbool_t expand = FALSE;         /* Flag to indicate a dimension has grown */
        hbool_t update_chunks = FALSE;  /* Flag to indicate chunk cache update is needed */

        /* Determine if we are shrinking and/or expanding any dimensions */
        for(u = 0; u < (size_t)dset->shared->ndims; u++) {
            /* Check for various status changes */
            if(size[u] < curr_dims[u])
                shrink = TRUE;
            if(size[u] > curr_dims[u])
                expand = TRUE;

            /* Chunked storage specific checks */
            if(H5D_CHUNKED == dset->shared->layout.type && dset->shared->ndims > 1) {
                hsize_t scaled;             /* Scaled value */

                /* Compute the scaled dimension size value */
                scaled = size[u] / dset->shared->layout.u.chunk.dim[u];

                /* Check if scaled dimension size changed */
                if(scaled != dset->shared->cache.chunk.scaled_dims[u]) {
                    hsize_t scaled_power2up;    /* Scaled value, rounded to next power of 2 */

                    /* Update the scaled dimension size value for the current dimension */
                    dset->shared->cache.chunk.scaled_dims[u] = scaled;

                    /* Check if algorithm for computing hash values will change */
                    if((scaled > dset->shared->cache.chunk.nslots &&
                                dset->shared->cache.chunk.scaled_dims[u] <= dset->shared->cache.chunk.nslots)
                            || (scaled <= dset->shared->cache.chunk.nslots &&
                                dset->shared->cache.chunk.scaled_dims[u] > dset->shared->cache.chunk.nslots))
                        update_chunks = TRUE;

                    /* Check if the number of bits required to encode the scaled size value changed */
                    if(dset->shared->cache.chunk.scaled_power2up[u] != (scaled_power2up = H5VM_power2up(scaled))) {
                        /* Update the 'power2up' & 'encode_bits' values for the current dimension */
                        dset->shared->cache.chunk.scaled_power2up[u] = scaled_power2up;
                        dset->shared->cache.chunk.scaled_encode_bits[u] = H5VM_log2_gen(scaled_power2up);

                        /* Indicate that the cached chunk indices need to be updated */
                        update_chunks = TRUE;
                    } /* end if */
                } /* end if */
            } /* end if */

            /* Update the cached copy of the dataset's dimensions */
            dset->shared->curr_dims[u] = size[u];
        } /* end for */

        /*-------------------------------------------------------------------------
         * Modify the dataset storage
         *-------------------------------------------------------------------------
         */
        /* Update the index values for the cached chunks for this dataset */
        if(H5D_CHUNKED == dset->shared->layout.type) {
            /* Set the cached chunk info */
            if(H5D__chunk_set_info(dset) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to update # of chunks")

            /* Check if updating the chunk cache indices is necessary */
            if(update_chunks)
                /* Update the chunk cache indices */
                if(H5D__chunk_update_cache(dset, dxpl_id) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to update cached chunk indices")
        } /* end if */

        /* Operations for virtual datasets */
        if(H5D_VIRTUAL == dset->shared->layout.type) {
            /* Check that the dimensions of the VDS are large enough */
            if(H5D_virtual_check_min_dims(dset) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "virtual dataset dimensions not large enough to contain all limited dimensions in all selections")

            /* Patch the virtual selection dataspaces */
            for(u = 0; u < dset->shared->layout.storage.u.virt.list_nused; u++) {
                /* Patch extent */
                if(H5S_set_extent(dset->shared->layout.storage.u.virt.list[u].source_dset.virtual_select, size) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to modify size of data space")
                dset->shared->layout.storage.u.virt.list[u].virtual_space_status = H5O_VIRTUAL_STATUS_CORRECT;

                /* Patch sub-source datasets */
                for(v = 0; v < dset->shared->layout.storage.u.virt.list[u].sub_dset_nalloc; v++)
                    if(H5S_set_extent(dset->shared->layout.storage.u.virt.list[u].sub_dset[v].virtual_select, size) < 0)
                        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to modify size of data space")
            } /* end for */

            /* Mark virtual datasets as not fully initialized so internal
             * selections are recalculated (at next I/O operation) */
            dset->shared->layout.storage.u.virt.init = FALSE;
        } /* end if */

        /* Allocate space for the new parts of the dataset, if appropriate */
        if(expand && dset->shared->dcpl_cache.fill.alloc_time == H5D_ALLOC_TIME_EARLY) {
            H5D_io_info_t io_info;

            io_info.dset = dset;
            io_info.raw_dxpl_id = H5AC_rawdata_dxpl_id;
            io_info.md_dxpl_id = dxpl_id;

            if(H5D__alloc_storage(&io_info, H5D_ALLOC_EXTEND, FALSE, curr_dims) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to extend dataset storage")
        }
        /*-------------------------------------------------------------------------
         * Remove chunk information in the case of chunked datasets
         * This removal takes place only in case we are shrinking the dateset
         * and if the chunks are written
         *-------------------------------------------------------------------------
         */
        if(H5D_CHUNKED == dset->shared->layout.type) {
            if(shrink && (*dset->shared->layout.ops->is_space_alloc)(&dset->shared->layout.storage))
                /* Remove excess chunks */
                if(H5D__chunk_prune_by_extent(dset, dxpl_id, curr_dims) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to remove chunks")

            /* Update chunks that are no longer edge chunks as a result of
             * expansion */
            if(expand && (dset->shared->layout.u.chunk.flags & H5O_LAYOUT_CHUNK_DONT_FILTER_PARTIAL_BOUND_CHUNKS)
                    && (dset->shared->dcpl_cache.pline.nused > 0))
                if(H5D__chunk_update_old_edge_chunks(dset, dxpl_id, curr_dims) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to do update old edge chunks")
	} /* end if */

        /* Mark the dataspace as dirty, for later writing to the file */
        if(H5D__mark(dset, dxpl_id, H5D_MARK_SPACE) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to mark dataspace as dirty")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI_TAG(ret_value, FAIL)
} /* end H5D__set_extent() */


/*-------------------------------------------------------------------------
 * Function:    H5D__flush_sieve_buf
 *
 * Purpose:     Flush any dataset sieve buffer info cached in memory
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:  Quincey Koziol
 *              July 27, 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__flush_sieve_buf(H5D_t *dataset, hid_t dxpl_id)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(dataset);

    /* Flush the raw data buffer, if we have a dirty one */
    if(dataset->shared->cache.contig.sieve_buf && dataset->shared->cache.contig.sieve_dirty) {
        HDassert(dataset->shared->layout.type != H5D_COMPACT);      /* We should never have a sieve buffer for compact storage */

        /* Write dirty data sieve buffer to file */
        if(H5F_block_write(dataset->oloc.file, H5FD_MEM_DRAW, dataset->shared->cache.contig.sieve_loc,
                dataset->shared->cache.contig.sieve_size, dxpl_id, dataset->shared->cache.contig.sieve_buf) < 0)
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "block write failed")

        /* Reset sieve buffer dirty flag */
        dataset->shared->cache.contig.sieve_dirty = FALSE;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__flush_sieve_buf() */


/*-------------------------------------------------------------------------
 * Function:    H5D__flush_real
 *
 * Purpose:     Flush any dataset information cached in memory
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:  Quincey Koziol
 *              December 6, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__flush_real(H5D_t *dataset, hid_t dxpl_id)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_PACKAGE_TAG(dxpl_id, dataset->oloc.addr, FAIL)

    /* Check args */
    HDassert(dataset);
    HDassert(dataset->shared);

    /* Avoid flushing the dataset (again) if it's closing */
    if(!dataset->shared->closing) {
        /* Flush cached raw data for each kind of dataset layout */
        if(dataset->shared->layout.ops->flush &&
                (dataset->shared->layout.ops->flush)(dataset, dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTFLUSH, FAIL, "unable to flush raw data")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI_TAG(ret_value, FAIL)
} /* end H5D__flush_real() */


/*-------------------------------------------------------------------------
 * Function:    H5D__format_convert
 *
 * Purpose:     For chunked: downgrade the chunk indexing type to version 1 B-tree
 *              For compact/contiguous: downgrade layout version to 3
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:  Vailin Choi
 *              Feb 2015
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__format_convert(H5D_t *dataset, hid_t dxpl_id)
{
    H5D_chk_idx_info_t new_idx_info;    	/* Index info for the new layout */
    H5D_chk_idx_info_t idx_info;        	/* Index info for the current layout */
    H5O_layout_t newlayout;			/* The new layout */
    hbool_t init_new_index = FALSE;		/* Indicate that the new chunk index is initialized */
    hbool_t delete_old_layout = FALSE;		/* Indicate that the old layout message is deleted */
    hbool_t add_new_layout = FALSE;		/* Indicate that the new layout message is added */
    herr_t ret_value = SUCCEED;         	/* Return value */

    FUNC_ENTER_PACKAGE_TAG(dxpl_id, dataset->oloc.addr, FAIL)

    /* Check args */
    HDassert(dataset);

    switch(dataset->shared->layout.type) {
        case H5D_CHUNKED:
	    HDassert(dataset->shared->layout.u.chunk.idx_type != H5D_CHUNK_IDX_BTREE);

	    /* Set up the current index info */
	    idx_info.f = dataset->oloc.file;
	    idx_info.dxpl_id = dxpl_id;
	    idx_info.pline = &dataset->shared->dcpl_cache.pline;
	    idx_info.layout = &dataset->shared->layout.u.chunk;
	    idx_info.storage = &dataset->shared->layout.storage.u.chunk;

	    /* Copy the current layout info to the new layout */
	    HDmemcpy(&newlayout, &dataset->shared->layout, sizeof(H5O_layout_t));

	    /* Set up info for version 1 B-tree in the new layout */
	    newlayout.version = H5O_LAYOUT_VERSION_3;
	    newlayout.storage.u.chunk.idx_type = H5D_CHUNK_IDX_BTREE;
	    newlayout.storage.u.chunk.idx_addr = HADDR_UNDEF;
	    newlayout.storage.u.chunk.ops = H5D_COPS_BTREE;
	    newlayout.storage.u.chunk.u.btree.shared = NULL;

	    /* Set up the index info to version 1 B-tree */
	    new_idx_info.f = dataset->oloc.file;
	    new_idx_info.dxpl_id = dxpl_id;
	    new_idx_info.pline = &dataset->shared->dcpl_cache.pline;
	    new_idx_info.layout = &newlayout.u.chunk;
	    new_idx_info.storage = &newlayout.storage.u.chunk;

	    /* Initialize version 1 B-tree */
	    if(new_idx_info.storage->ops->init && 
              (new_idx_info.storage->ops->init)(&new_idx_info, dataset->shared->space, dataset->oloc.addr) < 0)
		HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't initialize indexing information")
	    init_new_index = TRUE;

	    /* If the current chunk index exists */
	    if(H5F_addr_defined(idx_info.storage->idx_addr)) {

		/* Create v1 B-tree chunk index */
		if((new_idx_info.storage->ops->create)(&new_idx_info) < 0)
		    HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't create chunk index")

		/* Iterate over the chunks in the current index and insert the chunk addresses 
		 * into the version 1 B-tree chunk index */
		if(H5D__chunk_format_convert(dataset, &idx_info, &new_idx_info) < 0)
		    HGOTO_ERROR(H5E_DATASET, H5E_BADITER, FAIL, "unable to iterate/convert chunk index")
	    } /* end if */

	    /* Delete the old "current" layout message */
	    if(H5O_msg_remove(&dataset->oloc, H5O_LAYOUT_ID, H5O_ALL, FALSE, dxpl_id) < 0)
		HGOTO_ERROR(H5E_SYM, H5E_CANTDELETE, FAIL, "unable to delete layout message")

	    delete_old_layout = TRUE;

	    /* Append the new layout message to the object header */
	    if(H5O_msg_create(&dataset->oloc, H5O_LAYOUT_ID, 0, H5O_UPDATE_TIME, &newlayout, dxpl_id) < 0)
		HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update layout header message")

	    add_new_layout = TRUE;

	    /* Release the old (current) chunk index */
	    if(idx_info.storage->ops->dest && (idx_info.storage->ops->dest)(&idx_info) < 0)
		HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "unable to release chunk index info")

	    /* Copy the new layout to the dataset's layout */
	    HDmemcpy(&dataset->shared->layout, &newlayout, sizeof(H5O_layout_t));

	    break;

	case H5D_CONTIGUOUS:
        case H5D_COMPACT:
	    HDassert(dataset->shared->layout.version > H5O_LAYOUT_VERSION_DEFAULT);
	    dataset->shared->layout.version = H5O_LAYOUT_VERSION_DEFAULT;
	    if(H5O_msg_write(&(dataset->oloc), H5O_LAYOUT_ID, 0, H5O_UPDATE_TIME, &(dataset->shared->layout), dxpl_id) < 0)
		HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to update layout message")
	    break;

	case H5D_VIRTUAL:
	    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "virtual dataset layout not supported")

        case H5D_LAYOUT_ERROR:
        case H5D_NLAYOUTS:
	    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid dataset layout type")

	default: 
	    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "unknown dataset layout type")
    } /* end switch */

done:
    if(ret_value < 0 && dataset->shared->layout.type == H5D_CHUNKED) {
	/* Remove new layout message */
	if(add_new_layout)
	    if(H5O_msg_remove(&dataset->oloc, H5O_LAYOUT_ID, H5O_ALL, FALSE, dxpl_id) < 0)
		HDONE_ERROR(H5E_SYM, H5E_CANTDELETE, FAIL, "unable to delete layout message")

	/* Add back old layout message */
	if(delete_old_layout)
	    if(H5O_msg_create(&dataset->oloc, H5O_LAYOUT_ID, 0, H5O_UPDATE_TIME, &dataset->shared->layout, dxpl_id) < 0)
		HDONE_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to add layout header message")

	/* Clean up v1 b-tree chunk index */
	if(init_new_index) {
	    if(H5F_addr_defined(new_idx_info.storage->idx_addr)) {
		/* Check for valid address i.e. tag */
		if(!H5F_addr_defined(dataset->oloc.addr))
		    HDONE_ERROR(H5E_DATASET, H5E_BADVALUE, FAIL, "address undefined")

		/* Expunge from cache all v1 B-tree type entries associated with tag */
		if(H5AC_expunge_tag_type_metadata(dataset->oloc.file, dxpl_id, dataset->oloc.addr, H5AC_BT_ID, H5AC__NO_FLAGS_SET))
		    HDONE_ERROR(H5E_DATASET, H5E_CANTEXPUNGE, FAIL, "unable to expunge index metadata")
	    } /* end if */

	    /* Delete v1 B-tree chunk index */
	    if(new_idx_info.storage->ops->dest && (new_idx_info.storage->ops->dest)(&new_idx_info) < 0)
		HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "unable to release chunk index info")
	} /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI_TAG(ret_value, FAIL)
} /* end H5D__format_convert() */


/*-------------------------------------------------------------------------
 * Function:    H5D__mark
 *
 * Purpose:     Mark some aspect of a dataset as dirty
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:  Quincey Koziol
 *              July 4, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__mark(const H5D_t *dataset, hid_t H5_ATTR_UNUSED dxpl_id, unsigned flags)
{
    H5O_t *oh = NULL;                   /* Pointer to dataset's object header */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(dataset);
    HDassert(!(flags & (unsigned)~(H5D_MARK_SPACE | H5D_MARK_LAYOUT)));

    /* Mark aspects of the dataset as dirty */
    if(flags) {
        unsigned update_flags = H5O_UPDATE_TIME;        /* Modification time flag */

        /* Pin the object header */
        if(NULL == (oh = H5O_pin(&dataset->oloc, dxpl_id)))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTPIN, FAIL, "unable to pin dataset object header")

        /* Update the layout on disk, if it's been changed */
        if(flags & H5D_MARK_LAYOUT) {
            if(H5D__layout_oh_write(dataset, dxpl_id, oh, update_flags) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to update layout info")

            /* Reset the "update the modification time" flag, so we only do it once */
            update_flags = 0;
        } /* end if */

        /* Update the dataspace on disk, if it's been changed */
        if(flags & H5D_MARK_SPACE) {
            if(H5S_write(dataset->oloc.file, dxpl_id, oh, update_flags, dataset->shared->space) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to update file with new dataspace")

            /* Reset the "update the modification time" flag, so we only do it once */
            update_flags = 0;
        } /* end if */

        /* _Somebody_ should have update the modification time! */
        HDassert(update_flags == 0);
    } /* end if */

done:
    /* Release pointer to object header */
    if(oh != NULL)
        if(H5O_unpin(oh) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTUNPIN, FAIL, "unable to unpin dataset object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__mark() */


/*-------------------------------------------------------------------------
 * Function:    H5D__flush_cb
 *
 * Purpose:     Flush any dataset information cached in memory
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:  Quincey Koziol
 *              November 8, 2007
 *
 *-------------------------------------------------------------------------
 */
static int
H5D__flush_cb(void *_dataset, hid_t H5_ATTR_UNUSED id, void *_udata)
{
    H5D_t       *dataset = (H5D_t *)_dataset;   /* Dataset pointer */
    H5D_flush_ud_t *udata = (H5D_flush_ud_t *)_udata;   /* User data for callback */
    int         ret_value = H5_ITER_CONT;       /* Return value */

    FUNC_ENTER_STATIC

    /* Check args */
    HDassert(dataset);

    /* Check for dataset in same file */
    if(udata->f == dataset->oloc.file) {
        /* Flush the dataset's information */
        if(H5D__flush_real(dataset, udata->dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, H5_ITER_ERROR, "unable to flush cached dataset info")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__flush_cb() */


/*-------------------------------------------------------------------------
 * Function:    H5D_flush
 *
 * Purpose:     Flush any dataset information cached in memory
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:  Ray Lu
 *              August 14, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_flush(const H5F_t *f, hid_t dxpl_id)
{
    H5D_flush_ud_t udata;               /* User data for callback */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(f);

    /* Set user data for callback */
    udata.f = f;
    udata.dxpl_id = dxpl_id;

    /* Iterate over all the open datasets */
    if(H5I_iterate(H5I_DATASET, H5D__flush_cb, &udata, FALSE) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_BADITER, FAIL, "unable to flush cached dataset info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_flush() */


/*-------------------------------------------------------------------------
 * Function:	H5D_get_create_plist
 *
 * Purpose:	Private function for H5Dget_create_plist
 *
 * Return:	Success:	ID for a copy of the dataset creation
 *				property list.  The template should be
 *				released by calling H5P_close().
 *
 *		Failure:	FAIL
 *
 * Programmer:	Robb Matzke
 *		Tuesday, February  3, 1998
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5D_get_create_plist(H5D_t *dset)
{
    H5P_genplist_t      *dcpl_plist;            /* Dataset's DCPL */
    H5P_genplist_t      *new_plist;             /* Copy of dataset's DCPL */
    H5O_layout_t        copied_layout;          /* Layout to tweak */
    H5O_fill_t          copied_fill;            /* Fill value to tweak */
    H5O_efl_t           copied_efl;             /* External file list to tweak */
    hid_t		new_dcpl_id = FAIL;
    hid_t		ret_value = H5I_INVALID_HID;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    if(NULL == (dcpl_plist = (H5P_genplist_t *)H5I_object(dset->shared->dcpl_id)))
        HGOTO_ERROR(H5E_DATASET, H5E_BADTYPE, FAIL, "can't get property list")

    /* Copy the creation property list */
    if((new_dcpl_id = H5P_copy_plist(dcpl_plist, TRUE)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to copy the creation property list")
    if(NULL == (new_plist = (H5P_genplist_t *)H5I_object(new_dcpl_id)))
        HGOTO_ERROR(H5E_DATASET, H5E_BADTYPE, FAIL, "can't get property list")

    /* Retrieve any object creation properties */
    if(H5O_get_create_plist(&dset->oloc, H5AC_ind_read_dxpl_id, new_plist) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get object creation info")

    /* Get the layout property */
    if(H5P_peek(new_plist, H5D_CRT_LAYOUT_NAME, &copied_layout) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get layout")

    /* Reset layout values set when dataset is created */
    copied_layout.ops = NULL;
    switch(copied_layout.type) {
        case H5D_COMPACT:
            copied_layout.storage.u.compact.buf = H5MM_xfree(copied_layout.storage.u.compact.buf);
            HDmemset(&copied_layout.storage.u.compact, 0, sizeof(copied_layout.storage.u.compact));
            break;

        case H5D_CONTIGUOUS:
            copied_layout.storage.u.contig.addr = HADDR_UNDEF;
            copied_layout.storage.u.contig.size = 0;
            break;

        case H5D_CHUNKED:
            /* Reset chunk size */
            copied_layout.u.chunk.size = 0;

            /* Reset index info, if the chunk ops are set */
            if(copied_layout.storage.u.chunk.ops)
		/* Reset address and pointer of the array struct for the chunked storage index */
                if(H5D_chunk_idx_reset(&copied_layout.storage.u.chunk, TRUE) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to reset chunked storage index in dest")

            /* Reset chunk index ops */
            copied_layout.storage.u.chunk.ops = NULL;
            break;

        case H5D_VIRTUAL:
            copied_layout.storage.u.virt.serial_list_hobjid.addr = HADDR_UNDEF;
            copied_layout.storage.u.virt.serial_list_hobjid.idx = 0;
            break;

        case H5D_LAYOUT_ERROR:
        case H5D_NLAYOUTS:
        default:
            HDassert(0 && "Unknown layout type!");
    } /* end switch */

    /* Set back the (possibly modified) layout property to property list */
    if(H5P_poke(new_plist, H5D_CRT_LAYOUT_NAME, &copied_layout) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to set layout")

    /* Get the fill value property */
    if(H5P_peek(new_plist, H5D_CRT_FILL_VALUE_NAME, &copied_fill) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get fill value")

    /* Check if there is a fill value, but no type yet */
    if(copied_fill.buf != NULL && copied_fill.type == NULL) {
        H5T_path_t *tpath;      /* Conversion information*/

        /* Copy the dataset type into the fill value message */
        if(NULL == (copied_fill.type = H5T_copy(dset->shared->type, H5T_COPY_TRANSIENT)))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to copy dataset datatype for fill value")

        /* Set up type conversion function */
        if(NULL == (tpath = H5T_path_find(dset->shared->type, copied_fill.type, NULL, NULL, H5AC_noio_dxpl_id, FALSE)))
            HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "unable to convert between src and dest data types")

        /* Convert disk form of fill value into memory form */
        if(!H5T_path_noop(tpath)) {
            hid_t dst_id, src_id;       /* Source & destination datatypes for type conversion */
            uint8_t *bkg_buf = NULL;    /* Background conversion buffer */
            size_t bkg_size;            /* Size of background buffer */

            /* Wrap copies of types to convert */
            dst_id = H5I_register(H5I_DATATYPE, H5T_copy(copied_fill.type, H5T_COPY_TRANSIENT), FALSE);
            if(dst_id < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to copy/register datatype")
            src_id = H5I_register(H5I_DATATYPE, H5T_copy(dset->shared->type, H5T_COPY_ALL), FALSE);
            if(src_id < 0) {
                H5I_dec_ref(dst_id);
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to copy/register datatype")
            } /* end if */

            /* Allocate a background buffer */
            bkg_size = MAX(H5T_GET_SIZE(copied_fill.type), H5T_GET_SIZE(dset->shared->type));
            if(H5T_path_bkg(tpath) && NULL == (bkg_buf = H5FL_BLK_CALLOC(type_conv, bkg_size))) {
                H5I_dec_ref(src_id);
                H5I_dec_ref(dst_id);
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "memory allocation failed")
            } /* end if */

            /* Convert fill value */
            if(H5T_convert(tpath, src_id, dst_id, (size_t)1, (size_t)0, (size_t)0, copied_fill.buf, bkg_buf, H5AC_noio_dxpl_id) < 0) {
                H5I_dec_ref(src_id);
                H5I_dec_ref(dst_id);
                if(bkg_buf)
                    bkg_buf = H5FL_BLK_FREE(type_conv, bkg_buf);
                HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, FAIL, "datatype conversion failed")
            } /* end if */

            /* Release local resources */
            if(H5I_dec_ref(src_id) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTDEC, FAIL, "unable to close temporary object")
            if(H5I_dec_ref(dst_id) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTDEC, FAIL, "unable to close temporary object")
            if(bkg_buf)
                bkg_buf = H5FL_BLK_FREE(type_conv, bkg_buf);
        } /* end if */
    } /* end if */

    /* Set back the (possibly modified) fill value property to property list */
    if(H5P_poke(new_plist, H5D_CRT_FILL_VALUE_NAME, &copied_fill) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to set fill value")

    /* Get the fill value property */
    if(H5P_peek(new_plist, H5D_CRT_EXT_FILE_LIST_NAME, &copied_efl) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get external file list")

    /* Reset efl name_offset and heap_addr, these are the values when the dataset is created */
    if(copied_efl.slot) {
        unsigned u;

        copied_efl.heap_addr = HADDR_UNDEF;
        for(u = 0; u < copied_efl.nused; u++)
            copied_efl.slot[u].name_offset = 0;
    } /* end if */

    /* Set back the (possibly modified) external file list property to property list */
    if(H5P_poke(new_plist, H5D_CRT_EXT_FILE_LIST_NAME, &copied_efl) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to set external file list")

    /* Set the return value */
    ret_value = new_dcpl_id;

done:
    if(ret_value < 0)
        if(new_dcpl_id > 0)
            if(H5I_dec_app_ref(new_dcpl_id) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CANTDEC, FAIL, "unable to close temporary object")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_get_create_plist() */


/*-------------------------------------------------------------------------
 * Function:	H5D_get_access_plist
 *
 * Purpose:	Returns a copy of the dataset access property list.
 *
 * Return:	Success:	ID for a copy of the dataset access
 *				property list. 
 *
 *		Failure:	FAIL
 *
 * Programmer:	Mohamad Chaarawi
 *		March, 2012
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5D_get_access_plist(H5D_t *dset)
{
    H5P_genplist_t      *old_plist;     /* Default DAPL */
    H5P_genplist_t      *new_plist;     /* New DAPL */
    hid_t               new_dapl_id = FAIL;
    hid_t               ret_value = FAIL;

    FUNC_ENTER_NOAPI_NOINIT

    /* Make a copy of the default dataset access property list */
    if(NULL == (old_plist = (H5P_genplist_t *)H5I_object(H5P_LST_DATASET_ACCESS_ID_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")
    if((new_dapl_id = H5P_copy_plist(old_plist, TRUE)) < 0)
        HGOTO_ERROR(H5E_INTERNAL, H5E_CANTINIT, FAIL, "can't copy dataset access property list")
    if(NULL == (new_plist = (H5P_genplist_t *)H5I_object(new_dapl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")

    /* If the dataset is chunked then copy the rdcc & append flush parameters */
    if(dset->shared->layout.type == H5D_CHUNKED) {
        if(H5P_set(new_plist, H5D_ACS_DATA_CACHE_NUM_SLOTS_NAME, &(dset->shared->cache.chunk.nslots)) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set data cache number of slots")
        if(H5P_set(new_plist, H5D_ACS_DATA_CACHE_BYTE_SIZE_NAME, &(dset->shared->cache.chunk.nbytes_max)) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set data cache byte size")
        if(H5P_set(new_plist, H5D_ACS_PREEMPT_READ_CHUNKS_NAME, &(dset->shared->cache.chunk.w0)) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set preempt read chunks")
        if(H5P_set(new_plist, H5D_ACS_APPEND_FLUSH_NAME, &dset->shared->append_flush) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set append flush property")
    } /* end if */

    /* Set the VDS view & printf gap options */
    if(H5P_set(new_plist, H5D_ACS_VDS_VIEW_NAME, &(dset->shared->layout.storage.u.virt.view)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set VDS view")
    if(H5P_set(new_plist, H5D_ACS_VDS_PRINTF_GAP_NAME, &(dset->shared->layout.storage.u.virt.printf_gap)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set VDS printf gap")

    /* Set the external file prefix option */
    if(H5P_set(new_plist, H5D_ACS_EFILE_PREFIX_NAME, &(dset->shared->extfile_prefix)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set external file prefix")

    /* Set the return value */
    ret_value = new_dapl_id;

done:
    if(ret_value < 0) {
        if(new_dapl_id > 0)
            if(H5I_dec_app_ref(new_dapl_id) < 0)
                HDONE_ERROR(H5E_SYM, H5E_CANTDEC, FAIL, "can't free")
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_get_access_plist() */


/*-------------------------------------------------------------------------
 * Function:	H5D_get_space
 *
 * Purpose:	Returns and ID for the dataspace of the dataset.
 *
 * Return:	Success:	ID for dataspace
 *
 *		Failure:	FAIL
 *
 * Programmer:	Mohamad Chaarawi
 *		March, 2012
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5D_get_space(H5D_t *dset)
{
    H5S_t	*space = NULL;
    hid_t       ret_value = H5I_INVALID_HID;

    FUNC_ENTER_NOAPI_NOINIT

    /* If the layout is virtual, update the extent */
    if(dset->shared->layout.type == H5D_VIRTUAL)
        if(H5D__virtual_set_extent_unlim(dset, H5AC_ind_read_dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update virtual dataset extent")

    /* Read the data space message and return a data space object */
    if(NULL == (space = H5S_copy(dset->shared->space, FALSE, TRUE)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to get data space")

    /* Create an atom */
    if((ret_value = H5I_register(H5I_DATASPACE, space, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register dataspace")

done:
    if(ret_value < 0)
        if(space != NULL)
            if(H5S_close(space) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataspace")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_get_space() */


/*-------------------------------------------------------------------------
 * Function:	H5D_get_type
 *
 * Purpose:	Returns and ID for the datatype of the dataset.
 *
 * Return:	Success:	ID for datatype
 *
 *		Failure:	FAIL
 *
 * Programmer:	Mohamad Chaarawi
 *		March, 2012
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5D_get_type(H5D_t *dset)
{
    H5T_t	*dt = NULL;
    hid_t       ret_value = FAIL;

    FUNC_ENTER_NOAPI_NOINIT

    /* Patch the datatype's "top level" file pointer */
    if(H5T_patch_file(dset->shared->type, dset->oloc.file) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to patch datatype's file pointer")

    /* Copy the dataset's datatype */
    if(NULL == (dt = H5T_copy(dset->shared->type, H5T_COPY_REOPEN)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to copy datatype")

    /* Mark any datatypes as being in memory now */
    if(H5T_set_loc(dt, NULL, H5T_LOC_MEMORY) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "invalid datatype location")

    /* Lock copied type */
    if(H5T_lock(dt, FALSE) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to lock transient datatype")

    if((ret_value = H5I_register(H5I_DATATYPE, dt, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register datatype")

done:
    if(ret_value < 0) {
        if(dt && H5T_close(dt) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release datatype")
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_get_type() */


/*-------------------------------------------------------------------------
 * Function:	H5D__refresh
 *
 * Purpose:     Refreshes all buffers associated with a dataset.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:	Dana Robinson
 *		        November 2015
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__refresh(hid_t dset_id, H5D_t *dset, hid_t dxpl_id)
{
    H5D_virtual_held_file_t *head = NULL;       /* Pointer to list of files held open */
    hbool_t virt_dsets_held = FALSE;            /* Whether virtual datasets' files are held open */
    herr_t      ret_value   = SUCCEED;          /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(dset);
    HDassert(dset->shared);

    /* If the layout is virtual... */
    if(dset->shared->layout.type == H5D_VIRTUAL) {
        /* Hold open the source datasets' files */
        if(H5D__virtual_hold_source_dset_files(dset, &head) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINC, FAIL, "unable to hold VDS source files open")
        virt_dsets_held = TRUE;

        /* Refresh source datasets for virtual dataset */
        if(H5D__virtual_refresh_source_dsets(dset, dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTFLUSH, FAIL, "unable to refresh VDS source datasets")
    } /* end if */
    
    /* Refresh dataset object */
    if((H5O_refresh_metadata(dset_id, dset->oloc, dxpl_id)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTFLUSH, FAIL, "unable to refresh dataset")

done:
    /* Release hold on virtual datasets' files */
    if(virt_dsets_held) {
        /* Release the hold on source datasets' files */
        if(H5D__virtual_release_source_dset_files(head) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTDEC, FAIL, "can't release VDS source files held open")
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__refresh() */

