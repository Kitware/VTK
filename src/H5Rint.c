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

#include "H5Rmodule.h"          /* This source code file is part of the H5R module */


/***********/
/* Headers */
/***********/
#include "H5private.h"          /* Generic Functions                        */
#include "H5ACprivate.h"        /* Metadata cache                           */
#include "H5Dprivate.h"         /* Datasets                                 */
#include "H5Eprivate.h"         /* Error handling                           */
#include "H5Gprivate.h"         /* Groups                                   */
#include "H5HGprivate.h"        /* Global Heaps                             */
#include "H5Iprivate.h"         /* IDs                                      */
#include "H5MMprivate.h"        /* Memory management                        */
#include "H5Oprivate.h"         /* Object headers                           */
#include "H5Rpkg.h"             /* References                               */
#include "H5Sprivate.h"         /* Dataspaces                               */
#include "H5Tprivate.h"         /* Datatypes                                */


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

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/

/* Reference ID class
 *
 * NOTE: H5I_REFERENCE is not used by the library and has been deprecated
 *       with a tentative removal version of 1.12.0. (DER, July 2017)
 */
static const H5I_class_t H5I_REFERENCE_CLS[1] = {{
    H5I_REFERENCE,          /* ID class value                                       */
    0,                      /* Class flags                                          */
    0,                      /* # of reserved IDs for class                          */
    NULL                    /* Callback routine for closing objects of this class   */
}};

/* Flag indicating "top" of interface has been initialized */
static hbool_t H5R_top_package_initialize_s = FALSE;


/*--------------------------------------------------------------------------
NAME
   H5R__init_package -- Initialize interface-specific information
USAGE
    herr_t H5R__init_package()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.

--------------------------------------------------------------------------*/
herr_t
H5R__init_package(void)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Initialize the atom group for the file IDs */
    if (H5I_register_type(H5I_REFERENCE_CLS) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTINIT, FAIL, "unable to initialize interface")

    /* Mark "top" of interface as initialized, too */
    H5R_top_package_initialize_s = TRUE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5R__init_package() */


/*--------------------------------------------------------------------------
 NAME
    H5R_top_term_package
 PURPOSE
    Terminate various H5R objects
 USAGE
    void H5R_top_term_package()
 RETURNS
    void
 DESCRIPTION
    Release IDs for the atom group, deferring full interface shutdown
    until later (in H5R_term_package).
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
     Can't report errors...
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5R_top_term_package(void)
{
    int	n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if (H5R_top_package_initialize_s) {
        if (H5I_nmembers(H5I_REFERENCE) > 0) {
            (void)H5I_clear_type(H5I_REFERENCE, FALSE, FALSE);
            n++;
	    }

        /* Mark closed */
        if (0 == n)
            H5R_top_package_initialize_s = FALSE;
    }

    FUNC_LEAVE_NOAPI(n)
} /* end H5R_top_term_package() */


/*--------------------------------------------------------------------------
 NAME
    H5R_term_package
 PURPOSE
    Terminate various H5R objects
 USAGE
    void H5R_term_package()
 RETURNS
    void
 DESCRIPTION
    Release the atom group and any other resources allocated.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
     Can't report errors...

     Finishes shutting down the interface, after H5R_top_term_package()
     is called
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5R_term_package(void)
{
    int	n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if (H5_PKG_INIT_VAR) {
        /* Sanity checks */
        HDassert(0 == H5I_nmembers(H5I_REFERENCE));
        HDassert(FALSE == H5R_top_package_initialize_s);

        /* Destroy the reference id group */
        n += (H5I_dec_type_ref(H5I_REFERENCE) > 0);

        /* Mark closed */
        if (0 == n)
            H5_PKG_INIT_VAR = FALSE;
    }

    FUNC_LEAVE_NOAPI(n)
} /* end H5R_term_package() */


/*--------------------------------------------------------------------------
 NAME
    H5R__create
 PURPOSE
    Creates a particular kind of reference for the user
 USAGE
    herr_t H5R__create(ref, loc, name, ref_type, space)
        void *ref;          OUT: Reference created
        H5G_loc_t *loc;     IN: File location used to locate object pointed to
        const char *name;   IN: Name of object at location LOC_ID of object
                                    pointed to
        H5R_type_t ref_type;    IN: Type of reference to create
        H5S_t *space;       IN: Dataspace ID with selection, used for Dataset
                                    Region references.

 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Creates a particular type of reference specified with REF_TYPE, in the
    space pointed to by REF.  The LOC_ID and NAME are used to locate the object
    pointed to and the SPACE_ID is used to choose the region pointed to (for
    Dataset Region references).
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5R__create(void *_ref, H5G_loc_t *loc, const char *name, H5R_type_t ref_type,
    H5S_t *space)
{
    H5G_loc_t	obj_loc;		/* Group hier. location of object */
    H5G_name_t  path;            	/* Object group hier. path */
    H5O_loc_t   oloc;            	/* Object object location */
    hbool_t     obj_found = FALSE;      /* Object location found */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_PACKAGE

    HDassert(_ref);
    HDassert(loc);
    HDassert(name);
    HDassert(ref_type > H5R_BADTYPE && ref_type < H5R_MAXTYPE);

    /* Set up object location to fill in */
    obj_loc.oloc = &oloc;
    obj_loc.path = &path;
    H5G_loc_reset(&obj_loc);

    /* Set the FAPL for the API context */
    H5CX_set_libver_bounds(loc->oloc->file);

    /* Find the object */
    if(H5G_loc_find(loc, name, &obj_loc) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_NOTFOUND, FAIL, "object not found")
    obj_found = TRUE;

    switch (ref_type) {
        case H5R_OBJECT:
        {
            hobj_ref_t *ref = (hobj_ref_t *)_ref; /* Get pointer to correct type of reference struct */

            *ref = obj_loc.oloc->addr;
            break;
        }

        case H5R_DATASET_REGION:
        {
            H5HG_t hobjid;      /* Heap object ID */
            hdset_reg_ref_t *ref = (hdset_reg_ref_t *)_ref; /* Get pointer to correct type of reference struct */
            hssize_t buf_size;  /* Size of buffer needed to serialize selection */
            uint8_t *p;       /* Pointer to OID to store */
            uint8_t *buf;     /* Buffer to store serialized selection in */
            unsigned heapid_found;  /* Flag for non-zero heap ID found */
            unsigned u;        /* local index */

            /* Set up information for dataset region */

            /* Return any previous heap block to the free list if we are
             * garbage collecting
             */
            if (H5F_GC_REF(loc->oloc->file)) {
                /* Check for an existing heap ID in the reference */
                for (u = 0, heapid_found = 0, p = (uint8_t *)ref; u < H5R_DSET_REG_REF_BUF_SIZE; u++)
                    if (p[u] != 0) {
                        heapid_found = 1;
                        break;
                    }

                if (heapid_found != 0) {
                    /* Return heap block to free list */
                }
            }

            /* Zero the heap ID out, may leak heap space if user is re-using
             * reference and doesn't have garbage collection turned on
             */
            HDmemset(ref, 0, H5R_DSET_REG_REF_BUF_SIZE);

            /* Get the amount of space required to serialize the selection */
            if ((buf_size = H5S_SELECT_SERIAL_SIZE(space)) < 0)
                HGOTO_ERROR(H5E_REFERENCE, H5E_CANTINIT, FAIL, "Invalid amount of space for serializing selection")

            /* Increase buffer size to allow for the dataset OID */
            buf_size += (hssize_t)sizeof(haddr_t);

            /* Allocate the space to store the serialized information */
            H5_CHECK_OVERFLOW(buf_size, hssize_t, size_t);
            if (NULL == (buf = (uint8_t *)H5MM_malloc((size_t)buf_size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

            /* Serialize information for dataset OID into heap buffer */
            p = (uint8_t *)buf;
            H5F_addr_encode(loc->oloc->file, &p, obj_loc.oloc->addr);

            /* Serialize the selection into heap buffer */
            if (H5S_SELECT_SERIALIZE(space, &p) < 0)
                HGOTO_ERROR(H5E_REFERENCE, H5E_CANTCOPY, FAIL, "Unable to serialize selection")

            /* Save the serialized buffer for later */
            H5_CHECK_OVERFLOW(buf_size, hssize_t, size_t);
            if(H5HG_insert(loc->oloc->file, (size_t)buf_size, buf, &hobjid) < 0)
                HGOTO_ERROR(H5E_REFERENCE, H5E_WRITEERROR, FAIL, "Unable to serialize selection")

            /* Serialize the heap ID and index for storage in the file */
            p = (uint8_t *)ref;
            H5F_addr_encode(loc->oloc->file, &p, hobjid.addr);
            UINT32ENCODE(p, hobjid.idx);

            /* Free the buffer we serialized data in */
            H5MM_xfree(buf);
            break;
        } /* end case H5R_DATASET_REGION */

        case H5R_BADTYPE:
        case H5R_MAXTYPE:
        default:
            HDassert("unknown reference type" && 0);
            HGOTO_ERROR(H5E_REFERENCE, H5E_UNSUPPORTED, FAIL, "internal error (unknown reference type)")
    } /* end switch */

done:
    if (obj_found)
        H5G_loc_free(&obj_loc);

    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5R__create() */


/*--------------------------------------------------------------------------
 NAME
    H5R__dereference
 PURPOSE
    Opens the HDF5 object referenced.
 USAGE
    hid_t H5R__dereference(ref, oapl_id, ref_type, ref)
        H5F_t *file;        IN: File the object being dereferenced is within
        hid_t oapl_id;      IN: Object access property list ID
        H5R_type_t ref_type; IN: Type of reference
        void *ref;          IN: Reference to open.

 RETURNS
    Valid ID on success, Negative on failure
 DESCRIPTION
    Given a reference to some object, open that object and return an ID for
    that object.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    Currently only set up to work with references to datasets
 EXAMPLES
 REVISION LOG
    Raymond Lu
    13 July 2011
    I added the OAPL_ID parameter for the object being referenced.  It only
    supports dataset access property list currently.

    M. Scot Breitenfeld
    3 March 2015
    Added a check for undefined reference pointer.
--------------------------------------------------------------------------*/
hid_t
H5R__dereference(H5F_t *file, hid_t oapl_id, H5R_type_t ref_type,
    const void *_ref)
{
    H5O_loc_t oloc;             /* Object location */
    H5G_name_t path;            /* Path of object */
    H5G_loc_t loc;              /* Group location */
    unsigned rc;		/* Reference count of object */
    H5O_type_t obj_type;        /* Type of object */
    hid_t ret_value = H5I_INVALID_HID;  /* Return value */

    FUNC_ENTER_PACKAGE

    HDassert(_ref);
    HDassert(ref_type > H5R_BADTYPE && ref_type < H5R_MAXTYPE);
    HDassert(file);

    /* Initialize the object location */
    H5O_loc_reset(&oloc);
    oloc.file = file;

    switch (ref_type) {
        case H5R_OBJECT:
        {
            oloc.addr = *(const hobj_ref_t *)_ref; /* Only object references currently supported */
            if (!H5F_addr_defined(oloc.addr) || oloc.addr == 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "Undefined reference pointer")
            break;
        }

        case H5R_DATASET_REGION:
        {
            H5HG_t hobjid;  /* Heap object ID */
            uint8_t *buf;   /* Buffer to store serialized selection in */
            const uint8_t *p;           /* Pointer to OID to store */

            /* Get the heap ID for the dataset region */
            p = (const uint8_t *)_ref;
            H5F_addr_decode(oloc.file, &p, &(hobjid.addr));
            UINT32DECODE(p, hobjid.idx);

            if (!H5F_addr_defined(hobjid.addr) || hobjid.addr == 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "Undefined reference pointer")

            /* Get the dataset region from the heap (allocate inside routine) */
            if(NULL == (buf = (uint8_t *)H5HG_read(oloc.file, &hobjid, NULL, NULL)))
                HGOTO_ERROR(H5E_REFERENCE, H5E_READERROR, H5I_INVALID_HID, "Unable to read dataset region information")

            /* Get the object oid for the dataset */
            p = buf;
            H5F_addr_decode(oloc.file, &p, &(oloc.addr));

            /* Free the buffer allocated in H5HG_read() */
            H5MM_xfree(buf);
            break;
        } /* end case H5R_DATASET_REGION */

        case H5R_BADTYPE:
        case H5R_MAXTYPE:
        default:
            HDassert("unknown reference type" && 0);
            HGOTO_ERROR(H5E_REFERENCE, H5E_UNSUPPORTED, H5I_INVALID_HID, "internal error (unknown reference type)")
    } /* end switch */

    /* Get the # of links for object, and its type
     * (To check to make certain that this object hasn't been deleted
     *  since the reference was created)
     */
    if(H5O_get_rc_and_type(&oloc, &rc, &obj_type) < 0 || 0 == rc)
        HGOTO_ERROR(H5E_REFERENCE, H5E_LINKCOUNT, H5I_INVALID_HID, "dereferencing deleted object")

    /* Construct a group location for opening the object */
    H5G_name_reset(&path);
    loc.oloc = &oloc;
    loc.path = &path;

    /* Open the object */
    switch (obj_type) {
        case H5O_TYPE_GROUP:
        {
            H5G_t *group;               /* Pointer to group to open */

            if(NULL == (group = H5G_open(&loc)))
                HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, H5I_INVALID_HID, "not found")

            /* Create an atom for the group */
            if((ret_value = H5I_register(H5I_GROUP, group, TRUE)) < 0) {
                H5G_close(group);
                HGOTO_ERROR(H5E_SYM, H5E_CANTREGISTER, H5I_INVALID_HID, "can't register group")
            } /* end if */

            break;
        }

        case H5O_TYPE_NAMED_DATATYPE:
        {
            H5T_t *type;                /* Pointer to datatype to open */

            if(NULL == (type = H5T_open(&loc)))
                HGOTO_ERROR(H5E_DATATYPE, H5E_NOTFOUND, H5I_INVALID_HID, "not found")

            /* Create an atom for the datatype */
            if((ret_value = H5I_register(H5I_DATATYPE, type, TRUE)) < 0) {
                H5T_close(type);
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, H5I_INVALID_HID, "can't register datatype")
            } /* end if */

            break;
        }

        case H5O_TYPE_DATASET:
        {
            H5D_t *dset;                /* Pointer to dataset to open */

            /* Open the dataset */
            if(NULL == (dset = H5D_open(&loc, oapl_id)))
                HGOTO_ERROR(H5E_DATASET, H5E_NOTFOUND, H5I_INVALID_HID, "not found")

            /* Create an atom for the dataset */
            if((ret_value = H5I_register(H5I_DATASET, dset, TRUE)) < 0) {
                H5D_close(dset);
                HGOTO_ERROR(H5E_DATASET, H5E_CANTREGISTER, H5I_INVALID_HID, "can't register dataset")
            } /* end if */

            break;
        }

        case H5O_TYPE_UNKNOWN:
        case H5O_TYPE_NTYPES:
        default:
            HGOTO_ERROR(H5E_REFERENCE, H5E_BADTYPE, H5I_INVALID_HID, "can't identify type of object referenced")
     } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5R__dereference() */


/*--------------------------------------------------------------------------
 NAME
    H5R__get_region
 PURPOSE
    Retrieves a dataspace with the region pointed to selected.
 USAGE
    H5S_t *H5R__get_region(file, ref)
        H5F_t *file;        IN: File the object being dereferenced is within
        void *ref;          IN: Reference to open.

 RETURNS
    Pointer to the dataspace on success, NULL on failure
 DESCRIPTION
    Given a reference to some object, creates a copy of the dataset pointed
    to's dataspace and defines a selection in the copy which is the region
    pointed to.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5S_t *
H5R__get_region(H5F_t *file, const void *_ref)
{
    H5O_loc_t oloc;             /* Object location */
    const uint8_t *p;           /* Pointer to OID to store */
    H5HG_t hobjid;              /* Heap object ID */
    uint8_t *buf = NULL;        /* Buffer to store serialized selection in */
    H5S_t *ret_value;		/* Return value */

    FUNC_ENTER_PACKAGE

    HDassert(_ref);
    HDassert(file);

    /* Initialize the object location */
    H5O_loc_reset(&oloc);
    oloc.file = file;

    /* Get the heap ID for the dataset region */
    p = (const uint8_t *)_ref;
    H5F_addr_decode(oloc.file, &p, &(hobjid.addr));
    UINT32DECODE(p, hobjid.idx);

    /* Get the dataset region from the heap (allocate inside routine) */
    if(NULL == (buf = (uint8_t *)H5HG_read(oloc.file, &hobjid, NULL, NULL)))
        HGOTO_ERROR(H5E_REFERENCE, H5E_READERROR, NULL, "Unable to read dataset region information")

    /* Get the object oid for the dataset */
    p = buf;
    H5F_addr_decode(oloc.file, &p, &(oloc.addr));

    /* Open and copy the dataset's dataspace */
    if(NULL == (ret_value = H5S_read(&oloc)))
        HGOTO_ERROR(H5E_DATASPACE, H5E_NOTFOUND, NULL, "not found")

    /* Unserialize the selection */
    if(H5S_SELECT_DESERIALIZE(&ret_value, &p) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTDECODE, NULL, "can't deserialize selection")

done:
    /* Free the buffer allocated in H5HG_read() */
    if(buf)
        H5MM_xfree(buf);

    FUNC_LEAVE_NOAPI(ret_value)
}  /* end H5R__get_region() */


/*--------------------------------------------------------------------------
 NAME
    H5R__get_obj_type
 PURPOSE
    Retrieves the type of object that an object reference points to
 USAGE
    H5O_type_t H5R__get_obj_type(file, ref_type, ref)
        H5F_t *file;        IN: File the object being dereferenced is within
        H5R_type_t ref_type;    IN: Type of reference to query
        void *ref;          IN: Reference to query.
        H5O_type_t *obj_type;	OUT: The type of the object, set on success

 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Given a reference to some object, this function returns the type of object
    pointed to.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5R__get_obj_type(H5F_t *file, H5R_type_t ref_type, const void *_ref,
    H5O_type_t *obj_type)
{
    H5O_loc_t oloc;             /* Object location */
    unsigned rc;		/* Reference count of object    */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    HDassert(file);
    HDassert(_ref);

    /* Initialize the symbol table entry */
    H5O_loc_reset(&oloc);
    oloc.file = file;

    switch (ref_type) {
        case H5R_OBJECT:
        {
            /* Get the object oid */
            oloc.addr = *(const hobj_ref_t *)_ref; /* Only object references currently supported */
            break;
        }

        case H5R_DATASET_REGION:
        {
            H5HG_t hobjid;      /* Heap object ID */
            const uint8_t *p;   /* Pointer to reference to decode */
            uint8_t *buf;       /* Buffer to store serialized selection in */

            /* Get the heap ID for the dataset region */
            p = (const uint8_t *)_ref;
            H5F_addr_decode(oloc.file, &p, &(hobjid.addr));
            UINT32DECODE(p, hobjid.idx);

            /* Get the dataset region from the heap (allocate inside routine) */
            if(NULL == (buf = (uint8_t *)H5HG_read(oloc.file, &hobjid, NULL, NULL)))
                HGOTO_ERROR(H5E_REFERENCE, H5E_READERROR, FAIL, "Unable to read dataset region information")

            /* Get the object oid for the dataset */
            p = buf;
            H5F_addr_decode(oloc.file, &p, &(oloc.addr));

            /* Free the buffer allocated in H5HG_read() */
            H5MM_xfree(buf);

            break;
        }

        case H5R_BADTYPE:
        case H5R_MAXTYPE:
        default:
            HDassert("unknown reference type" && 0);
            HGOTO_ERROR(H5E_REFERENCE, H5E_UNSUPPORTED, FAIL, "internal error (unknown reference type)")
    } /* end switch */

    /* Get the # of links for object, and its type */
    /* (To check to make certain that this object hasn't been deleted since the reference was created) */
    if(H5O_get_rc_and_type(&oloc, &rc, obj_type) < 0 || 0 == rc)
        HGOTO_ERROR(H5E_REFERENCE, H5E_LINKCOUNT, FAIL, "dereferencing deleted object")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5R__get_obj_type() */


/*--------------------------------------------------------------------------
 NAME
    H5R__get_name
 PURPOSE
    Internal routine to determine a name for the object referenced
 USAGE
    ssize_t H5R__get_name(f, ref_type, ref, name, size)
        H5F_t *f;       IN: Pointer to the file that the reference is pointing
                            into
        hid_t lapl_id;  IN: LAPL to use for operation
        hid_t id;       IN: Location ID given for reference
        H5R_type_t ref_type;    IN: Type of reference
        void *_ref;     IN: Reference to query.
        char *name;     OUT: Buffer to place name of object referenced
        size_t size;    IN: Size of name buffer

 RETURNS
    Non-negative length of the path on success, -1 on failure
 DESCRIPTION
    Given a reference to some object, determine a path to the object
    referenced in the file.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This may not be the only path to that object.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
ssize_t
H5R__get_name(H5F_t *f, hid_t id, H5R_type_t ref_type, const void *_ref,
    char *name, size_t size)
{
    hid_t file_id = H5I_INVALID_HID;    /* ID for file that the reference is in */
    H5O_loc_t oloc;             /* Object location describing object for reference */
    ssize_t ret_value = -1;     /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(f);
    HDassert(_ref);

    /* Initialize the object location */
    H5O_loc_reset(&oloc);
    oloc.file = f;

    /* Get address for reference */
    switch (ref_type) {
        case H5R_OBJECT:
        {
            oloc.addr = *(const hobj_ref_t *)_ref;
            break;
        }

        case H5R_DATASET_REGION:
        {
            H5HG_t hobjid;  /* Heap object ID */
            uint8_t *buf;   /* Buffer to store serialized selection in */
            const uint8_t *p;           /* Pointer to OID to store */

            /* Get the heap ID for the dataset region */
            p = (const uint8_t *)_ref;
            H5F_addr_decode(oloc.file, &p, &(hobjid.addr));
            UINT32DECODE(p, hobjid.idx);

            /* Get the dataset region from the heap (allocate inside routine) */
            if ((buf = (uint8_t *)H5HG_read(oloc.file, &hobjid, NULL, NULL)) == NULL)
                HGOTO_ERROR(H5E_REFERENCE, H5E_READERROR, (-1), "Unable to read dataset region information")

            /* Get the object oid for the dataset */
            p = buf;
            H5F_addr_decode(oloc.file, &p, &(oloc.addr));

            /* Free the buffer allocated in H5HG_read() */
            H5MM_xfree(buf);

            break;
        }

        case H5R_BADTYPE:
        case H5R_MAXTYPE:
        default:
            HGOTO_ERROR(H5E_REFERENCE, H5E_UNSUPPORTED, (-1), "internal error (unknown reference type)")
    } /* end switch */

    /* Retrieve file ID for name search */
    if ((file_id = H5F_get_id(f, FALSE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTGET, (-1), "can't get file ID")

    /* Get name, length, etc. */
    if ((ret_value = H5G_get_name_by_addr(file_id, &oloc, name, size)) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, (-1), "can't determine name")

done:
    /* Close file ID used for search */
    if (file_id > 0 && H5I_dec_ref(file_id) < 0)
        HDONE_ERROR(H5E_REFERENCE, H5E_CANTDEC, (-1), "can't decrement ref count of temp ID")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5R__get_name() */

