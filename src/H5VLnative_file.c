/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Purpose:     File callbacks for the native VOL connector
 *
 */

#define H5F_FRIEND /* Suppress error about including H5Fpkg    */

#include "H5private.h"   /* Generic Functions                        */
#include "H5ACprivate.h" /* Metadata cache                           */
#include "H5Cprivate.h"  /* Cache                                    */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5Fpkg.h"      /* Files                                    */
#include "H5Gprivate.h"  /* Groups                                   */
#include "H5Iprivate.h"  /* IDs                                      */
#include "H5MFprivate.h" /* File memory management                   */
#include "H5Pprivate.h"  /* Property lists                           */
#include "H5PBprivate.h" /* Page buffering                           */
#include "H5VLprivate.h" /* Virtual Object Layer                     */

#include "H5VLnative_private.h" /* Native VOL connector                     */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_file_create
 *
 * Purpose:     Handles the file create callback
 *
 * Return:      Success:    file pointer
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL__native_file_create(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id,
                         hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req)
{
    H5F_t *new_file  = NULL;
    void * ret_value = NULL;

    FUNC_ENTER_PACKAGE

    /* Adjust bit flags by turning on the creation bit and making sure that
     * the EXCL or TRUNC bit is set.  All newly-created files are opened for
     * reading and writing.
     */
    if (0 == (flags & (H5F_ACC_EXCL | H5F_ACC_TRUNC)))
        flags |= H5F_ACC_EXCL; /* default */
    flags |= H5F_ACC_RDWR | H5F_ACC_CREAT;

    /* Create the file */
    if (NULL == (new_file = H5F_open(name, flags, fcpl_id, fapl_id)))
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to create file")
    new_file->id_exists = TRUE;

    ret_value = (void *)new_file;

done:
    if (NULL == ret_value && new_file)
        if (H5F__close(new_file) < 0)
            HDONE_ERROR(H5E_FILE, H5E_CANTCLOSEFILE, NULL, "problems closing file")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_file_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_file_open
 *
 * Purpose:     Handles the file open callback
 *
 * Return:      Success:    file pointer
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL__native_file_open(const char *name, unsigned flags, hid_t fapl_id, hid_t H5_ATTR_UNUSED dxpl_id,
                       void H5_ATTR_UNUSED **req)
{
    H5F_t *new_file  = NULL;
    void * ret_value = NULL;

    FUNC_ENTER_PACKAGE

    /* Open the file */
    if (NULL == (new_file = H5F_open(name, flags, H5P_FILE_CREATE_DEFAULT, fapl_id)))
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to open file")
    new_file->id_exists = TRUE;

    ret_value = (void *)new_file;

done:
    if (NULL == ret_value && new_file && H5F_try_close(new_file, NULL) < 0)
        HDONE_ERROR(H5E_FILE, H5E_CANTCLOSEFILE, NULL, "problems closing file")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_file_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_file_get
 *
 * Purpose:     Handles the file get callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_file_get(void *obj, H5VL_file_get_t get_type, hid_t H5_ATTR_UNUSED dxpl_id,
                      void H5_ATTR_UNUSED **req, va_list arguments)
{
    H5F_t *f         = NULL;    /* File struct */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (get_type) {
        /* "get container info" */
        case H5VL_FILE_GET_CONT_INFO: {
            H5VL_file_cont_info_t *info = HDva_arg(arguments, H5VL_file_cont_info_t *);

            /* Retrieve the file's container info */
            if (H5F__get_cont_info((H5F_t *)obj, info) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't get file container info")

            break;
        }

        /* H5Fget_access_plist */
        case H5VL_FILE_GET_FAPL: {
            H5P_genplist_t *new_plist; /* New property list */
            hid_t *         plist_id = HDva_arg(arguments, hid_t *);

            f = (H5F_t *)obj;

            /* Retrieve the file's access property list */
            if ((*plist_id = H5F_get_access_plist(f, TRUE)) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't get file access property list")

            if (NULL == (new_plist = (H5P_genplist_t *)H5I_object(*plist_id)))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")
            break;
        }

        /* H5Fget_create_plist */
        case H5VL_FILE_GET_FCPL: {
            H5P_genplist_t *plist; /* Property list */
            hid_t *         plist_id = HDva_arg(arguments, hid_t *);

            f = (H5F_t *)obj;
            if (NULL == (plist = (H5P_genplist_t *)H5I_object(f->shared->fcpl_id)))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")

            /* Create the property list object to return */
            if ((*plist_id = H5P_copy_plist(plist, TRUE)) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "unable to copy file creation properties")

            break;
        }

        /* H5Fget_intent */
        case H5VL_FILE_GET_INTENT: {
            unsigned *intent_flags = HDva_arg(arguments, unsigned *);

            f = (H5F_t *)obj;

            /* HDF5 uses some flags internally that users don't know about.
             * Simplify things for them so that they only get either H5F_ACC_RDWR
             * or H5F_ACC_RDONLY and any SWMR flags.
             */
            if (H5F_INTENT(f) & H5F_ACC_RDWR) {
                *intent_flags = H5F_ACC_RDWR;

                /* Check for SWMR write access on the file */
                if (H5F_INTENT(f) & H5F_ACC_SWMR_WRITE)
                    *intent_flags |= H5F_ACC_SWMR_WRITE;
            } /* end if */
            else {
                *intent_flags = H5F_ACC_RDONLY;

                /* Check for SWMR read access on the file */
                if (H5F_INTENT(f) & H5F_ACC_SWMR_READ)
                    *intent_flags |= H5F_ACC_SWMR_READ;
            } /* end else */

            break;
        }

        /* H5Fget_fileno */
        case H5VL_FILE_GET_FILENO: {
            unsigned long *fno       = HDva_arg(arguments, unsigned long *);
            unsigned long  my_fileno = 0;

            f = (H5F_t *)obj;
            H5F_GET_FILENO(f, my_fileno);
            *fno = my_fileno; /* sigh */

            break;
        }

        /* H5Fget_name */
        case H5VL_FILE_GET_NAME: {
            H5I_type_t type = (H5I_type_t)HDva_arg(arguments, int); /* enum work-around */
            size_t     size = HDva_arg(arguments, size_t);
            char *     name = HDva_arg(arguments, char *);
            ssize_t *  ret  = HDva_arg(arguments, ssize_t *);
            size_t     len;

            if (H5VL_native_get_file_struct(obj, type, &f) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

            len = HDstrlen(H5F_OPEN_NAME(f));

            if (name) {
                HDstrncpy(name, H5F_OPEN_NAME(f), MIN(len + 1, size));
                if (len >= size)
                    name[size - 1] = '\0';
            } /* end if */

            /* Set the return value for the API call */
            *ret = (ssize_t)len;
            break;
        }

        /* H5Fget_obj_count */
        case H5VL_FILE_GET_OBJ_COUNT: {
            unsigned types     = HDva_arg(arguments, unsigned);
            ssize_t *ret       = HDva_arg(arguments, ssize_t *);
            size_t   obj_count = 0; /* Number of opened objects */

            f = (H5F_t *)obj;
            /* Perform the query */
            if (H5F_get_obj_count(f, types, TRUE, &obj_count) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_BADITER, FAIL, "H5F_get_obj_count failed")

            /* Set the return value */
            *ret = (ssize_t)obj_count;
            break;
        }

        /* H5Fget_obj_ids */
        case H5VL_FILE_GET_OBJ_IDS: {
            unsigned types     = HDva_arg(arguments, unsigned);
            size_t   max_objs  = HDva_arg(arguments, size_t);
            hid_t *  oid_list  = HDva_arg(arguments, hid_t *);
            ssize_t *ret       = HDva_arg(arguments, ssize_t *);
            size_t   obj_count = 0; /* Number of opened objects */

            f = (H5F_t *)obj;
            /* Perform the query */
            if (H5F_get_obj_ids(f, types, max_objs, oid_list, TRUE, &obj_count) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_BADITER, FAIL, "H5F_get_obj_ids failed")

            /* Set the return value */
            *ret = (ssize_t)obj_count;
            break;
        }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't get this type of information")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_file_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_file_specific
 *
 * Purpose:     Handles the file specific callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_file_specific(void *obj, H5VL_file_specific_t specific_type, hid_t H5_ATTR_UNUSED dxpl_id,
                           void H5_ATTR_UNUSED **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (specific_type) {
        /* H5Fflush */
        case H5VL_FILE_FLUSH: {
            H5I_type_t  type  = (H5I_type_t)HDva_arg(arguments, int);  /* enum work-around */
            H5F_scope_t scope = (H5F_scope_t)HDva_arg(arguments, int); /* enum work-around */
            H5F_t *     f     = NULL;                                  /* File to flush */

            /* Get the file for the object */
            if (H5VL_native_get_file_struct(obj, type, &f) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

            /* Nothing to do if the file is read only. This determination is
             * made at the shared open(2) flags level, implying that opening a
             * file twice, once for read-only and once for read-write, and then
             * calling H5Fflush() with the read-only handle, still causes data
             * to be flushed.
             */
            if (H5F_ACC_RDWR & H5F_INTENT(f)) {
                /* Flush other files, depending on scope */
                if (H5F_SCOPE_GLOBAL == scope) {
                    /* Call the flush routine for mounted file hierarchies */
                    if (H5F_flush_mounts(f) < 0)
                        HGOTO_ERROR(H5E_FILE, H5E_CANTFLUSH, FAIL, "unable to flush mounted file hierarchy")
                } /* end if */
                else {
                    /* Call the flush routine, for this file */
                    if (H5F__flush(f) < 0)
                        HGOTO_ERROR(H5E_FILE, H5E_CANTFLUSH, FAIL,
                                    "unable to flush file's cached information")
                } /* end else */
            }     /* end if */
            break;
        }

        /* H5Freopen */
        case H5VL_FILE_REOPEN: {
            void **ret      = HDva_arg(arguments, void **);
            H5F_t *new_file = NULL;

            /* Reopen the file through the VOL connector */
            if (NULL == (new_file = H5F__reopen((H5F_t *)obj)))
                HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to reopen file")
            new_file->id_exists = TRUE;

            *ret = (void *)new_file;
            break;
        }

        /* H5Fmount */
        case H5VL_FILE_MOUNT: {
            H5I_type_t  type    = (H5I_type_t)HDva_arg(arguments, int); /* enum work-around */
            const char *name    = HDva_arg(arguments, const char *);
            H5F_t *     child   = HDva_arg(arguments, H5F_t *);
            hid_t       fmpl_id = HDva_arg(arguments, hid_t);
            H5G_loc_t   loc;

            if (H5G_loc_real(obj, type, &loc) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

            /* Do the mount */
            if (H5F__mount(&loc, name, child, fmpl_id) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_MOUNT, FAIL, "unable to mount file")

            break;
        }

        /* H5Funmount */
        case H5VL_FILE_UNMOUNT: {
            H5I_type_t  type = (H5I_type_t)HDva_arg(arguments, int); /* enum work-around */
            const char *name = HDva_arg(arguments, const char *);
            H5G_loc_t   loc;

            if (H5G_loc_real(obj, type, &loc) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

            /* Unmount */
            if (H5F__unmount(&loc, name) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_MOUNT, FAIL, "unable to unmount file")

            break;
        }

        /* H5Fis_accessible */
        case H5VL_FILE_IS_ACCESSIBLE: {
            hid_t       fapl_id = HDva_arg(arguments, hid_t);
            const char *name    = HDva_arg(arguments, const char *);
            htri_t *    result  = HDva_arg(arguments, htri_t *);

            /* Call private routine */
            if ((*result = H5F__is_hdf5(name, fapl_id)) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "error in HDF5 file check")
            break;
        }

        /* H5Fdelete */
        case H5VL_FILE_DELETE: {
            HGOTO_ERROR(H5E_FILE, H5E_UNSUPPORTED, FAIL,
                        "H5Fdelete() is currently not supported in the native VOL connector")
            break;
        }

        /* Check if two files are the same */
        case H5VL_FILE_IS_EQUAL: {
            H5F_t *  file2    = (H5F_t *)HDva_arg(arguments, void *);
            hbool_t *is_equal = HDva_arg(arguments, hbool_t *);

            if (!obj || !file2)
                *is_equal = FALSE;
            else
                *is_equal = (((H5F_t *)obj)->shared == file2->shared);
            break;
        }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "invalid specific operation")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_file_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_file_optional
 *
 * Purpose:     Handles the file optional callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_file_optional(void *obj, H5VL_file_optional_t optional_type, hid_t H5_ATTR_UNUSED dxpl_id,
                           void H5_ATTR_UNUSED **req, va_list arguments)
{
    H5F_t *f         = NULL;    /* File */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    f = (H5F_t *)obj;
    switch (optional_type) {
        /* H5Fget_filesize */
        case H5VL_NATIVE_FILE_GET_SIZE: {
            haddr_t  max_eof_eoa; /* Maximum of the EOA & EOF */
            haddr_t  base_addr;   /* Base address for the file */
            hsize_t *size = HDva_arg(arguments, hsize_t *);

            /* Go get the actual file size */
            if (H5F__get_max_eof_eoa(f, &max_eof_eoa) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "file can't get max eof/eoa ")

            base_addr = H5FD_get_base_addr(f->shared->lf);

            if (size)
                *size = (hsize_t)(max_eof_eoa +
                                  base_addr); /* Convert relative base address for file to absolute address */

            break;
        }

        /* H5Fget_file_image */
        case H5VL_NATIVE_FILE_GET_FILE_IMAGE: {
            void *   buf_ptr = HDva_arg(arguments, void *);
            ssize_t *ret     = HDva_arg(arguments, ssize_t *);
            size_t   buf_len = HDva_arg(arguments, size_t);

            /* Do the actual work */
            if ((*ret = H5F__get_file_image(f, buf_ptr, buf_len)) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "get file image failed")
            break;
        }

        /* H5Fget_freespace */
        case H5VL_NATIVE_FILE_GET_FREE_SPACE: {
            hsize_t   tot_space; /* Amount of free space in the file */
            hssize_t *ret = HDva_arg(arguments, hssize_t *);

            /* Go get the actual amount of free space in the file */
            if (H5MF_get_freespace(f, &tot_space, NULL) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to check free space for file")
            *ret = (hssize_t)tot_space;
            break;
        }

        /* H5Fget_free_sections */
        case H5VL_NATIVE_FILE_GET_FREE_SECTIONS: {
            H5F_sect_info_t *sect_info = HDva_arg(arguments, H5F_sect_info_t *);
            ssize_t *        ret       = HDva_arg(arguments, ssize_t *);
            H5F_mem_t        type      = (H5F_mem_t)HDva_arg(arguments, int); /* enum work-around */
            size_t           nsects    = HDva_arg(arguments, size_t);

            /* Go get the free-space section information in the file */
            if ((*ret = H5MF_get_free_sections(f, type, nsects, sect_info)) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to check free space for file")
            break;
        }

        /* H5Fget_info1/2 */
        case H5VL_NATIVE_FILE_GET_INFO: {
            H5I_type_t   type  = (H5I_type_t)HDva_arg(arguments, int); /* enum work-around */
            H5F_info2_t *finfo = HDva_arg(arguments, H5F_info2_t *);

            /* Get the file struct. This call is careful to not return the file pointer
             * for the top file in a mount hierarchy.
             */
            if (H5VL_native_get_file_struct(obj, type, &f) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "could not get a file struct")

            /* Get the file info */
            if (H5F__get_info(f, finfo) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to retrieve file info")

            break;
        }

        /* H5Fget_mdc_config */
        case H5VL_NATIVE_FILE_GET_MDC_CONF: {
            H5AC_cache_config_t *config_ptr = HDva_arg(arguments, H5AC_cache_config_t *);

            /* Go get the resize configuration */
            if (H5AC_get_cache_auto_resize_config(f->shared->cache, config_ptr) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5AC_get_cache_auto_resize_config() failed.")
            break;
        }

        /* H5Fget_mdc_hit_rate */
        case H5VL_NATIVE_FILE_GET_MDC_HR: {
            double *hit_rate_ptr = HDva_arg(arguments, double *);

            /* Go get the current hit rate */
            if (H5AC_get_cache_hit_rate(f->shared->cache, hit_rate_ptr) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5AC_get_cache_hit_rate() failed.")
            break;
        }

        /* H5Fget_mdc_size */
        case H5VL_NATIVE_FILE_GET_MDC_SIZE: {
            size_t * max_size_ptr        = HDva_arg(arguments, size_t *);
            size_t * min_clean_size_ptr  = HDva_arg(arguments, size_t *);
            size_t * cur_size_ptr        = HDva_arg(arguments, size_t *);
            int *    cur_num_entries_ptr = HDva_arg(arguments, int *);
            uint32_t cur_num_entries;

            /* Go get the size data */
            if (H5AC_get_cache_size(f->shared->cache, max_size_ptr, min_clean_size_ptr, cur_size_ptr,
                                    &cur_num_entries) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5AC_get_cache_size() failed.")

            if (cur_num_entries_ptr != NULL)
                *cur_num_entries_ptr = (int)cur_num_entries;
            break;
        }

        /* H5Fget_vfd_handle */
        case H5VL_NATIVE_FILE_GET_VFD_HANDLE: {
            void **file_handle = HDva_arg(arguments, void **);
            hid_t  fapl_id     = HDva_arg(arguments, hid_t);

            /* Retrieve the VFD handle for the file */
            if (H5F_get_vfd_handle(f, fapl_id, file_handle) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't retrieve VFD handle")
            break;
        }

        /* H5Fclear_elink_file_cache */
        case H5VL_NATIVE_FILE_CLEAR_ELINK_CACHE: {
            /* Release the EFC */
            if (f->shared->efc)
                if (H5F__efc_release(f->shared->efc) < 0)
                    HGOTO_ERROR(H5E_FILE, H5E_CANTRELEASE, FAIL, "can't release external file cache")
            break;
        }

        /* H5Freset_mdc_hit_rate_stats */
        case H5VL_NATIVE_FILE_RESET_MDC_HIT_RATE: {
            /* Reset the hit rate statistic */
            if (H5AC_reset_cache_hit_rate_stats(f->shared->cache) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "can't reset cache hit rate")
            break;
        }

        /* H5Fset_mdc_config */
        case H5VL_NATIVE_FILE_SET_MDC_CONFIG: {
            H5AC_cache_config_t *config_ptr = HDva_arg(arguments, H5AC_cache_config_t *);

            /* set the resize configuration  */
            if (H5AC_set_cache_auto_resize_config(f->shared->cache, config_ptr) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "H5AC_set_cache_auto_resize_config() failed")
            break;
        }

        /* H5Fget_metadata_read_retry_info */
        case H5VL_NATIVE_FILE_GET_METADATA_READ_RETRY_INFO: {
            H5F_retry_info_t *info = HDva_arg(arguments, H5F_retry_info_t *);

            if (H5F_get_metadata_read_retry_info(f, info) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "can't get metadata read retry info")

            break;
        }

        /* H5Fstart_swmr_write */
        case H5VL_NATIVE_FILE_START_SWMR_WRITE: {
            if (H5F__start_swmr_write(f) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "can't start SWMR write")

            break;
        }

        /* H5Fstart_mdc_logging */
        case H5VL_NATIVE_FILE_START_MDC_LOGGING: {
            /* Call mdc logging function */
            if (H5C_start_logging(f->shared->cache) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_LOGGING, FAIL, "unable to start mdc logging")

            break;
        }

        /* H5Fstop_mdc_logging */
        case H5VL_NATIVE_FILE_STOP_MDC_LOGGING: {
            /* Call mdc logging function */
            if (H5C_stop_logging(f->shared->cache) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_LOGGING, FAIL, "unable to stop mdc logging")

            break;
        }

        /* H5Fget_mdc_logging_status */
        case H5VL_NATIVE_FILE_GET_MDC_LOGGING_STATUS: {
            hbool_t *is_enabled           = HDva_arg(arguments, hbool_t *);
            hbool_t *is_currently_logging = HDva_arg(arguments, hbool_t *);

            /* Call mdc logging function */
            if (H5C_get_logging_status(f->shared->cache, is_enabled, is_currently_logging) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_LOGGING, FAIL, "unable to get logging status")

            break;
        }

        /* H5Fformat_convert */
        case H5VL_NATIVE_FILE_FORMAT_CONVERT: {
            /* Convert the format */
            if (H5F__format_convert(f) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTCONVERT, FAIL, "can't convert file format")

            break;
        }

        /* H5Freset_page_buffering_stats */
        case H5VL_NATIVE_FILE_RESET_PAGE_BUFFERING_STATS: {
            /* Sanity check */
            if (NULL == f->shared->page_buf)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "page buffering not enabled on file")

            /* Reset the statistics */
            if (H5PB_reset_stats(f->shared->page_buf) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't reset stats for page buffering")

            break;
        }

        /* H5Fget_page_buffering_stats */
        case H5VL_NATIVE_FILE_GET_PAGE_BUFFERING_STATS: {
            unsigned *accesses  = HDva_arg(arguments, unsigned *);
            unsigned *hits      = HDva_arg(arguments, unsigned *);
            unsigned *misses    = HDva_arg(arguments, unsigned *);
            unsigned *evictions = HDva_arg(arguments, unsigned *);
            unsigned *bypasses  = HDva_arg(arguments, unsigned *);

            /* Sanity check */
            if (NULL == f->shared->page_buf)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "page buffering not enabled on file")

            /* Get the statistics */
            if (H5PB_get_stats(f->shared->page_buf, accesses, hits, misses, evictions, bypasses) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't retrieve stats for page buffering")

            break;
        }

        /* H5Fget_mdc_image_info */
        case H5VL_NATIVE_FILE_GET_MDC_IMAGE_INFO: {
            haddr_t *image_addr = HDva_arg(arguments, haddr_t *);
            hsize_t *image_len  = HDva_arg(arguments, hsize_t *);

            /* Go get the address and size of the cache image */
            if (H5AC_get_mdc_image_info(f->shared->cache, image_addr, image_len) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't retrieve cache image info")

            break;
        }

        /* H5Fget_eoa */
        case H5VL_NATIVE_FILE_GET_EOA: {
            haddr_t *eoa = HDva_arg(arguments, haddr_t *);
            haddr_t  rel_eoa; /* Relative address of EOA */

            /* Sanity check */
            HDassert(eoa);

            /* This routine will work only for drivers with this feature enabled.*/
            /* We might introduce a new feature flag in the future */
            if (!H5F_HAS_FEATURE(f, H5FD_FEAT_SUPPORTS_SWMR_IO))
                HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL,
                            "must use a SWMR-compatible VFD for this public routine")

            /* The real work */
            if (HADDR_UNDEF == (rel_eoa = H5F_get_eoa(f, H5FD_MEM_DEFAULT)))
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "get_eoa request failed")

            /* Set return value */
            /* (Note compensating for base address subtraction in internal routine) */
            *eoa = rel_eoa + H5F_get_base_addr(f);

            break;
        }

        /* H5Fincrement_filesize */
        case H5VL_NATIVE_FILE_INCR_FILESIZE: {
            hsize_t increment = HDva_arg(arguments, hsize_t);
            haddr_t max_eof_eoa; /* Maximum of the relative EOA & EOF */

            /* This public routine will work only for drivers with this feature enabled.*/
            /* We might introduce a new feature flag in the future */
            if (!H5F_HAS_FEATURE(f, H5FD_FEAT_SUPPORTS_SWMR_IO))
                HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL,
                            "must use a SWMR-compatible VFD for this public routine")

            /* Get the maximum of EOA and EOF */
            if (H5F__get_max_eof_eoa(f, &max_eof_eoa) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "file can't get max eof/eoa ")

            /* Set EOA to the maximum value + increment */
            if (H5F__set_eoa(f, H5FD_MEM_DEFAULT, max_eof_eoa + increment) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "driver set_eoa request failed")

            break;
        }

        /* H5Fset_latest_format, H5Fset_libver_bounds */
        case H5VL_NATIVE_FILE_SET_LIBVER_BOUNDS: {
            H5F_libver_t low  = (H5F_libver_t)HDva_arg(arguments, int); /* enum work-around */
            H5F_libver_t high = (H5F_libver_t)HDva_arg(arguments, int); /* enum work-around */

            /* Call internal set_libver_bounds function */
            if (H5F__set_libver_bounds(f, low, high) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "cannot set low/high bounds")

            break;
        }

        /* H5Fget_dset_no_attrs_hint */
        case H5VL_NATIVE_FILE_GET_MIN_DSET_OHDR_FLAG: {
            hbool_t *minimize = HDva_arg(arguments, hbool_t *);
            *minimize         = H5F_GET_MIN_DSET_OHDR(f);
            break;
        }

        /* H5Fset_dset_no_attrs_hint */
        case H5VL_NATIVE_FILE_SET_MIN_DSET_OHDR_FLAG: {
            int minimize = HDva_arg(arguments, int);
            if (H5F_set_min_dset_ohdr(f, (hbool_t)minimize) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL,
                            "cannot set file's dataset object header minimization flag")
            break;
        }

#ifdef H5_HAVE_PARALLEL
        /* H5Fget_mpi_atomicity */
        case H5VL_NATIVE_FILE_GET_MPI_ATOMICITY: {
            hbool_t *flag = (hbool_t *)HDva_arg(arguments, hbool_t *);
            if (H5F_get_mpi_atomicity(f, flag) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "cannot get MPI atomicity");
            break;
        }

        /* H5Fset_mpi_atomicity */
        case H5VL_NATIVE_FILE_SET_MPI_ATOMICITY: {
            hbool_t flag = (hbool_t)HDva_arg(arguments, int);
            if (H5F_set_mpi_atomicity(f, flag) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "cannot set MPI atomicity");
            break;
        }
#endif /* H5_HAVE_PARALLEL */

        /* Finalize H5Fopen */
        case H5VL_NATIVE_FILE_POST_OPEN: {
            /* Call package routine */
            if (H5F__post_open((H5F_t *)obj) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "can't finish opening file")
            break;
        }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "invalid optional operation")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_file_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_file_close
 *
 * Purpose:     Handles the file close callback
 *
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL (file will not be closed)
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_file_close(void *file, hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req)
{
    int    nref;
    H5F_t *f         = (H5F_t *)file;
    hid_t  file_id   = H5I_INVALID_HID;
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* This routine should only be called when a file ID's ref count drops to zero */
    HDassert(H5F_ID_EXISTS(f));

    /* Flush file if this is the last reference to this id and we have write
     * intent, unless it will be flushed by the "shared" file being closed.
     * This is only necessary to replicate previous behaviour, and could be
     * disabled by an option/property to improve performance.
     */
    if ((H5F_NREFS(f) > 1) && (H5F_INTENT(f) & H5F_ACC_RDWR)) {
        /* Get the file ID corresponding to the H5F_t struct */
        if (H5I_find_id(f, H5I_FILE, &file_id) < 0 || H5I_INVALID_HID == file_id)
            HGOTO_ERROR(H5E_ATOM, H5E_CANTGET, FAIL, "invalid atom")

        /* Get the number of references outstanding for this file ID */
        if ((nref = H5I_get_ref(file_id, FALSE)) < 0)
            HGOTO_ERROR(H5E_ATOM, H5E_CANTGET, FAIL, "can't get ID ref count")
        if (nref == 1)
            if (H5F__flush(f) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "unable to flush cache")
    } /* end if */

    /* Close the file */
    if (H5F__close(f) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTDEC, FAIL, "can't close file")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_file_close() */
