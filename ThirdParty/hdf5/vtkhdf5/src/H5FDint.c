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

/*-------------------------------------------------------------------------
 *
 * Created:     H5FDint.c
 *
 * Purpose:     Internal routine for VFD operations
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#include "H5FDmodule.h" /* This source code file is part of the H5FD module */

/***********/
/* Headers */
/***********/
#include "H5private.h"   /* Generic Functions                        */
#include "H5CXprivate.h" /* API Contexts                             */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5Fprivate.h"  /* File access                              */
#include "H5FDpkg.h"     /* File Drivers                             */
#include "H5Iprivate.h"  /* IDs                                      */
#include "H5PLprivate.h" /* Plugins                                  */

/****************/
/* Local Macros */
/****************/

/******************/
/* Local Typedefs */
/******************/

/* Information needed for iterating over the registered VFD hid_t IDs.
 * The name or value of the new VFD that is being registered is stored
 * in the name (or value) field and the found_id field is initialized to
 * H5I_INVALID_HID (-1).  If we find a VFD with the same name / value,
 * we set the found_id field to the existing ID for return to the function.
 */
typedef struct H5FD_get_driver_ud_t {
    /* IN */
    H5PL_vfd_key_t key;

    /* OUT */
    hid_t found_id; /* The driver ID, if we found a match */
} H5FD_get_driver_ud_t;

/********************/
/* Package Typedefs */
/********************/

/********************/
/* Local Prototypes */
/********************/
static int H5FD__get_driver_cb(void *obj, hid_t id, void *_op_data);

/*********************/
/* Package Variables */
/*********************/

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/

/*-------------------------------------------------------------------------
 * Function:    H5FD_locate_signature
 *
 * Purpose:     Finds the HDF5 superblock signature in a file.  The
 *              signature can appear at address 0, or any power of two
 *              beginning with 512.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_locate_signature(H5FD_t *file, haddr_t *sig_addr)
{
    haddr_t  addr = HADDR_UNDEF;
    haddr_t  eoa  = HADDR_UNDEF;
    haddr_t  eof  = HADDR_UNDEF;
    uint8_t  buf[H5F_SIGNATURE_LEN];
    unsigned n;
    unsigned maxpow;
    herr_t   ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity checks */
    HDassert(file);
    HDassert(sig_addr);

    /* Find the least N such that 2^N is larger than the file size */
    eof  = H5FD_get_eof(file, H5FD_MEM_SUPER);
    eoa  = H5FD_get_eoa(file, H5FD_MEM_SUPER);
    addr = MAX(eof, eoa);
    if (HADDR_UNDEF == addr)
        HGOTO_ERROR(H5E_IO, H5E_CANTINIT, FAIL, "unable to obtain EOF/EOA value")
    for (maxpow = 0; addr; maxpow++)
        addr >>= 1;
    maxpow = MAX(maxpow, 9);

    /* Search for the file signature at format address zero followed by
     * powers of two larger than 9.
     */
    for (n = 8; n < maxpow; n++) {
        addr = (8 == n) ? 0 : (haddr_t)1 << n;
        if (H5FD_set_eoa(file, H5FD_MEM_SUPER, addr + H5F_SIGNATURE_LEN) < 0)
            HGOTO_ERROR(H5E_IO, H5E_CANTINIT, FAIL, "unable to set EOA value for file signature")
        if (H5FD_read(file, H5FD_MEM_SUPER, addr, (size_t)H5F_SIGNATURE_LEN, buf) < 0)
            HGOTO_ERROR(H5E_IO, H5E_CANTINIT, FAIL, "unable to read file signature")
        if (!HDmemcmp(buf, H5F_SIGNATURE, (size_t)H5F_SIGNATURE_LEN))
            break;
    }

    /* If the signature was not found then reset the EOA value and return
     * HADDR_UNDEF.
     */
    if (n >= maxpow) {
        if (H5FD_set_eoa(file, H5FD_MEM_SUPER, eoa) < 0)
            HGOTO_ERROR(H5E_IO, H5E_CANTINIT, FAIL, "unable to reset EOA value")
        *sig_addr = HADDR_UNDEF;
    }
    else
        /* Set return value */
        *sig_addr = addr;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_locate_signature() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_read
 *
 * Purpose:     Private version of H5FDread()
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_read(H5FD_t *file, H5FD_mem_t type, haddr_t addr, size_t size, void *buf /*out*/)
{
    hid_t  dxpl_id   = H5I_INVALID_HID; /* DXPL for operation */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(file);
    HDassert(file->cls);
    HDassert(buf);

    /* Get proper DXPL for I/O */
    dxpl_id = H5CX_get_dxpl();

#ifndef H5_HAVE_PARALLEL
    /* The no-op case
     *
     * Do not return early for Parallel mode since the I/O could be a
     * collective transfer.
     */
    if (0 == size)
        HGOTO_DONE(SUCCEED)
#endif /* H5_HAVE_PARALLEL */

    /* If the file is open for SWMR read access, allow access to data past
     * the end of the allocated space (the 'eoa').  This is done because the
     * eoa stored in the file's superblock might be out of sync with the
     * objects being written within the file by the application performing
     * SWMR write operations.
     */
    if (!(file->access_flags & H5F_ACC_SWMR_READ)) {
        haddr_t eoa;

        if (HADDR_UNDEF == (eoa = (file->cls->get_eoa)(file, type)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver get_eoa request failed")

        if ((addr + file->base_addr + size) > eoa)
            HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr overflow, addr = %llu, size = %llu, eoa = %llu",
                        (unsigned long long)(addr + file->base_addr), (unsigned long long)size,
                        (unsigned long long)eoa)
    }

    /* Dispatch to driver */
    if ((file->cls->read)(file, type, dxpl_id, addr + file->base_addr, size, buf) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "driver read request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_read() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_write
 *
 * Purpose:     Private version of H5FDwrite()
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_write(H5FD_t *file, H5FD_mem_t type, haddr_t addr, size_t size, const void *buf)
{
    hid_t   dxpl_id;                 /* DXPL for operation */
    haddr_t eoa       = HADDR_UNDEF; /* EOA for file */
    herr_t  ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(file);
    HDassert(file->cls);
    HDassert(buf);

    /* Get proper DXPL for I/O */
    dxpl_id = H5CX_get_dxpl();

#ifndef H5_HAVE_PARALLEL
    /* The no-op case
     *
     * Do not return early for Parallel mode since the I/O could be a
     * collective transfer.
     */
    if (0 == size)
        HGOTO_DONE(SUCCEED)
#endif /* H5_HAVE_PARALLEL */

    if (HADDR_UNDEF == (eoa = (file->cls->get_eoa)(file, type)))
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver get_eoa request failed")
    if ((addr + file->base_addr + size) > eoa)
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr overflow, addr = %llu, size=%llu, eoa=%llu",
                    (unsigned long long)(addr + file->base_addr), (unsigned long long)size,
                    (unsigned long long)eoa)

    /* Dispatch to driver */
    if ((file->cls->write)(file, type, dxpl_id, addr + file->base_addr, size, buf) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_WRITEERROR, FAIL, "driver write request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_write() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_set_eoa
 *
 * Purpose:     Private version of H5FDset_eoa()
 *
 *              This function expects the EOA is a RELATIVE address, i.e.
 *              relative to the base address.  This is NOT the same as the
 *              EOA stored in the superblock, which is an absolute
 *              address.  Object addresses are relative.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_set_eoa(H5FD_t *file, H5FD_mem_t type, haddr_t addr)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(file && file->cls);
    HDassert(H5F_addr_defined(addr) && addr <= file->maxaddr);

    /* Dispatch to driver, convert to absolute address */
    if ((file->cls->set_eoa)(file, type, addr + file->base_addr) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver set_eoa request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_set_eoa() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_get_eoa
 *
 * Purpose:     Private version of H5FDget_eoa()
 *
 *              This function returns the EOA as a RELATIVE address, i.e.
 *              relative to the base address.  This is NOT the same as the
 *              EOA stored in the superblock, which is an absolute
 *              address.  Object addresses are relative.
 *
 * Return:      Success:    First byte after allocated memory
 *
 *              Failure:    HADDR_UNDEF
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5FD_get_eoa(const H5FD_t *file, H5FD_mem_t type)
{
    haddr_t ret_value = HADDR_UNDEF; /* Return value */

    FUNC_ENTER_NOAPI(HADDR_UNDEF)

    HDassert(file && file->cls);

    /* Dispatch to driver */
    if (HADDR_UNDEF == (ret_value = (file->cls->get_eoa)(file, type)))
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, HADDR_UNDEF, "driver get_eoa request failed")

    /* Adjust for base address in file (convert to relative address) */
    ret_value -= file->base_addr;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_get_eoa() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_get_eof
 *
 * Purpose:     Private version of H5FDget_eof()
 *
 *              This function returns the EOF as a RELATIVE address, i.e.
 *              relative to the base address.  This will be different
 *              from  the end of the physical file if there is a user
 *              block.
 *
 * Return:      Success:    The EOF address.
 *
 *              Failure:    HADDR_UNDEF
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5FD_get_eof(const H5FD_t *file, H5FD_mem_t type)
{
    haddr_t ret_value = HADDR_UNDEF; /* Return value */

    FUNC_ENTER_NOAPI(HADDR_UNDEF)

    HDassert(file && file->cls);

    /* Dispatch to driver */
    if (file->cls->get_eof) {
        if (HADDR_UNDEF == (ret_value = (file->cls->get_eof)(file, type)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, HADDR_UNDEF, "driver get_eof request failed")
    }
    else
        ret_value = file->maxaddr;

    /* Adjust for base address in file (convert to relative address)  */
    ret_value -= file->base_addr;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_get_eof() */

/*-------------------------------------------------------------------------
 * Function:     H5FD_driver_query
 *
 * Purpose:      Similar to H5FD_query(), but intended for cases when we don't
 *               have a file available (e.g. before one is opened). Since we
 *               can't use the file to get the driver, the driver is passed in
 *               as a parameter.
 *
 * Return:       SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_driver_query(const H5FD_class_t *driver, unsigned long *flags /*out*/)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(driver);
    HDassert(flags);

    /* Check for the driver to query and then query it */
    if (driver->query)
        ret_value = (driver->query)(NULL, flags);
    else
        *flags = 0;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_driver_query() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_delete
 *
 * Purpose:     Private version of H5FDdelete()
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_delete(const char *filename, hid_t fapl_id)
{
    H5FD_class_t *     driver;              /* VFD for file */
    H5FD_driver_prop_t driver_prop;         /* Property for driver ID & info */
    H5P_genplist_t *   plist;               /* Property list pointer */
    herr_t             ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(filename);

    /* Get file access property list */
    if (NULL == (plist = (H5P_genplist_t *)H5I_object(fapl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list")

    /* Get the VFD to open the file with */
    if (H5P_peek(plist, H5F_ACS_FILE_DRV_NAME, &driver_prop) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get driver ID & info")

    /* Get driver info */
    if (NULL == (driver = (H5FD_class_t *)H5I_object(driver_prop.driver_id)))
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL, "invalid driver ID in file access property list")
    if (NULL == driver->del)
        HGOTO_ERROR(H5E_VFL, H5E_UNSUPPORTED, FAIL, "file driver has no 'del' method")

    /* Dispatch to file driver */
    if ((driver->del)(filename, fapl_id))
        HGOTO_ERROR(H5E_VFL, H5E_CANTDELETEFILE, FAIL, "delete failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_delete() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_check_plugin_load
 *
 * Purpose:     Check if a VFD plugin matches the search criteria, and can
 *              be loaded.
 *
 * Note:        Matching the driver's name / value, but the driver having
 *              an incompatible version is not an error, but means that the
 *              driver isn't a "match".  Setting the SUCCEED value to FALSE
 *              and not failing for that case allows the plugin framework
 *              to keep looking for other DLLs that match and have a
 *              compatible version.
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_check_plugin_load(const H5FD_class_t *cls, const H5PL_key_t *key, hbool_t *success)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOERR

    /* Sanity checks */
    HDassert(cls);
    HDassert(key);
    HDassert(success);

    /* Which kind of key are we looking for? */
    if (key->vfd.kind == H5FD_GET_DRIVER_BY_NAME) {
        /* Check if plugin name matches VFD class name */
        if (cls->name && !HDstrcmp(cls->name, key->vfd.u.name))
            *success = TRUE;
    }
    else {
        /* Sanity check */
        HDassert(key->vfd.kind == H5FD_GET_DRIVER_BY_VALUE);

        /* Check if plugin value matches VFD class value */
        if (cls->value == key->vfd.u.value)
            *success = TRUE;
    }

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_check_plugin_load() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__get_driver_cb
 *
 * Purpose:     Callback routine to search through registered VFDs
 *
 * Return:      Success:    H5_ITER_STOP if the class and op_data name
 *                          members match. H5_ITER_CONT otherwise.
 *              Failure:    Can't fail
 *
 *-------------------------------------------------------------------------
 */
static int
H5FD__get_driver_cb(void *obj, hid_t id, void *_op_data)
{
    H5FD_get_driver_ud_t *op_data   = (H5FD_get_driver_ud_t *)_op_data; /* User data for callback */
    H5FD_class_t *        cls       = (H5FD_class_t *)obj;
    int                   ret_value = H5_ITER_CONT; /* Callback return value */

    FUNC_ENTER_STATIC_NOERR

    if (H5FD_GET_DRIVER_BY_NAME == op_data->key.kind) {
        if (0 == HDstrcmp(cls->name, op_data->key.u.name)) {
            op_data->found_id = id;
            ret_value         = H5_ITER_STOP;
        } /* end if */
    }     /* end if */
    else {
        HDassert(H5FD_GET_DRIVER_BY_VALUE == op_data->key.kind);
        if (cls->value == op_data->key.u.value) {
            op_data->found_id = id;
            ret_value         = H5_ITER_STOP;
        } /* end if */
    }     /* end else */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__get_driver_cb() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_register_driver_by_name
 *
 * Purpose:     Registers a new VFD as a member of the virtual file driver
 *              class.
 *
 * Return:      Success:    A VFD ID which is good until the library is
 *                          closed.
 *
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FD_register_driver_by_name(const char *name, hbool_t app_ref)
{
    htri_t driver_is_registered = FALSE;
    hid_t  driver_id            = H5I_INVALID_HID;
    hid_t  ret_value            = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_NOAPI(H5I_INVALID_HID)

    /* Check if driver is already registered */
    if ((driver_is_registered = H5FD_is_driver_registered_by_name(name, &driver_id)) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_BADITER, H5I_INVALID_HID, "can't check if driver is already registered")

    /* If driver is already registered, increment ref count on ID and return ID */
    if (driver_is_registered) {
        HDassert(driver_id >= 0);

        if (H5I_inc_ref(driver_id, app_ref) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTINC, H5I_INVALID_HID, "unable to increment ref count on VFD")
    } /* end if */
    else {
        H5PL_key_t          key;
        const H5FD_class_t *cls;

        /* Try loading the driver */
        key.vfd.kind   = H5FD_GET_DRIVER_BY_NAME;
        key.vfd.u.name = name;
        if (NULL == (cls = (const H5FD_class_t *)H5PL_load(H5PL_TYPE_VFD, &key)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, H5I_INVALID_HID, "unable to load VFD")

        /* Register the driver we loaded */
        if ((driver_id = H5FD_register(cls, sizeof(*cls), app_ref)) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to register VFD ID")
    } /* end else */

    ret_value = driver_id;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_register_driver_by_name() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_register_driver_by_value
 *
 * Purpose:     Registers a new VFD as a member of the virtual file driver
 *              class.
 *
 * Return:      Success:    A VFD ID which is good until the library is
 *                          closed.
 *
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FD_register_driver_by_value(H5FD_class_value_t value, hbool_t app_ref)
{
    htri_t driver_is_registered = FALSE;
    hid_t  driver_id            = H5I_INVALID_HID;
    hid_t  ret_value            = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_NOAPI(H5I_INVALID_HID)

    /* Check if driver is already registered */
    if ((driver_is_registered = H5FD_is_driver_registered_by_value(value, &driver_id)) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_BADITER, H5I_INVALID_HID, "can't check if driver is already registered")

    /* If driver is already registered, increment ref count on ID and return ID */
    if (driver_is_registered) {
        HDassert(driver_id >= 0);

        if (H5I_inc_ref(driver_id, app_ref) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTINC, H5I_INVALID_HID, "unable to increment ref count on VFD")
    } /* end if */
    else {
        H5PL_key_t          key;
        const H5FD_class_t *cls;

        /* Try loading the driver */
        key.vfd.kind    = H5FD_GET_DRIVER_BY_VALUE;
        key.vfd.u.value = value;
        if (NULL == (cls = (const H5FD_class_t *)H5PL_load(H5PL_TYPE_VFD, &key)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, H5I_INVALID_HID, "unable to load VFD")

        /* Register the driver we loaded */
        if ((driver_id = H5FD_register(cls, sizeof(*cls), app_ref)) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to register VFD ID")
    } /* end else */

    ret_value = driver_id;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_register_driver_by_value() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_is_driver_registered_by_name
 *
 * Purpose:     Checks if a driver with a particular name is registered.
 *              If `registered_id` is non-NULL and a driver with the
 *              specified name has been registered, the driver's ID will be
 *              returned in `registered_id`.
 *
 * Return:      >0 if a VFD with that name has been registered
 *              0 if a VFD with that name has NOT been registered
 *              <0 on errors
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5FD_is_driver_registered_by_name(const char *driver_name, hid_t *registered_id)
{
    H5FD_get_driver_ud_t op_data;           /* Callback info for driver search */
    htri_t               ret_value = FALSE; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set up op data for iteration */
    op_data.key.kind   = H5FD_GET_DRIVER_BY_NAME;
    op_data.key.u.name = driver_name;
    op_data.found_id   = H5I_INVALID_HID;

    /* Find driver with name */
    if (H5I_iterate(H5I_VFL, H5FD__get_driver_cb, &op_data, FALSE) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_BADITER, FAIL, "can't iterate over VFDs")

    /* Found a driver with that name */
    if (op_data.found_id != H5I_INVALID_HID) {
        if (registered_id)
            *registered_id = op_data.found_id;
        ret_value = TRUE;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_is_driver_registered_by_name() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_is_driver_registered_by_value
 *
 * Purpose:     Checks if a driver with a particular value (ID) is
 *              registered. If `registered_id` is non-NULL and a driver
 *              with the specified value has been registered, the driver's
 *              ID will be returned in `registered_id`.
 *
 * Return:      >0 if a VFD with that value has been registered
 *              0 if a VFD with that value has NOT been registered
 *              <0 on errors
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5FD_is_driver_registered_by_value(H5FD_class_value_t driver_value, hid_t *registered_id)
{
    H5FD_get_driver_ud_t op_data;           /* Callback info for driver search */
    htri_t               ret_value = FALSE; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set up op data for iteration */
    op_data.key.kind    = H5FD_GET_DRIVER_BY_VALUE;
    op_data.key.u.value = driver_value;
    op_data.found_id    = H5I_INVALID_HID;

    /* Find driver with value */
    if (H5I_iterate(H5I_VFL, H5FD__get_driver_cb, &op_data, FALSE) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_BADITER, FAIL, "can't iterate over VFDs")

    /* Found a driver with that value */
    if (op_data.found_id != H5I_INVALID_HID) {
        if (registered_id)
            *registered_id = op_data.found_id;
        ret_value = TRUE;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_is_driver_registered_by_value() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_get_driver_id_by_name
 *
 * Purpose:     Retrieves the ID for a registered VFL driver.
 *
 * Return:      Positive if the VFL driver has been registered
 *              Negative on error (if the driver is not a valid driver or
 *                  is not registered)
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FD_get_driver_id_by_name(const char *name, hbool_t is_api)
{
    H5FD_get_driver_ud_t op_data;
    hid_t                ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_NOAPI(H5I_INVALID_HID)

    /* Set up op data for iteration */
    op_data.key.kind   = H5FD_GET_DRIVER_BY_NAME;
    op_data.key.u.name = name;
    op_data.found_id   = H5I_INVALID_HID;

    /* Find driver with specified name */
    if (H5I_iterate(H5I_VFL, H5FD__get_driver_cb, &op_data, FALSE) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_BADITER, H5I_INVALID_HID, "can't iterate over VFL drivers")

    /* Found a driver with that name */
    if (op_data.found_id != H5I_INVALID_HID) {
        ret_value = op_data.found_id;
        if (H5I_inc_ref(ret_value, is_api) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTINC, H5I_INVALID_HID, "unable to increment ref count on VFL driver")
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_get_driver_id_by_name() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_get_driver_id_by_value
 *
 * Purpose:     Retrieves the ID for a registered VFL driver.
 *
 * Return:      Positive if the VFL driver has been registered
 *              Negative on error (if the driver is not a valid driver or
 *                  is not registered)
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FD_get_driver_id_by_value(H5FD_class_value_t value, hbool_t is_api)
{
    H5FD_get_driver_ud_t op_data;
    hid_t                ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_NOAPI(H5I_INVALID_HID)

    /* Set up op data for iteration */
    op_data.key.kind    = H5FD_GET_DRIVER_BY_VALUE;
    op_data.key.u.value = value;
    op_data.found_id    = H5I_INVALID_HID;

    /* Find driver with specified value */
    if (H5I_iterate(H5I_VFL, H5FD__get_driver_cb, &op_data, FALSE) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_BADITER, H5I_INVALID_HID, "can't iterate over VFL drivers")

    /* Found a driver with that value */
    if (op_data.found_id != H5I_INVALID_HID) {
        ret_value = op_data.found_id;
        if (H5I_inc_ref(ret_value, is_api) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTINC, H5I_INVALID_HID, "unable to increment ref count on VFL driver")
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_get_driver_id_by_value() */
