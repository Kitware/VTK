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

/****************/
/* Module Setup */
/****************/

#define H5D_PACKAGE		/*suppress error about including H5Dpkg	  */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5D_init_pub_interface


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Dpkg.h"		/* Datasets 				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free lists                           */
#include "H5Iprivate.h"		/* IDs			  		*/


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Local Prototypes */
/********************/


/*********************/
/* Package Variables */
/*********************/

/* Declare extern the free list to manage blocks of VL data */
H5FL_BLK_EXTERN(vlen_vl_buf);

/* Declare extern the free list to manage other blocks of VL data */
H5FL_BLK_EXTERN(vlen_fl_buf);


/*****************************/
/* Library Private Variables */
/*****************************/

/* Declare extern the free list to manage blocks of type conversion data */
H5FL_BLK_EXTERN(type_conv);


/*******************/
/* Local Variables */
/*******************/



/*--------------------------------------------------------------------------
NAME
   H5D_init_pub_interface -- Initialize interface-specific information
USAGE
    herr_t H5D_init_pub_interface()
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5D_init() currently).

--------------------------------------------------------------------------*/
static herr_t
H5D_init_pub_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_init_pub_interface)

    FUNC_LEAVE_NOAPI(H5D_init())
} /* H5D_init_pub_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5Dcreate2
 *
 * Purpose:	Creates a new dataset named NAME at LOC_ID, opens the
 *		dataset for access, and associates with that dataset constant
 *		and initial persistent properties including the type of each
 *		datapoint as stored in the file (TYPE_ID), the size of the
 *		dataset (SPACE_ID), and other initial miscellaneous
 *		properties (DCPL_ID).
 *
 *		All arguments are copied into the dataset, so the caller is
 *		allowed to derive new types, data spaces, and creation
 *		parameters from the old ones and reuse them in calls to
 *		create other datasets.
 *
 * Return:	Success:	The object ID of the new dataset.  At this
 *				point, the dataset is ready to receive its
 *				raw data.  Attempting to read raw data from
 *				the dataset will probably return the fill
 *				value.	The dataset should be closed when the
 *				caller is no longer interested in it.
 *
 *		Failure:	FAIL
 *
 * Programmer:	Quincey Koziol
 *		Thursday, April 5, 2007
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Dcreate2(hid_t loc_id, const char *name, hid_t type_id, hid_t space_id,
    hid_t lcpl_id, hid_t dcpl_id, hid_t dapl_id)
{
    H5G_loc_t	   loc;                 /* Object location to insert dataset into */
    H5D_t	   *dset = NULL;        /* New dataset's info */
    const H5S_t    *space;              /* Dataspace for dataset */
    hid_t           ret_value;          /* Return value */

    FUNC_ENTER_API(H5Dcreate2, FAIL)
    H5TRACE7("i", "i*siiiii", loc_id, name, type_id, space_id, lcpl_id, dcpl_id,
             dapl_id);

    /* Check arguments */
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location ID")
    if(H5I_DATATYPE != H5I_get_type(type_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype ID")
    if(NULL == (space = (const H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace ID")

    /* Get correct property list */
    if(H5P_DEFAULT == lcpl_id)
        lcpl_id = H5P_LINK_CREATE_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lcpl_id, H5P_LINK_CREATE))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link creation property list")

    /* Get correct property list */
    if(H5P_DEFAULT == dcpl_id)
        dcpl_id = H5P_DATASET_CREATE_DEFAULT;
    else
        if(TRUE != H5P_isa_class(dcpl_id, H5P_DATASET_CREATE))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not dataset create property list ID")

    /* Get correct property list */
    if(H5P_DEFAULT == dapl_id)
        dapl_id = H5P_DATASET_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(dapl_id, H5P_DATASET_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not dataset access property list")

    /* Create the new dataset & get its ID */
    if(NULL == (dset = H5D_create_named(&loc, name, type_id, space, lcpl_id, dcpl_id, dapl_id, H5AC_dxpl_id)))
	HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to create dataset")
    if((ret_value = H5I_register(H5I_DATASET, dset, TRUE)) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTREGISTER, FAIL, "unable to register dataset")

done:
    if(ret_value < 0)
        if(dset && H5D_close(dset) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataset")

    FUNC_LEAVE_API(ret_value)
} /* end H5Dcreate2() */


/*-------------------------------------------------------------------------
 * Function:	H5Dcreate_anon
 *
 * Purpose:	Creates a new dataset named NAME at LOC_ID, opens the
 *		dataset for access, and associates with that dataset constant
 *		and initial persistent properties including the type of each
 *		datapoint as stored in the file (TYPE_ID), the size of the
 *		dataset (SPACE_ID), and other initial miscellaneous
 *		properties (DCPL_ID).
 *
 *		All arguments are copied into the dataset, so the caller is
 *		allowed to derive new types, data spaces, and creation
 *		parameters from the old ones and reuse them in calls to
 *		create other datasets.
 *
 *              The resulting ID should be linked into the file with
 *              H5Olink or it will be deleted when closed.
 *
 * Return:	Success:	The object ID of the new dataset.  At this
 *				point, the dataset is ready to receive its
 *				raw data.  Attempting to read raw data from
 *				the dataset will probably return the fill
 *				value.	The dataset should be linked into
 *                              the group hierarchy before being closed or
 *                              it will be deleted.  The dataset should be
 *                              closed when the caller is no longer interested
 *                              in it.
 *
 *		Failure:	FAIL
 *
 * Programmer:	James Laird
 *		Tuesday, January 24, 2006
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Dcreate_anon(hid_t loc_id, hid_t type_id, hid_t space_id, hid_t dcpl_id,
    hid_t dapl_id)
{
    H5G_loc_t	   loc;                 /* Object location to insert dataset into */
    H5D_t	   *dset = NULL;        /* New dataset's info */
    const H5S_t    *space;              /* Dataspace for dataset */
    hid_t           ret_value;          /* Return value */

    FUNC_ENTER_API(H5Dcreate_anon, FAIL)
    H5TRACE5("i", "iiiii", loc_id, type_id, space_id, dcpl_id, dapl_id);

    /* Check arguments */
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location ID")
    if(H5I_DATATYPE != H5I_get_type(type_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype ID")
    if(NULL == (space = (const H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace ID")
    if(H5P_DEFAULT == dcpl_id)
        dcpl_id = H5P_DATASET_CREATE_DEFAULT;
    else
        if(TRUE != H5P_isa_class(dcpl_id, H5P_DATASET_CREATE))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not dataset create property list ID")

    /* Get correct property list */
    if(H5P_DEFAULT == dapl_id)
        dapl_id = H5P_DATASET_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(dapl_id, H5P_DATASET_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not dataset access property list")

    /* build and open the new dataset */
    if(NULL == (dset = H5D_create(loc.oloc->file, type_id, space, dcpl_id, dapl_id, H5AC_dxpl_id)))
	HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to create dataset")

    /* Register the new dataset to get an ID for it */
    if((ret_value = H5I_register(H5I_DATASET, dset, TRUE)) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTREGISTER, FAIL, "unable to register dataset")

done:
    if(ret_value < 0)
        if(dset && H5D_close(dset) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataset")

    FUNC_LEAVE_API(ret_value)
} /* end H5Dcreate_anon() */


/*-------------------------------------------------------------------------
 * Function:	H5Dopen2
 *
 * Purpose:	Finds a dataset named NAME at LOC_ID, opens it, and returns
 *		its ID.	 The dataset should be close when the caller is no
 *		longer interested in it.
 *
 *              Takes a dataset access property list
 *
 * Return:	Success:	A new dataset ID
 *		Failure:	FAIL
 *
 * Programmer:	James Laird
 *		Thursday, July 27, 2006
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Dopen2(hid_t loc_id, const char *name, hid_t dapl_id)
{
    H5D_t       *dset = NULL;
    H5G_loc_t	 loc;		        /* Object location of group */
    H5G_loc_t	 dset_loc;		/* Object location of dataset */
    H5G_name_t   path;            	/* Dataset group hier. path */
    H5O_loc_t    oloc;            	/* Dataset object location */
    H5O_type_t   obj_type;              /* Type of object at location */
    hbool_t      loc_found = FALSE;     /* Location at 'name' found */
    hid_t        dxpl_id = H5AC_dxpl_id;    /* dxpl to use to open datset */
    hid_t        ret_value;

    FUNC_ENTER_API(H5Dopen2, FAIL)
    H5TRACE3("i", "i*si", loc_id, name, dapl_id);

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")

    /* Get correct property list */
    if(H5P_DEFAULT == dapl_id)
        dapl_id = H5P_DATASET_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(dapl_id, H5P_DATASET_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not dataset access property list")

    /* Set up dataset location to fill in */
    dset_loc.oloc = &oloc;
    dset_loc.path = &path;
    H5G_loc_reset(&dset_loc);

    /* Find the dataset object */
    if(H5G_loc_find(&loc, name, &dset_loc, dapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_NOTFOUND, FAIL, "not found")
    loc_found = TRUE;

    /* Check that the object found is the correct type */
    if(H5O_obj_type(&oloc, &obj_type, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get object type")
    if(obj_type != H5O_TYPE_DATASET)
        HGOTO_ERROR(H5E_DATASET, H5E_BADTYPE, FAIL, "not a dataset")

    /* Open the dataset */
    if(NULL == (dset = H5D_open(&dset_loc, dapl_id, dxpl_id)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't open dataset")

    /* Register an atom for the dataset */
    if((ret_value = H5I_register(H5I_DATASET, dset, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "can't register dataset atom")

done:
    if(ret_value < 0) {
        if(dset) {
            if(H5D_close(dset) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataset")
        } /* end if */
        else {
            if(loc_found && H5G_loc_free(&dset_loc) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "can't free location")
        } /* end else */
    } /* end if */

    FUNC_LEAVE_API(ret_value)
} /* end H5Dopen2() */


/*-------------------------------------------------------------------------
 * Function:	H5Dclose
 *
 * Purpose:	Closes access to a dataset (DATASET_ID) and releases
 *		resources used by it. It is illegal to subsequently use that
 *		same dataset ID in calls to other dataset functions.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Thursday, December  4, 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dclose(hid_t dset_id)
{
    herr_t       ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_API(H5Dclose, FAIL)
    H5TRACE1("e", "i", dset_id);

    /* Check args */
    if(NULL == H5I_object_verify(dset_id, H5I_DATASET))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    /*
     * Decrement the counter on the dataset.  It will be freed if the count
     * reaches zero.
     */
    if(H5I_dec_ref(dset_id, TRUE) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't free")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Dclose() */


/*-------------------------------------------------------------------------
 * Function:	H5Dget_space
 *
 * Purpose:	Returns a copy of the file data space for a dataset.
 *
 * Return:	Success:	ID for a copy of the data space.  The data
 *				space should be released by calling
 *				H5Sclose().
 *
 *		Failure:	FAIL
 *
 * Programmer:	Robb Matzke
 *		Wednesday, January 28, 1998
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Dget_space(hid_t dset_id)
{
    H5D_t	*dset = NULL;
    H5S_t	*space = NULL;
    hid_t	ret_value;

    FUNC_ENTER_API(H5Dget_space, FAIL)
    H5TRACE1("i", "i", dset_id);

    /* Check args */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    /* Read the data space message and return a data space object */
    if(NULL == (space = H5S_copy(dset->shared->space, FALSE, TRUE)))
	HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to get data space")

    /* Create an atom */
    if((ret_value = H5I_register (H5I_DATASPACE, space, TRUE)) < 0)
	HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register data space")

done:
    if(ret_value < 0) {
        if(space!=NULL) {
            if(H5S_close(space) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataspace")
        } /* end if */
    } /* end if */

    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5Dget_space_status
 *
 * Purpose:	Returns the status of data space allocation.
 *
 * Return:
 *		Success:	Non-negative
 *
 *		Failture:	Negative
 *
 * Programmer:	Raymond Lu
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dget_space_status(hid_t dset_id, H5D_space_status_t *allocation)
{
    H5D_t 	*dset = NULL;
    herr_t 	ret_value = SUCCEED;

    FUNC_ENTER_API(H5Dget_space_status, FAIL)
    H5TRACE2("e", "i*Ds", dset_id, allocation);

    /* Check arguments */
    if(NULL==(dset=(H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    /* Read data space address and return */
    if(FAIL==(ret_value=H5D_get_space_status(dset, allocation, H5AC_ind_dxpl_id)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to get space status")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5Dget_type
 *
 * Purpose:	Returns a copy of the file datatype for a dataset.
 *
 * Return:	Success:	ID for a copy of the datatype.	 The data
 *				type should be released by calling
 *				H5Tclose().
 *
 *		Failure:	FAIL
 *
 * Programmer:	Robb Matzke
 *		Tuesday, February  3, 1998
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Dget_type(hid_t dset_id)
{

    H5D_t	*dset;                  /* Dataset */
    H5T_t	*dt = NULL;             /* Datatype to return */
    hid_t	ret_value;              /* Return value */

    FUNC_ENTER_API(H5Dget_type, FAIL)
    H5TRACE1("i", "i", dset_id);

    /* Check args */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    /* Copy the dataset's datatype */
    if(NULL == (dt = H5T_copy(dset->shared->type, H5T_COPY_REOPEN)))
	HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to copy datatype")

    /* Mark any datatypes as being in memory now */
    if(H5T_set_loc(dt, NULL, H5T_LOC_MEMORY) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "invalid datatype location")

    /* Lock copied type */
    if(H5T_lock(dt, FALSE) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to lock transient datatype")

    /* Create an atom */
    if((ret_value = H5I_register(H5I_DATATYPE, dt, TRUE)) < 0)
	HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register datatype")

done:
    if(ret_value < 0) {
        if(dt && H5T_close(dt) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release datatype")
    } /* end if */

    FUNC_LEAVE_API(ret_value)
} /* end H5Dget_type() */


/*-------------------------------------------------------------------------
 * Function:	H5Dget_create_plist
 *
 * Purpose:	Returns a copy of the dataset creation property list.
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
H5Dget_create_plist(hid_t dset_id)
{
    H5D_t		*dset;                  /* Dataset structure */
    H5P_genplist_t      *dcpl_plist;            /* Dataset's DCPL */
    H5P_genplist_t      *new_plist;             /* Copy of dataset's DCPL */
    H5O_fill_t          copied_fill;            /* Fill value to tweak */
    hid_t		new_dcpl_id = FAIL;
    hid_t		ret_value;              /* Return value */

    FUNC_ENTER_API(H5Dget_create_plist, FAIL)
    H5TRACE1("i", "i", dset_id);

    /* Check args */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")
    if(NULL == (dcpl_plist = (H5P_genplist_t *)H5I_object(dset->shared->dcpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "can't get property list")

    /* Copy the creation property list */
    if((new_dcpl_id = H5P_copy_plist(dcpl_plist, TRUE)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to copy the creation property list")
    if(NULL == (new_plist = (H5P_genplist_t *)H5I_object(new_dcpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "can't get property list")

    /* Retrieve any object creation properties */
    if(H5O_get_create_plist(&dset->oloc, H5AC_ind_dxpl_id, new_plist) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get object creation info")

    /* Get the fill value property */
    if(H5P_get(new_plist, H5D_CRT_FILL_VALUE_NAME, &copied_fill) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fill value")

    /* Check if there is a fill value, but no type yet */
    if(copied_fill.buf != NULL && copied_fill.type == NULL) {
        H5T_path_t *tpath;      /* Conversion information*/

        /* Copy the dataset type into the fill value message */
        if(NULL == (copied_fill.type = H5T_copy(dset->shared->type, H5T_COPY_TRANSIENT)))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to copy dataset datatype for fill value")

        /* Set up type conversion function */
        if(NULL == (tpath = H5T_path_find(dset->shared->type, copied_fill.type, NULL, NULL, H5AC_ind_dxpl_id, FALSE)))
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
                H5I_dec_ref(dst_id, FALSE);
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to copy/register datatype")
            } /* end if */

            /* Allocate a background buffer */
            bkg_size = MAX(H5T_GET_SIZE(copied_fill.type), H5T_GET_SIZE(dset->shared->type));
            if(H5T_path_bkg(tpath) && NULL == (bkg_buf = H5FL_BLK_CALLOC(type_conv, bkg_size))) {
                H5I_dec_ref(src_id, FALSE);
                H5I_dec_ref(dst_id, FALSE);
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
            } /* end if */

            /* Convert fill value */
            if(H5T_convert(tpath, src_id, dst_id, (size_t)1, (size_t)0, (size_t)0, copied_fill.buf, bkg_buf, H5AC_ind_dxpl_id) < 0) {
                H5I_dec_ref(src_id, FALSE);
                H5I_dec_ref(dst_id, FALSE);
                if(bkg_buf)
                    bkg_buf = H5FL_BLK_FREE(type_conv, bkg_buf);
                HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, FAIL, "datatype conversion failed")
            } /* end if */

            /* Release local resources */
            H5I_dec_ref(src_id, FALSE);
            H5I_dec_ref(dst_id, FALSE);
            if(bkg_buf)
                bkg_buf = H5FL_BLK_FREE(type_conv, bkg_buf);
        } /* end if */
    } /* end if */

    /* Set back the fill value property to property list */
    if(H5P_set(new_plist, H5D_CRT_FILL_VALUE_NAME, &copied_fill) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to set property list fill value")

    /* Set the return value */
    ret_value = new_dcpl_id;

done:
    if(ret_value < 0)
        if(new_dcpl_id > 0)
            (void)H5I_dec_ref(new_dcpl_id, TRUE);

    FUNC_LEAVE_API(ret_value)
} /* end H5Dget_create_plist() */


/*-------------------------------------------------------------------------
 * Function:	H5Dget_access_plist
 *
 * Purpose:	Returns a copy of the dataset creation property list.
 *
 * Description: H5Dget_access_plist returns the dataset access property
 *      list identifier of the specified dataset.
 *
 *      The chunk cache parameters in the returned property lists will be
 *      those used by the dataset.  If the properties in the file access
 *      property list were used to determine the dataset’s chunk cache
 *      configuration, then those properties will be present in the
 *      returned dataset access property list.  If the dataset does not
 *      use a chunked layout, then the chunk cache properties will be set
 *      to the default.  The chunk cache properties in the returned list
 *      are considered to be “set”, and any use of this list will override
 *      the corresponding properties in the file’s file access property
 *      list.
 *
 *      All link access properties in the returned list will be set to the
 *      default values.
 *
 * Return:	Success:	ID for a copy of the dataset access
 *				property list.  The template should be
 *				released by calling H5Pclose().
 *
 *		Failure:	FAIL
 *
 * Programmer:	Neil Fortner
 *		Wednesday, October 29, 2008
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Dget_access_plist(hid_t dset_id)
{
    H5D_t           *dset;          /* Dataset structure */
    H5P_genplist_t  *old_plist;     /* Default DAPL */
    H5P_genplist_t  *new_plist;     /* New DAPL */
    hid_t           new_dapl_id = FAIL;
    hid_t           ret_value;      /* Return value */

    FUNC_ENTER_API(H5Dget_access_plist, FAIL)
    H5TRACE1("i", "i", dset_id);

    /* Check args */
    if (NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    /* Make a copy of the default dataset access property list */
    if (NULL == (old_plist = (H5P_genplist_t *)H5I_object(H5P_LST_DATASET_ACCESS_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")
    if ((new_dapl_id = H5P_copy_plist(old_plist, TRUE)) < 0)
        HGOTO_ERROR(H5E_INTERNAL, H5E_CANTINIT, FAIL, "can't copy dataset access property list")
    if (NULL == (new_plist = (H5P_genplist_t *)H5I_object(new_dapl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")

    /* If the dataset is chunked then copy the rdcc parameters */
    if (dset->shared->layout.type == H5D_CHUNKED) {
        if (H5P_set(new_plist, H5D_ACS_DATA_CACHE_NUM_SLOTS_NAME, &(dset->shared->cache.chunk.nslots)) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set data cache number of slots")
        if (H5P_set(new_plist, H5D_ACS_DATA_CACHE_BYTE_SIZE_NAME, &(dset->shared->cache.chunk.nbytes_max)) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set data cache byte size")
        if (H5P_set(new_plist, H5D_ACS_PREEMPT_READ_CHUNKS_NAME, &(dset->shared->cache.chunk.w0)) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set preempt read chunks")
    } /* end if */

    /* Set the return value */
    ret_value = new_dapl_id;

done:
    if(ret_value < 0)
        if(new_dapl_id >= 0)
            (void)H5I_dec_ref(new_dapl_id, TRUE);

    FUNC_LEAVE_API(ret_value)
} /* end H5Dget_access_plist() */


/*-------------------------------------------------------------------------
 * Function:	H5Dget_storage_size
 *
 * Purpose:	Returns the amount of storage that is required for the
 *		dataset. For chunked datasets this is the number of allocated
 *		chunks times the chunk size.
 *
 * Return:	Success:	The amount of storage space allocated for the
 *				dataset, not counting meta data. The return
 *				value may be zero if no data has been stored.
 *
 *		Failure:	Zero
 *
 * Programmer:	Robb Matzke
 *              Wednesday, April 21, 1999
 *
 *-------------------------------------------------------------------------
 */
hsize_t
H5Dget_storage_size(hid_t dset_id)
{
    H5D_t	*dset;          /* Dataset to query */
    hsize_t	ret_value;      /* Return value */

    FUNC_ENTER_API(H5Dget_storage_size, 0)
    H5TRACE1("h", "i", dset_id);

    /* Check args */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, 0, "not a dataset")

    /* Set return value */
    ret_value = H5D_get_storage_size(dset, H5AC_ind_dxpl_id);

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Dget_storage_size() */


/*-------------------------------------------------------------------------
 * Function:	H5Dget_offset
 *
 * Purpose:	Returns the address of dataset in file.
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
H5Dget_offset(hid_t dset_id)
{
    H5D_t	*dset;          /* Dataset to query */
    haddr_t	ret_value;      /* Return value */

    FUNC_ENTER_API(H5Dget_offset, HADDR_UNDEF)
    H5TRACE1("a", "i", dset_id);

    /* Check args */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, HADDR_UNDEF, "not a dataset")

    /* Set return value */
    ret_value = H5D_get_offset(dset);

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Dget_offset() */


/*-------------------------------------------------------------------------
 * Function:	H5Diterate
 *
 * Purpose:	This routine iterates over all the elements selected in a memory
 *      buffer.  The callback function is called once for each element selected
 *      in the dataspace.  The selection in the dataspace is modified so
 *      that any elements already iterated over are removed from the selection
 *      if the iteration is interrupted (by the H5D_operator_t function
 *      returning non-zero) in the "middle" of the iteration and may be
 *      re-started by the user where it left off.
 *
 *      NOTE: Until "subtracting" elements from a selection is implemented,
 *          the selection is not modified.
 *
 * Parameters:
 *      void *buf;          IN/OUT: Pointer to the buffer in memory containing
 *                              the elements to iterate over.
 *      hid_t type_id;      IN: Datatype ID for the elements stored in BUF.
 *      hid_t space_id;     IN: Dataspace ID for BUF, also contains the
 *                              selection to iterate over.
 *      H5D_operator_t op; IN: Function pointer to the routine to be
 *                              called for each element in BUF iterated over.
 *      void *operator_data;    IN/OUT: Pointer to any user-defined data
 *                              associated with the operation.
 *
 * Operation information:
 *      H5D_operator_t is defined as:
 *          typedef herr_t (*H5D_operator_t)(void *elem, hid_t type_id,
 *              unsigned ndim, const hsize_t *point, void *operator_data);
 *
 *      H5D_operator_t parameters:
 *          void *elem;         IN/OUT: Pointer to the element in memory containing
 *                                  the current point.
 *          hid_t type_id;      IN: Datatype ID for the elements stored in ELEM.
 *          unsigned ndim;       IN: Number of dimensions for POINT array
 *          const hsize_t *point; IN: Array containing the location of the element
 *                                  within the original dataspace.
 *          void *operator_data;    IN/OUT: Pointer to any user-defined data
 *                                  associated with the operation.
 *
 *      The return values from an operator are:
 *          Zero causes the iterator to continue, returning zero when all
 *              elements have been processed.
 *          Positive causes the iterator to immediately return that positive
 *              value, indicating short-circuit success.  The iterator can be
 *              restarted at the next element.
 *          Negative causes the iterator to immediately return that value,
 *              indicating failure. The iterator can be restarted at the next
 *              element.
 *
 * Return:	Returns the return value of the last operator if it was non-zero,
 *          or zero if all elements were processed. Otherwise returns a
 *          negative value.
 *
 * Programmer:	Quincey Koziol
 *              Friday, June 11, 1999
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Diterate(void *buf, hid_t type_id, hid_t space_id, H5D_operator_t op,
        void *operator_data)
{
    H5S_t *space;               /* Dataspace for iteration */
    herr_t ret_value;           /* Return value */

    FUNC_ENTER_API(H5Diterate, FAIL)
    H5TRACE5("e", "*xiix*x", buf, type_id, space_id, op, operator_data);

    /* Check args */
    if(NULL == op)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid operator")
    if(NULL == buf)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid buffer")
    if(H5I_DATATYPE != H5I_get_type(type_id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid datatype")
    if(NULL == (space = (H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid dataspace")
    if(!(H5S_has_extent(space)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "dataspace does not have extent set")

    ret_value = H5D_iterate(buf, type_id, space, op, operator_data);

done:
    FUNC_LEAVE_API(ret_value)
}   /* end H5Diterate() */


/*-------------------------------------------------------------------------
 * Function:	H5Dvlen_reclaim
 *
 * Purpose:	Frees the buffers allocated for storing variable-length data
 *      in memory.  Only frees the VL data in the selection defined in the
 *      dataspace.  The dataset transfer property list is required to find the
 *      correct allocation/free methods for the VL data in the buffer.
 *
 * Return:	Non-negative on success, negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, June 10, 1999
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dvlen_reclaim(hid_t type_id, hid_t space_id, hid_t plist_id, void *buf)
{
    H5S_t *space;               /* Dataspace for iteration */
    herr_t ret_value;           /* Return value */

    FUNC_ENTER_API(H5Dvlen_reclaim, FAIL)
    H5TRACE4("e", "iii*x", type_id, space_id, plist_id, buf);

    /* Check args */
    if(H5I_DATATYPE != H5I_get_type(type_id) || buf == NULL)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid argument")
    if(NULL == (space = (H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid dataspace")
    if(!(H5S_has_extent(space)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "dataspace does not have extent set")

    /* Get the default dataset transfer property list if the user didn't provide one */
    if(H5P_DEFAULT == plist_id)
        plist_id = H5P_DATASET_XFER_DEFAULT;
    else
        if(TRUE != H5P_isa_class(plist_id, H5P_DATASET_XFER))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not xfer parms")

    /* Call internal routine */
    ret_value = H5D_vlen_reclaim(type_id, space, plist_id, buf);

done:
    FUNC_LEAVE_API(ret_value)
}   /* end H5Dvlen_reclaim() */


/*-------------------------------------------------------------------------
 * Function:	H5Dvlen_get_buf_size
 *
 * Purpose:	This routine checks the number of bytes required to store the VL
 *      data from the dataset, using the space_id for the selection in the
 *      dataset on disk and the type_id for the memory representation of the
 *      VL data, in memory.  The *size value is modified according to how many
 *      bytes are required to store the VL data in memory.
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
 *              Wednesday, August 11, 1999
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dvlen_get_buf_size(hid_t dataset_id, hid_t type_id, hid_t space_id,
        hsize_t *size)
{
    H5D_vlen_bufsize_t vlen_bufsize = {0, 0, 0, 0, 0, 0, 0};
    char bogus;                 /* bogus value to pass to H5Diterate() */
    H5S_t *space;               /* Dataspace for iteration */
    H5P_genclass_t  *pclass;    /* Property class */
    H5P_genplist_t  *plist;     /* Property list */
    herr_t ret_value;           /* Return value */

    FUNC_ENTER_API(H5Dvlen_get_buf_size, FAIL)
    H5TRACE4("e", "iii*h", dataset_id, type_id, space_id, size);

    /* Check args */
    if(H5I_DATASET != H5I_get_type(dataset_id) ||
            H5I_DATATYPE != H5I_get_type(type_id) || size == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid argument")
    if(NULL == (space = (H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid dataspace")
    if(!(H5S_has_extent(space)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "dataspace does not have extent set")

    /* Save the dataset ID */
    vlen_bufsize.dataset_id = dataset_id;

    /* Get a copy of the dataspace ID */
    if((vlen_bufsize.fspace_id = H5Dget_space(dataset_id)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "can't copy dataspace")

    /* Create a scalar for the memory dataspace */
    if((vlen_bufsize.mspace_id = H5Screate(H5S_SCALAR)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "can't create dataspace")

    /* Grab the temporary buffers required */
    if(NULL == (vlen_bufsize.fl_tbuf = H5FL_BLK_MALLOC(vlen_fl_buf, (size_t)1)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "no temporary buffers available")
    if(NULL == (vlen_bufsize.vl_tbuf = H5FL_BLK_MALLOC(vlen_vl_buf, (size_t)1)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "no temporary buffers available")

    /* Get the pointer to the dataset transfer class */
    if(NULL == (pclass = (H5P_genclass_t *)H5I_object(H5P_CLS_DATASET_XFER_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list class")

    /* Change to the custom memory allocation routines for reading VL data */
    if((vlen_bufsize.xfer_pid = H5P_create_id(pclass, FALSE)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTCREATE, FAIL, "no dataset xfer plists available")

    /* Get the property list struct */
    if(NULL == (plist = (H5P_genplist_t *)H5I_object(vlen_bufsize.xfer_pid)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset transfer property list")

    /* Set the memory manager to the special allocation routine */
    if(H5P_set_vlen_mem_manager(plist, H5D_vlen_get_buf_size_alloc, &vlen_bufsize, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't set VL data allocation routine")

    /* Set the initial number of bytes required */
    vlen_bufsize.size = 0;

    /* Call H5D_iterate with args, etc. */
    ret_value = H5D_iterate(&bogus, type_id, space, H5D_vlen_get_buf_size, &vlen_bufsize);

    /* Get the size if we succeeded */
    if(ret_value >= 0)
        *size = vlen_bufsize.size;

done:
    if(vlen_bufsize.fspace_id > 0) {
        if(H5I_dec_ref(vlen_bufsize.fspace_id, FALSE) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataspace")
    } /* end if */
    if(vlen_bufsize.mspace_id > 0) {
        if(H5I_dec_ref(vlen_bufsize.mspace_id, FALSE) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataspace")
    } /* end if */
    if(vlen_bufsize.fl_tbuf != NULL)
        vlen_bufsize.fl_tbuf = H5FL_BLK_FREE(vlen_fl_buf, vlen_bufsize.fl_tbuf);
    if(vlen_bufsize.vl_tbuf != NULL)
        vlen_bufsize.vl_tbuf = H5FL_BLK_FREE(vlen_vl_buf, vlen_bufsize.vl_tbuf);
    if(vlen_bufsize.xfer_pid > 0) {
        if(H5I_dec_ref(vlen_bufsize.xfer_pid, FALSE) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTDEC, FAIL, "unable to decrement ref count on property list")
    } /* end if */

    FUNC_LEAVE_API(ret_value)
}   /* end H5Dvlen_get_buf_size() */


/*-------------------------------------------------------------------------
 * Function:	H5Dset_extent
 *
 * Purpose:	Modifies the dimensions of a dataset.
 *		Can change to a smaller dimension.
 *
 * Return:	Non-negative on success, negative on failure
 *
 * Programmer:  Pedro Vicente, pvn@ncsa.uiuc.edu
 *              April 9, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dset_extent(hid_t dset_id, const hsize_t size[])
{
    H5D_t *dset;                /* Dataset for this operation */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(H5Dset_extent, FAIL)
    H5TRACE2("e", "i*h", dset_id, size);

    /* Check args */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")
    if(!size)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no size specified")

    /* Private function */
    if(H5D_set_extent(dset, size, H5AC_dxpl_id) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to set extend dataset")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Dset_extent() */

