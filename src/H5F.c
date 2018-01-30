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
#include "H5private.h"		/* Generic Functions			*/
#include "H5Aprivate.h"		/* Attributes				*/
#include "H5ACprivate.h"        /* Metadata cache                       */
#include "H5Dprivate.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fpkg.h"             /* File access				*/
#include "H5FDprivate.h"	/* File drivers				*/
#include "H5Gprivate.h"		/* Groups				*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MFprivate.h"	/* File memory management		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Pprivate.h"		/* Property lists			*/
#include "H5SMprivate.h"	/* Shared Object Header Messages	*/
#include "H5Tprivate.h"		/* Datatypes				*/


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
    H5I_FILE,			/* ID class value */
    0,				/* Class flags */
    0,				/* # of reserved IDs for class */
    (H5I_free_t)H5F_close	/* Callback routine for closing objects of this class */
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
 * Function:	H5F_term_package
 *
 * Purpose:	Terminate this interface: free all memory and reset global
 *		variables to their initial values.  Release all ID groups
 *		associated with this interface.
 *
 * Return:	Success:	Positive if anything was done that might
 *				have affected other interfaces; zero
 *				otherwise.
 *
 *		Failure:        Never fails.
 *
 * Programmer:	Robb Matzke
 *              Friday, February 19, 1999
 *
 *-------------------------------------------------------------------------
 */
int
H5F_term_package(void)
{
    int	n = 0;

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
 * Function:	H5Fget_create_plist
 *
 * Purpose:	Get an atom for a copy of the file-creation property list for
 *		this file. This function returns an atom with a copy of the
 *		properties used to create a file.
 *
 * Return:	Success:	template ID
 *
 *		Failure:	FAIL
 *
 * Programmer:	Unknown
 *
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
 * Function:	H5Fget_access_plist
 *
 * Purpose:	Returns a copy of the file access property list of the
 *		specified file.
 *
 *              NOTE: Make sure that, if you are going to overwrite
 *              information in the copied property list that was
 *              previously opened and assigned to the property list, then
 *              you must close it before overwriting the values.
 *
 * Return:	Success:	Object ID for a copy of the file access
 *				property list.
 *
 *		Failure:	FAIL
 *
 * Programmer:	Robb Matzke
 *              Wednesday, February 18, 1998
 *
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
 * Function:	H5Fget_obj_count
 *
 * Purpose:	Public function returning the number of opened object IDs
 *		(files, datasets, groups and datatypes) in the same file.
 *
 * Return:	Non-negative on success; negative on failure.
 *
 * Programmer:	Raymond Lu
 *		Wednesday, Dec 5, 2001
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5Fget_obj_count(hid_t file_id, unsigned types)
{
    H5F_t    *f = NULL;         /* File to query */
    size_t  obj_count = 0;      /* Number of opened objects */
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
 * Function:	H5Fget_object_ids
 *
 * Purpose:	Public function to return a list of opened object IDs.
 *
 * Return:	Non-negative on success; negative on failure.
 *
 * Programmer:  Raymond Lu
 *              Wednesday, Dec 5, 2001
 *
 * Modification:
 *              Raymond Lu
 *              24 September 2008
 *              Changed the return value to ssize_t and MAX_OBJTS to size_t to
 *              accommadate potential large number of objects.
 *
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
 *
 * Programmer:  Raymond Lu
 *              Sep. 16, 2002
 *
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
 * Function:	H5Fis_hdf5
 *
 * Purpose:	Check the file signature to detect an HDF5 file.
 *
 * Bugs:	This function is not robust: it only uses the default file
 *		driver when attempting to open the file when in fact it
 *		should use all known file drivers.
 *
 * Return:	Success:	TRUE/FALSE
 *
 *		Failure:	Negative
 *
 * Programmer:	Unknown
 *
 * Modifications:
 *		Robb Matzke, 1999-08-02
 *		Rewritten to use the virtual file layer.
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
    if((ret_value = H5F__is_hdf5(name, H5AC_ind_read_dxpl_id, H5AC_rawdata_dxpl_id)) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_NOTHDF5, FAIL, "unable open file")

done:

    FUNC_LEAVE_API(ret_value)
} /* end H5Fis_hdf5() */


/*-------------------------------------------------------------------------
 * Function:	H5Fcreate
 *
 * Purpose:	This is the primary function for creating HDF5 files . The
 *		flags parameter determines whether an existing file will be
 *		overwritten or not.  All newly created files are opened for
 *		both reading and writing.  All flags may be combined with the
 *		bit-wise OR operator (`|') to change the behavior of the file
 *		create call.
 *
 *		The more complex behaviors of a file's creation and access
 *		are controlled through the file-creation and file-access
 *		property lists.  The value of H5P_DEFAULT for a template
 *		value indicates that the library should use the default
 *		values for the appropriate template.
 *
 * See also:	H5Fpublic.h for the list of supported flags. H5Ppublic.h for
 * 		the list of file creation and file access properties.
 *
 * Return:	Success:	A file ID
 *
 *		Failure:	FAIL
 *
 * Programmer:	Unknown
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Fcreate(const char *filename, unsigned flags, hid_t fcpl_id, hid_t fapl_id)
{
    hbool_t      ci_load = FALSE;       /* whether MDC ci load requested */
    hbool_t      ci_write = FALSE;      /* whether MDC CI write requested */
    H5F_t	*new_file = NULL;	/*file struct for new file	*/
    hid_t        dxpl_id = H5AC_ind_read_dxpl_id; /*dxpl used by library        */
    hid_t	 ret_value;	        /*return value			*/

    FUNC_ENTER_API(FAIL)
    H5TRACE4("i", "*sIuii", filename, flags, fcpl_id, fapl_id);

    /* Check/fix arguments */
    if(!filename || !*filename)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file name")
    /* In this routine, we only accept the following flags:
     *          H5F_ACC_EXCL, H5F_ACC_TRUNC and H5F_ACC_SWMR_WRITE
     */
    if(flags & ~(H5F_ACC_EXCL | H5F_ACC_TRUNC | H5F_ACC_SWMR_WRITE))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid flags")
    /* The H5F_ACC_EXCL and H5F_ACC_TRUNC flags are mutually exclusive */
    if((flags & H5F_ACC_EXCL) && (flags & H5F_ACC_TRUNC))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "mutually exclusive flags for file creation")

    /* Check file creation property list */
    if(H5P_DEFAULT == fcpl_id)
        fcpl_id = H5P_FILE_CREATE_DEFAULT;
    else
        if(TRUE != H5P_isa_class(fcpl_id, H5P_FILE_CREATE))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not file create property list")

    /* Verify access property list and get correct dxpl */
    if(H5P_verify_apl_and_dxpl(&fapl_id, H5P_CLS_FACC, &dxpl_id, H5I_INVALID_HID, TRUE) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "can't set access and transfer property lists")

    /*
     * Adjust bit flags by turning on the creation bit and making sure that
     * the EXCL or TRUNC bit is set.  All newly-created files are opened for
     * reading and writing.
     */
    if (0==(flags & (H5F_ACC_EXCL|H5F_ACC_TRUNC)))
        flags |= H5F_ACC_EXCL;	 /*default*/
    flags |= H5F_ACC_RDWR | H5F_ACC_CREAT;

    /*
     * Create a new file or truncate an existing file.
     */
    if(NULL == (new_file = H5F_open(filename, flags, fcpl_id, fapl_id, dxpl_id)))
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "unable to create file")

   /* Check to see if both SWMR and cache image are requested.  Fail if so */
   if(H5C_cache_image_status(new_file, &ci_load, &ci_write) < 0)
       HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't get MDC cache image status")
   if((ci_load || ci_write) && (flags & (H5F_ACC_SWMR_READ | H5F_ACC_SWMR_WRITE)))
       HGOTO_ERROR(H5E_FILE, H5E_UNSUPPORTED, FAIL, "can't have both SWMR and cache image")

    /* Get an atom for the file */
    if((ret_value = H5I_register(H5I_FILE, new_file, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to atomize file")

    /* Keep this ID in file object structure */
    new_file->file_id = ret_value;

done:
    if(ret_value < 0 && new_file && H5F_try_close(new_file, NULL) < 0)
        HDONE_ERROR(H5E_FILE, H5E_CANTCLOSEFILE, FAIL, "problems closing file")

    FUNC_LEAVE_API(ret_value)
} /* end H5Fcreate() */


/*-------------------------------------------------------------------------
 * Function:	H5Fopen
 *
 * Purpose:	This is the primary function for accessing existing HDF5
 *		files.  The FLAGS argument determines whether writing to an
 *		existing file will be allowed or not.  All flags may be
 *		combined with the bit-wise OR operator (`|') to change the
 *		behavior of the file open call.  The more complex behaviors
 *		of a file's access are controlled through the file-access
 *		property list.
 *
 * See Also:	H5Fpublic.h for a list of possible values for FLAGS.
 *
 * Return:	Success:	A file ID
 *
 *		Failure:	FAIL
 *
 * Programmer:	Unknown
 *
 * Modifications:
 *	  	Robb Matzke, 1997-07-18
 *		File struct creation and destruction is through H5F_new() and
 *		H5F__dest(). Reading the root symbol table entry is done with
 *		H5G_decode().
 *
 *  		Robb Matzke, 1997-09-23
 *		Most of the work is now done by H5F_open() since H5Fcreate()
 *		and H5Fopen() originally contained almost identical code.
 *
 *	 	Robb Matzke, 1998-02-18
 *		Added better error checking for the flags and the file access
 *		property list.  It used to be possible to make the library
 *		dump core by passing an object ID that was not a file access
 *		property list.
 *
 * 		Robb Matzke, 1999-08-02
 *		The file access property list is passed to the H5F_open() as
 *		object IDs.
 *-------------------------------------------------------------------------
 */
hid_t
H5Fopen(const char *filename, unsigned flags, hid_t fapl_id)
{
    hbool_t      ci_load = FALSE;       /* whether MDC ci load requested */
    hbool_t      ci_write = FALSE;      /* whether MDC CI write requested */
    H5F_t	*new_file = NULL;	/*file struct for new file	*/
    hid_t        dxpl_id = H5AC_ind_read_dxpl_id; /*dxpl used by library        */
    hid_t	 ret_value;	        /*return value			*/

    FUNC_ENTER_API(FAIL)
    H5TRACE3("i", "*sIui", filename, flags, fapl_id);

    /* Check/fix arguments. */
    if(!filename || !*filename)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file name")
    /* Reject undefined flags (~H5F_ACC_PUBLIC_FLAGS) and the H5F_ACC_TRUNC & H5F_ACC_EXCL flags */
    if((flags & ~H5F_ACC_PUBLIC_FLAGS) ||
            (flags & H5F_ACC_TRUNC) || (flags & H5F_ACC_EXCL))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file open flags")
    /* Asking for SWMR write access on a read-only file is invalid */
    if((flags & H5F_ACC_SWMR_WRITE) && 0 == (flags & H5F_ACC_RDWR))
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "SWMR write access on a file open for read-only access is not allowed")
    /* Asking for SWMR read access on a non-read-only file is invalid */
    if((flags & H5F_ACC_SWMR_READ) && (flags & H5F_ACC_RDWR))
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "SWMR read access on a file open for read-write access is not allowed")

    /* Verify access property list and get correct dxpl */
    if(H5P_verify_apl_and_dxpl(&fapl_id, H5P_CLS_FACC, &dxpl_id, H5I_INVALID_HID, TRUE) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "can't set access and transfer property lists")

    /* Open the file */
    if(NULL == (new_file = H5F_open(filename, flags, H5P_FILE_CREATE_DEFAULT, fapl_id, dxpl_id)))
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "unable to open file")

    /* Check to see if both SWMR and cache image are requested.  Fail if so */
    if(H5C_cache_image_status(new_file, &ci_load, &ci_write) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't get MDC cache image status")
    if((ci_load || ci_write) && (flags & (H5F_ACC_SWMR_READ | H5F_ACC_SWMR_WRITE)))
        HGOTO_ERROR(H5E_FILE, H5E_UNSUPPORTED, FAIL, "can't have both SWMR and cache image")

    /* Get an atom for the file */
    if((ret_value = H5I_register(H5I_FILE, new_file, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to atomize file handle")

    /* Keep this ID in file object structure */
    new_file->file_id = ret_value;

done:
    if(ret_value < 0 && new_file && H5F_try_close(new_file, NULL) < 0)
        HDONE_ERROR(H5E_FILE, H5E_CANTCLOSEFILE, FAIL, "problems closing file")

    FUNC_LEAVE_API(ret_value)
} /* end H5Fopen() */


/*-------------------------------------------------------------------------
 * Function:	H5Fflush
 *
 * Purpose:	Flushes all outstanding buffers of a file to disk but does
 *		not remove them from the cache.  The OBJECT_ID can be a file,
 *		dataset, group, attribute, or named data type.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, August  6, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Fflush(hid_t object_id, H5F_scope_t scope)
{
    H5F_t	*f = NULL;              /* File to flush */
    H5O_loc_t	*oloc = NULL;           /* Object location for ID */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "iFs", object_id, scope);

    switch(H5I_get_type(object_id)) {
        case H5I_FILE:
            if(NULL == (f = (H5F_t *)H5I_object(object_id)))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")
            break;

        case H5I_GROUP:
            {
                H5G_t	*grp;

                if(NULL == (grp = (H5G_t *)H5I_object(object_id)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid group identifier")
                oloc = H5G_oloc(grp);
            }
            break;

        case H5I_DATATYPE:
            {
                H5T_t	*type;

                if(NULL == (type = (H5T_t *)H5I_object(object_id)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid type identifier")
                oloc = H5T_oloc(type);
            }
            break;

        case H5I_DATASET:
            {
                H5D_t	*dset;

                if(NULL == (dset = (H5D_t *)H5I_object(object_id)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid dataset identifier")
                oloc = H5D_oloc(dset);
            }
            break;

        case H5I_ATTR:
            {
                H5A_t	*attr;

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
     * Nothing to do if the file is read only.	This determination is
     * made at the shared open(2) flags level, implying that opening a
     * file twice, once for read-only and once for read-write, and then
     * calling H5Fflush() with the read-only handle, still causes data
     * to be flushed.
     */
    if(H5F_ACC_RDWR & H5F_INTENT(f)) {
        /* Flush other files, depending on scope */
        if(H5F_SCOPE_GLOBAL == scope) {
            /* Call the flush routine for mounted file hierarchies */
            if(H5F_flush_mounts(f, H5AC_ind_read_dxpl_id, H5AC_rawdata_dxpl_id) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTFLUSH, FAIL, "unable to flush mounted file hierarchy")
        } /* end if */
        else {
            /* Call the flush routine, for this file */
            if(H5F__flush(f, H5AC_ind_read_dxpl_id, H5AC_rawdata_dxpl_id, FALSE) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTFLUSH, FAIL, "unable to flush file's cached information")
        } /* end else */
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fflush() */


/*-------------------------------------------------------------------------
 * Function:	H5Fclose
 *
 * Purpose:	This function closes the file specified by FILE_ID by
 *		flushing all data to storage, and terminating access to the
 *		file through FILE_ID.  If objects (e.g., datasets, groups,
 *		etc.) are open in the file then the underlying storage is not
 *		closed until those objects are closed; however, all data for
 *		the file and the open objects is flushed.
 *
 * Return:	Success:	Non-negative
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Saturday, February 20, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Fclose(hid_t file_id)
{
    H5F_t       *f = NULL;
    int         nref;
    herr_t	ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* Check/fix arguments. */
    if(H5I_FILE != H5I_get_type(file_id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file ID")

    /* Flush file if this is the last reference to this id and we have write
     * intent, unless it will be flushed by the "shared" file being closed.
     * This is only necessary to replicate previous behaviour, and could be
     * disabled by an option/property to improve performance. */
    if(NULL == (f = (H5F_t *)H5I_object(file_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")
    if((f->shared->nrefs > 1) && (H5F_INTENT(f) & H5F_ACC_RDWR)) {
        if((nref = H5I_get_ref(file_id, FALSE)) < 0)
            HGOTO_ERROR(H5E_ATOM, H5E_CANTGET, FAIL, "can't get ID ref count")
        if(nref == 1)
            if(H5F__flush(f, H5AC_ind_read_dxpl_id, H5AC_rawdata_dxpl_id, FALSE) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "unable to flush cache")
    } /* end if */

    /*
     * Decrement reference count on atom.  When it reaches zero the file will
     * be closed.
     */
    if(H5I_dec_app_ref(file_id) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTCLOSEFILE, FAIL, "decrementing file ID failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fclose() */


/*-------------------------------------------------------------------------
 * Function:	H5Freopen
 *
 * Purpose:	Reopen a file.  The new file handle which is returned points
 *		to the same file as the specified file handle.  Both handles
 *		share caches and other information.  The only difference
 *		between the handles is that the new handle is not mounted
 *		anywhere and no files are mounted on it.
 *
 * Return:	Success:	New file ID
 *
 *		Failure:	FAIL
 *
 * Programmer:	Robb Matzke
 *              Friday, October 16, 1998
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Freopen(hid_t file_id)
{
    H5F_t	*old_file = NULL;
    H5F_t	*new_file = NULL;
    hid_t	ret_value;

    FUNC_ENTER_API(FAIL)
    H5TRACE1("i", "i", file_id);

    /* Check arguments */
    if(NULL == (old_file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file")

    /* Get a new "top level" file struct, sharing the same "low level" file struct */
    if(NULL == (new_file = H5F_new(old_file->shared, 0, H5P_FILE_CREATE_DEFAULT, H5P_FILE_ACCESS_DEFAULT, NULL)))
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
        if(H5F__dest(new_file, H5AC_ind_read_dxpl_id, H5AC_rawdata_dxpl_id, FALSE) < 0)
            HDONE_ERROR(H5E_FILE, H5E_CANTCLOSEFILE, FAIL, "can't close file")

    FUNC_LEAVE_API(ret_value)
} /* end H5Freopen() */


/*-------------------------------------------------------------------------
 * Function:	H5Fget_intent
 *
 * Purpose:	Public API to retrieve the file's 'intent' flags passed
 *              during H5Fopen()
 *
 * Return:	Non-negative on success/negative on failure
 *
 * Programmer:	James Laird
 *		August 23, 2006
 *
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
        H5F_t * file;           /* Pointer to file structure */

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
 *
 * Programmer:  Quincey Koziol
 *              koziol@ncsa.uiuc.edu
 *              Oct  6, 2003
 *
 *-------------------------------------------------------------------------
 */
hssize_t
H5Fget_freespace(hid_t file_id)
{
    H5F_t      *file;           /* File object for file ID */
    hsize_t	tot_space;	/* Amount of free space in the file */
    hssize_t    ret_value;      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("Hs", "i", file_id);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    /* Go get the actual amount of free space in the file */
    if(H5MF_get_freespace(file, H5AC_ind_read_dxpl_id, &tot_space, NULL) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to check free space for file")

    ret_value = (hssize_t)tot_space;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_freespace() */


/*-------------------------------------------------------------------------
 * Function:    H5Fget_filesize
 *
 * Purpose:     Retrieves the file size of the HDF5 file. This function
 *              is called after an existing file is opened in order
 *		to learn the true size of the underlying file.
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  David Pitt
 *              david.pitt@bigpond.com
 *              Apr 27, 2004
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_filesize(hid_t file_id, hsize_t *size)
{
    H5F_t       *file;                  /* File object for file ID */
    haddr_t     eof;                    /* End of file address */
    haddr_t     eoa;                    /* End of allocation address */
    haddr_t     max_eof_eoa;            /* Maximum of the EOA & EOF */
    haddr_t     base_addr;              /* Base address for the file */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*h", file_id, size);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    /* Go get the actual file size */
    eof = H5FD_get_eof(file->shared->lf, H5FD_MEM_DEFAULT);
    eoa = H5FD_get_eoa(file->shared->lf, H5FD_MEM_DEFAULT);
    max_eof_eoa = MAX(eof, eoa);
    if(HADDR_UNDEF == max_eof_eoa)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "file get eof/eoa requests failed")
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
 *		big enough (size in buf_len argument), load *buf_ptr with
 *		an image of the open file whose ID is provided in the 
 *		file_id parameter, and return the number of bytes copied
 *		to the buffer.
 *
 *		If the buffer exists, but is too small to contain an image
 *		of the indicated file, return a negative number.
 *
 *		Finally, if no buffer is provided, return the size of the 
 *		buffer needed.  This value is simply the eoa of the target 
 *		file.
 *
 *		Note that any user block is skipped.
 *
 *		Also note that the function may not be used on files 
 *		opened with either the split/multi file driver or the
 *		family file driver.
 *
 *		In the former case, the sparse address space makes the 
 *		get file image operation impractical, due to the size of
 *		the image typically required.
 *
 *		In the case of the family file driver, the problem is
 *		the driver message in the super block, which will prevent
 *		the image being opened with any driver other than the
 *		family file driver -- which negates the purpose of the 
 *		operation.  This can be fixed, but no resources for 
 *		this now.
 *
 * Return:      Success:        Bytes copied / number of bytes needed.
 *              Failure:        negative value
 *
 * Programmer:  John Mainzer
 *              11/15/11
 *
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
    if((ret_value = H5F_get_file_image(file, buf_ptr, buf_len, H5AC_ind_read_dxpl_id, H5AC_rawdata_dxpl_id)) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get file image")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fget_file_image() */


/*-------------------------------------------------------------------------
 * Function:    H5Fget_mdc_config
 *
 * Purpose:     Retrieves the current automatic cache resize configuration
 *		from the metadata cache, and return it in *config_ptr.
 *
 *		Note that the version field of *config_Ptr must be correctly
 *		filled in by the caller.  This allows us to adapt for
 *		obsolete versions of the structure.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              3/24/05
 *
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
 *		configuration, using the contents of the instance of
 *		H5AC_cache_config_t pointed to by config_ptr.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              3/24/05
 *
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
 *		This rate is the overall hit rate since the last time
 *		the hit rate statistics were reset either manually or
 *		automatically.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              3/24/05
 *
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
 *		size, and current number of entries from the metadata
 *		cache associated with the specified file.  If any of
 *		the ptr parameters are NULL, the associated datum is
 *		not returned.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              3/24/05
 *
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
 *		be obtained via the H5Fget_mdc_hit_rate() call.  Note
 *		that this statistic will also be reset once per epoch
 *		by the automatic cache resize code if it is enabled.
 *
 *		It is probably a bad idea to call this function unless
 *		you are controlling cache size from your program instead
 *		of using our cache size control code.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              3/24/05
 *
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
 * Note:	This routine returns the name that was used to open the file,
 *		not the actual name after resolving symlinks, etc.
 *
 * Return:      Success:        The length of the file name
 *              Failure:        Negative
 *
 * Programmer:  Raymond Lu
 *              June 29, 2004
 *
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
        HDstrncpy(name, H5F_OPEN_NAME(f), MIN(len + 1,size));
        if(len >= size)
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
 *		1. Get storage size for superblock extension if there is one.
 *              2. Get the amount of btree and heap storage for entries
 *                 in the SOHM table if there is one.
 *		3. The amount of free space tracked in the file.
 *
 * Return:      Success:        non-negative on success
 *              Failure:        Negative
 *
 * Programmer:  Vailin Choi
 *              July 11, 2007
 *
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

    /* Reset file info struct */
    HDmemset(finfo, 0, sizeof(*finfo));

    /* Get the size of the superblock and any superblock extensions */
    if(H5F__super_size(f, H5AC_ind_read_dxpl_id, &finfo->super.super_size, &finfo->super.super_ext_size) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "Unable to retrieve superblock sizes")

    /* Get the size of any persistent free space */
    if(H5MF_get_freespace(f, H5AC_ind_read_dxpl_id, &finfo->free.tot_space, &finfo->free.meta_size) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "Unable to retrieve free space information")

    /* Check for SOHM info */
    if(H5F_addr_defined(f->shared->sohm_addr))
        if(H5SM_ih_size(f, H5AC_ind_read_dxpl_id, &finfo->sohm.hdr_size, &finfo->sohm.msgs_info) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "Unable to retrieve SOHM index & heap storage info")

    /* Set version # fields */
    finfo->super.version = f->shared->sblock->super_vers;
    finfo->sohm.version = f->shared->sohm_vers;
    finfo->free.version = HDF5_FREESPACE_VERSION;

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
 *
 * Programmer:  Vailin Choi; October 2013
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_metadata_read_retry_info(hid_t file_id, H5F_retry_info_t *info)
{
    H5F_t    	*file;           	/* File object for file ID */
    unsigned 	i, j;			/* Local index variable */
    size_t	tot_size;		/* Size of each retries[i] */
    herr_t   	ret_value = SUCCEED;   	/* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*x", file_id, info);

    /* Check args */
    if(!info)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no info struct")

    /* Get the file pointer */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    /* Copy the # of bins for "retries" array */
    info->nbins = file->shared->retries_nbins;

    /* Initialize the array of "retries" */
    HDmemset(info->retries, 0, sizeof(info->retries));

    /* Return if there are no bins -- no retries */
    if(!info->nbins) 
	HGOTO_DONE(SUCCEED);

    /* Calculate size for each retries[i] */
    tot_size = info->nbins * sizeof(uint32_t);

    /* Map and copy information to info's retries for metadata items with tracking for read retries */
    j = 0;
    for(i = 0; i < H5AC_NTYPES; i++) {
        switch(i) {
            case H5AC_OHDR_ID:
            case H5AC_OHDR_CHK_ID:
            case H5AC_BT2_HDR_ID:
            case H5AC_BT2_INT_ID:
            case H5AC_BT2_LEAF_ID:
            case H5AC_FHEAP_HDR_ID:
            case H5AC_FHEAP_DBLOCK_ID:
            case H5AC_FHEAP_IBLOCK_ID:
            case H5AC_FSPACE_HDR_ID:
            case H5AC_FSPACE_SINFO_ID:
            case H5AC_SOHM_TABLE_ID:
            case H5AC_SOHM_LIST_ID:
            case H5AC_EARRAY_HDR_ID:
            case H5AC_EARRAY_IBLOCK_ID:
            case H5AC_EARRAY_SBLOCK_ID:
            case H5AC_EARRAY_DBLOCK_ID:
            case H5AC_EARRAY_DBLK_PAGE_ID:
            case H5AC_FARRAY_HDR_ID:
            case H5AC_FARRAY_DBLOCK_ID:
            case H5AC_FARRAY_DBLK_PAGE_ID:
            case H5AC_SUPERBLOCK_ID:
                HDassert(j < H5F_NUM_METADATA_READ_RETRY_TYPES);
                if(file->shared->retries[i] != NULL) {
                    /* Allocate memory for retries[i]
                     *
                     * This memory should be released by the user with
                     * the H5free_memory() call.
                     */
                    if(NULL == (info->retries[j] = (uint32_t *)H5MM_malloc(tot_size)))
                        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

                    /* Copy the information */
                    HDmemcpy(info->retries[j], file->shared->retries[i], tot_size);
                } /* end if */

                /* Increment location in info->retries[] array */
                j++;
                break;

            default:
                break;
        } /* end switch */
    } /* end for */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fget_metadata_read_retry_info() */


/*-------------------------------------------------------------------------
 * Function:    H5Fget_free_sections
 *
 * Purpose:     To get free-space section information for free-space manager with
 *		TYPE that is associated with file FILE_ID.
 *		If SECT_INFO is null, this routine returns the total # of free-space
 *		sections.
 *
 * Return:      Success:        non-negative, the total # of free space sections
 *              Failure:        negative
 *
 * Programmer:  Vailin Choi; July 1st, 2009
 *
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

    /* Go get the free-space section information in the file */
    if((ret_value = H5MF_get_free_sections(file, H5AC_ind_read_dxpl_id, type, nsects, sect_info)) < 0)
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
 *
 * Programmer:  Neil Fortner; December 30, 2010
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Fclear_elink_file_cache(hid_t file_id)
{
    H5F_t         *file;        /* File */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* Check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a file ID")

    /* Release the EFC */
    if(file->shared->efc)
        if(H5F_efc_release(file->shared->efc) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTRELEASE, FAIL, "can't release external file cache")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fclear_elink_file_cache() */


/*-------------------------------------------------------------------------
 * Function:	H5Fstart_swmr_write
 *
 * Purpose:	To enable SWMR writing mode for the file
 *		1) Refresh opened objects: part 1
 *		2) Flush & reset accumulator
 *		3) Mark the file in SWMR writing mode
 *	 	4) Set metadata read attempts and retries info
 *		5) Disable accumulator
 *		6) Evict all cache entries except the superblock
 *		7) Refresh opened objects (part 2)
 *		8) Unlock the file
 *
 *		Pre-conditions:
 *		1) The file being opened has v3 superblock
 *		2) The file is opened with H5F_ACC_RDWR
 *		3) The file is not already marked for SWMR writing
 *		4) Current implementaion for opened objects:
 *		   --only allow datasets and groups without attributes
 *		   --disallow named datatype with/without attributes
 *		   --disallow opened attributes attached to objects
 *		NOTE: Currently, only opened groups and datasets are allowed
 *	      	when enabling SWMR via H5Fstart_swmr_write().
 *	      	Will later implement a different approach--
 *	      	set up flush dependency/proxy even for file opened without
 *	      	SWMR to resolve issues with opened objects.
 *	
 * Return:	Non-negative on success/negative on failure
 *
 * Programmer:	
 *	Vailin Choi; Feb 2014
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Fstart_swmr_write(hid_t file_id)
{
    hbool_t ci_load = FALSE;    /* whether MDC ci load requested */
    hbool_t ci_write = FALSE;   /* whether MDC CI write requested */
    H5F_t *file = NULL;         /* File info */
    size_t grp_dset_count=0; 	/* # of open objects: groups & datasets */
    size_t nt_attr_count=0; 	/* # of opened named datatypes  + opened attributes */
    hid_t *obj_ids=NULL;	/* List of ids */
    H5G_loc_t *obj_glocs=NULL;	/* Group location of the object */
    H5O_loc_t *obj_olocs=NULL;	/* Object location */
    H5G_name_t *obj_paths=NULL;	/* Group hierarchy path */
    size_t u;               	/* Local index variable */
    hbool_t setup = FALSE;	/* Boolean flag to indicate whether SWMR setting is enabled */
    H5F_io_info2_t fio_info;    /* I/O info for operation */
    herr_t ret_value = SUCCEED;	/* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* check args */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file")

    /* Should have write permission */
    if((H5F_INTENT(file) & H5F_ACC_RDWR) == 0)
        HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "no write intent on file")

    if(file->shared->sblock->super_vers < HDF5_SUPERBLOCK_VERSION_3)
        HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "file superblock version should be at least 3")
    HDassert(file->shared->latest_flags == H5F_LATEST_ALL_FLAGS);

    /* Should not be marked for SWMR writing mode already */
    if(file->shared->sblock->status_flags & H5F_SUPER_SWMR_WRITE_ACCESS)
        HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "file already in SWMR writing mode")

    HDassert(file->shared->sblock->status_flags & H5F_SUPER_WRITE_ACCESS);

    /* Check to see if cache image is enabled.  Fail if so */
    if(H5C_cache_image_status(file, &ci_load, &ci_write) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't get MDC cache image status")
    if(ci_load || ci_write )
        HGOTO_ERROR(H5E_FILE, H5E_UNSUPPORTED, FAIL, "can't have both SWMR and MDC cache image")

    /* Flush data buffers */
    if(H5F__flush(file, H5AC_ind_read_dxpl_id, H5AC_rawdata_dxpl_id, FALSE) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTFLUSH, FAIL, "unable to flush file's cached information")

    /* Get the # of opened named datatypes and attributes */
    if(H5F_get_obj_count(file, H5F_OBJ_DATATYPE|H5F_OBJ_ATTR, FALSE, &nt_attr_count) < 0)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADITER, FAIL, "H5F_get_obj_count failed")
    if(nt_attr_count)
        HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "named datatypes and/or attributes opened in the file")

    /* Get the # of opened datasets and groups */
    if(H5F_get_obj_count(file, H5F_OBJ_GROUP|H5F_OBJ_DATASET, FALSE, &grp_dset_count) < 0)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADITER, FAIL, "H5F_get_obj_count failed")

    if(grp_dset_count) {
        /* Allocate space for group and object locations */
	if((obj_ids = (hid_t *) H5MM_malloc(grp_dset_count * sizeof(hid_t))) == NULL)
	    HGOTO_ERROR(H5E_FILE, H5E_NOSPACE, FAIL, "can't allocate buffer for hid_t")
	if((obj_glocs = (H5G_loc_t *) H5MM_malloc(grp_dset_count * sizeof(H5G_loc_t))) == NULL)
	    HGOTO_ERROR(H5E_FILE, H5E_NOSPACE, FAIL, "can't allocate buffer for H5G_loc_t")
	if((obj_olocs = (H5O_loc_t *) H5MM_malloc(grp_dset_count * sizeof(H5O_loc_t))) == NULL)
	    HGOTO_ERROR(H5E_FILE, H5E_NOSPACE, FAIL, "can't allocate buffer for H5O_loc_t")
	if((obj_paths = (H5G_name_t *) H5MM_malloc(grp_dset_count * sizeof(H5G_name_t))) == NULL)
	    HGOTO_ERROR(H5E_FILE, H5E_NOSPACE, FAIL, "can't allocate buffer for H5G_name_t")

        /* Get the list of opened object ids (groups & datasets) */
	if(H5F_get_obj_ids(file, H5F_OBJ_GROUP|H5F_OBJ_DATASET, grp_dset_count, obj_ids, FALSE, &grp_dset_count) < 0)
	    HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "H5F_get_obj_ids failed")

        /* Refresh opened objects (groups, datasets) in the file */
        for(u = 0; u < grp_dset_count; u++) {
            H5O_loc_t *oloc;            /* object location */
            H5G_loc_t tmp_loc;

            /* Set up the id's group location */
            obj_glocs[u].oloc = &obj_olocs[u];
            obj_glocs[u].path = &obj_paths[u];
            H5G_loc_reset(&obj_glocs[u]);

            /* get the id's object location */
            if((oloc = H5O_get_loc(obj_ids[u])) == NULL)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an object")

            /* Make deep local copy of object's location information */
            H5G_loc(obj_ids[u], &tmp_loc);
            H5G_loc_copy(&obj_glocs[u], &tmp_loc, H5_COPY_DEEP);

            /* Close the object */
            if(H5I_dec_ref(obj_ids[u]) < 0)
                HGOTO_ERROR(H5E_ATOM, H5E_CANTCLOSEOBJ, FAIL, "decrementing object ID failed")
        } /* end for */
    } /* end if */

    /* Set up I/O info for operation */
    fio_info.f = file;
    if(NULL == (fio_info.meta_dxpl = (H5P_genplist_t *)H5I_object(H5AC_ind_read_dxpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "can't get property list")
    if(NULL == (fio_info.raw_dxpl = (H5P_genplist_t *)H5I_object(H5AC_rawdata_dxpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "can't get property list")

    /* Flush and reset the accumulator */
    if(H5F__accum_reset(&fio_info, TRUE) < 0)
        HGOTO_ERROR(H5E_IO, H5E_CANTRESET, FAIL, "can't reset accumulator")

    /* Turn on SWMR write in shared file open flags */
    file->shared->flags |= H5F_ACC_SWMR_WRITE;

    /* Mark the file in SWMR writing mode */
    file->shared->sblock->status_flags |= H5F_SUPER_SWMR_WRITE_ACCESS;

    /* Set up metadata read attempts */
    file->shared->read_attempts = H5F_SWMR_METADATA_READ_ATTEMPTS;

    /* Initialize "retries" and "retries_nbins" */
    if(H5F_set_retries(file) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "can't set retries and retries_nbins")

    /* Turn off usage of accumulator */
    file->shared->feature_flags &= ~(unsigned)H5FD_FEAT_ACCUMULATE_METADATA;
    if(H5FD_set_feature_flags(file->shared->lf, file->shared->feature_flags) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "can't set feature_flags in VFD")

    setup = TRUE;

    /* Mark superblock as dirty */
    if(H5F_super_dirty(file) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTMARKDIRTY, FAIL, "unable to mark superblock as dirty")

    /* Flush the superblock */
    if(H5F_flush_tagged_metadata(file, H5AC__SUPERBLOCK_TAG, H5AC_ind_read_dxpl_id) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTFLUSH, FAIL, "unable to flush superblock")

    /* Evict all flushed entries in the cache except the pinned superblock */
    if(H5F__evict_cache_entries(file, H5AC_ind_read_dxpl_id) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTFLUSH, FAIL, "unable to evict file's cached information")

    /* Refresh (reopen) the objects (groups & datasets) in the file */
    for(u = 0; u < grp_dset_count; u++)
        if(H5O_refresh_metadata_reopen(obj_ids[u], &obj_glocs[u], H5AC_ind_read_dxpl_id, TRUE) < 0)
            HGOTO_ERROR(H5E_ATOM, H5E_CLOSEERROR, FAIL, "can't refresh-close object")

    /* Unlock the file */
    if(H5FD_unlock(file->shared->lf) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "unable to unlock the file")

done:
    if(ret_value < 0 && setup) {
        HDassert(file);

        /* Re-enable accumulator */
        file->shared->feature_flags |= (unsigned)H5FD_FEAT_ACCUMULATE_METADATA;
        if(H5FD_set_feature_flags(file->shared->lf, file->shared->feature_flags) < 0)
            HDONE_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "can't set feature_flags in VFD")

        /* Reset the # of read attempts */
        file->shared->read_attempts = H5F_METADATA_READ_ATTEMPTS;
        if(H5F_set_retries(file) < 0)
            HDONE_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "can't set retries and retries_nbins")

        /* Un-set H5F_ACC_SWMR_WRITE in shared open flags */
        file->shared->flags &= ~H5F_ACC_SWMR_WRITE;

        /* Unmark the file: not in SWMR writing mode */
        file->shared->sblock->status_flags &= (uint8_t)(~H5F_SUPER_SWMR_WRITE_ACCESS);

        /* Mark superblock as dirty */
        if(H5F_super_dirty(file) < 0)
            HDONE_ERROR(H5E_FILE, H5E_CANTMARKDIRTY, FAIL, "unable to mark superblock as dirty")

        /* Flush the superblock */
        if(H5F_flush_tagged_metadata(file, H5AC__SUPERBLOCK_TAG, H5AC_ind_read_dxpl_id) < 0)
            HDONE_ERROR(H5E_FILE, H5E_CANTFLUSH, FAIL, "unable to flush superblock")
    } /* end if */

    /* Free memory */
    if(obj_ids)
        H5MM_xfree(obj_ids);
    if(obj_glocs)
        H5MM_xfree(obj_glocs);
    if(obj_olocs)
        H5MM_xfree(obj_olocs);
    if(obj_paths)
        H5MM_xfree(obj_paths);

    FUNC_LEAVE_API(ret_value)
} /* end H5Fstart_swmr_write() */


/*-------------------------------------------------------------------------
 * Function:    H5Fstart_mdc_logging
 *
 * Purpose:     Start metadata cache logging operations for a file.
 *                  - Logging must have been set up via the fapl.
 *
 * Return:      Non-negative on success/Negative on errors
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Fstart_mdc_logging(hid_t file_id)
{
    H5F_t *file;                /* File info */
    herr_t ret_value = SUCCEED;	/* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* Sanity check */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "hid_t identifier is not a file ID")

    /* Call mdc logging function */
    if(H5C_start_logging(file->shared->cache) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_LOGFAIL, FAIL, "unable to start mdc logging")

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
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Fstop_mdc_logging(hid_t file_id)
{
    H5F_t *file;                /* File info */
    herr_t ret_value = SUCCEED;	/* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", file_id);

    /* Sanity check */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "hid_t identifier is not a file ID")

    /* Call mdc logging function */
    if(H5C_stop_logging(file->shared->cache) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_LOGFAIL, FAIL, "unable to stop mdc logging")

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
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Fget_mdc_logging_status(hid_t file_id, hbool_t *is_enabled,
                          hbool_t *is_currently_logging)
{
    H5F_t *file;                /* File info */
    herr_t ret_value = SUCCEED;	/* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "i*b*b", file_id, is_enabled, is_currently_logging);

    /* Sanity check */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "hid_t identifier is not a file ID")

    /* Call mdc logging function */
    if(H5C_get_logging_status(file->shared->cache, is_enabled, is_currently_logging) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_LOGFAIL, FAIL, "unable to get logging status")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Fget_mdc_logging_status() */


/*-------------------------------------------------------------------------
 * Function:   H5Fset_latest_format
 *
 * Purpose:    Enable switching the "latest format" flag while a file is open.
 *
 * Return:     Non-negative on success/Negative on failure
 *
 * Programmer: Quincey Koziol
 *             Monday, September 21, 2015
 *-------------------------------------------------------------------------
 */
herr_t
H5Fset_latest_format(hid_t file_id, hbool_t latest_format)
{
    H5F_t *f;                           /* File */
    unsigned latest_flags;              /* Latest format flags for file */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "ib", file_id, latest_format);

    /* Check args */
    if(NULL == (f = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "not a file ID")

    /* Check if the value is changing */
    latest_flags = H5F_USE_LATEST_FLAGS(f, H5F_LATEST_ALL_FLAGS);
    if(latest_format != (H5F_LATEST_ALL_FLAGS == latest_flags)) {
        /* Call the flush routine, for this file */
        if(H5F__flush(f, H5AC_ind_read_dxpl_id, H5AC_rawdata_dxpl_id, FALSE) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTFLUSH, FAIL, "unable to flush file's cached information")

        /* Toggle the 'latest format' flag */
        H5F_SET_LATEST_FLAGS(f, latest_format ? H5F_LATEST_ALL_FLAGS : 0);
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fset_latest_format() */


/*-------------------------------------------------------------------------
 * Function:	H5Fformat_convert_super (Internal)
 *
 * Purpose:	Downgrade the superblock version to v2 and
 *		downgrade persistent file space to non-persistent
 *		for 1.8 library.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Vailin Choi
 *              Jan 2016
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Fformat_convert(hid_t fid)
{
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", fid);

    if(H5I_FILE == H5I_get_type(fid)) {
        H5F_t	*f;                     /* File to flush */
        hbool_t	mark_dirty = FALSE;

        /* Get file object */
        if(NULL == (f = (H5F_t *)H5I_object(fid)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")

        /* Check if the superblock should be downgraded */
        if(f->shared->sblock->super_vers > HDF5_SUPERBLOCK_VERSION_V18_LATEST) {
            f->shared->sblock->super_vers = HDF5_SUPERBLOCK_VERSION_V18_LATEST;
            mark_dirty = TRUE;
        } /* end if */

        /* Check for persistent freespace manager, which needs to be downgraded */
        if(!(f->shared->fs_strategy == H5F_FILE_SPACE_STRATEGY_DEF &&
             f->shared->fs_persist == H5F_FREE_SPACE_PERSIST_DEF &&
             f->shared->fs_threshold == H5F_FREE_SPACE_THRESHOLD_DEF &&
             f->shared->fs_page_size == H5F_FILE_SPACE_PAGE_SIZE_DEF)) {
            /* Check to remove free-space manager info message from superblock extension */
            if(H5F_addr_defined(f->shared->sblock->ext_addr))
                if(H5F_super_ext_remove_msg(f, H5AC_ind_read_dxpl_id, H5O_FSINFO_ID) < 0)
                    HGOTO_ERROR(H5E_FILE, H5E_CANTRELEASE, FAIL, "error in removing message from superblock extension")

            /* Close freespace manager */
            if(H5MF_try_close(f, H5AC_ind_read_dxpl_id) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTRELEASE, FAIL, "unable to free free-space address")

            /* Set non-persistent freespace manager */
            f->shared->fs_strategy = H5F_FILE_SPACE_STRATEGY_DEF;
            f->shared->fs_persist = H5F_FREE_SPACE_PERSIST_DEF;
            f->shared->fs_threshold = H5F_FREE_SPACE_THRESHOLD_DEF;
            f->shared->fs_page_size = H5F_FILE_SPACE_PAGE_SIZE_DEF;

            /* Indicate that the superblock should be marked dirty */
            mark_dirty = TRUE;
        } /* end if */

        /* Check if we should mark the superblock dirty */
        if(mark_dirty)
            /* Mark superblock as dirty */
            if(H5F_super_dirty(f) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTMARKDIRTY, FAIL, "unable to mark superblock as dirty")
    } /* end if */
    else
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

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
 *
 * Programmer:  Mohamad Chaarawi
 *
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
 *
 * Programmer:  Mohamad Chaarawi
 *
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
 *
 * Programmer:  Vailin Choi; March 2017
 *
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

