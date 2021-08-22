/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/****************/
/* Module Setup */
/****************/

#include "H5Fmodule.h" /* This source code file is part of the H5F module */

/***********/
/* Headers */
/***********/
#include "H5private.h"   /* Generic Functions                        */
#include "H5ACprivate.h" /* Metadata cache                           */
#include "H5CXprivate.h" /* API Contexts                             */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5Fpkg.h"      /* File access                              */
#include "H5FLprivate.h" /* Free lists                               */
#include "H5Iprivate.h"  /* IDs                                      */
#include "H5Pprivate.h"  /* Property lists                           */
#include "H5VLprivate.h" /* Virtual Object Layer                     */

#include "H5VLnative_private.h" /* Native VOL connector                     */

/****************/
/* Local Macros */
/****************/

/******************/
/* Local Typedefs */
/******************/

/* User data for traversal routine to get ID counts */
typedef struct {
    size_t   obj_count; /* Number of objects counted so far */
    unsigned types;     /* Types of objects to be counted */
} H5F_trav_obj_cnt_t;

/* User data for traversal routine to get ID lists */
typedef struct {
    size_t max_objs;  /* Maximum # of IDs to record */
    hid_t *oid_list;  /* Array of recorded IDs*/
    size_t obj_count; /* Number of objects counted so far */
} H5F_trav_obj_ids_t;

/********************/
/* Package Typedefs */
/********************/

/********************/
/* Local Prototypes */
/********************/

/* Callback for getting object counts in a file */
static int H5F__get_all_count_cb(void H5_ATTR_UNUSED *obj_ptr, hid_t H5_ATTR_UNUSED obj_id, void *key);

/* Callback for getting IDs for open objects in a file */
static int H5F__get_all_ids_cb(void H5_ATTR_UNUSED *obj_ptr, hid_t obj_id, void *key);

/*********************/
/* Package Variables */
/*********************/

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/

/* Declare a free list to manage the H5VL_t struct */
H5FL_EXTERN(H5VL_t);

/* Declare a free list to manage the H5VL_object_t struct */
H5FL_EXTERN(H5VL_object_t);

/*-------------------------------------------------------------------------
 * Function:    H5Fget_create_plist
 *
 * Purpose:     Get an atom for a copy of the file-creation property list for
 *              this file. This function returns an atom with a copy of the
 *              properties used to create a file.
 *
 * Return:      Success:    Object ID for a copy of the file creation
 *                          property list.
 *
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Fget_create_plist(hid_t file_id)
{
    H5VL_object_t *vol_obj;                     /* File info */
    hid_t          ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE1("i", "i", file_id);

    /* check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(file_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid file identifier")

    /* Retrieve the file creation property list */
    if (H5VL_file_get(vol_obj, H5VL_FILE_GET_FCPL, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, &ret_value) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTCOPY, H5I_INVALID_HID, "unable to retrieve file creation properties")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_create_plist() */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_access_plist
 *
 * Purpose:     Returns a copy of the file access property list of the
 *              specified file.
 *
 *              NOTE: Make sure that, if you are going to overwrite
 *              information in the copied property list that was
 *              previously opened and assigned to the property list, then
 *              you must close it before overwriting the values.
 *
 * Return:      Success:    Object ID for a copy of the file access
 *                          property list.
 *
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Fget_access_plist(hid_t file_id)
{
    H5VL_object_t *vol_obj;                     /* File info */
    hid_t          ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE1("i", "i", file_id);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(file_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid file identifier")

    /* Retrieve the file's access property list */
    if (H5VL_file_get(vol_obj, H5VL_FILE_GET_FAPL, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, &ret_value) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, H5I_INVALID_HID, "can't get file access property list")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_access_plist() */

/*-------------------------------------------------------------------------
 * Function:    H5F__get_all_count_cb
 *
 * Purpose:     Get counter of all object types currently open.
 *
 * Return:      Success:    H5_ITER_CONT or H5_ITER_STOP
 *
 *              Failure:    H5_ITER_ERROR
 *
 *-------------------------------------------------------------------------
 */
static int
H5F__get_all_count_cb(void H5_ATTR_UNUSED *obj_ptr, hid_t H5_ATTR_UNUSED obj_id, void *key)
{
    H5F_trav_obj_cnt_t *udata     = (H5F_trav_obj_cnt_t *)key;
    int                 ret_value = H5_ITER_CONT; /* Return value */

    FUNC_ENTER_STATIC_NOERR

    udata->obj_count++;

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5F_get_all_count_cb */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_obj_count
 *
 * Purpose:     Public function returning the number of opened object IDs
 *              (files, datasets, groups and datatypes) in the same file.
 *
 * Return:      Success:    The number of opened object IDs
 *
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5Fget_obj_count(hid_t file_id, unsigned types)
{
    ssize_t ret_value = 0; /* Return value */

    FUNC_ENTER_API((-1))
    H5TRACE2("Zs", "iIu", file_id, types);

    /* Check arguments */
    if (0 == (types & H5F_OBJ_ALL))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, (-1), "not an object type")

    /* Perform the query */
    /* If the 'special' ID wasn't passed in, just make a normal call to
     * count the IDs in the file.
     */
    if (file_id != (hid_t)H5F_OBJ_ALL) {
        H5VL_object_t *vol_obj;

        /* Get the file object */
        if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, (-1), "not a file id")

        /* Get the count */
        if (H5VL_file_get(vol_obj, H5VL_FILE_GET_OBJ_COUNT, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, types,
                          &ret_value) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTGET, (-1), "unable to get object count in file(s)")
    }
    /* If we passed in the 'special' ID, get the count for everything open in the
     * library, iterating over all open files and getting the object count for each.
     */
    else {
        H5F_trav_obj_cnt_t udata;

        /* Set up callback context */
        udata.types     = types | H5F_OBJ_LOCAL;
        udata.obj_count = 0;

        if (types & H5F_OBJ_FILE)
            if (H5I_iterate(H5I_FILE, H5F__get_all_count_cb, &udata, TRUE) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_BADITER, (-1), "iteration over file IDs failed")
        if (types & H5F_OBJ_DATASET)
            if (H5I_iterate(H5I_DATASET, H5F__get_all_count_cb, &udata, TRUE) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_BADITER, (-1), "iteration over dataset IDs failed")
        if (types & H5F_OBJ_GROUP)
            if (H5I_iterate(H5I_GROUP, H5F__get_all_count_cb, &udata, TRUE) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_BADITER, (-1), "iteration over group IDs failed")
        if (types & H5F_OBJ_DATATYPE)
            if (H5I_iterate(H5I_DATATYPE, H5F__get_all_count_cb, &udata, TRUE) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_BADITER, (-1), "iteration over datatype IDs failed")
        if (types & H5F_OBJ_ATTR)
            if (H5I_iterate(H5I_ATTR, H5F__get_all_count_cb, &udata, TRUE) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_BADITER, (-1), "iteration over attribute IDs failed")

        /* Set return value */
        ret_value = (ssize_t)udata.obj_count;
    } /* end else */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_obj_count() */

/*-------------------------------------------------------------------------
 * Function:    H5F__get_all_ids_cb
 *
 * Purpose:     Get IDs of all currently open objects of a given type.
 *
 * Return:      Success:    H5_ITER_CONT or H5_ITER_STOP
 *
 *              Failure:    H5_ITER_ERROR
 *
 *-------------------------------------------------------------------------
 */
static int
H5F__get_all_ids_cb(void H5_ATTR_UNUSED *obj_ptr, hid_t obj_id, void *key)
{
    H5F_trav_obj_ids_t *udata     = (H5F_trav_obj_ids_t *)key;
    int                 ret_value = H5_ITER_CONT; /* Return value */

    FUNC_ENTER_STATIC_NOERR

    if (udata->obj_count >= udata->max_objs)
        HGOTO_DONE(H5_ITER_STOP);

    /* Add the ID to the array */
    udata->oid_list[udata->obj_count] = obj_id;
    udata->obj_count++;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5F__get_all_ids_cb */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_object_ids
 *
 * Purpose:     Public function to return a list of opened object IDs.
 *
 * NOTE:        Type mismatch - You can ask for more objects than can be
 *              returned.
 *
 * NOTE:        Currently, the IDs' ref counts are not incremented.  Is this
 *              intentional and documented?
 *
 * Return:      Success:    The number of IDs in oid_list
 *
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5Fget_obj_ids(hid_t file_id, unsigned types, size_t max_objs, hid_t *oid_list)
{
    ssize_t ret_value = 0; /* Return value */

    FUNC_ENTER_API((-1))
    H5TRACE4("Zs", "iIuz*i", file_id, types, max_objs, oid_list);

    /* Check arguments */
    if (0 == (types & H5F_OBJ_ALL))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, (-1), "not an object type")
    if (!oid_list)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, (-1), "object ID list cannot be NULL")

    /* Perform the query */
    /* If the 'special' ID wasn't passed in, just make a normal VOL call to
     * get the IDs from the file.
     */
    if (file_id != (hid_t)H5F_OBJ_ALL) {
        H5VL_object_t *vol_obj;

        /* get the file object */
        if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, (-1), "invalid file identifier")

        /* Get the IDs */
        if (H5VL_file_get(vol_obj, H5VL_FILE_GET_OBJ_IDS, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, types,
                          max_objs, oid_list, &ret_value) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTGET, (-1), "unable to get object ids in file(s)")
    } /* end if */
    /* If we passed in the 'special' ID, get the count for everything open in the
     * library, iterating over all open files and getting the object count for each.
     *
     * XXX: Note that the RM states that passing in a negative value for max_objs
     *      gets you all the objects. This technically works, but is clearly wrong
     *      behavior since max_objs is an unsigned type.
     */
    else {
        H5F_trav_obj_ids_t udata;

        /* Set up callback context */
        udata.max_objs  = max_objs;
        udata.oid_list  = oid_list;
        udata.obj_count = 0;

        if (types & H5F_OBJ_FILE)
            if (H5I_iterate(H5I_FILE, H5F__get_all_ids_cb, &udata, TRUE) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_BADITER, (-1), "iteration over file IDs failed")
        if (types & H5F_OBJ_DATASET)
            if (H5I_iterate(H5I_DATASET, H5F__get_all_ids_cb, &udata, TRUE) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_BADITER, (-1), "iteration over dataset IDs failed")
        if (types & H5F_OBJ_GROUP)
            if (H5I_iterate(H5I_GROUP, H5F__get_all_ids_cb, &udata, TRUE) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_BADITER, (-1), "iteration over group IDs failed")
        if (types & H5F_OBJ_DATATYPE)
            if (H5I_iterate(H5I_DATATYPE, H5F__get_all_ids_cb, &udata, TRUE) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_BADITER, (-1), "iteration over datatype IDs failed")
        if (types & H5F_OBJ_ATTR)
            if (H5I_iterate(H5I_ATTR, H5F__get_all_ids_cb, &udata, TRUE) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_BADITER, (-1), "iteration over attribute IDs failed")

        /* Set return value */
        ret_value = (ssize_t)udata.obj_count;
    } /* end else */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_obj_ids() */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_vfd_handle
 *
 * Purpose:     Returns a pointer to the file handle of the low-level file
 *              driver.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_vfd_handle(hid_t file_id, hid_t fapl_id, void **file_handle)
{
    H5VL_object_t *vol_obj;             /* File info */
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "ii**x", file_id, fapl_id, file_handle);

    /* Check args */
    if (!file_handle)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file handle pointer")

    /* Get the file object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(file_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")

    /* Retrieve the VFD handle for the file */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_GET_VFD_HANDLE, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL, file_handle, fapl_id) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get VFD handle")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_vfd_handle() */

/*-------------------------------------------------------------------------
 * Function:    H5Fis_accessible
 *
 * Purpose:     Check if the file can be opened with the given fapl.
 *
 * Return:      Success:    TRUE/FALSE
 *              Failure:    -1 (includes file does not exist)
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Fis_accessible(const char *filename, hid_t fapl_id)
{
    htri_t ret_value; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("t", "*si", filename, fapl_id);

    /* Check args */
    if (!filename || !*filename)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "no file name specified")

    /* Check the file access property list */
    if (H5P_DEFAULT == fapl_id)
        fapl_id = H5P_FILE_ACCESS_DEFAULT;
    else if (TRUE != H5P_isa_class(fapl_id, H5P_FILE_ACCESS))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not file access property list")

    /* Check if file is accessible */
    if (H5VL_file_specific(NULL, H5VL_FILE_IS_ACCESSIBLE, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, fapl_id,
                           filename, &ret_value) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_NOTHDF5, FAIL, "unable to determine if file is accessible as HDF5")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fis_accessible() */

/*-------------------------------------------------------------------------
 * Function:    H5Fcreate
 *
 * Purpose:     This is the primary function for creating HDF5 files . The
 *              flags parameter determines whether an existing file will be
 *              overwritten or not.  All newly created files are opened for
 *              both reading and writing.  All flags may be combined with the
 *              bit-wise OR operator (`|') to change the behavior of the file
 *              create call.
 *
 *              The more complex behaviors of a file's creation and access
 *              are controlled through the file-creation and file-access
 *              property lists.  The value of H5P_DEFAULT for a template
 *              value indicates that the library should use the default
 *              values for the appropriate template.
 *
 * See also:    H5Fpublic.h for the list of supported flags. H5Ppublic.h for
 *              the list of file creation and file access properties.
 *
 * Return:      Success:    A file ID
 *              Failure:    H5I_INVALID_HID
 *-------------------------------------------------------------------------
 */
hid_t
H5Fcreate(const char *filename, unsigned flags, hid_t fcpl_id, hid_t fapl_id)
{
    void *                new_file = NULL; /* File struct for new file                 */
    H5P_genplist_t *      plist;           /* Property list pointer                    */
    H5VL_connector_prop_t connector_prop;  /* Property for VOL connector ID & info     */
    H5VL_object_t *       vol_obj = NULL;  /* VOL object for file                      */
    hbool_t               supported;       /* Whether 'post open' operation is supported by VOL connector */
    hid_t                 ret_value;       /* return value                             */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE4("i", "*sIuii", filename, flags, fcpl_id, fapl_id);

    /* Check/fix arguments */
    if (!filename || !*filename)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "invalid file name")

    /* In this routine, we only accept the following flags:
     *          H5F_ACC_EXCL, H5F_ACC_TRUNC and H5F_ACC_SWMR_WRITE
     */
    if (flags & ~(H5F_ACC_EXCL | H5F_ACC_TRUNC | H5F_ACC_SWMR_WRITE))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "invalid flags")

    /* The H5F_ACC_EXCL and H5F_ACC_TRUNC flags are mutually exclusive */
    if ((flags & H5F_ACC_EXCL) && (flags & H5F_ACC_TRUNC))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "mutually exclusive flags for file creation")

    /* Check file creation property list */
    if (H5P_DEFAULT == fcpl_id)
        fcpl_id = H5P_FILE_CREATE_DEFAULT;
    else if (TRUE != H5P_isa_class(fcpl_id, H5P_FILE_CREATE))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not file create property list")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&fapl_id, H5P_CLS_FACC, H5I_INVALID_HID, TRUE) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, H5I_INVALID_HID, "can't set access property list info")

    /* Get the VOL info from the fapl */
    if (NULL == (plist = (H5P_genplist_t *)H5I_object(fapl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not a file access property list")
    if (H5P_peek(plist, H5F_ACS_VOL_CONN_NAME, &connector_prop) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, H5I_INVALID_HID, "can't get VOL connector info")

    /* Stash a copy of the "top-level" connector property, before any pass-through
     *  connectors modify or unwrap it.
     */
    if (H5CX_set_vol_connector_prop(&connector_prop) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, H5I_INVALID_HID, "can't set VOL connector info in API context")

    /* Adjust bit flags by turning on the creation bit and making sure that
     * the EXCL or TRUNC bit is set.  All newly-created files are opened for
     * reading and writing.
     */
    if (0 == (flags & (H5F_ACC_EXCL | H5F_ACC_TRUNC)))
        flags |= H5F_ACC_EXCL; /*default*/
    flags |= H5F_ACC_RDWR | H5F_ACC_CREAT;

    /* Create a new file or truncate an existing file through the VOL */
    if (NULL == (new_file = H5VL_file_create(&connector_prop, filename, flags, fcpl_id, fapl_id,
                                             H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL)))
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, H5I_INVALID_HID, "unable to create file")

    /* Get an atom for the file */
    if ((ret_value = H5VL_register_using_vol_id(H5I_FILE, new_file, connector_prop.connector_id, TRUE)) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to atomize file handle")

    /* Get the file object */
    if (NULL == (vol_obj = H5VL_vol_object(ret_value)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid object identifier")

    /* Make the 'post open' callback */
    supported = FALSE;
    if (H5VL_introspect_opt_query(vol_obj, H5VL_SUBCLS_FILE, H5VL_NATIVE_FILE_POST_OPEN, &supported) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, H5I_INVALID_HID, "can't check for 'post open' operation")
    if (supported)
        if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_POST_OPEN, H5P_DATASET_XFER_DEFAULT,
                               H5_REQUEST_NULL) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, H5I_INVALID_HID, "unable to make file 'post open' callback")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fcreate() */

/*-------------------------------------------------------------------------
 * Function:    H5Fopen
 *
 * Purpose:     This is the primary function for accessing existing HDF5
 *              files.  The FLAGS argument determines whether writing to an
 *              existing file will be allowed or not.  All flags may be
 *              combined with the bit-wise OR operator (`|') to change the
 *              behavior of the file open call.  The more complex behaviors
 *              of a file's access are controlled through the file-access
 *              property list.
 *
 * See Also:    H5Fpublic.h for a list of possible values for FLAGS.
 *
 * Return:      Success:    A file ID
 *              Failure:    H5I_INVALID_HID
 *-------------------------------------------------------------------------
 */
hid_t
H5Fopen(const char *filename, unsigned flags, hid_t fapl_id)
{
    void *                new_file = NULL; /* File struct for new file                 */
    H5P_genplist_t *      plist;           /* Property list pointer                    */
    H5VL_connector_prop_t connector_prop;  /* Property for VOL connector ID & info     */
    H5VL_object_t *       vol_obj = NULL;  /* VOL object for file                      */
    hbool_t               supported;       /* Whether 'post open' operation is supported by VOL connector */
    hid_t                 ret_value;       /* Return value                             */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE3("i", "*sIui", filename, flags, fapl_id);

    /* Check arguments */
    if (!filename || !*filename)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "invalid file name")
    /* Reject undefined flags (~H5F_ACC_PUBLIC_FLAGS) and the H5F_ACC_TRUNC & H5F_ACC_EXCL flags */
    if ((flags & ~H5F_ACC_PUBLIC_FLAGS) || (flags & H5F_ACC_TRUNC) || (flags & H5F_ACC_EXCL))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "invalid file open flags")
    /* XXX (VOL MERGE): Might want to move SWMR flag checks to H5F_open() */
    /* Asking for SWMR write access on a read-only file is invalid */
    if ((flags & H5F_ACC_SWMR_WRITE) && 0 == (flags & H5F_ACC_RDWR))
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, H5I_INVALID_HID,
                    "SWMR write access on a file open for read-only access is not allowed")
    /* Asking for SWMR read access on a non-read-only file is invalid */
    if ((flags & H5F_ACC_SWMR_READ) && (flags & H5F_ACC_RDWR))
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, H5I_INVALID_HID,
                    "SWMR read access on a file open for read-write access is not allowed")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&fapl_id, H5P_CLS_FACC, H5I_INVALID_HID, TRUE) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, H5I_INVALID_HID, "can't set access property list info")

    /* Get the VOL info from the fapl */
    if (NULL == (plist = (H5P_genplist_t *)H5I_object(fapl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not a file access property list")
    if (H5P_peek(plist, H5F_ACS_VOL_CONN_NAME, &connector_prop) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, H5I_INVALID_HID, "can't get VOL connector info")

    /* Stash a copy of the "top-level" connector property, before any pass-through
     *  connectors modify or unwrap it.
     */
    if (H5CX_set_vol_connector_prop(&connector_prop) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, H5I_INVALID_HID, "can't set VOL connector info in API context")

    /* Open the file through the VOL layer */
    if (NULL == (new_file = H5VL_file_open(&connector_prop, filename, flags, fapl_id,
                                           H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL)))
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, H5I_INVALID_HID, "unable to open file")

    /* Get an ID for the file */
    if ((ret_value = H5VL_register_using_vol_id(H5I_FILE, new_file, connector_prop.connector_id, TRUE)) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to atomize file handle")

    /* Get the file object */
    if (NULL == (vol_obj = H5VL_vol_object(ret_value)))
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, H5I_INVALID_HID, "invalid object identifier")

    /* Make the 'post open' callback */
    supported = FALSE;
    if (H5VL_introspect_opt_query(vol_obj, H5VL_SUBCLS_FILE, H5VL_NATIVE_FILE_POST_OPEN, &supported) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, H5I_INVALID_HID, "can't check for 'post open' operation")
    if (supported)
        if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_POST_OPEN, H5P_DATASET_XFER_DEFAULT,
                               H5_REQUEST_NULL) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, H5I_INVALID_HID, "unable to make file 'post open' callback")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fopen() */

/*-------------------------------------------------------------------------
 * Function:    H5Fflush
 *
 * Purpose:     Flushes all outstanding buffers of a file to disk but does
 *              not remove them from the cache.  The OBJECT_ID can be a file,
 *              dataset, group, attribute, or named data type.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fflush(hid_t object_id, H5F_scope_t scope)
{
    H5VL_object_t *vol_obj = NULL;      /* Object info      */
    H5I_type_t     obj_type;            /* Type of object   */
    herr_t         ret_value = SUCCEED; /* Return value     */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "iFs", object_id, scope);

    /* Get the type of object we're flushing + sanity check */
    obj_type = H5I_get_type(object_id);
    if (H5I_FILE != obj_type && H5I_GROUP != obj_type && H5I_DATATYPE != obj_type &&
        H5I_DATASET != obj_type && H5I_ATTR != obj_type)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

    /* Get the file object */
    if (NULL == (vol_obj = H5VL_vol_object(object_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid object identifier")

    /* Flush the object */
    if (H5VL_file_specific(vol_obj, H5VL_FILE_FLUSH, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, (int)obj_type,
                           (int)scope) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTFLUSH, FAIL, "unable to flush file")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fflush() */

/*-------------------------------------------------------------------------
 * Function:    H5Fclose
 *
 * Purpose:     This function closes the file specified by FILE_ID by
 *              flushing all data to storage, and terminating access to the
 *              file through FILE_ID.  If objects (e.g., datasets, groups,
 *              etc.) are open in the file then the underlying storage is not
 *              closed until those objects are closed; however, all data for
 *              the file and the open objects is flushed.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fclose(hid_t file_id)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* Check arguments */
    if (H5I_FILE != H5I_get_type(file_id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file ID")

    /* Close the file */
    if (H5I_dec_app_ref(file_id) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTCLOSEFILE, FAIL, "decrementing file ID failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fclose() */

/*-------------------------------------------------------------------------
 * Function:    H5Fdelete
 *
 * Purpose:     Deletes an HDF5 file.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Fdelete(const char *filename, hid_t fapl_id)
{
    H5P_genplist_t *      plist;          /* Property list pointer */
    H5VL_connector_prop_t connector_prop; /* Property for VOL connector ID & info */
    htri_t                is_hdf5   = FAIL;
    herr_t                ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "*si", filename, fapl_id);

    /* Check args */
    if (!filename || !*filename)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "no file name specified")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&fapl_id, H5P_CLS_FACC, H5I_INVALID_HID, TRUE) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, H5I_INVALID_HID, "can't set access property list info")

    /* Get the VOL info from the fapl */
    if (NULL == (plist = (H5P_genplist_t *)H5I_object_verify(fapl_id, H5I_GENPROP_LST)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not a property list")
    if (H5P_peek(plist, H5F_ACS_VOL_CONN_NAME, &connector_prop) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, H5I_INVALID_HID, "can't get VOL connector info")

    /* Stash a copy of the "top-level" connector property, before any pass-through
     *  connectors modify or unwrap it.
     */
    if (H5CX_set_vol_connector_prop(&connector_prop) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, H5I_INVALID_HID, "can't set VOL connector info in API context")

    /* Make sure this is HDF5 storage for this VOL connector */
    if (H5VL_file_specific(NULL, H5VL_FILE_IS_ACCESSIBLE, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, fapl_id,
                           filename, &is_hdf5) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_NOTHDF5, FAIL, "unable to determine if file is accessible as HDF5")
    if (!is_hdf5)
        HGOTO_ERROR(H5E_FILE, H5E_NOTHDF5, FAIL, "not an HDF5 file")

    /* Delete the file */
    if (H5VL_file_specific(NULL, H5VL_FILE_DELETE, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, fapl_id,
                           filename, &ret_value) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTDELETEFILE, FAIL, "unable to delete the file")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fdelete() */

/*-------------------------------------------------------------------------
 * Function:    H5Freopen
 *
 * Purpose:     Reopen a file.  The new file handle which is returned points
 *              to the same file as the specified file handle.  Both handles
 *              share caches and other information.  The only difference
 *              between the handles is that the new handle is not mounted
 *              anywhere and no files are mounted on it.
 *
 * Return:      Success:    New file ID
 *
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Freopen(hid_t file_id)
{
    void *         file    = NULL; /* File struct for new file */
    H5VL_object_t *vol_obj = NULL; /* VOL object for file                      */
    hbool_t        supported;      /* Whether 'post open' operation is supported by VOL connector */
    hid_t          ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE1("i", "i", file_id);

    /* Get the file object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid file identifier")

    /* Reopen the file */
    if (H5VL_file_specific(vol_obj, H5VL_FILE_REOPEN, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, &file) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, H5I_INVALID_HID, "unable to reopen file via the VOL connector")

    /* Make sure that worked */
    if (NULL == file)
        HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, H5I_INVALID_HID, "unable to reopen file")

    /* Get an atom for the file */
    if ((ret_value = H5VL_register(H5I_FILE, file, vol_obj->connector, TRUE)) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to atomize file handle")

    /* Get the file object */
    if (NULL == (vol_obj = H5VL_vol_object(ret_value)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid object identifier")

    /* Make the 'post open' callback */
    supported = FALSE;
    if (H5VL_introspect_opt_query(vol_obj, H5VL_SUBCLS_FILE, H5VL_NATIVE_FILE_POST_OPEN, &supported) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, H5I_INVALID_HID, "can't check for 'post open' operation")
    if (supported)
        if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_POST_OPEN, H5P_DATASET_XFER_DEFAULT,
                               H5_REQUEST_NULL) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, H5I_INVALID_HID, "unable to make file 'post open' callback")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Freopen() */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_intent
 *
 * Purpose:     Public API to retrieve the file's 'intent' flags passed
 *              during H5Fopen()
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_intent(hid_t file_id, unsigned *intent_flags)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*Iu", file_id, intent_flags);

    /* If no intent flags were passed in, exit quietly */
    if (intent_flags) {
        H5VL_object_t *vol_obj; /* File info */

        /* Get the internal file structure */
        if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(file_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")

        /* Get the flags */
        if ((ret_value = H5VL_file_get(vol_obj, H5VL_FILE_GET_INTENT, H5P_DATASET_XFER_DEFAULT,
                                       H5_REQUEST_NULL, intent_flags)) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get file's intent flags")
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_intent() */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_fileno
 *
 * Purpose:     Public API to retrieve the file's 'file number' that uniquely
 *              identifies each open file.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_fileno(hid_t file_id, unsigned long *fnumber)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*Ul", file_id, fnumber);

    /* If no fnumber pointer was passed in, exit quietly */
    if (fnumber) {
        H5VL_object_t *vol_obj; /* File info */

        /* Get the internal file structure */
        if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(file_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")

        /* Get the flags */
        if ((ret_value = H5VL_file_get(vol_obj, H5VL_FILE_GET_FILENO, H5P_DATASET_XFER_DEFAULT,
                                       H5_REQUEST_NULL, fnumber)) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get file's 'file number'")
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_fileno() */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_freespace
 *
 * Purpose:     Retrieves the amount of free space in the file.
 *
 * Return:      Success:    Amount of free space for type
 *              Failure:    -1
 *-------------------------------------------------------------------------
 */
hssize_t
H5Fget_freespace(hid_t file_id)
{
    H5VL_object_t *vol_obj = NULL;
    hssize_t       ret_value; /* Return value */

    FUNC_ENTER_API((-1))
    H5TRACE1("Hs", "i", file_id);

    /* Get the file object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(file_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, (-1), "invalid file identifier")

    /* Get the amount of free space in the file */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_GET_FREE_SPACE, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL, &ret_value) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, (-1), "unable to get file free space")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_freespace() */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_filesize
 *
 * Purpose:     Retrieves the file size of the HDF5 file. This function
 *              is called after an existing file is opened in order
 *              to learn the true size of the underlying file.
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_filesize(hid_t file_id, hsize_t *size)
{
    H5VL_object_t *vol_obj;             /* File info */
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*h", file_id, size);

    /* Check args */
    if (!size)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "size parameter cannot be NULL")
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    /* Get the file size */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_GET_SIZE, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL,
                           size) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get file size")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_filesize() */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_file_image
 *
 * Purpose:     If a buffer is provided (via the buf_ptr argument) and is
 *              big enough (size in buf_len argument), load *buf_ptr with
 *              an image of the open file whose ID is provided in the
 *              file_id parameter, and return the number of bytes copied
 *              to the buffer.
 *
 *              If the buffer exists, but is too small to contain an image
 *              of the indicated file, return a negative number.
 *
 *              Finally, if no buffer is provided, return the size of the
 *              buffer needed.  This value is simply the eoa of the target
 *              file.
 *
 *              Note that any user block is skipped.
 *
 *              Also note that the function may not be used on files
 *              opened with either the split/multi file driver or the
 *              family file driver.
 *
 *              In the former case, the sparse address space makes the
 *              get file image operation impractical, due to the size of
 *              the image typically required.
 *
 *              In the case of the family file driver, the problem is
 *              the driver message in the super block, which will prevent
 *              the image being opened with any driver other than the
 *              family file driver -- which negates the purpose of the
 *              operation.  This can be fixed, but no resources for
 *              this now.
 *
 * Return:      Success:    Bytes copied / number of bytes needed
 *              Failure:    -1
 *-------------------------------------------------------------------------
 */
ssize_t
H5Fget_file_image(hid_t file_id, void *buf, size_t buf_len)
{
    H5VL_object_t *vol_obj;   /* File object for file ID  */
    ssize_t        ret_value; /* Return value             */

    FUNC_ENTER_API((-1))
    H5TRACE3("Zs", "i*xz", file_id, buf, buf_len);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, (-1), "not a file ID")

    /* Get the file image */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_GET_FILE_IMAGE, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL, buf, &ret_value, buf_len) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, (-1), "unable to get file image")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fget_file_image() */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_mdc_config
 *
 * Purpose:     Retrieves the current automatic cache resize configuration
 *              from the metadata cache, and return it in *config_ptr.
 *
 *              Note that the version field of *config_Ptr must be correctly
 *              filled in by the caller.  This allows us to adapt for
 *              obsolete versions of the structure.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_mdc_config(hid_t file_id, H5AC_cache_config_t *config)
{
    H5VL_object_t *vol_obj   = NULL;
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*x", file_id, config);

    /* Check args */
    if ((NULL == config) || (config->version != H5AC__CURR_CACHE_CONFIG_VERSION))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "Bad configptr")

    /* Get the file object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(file_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")

    /* Get the metadata cache configuration */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_GET_MDC_CONF, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL,
                           config) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get metadata cache configuration")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fget_mdc_config() */

/*-------------------------------------------------------------------------
 * Function:    H5Fset_mdc_config
 *
 * Purpose:     Sets the current metadata cache automatic resize
 *              configuration, using the contents of the instance of
 *              H5AC_cache_config_t pointed to by config_ptr.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fset_mdc_config(hid_t file_id, H5AC_cache_config_t *config_ptr)
{
    H5VL_object_t *vol_obj   = NULL;
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*x", file_id, config_ptr);

    /* Get the file object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(file_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")

    /* Set the metadata cache configuration  */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_SET_MDC_CONFIG, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL, config_ptr) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set metadata cache configuration")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fset_mdc_config() */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_mdc_hit_rate
 *
 * Purpose:     Retrieves the current hit rate from the metadata cache.
 *              This rate is the overall hit rate since the last time
 *              the hit rate statistics were reset either manually or
 *              automatically.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_mdc_hit_rate(hid_t file_id, double *hit_rate)
{
    H5VL_object_t *vol_obj;
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*d", file_id, hit_rate);

    /* Check args */
    if (NULL == hit_rate)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL hit rate pointer")
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    /* Get the current hit rate */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_GET_MDC_HR, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL,
                           hit_rate) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get MDC hit rate")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fget_mdc_hit_rate() */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_mdc_size
 *
 * Purpose:     Retrieves the maximum size, minimum clean size, current
 *              size, and current number of entries from the metadata
 *              cache associated with the specified file.  If any of
 *              the ptr parameters are NULL, the associated datum is
 *              not returned.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_mdc_size(hid_t file_id, size_t *max_size, size_t *min_clean_size, size_t *cur_size,
                int *cur_num_entries)
{
    H5VL_object_t *vol_obj;
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "i*z*z*z*Is", file_id, max_size, min_clean_size, cur_size, cur_num_entries);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    /* Get the size data */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_GET_MDC_SIZE, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL,
                           max_size, min_clean_size, cur_size, cur_num_entries) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get MDC size")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fget_mdc_size() */

/*-------------------------------------------------------------------------
 * Function:    H5Freset_mdc_hit_rate_stats
 *
 * Purpose:     Reset the hit rate statistic whose current value can
 *              be obtained via the H5Fget_mdc_hit_rate() call.  Note
 *              that this statistic will also be reset once per epoch
 *              by the automatic cache resize code if it is enabled.
 *
 *              It is probably a bad idea to call this function unless
 *              you are controlling cache size from your program instead
 *              of using our cache size control code.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Freset_mdc_hit_rate_stats(hid_t file_id)
{
    H5VL_object_t *vol_obj   = NULL;
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* Get the file object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(file_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")

    /* Reset the hit rate statistic */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_RESET_MDC_HIT_RATE, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "can't reset cache hit rate")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Freset_mdc_hit_rate_stats() */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_name
 *
 * Purpose:     Gets the name of the file to which object OBJ_ID belongs.
 *              If 'name' is non-NULL then write up to 'size' bytes into that
 *              buffer and always return the length of the entry name.
 *              Otherwise `size' is ignored and the function does not store
 *              the name, just returning the number of characters required to
 *              store the name. If an error occurs then the buffer pointed to
 *              by 'name' (NULL or non-NULL) is unchanged and the function
 *              returns a negative value.
 *
 * Note:        This routine returns the name that was used to open the file,
 *              not the actual name after resolving symlinks, etc.
 *
 * Return:      Success:    The length of the file name
 *              Failure:    -1
 *-------------------------------------------------------------------------
 */
ssize_t
H5Fget_name(hid_t obj_id, char *name /*out*/, size_t size)
{
    H5VL_object_t *vol_obj = NULL;
    H5I_type_t     type;
    ssize_t        ret_value = -1; /* Return value */

    FUNC_ENTER_API((-1))
    H5TRACE3("Zs", "ixz", obj_id, name, size);

    /* Check the type */
    type = H5I_get_type(obj_id);
    if (H5I_FILE != type && H5I_GROUP != type && H5I_DATATYPE != type && H5I_DATASET != type &&
        H5I_ATTR != type)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, (-1), "not a file or file object")

    /* Get the file object */
    if (NULL == (vol_obj = H5VL_vol_object(obj_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, (-1), "invalid file identifier")

    /* Get the filename via the VOL */
    if (H5VL_file_get(vol_obj, H5VL_FILE_GET_NAME, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, (int)type, size,
                      name, &ret_value) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, (-1), "unable to get file name")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_name() */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_info2
 *
 * Purpose:     Gets general information about the file, including:
 *              1. Get storage size for superblock extension if there is one.
 *              2. Get the amount of btree and heap storage for entries
 *                 in the SOHM table if there is one.
 *              3. The amount of free space tracked in the file.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_info2(hid_t obj_id, H5F_info2_t *finfo)
{
    H5VL_object_t *vol_obj = NULL;
    H5I_type_t     type;
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*x", obj_id, finfo);

    /* Check args */
    if (!finfo)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file info pointer can't be NULL")

    /* Check the type */
    type = H5I_get_type(obj_id);
    if (H5I_FILE != type && H5I_GROUP != type && H5I_DATATYPE != type && H5I_DATASET != type &&
        H5I_ATTR != type)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

    /* Get the file object */
    if (NULL == (vol_obj = H5VL_vol_object(obj_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid object identifier")

    /* Get the file information */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_GET_INFO, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL,
                           (int)type, finfo) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to retrieve file info")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_info2() */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_metadata_read_retry_info
 *
 * Purpose:     To retrieve the collection of read retries for metadata
 *              items with checksum.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_metadata_read_retry_info(hid_t file_id, H5F_retry_info_t *info)
{
    H5VL_object_t *vol_obj   = NULL;    /* File object for file ID */
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*x", file_id, info);

    /* Check args */
    if (!info)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no info struct")

    /* Get the file pointer */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    /* Get the retry info */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_GET_METADATA_READ_RETRY_INFO, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL, info) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTRELEASE, FAIL, "can't get metadata read retry info")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_metadata_read_retry_info() */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_free_sections
 *
 * Purpose:     To get free-space section information for free-space manager with
 *              TYPE that is associated with file FILE_ID.
 *              If SECT_INFO is null, this routine returns the total # of free-space
 *              sections.
 *
 * Return:      Success:   The total # of free space sections
 *              Failure:   -1
 *-------------------------------------------------------------------------
 */
ssize_t
H5Fget_free_sections(hid_t file_id, H5F_mem_t type, size_t nsects, H5F_sect_info_t *sect_info /*out*/)
{
    H5VL_object_t *vol_obj   = NULL;
    ssize_t        ret_value = -1; /* Return value */

    FUNC_ENTER_API((-1))
    H5TRACE4("Zs", "iFmzx", file_id, type, nsects, sect_info);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, (-1), "invalid file identifier")
    if (sect_info && nsects == 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, (-1), "nsects must be > 0")

    /* Get the free-space section information in the file */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_GET_FREE_SECTIONS, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL, sect_info, &ret_value, (int)type, nsects) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, (-1), "unable to get file free sections")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_free_sections() */

/*-------------------------------------------------------------------------
 * Function:    H5Fclear_elink_file_cache
 *
 * Purpose:     Releases the external file cache associated with the
 *              provided file, potentially closing any cached files
 *              unless they are held open from somewhere\ else.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fclear_elink_file_cache(hid_t file_id)
{
    H5VL_object_t *vol_obj;             /* File */
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    /* Release the EFC */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_CLEAR_ELINK_CACHE, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTRELEASE, FAIL, "can't release external file cache")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fclear_elink_file_cache() */

/*-------------------------------------------------------------------------
 * Function:    H5Fstart_swmr_write
 *
 * Purpose:     To enable SWMR writing mode for the file
 *
 *              1) Refresh opened objects: part 1
 *              2) Flush & reset accumulator
 *              3) Mark the file in SWMR writing mode
 *              4) Set metadata read attempts and retries info
 *              5) Disable accumulator
 *              6) Evict all cache entries except the superblock
 *              7) Refresh opened objects (part 2)
 *              8) Unlock the file
 *
 *              Pre-conditions:
 *
 *              1) The file being opened has v3 superblock
 *              2) The file is opened with H5F_ACC_RDWR
 *              3) The file is not already marked for SWMR writing
 *              4) Current implementaion for opened objects:
 *                  --only allow datasets and groups without attributes
 *                  --disallow named datatype with/without attributes
 *                  --disallow opened attributes attached to objects
 *
 * NOTE:        Currently, only opened groups and datasets are allowed
 *              when enabling SWMR via H5Fstart_swmr_write().
 *              Will later implement a different approach--
 *              set up flush dependency/proxy even for file opened without
 *              SWMR to resolve issues with opened objects.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fstart_swmr_write(hid_t file_id)
{
    H5VL_object_t *vol_obj   = NULL;    /* File info */
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "hid_t identifier is not a file ID")

    /* Set up collective metadata if appropriate */
    if (H5CX_set_loc(file_id) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "can't set collective metadata read info")

    /* Start SWMR writing */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_START_SWMR_WRITE, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_SYSTEM, FAIL, "unable to start SWMR writing")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fstart_swmr_write() */

/*-------------------------------------------------------------------------
 * Function:    H5Fstart_mdc_logging
 *
 * Purpose:     Start metadata cache logging operations for a file.
 *                  - Logging must have been set up via the fapl.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fstart_mdc_logging(hid_t file_id)
{
    H5VL_object_t *vol_obj;             /* File info */
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* Sanity check */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "hid_t identifier is not a file ID")

    /* Call mdc logging function */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_START_MDC_LOGGING, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_LOGGING, FAIL, "unable to start mdc logging")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fstart_mdc_logging() */

/*-------------------------------------------------------------------------
 * Function:    H5Fstop_mdc_logging
 *
 * Purpose:     Stop metadata cache logging operations for a file.
 *                  - Does not close the log file.
 *                  - Logging must have been set up via the fapl.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fstop_mdc_logging(hid_t file_id)
{
    H5VL_object_t *vol_obj;             /* File info */
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* Sanity check */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "hid_t identifier is not a file ID")

    /* Call mdc logging function */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_STOP_MDC_LOGGING, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_LOGGING, FAIL, "unable to stop mdc logging")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fstop_mdc_logging() */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_mdc_logging_status
 *
 * Purpose:     Get the logging flags. is_enabled determines if logging was
 *              set up via the fapl. is_currently_logging determines if
 *              log messages are being recorded at this time.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_mdc_logging_status(hid_t file_id, hbool_t *is_enabled, hbool_t *is_currently_logging)
{
    H5VL_object_t *vol_obj;             /* File info */
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "i*b*b", file_id, is_enabled, is_currently_logging);

    /* Sanity check */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "hid_t identifier is not a file ID")

    /* Call mdc logging function */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_GET_MDC_LOGGING_STATUS, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL, is_enabled, is_currently_logging) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_LOGGING, FAIL, "unable to get logging status")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fget_mdc_logging_status() */

/*-------------------------------------------------------------------------
 * Function:    H5Fset_libver_bounds
 *
 * Purpose:     Set to a different low and high bounds while a file is open.
 *              This public routine is introduced in place of
 *              H5Fset_latest_format() starting release 1.10.2.
 *              See explanation for H5Fset_latest_format() in H5Fdeprec.c.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fset_libver_bounds(hid_t file_id, H5F_libver_t low, H5F_libver_t high)
{
    H5VL_object_t *vol_obj;             /* File as VOL object           */
    herr_t         ret_value = SUCCEED; /* Return value 				*/

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "iFvFv", file_id, low, high);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "not a file ID")

    /* Set up collective metadata if appropriate */
    if (H5CX_set_loc(file_id) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "can't set collective metadata read info")

    /* Set the library's version bounds */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_SET_LIBVER_BOUNDS, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL, low, high) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "can't set library version bounds")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fset_libver_bounds() */

/*-------------------------------------------------------------------------
 * Function:    H5Fformat_convert (Internal)
 *
 * Purpose:     Downgrade the superblock version to v2 and
 *              downgrade persistent file space to non-persistent
 *              for 1.8 library.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fformat_convert(hid_t file_id)
{
    H5VL_object_t *vol_obj   = NULL;    /* File */
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "file_id parameter is not a valid file identifier")

    /* Set up collective metadata if appropriate */
    if (H5CX_set_loc(file_id) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "can't set collective metadata read info")

    /* Convert the format */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_FORMAT_CONVERT, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTCONVERT, FAIL, "can't convert file format")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fformat_convert() */

/*-------------------------------------------------------------------------
 * Function:    H5Freset_page_buffering_stats
 *
 * Purpose:     Resets statistics for the page buffer layer.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Freset_page_buffering_stats(hid_t file_id)
{
    H5VL_object_t *vol_obj;             /* File to reset stats on */
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")

    /* Reset the statistics */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_RESET_PAGE_BUFFERING_STATS, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "can't reset stats for page buffering")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Freset_page_buffering_stats() */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_page_buffering_stats
 *
 * Purpose:     Retrieves statistics for the page buffer layer.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_page_buffering_stats(hid_t file_id, unsigned accesses[2], unsigned hits[2], unsigned misses[2],
                            unsigned evictions[2], unsigned bypasses[2])
{
    H5VL_object_t *vol_obj;             /* File object */
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "i*Iu*Iu*Iu*Iu*Iu", file_id, accesses, hits, misses, evictions, bypasses);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")
    if (NULL == accesses || NULL == hits || NULL == misses || NULL == evictions || NULL == bypasses)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL input parameters for stats")

    /* Get the statistics */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_GET_PAGE_BUFFERING_STATS, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL, accesses, hits, misses, evictions, bypasses) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't retrieve stats for page buffering")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fget_page_buffering_stats() */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_mdc_image_info
 *
 * Purpose:     Retrieves the image_addr and image_len for the cache image in the file.
 *              image_addr:  --base address of the on disk metadata cache image
 *                           --HADDR_UNDEF if no cache image
 *              image_len:   --size of the on disk metadata cache image
 *                           --zero if no cache image
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_mdc_image_info(hid_t file_id, haddr_t *image_addr, hsize_t *image_len)
{
    H5VL_object_t *vol_obj;             /* File info */
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "i*a*h", file_id, image_addr, image_len);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "hid_t identifier is not a file ID")

    /* Go get the address and size of the cache image */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_GET_MDC_IMAGE_INFO, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL, image_addr, image_len) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't retrieve cache image info")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fget_mdc_image_info() */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_eoa
 *
 * Purpose:     Gets the address of the first byte after the last
 *              allocated memory in the file.
 *              (See H5FDget_eoa() in H5FD.c)
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_eoa(hid_t file_id, haddr_t *eoa)
{
    H5VL_object_t *vol_obj;             /* File info */
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*a", file_id, eoa);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "hid_t identifier is not a file ID")

    /* Only do work if valid pointer to fill in */
    if (eoa) {
        /* Retrieve the EOA for the file */
        if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_GET_EOA, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL,
                               eoa) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get EOA")
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fget_eoa() */

/*-------------------------------------------------------------------------
 * Function:    H5Fincrement_filesize
 *
 * Purpose:     Set the EOA for the file to the maximum of (EOA, EOF) + increment
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fincrement_filesize(hid_t file_id, hsize_t increment)
{
    H5VL_object_t *vol_obj;             /* File info */
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "ih", file_id, increment);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "hid_t identifier is not a file ID")

    /* Increment the file size */
    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_INCR_FILESIZE, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL,
                           increment) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to increment file size")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fincrement_filesize() */

/*-------------------------------------------------------------------------
 * Function:    H5Fget_dset_no_attrs_hint
 *
 * Purpose:     Get the file-level setting to create minimized dataset object headers.
 *              Result is stored at pointer `minimize`.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_dset_no_attrs_hint(hid_t file_id, hbool_t *minimize)
{
    H5VL_object_t *vol_obj   = NULL;
    herr_t         ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*b", file_id, minimize);

    if (NULL == minimize)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "out pointer 'minimize' cannot be NULL")

    vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE);
    if (NULL == vol_obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")

    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_GET_MIN_DSET_OHDR_FLAG, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL, minimize) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set file's dataset header minimization flag")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fget_dset_no_attrs_hint */

/*-------------------------------------------------------------------------
 * Function:    H5Fset_dset_no_attrs_hint
 *
 * Purpose:     Set the file-level setting to create minimized dataset object
 *              headers.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fset_dset_no_attrs_hint(hid_t file_id, hbool_t minimize)
{
    H5VL_object_t *vol_obj   = NULL;
    herr_t         ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "ib", file_id, minimize);

    vol_obj = (H5VL_object_t *)H5I_object_verify(file_id, H5I_FILE);
    if (NULL == vol_obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")

    if (H5VL_file_optional(vol_obj, H5VL_NATIVE_FILE_SET_MIN_DSET_OHDR_FLAG, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL, minimize) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set file's dataset header minimization flag")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fset_dset_no_attrs_hint */
