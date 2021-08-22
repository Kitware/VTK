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
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Purpose:     This is the MPI-2 I/O driver.
 *
 */

#include "H5FDdrvr_module.h" /* This source code file is part of the H5FD driver module */

#include "H5private.h"   /* Generic Functions                    */
#include "H5CXprivate.h" /* API Contexts                         */
#include "H5Dprivate.h"  /* Dataset functions                    */
#include "H5Eprivate.h"  /* Error handling                       */
#include "H5Fprivate.h"  /* File access                          */
#include "H5FDprivate.h" /* File drivers                         */
#include "H5FDmpi.h"     /* MPI-based file drivers               */
#include "H5Iprivate.h"  /* IDs                                  */
#include "H5MMprivate.h" /* Memory management                    */
#include "H5Pprivate.h"  /* Property lists                       */

#ifdef H5_HAVE_PARALLEL

/*
 * The driver identification number, initialized at runtime if H5_HAVE_PARALLEL
 * is defined. This allows applications to still have the H5FD_MPIO
 * "constants" in their source code.
 */
static hid_t H5FD_MPIO_g = 0;

/* Whether to allow collective I/O operations */
/* (Can be changed by setting "HDF5_MPI_OPT_TYPES" environment variable to '0' or '1') */
hbool_t H5FD_mpi_opt_types_g = TRUE;

/*
 * The view is set to this value
 */
static char H5FD_mpi_native_g[] = "native";

/*
 * The description of a file belonging to this driver.
 * The EOF value is only used just after the file is opened in order for the
 * library to determine whether the file is empty, truncated, or okay. The MPIO
 * driver doesn't bother to keep it updated since it's an expensive operation.
 */
typedef struct H5FD_mpio_t {
    H5FD_t   pub;       /* Public stuff, must be first                  */
    MPI_File f;         /* MPIO file handle                             */
    MPI_Comm comm;      /* MPI Communicator                             */
    MPI_Info info;      /* MPI info object                              */
    int      mpi_rank;  /* This process's rank                          */
    int      mpi_size;  /* Total number of processes                    */
    haddr_t  eof;       /* End-of-file marker                           */
    haddr_t  eoa;       /* End-of-address marker                        */
    haddr_t  last_eoa;  /* Last known end-of-address marker             */
    haddr_t  local_eof; /* Local end-of-file address for each process   */
} H5FD_mpio_t;

/* Private Prototypes */

/* Callbacks */
static herr_t   H5FD__mpio_term(void);
static H5FD_t * H5FD__mpio_open(const char *name, unsigned flags, hid_t fapl_id, haddr_t maxaddr);
static herr_t   H5FD__mpio_close(H5FD_t *_file);
static herr_t   H5FD__mpio_query(const H5FD_t *_f1, unsigned long *flags);
static haddr_t  H5FD__mpio_get_eoa(const H5FD_t *_file, H5FD_mem_t type);
static herr_t   H5FD__mpio_set_eoa(H5FD_t *_file, H5FD_mem_t type, haddr_t addr);
static haddr_t  H5FD__mpio_get_eof(const H5FD_t *_file, H5FD_mem_t type);
static herr_t   H5FD__mpio_get_handle(H5FD_t *_file, hid_t fapl, void **file_handle);
static herr_t   H5FD__mpio_read(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, size_t size,
                                void *buf);
static herr_t   H5FD__mpio_write(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, size_t size,
                                 const void *buf);
static herr_t   H5FD__mpio_flush(H5FD_t *_file, hid_t dxpl_id, hbool_t closing);
static herr_t   H5FD__mpio_truncate(H5FD_t *_file, hid_t dxpl_id, hbool_t closing);
static int      H5FD__mpio_mpi_rank(const H5FD_t *_file);
static int      H5FD__mpio_mpi_size(const H5FD_t *_file);
static MPI_Comm H5FD__mpio_communicator(const H5FD_t *_file);

/* The MPIO file driver information */
static const H5FD_class_mpi_t H5FD_mpio_g = {
    {
        /* Start of superclass information */
        "mpio",                /*name			*/
        HADDR_MAX,             /*maxaddr		*/
        H5F_CLOSE_SEMI,        /*fc_degree		*/
        H5FD__mpio_term,       /*terminate             */
        NULL,                  /*sb_size		*/
        NULL,                  /*sb_encode		*/
        NULL,                  /*sb_decode		*/
        0,                     /*fapl_size		*/
        NULL,                  /*fapl_get		*/
        NULL,                  /*fapl_copy		*/
        NULL,                  /*fapl_free		*/
        0,                     /*dxpl_size		*/
        NULL,                  /*dxpl_copy		*/
        NULL,                  /*dxpl_free		*/
        H5FD__mpio_open,       /*open			*/
        H5FD__mpio_close,      /*close			*/
        NULL,                  /*cmp			*/
        H5FD__mpio_query,      /*query			*/
        NULL,                  /*get_type_map		*/
        NULL,                  /*alloc			*/
        NULL,                  /*free			*/
        H5FD__mpio_get_eoa,    /*get_eoa		*/
        H5FD__mpio_set_eoa,    /*set_eoa		*/
        H5FD__mpio_get_eof,    /*get_eof		*/
        H5FD__mpio_get_handle, /*get_handle            */
        H5FD__mpio_read,       /*read			*/
        H5FD__mpio_write,      /*write			*/
        H5FD__mpio_flush,      /*flush			*/
        H5FD__mpio_truncate,   /*truncate		*/
        NULL,                  /*lock                  */
        NULL,                  /*unlock                */
        H5FD_FLMAP_DICHOTOMY   /*fl_map                */
    },                         /* End of superclass information */
    H5FD__mpio_mpi_rank,       /*get_rank              */
    H5FD__mpio_mpi_size,       /*get_size              */
    H5FD__mpio_communicator    /*get_comm              */
};

#ifdef H5FDmpio_DEBUG
/* Flags to control debug actions in the MPI-IO VFD.
 * (Meant to be indexed by characters)
 *
 * These flags can be set with either (or both) the environment variable
 *      "H5FD_mpio_Debug" set to a string containing one or more characters
 *      (flags) or by setting them as a string value for the
 *      "H5F_mpio_debug_key" MPI Info key.
 *
 * Supported characters in 'H5FD_mpio_Debug' string:
 *      't' trace function entry and exit
 *      'r' show read offset and size
 *      'w' show write offset and size
 *      '0'-'9' only show output from a single MPI rank (ranks 0-9 supported)
 */
static int H5FD_mpio_debug_flags_s[256];
static int H5FD_mpio_debug_rank_s = -1;

/* Indicate if this rank should output tracing info */
#define H5FD_MPIO_TRACE_THIS_RANK(file)                                                                      \
    (H5FD_mpio_debug_rank_s < 0 || H5FD_mpio_debug_rank_s == (file)->mpi_rank)
#endif

/*--------------------------------------------------------------------------
NAME
   H5FD__init_package -- Initialize interface-specific information

USAGE
    herr_t H5FD__init_package()

RETURNS
    SUCCEED/FAIL

DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5FD_mpio_init currently).

--------------------------------------------------------------------------*/
static herr_t
H5FD__init_package(void)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    if (H5FD_mpio_init() < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "unable to initialize mpio VFD")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5FD__init_package() */

#ifdef H5FDmpio_DEBUG

/*---------------------------------------------------------------------------
 * Function:    H5FD__mpio_parse_debug_str
 *
 * Purpose:     Parse a string for debugging flags
 *
 * Returns:     N/A
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, Aug 12, 2020
 *
 *---------------------------------------------------------------------------
 */
static void
H5FD__mpio_parse_debug_str(const char *s)
{
    FUNC_ENTER_STATIC_NOERR

    /* Sanity check */
    HDassert(s);

    /* Set debug mask */
    while (*s) {
        if ((int)(*s) >= (int)'0' && (int)(*s) <= (int)'9')
            H5FD_mpio_debug_rank_s = ((int)*s) - (int)'0';
        else
            H5FD_mpio_debug_flags_s[(int)*s]++;
        s++;
    } /* end while */

    FUNC_LEAVE_NOAPI_VOID
} /* end H5FD__mpio_parse_debug_str() */
#endif /* H5FDmpio_DEBUG */

/*-------------------------------------------------------------------------
 * Function:    H5FD_mpio_init
 *
 * Purpose:     Initialize this driver by registering the driver with the
 *              library.
 *
 * Return:      Success:    The driver ID for the mpio driver
 *              Failure:    H5I_INVALID_HID
 *
 * Programmer:  Robb Matzke
 *              Thursday, August 5, 1999
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FD_mpio_init(void)
{
    static int H5FD_mpio_Debug_inited = 0;
    hid_t      ret_value              = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_NOAPI(H5I_INVALID_HID)

    /* Register the MPI-IO VFD, if it isn't already */
    if (H5I_VFL != H5I_get_type(H5FD_MPIO_g))
        H5FD_MPIO_g = H5FD_register((const H5FD_class_t *)&H5FD_mpio_g, sizeof(H5FD_class_mpi_t), FALSE);

    if (!H5FD_mpio_Debug_inited) {
        const char *s; /* String for environment variables */

        /* Allow MPI buf-and-file-type optimizations? */
        s = HDgetenv("HDF5_MPI_OPT_TYPES");
        if (s && HDisdigit(*s))
            H5FD_mpi_opt_types_g = (0 == HDstrtol(s, NULL, 0)) ? FALSE : TRUE;

#ifdef H5FDmpio_DEBUG
        /* Clear the flag buffer */
        HDmemset(H5FD_mpio_debug_flags_s, 0, sizeof(H5FD_mpio_debug_flags_s));

        /* Retrieve MPI-IO debugging environment variable */
        s = HDgetenv("H5FD_mpio_Debug");
        if (s)
            H5FD__mpio_parse_debug_str(s);
#endif /* H5FDmpio_DEBUG */

        H5FD_mpio_Debug_inited++;
    } /* end if */

    /* Set return value */
    ret_value = H5FD_MPIO_g;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_mpio_init() */

/*---------------------------------------------------------------------------
 * Function:    H5FD__mpio_term
 *
 * Purpose:     Shut down the VFD
 *
 * Returns:     Non-negative on success or negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Friday, Jan 30, 2004
 *
 *---------------------------------------------------------------------------
 */
static herr_t
H5FD__mpio_term(void)
{
    FUNC_ENTER_STATIC_NOERR

    /* Reset VFL ID */
    H5FD_MPIO_g = 0;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5FD__mpio_term() */

/*-------------------------------------------------------------------------
 * Function:    H5Pset_fapl_mpio
 *
 * Purpose:     Store the user supplied MPIO communicator comm and info in
 *              the file access property list FAPL_ID which can then be used
 *              to create and/or open the file.  This function is available
 *              only in the parallel HDF5 library and is not collective.
 *
 *              comm is the MPI communicator to be used for file open as
 *              defined in MPI_FILE_OPEN of MPI-2. This function makes a
 *              duplicate of comm. Any modification to comm after this function
 *              call returns has no effect on the access property list.
 *
 *              info is the MPI Info object to be used for file open as
 *              defined in MPI_FILE_OPEN of MPI-2. This function makes a
 *              duplicate of info. Any modification to info after this
 *              function call returns has no effect on the access property
 *              list.
 *
 *              If fapl_id has previously set comm and info values, they
 *              will be replaced and the old communicator and Info object
 *              are freed.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 * Programmer:  Albert Cheng
 *              Feb 3, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fapl_mpio(hid_t fapl_id, MPI_Comm comm, MPI_Info info)
{
    H5P_genplist_t *plist; /* Property list pointer */
    herr_t          ret_value;

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "iMcMi", fapl_id, comm, info);

    /* Check arguments */
    if (fapl_id == H5P_DEFAULT)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "can't set values in default property list")
    if (NULL == (plist = H5P_object_verify(fapl_id, H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a file access list")
    if (MPI_COMM_NULL == comm)
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "MPI_COMM_NULL is not a valid communicator")

    /* Set the MPI communicator and info object */
    if (H5P_set(plist, H5F_ACS_MPI_PARAMS_COMM_NAME, &comm) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set MPI communicator")
    if (H5P_set(plist, H5F_ACS_MPI_PARAMS_INFO_NAME, &info) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set MPI info object")

    /* duplication is done during driver setting. */
    ret_value = H5P_set_driver(plist, H5FD_MPIO, NULL);

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Pset_fapl_mpio() */

/*-------------------------------------------------------------------------
 * Function:    H5Pget_fapl_mpio
 *
 * Purpose:     If the file access property list is set to the H5FD_MPIO
 *              driver then this function returns duplicates of the MPI
 *              communicator and Info object stored through the comm and
 *              info pointers.  It is the responsibility of the application
 *              to free the returned communicator and Info object.
 *
 * Return:      Success:    Non-negative with the communicator and
 *                          Info object returned through the comm and
 *                          info arguments if non-null. Since they are
 *                          duplicates of the stored objects, future
 *                          modifications to the access property list do
 *                          not affect them and it is the responsibility
 *                          of the application to free them.
 *              Failure:    Negative
 *
 * Programmer:  Robb Matzke
 *              Thursday, February 26, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_fapl_mpio(hid_t fapl_id, MPI_Comm *comm /*out*/, MPI_Info *info /*out*/)
{
    H5P_genplist_t *plist;               /* Property list pointer */
    herr_t          ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "ixx", fapl_id, comm, info);

    /* Set comm and info in case we have problems */
    if (comm)
        *comm = MPI_COMM_NULL;
    if (info)
        *info = MPI_INFO_NULL;

    /* Check arguments */
    if (NULL == (plist = H5P_object_verify(fapl_id, H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a file access list")
    if (H5FD_MPIO != H5P_peek_driver(plist))
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "VFL driver is not MPI-I/O")

    /* Get the MPI communicator and info object */
    if (comm)
        if (H5P_get(plist, H5F_ACS_MPI_PARAMS_COMM_NAME, comm) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get MPI communicator")
    if (info)
        if (H5P_get(plist, H5F_ACS_MPI_PARAMS_INFO_NAME, info) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get MPI info object")

done:
    /* Clean up anything duplicated on errors. The free calls will set
     * the output values to MPI_COMM|INFO_NULL.
     */
    if (ret_value != SUCCEED) {
        if (comm)
            if (H5_mpi_comm_free(comm) < 0)
                HDONE_ERROR(H5E_PLIST, H5E_CANTFREE, FAIL, "unable to free MPI communicator")
        if (info)
            if (H5_mpi_info_free(info) < 0)
                HDONE_ERROR(H5E_PLIST, H5E_CANTFREE, FAIL, "unable to free MPI info object")
    }

    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_fapl_mpio() */

/*-------------------------------------------------------------------------
 * Function:    H5Pset_dxpl_mpio
 *
 * Purpose:     Set the data transfer property list DXPL_ID to use transfer
 *              mode XFER_MODE. The property list can then be used to control
 *              the I/O transfer mode during data I/O operations. The valid
 *              transfer modes are:
 *
 *              H5FD_MPIO_INDEPENDENT:
 *                  Use independent I/O access (the default).
 *
 *              H5FD_MPIO_COLLECTIVE:
 *                  Use collective I/O access.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 * Programmer:  Albert Cheng
 *              April 2, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_dxpl_mpio(hid_t dxpl_id, H5FD_mpio_xfer_t xfer_mode)
{
    H5P_genplist_t *plist;               /* Property list pointer */
    herr_t          ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "iDt", dxpl_id, xfer_mode);

    /* Check arguments */
    if (dxpl_id == H5P_DEFAULT)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "can't set values in default property list")
    if (NULL == (plist = H5P_object_verify(dxpl_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a dxpl")
    if (H5FD_MPIO_INDEPENDENT != xfer_mode && H5FD_MPIO_COLLECTIVE != xfer_mode)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "incorrect xfer_mode")

    /* Set the transfer mode */
    if (H5P_set(plist, H5D_XFER_IO_XFER_MODE_NAME, &xfer_mode) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_dxpl_mpio() */

/*-------------------------------------------------------------------------
 * Function:    H5Pget_dxpl_mpio
 *
 * Purpose:     Queries the transfer mode current set in the data transfer
 *              property list DXPL_ID. This is not collective.
 *
 * Return:      Success:    Non-negative, with the transfer mode returned
 *                          through the XFER_MODE argument if it is
 *                          non-null.
 *              Failure:    Negative
 *
 * Programmer:  Albert Cheng
 *              April 2, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_dxpl_mpio(hid_t dxpl_id, H5FD_mpio_xfer_t *xfer_mode /*out*/)
{
    H5P_genplist_t *plist;               /* Property list pointer */
    herr_t          ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "ix", dxpl_id, xfer_mode);

    /* Check arguments */
    if (NULL == (plist = H5P_object_verify(dxpl_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a dxpl")

    /* Get the transfer mode */
    if (xfer_mode)
        if (H5P_get(plist, H5D_XFER_IO_XFER_MODE_NAME, xfer_mode) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to get value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_dxpl_mpio() */

/*-------------------------------------------------------------------------
 * Function:    H5Pset_dxpl_mpio_collective_opt
 *
 * Purpose:     To set a flag to choose linked chunk I/O or multi-chunk I/O
 *              without involving decision-making inside HDF5
 *
 * Note:        The library will do linked chunk I/O or multi-chunk I/O without
 *              involving communications for decision-making process.
 *              The library won't behave as it asks for only when we find
 *              that the low-level MPI-IO package doesn't support this.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 * Programmer:  Kent Yang
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_dxpl_mpio_collective_opt(hid_t dxpl_id, H5FD_mpio_collective_opt_t opt_mode)
{
    H5P_genplist_t *plist;               /* Property list pointer */
    herr_t          ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "iDc", dxpl_id, opt_mode);

    /* Check arguments */
    if (dxpl_id == H5P_DEFAULT)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "can't set values in default property list")
    if (NULL == (plist = H5P_object_verify(dxpl_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a dxpl")

    /* Set the transfer mode */
    if (H5P_set(plist, H5D_XFER_MPIO_COLLECTIVE_OPT_NAME, &opt_mode) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_dxpl_mpio_collective_opt() */

/*-------------------------------------------------------------------------
 * Function:    H5Pset_dxpl_mpio_chunk_opt
 *
 * Purpose:     To set a flag to choose linked chunk I/O or multi-chunk I/O
 *              without involving decision-making inside HDF5
 *
 * Note:        The library will do linked chunk I/O or multi-chunk I/O without
 *              involving communications for decision-making process.
 *              The library won't behave as it asks for only when we find
 *              that the low-level MPI-IO package doesn't support this.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 * Programmer:  Kent Yang
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_dxpl_mpio_chunk_opt(hid_t dxpl_id, H5FD_mpio_chunk_opt_t opt_mode)
{
    H5P_genplist_t *plist;               /* Property list pointer */
    herr_t          ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "iDh", dxpl_id, opt_mode);

    /* Check arguments */
    if (dxpl_id == H5P_DEFAULT)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "can't set values in default property list")
    if (NULL == (plist = H5P_object_verify(dxpl_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a dxpl")

    /* Set the transfer mode */
    if (H5P_set(plist, H5D_XFER_MPIO_CHUNK_OPT_HARD_NAME, &opt_mode) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_dxpl_mpio_chunk_opt() */

/*-------------------------------------------------------------------------
 * Function:    H5Pset_dxpl_mpio_chunk_opt_num
 *
 * Purpose:     To set a threshold for doing linked chunk IO
 *
 * Note:        If the number is greater than the threshold set by the user,
 *              the library will do linked chunk I/O; otherwise, I/O will be
 *              done for every chunk.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 * Programmer:  Kent Yang
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_dxpl_mpio_chunk_opt_num(hid_t dxpl_id, unsigned num_chunk_per_proc)
{
    H5P_genplist_t *plist;               /* Property list pointer */
    herr_t          ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "iIu", dxpl_id, num_chunk_per_proc);

    /* Check arguments */
    if (dxpl_id == H5P_DEFAULT)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "can't set values in default property list")
    if (NULL == (plist = H5P_object_verify(dxpl_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a dxpl")

    /* Set the transfer mode */
    if (H5P_set(plist, H5D_XFER_MPIO_CHUNK_OPT_NUM_NAME, &num_chunk_per_proc) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_dxpl_mpio_chunk_opt_num() */

/*-------------------------------------------------------------------------
 * Function:    H5Pset_dxpl_mpio_chunk_opt_ratio
 *
 * Purpose:     To set a threshold for doing collective I/O for each chunk
 *
 * Note:        The library will calculate the percentage of the number of
 *              process holding selections at each chunk. If that percentage
 *              of number of process in the individual chunk is greater than
 *              the threshold set by the user, the library will do collective
 *              chunk I/O for this chunk; otherwise, independent I/O will be
 *              done for this chunk.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 * Programmer:  Kent Yang
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_dxpl_mpio_chunk_opt_ratio(hid_t dxpl_id, unsigned percent_num_proc_per_chunk)
{
    H5P_genplist_t *plist;               /* Property list pointer */
    herr_t          ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "iIu", dxpl_id, percent_num_proc_per_chunk);

    /* Check arguments */
    if (dxpl_id == H5P_DEFAULT)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "can't set values in default property list")
    if (NULL == (plist = H5P_object_verify(dxpl_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a dxpl")

    /* Set the transfer mode */
    if (H5P_set(plist, H5D_XFER_MPIO_CHUNK_OPT_RATIO_NAME, &percent_num_proc_per_chunk) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_dxpl_mpio_chunk_opt_ratio() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_set_mpio_atomicity
 *
 * Purpose:     Sets the atomicity mode
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Mohamad Chaarawi
 *              Feb 14, 2012
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_set_mpio_atomicity(H5FD_t *_file, hbool_t flag)
{
    H5FD_mpio_t *file = (H5FD_mpio_t *)_file;
#ifdef H5FDmpio_DEBUG
    hbool_t H5FD_mpio_debug_t_flag = (H5FD_mpio_debug_flags_s[(int)'t'] && H5FD_MPIO_TRACE_THIS_RANK(file));
#endif
    int    mpi_code; /* MPI return code */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_debug_t_flag)
        HDfprintf(stderr, "%s: (%d) Entering\n", FUNC, file->mpi_rank);
#endif

    /* set atomicity value */
    if (MPI_SUCCESS != (mpi_code = MPI_File_set_atomicity(file->f, (int)(flag != FALSE))))
        HMPI_GOTO_ERROR(FAIL, "MPI_File_set_atomicity", mpi_code)

done:
#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_debug_t_flag)
        HDfprintf(stderr, "%s: (%d) Leaving\n", FUNC, file->mpi_rank);
#endif

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_set_mpio_atomicity() */

/*-------------------------------------------------------------------------
 * Function:    H5FD_get_mpio_atomicity
 *
 * Purpose:     Returns the atomicity mode
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Mohamad Chaarawi
 *              Feb 14, 2012
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD_get_mpio_atomicity(H5FD_t *_file, hbool_t *flag)
{
    H5FD_mpio_t *file = (H5FD_mpio_t *)_file;
    int          temp_flag;
#ifdef H5FDmpio_DEBUG
    hbool_t H5FD_mpio_debug_t_flag = (H5FD_mpio_debug_flags_s[(int)'t'] && H5FD_MPIO_TRACE_THIS_RANK(file));
#endif
    int    mpi_code; /* MPI return code */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_debug_t_flag)
        HDfprintf(stderr, "%s: (%d) Entering\n", FUNC, file->mpi_rank);
#endif

    /* Get atomicity value */
    if (MPI_SUCCESS != (mpi_code = MPI_File_get_atomicity(file->f, &temp_flag)))
        HMPI_GOTO_ERROR(FAIL, "MPI_File_get_atomicity", mpi_code)

    if (0 != temp_flag)
        *flag = TRUE;
    else
        *flag = FALSE;

done:
#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_debug_t_flag)
        HDfprintf(stderr, "%s: (%d) Leaving\n", FUNC, file->mpi_rank);
#endif

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_get_mpio_atomicity() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__mpio_open
 *
 * Purpose:     Opens a file with name NAME.  The FLAGS are a bit field with
 *              purpose similar to the second argument of open(2) and which
 *              are defined in H5Fpublic.h. The file access property list
 *              FAPL_ID contains the properties driver properties and MAXADDR
 *              is the largest address which this file will be expected to
 *              access.  This is collective.
 *
 * Return:      Success:    A new file pointer
 *              Failure:    NULL
 *
 * Programmer:  Robert Kim Yates
 *              January 30, 1998
 *
 *-------------------------------------------------------------------------
 */
static H5FD_t *
H5FD__mpio_open(const char *name, unsigned flags, hid_t fapl_id, haddr_t H5_ATTR_UNUSED maxaddr)
{
    H5FD_mpio_t *   file = NULL;          /* VFD File struct for new file */
    H5P_genplist_t *plist;                /* Property list pointer */
    MPI_Comm        comm = MPI_COMM_NULL; /* MPI Communicator, from plist */
    MPI_Info        info = MPI_INFO_NULL; /* MPI Info, from plist */
    MPI_File        fh;                   /* MPI file handle */
    hbool_t         file_opened = FALSE;  /* Flag to indicate that the file was successfully opened */
    int             mpi_amode;            /* MPI file access flags */
    int             mpi_rank = INT_MAX;   /* MPI rank of this process */
    int             mpi_size;             /* Total number of MPI processes */
    MPI_Offset      file_size;            /* File size (of existing files) */
#ifdef H5FDmpio_DEBUG
    hbool_t H5FD_mpio_debug_t_flag = FALSE;
#endif
    int     mpi_code;         /* MPI return code */
    H5FD_t *ret_value = NULL; /* Return value */

    FUNC_ENTER_STATIC

    /* Get a pointer to the fapl */
    if (NULL == (plist = H5P_object_verify(fapl_id, H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access property list")

    /* Get the MPI communicator and info object from the property list */
    if (H5P_get(plist, H5F_ACS_MPI_PARAMS_COMM_NAME, &comm) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, NULL, "can't get MPI communicator")
    if (H5P_get(plist, H5F_ACS_MPI_PARAMS_INFO_NAME, &info) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, NULL, "can't get MPI info object")

    /* Get the MPI rank of this process and the total number of processes */
    if (MPI_SUCCESS != (mpi_code = MPI_Comm_rank(comm, &mpi_rank)))
        HMPI_GOTO_ERROR(NULL, "MPI_Comm_rank failed", mpi_code)
    if (MPI_SUCCESS != (mpi_code = MPI_Comm_size(comm, &mpi_size)))
        HMPI_GOTO_ERROR(NULL, "MPI_Comm_size failed", mpi_code)

#ifdef H5FDmpio_DEBUG
    H5FD_mpio_debug_t_flag = (H5FD_mpio_debug_flags_s[(int)'t'] &&
                              (H5FD_mpio_debug_rank_s < 0 || H5FD_mpio_debug_rank_s == mpi_rank));
    if (H5FD_mpio_debug_t_flag)
        HDfprintf(stderr, "%s: (%d) Entering - name = \"%s\", flags = 0x%x, fapl_id = %d, maxaddr = %lu\n",
                  FUNC, mpi_rank, name, flags, (int)fapl_id, (unsigned long)maxaddr);
#endif

    /* Convert HDF5 flags to MPI-IO flags */
    /* Some combinations are illegal; let MPI-IO figure it out */
    mpi_amode = (flags & H5F_ACC_RDWR) ? MPI_MODE_RDWR : MPI_MODE_RDONLY;
    if (flags & H5F_ACC_CREAT)
        mpi_amode |= MPI_MODE_CREATE;
    if (flags & H5F_ACC_EXCL)
        mpi_amode |= MPI_MODE_EXCL;

#ifdef H5FDmpio_DEBUG
    /* Check for debug commands in the info parameter */
    if (MPI_INFO_NULL != info) {
        char debug_str[128];
        int  flag;

        MPI_Info_get(info, H5F_MPIO_DEBUG_KEY, sizeof(debug_str) - 1, debug_str, &flag);
        if (flag)
            H5FD__mpio_parse_debug_str(debug_str);
    } /* end if */
#endif

    if (MPI_SUCCESS != (mpi_code = MPI_File_open(comm, name, mpi_amode, info, &fh)))
        HMPI_GOTO_ERROR(NULL, "MPI_File_open failed", mpi_code)
    file_opened = TRUE;

    /* Build the return value and initialize it */
    if (NULL == (file = (H5FD_mpio_t *)H5MM_calloc(sizeof(H5FD_mpio_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
    file->f        = fh;
    file->comm     = comm;
    file->info     = info;
    file->mpi_rank = mpi_rank;
    file->mpi_size = mpi_size;

    /* Only processor p0 will get the filesize and broadcast it. */
    if (mpi_rank == 0)
        if (MPI_SUCCESS != (mpi_code = MPI_File_get_size(fh, &file_size)))
            HMPI_GOTO_ERROR(NULL, "MPI_File_get_size failed", mpi_code)

    /* Broadcast file size */
    if (MPI_SUCCESS != (mpi_code = MPI_Bcast(&file_size, (int)sizeof(MPI_Offset), MPI_BYTE, 0, comm)))
        HMPI_GOTO_ERROR(NULL, "MPI_Bcast failed", mpi_code)

    /* Determine if the file should be truncated */
    if (file_size && (flags & H5F_ACC_TRUNC)) {
        /* Truncate the file */
        if (MPI_SUCCESS != (mpi_code = MPI_File_set_size(fh, (MPI_Offset)0)))
            HMPI_GOTO_ERROR(NULL, "MPI_File_set_size failed", mpi_code)

        /* Don't let any proc return until all have truncated the file. */
        if (MPI_SUCCESS != (mpi_code = MPI_Barrier(comm)))
            HMPI_GOTO_ERROR(NULL, "MPI_Barrier failed", mpi_code)

        /* File is zero size now */
        file_size = 0;
    } /* end if */

    /* Set the size of the file (from library's perspective) */
    file->eof       = H5FD_mpi_MPIOff_to_haddr(file_size);
    file->local_eof = file->eof;

    /* Set return value */
    ret_value = (H5FD_t *)file;

done:
    if (ret_value == NULL) {
        if (file_opened)
            MPI_File_close(&fh);
        if (H5_mpi_comm_free(&comm) < 0)
            HDONE_ERROR(H5E_VFL, H5E_CANTFREE, NULL, "unable to free MPI communicator")
        if (H5_mpi_info_free(&info) < 0)
            HDONE_ERROR(H5E_VFL, H5E_CANTFREE, NULL, "unable to free MPI info object")
        if (file)
            H5MM_xfree(file);
    } /* end if */

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_debug_t_flag)
        HDfprintf(stderr, "%s: (%d) Leaving\n", FUNC, mpi_rank);
#endif

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__mpio_open() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__mpio_close
 *
 * Purpose:     Closes a file.  This is collective.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Unknown
 *              January 30, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__mpio_close(H5FD_t *_file)
{
    H5FD_mpio_t *file = (H5FD_mpio_t *)_file;
#ifdef H5FDmpio_DEBUG
    hbool_t H5FD_mpio_debug_t_flag = (H5FD_mpio_debug_flags_s[(int)'t'] && H5FD_MPIO_TRACE_THIS_RANK(file));
    int     mpi_rank               = file->mpi_rank;
#endif
    int    mpi_code;            /* MPI return code */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_debug_t_flag)
        HDfprintf(stderr, "%s: (%d) Entering\n", FUNC, file->mpi_rank);
#endif

    /* Sanity checks */
    HDassert(file);
    HDassert(H5FD_MPIO == file->pub.driver_id);

    /* MPI_File_close sets argument to MPI_FILE_NULL */
    if (MPI_SUCCESS != (mpi_code = MPI_File_close(&(file->f))))
        HMPI_GOTO_ERROR(FAIL, "MPI_File_close failed", mpi_code)

    /* Clean up other stuff */
    H5_mpi_comm_free(&file->comm);
    H5_mpi_info_free(&file->info);
    H5MM_xfree(file);

done:
#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_debug_t_flag)
        HDfprintf(stderr, "%s: (%d) Leaving\n", FUNC, mpi_rank);
#endif

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__mpio_close() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__mpio_query
 *
 * Purpose:     Set the flags that this VFL driver is capable of supporting.
 *              (listed in H5FDpublic.h)
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Quincey Koziol
 *              Friday, August 25, 2000
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__mpio_query(const H5FD_t H5_ATTR_UNUSED *_file, unsigned long *flags /* out */)
{
    FUNC_ENTER_STATIC_NOERR

    /* Set the VFL feature flags that this driver supports */
    if (flags) {
        *flags = 0;
        *flags |= H5FD_FEAT_AGGREGATE_METADATA;  /* OK to aggregate metadata allocations  */
        *flags |= H5FD_FEAT_AGGREGATE_SMALLDATA; /* OK to aggregate "small" raw data allocations */
        *flags |= H5FD_FEAT_HAS_MPI; /* This driver uses MPI                                             */
        *flags |= H5FD_FEAT_ALLOCATE_EARLY;         /* Allocate space early instead of late         */
        *flags |= H5FD_FEAT_DEFAULT_VFD_COMPATIBLE; /* VFD creates a file which can be opened with the default
                                                       VFD */
    }                                               /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5FD__mpio_query() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__mpio_get_eoa
 *
 * Purpose:     Gets the end-of-address marker for the file. The EOA marker
 *              is the first address past the last byte allocated in the
 *              format address space.
 *
 * Return:      Success:    The end-of-address marker
 *              Failure:    HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              Friday, August  6, 1999
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD__mpio_get_eoa(const H5FD_t *_file, H5FD_mem_t H5_ATTR_UNUSED type)
{
    const H5FD_mpio_t *file = (const H5FD_mpio_t *)_file;

    FUNC_ENTER_STATIC_NOERR

    /* Sanity checks */
    HDassert(file);
    HDassert(H5FD_MPIO == file->pub.driver_id);

    FUNC_LEAVE_NOAPI(file->eoa)
} /* end H5FD__mpio_get_eoa() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__mpio_set_eoa
 *
 * Purpose:     Set the end-of-address marker for the file. This function is
 *              called shortly after an existing HDF5 file is opened in order
 *              to tell the driver where the end of the HDF5 data is located.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Robb Matzke
 *              Friday, August 6, 1999
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__mpio_set_eoa(H5FD_t *_file, H5FD_mem_t H5_ATTR_UNUSED type, haddr_t addr)
{
    H5FD_mpio_t *file = (H5FD_mpio_t *)_file;

    FUNC_ENTER_STATIC_NOERR

    /* Sanity checks */
    HDassert(file);
    HDassert(H5FD_MPIO == file->pub.driver_id);

    file->eoa = addr;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5FD__mpio_set_eoa() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__mpio_get_eof
 *
 * Purpose:     Gets the end-of-file marker for the file. The EOF marker
 *              is the real size of the file.
 *
 *              The MPIO driver doesn't bother keeping this field updated
 *              since that's a relatively expensive operation. Fortunately
 *              the library only needs the EOF just after the file is opened
 *              in order to determine whether the file is empty, truncated,
 *              or okay.  Therefore, any MPIO I/O function will set its value
 *              to HADDR_UNDEF which is the error return value of this
 *              function.
 *
 *              Keeping the EOF updated (during write calls) is expensive
 *              because any process may extend the physical end of the
 *              file. -QAK
 *
 * Return:      Success:    The end-of-file marker
 *              Failure:    HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              Friday, August  6, 1999
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD__mpio_get_eof(const H5FD_t *_file, H5FD_mem_t H5_ATTR_UNUSED type)
{
    const H5FD_mpio_t *file = (const H5FD_mpio_t *)_file;

    FUNC_ENTER_STATIC_NOERR

    /* Sanity checks */
    HDassert(file);
    HDassert(H5FD_MPIO == file->pub.driver_id);

    FUNC_LEAVE_NOAPI(file->eof)
} /* end H5FD__mpio_get_eof() */

/*-------------------------------------------------------------------------
 * Function:       H5FD__mpio_get_handle
 *
 * Purpose:        Returns the file handle of MPIO file driver.
 *
 * Returns:        SUCCEED/FAIL
 *
 * Programmer:     Raymond Lu
 *                 Sept. 16, 2002
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__mpio_get_handle(H5FD_t *_file, hid_t H5_ATTR_UNUSED fapl, void **file_handle)
{
    H5FD_mpio_t *file      = (H5FD_mpio_t *)_file;
    herr_t       ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    if (!file_handle)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file handle not valid")

    *file_handle = &(file->f);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__mpio_get_handle() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__mpio_read
 *
 * Purpose:     Reads SIZE bytes of data from FILE beginning at address ADDR
 *              into buffer BUF according to data transfer properties in
 *              DXPL_ID using potentially complex file and buffer types to
 *              effect the transfer.
 *
 *              Reading past the end of the MPI file returns zeros instead of
 *              failing.  MPI is able to coalesce requests from different
 *              processes (collective or independent).
 *
 * Return:      Success:    SUCCEED. Result is stored in caller-supplied
 *                          buffer BUF.
 *
 *              Failure:    FAIL. Contents of buffer BUF are undefined.
 *
 * Programmer:  rky, 1998-01-30
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__mpio_read(H5FD_t *_file, H5FD_mem_t H5_ATTR_UNUSED type, hid_t H5_ATTR_UNUSED dxpl_id, haddr_t addr,
                size_t size, void *buf /*out*/)
{
    H5FD_mpio_t *file = (H5FD_mpio_t *)_file;
    MPI_Offset   mpi_off;
    MPI_Status   mpi_stat;            /* Status from I/O operation */
    MPI_Datatype buf_type = MPI_BYTE; /* MPI description of the selection in memory */
    int          size_i;              /* Integer copy of 'size' to read */
#if MPI_VERSION >= 3
    MPI_Count bytes_read = 0; /* Number of bytes read in */
    MPI_Count type_size;      /* MPI datatype used for I/O's size */
    MPI_Count io_size;        /* Actual number of bytes requested */
    MPI_Count n;
#else
    int bytes_read = 0; /* Number of bytes read in */
    int type_size;      /* MPI datatype used for I/O's size */
    int io_size;        /* Actual number of bytes requested */
    int n;
#endif
    hbool_t use_view_this_time = FALSE;
    hbool_t rank0_bcast        = FALSE; /* If read-with-rank0-and-bcast flag was used */
#ifdef H5FDmpio_DEBUG
    hbool_t H5FD_mpio_debug_t_flag = (H5FD_mpio_debug_flags_s[(int)'t'] && H5FD_MPIO_TRACE_THIS_RANK(file));
    hbool_t H5FD_mpio_debug_r_flag = (H5FD_mpio_debug_flags_s[(int)'r'] && H5FD_MPIO_TRACE_THIS_RANK(file));
#endif
    int    mpi_code; /* MPI return code */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_STATIC

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_debug_t_flag)
        HDfprintf(stderr, "%s: (%d) Entering\n", FUNC, file->mpi_rank);
#endif

    /* Sanity checks */
    HDassert(file);
    HDassert(H5FD_MPIO == file->pub.driver_id);
    HDassert(buf);

    /* Portably initialize MPI status variable */
    HDmemset(&mpi_stat, 0, sizeof(MPI_Status));

    /* some numeric conversions */
    if (H5FD_mpi_haddr_to_MPIOff(addr, &mpi_off /*out*/) < 0)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADRANGE, FAIL, "can't convert from haddr to MPI off")
    size_i = (int)size;
    if ((hsize_t)size_i != size)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADRANGE, FAIL, "can't convert from size to size_i")

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_debug_r_flag)
        HDfprintf(stderr, "%s: (%d) mpi_off = %ld  size_i = %d\n", FUNC, file->mpi_rank, (long)mpi_off,
                  size_i);
#endif

    /* Only look for MPI views for raw data transfers */
    if (type == H5FD_MEM_DRAW) {
        H5FD_mpio_xfer_t xfer_mode; /* I/O transfer mode */

        /* Get the transfer mode from the API context */
        if (H5CX_get_io_xfer_mode(&xfer_mode) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "can't get MPI-I/O transfer mode")

        /*
         * Set up for a fancy xfer using complex types, or single byte block. We
         * wouldn't need to rely on the use_view field if MPI semantics allowed
         * us to test that btype=ftype=MPI_BYTE (or even MPI_TYPE_NULL, which
         * could mean "use MPI_BYTE" by convention).
         */
        if (xfer_mode == H5FD_MPIO_COLLECTIVE) {
            MPI_Datatype file_type;

            /* Remember that views are used */
            use_view_this_time = TRUE;

            /* Prepare for a full-blown xfer using btype, ftype, and disp */
            if (H5CX_get_mpi_coll_datatypes(&buf_type, &file_type) < 0)
                HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "can't get MPI-I/O datatypes")

            /*
             * Set the file view when we are using MPI derived types
             */
            if (MPI_SUCCESS != (mpi_code = MPI_File_set_view(file->f, mpi_off, MPI_BYTE, file_type,
                                                             H5FD_mpi_native_g, file->info)))
                HMPI_GOTO_ERROR(FAIL, "MPI_File_set_view failed", mpi_code)

            /* When using types, use the address as the displacement for
             * MPI_File_set_view and reset the address for the read to zero
             */
            mpi_off = 0;
        } /* end if */
    }     /* end if */

    /* Read the data. */
    if (use_view_this_time) {
        H5FD_mpio_collective_opt_t coll_opt_mode;

#ifdef H5FDmpio_DEBUG
        if (H5FD_mpio_debug_r_flag)
            HDfprintf(stderr, "%s: (%d) using MPIO collective mode\n", FUNC, file->mpi_rank);
#endif
        /* Get the collective_opt property to check whether the application wants to do IO individually. */
        if (H5CX_get_mpio_coll_opt(&coll_opt_mode) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "can't get MPI-I/O collective_op property")

        if (coll_opt_mode == H5FD_MPIO_COLLECTIVE_IO) {
#ifdef H5FDmpio_DEBUG
            if (H5FD_mpio_debug_r_flag)
                HDfprintf(stderr, "%s: (%d) doing MPI collective IO\n", FUNC, file->mpi_rank);
#endif
            /* Check whether we should read from rank 0 and broadcast to other ranks */
            if (H5CX_get_mpio_rank0_bcast()) {
#ifdef H5FDmpio_DEBUG
                if (H5FD_mpio_debug_r_flag)
                    HDfprintf(stderr, "%s: (%d) doing read-rank0-and-MPI_Bcast\n", FUNC, file->mpi_rank);
#endif
                /* Indicate path we've taken */
                rank0_bcast = TRUE;

                /* Read on rank 0 Bcast to other ranks */
                if (file->mpi_rank == 0)
                    if (MPI_SUCCESS !=
                        (mpi_code = MPI_File_read_at(file->f, mpi_off, buf, size_i, buf_type, &mpi_stat)))
                        HMPI_GOTO_ERROR(FAIL, "MPI_File_read_at failed", mpi_code)
                if (MPI_SUCCESS != (mpi_code = MPI_Bcast(buf, size_i, buf_type, 0, file->comm)))
                    HMPI_GOTO_ERROR(FAIL, "MPI_Bcast failed", mpi_code)
            } /* end if */
            else
                /* Perform collective read operation */
                if (MPI_SUCCESS !=
                    (mpi_code = MPI_File_read_at_all(file->f, mpi_off, buf, size_i, buf_type, &mpi_stat)))
                HMPI_GOTO_ERROR(FAIL, "MPI_File_read_at_all failed", mpi_code)
        } /* end if */
        else {
#ifdef H5FDmpio_DEBUG
            if (H5FD_mpio_debug_r_flag)
                HDfprintf(stderr, "%s: (%d) doing MPI independent IO\n", FUNC, file->mpi_rank);
#endif

            /* Perform independent read operation */
            if (MPI_SUCCESS !=
                (mpi_code = MPI_File_read_at(file->f, mpi_off, buf, size_i, buf_type, &mpi_stat)))
                HMPI_GOTO_ERROR(FAIL, "MPI_File_read_at failed", mpi_code)
        } /* end else */

        /*
         * Reset the file view when we used MPI derived types
         */
        if (MPI_SUCCESS != (mpi_code = MPI_File_set_view(file->f, (MPI_Offset)0, MPI_BYTE, MPI_BYTE,
                                                         H5FD_mpi_native_g, file->info)))
            HMPI_GOTO_ERROR(FAIL, "MPI_File_set_view failed", mpi_code)
    } /* end if */
    else {
#ifdef H5FDmpio_DEBUG
        if (H5FD_mpio_debug_r_flag)
            HDfprintf(stderr, "%s: (%d) doing MPI independent IO\n", FUNC, file->mpi_rank);
#endif

        /* Perform independent read operation */
        if (MPI_SUCCESS != (mpi_code = MPI_File_read_at(file->f, mpi_off, buf, size_i, buf_type, &mpi_stat)))
            HMPI_GOTO_ERROR(FAIL, "MPI_File_read_at failed", mpi_code)
    } /* end else */

    /* Only retrieve bytes read if this rank _actually_ participated in I/O */
    if (!rank0_bcast || (rank0_bcast && file->mpi_rank == 0)) {
        /* How many bytes were actually read? */
#if MPI_VERSION >= 3
        if (MPI_SUCCESS != (mpi_code = MPI_Get_elements_x(&mpi_stat, buf_type, &bytes_read)))
#else
        if (MPI_SUCCESS != (mpi_code = MPI_Get_elements(&mpi_stat, MPI_BYTE, &bytes_read)))
#endif
            HMPI_GOTO_ERROR(FAIL, "MPI_Get_elements failed", mpi_code)
    } /* end if */

    /* If the rank0-bcast feature was used, broadcast the # of bytes read to
     * other ranks, which didn't perform any I/O.
     */
    /* NOTE: This could be optimized further to be combined with the broadcast
     *          of the data.  (QAK - 2019/1/2)
     */
    if (rank0_bcast)
        if (MPI_SUCCESS != MPI_Bcast(&bytes_read, 1, MPI_LONG_LONG, 0, file->comm))
            HMPI_GOTO_ERROR(FAIL, "MPI_Bcast failed", 0)

            /* Get the type's size */
#if MPI_VERSION >= 3
    if (MPI_SUCCESS != (mpi_code = MPI_Type_size_x(buf_type, &type_size)))
#else
    if (MPI_SUCCESS != (mpi_code = MPI_Type_size(buf_type, &type_size)))
#endif
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_size failed", mpi_code)

    /* Compute the actual number of bytes requested */
    io_size = type_size * size_i;

    /* Check for read failure */
    if (bytes_read < 0 || bytes_read > io_size)
        HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "file read failed")

    /*
     * This gives us zeroes beyond end of physical MPI file.
     */
    if ((n = (io_size - bytes_read)) > 0)
        HDmemset((char *)buf + bytes_read, 0, (size_t)n);

done:
#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_debug_t_flag)
        HDfprintf(stderr, "%s: (%d) Leaving\n", FUNC, file->mpi_rank);
#endif

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__mpio_read() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__mpio_write
 *
 * Purpose:     Writes SIZE bytes of data to FILE beginning at address ADDR
 *              from buffer BUF according to data transfer properties in
 *              DXPL_ID using potentially complex file and buffer types to
 *              effect the transfer.
 *
 *              MPI is able to coalesce requests from different processes
 *              (collective and independent).
 *
 * Return:      Success:    SUCCEED. USE_TYPES and OLD_USE_TYPES in the
 *                          access params are altered.
 *              Failure:    FAIL. USE_TYPES and OLD_USE_TYPES in the
 *                          access params may be altered.
 *
 * Programmer:  Robert Kim Yates
 *              January 30, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__mpio_write(H5FD_t *_file, H5FD_mem_t type, hid_t H5_ATTR_UNUSED dxpl_id, haddr_t addr, size_t size,
                 const void *buf)
{
    H5FD_mpio_t *file = (H5FD_mpio_t *)_file;
    MPI_Offset   mpi_off;
    MPI_Status   mpi_stat;            /* Status from I/O operation */
    MPI_Datatype buf_type = MPI_BYTE; /* MPI description of the selection in memory */
#if MPI_VERSION >= 3
    MPI_Count bytes_written;
    MPI_Count type_size; /* MPI datatype used for I/O's size */
    MPI_Count io_size;   /* Actual number of bytes requested */
#else
    int bytes_written;
    int type_size; /* MPI datatype used for I/O's size */
    int io_size;   /* Actual number of bytes requested */
#endif
    int              size_i;
    hbool_t          use_view_this_time = FALSE;
    hbool_t          derived_type       = FALSE;
    H5FD_mpio_xfer_t xfer_mode; /* I/O transfer mode */
#ifdef H5FDmpio_DEBUG
    hbool_t H5FD_mpio_debug_t_flag = (H5FD_mpio_debug_flags_s[(int)'t'] && H5FD_MPIO_TRACE_THIS_RANK(file));
    hbool_t H5FD_mpio_debug_w_flag = (H5FD_mpio_debug_flags_s[(int)'w'] && H5FD_MPIO_TRACE_THIS_RANK(file));
#endif
    int    mpi_code; /* MPI return code */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_STATIC

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_debug_t_flag)
        HDfprintf(stderr, "%s: (%d) Entering\n", FUNC, file->mpi_rank);
#endif

    /* Sanity checks */
    HDassert(file);
    HDassert(H5FD_MPIO == file->pub.driver_id);
    HDassert(buf);

    /* Verify that no data is written when between MPI_Barrier()s during file flush */
    HDassert(!H5CX_get_mpi_file_flushing());

    /* Portably initialize MPI status variable */
    HDmemset(&mpi_stat, 0, sizeof(MPI_Status));

    /* some numeric conversions */
    if (H5FD_mpi_haddr_to_MPIOff(addr, &mpi_off) < 0)
        HGOTO_ERROR(H5E_INTERNAL, H5E_BADRANGE, FAIL, "can't convert from haddr to MPI off")
    size_i = (int)size;

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_debug_w_flag)
        HDfprintf(stderr, "%s: (%d) mpi_off = %ld  size_i = %d\n", FUNC, file->mpi_rank, (long)mpi_off,
                  size_i);
#endif

    /* Get the transfer mode from the API context */
    if (H5CX_get_io_xfer_mode(&xfer_mode) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "can't get MPI-I/O transfer mode")

    /*
     * Set up for a fancy xfer using complex types, or single byte block. We
     * wouldn't need to rely on the use_view field if MPI semantics allowed
     * us to test that btype=ftype=MPI_BYTE (or even MPI_TYPE_NULL, which
     * could mean "use MPI_BYTE" by convention).
     */
    if (xfer_mode == H5FD_MPIO_COLLECTIVE) {
        MPI_Datatype file_type;

        /* Remember that views are used */
        use_view_this_time = TRUE;

        /* Prepare for a full-blown xfer using btype, ftype, and disp */
        if (H5CX_get_mpi_coll_datatypes(&buf_type, &file_type) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "can't get MPI-I/O datatypes")

        /*
         * Set the file view when we are using MPI derived types
         */
        if (MPI_SUCCESS != (mpi_code = MPI_File_set_view(file->f, mpi_off, MPI_BYTE, file_type,
                                                         H5FD_mpi_native_g, file->info)))
            HMPI_GOTO_ERROR(FAIL, "MPI_File_set_view failed", mpi_code)

        /* When using types, use the address as the displacement for
         * MPI_File_set_view and reset the address for the read to zero
         */
        mpi_off = 0;
    } /* end if */
    else if (size != (hsize_t)size_i) {
        /* If HERE, then we need to work around the integer size limit
         * of 2GB. The input size_t size variable cannot fit into an integer,
         * but we can get around that limitation by creating a different datatype
         * and then setting the integer size (or element count) to 1 when using
         * the derived_type.
         */

        if (H5_mpio_create_large_type(size, 0, MPI_BYTE, &buf_type) < 0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_CANTGET, FAIL, "can't create MPI-I/O datatype")

        derived_type = TRUE;
        size_i       = 1;
    }

    /* Write the data. */
    if (use_view_this_time) {
        H5FD_mpio_collective_opt_t coll_opt_mode;

#ifdef H5FDmpio_DEBUG
        if (H5FD_mpio_debug_w_flag)
            HDfprintf(stderr, "%s: (%d) using MPIO collective mode\n", FUNC, file->mpi_rank);
#endif

        /* Get the collective_opt property to check whether the application wants to do IO individually. */
        if (H5CX_get_mpio_coll_opt(&coll_opt_mode) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "can't get MPI-I/O collective_op property")

        if (coll_opt_mode == H5FD_MPIO_COLLECTIVE_IO) {
#ifdef H5FDmpio_DEBUG
            if (H5FD_mpio_debug_w_flag)
                HDfprintf(stderr, "%s: (%d) doing MPI collective IO\n", FUNC, file->mpi_rank);
#endif
            /* Perform collective write operation */
            if (MPI_SUCCESS !=
                (mpi_code = MPI_File_write_at_all(file->f, mpi_off, buf, size_i, buf_type, &mpi_stat)))
                HMPI_GOTO_ERROR(FAIL, "MPI_File_write_at_all failed", mpi_code)
        } /* end if */
        else {
            if (type != H5FD_MEM_DRAW)
                HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL,
                            "Metadata Coll opt property should be collective at this point")

#ifdef H5FDmpio_DEBUG
            if (H5FD_mpio_debug_w_flag)
                HDfprintf(stderr, "%s: (%d) doing MPI independent IO\n", FUNC, file->mpi_rank);
#endif
            /* Perform independent write operation */
            if (MPI_SUCCESS !=
                (mpi_code = MPI_File_write_at(file->f, mpi_off, buf, size_i, buf_type, &mpi_stat)))
                HMPI_GOTO_ERROR(FAIL, "MPI_File_write_at failed", mpi_code)
        } /* end else */

        /* Reset the file view when we used MPI derived types */
        if (MPI_SUCCESS != (mpi_code = MPI_File_set_view(file->f, (MPI_Offset)0, MPI_BYTE, MPI_BYTE,
                                                         H5FD_mpi_native_g, file->info)))
            HMPI_GOTO_ERROR(FAIL, "MPI_File_set_view failed", mpi_code)
    } /* end if */
    else {
#ifdef H5FDmpio_DEBUG
        if (H5FD_mpio_debug_w_flag)
            HDfprintf(stderr, "%s: (%d) doing MPI independent IO\n", FUNC, file->mpi_rank);
#endif

        /* Perform independent write operation */
        if (MPI_SUCCESS != (mpi_code = MPI_File_write_at(file->f, mpi_off, buf, size_i, buf_type, &mpi_stat)))
            HMPI_GOTO_ERROR(FAIL, "MPI_File_write_at failed", mpi_code)
    } /* end else */

    /* How many bytes were actually written? */
#if MPI_VERSION >= 3
    if (MPI_SUCCESS != (mpi_code = MPI_Get_elements_x(&mpi_stat, buf_type, &bytes_written)))
#else
    if (MPI_SUCCESS != (mpi_code = MPI_Get_elements(&mpi_stat, MPI_BYTE, &bytes_written)))
#endif
        HMPI_GOTO_ERROR(FAIL, "MPI_Get_elements failed", mpi_code)

        /* Get the type's size */
#if MPI_VERSION >= 3
    if (MPI_SUCCESS != (mpi_code = MPI_Type_size_x(buf_type, &type_size)))
#else
    if (MPI_SUCCESS != (mpi_code = MPI_Type_size(buf_type, &type_size)))
#endif
        HMPI_GOTO_ERROR(FAIL, "MPI_Type_size failed", mpi_code)

    /* Compute the actual number of bytes requested */
    io_size = type_size * size_i;

    /* Check for write failure */
    if (bytes_written != io_size || bytes_written < 0)
        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "file write failed")

    /* Each process will keep track of its perceived EOF value locally, and
     * ultimately we will reduce this value to the maximum amongst all
     * processes, but until then keep the actual eof at HADDR_UNDEF just in
     * case something bad happens before that point. (rather have a value
     * we know is wrong sitting around rather than one that could only
     * potentially be wrong.) */
    file->eof = HADDR_UNDEF;

    if (bytes_written && (((haddr_t)bytes_written + addr) > file->local_eof))
        file->local_eof = addr + (haddr_t)bytes_written;

done:
    if (derived_type)
        MPI_Type_free(&buf_type);

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_debug_t_flag)
        HDfprintf(stderr, "%s: (%d) Leaving: ret_value = %d\n", FUNC, file->mpi_rank, ret_value);
#endif

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__mpio_write() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__mpio_flush
 *
 * Purpose:     Makes sure that all data is on disk.  This is collective.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Robb Matzke
 *              January 30, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__mpio_flush(H5FD_t *_file, hid_t H5_ATTR_UNUSED dxpl_id, hbool_t closing)
{
    H5FD_mpio_t *file = (H5FD_mpio_t *)_file;
#ifdef H5FDmpio_DEBUG
    hbool_t H5FD_mpio_debug_t_flag = (H5FD_mpio_debug_flags_s[(int)'t'] && H5FD_MPIO_TRACE_THIS_RANK(file));
#endif
    int    mpi_code; /* mpi return code */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_STATIC

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_debug_t_flag)
        HDfprintf(stderr, "%s: (%d) Entering\n", FUNC, file->mpi_rank);
#endif

    /* Sanity checks */
    HDassert(file);
    HDassert(H5FD_MPIO == file->pub.driver_id);

    /* Only sync the file if we are not going to immediately close it */
    if (!closing)
        if (MPI_SUCCESS != (mpi_code = MPI_File_sync(file->f)))
            HMPI_GOTO_ERROR(FAIL, "MPI_File_sync failed", mpi_code)

done:
#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_debug_t_flag)
        HDfprintf(stderr, "%s: (%d) Leaving\n", FUNC, file->mpi_rank);
#endif

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__mpio_flush() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__mpio_truncate
 *
 * Purpose:     Make certain the file's size matches it's allocated size
 *
 *              This is a little sticky in the mpio case, as it is not
 *              easy for us to track the current EOF by extracting it from
 *              write calls, since other ranks could have written to the
 *              file beyond the local EOF.
 *
 *              Instead, we first check to see if the EOA has changed since
 *              the last call to this function.  If it has, we call
 *              MPI_File_get_size() to determine the current EOF, and
 *              only call MPI_File_set_size() if this value disagrees
 *              with the current EOA.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Quincey Koziol
 *              January 31, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__mpio_truncate(H5FD_t *_file, hid_t H5_ATTR_UNUSED dxpl_id, hbool_t H5_ATTR_UNUSED closing)
{
    H5FD_mpio_t *file = (H5FD_mpio_t *)_file;
#ifdef H5FDmpio_DEBUG
    hbool_t H5FD_mpio_debug_t_flag = (H5FD_mpio_debug_flags_s[(int)'t'] && H5FD_MPIO_TRACE_THIS_RANK(file));
#endif
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_STATIC

#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_debug_t_flag)
        HDfprintf(stderr, "%s: (%d) Entering\n", FUNC, file->mpi_rank);
#endif

    /* Sanity checks */
    HDassert(file);
    HDassert(H5FD_MPIO == file->pub.driver_id);

    if (!H5F_addr_eq(file->eoa, file->last_eoa)) {
        int        mpi_code; /* mpi return code */
        MPI_Offset size;
        MPI_Offset needed_eof;

        /* In principle, it is possible for the size returned by the
         * call to MPI_File_get_size() to depend on whether writes from
         * all proceeses have completed at the time process 0 makes the
         * call.
         *
         * In practice, most (all?) truncate calls will come after a barrier
         * and with no interviening writes to the file (with the possible
         * exception of sueprblock / superblock extension message updates).
         *
         * Check the "MPI file closing" flag in the API context to determine
         * if we can skip the barrier.
         */
        if (!H5CX_get_mpi_file_flushing())
            if (MPI_SUCCESS != (mpi_code = MPI_Barrier(file->comm)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Barrier failed", mpi_code)

        /* Only processor p0 will get the filesize and broadcast it. */
        /* (Note that throwing an error here will cause non-rank 0 processes
         *      to hang in following Bcast.  -QAK, 3/17/2018)
         */
        if (0 == file->mpi_rank)
            if (MPI_SUCCESS != (mpi_code = MPI_File_get_size(file->f, &size)))
                HMPI_GOTO_ERROR(FAIL, "MPI_File_get_size failed", mpi_code)

        /* Broadcast file size */
        if (MPI_SUCCESS != (mpi_code = MPI_Bcast(&size, (int)sizeof(MPI_Offset), MPI_BYTE, 0, file->comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Bcast failed", mpi_code)

        if (H5FD_mpi_haddr_to_MPIOff(file->eoa, &needed_eof) < 0)
            HGOTO_ERROR(H5E_INTERNAL, H5E_BADRANGE, FAIL, "cannot convert from haddr_t to MPI_Offset")

        /* EOA != EOF.  Set EOF to EOA */
        if (size != needed_eof) {
            /* Extend the file's size */
            if (MPI_SUCCESS != (mpi_code = MPI_File_set_size(file->f, needed_eof)))
                HMPI_GOTO_ERROR(FAIL, "MPI_File_set_size failed", mpi_code)

            /* In general, we must wait until all processes have finished
             * the truncate before any process can continue, since it is
             * possible that a process would write at the end of the
             * file, and this write would be discarded by the truncate.
             *
             * While this is an issue for a user initiated flush, it may
             * not be an issue at file close.  If so, we may be able to
             * optimize out the following barrier in that case.
             */
            if (MPI_SUCCESS != (mpi_code = MPI_Barrier(file->comm)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Barrier failed", mpi_code)
        } /* end if */

        /* Update the 'last' eoa value */
        file->last_eoa = file->eoa;
    } /* end if */

done:
#ifdef H5FDmpio_DEBUG
    if (H5FD_mpio_debug_t_flag)
        HDfprintf(stderr, "%s: (%d) Leaving\n", FUNC, file->mpi_rank);
#endif

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__mpio_truncate() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__mpio_mpi_rank
 *
 * Purpose:     Returns the MPI rank for a process
 *
 * Return:      Success: non-negative
 *              Failure: negative
 *
 * Programmer:  Quincey Koziol
 *              Thursday, May 16, 2002
 *
 *-------------------------------------------------------------------------
 */
static int
H5FD__mpio_mpi_rank(const H5FD_t *_file)
{
    const H5FD_mpio_t *file = (const H5FD_mpio_t *)_file;

    FUNC_ENTER_STATIC_NOERR

    /* Sanity checks */
    HDassert(file);
    HDassert(H5FD_MPIO == file->pub.driver_id);

    FUNC_LEAVE_NOAPI(file->mpi_rank)
} /* end H5FD__mpio_mpi_rank() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__mpio_mpi_size
 *
 * Purpose:     Returns the number of MPI processes
 *
 * Return:      Success: non-negative
 *              Failure: negative
 *
 * Programmer:  Quincey Koziol
 *              Thursday, May 16, 2002
 *
 *-------------------------------------------------------------------------
 */
static int
H5FD__mpio_mpi_size(const H5FD_t *_file)
{
    const H5FD_mpio_t *file = (const H5FD_mpio_t *)_file;

    FUNC_ENTER_STATIC_NOERR

    /* Sanity checks */
    HDassert(file);
    HDassert(H5FD_MPIO == file->pub.driver_id);

    FUNC_LEAVE_NOAPI(file->mpi_size)
} /* end H5FD__mpio_mpi_size() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__mpio_communicator
 *
 * Purpose:     Returns the MPI communicator for the file.
 *
 * Return:      Success:    The communicator
 *              Failure:    Can't fail
 *
 * Programmer:  Robb Matzke
 *              Monday, August  9, 1999
 *
 *-------------------------------------------------------------------------
 */
static MPI_Comm
H5FD__mpio_communicator(const H5FD_t *_file)
{
    const H5FD_mpio_t *file = (const H5FD_mpio_t *)_file;

    FUNC_ENTER_STATIC_NOERR

    /* Sanity checks */
    HDassert(file);
    HDassert(H5FD_MPIO == file->pub.driver_id);

    FUNC_LEAVE_NOAPI(file->comm)
} /* end H5FD__mpio_communicator() */
#endif /* H5_HAVE_PARALLEL */
