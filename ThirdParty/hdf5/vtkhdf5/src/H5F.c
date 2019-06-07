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

#include "H5Fmodule.h"          /* This source code file is part of the H5F module */


/***********/
/* Headers */
/***********/
#include "H5private.h"          /* Generic Functions                    */
#include "H5Aprivate.h"         /* Attributes                           */
#include "H5ACprivate.h"        /* Metadata cache                       */
#include "H5CXprivate.h"        /* API Contexts                         */
#include "H5Dprivate.h"         /* Datasets                             */
#include "H5Eprivate.h"         /* Error handling                       */
#include "H5Fpkg.h"             /* File access                          */
#include "H5FDprivate.h"        /* File drivers                         */
#include "H5Gprivate.h"         /* Groups                               */
#include "H5Iprivate.h"         /* IDs                                  */
#include "H5MFprivate.h"        /* File memory management               */
#include "H5MMprivate.h"        /* Memory management                    */
#include "H5Pprivate.h"         /* Property lists                       */
#include "H5Tprivate.h"         /* Datatypes                            */


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

/* Package initialization variable */
hbool_t H5_PKG_INIT_VAR = FALSE;


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/


/* File ID class */
static const H5I_class_t H5I_FILE_CLS[1] = {{
    H5I_FILE,            /* ID class value */
    0,                   /* Class flags */
    0,                   /* # of reserved IDs for class */
    (H5I_free_t)H5F__close_cb  /* Callback routine for closing objects of this class */
}};


/*--------------------------------------------------------------------------
NAME
   H5F__init_package -- Initialize interface-specific information
USAGE
    herr_t H5F__init_package()
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.

--------------------------------------------------------------------------*/
herr_t
H5F__init_package(void)
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_PACKAGE

    /*
     * Initialize the atom group for the file IDs.
     */
    if(H5I_register_type(H5I_FILE_CLS) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to initialize interface")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5F__init_package() */


/*-------------------------------------------------------------------------
 * Function: H5F_term_package
 *
 * Purpose:  Terminate this interface: free all memory and reset global
 *           variables to their initial values.  Release all ID groups
 *           associated with this interface.
 *
 * Return:   Success:   Positive if anything was done that might
 *                      have affected other interfaces;
 *                      zero otherwise.
 *           Failure:   Never fails.
 *-------------------------------------------------------------------------
 */
int
H5F_term_package(void)
{
    int    n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(H5_PKG_INIT_VAR) {
        if(H5I_nmembers(H5I_FILE) > 0) {
            (void)H5I_clear_type(H5I_FILE, FALSE, FALSE);
            n++; /*H5I*/
        } /* end if */
        else {
            /* Make certain we've cleaned up all the shared file objects */
            H5F_sfile_assert_num(0);

            /* Destroy the file object id group */
            n += (H5I_dec_type_ref(H5I_FILE) > 0);

            /* Mark closed */
            if(0 == n)
                H5_PKG_INIT_VAR = FALSE;
        } /* end else */
    } /* end if */

    FUNC_LEAVE_NOAPI(n)
} /* end H5F_term_package() */


/*-------------------------------------------------------------------------
 * Function: H5Fget_create_plist
 *
 * Purpose:  Get an atom for a copy of the file-creation property list for
 *           this file. This function returns an atom with a copy of the
 *           properties used to create a file.
 *
 * Return:   Success:    template ID
 *           Failure:    FAIL
 *-------------------------------------------------------------------------
 */
hid_t
H5Fget_create_plist(hid_t file_id)
{
    H5F_t *file;                /* File info */
    H5P_genplist_t *plist;      /* Property list */
    hid_t ret_value;            /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("i", "i", file_id);

    /* check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file")
    if(NULL == (plist = (H5P_genplist_t *)H5I_object(file->shared->fcpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")

    /* Create the property list object to return */
    if((ret_value = H5P_copy_plist(plist, TRUE)) < 0)
        HGOTO_ERROR(H5E_INTERNAL, H5E_CANTINIT, FAIL, "unable to copy file creation properties")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_create_plist() */


/*-------------------------------------------------------------------------
 * Function: H5Fget_access_plist
 *
 * Purpose:  Returns a copy of the file access property list of the
 *           specified file.
 *
 *              NOTE: Make sure that, if you are going to overwrite
 *              information in the copied property list that was
 *              previously opened and assigned to the property list, then
 *              you must close it before overwriting the values.
 *
 * Return:   Success:    Object ID for a copy of the file access
 *                       property list.
 *           Failure:    FAIL
 *-------------------------------------------------------------------------
 */
hid_t
H5Fget_access_plist(hid_t file_id)
{
    H5F_t *f;           /* File info */
    hid_t ret_value;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("i", "i", file_id);

    /* Check args */
    if(NULL == (f = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file")

    /* Retrieve the file's access property list */
    if((ret_value = H5F_get_access_plist(f, TRUE)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get file access property list")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_access_plist() */


/*-------------------------------------------------------------------------
 * Function: H5Fget_obj_count
 *
 * Purpose:  Public function returning the number of opened object IDs
 *           (files, datasets, groups and datatypes) in the same file.
 *
 * Return:   Non-negative on success; negative on failure.
 *-------------------------------------------------------------------------
 */
ssize_t
H5Fget_obj_count(hid_t file_id, unsigned types)
{
    H5F_t    *f = NULL;         /* File to query */
    size_t   obj_count = 0;     /* Number of opened objects */
    ssize_t  ret_value;         /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("Zs", "iIu", file_id, types);

    /* Check arguments */
    if(file_id != (hid_t)H5F_OBJ_ALL && (NULL == (f = (H5F_t *)H5I_object_verify(file_id, H5I_FILE))))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file id")
    if(0 == (types & H5F_OBJ_ALL))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not an object type")

    /* Perform the query */
    if(H5F_get_obj_count(f, types, TRUE, &obj_count) < 0)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADITER, FAIL, "H5F_get_obj_count failed")

    /* Set the return value */
    ret_value = (ssize_t)obj_count;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_obj_count() */


/*-------------------------------------------------------------------------
 * Function: H5Fget_object_ids
 *
 * Purpose:  Public function to return a list of opened object IDs.
 *
 * Return:   Non-negative on success; negative on failure.
 *-------------------------------------------------------------------------
 */
ssize_t
H5Fget_obj_ids(hid_t file_id, unsigned types, size_t max_objs, hid_t *oid_list)
{
    H5F_t    *f = NULL;         /* File to query */
    size_t    obj_id_count = 0; /* Number of open objects */
    ssize_t   ret_value;        /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("Zs", "iIuz*i", file_id, types, max_objs, oid_list);

    /* Check arguments */
    if(file_id != (hid_t)H5F_OBJ_ALL && (NULL == (f = (H5F_t *)H5I_object_verify(file_id, H5I_FILE))))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file id")
    if(0 == (types & H5F_OBJ_ALL))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not an object type")
    if(!oid_list)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "object ID list is NULL")

    /* Perform the query */
    if(H5F_get_obj_ids(f, types, max_objs, oid_list, TRUE, &obj_id_count) < 0)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADITER, FAIL, "H5F_get_obj_ids failed")

    /* Set the return value */
    ret_value = (ssize_t)obj_id_count;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_obj_ids() */


/*-------------------------------------------------------------------------
 * Function:    H5Fget_vfd_handle
 *
 * Purpose:     Returns a pointer to the file handle of the low-level file
 *              driver.
 *
 * Return:      Success:        non-negative value.
 *              Failure:        negative.
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_vfd_handle(hid_t file_id, hid_t fapl, void **file_handle)
{
    H5F_t               *file;          /* File to query */
    herr_t              ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "ii**x", file_id, fapl, file_handle);

    /* Check args */
    if(!file_handle)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file handle pointer")

    /* Get the file */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file id")

    /* Retrieve the VFD handle for the file */
    if(H5F_get_vfd_handle(file, fapl, file_handle) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't retrieve VFD handle")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_vfd_handle() */


/*-------------------------------------------------------------------------
 * Function: H5Fis_hdf5
 *
 * Purpose:  Check the file signature to detect an HDF5 file.
 *
 * Bugs:     This function is not robust: it only uses the default file
 *           driver when attempting to open the file when in fact it
 *           should use all known file drivers.
 *
 * Return:   Success:    TRUE/FALSE
 *           Failure:    Negative
 *-------------------------------------------------------------------------
 */
htri_t
H5Fis_hdf5(const char *name)
{
    htri_t      ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("t", "*s", name);

    /* Check args and all the boring stuff. */
    if(!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "no file name specified")

    /* call the private is_HDF5 function */
    /* (Should not trigger raw data I/O - QAK, 2018/01/03) */
    if((ret_value = H5F__is_hdf5(name)) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_NOTHDF5, FAIL, "unable open file")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fis_hdf5() */


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
 *              Failure:    FAIL
 *-------------------------------------------------------------------------
 */
hid_t
H5Fcreate(const char *filename, unsigned flags, hid_t fcpl_id, hid_t fapl_id)
{
    H5F_t   *new_file = NULL;               /* file struct for new file                 */
    hid_t   ret_value;                      /* return value                             */

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
    else
        if (TRUE != H5P_isa_class(fcpl_id, H5P_FILE_CREATE))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not file create property list")

    /* Verify access property list and set up collective metadata if appropriate */
    if(H5CX_set_apl(&fapl_id, H5P_CLS_FACC, H5I_INVALID_HID, TRUE) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, H5I_INVALID_HID, "can't set access property list info")

    /* Adjust bit flags by turning on the creation bit and making sure that
     * the EXCL or TRUNC bit is set.  All newly-created files are opened for
     * reading and writing.
     */
    if (0 == (flags & (H5F_ACC_EXCL | H5F_ACC_TRUNC)))
        flags |= H5F_ACC_EXCL;	 /*default*/
    flags |= H5F_ACC_RDWR | H5F_ACC_CREAT;

    /* Create a new file or truncate an existing file. */
    if(NULL == (new_file = H5F_open(filename, flags, fcpl_id, fapl_id)))
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, H5I_INVALID_HID, "unable to create file")

    /* Get an atom for the file */
    if ((ret_value = H5I_register(H5I_FILE, new_file, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to atomize file")

    /* Keep this ID in file object structure */
    new_file->file_id = ret_value;

done:
    if(ret_value < 0 && new_file && H5F_try_close(new_file, NULL) < 0)
        HDONE_ERROR(H5E_FILE, H5E_CANTCLOSEFILE, H5I_INVALID_HID, "problems closing file")

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
 *
 *              Failure:    FAIL
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Fopen(const char *filename, unsigned flags, hid_t fapl_id)
{
    H5F_t       *new_file = NULL;                   /* file struct for new file                 */
    hid_t       ret_value;                          /* return value                             */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE3("i", "*sIui", filename, flags, fapl_id);

    /* Check/fix arguments. */
    if(!filename || !*filename)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "invalid file name")
    /* Reject undefined flags (~H5F_ACC_PUBLIC_FLAGS) and the H5F_ACC_TRUNC & H5F_ACC_EXCL flags */
    if((flags & ~H5F_ACC_PUBLIC_FLAGS) ||
            (flags & H5F_ACC_TRUNC) || (flags & H5F_ACC_EXCL))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "invalid file open flags")
    /* Asking for SWMR write access on a read-only file is invalid */
    if((flags & H5F_ACC_SWMR_WRITE) && 0 == (flags & H5F_ACC_RDWR))
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, H5I_INVALID_HID, "SWMR write access on a file open for read-only access is not allowed")
    /* Asking for SWMR read access on a non-read-only file is invalid */
    if((flags & H5F_ACC_SWMR_READ) && (flags & H5F_ACC_RDWR))
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, H5I_INVALID_HID, "SWMR read access on a file open for read-write access is not allowed")

    /* Verify access property list and set up collective metadata if appropriate */
    if(H5CX_set_apl(&fapl_id, H5P_CLS_FACC, H5I_INVALID_HID, TRUE) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, H5I_INVALID_HID, "can't set access property list info")

    /* Open the file */
    if(NULL == (new_file = H5F_open(filename, flags, H5P_FILE_CREATE_DEFAULT, fapl_id)))
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, H5I_INVALID_HID, "unable to open file")

    /* Get an atom for the file */
    if((ret_value = H5I_register(H5I_FILE, new_file, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to atomize file handle")

    /* Keep this ID in file object structure */
    new_file->file_id = ret_value;

done:
    if(ret_value < 0 && new_file && H5F_try_close(new_file, NULL) < 0)
        HDONE_ERROR(H5E_FILE, H5E_CANTCLOSEFILE, H5I_INVALID_HID, "problems closing file")

    FUNC_LEAVE_API(ret_value)
} /* end H5Fopen() */


/*-------------------------------------------------------------------------
 * Function: H5Fflush
 *
 * Purpose:  Flushes all outstanding buffers of a file to disk but does
 *           not remove them from the cache.  The OBJECT_ID can be a file,
 *           dataset, group, attribute, or named data type.
 *
 * Return:   Non-negative on success/Negative on failure
 *-------------------------------------------------------------------------
 */
herr_t
H5Fflush(hid_t object_id, H5F_scope_t scope)
{
    H5F_t      *f = NULL;              /* File to flush */
    H5O_loc_t  *oloc = NULL;           /* Object location for ID */
    herr_t      ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "iFs", object_id, scope);

    switch(H5I_get_type(object_id)) {
        case H5I_FILE:
            if(NULL == (f = (H5F_t *)H5I_object(object_id)))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")
            break;

        case H5I_GROUP:
            {
                H5G_t    *grp;

                if(NULL == (grp = (H5G_t *)H5I_object(object_id)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid group identifier")
                oloc = H5G_oloc(grp);
            }
            break;

        case H5I_DATATYPE:
            {
                H5T_t    *type;

                if(NULL == (type = (H5T_t *)H5I_object(object_id)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid type identifier")
                oloc = H5T_oloc(type);
            }
            break;

        case H5I_DATASET:
            {
                H5D_t    *dset;

                if(NULL == (dset = (H5D_t *)H5I_object(object_id)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid dataset identifier")
                oloc = H5D_oloc(dset);
            }
            break;

        case H5I_ATTR:
            {
                H5A_t    *attr;

                if(NULL == (attr = (H5A_t *)H5I_object(object_id)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid attribute identifier")
                oloc = H5A_oloc(attr);
            }
            break;

        case H5I_UNINIT:
        case H5I_BADID:
        case H5I_DATASPACE:
        case H5I_REFERENCE:
        case H5I_VFL:
        case H5I_GENPROP_CLS:
        case H5I_GENPROP_LST:
        case H5I_ERROR_CLASS:
        case H5I_ERROR_MSG:
        case H5I_ERROR_STACK:
        case H5I_NTYPES:
        default:
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")
    } /* end switch */

    if(!f) {
        if(!oloc)
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "object is not assocated with a file")
        f = oloc->file;
    } /* end if */
    if(!f)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "object is not associated with a file")

    /* Flush the file */
    /*
     * Nothing to do if the file is read only.    This determination is
     * made at the shared open(2) flags level, implying that opening a
     * file twice, once for read-only and once for read-write, and then
     * calling H5Fflush() with the read-only handle, still causes data
     * to be flushed.
     */
    if(H5F_ACC_RDWR & H5F_INTENT(f)) {
        hid_t fapl_id = H5P_DEFAULT;    /* FAPL to use */

        /* Verify access property list and set up collective metadata if appropriate */
        if(H5CX_set_apl(&fapl_id, H5P_CLS_FACC, object_id, TRUE) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "can't set access property list info")

        /* Flush other files, depending on scope */
        if(H5F_SCOPE_GLOBAL == scope) {
            /* Call the flush routine for mounted file hierarchies */
            if(H5F_flush_mounts(f) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTFLUSH, FAIL, "unable to flush mounted file hierarchy")
        } /* end if */
        else
            /* Call the flush routine, for this file */
            if(H5F__flush(f) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTFLUSH, FAIL, "unable to flush file's cached information")
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fflush() */


/*-------------------------------------------------------------------------
 * Function: H5Fclose
 *
 * Purpose:  This function closes the file specified by FILE_ID by
 *        flushing all data to storage, and terminating access to the
 *        file through FILE_ID.  If objects (e.g., datasets, groups,
 *        etc.) are open in the file then the underlying storage is not
 *        closed until those objects are closed; however, all data for
 *        the file and the open objects is flushed.
 *
 * Return:   Success:    Non-negative
 *           Failure:    Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fclose(hid_t file_id)
{
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* Check arguments */
    if(H5I_FILE != H5I_get_type(file_id))
        HGOTO_ERROR(H5E_FILE, H5E_BADTYPE, FAIL, "not a file ID")

    /* Close the file */
    if(H5F__close(file_id) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTCLOSEFILE, FAIL, "closing file ID failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fclose() */


/*-------------------------------------------------------------------------
 * Function: H5Freopen
 *
 * Purpose:  Reopen a file.  The new file handle which is returned points
 *        to the same file as the specified file handle.  Both handles
 *        share caches and other information.  The only difference
 *        between the handles is that the new handle is not mounted
 *        anywhere and no files are mounted on it.
 *
 * Return:   Success:    New file ID
 *           Failure:    FAIL
 *-------------------------------------------------------------------------
 */
hid_t
H5Freopen(hid_t file_id)
{
    H5F_t    *old_file = NULL;
    H5F_t    *new_file = NULL;
    hid_t    ret_value;

    FUNC_ENTER_API(FAIL)
    H5TRACE1("i", "i", file_id);

    /* Check arguments */
    if(NULL == (old_file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file")

    /* Get a new "top level" file struct, sharing the same "low level" file struct */
    if(NULL == (new_file = H5F__new(old_file->shared, 0, H5P_FILE_CREATE_DEFAULT, H5P_FILE_ACCESS_DEFAULT, NULL)))
        HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to reopen file")

    /* Duplicate old file's names */
    new_file->open_name = H5MM_xstrdup(old_file->open_name);
    new_file->actual_name = H5MM_xstrdup(old_file->actual_name);
    new_file->extpath = H5MM_xstrdup(old_file->extpath);

    if((ret_value = H5I_register(H5I_FILE, new_file, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to atomize file handle")

    /* Keep this ID in file object structure */
    new_file->file_id = ret_value;

done:
    if(ret_value < 0 && new_file)
        if(H5F__dest(new_file, FALSE) < 0)
            HDONE_ERROR(H5E_FILE, H5E_CANTCLOSEFILE, FAIL, "can't close file")

    FUNC_LEAVE_API(ret_value)
} /* end H5Freopen() */


/*-------------------------------------------------------------------------
 * Function: H5Fget_intent
 *
 * Purpose:  Public API to retrieve the file's 'intent' flags passed
 *           during H5Fopen()
 *
 * Return:   Non-negative on success/negative on failure
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_intent(hid_t file_id, unsigned *intent_flags)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*Iu", file_id, intent_flags);

    /* If no intent flags were passed in, exit quietly */
    if(intent_flags) {
        H5F_t *file;           /* Pointer to file structure */

        /* Get the internal file structure */
        if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file")

        /* HDF5 uses some flags internally that users don't know about.
         * Simplify things for them so that they only get either H5F_ACC_RDWR
         * or H5F_ACC_RDONLY.
         */
        if(H5F_INTENT(file) & H5F_ACC_RDWR) {
            *intent_flags = H5F_ACC_RDWR;

            /* Check for SWMR write access on the file */
            if(H5F_INTENT(file) & H5F_ACC_SWMR_WRITE)
                *intent_flags |= H5F_ACC_SWMR_WRITE;
        } /* end if */
        else {
            *intent_flags = H5F_ACC_RDONLY;

            /* Check for SWMR read access on the file */
            if(H5F_INTENT(file) & H5F_ACC_SWMR_READ)
                *intent_flags |= H5F_ACC_SWMR_READ;
        } /* end else */
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_intent() */


/*-------------------------------------------------------------------------
 * Function:    H5Fget_freespace
 *
 * Purpose:     Retrieves the amount of free space in the file.
 *
 * Return:      Success:        Amount of free space for type
 *              Failure:        Negative
 *-------------------------------------------------------------------------
 */
hssize_t
H5Fget_freespace(hid_t file_id)
{
    H5F_t      *file;           /* File object for file ID */
    hsize_t     tot_space;      /* Amount of free space in the file */
    hssize_t    ret_value;      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("Hs", "i", file_id);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    /* Go get the actual amount of free space in the file */
    if(H5MF_get_freespace(file, &tot_space, NULL) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get free space for file")

    ret_value = (hssize_t)tot_space;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_freespace() */


/*-------------------------------------------------------------------------
 * Function:    H5Fget_filesize
 *
 * Purpose:     Retrieves the file size of the HDF5 file. This function
 *              is called after an existing file is opened in order
 *        to learn the true size of the underlying file.
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_filesize(hid_t file_id, hsize_t *size)
{
    H5F_t       *file;                  /* File object for file ID */
    haddr_t     max_eof_eoa;            /* Maximum of the EOA & EOF */
    haddr_t     base_addr;              /* Base address for the file */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*h", file_id, size);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    /* Go get the actual file size */
    if(H5F__get_max_eof_eoa(file, &max_eof_eoa) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "file can't get max eof/eoa ")

    base_addr = H5FD_get_base_addr(file->shared->lf);

    if(size)
        *size = (hsize_t)(max_eof_eoa + base_addr);     /* Convert relative base address for file to absolute address */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_filesize() */


/*-------------------------------------------------------------------------
 * Function:    H5Fget_file_image
 *
 * Purpose:     If a buffer is provided (via the buf_ptr argument) and is
 *        big enough (size in buf_len argument), load *buf_ptr with
 *        an image of the open file whose ID is provided in the
 *        file_id parameter, and return the number of bytes copied
 *        to the buffer.
 *
 *        If the buffer exists, but is too small to contain an image
 *        of the indicated file, return a negative number.
 *
 *        Finally, if no buffer is provided, return the size of the
 *        buffer needed.  This value is simply the eoa of the target
 *        file.
 *
 *        Note that any user block is skipped.
 *
 *        Also note that the function may not be used on files
 *        opened with either the split/multi file driver or the
 *        family file driver.
 *
 *        In the former case, the sparse address space makes the
 *        get file image operation impractical, due to the size of
 *        the image typically required.
 *
 *        In the case of the family file driver, the problem is
 *        the driver message in the super block, which will prevent
 *        the image being opened with any driver other than the
 *        family file driver -- which negates the purpose of the
 *        operation.  This can be fixed, but no resources for
 *        this now.
 *
 * Return:      Success:        Bytes copied / number of bytes needed.
 *              Failure:        negative value
 *-------------------------------------------------------------------------
 */
ssize_t
H5Fget_file_image(hid_t file_id, void *buf_ptr, size_t buf_len)
{
    H5F_t      *file;                   /* File object for file ID */
    ssize_t     ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("Zs", "i*xz", file_id, buf_ptr, buf_len);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    /* call private get_file_image function */
    /* (Should not trigger raw data I/O - QAK, 2018/01/03) */
    if((ret_value = H5F__get_file_image(file, buf_ptr, buf_len)) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get file image")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fget_file_image() */


/*-------------------------------------------------------------------------
 * Function: H5Fget_mdc_config
 *
 * Purpose:  Retrieves the current automatic cache resize configuration
 *        from the metadata cache, and return it in *config_ptr.
 *
 *        Note that the version field of *config_Ptr must be correctly
 *        filled in by the caller.  This allows us to adapt for
 *        obsolete versions of the structure.
 *
 * Return:   Success:        SUCCEED
 *           Failure:        FAIL
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_mdc_config(hid_t file_id, H5AC_cache_config_t *config_ptr)
{
    H5F_t      *file;                   /* File object for file ID */
    herr_t     ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*x", file_id, config_ptr);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")
    if((NULL == config_ptr) || (config_ptr->version != H5AC__CURR_CACHE_CONFIG_VERSION))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "Bad config_ptr")

    /* Go get the resize configuration */
    if(H5AC_get_cache_auto_resize_config(file->shared->cache, config_ptr) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5AC_get_cache_auto_resize_config() failed.")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fget_mdc_config() */


/*-------------------------------------------------------------------------
 * Function:    H5Fset_mdc_config
 *
 * Purpose:     Sets the current metadata cache automatic resize
 *        configuration, using the contents of the instance of
 *        H5AC_cache_config_t pointed to by config_ptr.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *-------------------------------------------------------------------------
 */
herr_t
H5Fset_mdc_config(hid_t file_id, H5AC_cache_config_t *config_ptr)
{
    H5F_t      *file;                   /* File object for file ID */
    herr_t     ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*x", file_id, config_ptr);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
         HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    /* set the resize configuration  */
    if(H5AC_set_cache_auto_resize_config(file->shared->cache, config_ptr) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "H5AC_set_cache_auto_resize_config() failed.")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fset_mdc_config() */


/*-------------------------------------------------------------------------
 * Function:    H5Fget_mdc_hit_rate
 *
 * Purpose:     Retrieves the current hit rate from the metadata cache.
 *        This rate is the overall hit rate since the last time
 *        the hit rate statistics were reset either manually or
 *        automatically.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_mdc_hit_rate(hid_t file_id, double *hit_rate_ptr)
{
    H5F_t      *file;                   /* File object for file ID */
    herr_t     ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*d", file_id, hit_rate_ptr);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    if(NULL == hit_rate_ptr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL hit rate pointer")

    /* Go get the current hit rate */
    if(H5AC_get_cache_hit_rate(file->shared->cache, hit_rate_ptr) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5AC_get_cache_hit_rate() failed.")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fget_mdc_hit_rate() */


/*-------------------------------------------------------------------------
 * Function:    H5Fget_mdc_size
 *
 * Purpose:     Retrieves the maximum size, minimum clean size, current
 *        size, and current number of entries from the metadata
 *        cache associated with the specified file.  If any of
 *        the ptr parameters are NULL, the associated datum is
 *        not returned.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_mdc_size(hid_t file_id, size_t *max_size_ptr, size_t *min_clean_size_ptr,
    size_t *cur_size_ptr, int *cur_num_entries_ptr)
{
    H5F_t      *file;                   /* File object for file ID */
    uint32_t   cur_num_entries;
    herr_t     ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "i*z*z*z*Is", file_id, max_size_ptr, min_clean_size_ptr,
             cur_size_ptr, cur_num_entries_ptr);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
         HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    /* Go get the size data */
    if(H5AC_get_cache_size(file->shared->cache, max_size_ptr,
            min_clean_size_ptr, cur_size_ptr, &cur_num_entries) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5AC_get_cache_size() failed.")

    if(cur_num_entries_ptr != NULL)
        *cur_num_entries_ptr = (int)cur_num_entries;

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fget_mdc_size() */


/*-------------------------------------------------------------------------
 * Function:    H5Freset_mdc_hit_rate_stats
 *
 * Purpose:     Reset the hit rate statistic whose current value can
 *        be obtained via the H5Fget_mdc_hit_rate() call.  Note
 *        that this statistic will also be reset once per epoch
 *        by the automatic cache resize code if it is enabled.
 *
 *        It is probably a bad idea to call this function unless
 *        you are controlling cache size from your program instead
 *        of using our cache size control code.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *-------------------------------------------------------------------------
 */
herr_t
H5Freset_mdc_hit_rate_stats(hid_t file_id)
{
    H5F_t      *file;                   /* File object for file ID */
    herr_t     ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
         HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    /* Reset the hit rate statistic */
    if(H5AC_reset_cache_hit_rate_stats(file->shared->cache) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "can't reset cache hit rate")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Freset_mdc_hit_rate_stats() */


/*-------------------------------------------------------------------------
 * Function:    H5Fget_name
 *
 * Purpose:     Gets the name of the file to which object OBJ_ID belongs.
 *              If `name' is non-NULL then write up to `size' bytes into that
 *              buffer and always return the length of the entry name.
 *              Otherwise `size' is ignored and the function does not store the name,
 *              just returning the number of characters required to store the name.
 *              If an error occurs then the buffer pointed to by `name' (NULL or non-NULL)
 *              is unchanged and the function returns a negative value.
 *
 * Note:        This routine returns the name that was used to open the file,
 *              not the actual name after resolving symlinks, etc.
 *
 * Return:      Success:        The length of the file name
 *              Failure:        Negative
 *-------------------------------------------------------------------------
 */
ssize_t
H5Fget_name(hid_t obj_id, char *name/*out*/, size_t size)
{
    H5F_t         *f;           /* Top file in mount hierarchy */
    size_t        len;
    ssize_t       ret_value;

    FUNC_ENTER_API(FAIL)
    H5TRACE3("Zs", "ixz", obj_id, name, size);

    /* For file IDs, get the file object directly */
    /* (This prevents the H5G_loc() call from returning the file pointer for
     * the top file in a mount hierarchy)
     */
    if(H5I_get_type(obj_id) == H5I_FILE ) {
        if(NULL == (f = (H5F_t *)H5I_object(obj_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file")
    } /* end if */
    else {
        H5G_loc_t     loc;        /* Object location */

        /* Get symbol table entry */
        if(H5G_loc(obj_id, &loc) < 0)
             HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a valid object ID")
        f = loc.oloc->file;
    } /* end else */

    len = HDstrlen(H5F_OPEN_NAME(f));

    if(name) {
        HDstrncpy(name, H5F_OPEN_NAME(f), size);
        name[size-1]='\0';
    } /* end if */

    /* Set return value */
    ret_value = (ssize_t)len;

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
 * Return:      Success:        non-negative on success
 *              Failure:        Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_info2(hid_t obj_id, H5F_info2_t *finfo)
{
    H5F_t *f;                           /* Top file in mount hierarchy */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*x", obj_id, finfo);

    /* Check args */
    if(!finfo)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no info struct")

    /* For file IDs, get the file object directly */
    /* (This prevents the H5G_loc() call from returning the file pointer for
     * the top file in a mount hierarchy)
     */
    if(H5I_get_type(obj_id) == H5I_FILE ) {
        if(NULL == (f = (H5F_t *)H5I_object(obj_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file")
    } /* end if */
    else {
        H5G_loc_t     loc;        /* Object location */

        /* Get symbol table entry */
        if(H5G_loc(obj_id, &loc) < 0)
             HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a valid object ID")
        f = loc.oloc->file;
    } /* end else */
    HDassert(f->shared);

    /* Get the file info */
    if(H5F__get_info(f, finfo) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to retrieve file info")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_info2() */


/*-------------------------------------------------------------------------
 * Function:    H5Fget_metadata_read_retry_info
 *
 * Purpose:     To retrieve the collection of read retries for metadata items with checksum.
 *
 * Return:      Success:        non-negative on success
 *              Failure:        Negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_metadata_read_retry_info(hid_t file_id, H5F_retry_info_t *info)
{
    H5F_t          *file;                       /* File object for file ID */
    herr_t          ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*x", file_id, info);

    /* Check args */
    if (!info)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no info struct")

    /* Get the file pointer */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    /* Get the retry info */
    if(H5F__get_metadata_read_retry_info(file, info) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTRELEASE, FAIL, "can't get metadata read retry info")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_metadata_read_retry_info() */


/*-------------------------------------------------------------------------
 * Function: H5Fget_free_sections
 *
 * Purpose:  To get free-space section information for free-space manager with
 *           TYPE that is associated with file FILE_ID.
 *           If SECT_INFO is null, this routine returns the total # of free-space
 *           sections.
 *
 * Return:   Success:        non-negative, the total # of free space sections
 *           Failure:        negative
 *-------------------------------------------------------------------------
 */
ssize_t
H5Fget_free_sections(hid_t file_id, H5F_mem_t type, size_t nsects,
    H5F_sect_info_t *sect_info/*out*/)
{
    H5F_t         *file;        /* Top file in mount hierarchy */
    ssize_t       ret_value;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("Zs", "iFmzx", file_id, type, nsects, sect_info);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")
    if(sect_info && nsects == 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "nsects must be > 0")

    /* Get the free-space section information in the file */
    if((ret_value = H5MF_get_free_sections(file, type, nsects, sect_info)) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to check free space for file")

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
 * Return:      Success:        non-negative
 *              Failure:        negative
 *-------------------------------------------------------------------------
 */
herr_t
H5Fclear_elink_file_cache(hid_t file_id)
{
    H5F_t       *file;        /* File */
    herr_t      ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    /* See if there's an EFC */
    if(file->shared->efc)
        /* Release the EFC */
        if(H5F__efc_release(file->shared->efc) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTRELEASE, FAIL, "can't release external file cache")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fclear_elink_file_cache() */


/*-------------------------------------------------------------------------
 * Function:    H5Fstart_swmr_write
 *
 * Purpose:    To enable SWMR writing mode for the file
 *
 * Return:    Non-negative on success/negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Fstart_swmr_write(hid_t file_id)
{
    H5F_t      *file = NULL;            /* File info */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file")

    /* Set up collective metadata if appropriate */
    if(H5CX_set_loc(file_id) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "can't set collective metadata read info")

    /* Call the internal routine */
    if(H5F__start_swmr_write(file) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTCONVERT, FAIL, "unable to convert file format")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fstart_swmr_write() */


/*-------------------------------------------------------------------------
 * Function:    H5Fstart_mdc_logging
 *
 * Purpose:     Start metadata cache logging operations for a file.
 *                  - Logging must have been set up via the fapl.
 *
 * Return:      Non-negative on success/Negative on errors
 *-------------------------------------------------------------------------
 */
herr_t
H5Fstart_mdc_logging(hid_t file_id)
{
    H5F_t *file;                   /* File info */
    herr_t ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* Sanity check */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "hid_t identifier is not a file ID")

    /* Call mdc logging function */
    if(H5C_start_logging(file->shared->cache) < 0)
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
 * Return:      Non-negative on success/Negative on errors
 *-------------------------------------------------------------------------
 */
herr_t
H5Fstop_mdc_logging(hid_t file_id)
{
    H5F_t *file;                   /* File info */
    herr_t ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* Sanity check */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "hid_t identifier is not a file ID")

    /* Call mdc logging function */
    if(H5C_stop_logging(file->shared->cache) < 0)
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
 * Return:      Non-negative on success/Negative on errors
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_mdc_logging_status(hid_t file_id, hbool_t *is_enabled,
                          hbool_t *is_currently_logging)
{
    H5F_t *file;                   /* File info */
    herr_t ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "i*b*b", file_id, is_enabled, is_currently_logging);

    /* Sanity check */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "hid_t identifier is not a file ID")

    /* Call MDC logging function */
    if(H5C_get_logging_status(file->shared->cache, is_enabled, is_currently_logging) < 0)
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
 * Return:     Non-negative on success/Negative on failure
 *-------------------------------------------------------------------------
 */
herr_t
H5Fset_libver_bounds(hid_t file_id, H5F_libver_t low, H5F_libver_t high)
{
    H5F_t *f;                           /* File */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "iFvFv", file_id, low, high);

    /* Check args */
    if(NULL == (f = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "not a file ID")

    /* Set up collective metadata if appropriate */
    if(H5CX_set_loc(file_id) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "can't set collective metadata read info")

    /* Call internal set_libver_bounds function */
    if(H5F__set_libver_bounds(f, low, high) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "cannot set low/high bounds")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fset_libver_bounds() */


/*-------------------------------------------------------------------------
 * Function: H5Fformat_convert
 *
 * Purpose:  Downgrade the superblock version to v2 and
 *           downgrade persistent file space to non-persistent
 *           for 1.8 library.
 *
 * Return:   Non-negative on success/Negative on failure
 *-------------------------------------------------------------------------
 */
herr_t
H5Fformat_convert(hid_t fid)
{
    H5F_t    *f;                     /* File to flush */
    herr_t    ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", fid);

    if(H5I_FILE != H5I_get_type(fid))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

    /* Get file object */
    if(NULL == (f = (H5F_t *)H5I_object(fid)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")

    /* Set up collective metadata if appropriate */
    if(H5CX_set_loc(fid) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "can't set collective metadata read info")

    /* Call the internal routine */
    if(H5F__format_convert(f) < 0)
	HGOTO_ERROR(H5E_FILE, H5E_CANTCONVERT, FAIL, "unable to convert file format")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fformat_convert() */


/*-------------------------------------------------------------------------
 * Function:    H5Freset_page_buffering_stats
 *
 * Purpose:     Resets statistics for the page buffer layer.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *-------------------------------------------------------------------------
 */
herr_t
H5Freset_page_buffering_stats(hid_t file_id)
{
    H5F_t   *file;                      /* File to reset stats on */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object(file_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")
    if(NULL == file->shared->page_buf)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "page buffering not enabled on file")

    /* Reset the statistics */
    if(H5PB_reset_stats(file->shared->page_buf) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't reset stats for page buffering")

done:
    FUNC_LEAVE_API(ret_value)
}   /* H5Freset_page_buffering_stats() */


/*-------------------------------------------------------------------------
 * Function:    H5Fget_page_buffering_stats
 *
 * Purpose:     Retrieves statistics for the page buffer layer.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_page_buffering_stats(hid_t file_id, unsigned accesses[2], unsigned hits[2],
    unsigned misses[2], unsigned evictions[2], unsigned bypasses[2])
{
    H5F_t      *file;                   /* File object for file ID */
    herr_t     ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "i*Iu*Iu*Iu*Iu*Iu", file_id, accesses, hits, misses, evictions,
             bypasses);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")
    if(NULL == file->shared->page_buf)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "page buffering not enabled on file")
    if(NULL == accesses || NULL == hits || NULL == misses || NULL == evictions || NULL == bypasses)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL input parameters for stats")

    /* Get the statistics */
    if(H5PB_get_stats(file->shared->page_buf, accesses, hits, misses, evictions, bypasses) < 0)
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
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_mdc_image_info(hid_t file_id, haddr_t *image_addr, hsize_t *image_len)
{
    H5F_t      *file;                   /* File object for file ID */
    herr_t     ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "i*a*h", file_id, image_addr, image_len);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")
    if(NULL == image_addr || NULL == image_len)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL image addr or image len")

    /* Go get the address and size of the cache image */
    if(H5AC_get_mdc_image_info(file->shared->cache, image_addr, image_len) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTGET, FAIL, "can't retrieve cache image info")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fget_mdc_image_info() */


/*-------------------------------------------------------------------------
 * Function:    H5Fget_eoa
 *
 * Purpose:     Returns the address of the first byte after the last
 *              allocated memory in the file.
 *              (See H5FDget_eoa() in H5FD.c)
 *
 * Return:      Success:    First byte after allocated memory.
 *              Failure:    HADDR_UNDEF
 *
 * Return:      Non-negative on success/Negative on errors
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_eoa(hid_t file_id, haddr_t *eoa)
{
    H5F_t *file;                    /* File object for file ID */
    haddr_t rel_eoa;                /* Relative address of EOA */
    herr_t ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*a", file_id, eoa);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "hid_t identifier is not a file ID")

    /* This public routine will work only for drivers with this feature enabled.*/
    /* We might introduce a new feature flag in the future */
    if(!H5F_HAS_FEATURE(file, H5FD_FEAT_SUPPORTS_SWMR_IO))
        HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "must use a SWMR-compatible VFD for this public routine")

    /* The real work */
    if(HADDR_UNDEF == (rel_eoa = H5FD_get_eoa(file->shared->lf, H5FD_MEM_DEFAULT)))
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "get_eoa request failed")

    /* (Note compensating for base address subtraction in internal routine) */
    if(eoa)
        *eoa = rel_eoa + H5FD_get_base_addr(file->shared->lf);
done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fget_eoa() */


/*-------------------------------------------------------------------------
 * Function:    H5Fincrement_filesize
 *
 * Purpose:     Set the EOA for the file to the maximum of (EOA, EOF) + increment
 *
 * Return:      Non-negative on success/Negative on errors
 *-------------------------------------------------------------------------
 */
herr_t
H5Fincrement_filesize(hid_t file_id, hsize_t increment)
{
    H5F_t *file;                /* File object for file ID */
    haddr_t max_eof_eoa;        /* Maximum of the relative EOA & EOF */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "ih", file_id, increment);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "hid_t identifier is not a file ID")

    /* This public routine will work only for drivers with this feature enabled.*/
    /* We might introduce a new feature flag in the future */
    if(!H5F_HAS_FEATURE(file, H5FD_FEAT_SUPPORTS_SWMR_IO))
        HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "must use a SWMR-compatible VFD for this public routine")

    /* Get the maximum of EOA and EOF */
    if(H5F__get_max_eof_eoa(file, &max_eof_eoa) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "file can't get max eof/eoa ")

    /* Set EOA to the maximum value + increment */
    /* H5FD_set_eoa() will add base_addr to max_eof_eoa */
    if(H5FD_set_eoa(file->shared->lf, H5FD_MEM_DEFAULT, max_eof_eoa + increment) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "driver set_eoa request failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fincrement_filesize() */


/*-------------------------------------------------------------------------
 * Function: H5Fget_dset_no_attrs_hint
 *
 * Purpose:
 *
 *     Get the file-level setting to create minimized dataset object headers.
 *     Result is stored at pointer `minimize`.
 *
 * Return:
 *
 *     Success: SUCCEED (0) (non-negative value)
 *     Failure: FAIL (-1) (negative value)
 *
 * Programmer:
 *
 *     Jacob Smith
 *     15 August 2018
 *
 * Changes: None.
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_dset_no_attrs_hint(hid_t file_id, hbool_t *minimize)
{
    H5F_t  *file      = NULL;    /* File object for file ID */
    herr_t          ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*b", file_id, minimize);

    if(NULL == minimize)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "out pointer 'minimize' cannot be NULL")

    file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE);
    if(NULL == file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")

    *minimize = H5F_GET_MIN_DSET_OHDR(file);

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fget_dset_no_attrs_hint */


/*-------------------------------------------------------------------------
 * Function: H5Fset_dset_no_attrs_hint
 *
 * Purpose:
 *
 *     Set the file-level setting to create minimized dataset object headers.
 *
 * Return:
 *
 *     Success: SUCCEED (0) (non-negative value)
 *     Failure: FAIL (-1) (negative value)
 *
 * Programmer:
 *
 *     Jacob Smith
 *     15 August 2018
 *
 * Changes: None.
 *-------------------------------------------------------------------------
 */
herr_t
H5Fset_dset_no_attrs_hint(hid_t file_id, hbool_t minimize)
{
    H5F_t  *file      = NULL;    /* File object for file ID */
    herr_t  ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "ib", file_id, minimize);

    file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE);
    if(NULL == file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")

    H5F_SET_MIN_DSET_OHDR(file, minimize);

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fset_dset_no_attrs_hint */
