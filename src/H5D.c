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
#include "H5CXprivate.h"        /* API Contexts                         */
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

/* Package initialization variable */
hbool_t H5_PKG_INIT_VAR = FALSE;

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
 *		allowed to derive new types, dataspaces, and creation
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

    FUNC_ENTER_API(FAIL)
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

    /* Verify access property list and set up collective metadata if appropriate */
    if(H5CX_set_apl(&dapl_id, H5P_CLS_DACC, loc_id, TRUE) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, H5I_INVALID_HID, "can't set access property list info")

    /* Create the new dataset & get its ID */
    if(NULL == (dset = H5D__create_named(&loc, name, type_id, space, lcpl_id, dcpl_id, dapl_id)))
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
 *		allowed to derive new types, dataspaces, and creation
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

    FUNC_ENTER_API(FAIL)
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

    /* Verify access property list and set up collective metadata if appropriate */
    if(H5CX_set_apl(&dapl_id, H5P_CLS_DACC, loc_id, TRUE) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, H5I_INVALID_HID, "can't set access property list info")

    /* build and open the new dataset */
    if(NULL == (dset = H5D__create(loc.oloc->file, type_id, space, dcpl_id, dapl_id)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to create dataset")

    /* Register the new dataset to get an ID for it */
    if((ret_value = H5I_register(H5I_DATASET, dset, TRUE)) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTREGISTER, FAIL, "unable to register dataset")

done:
    /* Release the dataset's object header, if it was created */
    if(dset) {
        H5O_loc_t *oloc;         /* Object location for dataset */

        /* Get the new dataset's object location */
        if(NULL == (oloc = H5D_oloc(dset)))
            HDONE_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to get object location of dataset")

        /* Decrement refcount on dataset's object header in memory */
        if(H5O_dec_rc_by_loc(oloc) < 0)
           HDONE_ERROR(H5E_DATASET, H5E_CANTDEC, FAIL, "unable to decrement refcount on newly created object")
    } /* end if */

    /* Cleanup on failure */
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
    H5G_loc_t   loc;                    /* Object location of group */
    hid_t       ret_value;

    FUNC_ENTER_API(FAIL)
    H5TRACE3("i", "i*si", loc_id, name, dapl_id);

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")

    /* Verify access property list and set up collective metadata if appropriate */
    if(H5CX_set_apl(&dapl_id, H5P_CLS_DACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, H5I_INVALID_HID, "can't set access property list info")

    /* Open the dataset */
    if(NULL == (dset = H5D__open_name(&loc, name, dapl_id)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTOPENOBJ, FAIL, "unable to open dataset")

    /* Register an atom for the dataset */
    if((ret_value = H5I_register(H5I_DATASET, dset, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "can't register dataset atom")

done:
    if(ret_value < 0)
        if(dset && H5D_close(dset) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataset")

    FUNC_LEAVE_API(ret_value)
} /* end H5Dopen2() */


/*-------------------------------------------------------------------------
 * Function:    H5Dclose
 *
 * Purpose:     Closes access to a dataset (DATASET_ID) and releases
 *              resources used by it. It is illegal to subsequently use that
 *              same dataset ID in calls to other dataset functions.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, December  4, 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dclose(hid_t dset_id)
{
    herr_t  ret_value = SUCCEED;    /* Return value                     */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", dset_id);

    /* Check args */
    if(NULL == H5I_object_verify(dset_id, H5I_DATASET))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    /*
     * Decrement the counter on the dataset.  It will be freed if the count
     * reaches zero.  
     */
    if(H5I_dec_app_ref_always_close(dset_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTDEC, FAIL, "can't decrement count on dataset ID")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Dclose() */


/*-------------------------------------------------------------------------
 * Function:	H5Dget_space
 *
 * Purpose:	Returns a copy of the file dataspace for a dataset.
 *
 * Return:	Success:	ID for a copy of the dataspace.  The data
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
    hid_t	ret_value;

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE1("i", "i", dset_id);

    /* Check args */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not a dataset")

    if((ret_value = H5D__get_space(dset)) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, H5I_INVALID_HID, "unable to get dataspace")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Dget_space() */


/*-------------------------------------------------------------------------
 * Function:	H5Dget_space_status
 *
 * Purpose:	Returns the status of dataspace allocation.
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

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*Ds", dset_id, allocation);

    /* Check arguments */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    /* Read dataspace address and return */
    if(H5D__get_space_status(dset, allocation) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to get space status")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Dget_space_status() */


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
    hid_t	ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("i", "i", dset_id);

    /* Check args */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    if((ret_value = H5D__get_type(dset)) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to get dataspace")

done:
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
    H5D_t	*dataset;               /* Dataset structure */
    hid_t	ret_value;    		/* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE1("i", "i", dset_id);

    /* Check args */
    if(NULL == (dataset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not a dataset")

    if((ret_value = H5D_get_create_plist(dataset)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, H5I_INVALID_HID, "Can't get creation plist")

done:
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
    hid_t           ret_value;      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("i", "i", dset_id);

    /* Check args */
    if (NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    if((ret_value = H5D_get_access_plist(dset)) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "Can't get access plist")

done:
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

    FUNC_ENTER_API(0)
    H5TRACE1("h", "i", dset_id);

    /* Check args */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, 0, "not a dataset")

    /* Set return value */
    if(H5D__get_storage_size(dset, &ret_value) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, 0, "can't get size of dataset's storage")

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

    FUNC_ENTER_API(HADDR_UNDEF)
    H5TRACE1("a", "i", dset_id);

    /* Check args */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, HADDR_UNDEF, "not a dataset")

    /* Set return value */
    ret_value = H5D__get_offset(dset);

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
    H5T_t *type;                /* Datatype */
    H5S_t *space;               /* Dataspace for iteration */
    H5S_sel_iter_op_t dset_op;  /* Operator for iteration */
    herr_t ret_value;           /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "*xiix*x", buf, type_id, space_id, op, operator_data);

    /* Check args */
    if(NULL == op)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid operator")
    if(NULL == buf)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid buffer")
    if(H5I_DATATYPE != H5I_get_type(type_id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid datatype")
    if(NULL == (type = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an valid base datatype")
    if(NULL == (space = (H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid dataspace")
    if(!(H5S_has_extent(space)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "dataspace does not have extent set")

    dset_op.op_type = H5S_SEL_ITER_OP_APP;
    dset_op.u.app_op.op = op;
    dset_op.u.app_op.type_id = type_id;

    ret_value = H5S_select_iterate(buf, type, space, &dset_op, operator_data);

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
H5Dvlen_reclaim(hid_t type_id, hid_t space_id, hid_t dxpl_id, void *buf)
{
    H5S_t *space;               /* Dataspace for iteration */
    herr_t ret_value;           /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "iii*x", type_id, space_id, dxpl_id, buf);

    /* Check args */
    if(H5I_DATATYPE != H5I_get_type(type_id) || buf == NULL)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid argument")
    if(NULL == (space = (H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid dataspace")
    if(!(H5S_has_extent(space)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "dataspace does not have extent set")

    /* Get the default dataset transfer property list if the user didn't provide one */
    if(H5P_DEFAULT == dxpl_id)
        dxpl_id = H5P_DATASET_XFER_DEFAULT;
    else
        if(TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not xfer parms")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Call internal routine */
    ret_value = H5D_vlen_reclaim(type_id, space, buf);

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
    H5D_vlen_bufsize_t vlen_bufsize = {0, 0, 0, 0, 0, 0};
    H5D_t *dset;                /* Dataset for operation */
    H5S_t *fspace = NULL;       /* Dataset's dataspace */
    H5S_t *mspace = NULL;       /* Memory dataspace */
    char bogus;                 /* bogus value to pass to H5Diterate() */
    H5S_t *space;               /* Dataspace for iteration */
    H5T_t *type;                /* Datatype */
    H5S_sel_iter_op_t dset_op;  /* Operator for iteration */
    herr_t ret_value;           /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "iii*h", dataset_id, type_id, space_id, size);

    /* Check args */
    if(H5I_DATASET != H5I_get_type(dataset_id) ||
            H5I_DATATYPE != H5I_get_type(type_id) || size == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid argument")
    if(NULL == (dset = (H5D_t *)H5I_object(dataset_id)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")
    if(NULL == (type = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an valid base datatype")
    if(NULL == (space = (H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid dataspace")
    if(!(H5S_has_extent(space)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "dataspace does not have extent set")

    /* Save the dataset */
    vlen_bufsize.dset = dset;

    /* Get a copy of the dataset's dataspace */
    if(NULL == (fspace = H5S_copy(dset->shared->space, FALSE, TRUE)))
	HGOTO_ERROR(H5E_DATASET, H5E_CANTCOPY, FAIL, "unable to get dataspace")
    vlen_bufsize.fspace = fspace;

    /* Create a scalar for the memory dataspace */
    if(NULL == (mspace = H5S_create(H5S_SCALAR)))
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCREATE, FAIL, "can't create dataspace")
    vlen_bufsize.mspace = mspace;

    /* Grab the temporary buffers required */
    if(NULL == (vlen_bufsize.fl_tbuf = H5FL_BLK_MALLOC(vlen_fl_buf, (size_t)1)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "no temporary buffers available")
    if(NULL == (vlen_bufsize.vl_tbuf = H5FL_BLK_MALLOC(vlen_vl_buf, (size_t)1)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "no temporary buffers available")

    /* Set the memory manager to the special allocation routine */
    if(H5CX_set_vlen_alloc_info(H5D__vlen_get_buf_size_alloc, &vlen_bufsize, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set VL data allocation routine")

    /* Set the initial number of bytes required */
    vlen_bufsize.size = 0;

    /* Call H5S_select_iterate with args, etc. */
    dset_op.op_type = H5S_SEL_ITER_OP_APP;
    dset_op.u.app_op.op = H5D__vlen_get_buf_size;
    dset_op.u.app_op.type_id = type_id;

    ret_value = H5S_select_iterate(&bogus, type, space, &dset_op, &vlen_bufsize);

    /* Get the size if we succeeded */
    if(ret_value >= 0)
        *size = vlen_bufsize.size;

done:
    if(fspace && H5S_close(fspace) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataspace")
    if(mspace && H5S_close(mspace) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataspace")
    if(vlen_bufsize.fl_tbuf != NULL)
        vlen_bufsize.fl_tbuf = H5FL_BLK_FREE(vlen_fl_buf, vlen_bufsize.fl_tbuf);
    if(vlen_bufsize.vl_tbuf != NULL)
        vlen_bufsize.vl_tbuf = H5FL_BLK_FREE(vlen_vl_buf, vlen_bufsize.vl_tbuf);

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

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*h", dset_id, size);

    /* Check args */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")
    if(!size)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no size specified")

    /* Set up collective metadata if appropriate */
    if(H5CX_set_loc(dset_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set collective metadata read info")

    /* Private function */
    if(H5D__set_extent(dset, size) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to set extend dataset")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Dset_extent() */


/*-------------------------------------------------------------------------
 * Function:    H5Dflush
 *
 * Purpose:     Flushes all buffers associated with a dataset.
 *
 * Return:      Non-negative on success, negative on failure
 *
 * Programmer:  Mike McGreevy
 *              May 19, 2010
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dflush(hid_t dset_id)
{
    H5D_t *dset;                /* Dataset for this operation */
    herr_t ret_value = SUCCEED; /* return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", dset_id);
    
    /* Check args */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    /* Set up collective metadata if appropriate */
    if(H5CX_set_loc(dset_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set collective metadata read info")

    /* Flush dataset information cached in memory */
    if(H5D__flush(dset, dset_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTFLUSH, FAIL, "unable to flush cached dataset info")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Dflush */


/*-------------------------------------------------------------------------
 * Function:    H5Drefresh
 *
 * Purpose:     Refreshes all buffers associated with a dataset.
 *
 * Return:      Non-negative on success, negative on failure
 *
 * Programmer:  Mike McGreevy
 *              July 21, 2010
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Drefresh(hid_t dset_id)
{
    H5D_t *dset;                   /* Dataset to refresh */
    herr_t ret_value = SUCCEED;    /* return value */
    
    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", dset_id);

    /* Check args */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    /* Set up collective metadata if appropriate */
    if(H5CX_set_loc(dset_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set collective metadata read info")

    /* Call private function to refresh the dataset object */
    if((H5D__refresh(dset_id, dset)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTLOAD, FAIL, "unable to refresh dataset")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Drefresh() */


/*-------------------------------------------------------------------------
 * Function:    H5Dformat_convert (Internal)
 *
 * Purpose:     For chunked: 
 *		  Convert the chunk indexing type to version 1 B-tree if not
 *		For compact/contiguous: 
 *		  Downgrade layout version to 3 if greater than 3
 *		For virtual: no conversion
 *
 * Return:      Non-negative on success, negative on failure
 *
 * Programmer:  Vailin Choi
 *              Feb 2015
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dformat_convert(hid_t dset_id)
{
    H5D_t *dset;                /* Dataset to refresh */
    herr_t ret_value = SUCCEED; /* return value */
    
    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", dset_id);

    /* Check args */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    /* Set up collective metadata if appropriate */
    if(H5CX_set_loc(dset_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set collective metadata read info")

    switch(dset->shared->layout.type) {
	case H5D_CHUNKED:
	    /* Convert the chunk indexing type to version 1 B-tree if not */
	    if(dset->shared->layout.u.chunk.idx_type != H5D_CHUNK_IDX_BTREE)
                if((H5D__format_convert(dset)) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTLOAD, FAIL, "unable to downgrade chunk indexing type for dataset")
	    break;

	case H5D_CONTIGUOUS:
	case H5D_COMPACT:
	    /* Downgrade the layout version to 3 if greater than 3 */
	    if(dset->shared->layout.version > H5O_LAYOUT_VERSION_DEFAULT)
                if((H5D__format_convert(dset)) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTLOAD, FAIL, "unable to downgrade layout version for dataset")
	    break;

	case H5D_VIRTUAL:
	    /* Nothing to do even though layout is version 4 */
	    break;

        case H5D_LAYOUT_ERROR:
        case H5D_NLAYOUTS:
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid dataset layout type")

	default: 
	    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "unknown dataset layout type")
    } /* end switch */

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Dformat_convert */


/*-------------------------------------------------------------------------
 * Function:    H5Dget_chunk_index_type (Internal)
 *
 * Purpose:     Retrieve a dataset's chunk indexing type
 *
 * Return:      Non-negative on success, negative on failure
 *
 * Programmer:  Vailin Choi
 *              Feb 2015
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dget_chunk_index_type(hid_t did, H5D_chunk_index_t *idx_type)
{
    H5D_t *dset;                /* Dataset to refresh */
    herr_t ret_value = SUCCEED; /* return value */
    
    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*Dk", did, idx_type);

    /* Check args */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(did, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    /* Should be a chunked dataset */
    if(dset->shared->layout.type != H5D_CHUNKED)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "dataset is not chunked")

    /* Get the chunk indexing type */
    if(idx_type)
        *idx_type = dset->shared->layout.u.chunk.idx_type;

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Dget_chunk_index_type() */


/*-------------------------------------------------------------------------
 * Function:    H5Dget_chunk_storage_size
 *
 * Purpose:     Returns the size of an allocated chunk.
 *
 * Return:	Non-negative on success, negative on failure
 *
 * Programmer:  Matthew Strong (GE Healthcare)
 *              20 October 2016
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dget_chunk_storage_size(hid_t dset_id, const hsize_t *offset, hsize_t *chunk_nbytes)
{
    H5D_t       *dset = NULL;
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "i*h*h", dset_id, offset, chunk_nbytes);

    /* Check arguments */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")
    if( NULL == offset )
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid argument (null)")
    if( NULL == chunk_nbytes )
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid argument (null)")

    if(H5D_CHUNKED != dset->shared->layout.type)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a chunked dataset")

    /* Call private function */
    if(H5D__get_chunk_storage_size(dset, offset, chunk_nbytes) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get storage size of chunk")

done:
    FUNC_LEAVE_API(ret_value);
} /* H5Dget_chunk_storage_size() */


/*-------------------------------------------------------------------------
 * Function:    H5Dget_num_chunks
 *
 * Purpose:     Retrieves the number of chunks that have nonempty intersection
 *              with a specified selection.
 *
 * Note:        Currently, this function only gets the number of all written
 *              chunks, regardless the dataspace.
 *
 * Parameters:
 *              hid_t dset_id;      IN: Chunked dataset ID
 *              hid_t fspace_id;    IN: File dataspace ID
 *              hsize_t *nchunks;   OUT:: Number of non-empty chunks
 *
 * Return:      Non-negative on success, negative on failure
 *
 * Programmer:  Binh-Minh Ribler
 *              August 2018 (HDFFV-10615)
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dget_num_chunks(hid_t dset_id, hid_t fspace_id, hsize_t *nchunks)
{
    H5D_t       *dset = NULL;
    const H5S_t *space;              /* Dataspace for dataset */
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "ii*h", dset_id, fspace_id, nchunks);

    /* Check arguments */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")
    if(NULL == (space = (const H5S_t *)H5I_object_verify(fspace_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace ID")
    if(NULL == nchunks)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid argument (null)")

    if(H5D_CHUNKED != dset->shared->layout.type)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a chunked dataset")

    /* Get the number of written chunks */
    if(H5D__get_num_chunks(dset, space, nchunks) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "error getting number of chunks")

done:
    FUNC_LEAVE_API(ret_value);
} /* H5Dget_num_chunks() */
 

/*-------------------------------------------------------------------------
 * Function:    H5Dget_chunk_info
 *
 * Purpose:     Retrieves information about a chunk specified by its index.
 *
 * Parameters:
 *              hid_t dset_id;          IN: Chunked dataset ID
 *              hid_t fspace_id;        IN: File dataspace ID
 *              hsize_t chk_idx;        IN: Index of allocated/written chunk
 *              hsize_t *offset         OUT: Offset coordinates of the chunk
 *              unsigned *filter_mask   OUT: Filter mask
 *              haddr_t *addr           OUT: Address of the chunk
 *              hsize_t *size           OUT: Size of the chunk
 *
 * Return:      Non-negative on success, negative on failure
 *
 * Programmer:  Binh-Minh Ribler
 *              August 2018 (HDFFV-10615)
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dget_chunk_info(hid_t dset_id, hid_t fspace_id, hsize_t chk_idx, hsize_t *offset, unsigned *filter_mask, haddr_t *addr, hsize_t *size)
{
    H5D_t       *dset = NULL;
    const H5S_t *space;              /* Dataspace for dataset */
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE7("e", "iih*h*Iu*a*h", dset_id, fspace_id, chk_idx, offset, filter_mask,
             addr, size);

    /* Check arguments */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset ID")
    if(NULL == (space = (const H5S_t *)H5I_object_verify(fspace_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace ID")
    if(NULL == offset && NULL == filter_mask && NULL == addr && NULL == size)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid arguments, must have at least one non-null output argument")

    if(H5D_CHUNKED != dset->shared->layout.type)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a chunked dataset")

    /* Call private function to get the chunk info given the chunk's index */
    if(H5D__get_chunk_info(dset, space, chk_idx, offset, filter_mask, addr, size) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get chunk info")

done:
    FUNC_LEAVE_API(ret_value);
} /* H5Dget_chunk_info() */


/*-------------------------------------------------------------------------
 * Function:    H5Dget_chunk_info_by_coord
 *
 * Purpose:     Retrieves information about a chunk specified by its offset
 *              coordinates.
 *
 * Parameters:
 *              hid_t dset_id           IN: Chunked dataset ID
 *              hsize_t *offset         IN: Offset coordinates of the chunk
 *              unsigned *filter_mask   OUT: Filter mask
 *              haddr_t *addr           OUT: Address of the chunk
 *              hsize_t *size           OUT: Size of the chunk
 *
 * Return:      Non-negative on success, negative on failure
 *
 * Programmer:  Binh-Minh Ribler
 *              August 2018 (HDFFV-10615)
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dget_chunk_info_by_coord(hid_t dset_id, const hsize_t *offset, unsigned *filter_mask, haddr_t *addr, hsize_t *size)
{
    H5D_t      *dset = NULL;
    herr_t      ret_value = SUCCEED;            /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "i*h*Iu*a*h", dset_id, offset, filter_mask, addr, size);

    /* Check arguments */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")
    if(NULL == offset)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid argument (null)")
    if(NULL == filter_mask && NULL == addr && NULL == size)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid arguments, must have at least one non-null output argument")

    if(H5D_CHUNKED != dset->shared->layout.type)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a chunked dataset")

    /* Internal function to get the chunk info */
    if (H5D__get_chunk_info_by_coord(dset, offset, filter_mask, addr, size) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "can't get chunk info")
done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Dget_chunk_info_by_coord() */
