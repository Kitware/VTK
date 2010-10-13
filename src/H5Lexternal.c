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

#define H5G_PACKAGE		/*suppress error about including H5Gpkg   */
#define H5L_PACKAGE		/*suppress error about including H5Lpkg   */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5L_init_extern_interface

#include "H5private.h"          /* Generic Functions                    */
#include "H5ACprivate.h"        /* Metadata cache                       */
#include "H5Eprivate.h"         /* Error handling                       */
#include "H5Gpkg.h"             /* Groups                               */
#include "H5Iprivate.h"		/* IDs					*/
#include "H5Lpkg.h"             /* Links                                */
#include "H5MMprivate.h"        /* Memory management                    */
#include "H5Opublic.h"          /* File objects                         */
#include "H5Pprivate.h"         /* Property lists                       */

static hid_t H5L_extern_traverse(const char UNUSED *link_name, hid_t cur_group,
    const void *udata, size_t UNUSED udata_size, hid_t lapl_id);
static ssize_t H5L_extern_query(const char UNUSED * link_name, const void *udata,
    size_t udata_size, void * buf /*out*/, size_t buf_size);

/* Default External Link link class */
const H5L_class_t H5L_EXTERN_LINK_CLASS[1] = {{
    H5L_LINK_CLASS_T_VERS,      /* H5L_class_t version            */
    H5L_TYPE_EXTERNAL,		/* Link type id number            */
    "external",                 /* Link name for debugging        */
    NULL,                       /* Creation callback              */
    NULL,                       /* Move callback                  */
    NULL,                       /* Copy callback                  */
    H5L_extern_traverse,        /* The actual traversal function  */
    NULL,                       /* Deletion callback              */
    H5L_extern_query            /* Query callback                 */
}};

/* Version of external link format */
#define H5L_EXT_VERSION         0

/* Valid flags for external links */
#define H5L_EXT_FLAGS_ALL       0


/*--------------------------------------------------------------------------
NAME
   H5L_init_extern_interface -- Initialize interface-specific information
USAGE
    herr_t H5L_init_extern_interface()

RETURNS
    Non-negative on success/Negative on failure

DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5L_init() currently).

--------------------------------------------------------------------------*/
static herr_t
H5L_init_extern_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5L_init_extern_interface)

    FUNC_LEAVE_NOAPI(H5L_init())
} /* H5L_init_extern_interface() */



/*--------------------------------------------------------------------------
 * Function: H5L_getenv_prefix_name --
 *
 * Purpose:  Get the first pathname in the list of pathnames stored in ENV_PREFIX,
 *           which is separated by the environment delimiter.
 *           ENV_PREFIX is modified to point to the remaining pathnames
 *           in the list.
 *
 * Return:   A pointer to a pathname
 *
 * Programmer:	Vailin Choi, April 2, 2008
 *
--------------------------------------------------------------------------*/
static char *
H5L_getenv_prefix_name(char **env_prefix/*in,out*/)
{
    char        *retptr=NULL;
    char        *strret=NULL;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5L_getenv_prefix_name)

    strret = HDstrchr(*env_prefix, COLON_SEPC);
    if (strret == NULL) {
        retptr = *env_prefix;
        *env_prefix = strret;
    } else {
        retptr = *env_prefix;
        *env_prefix = strret + 1;
        *strret = '\0';
    }

    FUNC_LEAVE_NOAPI(retptr)
} /* end H5L_getenv_prefix_name() */


/*--------------------------------------------------------------------------
 * Function: H5L_build_name
 *
 * Purpose:  Prepend PREFIX to FILE_NAME and store in FULL_NAME
 *
 * Return:   Non-negative on success/Negative on failure
 *
 * Programmer:	Vailin Choi, April 2, 2008
 *
 * Modification: Raymond Lu, 14 Jan. 2009
 *           Added support for OpenVMS pathname
--------------------------------------------------------------------------*/
static herr_t
H5L_build_name(char *prefix, char *file_name, char **full_name/*out*/)
{
    size_t      prefix_len;             /* length of prefix */
    size_t      fname_len;              /* Length of external link file name */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5L_build_name)

    prefix_len = HDstrlen(prefix);
    fname_len = HDstrlen(file_name);

    /* Allocate a buffer to hold the filename + prefix + possibly the delimiter + terminating null byte */
    if(NULL == (*full_name = (char *)H5MM_malloc(prefix_len + fname_len + 2)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate filename buffer")

    /* Copy the prefix into the buffer */
    HDstrcpy(*full_name, prefix);
#ifndef H5_VMS
    if (!CHECK_DELIMITER(prefix[prefix_len-1]))
        HDstrcat(*full_name, DIR_SEPS);
#endif

    /* Add the external link's filename to the prefix supplied */
    HDstrcat(*full_name, file_name);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5L_build_name() */


/*-------------------------------------------------------------------------
 * Function:	H5L_extern_traverse
 *
 * Purpose:	Default traversal function for external links. This can
 *              be overridden using H5Lregister().
 *
 *              Given a filename and path packed into the link udata,
 *              attempts to open an object within an external file.
 *              If the H5L_ELINK_PREFIX_NAME property is set in the
 *              link access property list, appends that prefix to the
 *              filename being opened.
 *
 * Return:	ID of the opened object on success/Negative on failure
 *
 * Programmer:	James Laird
 *              Monday, July 10, 2006
 * Modifications:
 *		Vailin Choi, April 2, 2008
 *		Add handling to search for the target file
 *		See description in RM: H5Lcreate_external
 *
 *		Vailin Choi; Sept. 12th, 2008; bug #1247
 *		Retrieve the file access property list identifer that is set
 *		for link access property via H5Pget_elink_fapl().
 *		If the return value is H5P_DEFAULT, the parent's file access
 *		property is used to H5F_open() the target file;
 *		Otherwise, the file access property retrieved from H5Pget_elink_fapl()
 *		is used to H5F_open() the target file.
 *
 *-------------------------------------------------------------------------
 */
static hid_t
H5L_extern_traverse(const char UNUSED *link_name, hid_t cur_group,
    const void *_udata, size_t UNUSED udata_size, hid_t lapl_id)
{
    H5P_genplist_t *plist;              /* Property list pointer */
    char       *my_prefix;              /* Library's copy of the prefix */
    H5G_loc_t   root_loc;               /* Location of root group in external file */
    H5G_loc_t   loc;                    /* Location of object */
    H5F_t	*ext_file = NULL;	/* File struct for external file */
    const uint8_t *p = (const uint8_t *)_udata;  /* Pointer into external link buffer */
    const char *file_name;              /* Name of file containing external link's object */
    char *full_name = NULL;             /* File name with prefix */
    const char  *obj_name;              /* Name external link's object */
    size_t      fname_len;              /* Length of external link file name */
    unsigned    intent;                 /* File access permissions */
    H5L_elink_cb_t cb_info;             /* Callback info struct */
    hid_t       fapl_id = -1;           /* File access property list for external link's file */
    hid_t       ext_obj = -1;           /* ID for external link's object */
    char        *temp_group_name = NULL;/* Temporary pointer to group name */
    char        *temp_file_name = NULL; /* Temporary pointer to file name */
    char        *actual_file_name = NULL; /* Parent file's actual name */
    H5P_genplist_t  *fa_plist;          /* File access property list pointer */
    H5F_close_degree_t 	fc_degree = H5F_CLOSE_WEAK;  /* File close degree for target file */
    hid_t       ret_value;              /* Return value */

    FUNC_ENTER_NOAPI(H5L_extern_traverse, FAIL)

    /* Sanity checks */
    HDassert(p);

    /* Check external link version & flags */
    if(((*p >> 4) & 0x0F) > H5L_EXT_VERSION)
        HGOTO_ERROR(H5E_LINK, H5E_CANTDECODE, FAIL, "bad version number for external link")
    if((*p & 0x0F) & ~H5L_EXT_FLAGS_ALL)
        HGOTO_ERROR(H5E_LINK, H5E_CANTDECODE, FAIL, "bad flags for external link")
    p++;

    /* Gather some information from the external link's user data */
    file_name = (const char *)p;
    fname_len = HDstrlen(file_name);
    obj_name = (const char *)p + fname_len + 1;

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(lapl_id, H5P_LINK_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Get the fapl_id set for lapl_id if any */
    if(H5P_get(plist, H5L_ACS_ELINK_FAPL_NAME, &fapl_id) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get fapl for links")

    /* Get the location for the group holding the external link */
    if(H5G_loc(cur_group, &loc) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't get object location")

    /* get the access flags set for lapl_id if any */
    if(H5P_get(plist, H5L_ACS_ELINK_FLAGS_NAME, &intent) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get elink file access flags")

    /* get the file access mode flags for the parent file, if they were not set
     * on lapl_id */
    if(intent == H5F_ACC_DEFAULT)
        intent = H5F_INTENT(loc.oloc->file);

    if((fapl_id == H5P_DEFAULT) && ((fapl_id = H5F_get_access_plist(loc.oloc->file, FALSE)) < 0))
	HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't get parent's file access property list")

    /* Get callback_info */
    if(H5P_get(plist, H5L_ACS_ELINK_CB_NAME, &cb_info)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get elink callback info")

    /* Get file access property list */
    if(NULL == (fa_plist = H5P_object_verify(fapl_id, H5P_FILE_ACCESS)))
	HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Make callback if it exists */
    if(cb_info.func) {
        const char  *parent_file_name;  /* Parent file name */
        const char  *parent_group_name; /* Parent group name */

        /* Get parent file name */
        parent_file_name = H5F_OPEN_NAME(loc.oloc->file);

        /* Get parent group name */
        if(loc.path->user_path_r != NULL && loc.path->obj_hidden == 0)
            /* Use user_path_r if possible */
            parent_group_name = H5RS_get_str(loc.path->user_path_r);
        else {
            /* Otherwise use H5G_get_name */
            ssize_t group_name_len; /* Length of parent group name */

            /* Get length of parent group name */
            if((group_name_len = H5G_get_name(cur_group, NULL, (size_t) 0, lapl_id, H5AC_ind_dxpl_id)) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "unable to retrieve length of group name")

            /* account for null terminator */
            group_name_len++;

            /* Copy parent group name */
            if(NULL == (temp_group_name = (char *)H5MM_malloc((size_t)group_name_len)))
                HGOTO_ERROR(H5E_LINK, H5E_NOSPACE, FAIL, "memory allocation failed")
            if(H5G_get_name(cur_group, temp_group_name, (size_t) group_name_len, lapl_id, H5AC_ind_dxpl_id) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "unable to retrieve group name")
            parent_group_name = temp_group_name;
        } /* end else */

        /* Make callback */
        if((cb_info.func)(parent_file_name, parent_group_name, file_name, obj_name, &intent, fapl_id, cb_info.user_data) < 0)
            HGOTO_ERROR(H5E_LINK, H5E_CALLBACK, FAIL, "traversal operator failed")

        /* Free temp_group_name */
        temp_group_name = (char *)H5MM_xfree(temp_group_name);

        /* Check access flags */
        if((intent & H5F_ACC_TRUNC) || (intent & H5F_ACC_EXCL))
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file open flags")
    } /* end if */

    /* Set file close degree for new file to "weak" */
    if(H5P_set(fa_plist, H5F_ACS_CLOSE_DEGREE_NAME, &fc_degree) < 0)
	HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set file close degree")

    /*
     * Start searching for the target file
     */

    /* Simplify intent flags for open calls */
    intent = ((intent & H5F_ACC_RDWR) ? H5F_ACC_RDWR : H5F_ACC_RDONLY);

    /* Copy the file name to use */
    if(NULL == (temp_file_name = H5MM_strdup(file_name)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* target file_name is an absolute pathname: see RM for detailed description */
    if(CHECK_ABSOLUTE(file_name) || CHECK_ABS_PATH(file_name)) {
        /* Try opening file */
        if(NULL == (ext_file = H5F_open(file_name, intent, H5P_FILE_CREATE_DEFAULT, fapl_id, H5AC_dxpl_id))) {
            char *ptr = NULL;

            H5E_clear_stack(NULL);

            /* get last component of file_name */
	    GET_LAST_DELIMITER(file_name, ptr)
	    HDassert(ptr);
	    HDstrcpy(temp_file_name, ++ptr);
        } /* end if */
    } /* end if */
    else if(CHECK_ABS_DRIVE(file_name)) {
        if(NULL == (ext_file = H5F_open(file_name, intent, H5P_FILE_CREATE_DEFAULT, fapl_id, H5AC_dxpl_id))) {
            H5E_clear_stack(NULL);
	    /* strip "<drive-letter>:" */
	    HDstrcpy(temp_file_name, &file_name[2]);
	} /* end if */
    } /* end if */

    /* try searching from paths set in the environment variable */
    if(ext_file == NULL) {
        char *env_prefix;

        if(NULL != (env_prefix = HDgetenv("HDF5_EXT_PREFIX"))) {
            char *tmp_env_prefix;

            if(NULL == (tmp_env_prefix = H5MM_strdup(env_prefix)))
                HGOTO_ERROR(H5E_LINK, H5E_NOSPACE, FAIL, "memory allocation failed")

            while((tmp_env_prefix) && (*tmp_env_prefix)) {
                char *out_prefix_name;

                out_prefix_name = H5L_getenv_prefix_name(&tmp_env_prefix/*in,out*/);
                if(out_prefix_name && (*out_prefix_name)) {
                    if(H5L_build_name(out_prefix_name, temp_file_name, &full_name/*out*/) < 0)
                        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't prepend prefix to filename")

                    ext_file = H5F_open(full_name, intent, H5P_FILE_CREATE_DEFAULT, fapl_id, H5AC_dxpl_id);
                    full_name = (char *)H5MM_xfree(full_name);
                    if(ext_file != NULL)
                        break;
                    H5E_clear_stack(NULL);
                } /* end if */
            } /* end while */
        } /* end if */
    } /* end if */

    /* try searching from property list */
    if(ext_file == NULL) {
        if(H5P_get(plist, H5L_ACS_ELINK_PREFIX_NAME, &my_prefix) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get external link prefix")
        if(my_prefix) {
            if(H5L_build_name(my_prefix, temp_file_name, &full_name/*out*/) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't prepend prefix to filename")
            if(NULL == (ext_file = H5F_open(full_name, intent, H5P_FILE_CREATE_DEFAULT, fapl_id, H5AC_dxpl_id)))
                H5E_clear_stack(NULL);
            full_name = (char *)H5MM_xfree(full_name);
        } /* end if */
    } /* end if */

    /* try searching from main file's "extpath": see description in H5F_open() & H5_build_extpath() */
    if(ext_file == NULL) {
        char *extpath;

        if(NULL != (extpath = H5F_EXTPATH(loc.oloc->file))) {
            if(H5L_build_name(extpath, temp_file_name, &full_name/*out*/) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't prepend prefix to filename")
            if(NULL == (ext_file = H5F_open(full_name, intent, H5P_FILE_CREATE_DEFAULT, fapl_id, H5AC_dxpl_id)))
                H5E_clear_stack(NULL);
            full_name = (char *)H5MM_xfree(full_name);
        } /* end if */
    } /* end if */

    /* try the relative file_name stored in temp_file_name */
    if(ext_file == NULL) {
        if(NULL == (ext_file = H5F_open(temp_file_name, intent, H5P_FILE_CREATE_DEFAULT, fapl_id, H5AC_dxpl_id)))
            H5E_clear_stack(NULL);
    } /* end if */

    /* try the 'resolved' name for the parent file (i.e. the name after symlinks
     * were resolved)
     */
    if(ext_file == NULL) {
        char *ptr = NULL;

        /* Copy resolved file name */
        if(NULL == (actual_file_name = H5MM_strdup(H5F_ACTUAL_NAME(loc.oloc->file))))
            HGOTO_ERROR(H5E_LINK, H5E_CANTALLOC, FAIL, "can't duplicate resolved file name string")

        /* get last component of file_name */
        GET_LAST_DELIMITER(actual_file_name, ptr)
        if(!ptr)
            HGOTO_ERROR(H5E_LINK, H5E_CANTOPENFILE, FAIL, "unable to open external file, external link file name = '%s', temp_file_name = '%s'", file_name, temp_file_name)

        /* Truncate filename portion from actual file name path */
        *ptr = '\0';

        /* Build new file name for the external file */
        if(H5L_build_name(actual_file_name, temp_file_name, &full_name/*out*/) < 0)
            HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't prepend prefix to filename")

        /* Try opening with the resolved name */
        if(NULL == (ext_file = H5F_open(full_name, intent, H5P_FILE_CREATE_DEFAULT, fapl_id, H5AC_dxpl_id)))
            HGOTO_ERROR(H5E_LINK, H5E_CANTOPENFILE, FAIL, "unable to open external file, external link file name = '%s', temp_file_name = '%s'", file_name, temp_file_name)
        full_name = (char *)H5MM_xfree(full_name);
    } /* end if */


    /* Increment the number of open objects, to hold the file open */
    H5F_incr_nopen_objs(ext_file);

    /* Retrieve the "group location" for the file's root group */
    if(H5G_loc_root(ext_file, &root_loc) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_BADVALUE, FAIL, "unable to create location for file")

    /* Open the object referenced in the external file */
    if((ext_obj = H5O_open_name(&root_loc, obj_name, lapl_id, FALSE)) < 0) {
        H5F_decr_nopen_objs(ext_file);
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to open object")
    } /* end if */

    /* Decrement the number of open objects, to let the file close */
    H5F_decr_nopen_objs(ext_file);

    /* Close the external file */
    if(H5F_try_close(ext_file) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTCLOSEFILE, FAIL, "problem closing external file")
    ext_file = NULL;

    /* Set return value */
    ret_value = ext_obj;

done:
    /* Release resources */
    if(fapl_id > 0 && H5I_dec_ref(fapl_id, FALSE) < 0)
        HDONE_ERROR(H5E_ATOM, H5E_CANTRELEASE, FAIL, "unable to close atom for file access property list")
    if(ext_file && H5F_try_close(ext_file) < 0)
        HDONE_ERROR(H5E_LINK, H5E_CANTCLOSEFILE, FAIL, "problem closing external file")
    full_name = (char *)H5MM_xfree(full_name);
    temp_group_name = (char *)H5MM_xfree(temp_group_name);
    temp_file_name = (char *)H5MM_xfree(temp_file_name);
    actual_file_name = (char *)H5MM_xfree(actual_file_name);

    if(ret_value < 0) {
        /* Close object if it's open and something failed */
        if(ext_obj >= 0 && H5I_dec_ref(ext_obj, FALSE) < 0)
            HDONE_ERROR(H5E_ATOM, H5E_CANTRELEASE, FAIL, "unable to close atom for external object")
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_extern_traverse() */


/*-------------------------------------------------------------------------
 * Function:	H5L_extern_query
 *
 * Purpose:	Default query function for external links. This can
 *              be overridden using H5Lregister().
 *
 *              Returns the size of the link's user data. If a buffer of
 *              is provided, copies at most buf_size bytes of the udata
 *              into it.
 *
 * Return:	Size of buffer on success/Negative on failure
 *
 * Programmer:	James Laird
 *              Monday, July 10, 2006
 *
 *-------------------------------------------------------------------------
 */
static ssize_t
H5L_extern_query(const char UNUSED * link_name, const void *_udata, size_t udata_size,
    void *buf /*out*/, size_t buf_size)
{
    const uint8_t *udata = (const uint8_t *)_udata;      /* Pointer to external link buffer */
    ssize_t     ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5L_extern_query)

    /* Check external link version & flags */
    if(((*udata >> 4) & 0x0F) != H5L_EXT_VERSION)
        HGOTO_ERROR(H5E_LINK, H5E_CANTDECODE, FAIL, "bad version number for external link")
    if((*udata & 0x0F) & ~H5L_EXT_FLAGS_ALL)
        HGOTO_ERROR(H5E_LINK, H5E_CANTDECODE, FAIL, "bad flags for external link")

    /* If the buffer is NULL, skip writing anything in it and just return
     * the size needed */
    if(buf) {
        if(udata_size < buf_size)
            buf_size = udata_size;

        /* Copy the udata verbatim up to buf_size */
        HDmemcpy(buf, udata, buf_size);
    } /* end if */

    /* Set return value */
    ret_value = (ssize_t)udata_size;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_extern_query() */


/*-------------------------------------------------------------------------
 * Function:	H5Lcreate_external
 *
 * Purpose:	Creates an external link from LINK_NAME to OBJ_NAME.
 *
 *              External links are links to objects in other HDF5 files.  They
 *              are allowed to "dangle" like soft links internal to a file.
 *              FILE_NAME is the name of the file that OBJ_NAME is is contained
 *              within.  If OBJ_NAME is given as a relative path name, the
 *              path will be relative to the root group of FILE_NAME.
 *		LINK_NAME is interpreted relative to LINK_LOC_ID, which is
 *              either a file ID or a group ID.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Wednesday, May 18, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lcreate_external(const char *file_name, const char *obj_name,
    hid_t link_loc_id, const char *link_name, hid_t lcpl_id, hid_t lapl_id)
{
    H5G_loc_t	link_loc;               /* Group location to create link */
    char       *norm_obj_name = NULL;	/* Pointer to normalized current name */
    void       *ext_link_buf = NULL;    /* Buffer to contain external link */
    size_t      buf_size;               /* Size of buffer to hold external link */
    uint8_t    *p;                      /* Pointer into external link buffer */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(H5Lcreate_external, FAIL)
    H5TRACE6("e", "*s*si*sii", file_name, obj_name, link_loc_id, link_name,
             lcpl_id, lapl_id);

    /* Check arguments */
    if(!file_name || !*file_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no file name specified")
    if(!obj_name || !*obj_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no object name specified")
    if(H5G_loc(link_loc_id, &link_loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!link_name || !*link_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no link name specified")

    /* Get normalized copy of the link target */
    if(NULL == (norm_obj_name = H5G_normalize(obj_name)))
        HGOTO_ERROR(H5E_SYM, H5E_BADVALUE, FAIL, "can't normalize object name")

    /* Combine the filename and link name into a single buffer to give to the UD link */
    buf_size = 1 + (HDstrlen(file_name) + 1) + (HDstrlen(norm_obj_name) + 1);
    if(NULL == (ext_link_buf = H5MM_malloc(buf_size)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate udata buffer")

    /* Encode the external link information */
    p = (uint8_t *)ext_link_buf;
    *p++ = (H5L_EXT_VERSION << 4) | H5L_EXT_FLAGS_ALL;  /* External link version & flags */
    HDstrcpy((char *)p, file_name);     /* Name of file containing external link's object */
    p += HDstrlen(file_name) + 1;
    HDstrcpy((char *)p, norm_obj_name);       /* External link's object */

    /* Create an external link */
    if(H5L_create_ud(&link_loc, link_name, ext_link_buf, buf_size, H5L_TYPE_EXTERNAL, lcpl_id, lapl_id, H5AC_dxpl_id) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create link")

done:
    H5MM_xfree(ext_link_buf);
    H5MM_xfree(norm_obj_name);

    FUNC_LEAVE_API(ret_value)
} /* end H5Lcreate_external() */


/*-------------------------------------------------------------------------
 * Function: H5L_register_external
 *
 * Purpose: Registers default "External Link" link class.
 *              Use during library initialization or to restore the default
 *              after users change it.
 *
 * Return: Non-negative on success/ negative on failure
 *
 * Programmer:  James Laird
 *              Monday, July 17, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_register_external(void)
{
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5L_register_external, FAIL)

    if(H5L_register(H5L_EXTERN_LINK_CLASS) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_NOTREGISTERED, FAIL, "unable to register external link class")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_register_external() */


/*-------------------------------------------------------------------------
 * Function: H5Lunpack_elink_val
 *
 * Purpose: Given a buffer holding the "link value" from an external link,
 *              gets pointers to the information within the link value buffer.
 *
 *              External link link values contain some flags and
 *              two NULL-terminated strings, one after the other.
 *
 *              The FLAGS value will be filled in and FILENAME and
 *              OBJ_PATH will be set to pointers within ext_linkval (unless
 *              any of these values is NULL).
 *
 *              Using this function on strings that aren't external link
 *              udata buffers can result in segmentation faults.
 *
 * Return: Non-negative on success/ Negative on failure
 *
 * Programmer:  James Laird
 *              Monday, July 17, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lunpack_elink_val(const void *_ext_linkval, size_t link_size,
    unsigned *flags, const char **filename, const char **obj_path)
{
    const uint8_t *ext_linkval = (const uint8_t *)_ext_linkval; /* Pointer to the link value */
    unsigned    lnk_version;            /* External link format version */
    unsigned    lnk_flags;              /* External link flags */
    size_t      len;                    /* Length of the filename in the linkval*/
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(H5Lunpack_elink_val, FAIL)
    H5TRACE5("e", "*xz*Iu**s**s", _ext_linkval, link_size, flags, filename,
             obj_path);

    /* Sanity check external link buffer */
    if(ext_linkval == NULL )
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not an external link linkval buffer")
    lnk_version = (*ext_linkval >> 4) & 0x0F;
    lnk_flags = *ext_linkval & 0x0F;
    if(lnk_version > H5L_EXT_VERSION)
        HGOTO_ERROR(H5E_LINK, H5E_CANTDECODE, FAIL, "bad version number for external link")
    if(lnk_flags & (unsigned)~H5L_EXT_FLAGS_ALL)
        HGOTO_ERROR(H5E_LINK, H5E_CANTDECODE, FAIL, "bad flags for external link")
    if(link_size <= 2)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a valid external link buffer")

    /* Try to do some error checking.  If the last character in the linkval
     * (the last character of obj_path) isn't NULL, then something's wrong.
     */
    if(ext_linkval[link_size - 1] != '\0')
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "linkval buffer is not NULL-terminated")

    /* We're now guaranteed that HDstrlen won't segfault, since the buffer has
     * at least one NULL in it.
     */
    len = HDstrlen((const char *)ext_linkval + 1);

    /* If the first NULL we found was at the very end of the buffer, then
     * this external link value has no object name and is invalid.
     */
    if((len + 1) >= (link_size - 1))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "linkval buffer doesn't contain an object path")

    /* If we got here then the buffer contains (at least) two strings packed
     * in the correct way.  Assume it's correct and return pointers to the
     * filename and object path.
     */
    if(filename)
        *filename = (const char *)ext_linkval + 1;
    if(obj_path)
        *obj_path = ((const char *)ext_linkval + 1) + len + 1;  /* Add one for NULL terminator */

    /* Set the flags to return */
    if(flags)
        *flags = lnk_flags;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lunpack_elink_val() */

