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

/*
 * Programmer:  Scott Wegner <swegner@hdfgroup.org>
 *				Based on code by Robb Matzke
 *				May 24, 2007
 *
 * Purpose:	We would like to create a driver specifically for Windows
 *			to utilize the Win32 API, and reduce the maintenence demands
 *			for the other file drivers.  Our other motivation is that
 *			the Windows system calls of the existing sec2 driver differ
 *			from those on other platforms, and are not 64-bit compatible.
 *			From the start, this will have the structure very similar
 *			to our sec2 driver, but make system calls more similar to
 *			our stdio driver.
 */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5FD_windows_init_interface


#include "H5private.h"		/* Generic Functions		*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"		/* File access				*/
#include "H5FDprivate.h"	/* File drivers				*/
#include "H5FDwindows.h"	/* Windows file driver		*/
#include "H5FLprivate.h"	/* Free Lists               */
#include "H5Iprivate.h"		/* IDs			  			*/
#include "H5MMprivate.h"	/* Memory management		*/
#include "H5Pprivate.h"		/* Property lists			*/

#ifdef H5_HAVE_WINDOWS

/* The driver identification number, initialized at runtime */
static hid_t H5FD_WINDOWS_g = 0;

/* File operations */
#define OP_UNKNOWN	0
#define OP_READ		1
#define OP_WRITE	2

/*
 * The description of a file belonging to this driver. The `eoa' and `eof'
 * determine the amount of hdf5 address space in use and the high-water mark
 * of the file (the current size of the underlying file). The `pos'
 * value is used to eliminate file position updates when they would be a
 * no-op. Unfortunately we've found systems that use separate file position
 * indicators for reading and writing so the lseek can only be eliminated if
 * the current operation is the same as the previous operation.  When opening
 * a file the `eof' will be set to the current file size, `eoa' will be set
 * to zero, `pos' will be set to H5F_ADDR_UNDEF (as it is when an error
 * occurs), and `op' will be set to H5F_OP_UNKNOWN.
 */
typedef struct H5FD_windows_t {
    H5FD_t	pub;			/*public stuff, must be first	*/
	/*
	 * .NET doesn't support our 64-bit safe stdio functions,
	 * so we will use io.h functions instead.
     */
#ifndef WINDOWS_USE_STDIO
	int		fd;				/*the file descriptor			*/
#else
    FILE 	*fp;			/*the file handle				*/
#endif /*MSC_VER */
    haddr_t	eoa;			/*end of allocated region		*/
    haddr_t	eof;			/*end of file; current file size*/
    haddr_t	pos;			/*current file I/O position		*/
    int		op;				/*last operation				*/
	unsigned write_access;	/*flag to indicate the file was opened with write access */
    /*
     * On _WIN32 the low-order word of a unique identifier associated with the
     * file and the volume serial number uniquely identify a file. This number
     * may change when the system is restarted or when the
     * file is opened. After a process opens a file, the identifier is
     * constant until the file is closed. An application can use this
     * identifier and the volume serial number to determine whether two
     * handles refer to the same file.
     */
    DWORD fileindexlo;
    DWORD fileindexhi;
    DWORD volumeserialnumber;

    /* Information from properties set by 'h5repart' tool */
    hbool_t     fam_to_sec2;    /* Whether to eliminate the family driver info
                                 * and convert this file to a single file */
} H5FD_windows_t;


/* These are used by the macros below */
#define file_offset_t		__int64
#define fseek_offset_t		__int64

/*
 * These macros check for overflow of various quantities.  These macros
 * assume that fseek_offset_t is signed and haddr_t and size_t are unsigned.
 *
 * ADDR_OVERFLOW:	Checks whether a file address of type `haddr_t'
 *			is too large to be represented by the second argument
 *			of the file seek function.
 *
 * SIZE_OVERFLOW:	Checks whether a buffer size of type `hsize_t' is too
 *			large to be represented by the `size_t' type.
 *
 * REGION_OVERFLOW:	Checks whether an address and size pair describe data
 *			which can be addressed entirely by the second
 *			argument of the file seek function.
 */
#define MAXADDR (((haddr_t)1<<(8*sizeof(file_offset_t)-1))-1)
#define ADDR_OVERFLOW(A)	(HADDR_UNDEF==(A) ||			      \
				 ((A) & ~(haddr_t)MAXADDR))
#define SIZE_OVERFLOW(Z)	((Z) & ~(hsize_t)MAXADDR)
#define REGION_OVERFLOW(A,Z)	(ADDR_OVERFLOW(A) || SIZE_OVERFLOW(Z) ||      \
                                 HADDR_UNDEF==(A)+(Z) ||		      \
				 (fseek_offset_t)((A)+(Z))<(fseek_offset_t)(A))

/* Prototypes */
static H5FD_t *H5FD_windows_open(const char *name, unsigned flags, hid_t fapl_id,
			      haddr_t maxaddr);
static herr_t H5FD_windows_close(H5FD_t *_file);
static int H5FD_windows_cmp(const H5FD_t *_f1, const H5FD_t *_f2);
static herr_t H5FD_windows_query(const H5FD_t *_f1, unsigned long *flags);
static haddr_t H5FD_windows_alloc(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id,
			      hsize_t size);
static haddr_t H5FD_windows_get_eoa(const H5FD_t *_file, H5FD_mem_t type);
static herr_t H5FD_windows_set_eoa(H5FD_t *_file, H5FD_mem_t type, haddr_t addr);
static haddr_t H5FD_windows_get_eof(const H5FD_t *_file);
static herr_t  H5FD_windows_get_handle(H5FD_t *_file, hid_t fapl, void** file_handle);
static herr_t H5FD_windows_read(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
			     size_t size, void *buf);
static herr_t H5FD_windows_write(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
			      size_t size, const void *buf);
static herr_t H5FD_windows_flush(H5FD_t *_file, hid_t dxpl_id, unsigned closing);
static herr_t H5FD_windows_truncate(H5FD_t *_file, hid_t dxpl_id, hbool_t closing);

static const H5FD_class_t H5FD_windows_g = {
    "windows",					/*name			*/
    MAXADDR,					/*maxaddr		*/
    H5F_CLOSE_WEAK,				/* fc_degree	*/
    NULL,					/*sb_size		*/
    NULL,					/*sb_encode		*/
    NULL,					/*sb_decode		*/
    0, 						/*fapl_size		*/
    NULL,					/*fapl_get		*/
    NULL,					/*fapl_copy		*/
    NULL, 					/*fapl_free		*/
    0,						/*dxpl_size		*/
    NULL,					/*dxpl_copy		*/
    NULL,					/*dxpl_free		*/
    H5FD_windows_open,				/*open			*/
    H5FD_windows_close,				/*close			*/
    H5FD_windows_cmp,				/*cmp			*/
    H5FD_windows_query,				/*query			*/
    NULL,					/*get_type_map		*/
    NULL,					/*alloc			*/
    NULL,					/*free			*/
    H5FD_windows_get_eoa,			/*get_eoa		*/
    H5FD_windows_set_eoa,			/*set_eoa		*/
    H5FD_windows_get_eof,			/*get_eof		*/
    H5FD_windows_get_handle,			/*get_handle    */
    H5FD_windows_read,				/*read			*/
    H5FD_windows_write,				/*write			*/
    H5FD_windows_flush,				/*flush			*/
    H5FD_windows_truncate,			/*truncate		*/
    NULL,					/*lock          */
    NULL,					/*unlock        */
    H5FD_FLMAP_SINGLE 				/*fl_map		*/
};

/* Declare a free list to manage the H5FD_windows_t struct */
H5FL_DEFINE_STATIC(H5FD_windows_t);

/*--------------------------------------------------------------------------
NAME
   H5FD_windows_init_interface -- Initialize interface-specific information
USAGE
    herr_t H5FD_windows_init_interface()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5FD_windows_init currently).

--------------------------------------------------------------------------*/
static herr_t
H5FD_windows_init_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5FD_windows_init_interface)

    FUNC_LEAVE_NOAPI(H5FD_windows_init())
} /* H5FD_windows_init_interface() */

/*-------------------------------------------------------------------------
 * Function:	H5FD_windows_init
 *
 * Purpose:	Initialize this driver by registering the driver with the
 *			library.
 *
 * Return:	Success:	The driver ID for the windows driver.
 *			Failure:	Negative.
 *
 * Programmer:	Scott Wegner
 *				Based on code by Robb Matzke
 *              Thursday, May 24, 2007
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FD_windows_init(void)
{
    hid_t ret_value;        /* Return value */

    FUNC_ENTER_NOAPI(H5FD_windows_init, FAIL)

    if(H5I_VFL != H5I_get_type(H5FD_WINDOWS_g))
        H5FD_WINDOWS_g = H5FD_register(&H5FD_windows_g, sizeof(H5FD_class_t), FALSE);

    /* Set return value */
    ret_value = H5FD_WINDOWS_g;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_windows_init() */

/*---------------------------------------------------------------------------
 * Function:	H5FD_windows_term
 *
 * Purpose:	Shut down the VFD
 *
 * Return:	<none>
 *
 * Programmer:  Scott Wegner
 *				Based on code by Quincey Koziol
 *              Thursday, May 24 2007
 *
 * Modification:
 *
 *---------------------------------------------------------------------------
 */
void
H5FD_windows_term(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5FD_windows_term)

    /* Reset VFL ID */
    H5FD_WINDOWS_g=0;

    FUNC_LEAVE_NOAPI_VOID
} /* end H5FD_windows_term() */

/*-------------------------------------------------------------------------
 * Function:	H5Pset_fapl_windows
 *
 * Purpose:	Modify the file access property list to use the H5FD_WINDOWS
 *		driver defined in this source file.  There are no driver
 *		specific properties.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Scott Wegner
 *				Based on code by Robb Matzke
 *				Thursday, May 24 2007
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fapl_windows(hid_t fapl_id)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value;

    FUNC_ENTER_API(H5Pset_fapl_windows, FAIL)
    H5TRACE1("e", "i", fapl_id);

    if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list")

    ret_value= H5P_set_driver(plist, H5FD_WINDOWS, NULL);

done:
    FUNC_LEAVE_API(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:	H5FD_windows_open
 *
 * Purpose:	Create and/or opens a Windows file as an HDF5 file.
 *
 * Return:	Success:	A pointer to a new file data structure. The
 *						public fields will be initialized by the
 *						caller, which is always H5FD_open().
 *
 *		Failure:	NULL
 *
 * Programmer:	Scott Wegner
 *				Based on code by Robb Matzke
 *              Thursday, May 24 2007
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static H5FD_t *
H5FD_windows_open(const char *name, unsigned flags, hid_t UNUSED fapl_id,
				  haddr_t maxaddr)
{
    int             fd = -1;
    FILE			*f = NULL;
    H5FD_windows_t	*file = NULL;
    HANDLE			filehandle;
    struct _BY_HANDLE_FILE_INFORMATION fileinfo;
    H5FD_t			*ret_value;
    struct _stati64 sb;
#ifndef WINDOWS_USE_STDIO
    int o_flags;
#else
	unsigned		write_access = 0;
#endif /* WINDOWS_USE_STDIO */

    FUNC_ENTER_NOAPI(H5FD_windows_open, NULL)

    /* Sanity check on file offsets */
    assert(sizeof(file_offset_t)>=sizeof(size_t));

    /* Check arguments */
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid file name")
    if (0==maxaddr || HADDR_UNDEF==maxaddr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, NULL, "bogus maxaddr")
    if (ADDR_OVERFLOW(maxaddr))
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, NULL, "bogus maxaddr")

#ifndef WINDOWS_USE_STDIO
    /* Build the open flags */
    o_flags = (H5F_ACC_RDWR & flags) ? O_RDWR : O_RDONLY;
    if (H5F_ACC_TRUNC & flags) o_flags |= O_TRUNC;
    if (H5F_ACC_CREAT & flags) o_flags |= O_CREAT;
    if (H5F_ACC_EXCL & flags) o_flags |= O_EXCL;
	/* Windows needs O_BINARY to correctly handle eol characters */
	o_flags |= O_BINARY;

    /* Open the file */
    if ((fd=_open(name, o_flags, 0666))<0)
        HSYS_GOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to open file")
    if (_fstati64(fd, &sb) == -1)
        HSYS_GOTO_ERROR(H5E_FILE, H5E_BADFILE, NULL, "unable to fstat file")

#else /* WINDOWS_USE_STDIO */
	/* Translate our flags into a mode, and open the file */
    if (_access_s(name, F_OK)) {
        if ((flags & H5F_ACC_CREAT) && (flags & H5F_ACC_RDWR)) {
            f = fopen(name, "wb+");
            write_access=1;     /* Note the write access */
        }
        else
			HSYS_GOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "file doesn't exist and CREAT wasn't specified")
    } else if ((flags & H5F_ACC_CREAT) && (flags & H5F_ACC_EXCL)) {
		HSYS_GOTO_ERROR(H5E_FILE, H5E_FILEEXISTS, NULL, "file exists but CREAT and EXCL were specified")
    } else if (flags & H5F_ACC_RDWR) {
		if (flags & H5F_ACC_TRUNC)
            f = fopen(name, "wb+");
		else
            f = fopen(name, "rb+");
        write_access=1;     /* Note the write access */
    } else
        f = fopen(name, "rb");

    if (!f)
		HSYS_GOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "fsopen failed")

    if (_stati64(name, &sb) == -1)
        HSYS_GOTO_ERROR(H5E_FILE, H5E_BADFILE, NULL, "unable to stat file")
#endif /* WINDOWS_USE_STDIO */

	/* Create the new file struct */
    if (NULL==(file=H5FL_CALLOC(H5FD_windows_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "unable to allocate file struct")

    H5_ASSIGN_OVERFLOW(file->eof,sb.st_size,h5_stat_size_t,haddr_t);
    file->pos = HADDR_UNDEF;
    file->op = OP_UNKNOWN;

#ifndef WINDOWS_USE_STDIO
    file->fd = fd;
#else
    file->fp = f;
	if((fd = _fileno(f)) == -1)
		HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to get file descriptor for file")
    file->write_access=write_access;    /* Note the write_access for later */
#endif /* WINDOWS_USE_STDIO */

    if( (filehandle = (HANDLE)_get_osfhandle(fd)) == INVALID_HANDLE_VALUE)
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to get file handle for file")
    if(!GetFileInformationByHandle(filehandle, &fileinfo))
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to get file information")
    file->fileindexhi = fileinfo.nFileIndexHigh;
    file->fileindexlo = fileinfo.nFileIndexLow;
    file->volumeserialnumber = fileinfo.dwVolumeSerialNumber;

    /* Check for non-default FAPL */
    if(H5P_FILE_ACCESS_DEFAULT != fapl_id) {
        H5P_genplist_t      *plist;      /* Property list pointer */

        /* Get the FAPL */
        if(NULL == (plist = (H5P_genplist_t *)H5I_object(fapl_id)))
            HGOTO_ERROR(H5E_VFL, H5E_BADTYPE, NULL, "not a file access property list")

        /* This step is for h5repart tool only. If user wants to change file driver from
         * family to sec2 while using h5repart, this private property should be set so that
         * in the later step, the library can ignore the family driver information saved
         * in the superblock.
         */
        if(H5P_exist_plist(plist, H5F_ACS_FAMILY_TO_SEC2_NAME) > 0)
            if(H5P_get(plist, H5F_ACS_FAMILY_TO_SEC2_NAME, &file->fam_to_sec2) < 0)
                HGOTO_ERROR(H5E_VFL, H5E_CANTGET, NULL, "can't get property of changing family to sec2")
    } /* end if */

    /* Set return value */
    ret_value=(H5FD_t*)file;

done:
    if(ret_value==NULL) {
#ifndef WINDOWS_USE_STDIO
        if(fd>=0)
            _close(fd);
#else
        if(f)
			fclose(file->fp);
#endif /* WINDOWS_USE_STDIO */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:	H5FD_windows_close
 *
 * Purpose:	Closes a Windows file.
 *
 * Return:	Success:	0
 *
 *		Failure:	-1, file not closed.
 *
 * Programmer:	Scott Wegner
 *				Based on code by Robb Matzke
 *              Thursday, May 24 2007
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_windows_close(H5FD_t *_file)
{
    H5FD_windows_t	*file = (H5FD_windows_t*)_file;
    herr_t			ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_windows_close, FAIL)

#ifndef WINDOWS_USE_STDIO
    if (_close(file->fd)<0)
#else
    if (fclose(file->fp)<0)
#endif /* WINDOWS_USE_STDIO */
        HSYS_GOTO_ERROR(H5E_IO, H5E_CANTCLOSEFILE, FAIL, "unable to close file")

    H5FL_FREE(H5FD_windows_t,file);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:	H5FD_windows_cmp
 *
 * Purpose:	Compares two files belonging to this driver using an
 *		arbitrary (but consistent) ordering.
 *
 * Return:	Success:	A value like strcmp()
 *
 *			Failure:	never fails (arguments were checked by the
 *						caller).
 *
 * Programmer:	Scott Wegner
 *				Based on code by Robb Matzke
 *              Thursday, May 24 2007
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
H5FD_windows_cmp(const H5FD_t *_f1, const H5FD_t *_f2)
{
    const H5FD_windows_t	*f1 = (const H5FD_windows_t*)_f1;
    const H5FD_windows_t	*f2 = (const H5FD_windows_t*)_f2;
    int ret_value=0;

    FUNC_ENTER_NOAPI(H5FD_windows_cmp, H5FD_VFD_DEFAULT)

	if (f1->volumeserialnumber < f2->volumeserialnumber) HGOTO_DONE(-1)
	if (f1->volumeserialnumber > f2->volumeserialnumber) HGOTO_DONE(1)

    if (f1->fileindexhi < f2->fileindexhi) HGOTO_DONE(-1)
    if (f1->fileindexhi > f2->fileindexhi) HGOTO_DONE(1)

    if (f1->fileindexlo < f2->fileindexlo) HGOTO_DONE(-1)
    if (f1->fileindexlo > f2->fileindexlo) HGOTO_DONE(1)


done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:	H5FD_windows_query
 *
 * Purpose:	Set the flags that this VFL driver is capable of supporting.
 *              (listed in H5FDpublic.h)
 *
 * Return:	Success:	non-negative
 *
 *		    Failure:	negative
 *
 * Programmer:	Scott Wegner
				Based on code by Quincey Koziol
 *              Thursday, May 24 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_windows_query(const H5FD_t *_file, unsigned long *flags /* out */)
{
    const H5FD_windows_t *file = (const H5FD_windows_t*)_file;    /* windows VFD info */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5FD_windows_query)

    /* Set the VFL feature flags that this driver supports */
    if(flags) {
        *flags = 0;
        *flags|=H5FD_FEAT_AGGREGATE_METADATA;	/* OK to aggregate metadata allocations */
        *flags|=H5FD_FEAT_ACCUMULATE_METADATA;	/* OK to accumulate metadata for faster writes */
        *flags|=H5FD_FEAT_DATA_SIEVE;			/* OK to perform data sieving for faster raw data reads & writes */
        *flags|=H5FD_FEAT_AGGREGATE_SMALLDATA;	/* OK to aggregate "small" raw data allocations */

        /* Check for flags that are set by h5repart */
        if(file->fam_to_sec2)
            *flags |= H5FD_FEAT_IGNORE_DRVRINFO; /* Ignore the driver info when file is opened (which eliminates it) */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5FD_windows_query() */

/*-------------------------------------------------------------------------
 * Function:	H5FD_windows_get_eoa
 *
 * Purpose:	Gets the end-of-address marker for the file. The EOA marker
 *			is the first address past the last byte allocated in the
 *			format address space.
 *
 * Return:	Success:	The end-of-address marker.
 *
 *			Failure:	HADDR_UNDEF
 *
 * Programmer:	Scott Wegner
 *				Based on code by Robb Matzke
 *              Thursday, May 24 2007
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_windows_get_eoa(const H5FD_t *_file, H5FD_mem_t UNUSED type)
{
    const H5FD_windows_t	*file = (const H5FD_windows_t*)_file;
    haddr_t ret_value;  /* Return value */

    FUNC_ENTER_NOAPI(H5FD_windows_get_eoa, HADDR_UNDEF)

    /* Set return value */
    ret_value=file->eoa;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_windows_set_eoa
 *
 * Purpose:	Set the end-of-address marker for the file. This function is
 *			called shortly after an existing HDF5 file is opened in order
 *			to tell the driver where the end of the HDF5 data is located.
 *
 * Return:	Success:	0
 *
 *			Failure:	-1
 *
 * Programmer:	Scott Wegner
 *				Based on code by Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_windows_set_eoa(H5FD_t *_file, H5FD_mem_t UNUSED type, haddr_t addr)
{
    H5FD_windows_t	*file = (H5FD_windows_t*)_file;
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5FD_windows_set_eoa, FAIL)

    file->eoa = addr;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:	H5FD_windows_get_eof
 *
 * Purpose:	Returns the end-of-file marker, which is the greater of
 *			either the Windows end-of-file or the HDF5 end-of-address
 *			markers.
 *
 * Return:	Success:	End of file address, the first address past
 *						the end of the "file", either the Unix file
 *						or the HDF5 file.
 *
 *			Failure:	HADDR_UNDEF
 *
 * Programmer:	Scott Wegner
 *				Based on code by Robb Matzke
 *              Thursday, May 24 2007
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_windows_get_eof(const H5FD_t *_file)
{
    const H5FD_windows_t	*file = (const H5FD_windows_t*)_file;
    haddr_t ret_value;  /* Return value */

    FUNC_ENTER_NOAPI(H5FD_windows_get_eof, HADDR_UNDEF)

    /* Set return value */
    ret_value=MAX(file->eof, file->eoa);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:		H5FD_windows_get_handle
 *
 * Purpose:			Returns the file handle of windows file driver.
 *
 * Returns:			Non-negative if succeed or negative if fails.
 *
 * Programmer:		Scott Wegner
					Based on code by Raymond Lu
 *					Thursday, May 24 2007
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5FD_windows_get_handle(H5FD_t *_file, hid_t UNUSED fapl, void** file_handle)
{
    H5FD_windows_t         *file = (H5FD_windows_t *)_file;
    herr_t              ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_windows_get_handle, FAIL)

    if(!file_handle)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file handle not valid")
#ifndef WINDOWS_USE_STDIO
    *file_handle = &(file->fd);
#else
    *file_handle = &(file->fp);
#endif /* WINDOWS_USE_STDIO */
	if(*file_handle==NULL)
        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "get handle failed")


done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:	H5FD_windows_alloc
 *
 * Purpose:	Allocates file memory.
 *
 * Return:	Success:	Address of new memory
 *
 *			Failure:	HADDR_UNDEF
 *
 * Programmer:	Scott Wegner
				Based on code by Raymond Lu
 *              Thursday, May 24 2007
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_windows_alloc(H5FD_t *_file, H5FD_mem_t UNUSED type, hid_t UNUSED dxpl_id, hsize_t size)
{
    H5FD_windows_t	*file = (H5FD_windows_t*)_file;
    haddr_t		addr;
    haddr_t ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5FD_windows_alloc, HADDR_UNDEF)

    /* Compute the address for the block to allocate */
    addr = file->eoa;

    /* Check if we need to align this block */
    if(size >= file->pub.threshold) {
        /* Check for an already aligned block */
        if((addr % file->pub.alignment) != 0)
            addr = ((addr / file->pub.alignment) + 1) * file->pub.alignment;
    }


    file->eoa = addr + size;

    /* Set return value */
    ret_value = addr;

done:
    FUNC_LEAVE_NOAPI(ret_value)

}


/*-------------------------------------------------------------------------
 * Function:	H5FD_windows_read
 *
 * Purpose:	Reads SIZE bytes of data from FILE beginning at address ADDR
 *			into buffer BUF according to data transfer properties in
 *			DXPL_ID.
 *
 * Return:	Success:	Zero. Result is stored in caller-supplied
 *						buffer BUF.
 *
 *			Failure:	-1, Contents of buffer BUF are undefined.
 *
 * Programmer:	Scott Wegner
				Based on code by Robb Matzke
 *              Thursday, May 24 2007
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5FD_windows_read(H5FD_t *_file, H5FD_mem_t UNUSED type, hid_t UNUSED dxpl_id, haddr_t addr,
	       size_t size, void *buf/*out*/)
{
    H5FD_windows_t		*file = (H5FD_windows_t*)_file;
    int					nbytes;
    herr_t				ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_windows_read, FAIL)

    assert(file && file->pub.cls);
    assert(buf);

    /* Check for overflow conditions */
    if (HADDR_UNDEF==addr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "addr undefined")
    if (REGION_OVERFLOW(addr, size))
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr overflow")
    if((addr + size) > file->eoa)
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr overflow")

    /* Seek to the correct location */
    if ((addr!=file->pos || OP_READ!=file->op))
#ifndef WINDOWS_USE_STDIO
		if (_lseeki64(file->fd, addr, SEEK_SET) == -1) {
#else
        if (_fseeki64(file->fp, addr, SEEK_SET) == -1) {
#endif /* WINDOWS_USE_STDIO */
            file->op = OP_UNKNOWN;
            file->pos = HADDR_UNDEF;
			HSYS_GOTO_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "unable to seek to proper position")
		}

    /*
     * Read data, being careful of interrupted system calls, partial results,
     * and the end of the file.
     */
    while (size>0) {
#ifndef WINDOWS_USE_STDIO
        do {
			nbytes = _read(file->fd, buf, (unsigned)(size <= WINDOWS_MAX_BUF ? size: WINDOWS_MAX_BUF));
        } while (-1==nbytes && EINTR==errno);
        if (-1==nbytes) /* error */
            HSYS_GOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "file read failed")
        if (0==nbytes) {
            /* end of file but not end of format address space */
            HDmemset(buf, 0, size);
            break;
        }
#else
        do {
            nbytes = fread(buf,(size_t)1,(size <= WINDOWS_MAX_BUF ? size: WINDOWS_MAX_BUF),file->fp);
        } while (!nbytes && EINTR==errno);
		if(!nbytes) {
            if (ferror(file->fp)) /* error */
                HSYS_GOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "file read failed")
			if (feof(file->fp)) {
				/* end of file but not end of format address space */
				HDmemset(buf, 0, size);
				break;
			}
		}
#endif /* WINDOWS_USE_STDIO */
        assert(nbytes>=0);
        assert((size_t)nbytes<=size);
        H5_CHECK_OVERFLOW(nbytes,ssize_t,size_t);
        size -= (size_t)nbytes;
        H5_CHECK_OVERFLOW(nbytes,ssize_t,haddr_t);
        addr += (haddr_t)nbytes;
        buf = (char*)buf + nbytes;
    }

    /* Update current position */
    file->pos = addr;
    file->op = OP_READ;

done:
    if(ret_value<0) {
        /* Reset last file I/O information */
        file->pos = HADDR_UNDEF;
        file->op = OP_UNKNOWN;
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_windows_write
 *
 * Purpose:	Writes SIZE bytes of data to FILE beginning at address ADDR
 *		from buffer BUF according to data transfer properties in
 *		DXPL_ID.
 *
 * Return:	Success:	Zero
 *
 *			Failure:	-1
 *
 * Programmer:	Scott Wegner
 *				Based on code by Robb Matzke
 *              Thursday, May 24 2007
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5FD_windows_write(H5FD_t *_file, H5FD_mem_t UNUSED type, hid_t UNUSED dxpl_id, haddr_t addr,
		size_t size, const void *buf)
{
    H5FD_windows_t		*file = (H5FD_windows_t*)_file;
    ssize_t		nbytes;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_windows_write, FAIL)

    assert(file && file->pub.cls);
    assert(buf);

    /* Check for overflow conditions */
    if (HADDR_UNDEF==addr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "addr undefined")
    if (REGION_OVERFLOW(addr, size))
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr overflow")
    if (addr+size>file->eoa)
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr overflow")

    /* Seek to the correct location */
    if ((addr!=file->pos || OP_WRITE!=file->op))
	{
#ifndef WINDOWS_USE_STDIO
		if (_lseeki64(file->fd, addr, SEEK_SET) == -1) {
#else
        if (_fseeki64(file->fp, addr, SEEK_SET) == -1) {
#endif /* WINDOWS_USE_STDIO */
            file->op = OP_UNKNOWN;
            file->pos = HADDR_UNDEF;
	        HSYS_GOTO_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "unable to seek to proper position")
		}
	}

    /*
     * Write the data, being careful of interrupted system calls and partial
     * results
     */
	while (size>0) {
        do {
#ifndef WINDOWS_USE_STDIO
            nbytes = _write(file->fd, buf, (unsigned)(size <= WINDOWS_MAX_BUF ? size: WINDOWS_MAX_BUF));
        } while (-1==nbytes && EINTR==errno);
        if (-1==nbytes) /* error */
#else


		/* Write 1GB or less at a time */
			nbytes = fwrite(buf, 1, (size <= WINDOWS_MAX_BUF ? size: WINDOWS_MAX_BUF),file->fp);
		} while (!nbytes && EINTR==errno);
        if (!nbytes) /* error */
#endif /* WINDOWS_USE_STDIO */
            HSYS_GOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "file write failed")
        assert(nbytes>0);
        assert((size_t)nbytes<=size);
        H5_CHECK_OVERFLOW(nbytes,ssize_t,size_t);
        size -= (size_t)nbytes;
        H5_CHECK_OVERFLOW(nbytes,ssize_t,haddr_t);
        addr += (haddr_t)nbytes;
        buf = (const char*)buf + nbytes;
	}

    /* Update current position and eof */
    file->pos = addr;
    file->op = OP_WRITE;
    if (file->pos>file->eof)
        file->eof = file->pos;

done:
    if(ret_value<0) {
        /* Reset last file I/O information */
        file->pos = HADDR_UNDEF;
        file->op = OP_UNKNOWN;
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_windows_flush
 *
 * Purpose:	Makes sure that the true file size is the same (or larger)
 *		than the end-of-address.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Scott Wegner
 *		Based on code by Robb Matzke
 *              Thursday, May 24 2007
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5FD_windows_flush(H5FD_t *_file, hid_t UNUSED dxpl_id, unsigned closing)
{
    H5FD_windows_t	*file = (H5FD_windows_t*)_file;
    herr_t		ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_windows_flush, FAIL)

    HDassert(file);

    /* Only try to flush if we have write access */
    if(file->write_access) {
        /* Flush */
        if(!closing) {
#ifdef WINDOWS_USE_STDIO
            if(fflush(file->fp) == EOF)
                HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "fflush failed")
#endif /* WINDOWS_USE_STDIO */

            /* Reset last file I/O information */
            file->pos = HADDR_UNDEF;
            file->op = OP_UNKNOWN;
        } /* end if */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_windows_flush() */


/*-------------------------------------------------------------------------
 * Function:	H5FD_windows_truncate
 *
 * Purpose:	Makes sure that the true file size is the same (or larger)
 *		than the end-of-address.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Scott Wegner
 *		Based on code by Robb Matzke
 *              Thursday, May 24 2007
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5FD_windows_truncate(H5FD_t *_file, hid_t UNUSED dxpl_id, hbool_t UNUSED closing)
{
    H5FD_windows_t	*file = (H5FD_windows_t*)_file;
#ifndef WINDOWS_USE_STDIO
    LARGE_INTEGER	li;
    HANDLE		filehandle;
#else
    int			fd;
#endif /* WINDOWS_USE_STDIO */
    herr_t		ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_windows_truncate, FAIL)

    HDassert(file);

    if(file->eoa != file->eof) {
#ifndef WINDOWS_USE_STDIO
        /* Extend the file to make sure it's large enough */
        if((filehandle = (HANDLE)_get_osfhandle(file->fd)) == INVALID_HANDLE_VALUE)
            HGOTO_ERROR(H5E_FILE, H5E_FILEOPEN, FAIL, "unable to get file handle for file")

        li.QuadPart = (__int64)file->eoa;
        (void)SetFilePointer((HANDLE)filehandle, li.LowPart, &li.HighPart, FILE_BEGIN);
        if(SetEndOfFile(filehandle) == 0)
            HGOTO_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "unable to extend file properly")
#else /* WINDOWS_USE_STDIO */
        /* Only try to flush if we have write access */
        if(!file->write_access)
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "cannot flush without write access")

        if((fd = _fileno(file->fp)) == -1)
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "unable to get file descriptor for file")
        if(_chsize_s(fd, file->eoa))
            HGOTO_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "unable to extend file properly")
#endif /* WINDOWS_USE_STDIO */

        /* Update the eof value */
        file->eof = file->eoa;

        /* Reset last file I/O information */
        file->pos = HADDR_UNDEF;
        file->op = OP_UNKNOWN;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_windows_truncate() */

#endif /* H5_HAVE_WINDOWS */

