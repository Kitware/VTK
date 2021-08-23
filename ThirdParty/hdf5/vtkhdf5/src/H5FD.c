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

/*
 * Purpose: The Virtual File Layer as described in documentation.
 *          This is the greatest common denominator for all types of
 *          storage access whether a file, memory, network, etc. This
 *          layer usually just dispatches the request to an actual
 *          file driver layer.
 */

/****************/
/* Module Setup */
/****************/

#define H5F_FRIEND      /* Suppress error about including H5Fpkg */
#include "H5FDmodule.h" /* This source code file is part of the H5FD module */

/***********/
/* Headers */
/***********/
#include "H5private.h"   /* Generic Functions                        */
#include "H5CXprivate.h" /* API Contexts                             */
#include "H5Dprivate.h"  /* Datasets                                 */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5Fpkg.h"      /* File access                              */
#include "H5FDpkg.h"     /* File Drivers                             */
#include "H5Iprivate.h"  /* IDs                                      */
#include "H5MMprivate.h" /* Memory management                        */
#include "H5Pprivate.h"  /* Property lists                           */

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
static herr_t H5FD__free_cls(H5FD_class_t *cls);
static herr_t H5FD__query(const H5FD_t *f, unsigned long *flags /*out*/);

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

/*
 * Global count of the number of H5FD_t's handed out.  This is used as a
 * "serial number" for files that are currently open and is used for the
 * 'fileno' field in H5O_info_t.  However, if a VFL driver is not able
 * to detect whether two files are the same, a file that has been opened
 * by H5Fopen more than once with that VFL driver will have two different
 * serial numbers.  :-/
 *
 * Also, if a file is opened, the 'fileno' field is retrieved for an
 * object and the file is closed and re-opened, the 'fileno' value will
 * be different.
 */
static unsigned long H5FD_file_serial_no_g;

/* File driver ID class */
static const H5I_class_t H5I_VFL_CLS[1] = {{
    H5I_VFL,                   /* ID class value */
    0,                         /* Class flags */
    0,                         /* # of reserved IDs for class */
    (H5I_free_t)H5FD__free_cls /* Callback routine for closing objects of this class */
}};

/*-------------------------------------------------------------------------
 * Function:    H5FD__init_package
 *
 * Purpose:     Initialize the virtual file layer.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__init_package(void)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    if (H5I_register_type(H5I_VFL_CLS) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "unable to initialize interface")

    /* Reset the file serial numbers */
    H5FD_file_serial_no_g = 0;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__init_package() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_term_package
 *
 * Purpose:     Terminate this interface: free all memory and reset global
 *              variables to their initial values.  Release all ID groups
 *              associated with this interface.
 *
 * Return:      Success:    Positive if anything was done that might
 *                          have affected other interfaces; zero
 *                          otherwise.
 *
 *              Failure:    Never fails.
 *
 *-------------------------------------------------------------------------
 */
int
H5FD_term_package(void)
{
    int n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if (H5_PKG_INIT_VAR) {
        if (H5I_nmembers(H5I_VFL) > 0) {
            (void)H5I_clear_type(H5I_VFL, FALSE, FALSE);
            n++; /*H5I*/
        }        /* end if */
        else {
            /* Destroy the VFL driver ID group */
            n += (H5I_dec_type_ref(H5I_VFL) > 0);

            /* Mark closed */
            if (0 == n)
                H5_PKG_INIT_VAR = FALSE;
        } /* end else */
    }     /* end if */

    FUNC_LEAVE_NOAPI(n)
} /* end H5FD_term_package() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__free_cls
 *
 * Purpose:     Frees a file driver class struct and returns an indication of
 *              success. This function is used as the free callback for the
 *              virtual file layer object identifiers (cf H5FD__init_package).
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__free_cls(H5FD_class_t *cls)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    /* Sanity checks */
    HDassert(cls);

    /* If the file driver has a terminate callback, call it to give the file
     * driver a chance to free singletons or other resources which will become
     * invalid once the class structure is freed.
     */
    if (cls->terminate && cls->terminate() < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTCLOSEOBJ, FAIL, "virtual file driver '%s' did not terminate cleanly",
                    cls->name)

    H5MM_xfree(cls);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__free_cls() */

/*-------------------------------------------------------------------------
 * Function:    H5FDregister
 *
 * Purpose:     Registers a new file driver as a member of the virtual file
 *              driver class.  Certain fields of the class struct are
 *              required and that is checked here so it doesn't have to be
 *              checked every time the field is accessed.
 *
 * Return:      Success:    A file driver ID which is good until the
 *                          library is closed or the driver is
 *                          unregistered.
 *
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FDregister(const H5FD_class_t *cls)
{
    H5FD_mem_t type;
    hid_t      ret_value = H5I_INVALID_HID;

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE1("i", "*x", cls);

    /* Check arguments */
    if (!cls)
        HGOTO_ERROR(H5E_ARGS, H5E_UNINITIALIZED, H5I_INVALID_HID, "null class pointer is disallowed")
    if (!cls->open || !cls->close)
        HGOTO_ERROR(H5E_ARGS, H5E_UNINITIALIZED, H5I_INVALID_HID,
                    "'open' and/or 'close' methods are not defined")
    if (!cls->get_eoa || !cls->set_eoa)
        HGOTO_ERROR(H5E_ARGS, H5E_UNINITIALIZED, H5I_INVALID_HID,
                    "'get_eoa' and/or 'set_eoa' methods are not defined")
    if (!cls->get_eof)
        HGOTO_ERROR(H5E_ARGS, H5E_UNINITIALIZED, H5I_INVALID_HID, "'get_eof' method is not defined")
    if (!cls->read || !cls->write)
        HGOTO_ERROR(H5E_ARGS, H5E_UNINITIALIZED, H5I_INVALID_HID,
                    "'read' and/or 'write' method is not defined")
    for (type = H5FD_MEM_DEFAULT; type < H5FD_MEM_NTYPES; type++)
        if (cls->fl_map[type] < H5FD_MEM_NOLIST || cls->fl_map[type] >= H5FD_MEM_NTYPES)
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "invalid free-list mapping")

    /* Create the new class ID */
    if ((ret_value = H5FD_register(cls, sizeof(H5FD_class_t), TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to register file driver ID")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5FDregister() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_register
 *
 * Purpose:     Registers a new file driver as a member of the virtual file
 *              driver class.  Certain fields of the class struct are
 *              required and that is checked here so it doesn't have to be
 *              checked every time the field is accessed.
 *
 * Return:      Success:    A file driver ID which is good until the
 *                          library is closed or the driver is
 *                          unregistered.
 *
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FD_register(const void *_cls, size_t size, hbool_t app_ref)
{
    const H5FD_class_t *cls   = (const H5FD_class_t *)_cls;
    H5FD_class_t *      saved = NULL;
    H5FD_mem_t          type;
    hid_t               ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_NOAPI(H5I_INVALID_HID)

    /* Sanity checks */
    HDassert(cls);
    HDassert(cls->open && cls->close);
    HDassert(cls->get_eoa && cls->set_eoa);
    HDassert(cls->get_eof);
    HDassert(cls->read && cls->write);
    for (type = H5FD_MEM_DEFAULT; type < H5FD_MEM_NTYPES; type++) {
        HDassert(cls->fl_map[type] >= H5FD_MEM_NOLIST && cls->fl_map[type] < H5FD_MEM_NTYPES);
    }

    /* Copy the class structure so the caller can reuse or free it */
    if (NULL == (saved = (H5FD_class_t *)H5MM_malloc(size)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, H5I_INVALID_HID,
                    "memory allocation failed for file driver class struct")
    H5MM_memcpy(saved, cls, size);

    /* Create the new class ID */
    if ((ret_value = H5I_register(H5I_VFL, saved, app_ref)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to register file driver ID")

done:
    if (H5I_INVALID_HID == ret_value)
        if (saved)
            saved = (H5FD_class_t *)H5MM_xfree(saved);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_register() */

/*-------------------------------------------------------------------------
 * Function:    H5FDunregister
 *
 * Purpose:     Removes a driver ID from the library. This in no way affects
 *              file access property lists which have been defined to use
 *              this driver or files which are already opened under this
 *              driver.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDunregister(hid_t driver_id)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", driver_id);

    /* Check arguments */
    if (NULL == H5I_object_verify(driver_id, H5I_VFL))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file driver")

    /* The H5FD_class_t struct will be freed by this function */
    if (H5I_dec_app_ref(driver_id) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTDEC, FAIL, "unable to unregister file driver")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5FDunregister() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_get_class
 *
 * Purpose:     Obtains a pointer to the driver struct containing all the
 *              callback pointers, etc. The PLIST_ID argument can be a file
 *              access property list, a data transfer property list, or a
 *              file driver identifier.
 *
 * Return:      Success:    Ptr to the driver information. The pointer is
 *                          only valid as long as the driver remains
 *                          registered or some file or property list
 *                          exists which references the driver.
 *
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
H5FD_class_t *
H5FD_get_class(hid_t id)
{
    H5FD_class_t *ret_value = NULL;

    FUNC_ENTER_NOAPI(NULL)

    if (H5I_VFL == H5I_get_type(id))
        ret_value = (H5FD_class_t *)H5I_object(id);
    else {
        H5P_genplist_t *plist; /* Property list pointer */

        /* Get the plist structure */
        if (NULL == (plist = (H5P_genplist_t *)H5I_object(id)))
            HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, NULL, "can't find object for ID")

        if (TRUE == H5P_isa_class(id, H5P_FILE_ACCESS)) {
            H5FD_driver_prop_t driver_prop; /* Property for driver ID & info */

            if (H5P_peek(plist, H5F_ACS_FILE_DRV_NAME, &driver_prop) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't get driver ID & info")
            ret_value = H5FD_get_class(driver_prop.driver_id);
        } /* end if */
        else
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a driver id or file access property list")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_get_class() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_sb_size
 *
 * Purpose:     Obtains the number of bytes required to store the driver file
 *              access data in the HDF5 superblock.
 *
 * Return:      Success:    Number of bytes required. May be zero if the
 *                          driver has no data to store in the superblock.
 *
 *              Failure:    This function cannot indicate errors.
 *
 *-------------------------------------------------------------------------
 */
hsize_t
H5FD_sb_size(H5FD_t *file)
{
    hsize_t ret_value = 0;

    FUNC_ENTER_NOAPI(0)

    /* Sanity checks */
    HDassert(file);
    HDassert(file->cls);

    /* Dispatch to driver */
    if (file->cls->sb_size)
        ret_value = (file->cls->sb_size)(file);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD_sb_encode
 *
 * Purpose:     Encode driver-specific data into the output arguments. The
 *              NAME is a nine-byte buffer which should get an
 *              eight-character driver name and/or version followed by a null
 *              terminator. The BUF argument is a buffer to receive the
 *              encoded driver-specific data. The size of the BUF array is
 *              the size returned by the H5FD_sb_size() call.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_sb_encode(H5FD_t *file, char *name /*out*/, uint8_t *buf)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(file);
    HDassert(file->cls);

    /* Dispatch to driver */
    if (file->cls->sb_encode && (file->cls->sb_encode)(file, name /*out*/, buf /*out*/) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver sb_encode request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_sb_encode() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__sb_decode
 *
 * Purpose:     Decodes the driver information block.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__sb_decode(H5FD_t *file, const char *name, const uint8_t *buf)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity checks */
    HDassert(file);
    HDassert(file->cls);

    /* Dispatch to driver */
    if (file->cls->sb_decode && (file->cls->sb_decode)(file, name, buf) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver sb_decode request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__sb_decode() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_sb_load
 *
 * Purpose:     Validate and decode the driver information block.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_sb_load(H5FD_t *file, const char *name, const uint8_t *buf)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(file);
    HDassert(file->cls);

    /* Check if driver matches driver information saved. Unfortunately, we can't push this
     * function to each specific driver because we're checking if the driver is correct.
     */
    if (!HDstrncmp(name, "NCSAfami", (size_t)8) && HDstrcmp(file->cls->name, "family") != 0)
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL, "family driver should be used")
    if (!HDstrncmp(name, "NCSAmult", (size_t)8) && HDstrcmp(file->cls->name, "multi") != 0)
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL, "multi driver should be used")

    /* Decode driver information */
    if (H5FD__sb_decode(file, name, buf) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTDECODE, FAIL, "unable to decode driver information")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_sb_load() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_fapl_get
 *
 * Purpose:     Gets the file access property list associated with a file.
 *              Usually the file will copy what it needs from the original
 *              file access property list when the file is created. The
 *              purpose of this function is to create a new file access
 *              property list based on the settings in the file, which may
 *              have been modified from the original file access property
 *              list.
 *
 * Return:      Success:    Pointer to a new file access property list
 *                          with all members copied.  If the file is
 *                          closed then this property list lives on, and
 *                          vice versa.
 *
 *                          This can be NULL if the file has no properties.
 *
 *              Failure:    This function cannot indicate errors.
 *
 *-------------------------------------------------------------------------
 */
void *
H5FD_fapl_get(H5FD_t *file)
{
    void *ret_value = NULL;

    FUNC_ENTER_NOAPI(NULL)

    /* Sanity checks */
    HDassert(file);
    HDassert(file->cls);

    /* Dispatch to driver */
    if (file->cls->fapl_get)
        ret_value = (file->cls->fapl_get)(file);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_fapl_get() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_free_driver_info
 *
 * Purpose:     Frees a driver's info
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_free_driver_info(hid_t driver_id, const void *driver_info)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if (driver_id > 0 && driver_info) {
        H5FD_class_t *driver;

        /* Retrieve the driver for the ID */
        if (NULL == (driver = (H5FD_class_t *)H5I_object(driver_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a driver ID")

        /* Allow driver to free info or do it ourselves */
        if (driver->fapl_free) {
            /* Free the const pointer */
            /* Cast through uintptr_t to de-const memory */
            if ((driver->fapl_free)((void *)(uintptr_t)driver_info) < 0)
                HGOTO_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "driver free request failed")
        }
        else
            driver_info = H5MM_xfree_const(driver_info);
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_free_driver_info() */

/*-------------------------------------------------------------------------
 * Function:    H5FDopen
 *
 * Purpose:     Opens a file named NAME for the type(s) of access described
 *              by the bit vector FLAGS according to a file access property
 *              list FAPL_ID (which may be the constant H5P_DEFAULT). The
 *              file should expect to handle format addresses in the range [0,
 *              MAXADDR] (if MAXADDR is the undefined address then the caller
 *              doesn't care about the address range).
 *
 *              Possible values for the FLAGS bits are:
 *
 *              H5F_ACC_RDWR:   Open the file for read and write access. If
 *                              this bit is not set then open the file for
 *                              read only access. It is permissible to open a
 *                              file for read and write access when only read
 *                              access is requested by the library (the
 *                              library will never attempt to write to a file
 *                              which it opened with only read access).
 *
 *              H5F_ACC_CREATE: Create the file if it doesn't already exist.
 *                              However, see H5F_ACC_EXCL below.
 *
 *              H5F_ACC_TRUNC:  Truncate the file if it already exists. This
 *                              is equivalent to deleting the file and then
 *                              creating a new empty file.
 *
 *              H5F_ACC_EXCL:   When used with H5F_ACC_CREATE, if the file
 *                              already exists then the open should fail.
 *                              Note that this is unsupported/broken with
 *                              some file drivers (e.g., sec2 across nfs) and
 *                              will contain a race condition when used to
 *                              perform file locking.
 *
 *              The MAXADDR is the maximum address which will be requested by
 *              the library during an allocation operation. Usually this is
 *              the same value as the MAXADDR field of the class structure,
 *              but it can be smaller if the driver is being used under some
 *              other driver.
 *
 *              Note that when the driver 'open' callback gets control that
 *              the public part of the file struct (the H5FD_t part) will be
 *              incomplete and will be filled in after that callback returns.
 *
 * Return:      Success:    Pointer to a new file driver struct.
 *
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
H5FD_t *
H5FDopen(const char *name, unsigned flags, hid_t fapl_id, haddr_t maxaddr)
{
    H5FD_t *ret_value = NULL;

    FUNC_ENTER_API(NULL)
    H5TRACE4("*x", "*sIuia", name, flags, fapl_id, maxaddr);

    /* Check arguments */
    if (H5P_DEFAULT == fapl_id)
        fapl_id = H5P_FILE_ACCESS_DEFAULT;
    else if (TRUE != H5P_isa_class(fapl_id, H5P_FILE_ACCESS))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access property list")

    /* Call private function */
    if (NULL == (ret_value = H5FD_open(name, flags, fapl_id, maxaddr)))
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, NULL, "unable to open file")

done:
    FUNC_LEAVE_API(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD_open
 *
 * Purpose:     Private version of H5FDopen()
 *
 * Return:      Success:    Pointer to a new file driver struct
 *
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
H5FD_t *
H5FD_open(const char *name, unsigned flags, hid_t fapl_id, haddr_t maxaddr)
{
    H5FD_class_t *         driver;           /* VFD for file */
    H5FD_t *               file = NULL;      /* VFD file struct */
    H5FD_driver_prop_t     driver_prop;      /* Property for driver ID & info */
    H5P_genplist_t *       plist;            /* Property list pointer */
    unsigned long          driver_flags = 0; /* File-inspecific driver feature flags */
    H5FD_file_image_info_t file_image_info;  /* Initial file image */
    H5FD_t *               ret_value = NULL; /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Sanity checks */
    if (0 == maxaddr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "zero format address range")

    /* Get file access property list */
    if (NULL == (plist = (H5P_genplist_t *)H5I_object(fapl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access property list")

    /* Get the VFD to open the file with */
    if (H5P_peek(plist, H5F_ACS_FILE_DRV_NAME, &driver_prop) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't get driver ID & info")

    /* Get driver info */
    if (NULL == (driver = (H5FD_class_t *)H5I_object(driver_prop.driver_id)))
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, NULL, "invalid driver ID in file access property list")
    if (NULL == driver->open)
        HGOTO_ERROR(H5E_VFL, H5E_UNSUPPORTED, NULL, "file driver has no `open' method")

    /* Query driver flag */
    if (H5FD_driver_query(driver, &driver_flags) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, NULL, "can't query VFD flags")

    /* Get initial file image info */
    if (H5P_peek(plist, H5F_ACS_FILE_IMAGE_INFO_NAME, &file_image_info) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't get file image info")

    /* If an image is provided, make sure the driver supports this feature */
    HDassert(((file_image_info.buffer != NULL) && (file_image_info.size > 0)) ||
             ((file_image_info.buffer == NULL) && (file_image_info.size == 0)));
    if ((file_image_info.buffer != NULL) && !(driver_flags & H5FD_FEAT_ALLOW_FILE_IMAGE))
        HGOTO_ERROR(H5E_VFL, H5E_UNSUPPORTED, NULL, "file image set, but not supported.")

    /* Dispatch to file driver */
    if (HADDR_UNDEF == maxaddr)
        maxaddr = driver->maxaddr;
    if (NULL == (file = (driver->open)(name, flags, fapl_id, maxaddr)))
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, NULL, "open failed")

    /* Set the file access flags */
    file->access_flags = flags;

    /* Fill in public fields. We must increment the reference count on the
     * driver ID to prevent it from being freed while this file is open.
     */
    file->driver_id = driver_prop.driver_id;
    if (H5I_inc_ref(file->driver_id, FALSE) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINC, NULL, "unable to increment ref count on VFL driver")
    file->cls     = driver;
    file->maxaddr = maxaddr;
    if (H5P_get(plist, H5F_ACS_ALIGN_THRHD_NAME, &(file->threshold)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't get alignment threshold")
    if (H5P_get(plist, H5F_ACS_ALIGN_NAME, &(file->alignment)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't get alignment")

    /* Retrieve the VFL driver feature flags */
    if (H5FD__query(file, &(file->feature_flags)) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, NULL, "unable to query file driver")

    /* Increment the global serial number & assign it to this H5FD_t object */
    if (++H5FD_file_serial_no_g == 0) {
        /* (Just error out if we wrap around for now...) */
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, NULL, "unable to get file serial number")
    } /* end if */
    file->fileno = H5FD_file_serial_no_g;

    /* Start with base address set to 0 */
    /* (This will be changed later, when the superblock is located) */
    file->base_addr = 0;

    /* Set return value */
    ret_value = file;

done:
    /* Can't cleanup 'file' information, since we don't know what type it is */
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_open() */

/*-------------------------------------------------------------------------
 * Function:    H5FDclose
 *
 * Purpose:     Closes the file by calling the driver 'close' callback, which
 *              should free all driver-private data and free the file struct.
 *              Note that the public part of the file struct (the H5FD_t part)
 *              will be all zero during the driver close callback like during
 *              the 'open' callback.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDclose(H5FD_t *file)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "*x", file);

    /* Check arguments */
    if (!file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file pointer cannot be NULL")
    if (!file->cls)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file class pointer cannot be NULL")

    /* Call private function */
    if (H5FD_close(file) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTCLOSEFILE, FAIL, "unable to close file")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5FDclose() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_close
 *
 * Purpose:     Private version of H5FDclose()
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_close(H5FD_t *file)
{
    const H5FD_class_t *driver;
    herr_t              ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(file);
    HDassert(file->cls);

    /* Prepare to close file by clearing all public fields */
    driver = file->cls;
    if (H5I_dec_ref(file->driver_id) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTDEC, FAIL, "can't close driver ID")

    /* Dispatch to the driver for actual close. If the driver fails to
     * close the file then the file will be in an unusable state.
     */
    HDassert(driver->close);
    if ((driver->close)(file) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTCLOSEFILE, FAIL, "close failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_close() */

/*-------------------------------------------------------------------------
 * Function:    H5FDcmp
 *
 * Purpose:     Compare the keys of two files using the file driver callback
 *              if the files belong to the same driver, otherwise sort the
 *              files by driver class pointer value.
 *
 * Return:      Success:    A value like strcmp()
 *
 *              Failure:    Must never fail. If both file handles are
 *                          invalid then they compare equal. If one file
 *                          handle is invalid then it compares less than
 *                          the other.  If both files belong to the same
 *                          driver and the driver doesn't provide a
 *                          comparison callback then the file pointers
 *                          themselves are compared.
 *
 *-------------------------------------------------------------------------
 */
int
H5FDcmp(const H5FD_t *f1, const H5FD_t *f2)
{
    int ret_value = -1;

    FUNC_ENTER_API(-1) /* return value is arbitrary */
    H5TRACE2("Is", "*x*x", f1, f2);

    /* Call private function */
    ret_value = H5FD_cmp(f1, f2);

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5FDcmp() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_cmp
 *
 * Purpose:     Private version of H5FDcmp()
 *
 * Return:      Success:    A value like strcmp()
 *
 *              Failure:    Must never fail.
 *
 *-------------------------------------------------------------------------
 */
int
H5FD_cmp(const H5FD_t *f1, const H5FD_t *f2)
{
    int ret_value = -1; /* Return value */

    FUNC_ENTER_NOAPI(-1) /* return value is arbitrary */

    if ((!f1 || !f1->cls) && (!f2 || !f2->cls))
        HGOTO_DONE(0)
    if (!f1 || !f1->cls)
        HGOTO_DONE(-1)
    if (!f2 || !f2->cls)
        HGOTO_DONE(1)
    if (f1->cls < f2->cls)
        HGOTO_DONE(-1)
    if (f1->cls > f2->cls)
        HGOTO_DONE(1)

    /* Files are same driver; no cmp callback */
    if (!f1->cls->cmp) {
        if (f1 < f2)
            HGOTO_DONE(-1)
        if (f1 > f2)
            HGOTO_DONE(1)
        HGOTO_DONE(0)
    }

    /* Dispatch to driver */
    ret_value = (f1->cls->cmp)(f1, f2);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_cmp() */

/*-------------------------------------------------------------------------
 * Function:    H5FDquery
 *
 * Purpose:     Query a VFL driver for its feature flags. (listed in H5FDpublic.h)
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
int
H5FDquery(const H5FD_t *file, unsigned long *flags /*out*/)
{
    int ret_value = 0;

    FUNC_ENTER_API((-1))
    H5TRACE2("Is", "*xx", file, flags);

    /* Check arguments */
    if (!file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, (-1), "file pointer cannot be NULL")
    if (!file->cls)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, (-1), "file class pointer cannot be NULL")
    if (!flags)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, (-1), "flags parameter cannot be NULL")

    /* Call private function */
    if (H5FD__query(file, flags) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, (-1), "unable to query feature flags")

done:
    FUNC_LEAVE_API(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD_query
 *
 * Purpose:     Private version of H5FDquery()
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__query(const H5FD_t *file, unsigned long *flags /*out*/)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity checks */
    HDassert(file);
    HDassert(file->cls);
    HDassert(flags);

    /* Dispatch to driver (if available) */
    if (file->cls->query) {
        if ((file->cls->query)(file, flags) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "unable to query feature flags")
    }
    else
        *flags = 0;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_query() */

/*-------------------------------------------------------------------------
 * Function:    H5FDalloc
 *
 * Purpose:     Allocates SIZE bytes of memory from the FILE. The memory will
 *              be used according to the allocation class TYPE. First we try
 *              to satisfy the request from one of the free lists, according
 *              to the free list map provided by the driver. The free list
 *              array has one entry for each request type and the value of
 *              that array element can be one of four possibilities:
 *
 *              It can be the constant H5FD_MEM_DEFAULT (or zero) which
 *              indicates that the identity mapping is used. In other
 *              words, the request type maps to its own free list.
 *
 *              It can be the request type itself, which has the same
 *              effect as the H5FD_MEM_DEFAULT value above.
 *
 *              It can be the ID for another request type, which
 *              indicates that the free list for the specified type
 *              should be used instead.
 *
 *              It can be the constant H5FD_MEM_NOLIST which means that
 *              no free list should be used for this type of request.
 *
 *              If the request cannot be satisfied from a free list then
 *              either the driver's 'alloc' callback is invoked (if one was
 *              supplied) or the end-of-address marker is extended. The
 *              'alloc' callback is always called with the same arguments as
 *              the H5FDalloc().
 *
 * Return:      Success:    The format address of the new file memory.
 *
 *              Failure:    The undefined address HADDR_UNDEF
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5FDalloc(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, hsize_t size)
{
    haddr_t ret_value = HADDR_UNDEF;

    FUNC_ENTER_API(HADDR_UNDEF)
    H5TRACE4("a", "*xMtih", file, type, dxpl_id, size);

    /* Check arguments */
    if (!file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, HADDR_UNDEF, "file pointer cannot be NULL")
    if (!file->cls)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, HADDR_UNDEF, "file class pointer cannot be NULL")
    if (type < H5FD_MEM_DEFAULT || type >= H5FD_MEM_NTYPES)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, HADDR_UNDEF, "invalid request type")
    if (size == 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, HADDR_UNDEF, "zero-size request")
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id = H5P_DATASET_XFER_DEFAULT;
    else if (TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, HADDR_UNDEF, "not a data transfer property list")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Call private function */
    if (HADDR_UNDEF == (ret_value = H5FD__alloc_real(file, type, size, NULL, NULL)))
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, HADDR_UNDEF, "unable to allocate file memory")

    /* (Note compensating for base address subtraction in internal routine) */
    ret_value += file->base_addr;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5FDalloc() */

/*-------------------------------------------------------------------------
 * Function:    H5FDfree
 *
 * Purpose:     Frees format addresses starting with ADDR and continuing for
 *              SIZE bytes in the file FILE. The type of space being freed is
 *              specified by TYPE, which is mapped to a free list as
 *              described for the H5FDalloc() function above.  If the request
 *              doesn't map to a free list then either the application 'free'
 *              callback is invoked (if defined) or the memory is leaked.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDfree(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, hsize_t size)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "*xMtiah", file, type, dxpl_id, addr, size);

    /* Check arguments */
    if (!file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file pointer cannot be NULL")
    if (!file->cls)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file class pointer cannot be NULL")
    if (type < H5FD_MEM_DEFAULT || type >= H5FD_MEM_NTYPES)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid request type")
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id = H5P_DATASET_XFER_DEFAULT;
    else if (TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data transfer property list")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Call private function */
    /* (Note compensating for base address addition in internal routine) */
    if (H5FD__free_real(file, type, addr - file->base_addr, size) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "file deallocation request failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5FDfree() */

/*-------------------------------------------------------------------------
 * Function:    H5FDget_eoa
 *
 * Purpose:     Returns the address of the first byte after the last
 *              allocated memory in the file.
 *
 * Return:      Success:    First byte after allocated memory.
 *              Failure:    HADDR_UNDEF
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5FDget_eoa(H5FD_t *file, H5FD_mem_t type)
{
    haddr_t ret_value;

    FUNC_ENTER_API(HADDR_UNDEF)
    H5TRACE2("a", "*xMt", file, type);

    /* Check arguments */
    if (!file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, HADDR_UNDEF, "file pointer cannot be NULL")
    if (!file->cls)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, HADDR_UNDEF, "file class pointer cannot be NULL")
    if (type < H5FD_MEM_DEFAULT || type >= H5FD_MEM_NTYPES)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, HADDR_UNDEF, "invalid file type")

    /* Call private function */
    if (HADDR_UNDEF == (ret_value = H5FD_get_eoa(file, type)))
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, HADDR_UNDEF, "file get eoa request failed")

    /* (Note compensating for base address subtraction in internal routine) */
    ret_value += file->base_addr;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5FDget_eoa() */

/*-------------------------------------------------------------------------
 * Function:	H5FDset_eoa
 *
 * Purpose:	Set the end-of-address marker for the file. The ADDR is the
 *		address of the first byte past the last allocated byte of the
 *		file. This function is called from two places:
 *
 *		    It is called after an existing file is opened in order to
 *		    "allocate" enough space to read the superblock and then
 *		    to "allocate" the entire hdf5 file based on the contents
 *		    of the superblock.
 *
 *		    It is called during file memory allocation if the
 *		    allocation request cannot be satisfied from the free list
 *		    and the driver didn't supply an allocation callback.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative, no side effect
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDset_eoa(H5FD_t *file, H5FD_mem_t type, haddr_t addr)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "*xMta", file, type, addr);

    /* Check arguments */
    if (!file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file pointer cannot be NULL")
    if (!file->cls)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file class pointer cannot be NULL")
    if (type < H5FD_MEM_DEFAULT || type >= H5FD_MEM_NTYPES)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file type")
    if (!H5F_addr_defined(addr) || addr > file->maxaddr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid end-of-address value")

    /* Call private function */
    /* (Note compensating for base address addition in internal routine) */
    if (H5FD_set_eoa(file, type, addr - file->base_addr) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "file set eoa request failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5FDset_eoa() */

/*-------------------------------------------------------------------------
 * Function:    H5FDget_eof
 *
 * Purpose:     Returns the end-of-file address, which is the greater of the
 *              end-of-format address and the actual EOF marker. This
 *              function is called after an existing file is opened in order
 *              for the library to learn the true size of the underlying file
 *              and to determine whether the hdf5 data has been truncated.
 *
 *              It is also used when a file is first opened to learn whether
 *              the file is empty or not.
 *
 *              It is permissible for the driver to return the maximum address
 *              for the file size if the file is not empty.
 *
 * Return:      Success:    The EOF address.
 *
 *              Failure:    HADDR_UNDEF
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5FDget_eof(H5FD_t *file, H5FD_mem_t type)
{
    haddr_t ret_value;

    FUNC_ENTER_API(HADDR_UNDEF)
    H5TRACE2("a", "*xMt", file, type);

    /* Check arguments */
    if (!file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, HADDR_UNDEF, "file pointer cannot be NULL")
    if (!file->cls)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, HADDR_UNDEF, "file class pointer cannot be NULL")

    /* Call private function */
    if (HADDR_UNDEF == (ret_value = H5FD_get_eof(file, type)))
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, HADDR_UNDEF, "file get eof request failed")

    /* (Note compensating for base address subtraction in internal routine) */
    ret_value += file->base_addr;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5FDget_eof() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_get_maxaddr
 *
 * Purpose:     Private version of H5FDget_eof()
 *
 * Return:      Success:    The maximum address allowed in the file.
 *              Failure:    HADDR_UNDEF
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5FD_get_maxaddr(const H5FD_t *file)
{
    haddr_t ret_value = HADDR_UNDEF; /* Return value */

    FUNC_ENTER_NOAPI(HADDR_UNDEF)

    /* Sanity checks */
    HDassert(file);

    /* Set return value */
    ret_value = file->maxaddr;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_get_maxaddr() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_get_feature_flags
 *
 * Purpose:     Retrieve the feature flags for the VFD
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_get_feature_flags(const H5FD_t *file, unsigned long *feature_flags)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity checks */
    HDassert(file);
    HDassert(feature_flags);

    /* Set feature flags to return */
    *feature_flags = file->feature_flags;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5FD_get_feature_flags() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_set_feature_flags
 *
 * Purpose:     Set the feature flags for the VFD
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_set_feature_flags(H5FD_t *file, unsigned long feature_flags)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity checks */
    HDassert(file);

    /* Set the file's feature flags */
    file->feature_flags = feature_flags;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5FD_set_feature_flags() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_get_fs_type_map
 *
 * Purpose:     Retrieve the free space type mapping for the VFD
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_get_fs_type_map(const H5FD_t *file, H5FD_mem_t *type_map)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(file);
    HDassert(file->cls);
    HDassert(type_map);

    /* Check for VFD class providing a type map retrieval rouine */
    if (file->cls->get_type_map) {
        /* Retrieve type mapping for this file */
        if ((file->cls->get_type_map)(file, type_map) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "driver get type map failed")
    } /* end if */
    else
        /* Copy class's default free space type mapping */
        H5MM_memcpy(type_map, file->cls->fl_map, sizeof(file->cls->fl_map));

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_get_fs_type_map() */

/*-------------------------------------------------------------------------
 * Function:    H5FDread
 *
 * Purpose:     Reads SIZE bytes from FILE beginning at address ADDR
 *              according to the data transfer property list DXPL_ID (which may
 *              be the constant H5P_DEFAULT). The result is written into the
 *              buffer BUF.
 *
 * Return:      Success:    Non-negative
 *                          The read result is written into the BUF buffer
 *                          which should be allocated by the caller.
 *
 *              Failure:	Negative
 *                          The contents of BUF are undefined.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDread(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, size_t size, void *buf /*out*/)
{
    herr_t ret_value = SUCCEED; /* Return value             */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "*xMtiazx", file, type, dxpl_id, addr, size, buf);

    /* Check arguments */
    if (!file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file pointer cannot be NULL")
    if (!file->cls)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file class pointer cannot be NULL")
    if (!buf)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "result buffer parameter can't be NULL")

    /* Get the default dataset transfer property list if the user didn't provide one */
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id = H5P_DATASET_XFER_DEFAULT;
    else if (TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data transfer property list")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Call private function */
    /* (Note compensating for base address addition in internal routine) */
    if (H5FD_read(file, type, addr - file->base_addr, size, buf) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "file read request failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5FDread() */

/*-------------------------------------------------------------------------
 * Function:    H5FDwrite
 *
 * Purpose:     Writes SIZE bytes to FILE beginning at address ADDR according
 *              to the data transfer property list DXPL_ID (which may be the
 *              constant H5P_DEFAULT). The bytes to be written come from the
 *              buffer BUF.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDwrite(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, size_t size, const void *buf)
{
    herr_t ret_value = SUCCEED; /* Return value             */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "*xMtiaz*x", file, type, dxpl_id, addr, size, buf);

    /* Check arguments */
    if (!file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file pointer cannot be NULL")
    if (!file->cls)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file class pointer cannot be NULL")
    if (!buf)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "result buffer parameter can't be NULL")

    /* Get the default dataset transfer property list if the user didn't provide one */
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id = H5P_DATASET_XFER_DEFAULT;
    else if (TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data transfer property list")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Call private function */
    /* (Note compensating for base address addition in internal routine) */
    if (H5FD_write(file, type, addr - file->base_addr, size, buf) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_WRITEERROR, FAIL, "file write request failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5FDwrite() */

/*-------------------------------------------------------------------------
 * Function:    H5FDflush
 *
 * Purpose:     Notify driver to flush all cached data.  If the driver has no
 *              flush method then nothing happens.
 *
 * Return:      Non-negative on success/Negative on failureL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDflush(H5FD_t *file, hid_t dxpl_id, hbool_t closing)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "*xib", file, dxpl_id, closing);

    /* Check arguments */
    if (!file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file pointer cannot be NULL")
    if (!file->cls)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file class pointer cannot be NULL")

    if (H5P_DEFAULT == dxpl_id)
        dxpl_id = H5P_DATASET_XFER_DEFAULT;
    else if (TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data transfer property list")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Call private function */
    if (H5FD_flush(file, closing) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTFLUSH, FAIL, "file flush request failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5FDflush() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_flush
 *
 * Purpose:     Private version of H5FDflush()
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_flush(H5FD_t *file, hbool_t closing)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(file);
    HDassert(file->cls);

    /* Dispatch to driver */
    if (file->cls->flush && (file->cls->flush)(file, H5CX_get_dxpl(), closing) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "driver flush request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_flush() */

/*-------------------------------------------------------------------------
 * Function:    H5FDtruncate
 *
 * Purpose:     Notify driver to truncate the file back to the allocated size.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDtruncate(H5FD_t *file, hid_t dxpl_id, hbool_t closing)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "*xib", file, dxpl_id, closing);

    /* Check arguments */
    if (!file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file pointer cannot be NULL")
    if (!file->cls)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file class pointer cannot be NULL")
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id = H5P_DATASET_XFER_DEFAULT;
    else if (TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data transfer property list")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Call private function */
    if (H5FD_truncate(file, closing) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTUPDATE, FAIL, "file flush request failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5FDtruncate() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_truncate
 *
 * Purpose:     Private version of H5FDtruncate()
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_truncate(H5FD_t *file, hbool_t closing)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(file);
    HDassert(file->cls);

    /* Dispatch to driver */
    if (file->cls->truncate && (file->cls->truncate)(file, H5CX_get_dxpl(), closing) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTUPDATE, FAIL, "driver truncate request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_truncate() */

/*-------------------------------------------------------------------------
 * Function:    H5FDlock
 *
 * Purpose:     Set a file lock
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDlock(H5FD_t *file, hbool_t rw)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "*xb", file, rw);

    /* Check arguments */
    if (!file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file pointer cannot be NULL")
    if (!file->cls)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file class pointer cannot be NULL")

    /* Call private function */
    if (H5FD_lock(file, rw) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTLOCKFILE, FAIL, "file lock request failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5FDlock() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_lock
 *
 * Purpose:     Private version of H5FDlock()
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_lock(H5FD_t *file, hbool_t rw)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(file);
    HDassert(file->cls);

    /* Dispatch to driver */
    if (file->cls->lock && (file->cls->lock)(file, rw) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTLOCKFILE, FAIL, "driver lock request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_lock() */

/*-------------------------------------------------------------------------
 * Function:    H5FDunlock
 *
 * Purpose:     Remove a file lock
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDunlock(H5FD_t *file)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "*x", file);

    /* Check arguments */
    if (!file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file pointer cannot be NULL")
    if (!file->cls)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file class pointer cannot be NULL")

    /* Call private function */
    if (H5FD_unlock(file) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTUNLOCKFILE, FAIL, "file unlock request failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5FDunlock() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_unlock
 *
 * Purpose:     Private version of H5FDunlock()
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_unlock(H5FD_t *file)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(file);
    HDassert(file->cls);

    /* Dispatch to driver */
    if (file->cls->unlock && (file->cls->unlock)(file) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTUNLOCKFILE, FAIL, "driver unlock request failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_unlock() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_get_fileno
 *
 * Purpose:     Quick and dirty routine to retrieve the file's 'fileno' value
 *              (Mainly added to stop non-file routines from poking about in the
 *              H5FD_t data structure)
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_get_fileno(const H5FD_t *file, unsigned long *filenum)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity checks */
    HDassert(file);
    HDassert(filenum);

    /* Retrieve the file's serial number */
    *filenum = file->fileno;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5FD_get_fileno() */

/*--------------------------------------------------------------------------
 * Function:    H5FDget_vfd_handle
 *
 * Purpose:     Returns a pointer to the file handle of low-level virtual
 *              file driver.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5FDget_vfd_handle(H5FD_t *file, hid_t fapl_id, void **file_handle)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "*xi**x", file, fapl_id, file_handle);

    /* Check arguments */
    if (!file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file pointer cannot be NULL")
    if (!file->cls)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file class pointer cannot be NULL")
    if (FALSE == H5P_isa_class(fapl_id, H5P_FILE_ACCESS))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "fapl_id parameter is not a file access property list")
    if (!file_handle)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file handle parameter cannot be NULL")

    /* Call private function */
    if (H5FD_get_vfd_handle(file, fapl_id, file_handle) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't get file handle for file driver")

done:
    if (FAIL == ret_value)
        *file_handle = NULL;

    FUNC_LEAVE_API(ret_value)
} /* end H5FDget_vfd_handle() */

/*--------------------------------------------------------------------------
 * Function:    H5FD_get_vfd_handle
 *
 * Purpose:     Private version of H5FDget_vfd_handle()
 *
 * Return:      SUCCEED/FAIL
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5FD_get_vfd_handle(H5FD_t *file, hid_t fapl_id, void **file_handle)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(file);
    HDassert(file->cls);
    HDassert(file_handle);

    /* Dispatch to driver */
    if (NULL == file->cls->get_handle)
        HGOTO_ERROR(H5E_VFL, H5E_UNSUPPORTED, FAIL, "file driver has no `get_vfd_handle' method")
    if ((file->cls->get_handle)(file, fapl_id, file_handle) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't get file handle for file driver")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_get_vfd_handle() */

/*--------------------------------------------------------------------------
 * Function:    H5FD_set_base_addr
 *
 * Purpose:     Set the base address for the file
 *
 * Return:      SUCCEED (Can't fail)
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5FD_set_base_addr(H5FD_t *file, haddr_t base_addr)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity checks */
    HDassert(file);
    HDassert(H5F_addr_defined(base_addr));

    /* Set the file's base address */
    file->base_addr = base_addr;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5FD_set_base_addr() */

/*--------------------------------------------------------------------------
 * Function:    H5FD_get_base_addr
 *
 * Purpose:     Get the base address for the file
 *
 * Return:      Success:    The absolute base address of the file
 *                          (Can't fail)
 *
 *--------------------------------------------------------------------------
 */
haddr_t
H5FD_get_base_addr(const H5FD_t *file)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity checks */
    HDassert(file);

    /* Return the file's base address */
    FUNC_LEAVE_NOAPI(file->base_addr)
} /* end H5FD_get_base_addr() */

/*--------------------------------------------------------------------------
 * Function:    H5FD_set_paged_aggr
 *
 * Purpose:     Set "paged_aggr" for the file.
 *
 * Return:      SUCCEED (Can't fail)
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5FD_set_paged_aggr(H5FD_t *file, hbool_t paged)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity checks */
    HDassert(file);

    /* Indicate whether paged aggregation for handling file space is enabled or not */
    file->paged_aggr = paged;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5FD_set_paged_aggr() */

/*-------------------------------------------------------------------------
 * Function: H5FDdriver_query
 *
 * Purpose:  Similar to H5FD_query(), but intended for cases when we don't
 *           have a file available (e.g. before one is opened). Since we
 *           can't use the file to get the driver, the driver ID is passed
 *           in as a parameter.
 *
 * Return:   Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FDdriver_query(hid_t driver_id, unsigned long *flags /*out*/)
{
    H5FD_class_t *driver    = NULL;    /* Pointer to VFD class struct  */
    herr_t        ret_value = SUCCEED; /* Return value                 */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "ix", driver_id, flags);

    /* Check arguments */
    if (NULL == flags)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "flags parameter cannot be NULL")

    /* Check for the driver to query and then query it */
    if (NULL == (driver = (H5FD_class_t *)H5I_object_verify(driver_id, H5I_VFL)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "not a VFL ID")
    if (H5FD_driver_query(driver, flags) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL, "driver flag query failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5FDdriver_query() */
