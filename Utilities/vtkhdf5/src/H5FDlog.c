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
 * Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              Monday, April 17, 2000
 *
 * Purpose:	The POSIX unbuffered file driver using only the HDF5 public
 *		API and with a few optimizations: the lseek() call is made
 *		only when the current file position is unknown or needs to be
 *		changed based on previous I/O through this driver (don't mix
 *		I/O from this driver with I/O from other parts of the
 *		application to the same file).
 *          With custom modifications...
 */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5FD_log_init_interface


#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"		/* File access				*/
#include "H5FDprivate.h"	/* File drivers				*/
#include "H5FDlog.h"		/* Logging file driver			*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Pprivate.h"		/* Property lists			*/

#ifdef MAX
#undef MAX
#define MAX(X,Y)	((X)>(Y)?(X):(Y))
#endif /* MAX */

/* The driver identification number, initialized at runtime */
static hid_t H5FD_LOG_g = 0;

/* File operations */
#define OP_UNKNOWN	0
#define OP_READ		1
#define OP_WRITE	2

/* Driver-specific file access properties */
typedef struct H5FD_log_fapl_t {
    char *logfile;	        /* Allocated log file name */
    unsigned flags;             /* Flags for logging behavior */
    size_t buf_size;            /* Size of buffers for track flavor and number of times each byte is accessed */
} H5FD_log_fapl_t;

/* Define strings for the different file memory types */
static const char *flavors[]={   /* These are defined in H5FDpublic.h */
    "H5FD_MEM_DEFAULT",
    "H5FD_MEM_SUPER",
    "H5FD_MEM_BTREE",
    "H5FD_MEM_DRAW",
    "H5FD_MEM_GHEAP",
    "H5FD_MEM_LHEAP",
    "H5FD_MEM_OHDR",
};

/*
 * The description of a file belonging to this driver. The `eoa' and `eof'
 * determine the amount of hdf5 address space in use and the high-water mark
 * of the file (the current size of the underlying Unix file). The `pos'
 * value is used to eliminate file position updates when they would be a
 * no-op. Unfortunately we've found systems that use separate file position
 * indicators for reading and writing so the lseek can only be eliminated if
 * the current operation is the same as the previous operation.  When opening
 * a file the `eof' will be set to the current file size, `eoa' will be set
 * to zero, `pos' will be set to H5F_ADDR_UNDEF (as it is when an error
 * occurs), and `op' will be set to H5F_OP_UNKNOWN.
 */
typedef struct H5FD_log_t {
    H5FD_t	pub;			/*public stuff, must be first	*/
    int		fd;			/*the unix file			*/
    haddr_t	eoa;			/*end of allocated region	*/
    haddr_t	eof;			/*end of file; current file size*/
    haddr_t	pos;			/*current file I/O position	*/
    int		op;			    /*last operation		*/
    unsigned char *nread;   /* Number of reads from a file location */
    unsigned char *nwrite;  /* Number of write to a file location */
    unsigned char *flavor;  /* Flavor of information written to file location */
    size_t  iosize;         /* Size of I/O information buffers */
    FILE   *logfp;          /* Log file pointer */
    H5FD_log_fapl_t fa;	/*driver-specific file access properties*/
#ifndef _WIN32
    /*
     * On most systems the combination of device and i-node number uniquely
     * identify a file.
     */
    dev_t	device;			/*file device number		*/
    ino_t	inode;			/*file i-node number		*/
#else
    /*
     * On _WIN32 the low-order word of a unique identifier associated with the
     * file and the volume serial number uniquely identify a file. This number
     * (which, both? -rpm) may change when the system is restarted or when the
     * file is opened. After a process opens a file, the identifier is
     * constant until the file is closed. An application can use this
     * identifier and the volume serial number to determine whether two
     * handles refer to the same file.
     */
    DWORD fileindexlo;
    DWORD fileindexhi;
#endif
} H5FD_log_t;


/*
 * This driver supports systems that have the lseek64() function by defining
 * some macros here so we don't have to have conditional compilations later
 * throughout the code.
 *
 * file_offset_t:	The datatype for file offsets, the second argument of
 *			the lseek() or lseek64() call.
 *
 * file_seek:		The function which adjusts the current file position,
 *			either lseek() or lseek64().
 */
/* adding for windows NT file system support. */

#ifdef H5_HAVE_LSEEK64
#   define file_offset_t	off64_t
#   define file_seek		lseek64
#elif defined (_WIN32)
#   ifdef __MWERKS__
#       define file_offset_t off_t
#       define file_seek lseek
#   else /*MSVC*/
#       define file_offset_t __int64
#       define file_seek _lseeki64
#   endif
#else
#   define file_offset_t	off_t
#   define file_seek		lseek
#endif


/*
 * These macros check for overflow of various quantities.  These macros
 * assume that file_offset_t is signed and haddr_t and size_t are unsigned.
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
				 sizeof(file_offset_t)<sizeof(size_t) ||      \
                                 HADDR_UNDEF==(A)+(Z) ||		      \
				 (file_offset_t)((A)+(Z))<(file_offset_t)(A))

/* Prototypes */
static void *H5FD_log_fapl_get(H5FD_t *file);
static void *H5FD_log_fapl_copy(const void *_old_fa);
static herr_t H5FD_log_fapl_free(void *_fa);
static H5FD_t *H5FD_log_open(const char *name, unsigned flags, hid_t fapl_id,
			      haddr_t maxaddr);
static herr_t H5FD_log_close(H5FD_t *_file);
static int H5FD_log_cmp(const H5FD_t *_f1, const H5FD_t *_f2);
static herr_t H5FD_log_query(const H5FD_t *_f1, unsigned long *flags);
static haddr_t H5FD_log_alloc(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id, hsize_t size);
static haddr_t H5FD_log_get_eoa(const H5FD_t *_file, H5FD_mem_t type);
static herr_t H5FD_log_set_eoa(H5FD_t *_file, H5FD_mem_t type, haddr_t addr);
static haddr_t H5FD_log_get_eof(const H5FD_t *_file);
static herr_t  H5FD_log_get_handle(H5FD_t *_file, hid_t fapl, void** file_handle);
static herr_t H5FD_log_read(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
			     size_t size, void *buf);
static herr_t H5FD_log_write(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
			      size_t size, const void *buf);
static herr_t H5FD_log_truncate(H5FD_t *_file, hid_t dxpl_id, hbool_t closing);

#ifdef OLD_WAY
/*
 * The free list map which causes each request type to use no free lists
 */
#define H5FD_FLMAP_NOLIST {						      \
    H5FD_MEM_NOLIST,			/*default*/			      \
    H5FD_MEM_NOLIST,			/*super*/			      \
    H5FD_MEM_NOLIST,			/*btree*/			      \
    H5FD_MEM_NOLIST,			/*draw*/			      \
    H5FD_MEM_NOLIST,			/*gheap*/			      \
    H5FD_MEM_NOLIST,			/*lheap*/			      \
    H5FD_MEM_NOLIST			/*ohdr*/			      \
}
#endif /* OLD_WAY */

static const H5FD_class_t H5FD_log_g = {
    "log",					/*name			*/
    MAXADDR,					/*maxaddr		*/
    H5F_CLOSE_WEAK,				/* fc_degree		*/
    NULL,					/*sb_size		*/
    NULL,					/*sb_encode		*/
    NULL,					/*sb_decode		*/
    sizeof(H5FD_log_fapl_t),                    /*fapl_size		*/
    H5FD_log_fapl_get,		                /*fapl_get		*/
    H5FD_log_fapl_copy,		                /*fapl_copy		*/
    H5FD_log_fapl_free,		                /*fapl_free		*/
    0,						/*dxpl_size		*/
    NULL,					/*dxpl_copy		*/
    NULL,					/*dxpl_free		*/
    H5FD_log_open,				/*open			*/
    H5FD_log_close,				/*close			*/
    H5FD_log_cmp,				/*cmp			*/
    H5FD_log_query,				/*query			*/
    NULL,					/*get_type_map		*/
    H5FD_log_alloc,				/*alloc			*/
    NULL,					/*free			*/
    H5FD_log_get_eoa,				/*get_eoa		*/
    H5FD_log_set_eoa, 				/*set_eoa		*/
    H5FD_log_get_eof,				/*get_eof		*/
    H5FD_log_get_handle,                        /*get_handle            */
    H5FD_log_read,				/*read			*/
    H5FD_log_write,				/*write			*/
    NULL,					/*flush			*/
    H5FD_log_truncate,				/*truncate		*/
    NULL,                                       /*lock                  */
    NULL,                                       /*unlock                */
    H5FD_FLMAP_SINGLE 				/*fl_map		*/
};


/*--------------------------------------------------------------------------
NAME
   H5FD_log_init_interface -- Initialize interface-specific information
USAGE
    herr_t H5FD_log_init_interface()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5FD_log_init currently).

--------------------------------------------------------------------------*/
static herr_t
H5FD_log_init_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5FD_log_init_interface)

    FUNC_LEAVE_NOAPI(H5FD_log_init())
} /* H5FD_log_init_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5FD_log_init
 *
 * Purpose:	Initialize this driver by registering the driver with the
 *		library.
 *
 * Return:	Success:	The driver ID for the log driver.
 *
 *		Failure:	Negative.
 *
 * Programmer:	Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FD_log_init(void)
{
    hid_t ret_value;         /* Return value */

    FUNC_ENTER_NOAPI(H5FD_log_init, FAIL)

    if (H5I_VFL!=H5Iget_type(H5FD_LOG_g))
        H5FD_LOG_g = H5FD_register(&H5FD_log_g,sizeof(H5FD_class_t),FALSE);

    /* Set return value */
    ret_value=H5FD_LOG_g;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*---------------------------------------------------------------------------
 * Function:	H5FD_log_term
 *
 * Purpose:	Shut down the VFD
 *
 * Return:	<none>
 *
 * Programmer:  Quincey Koziol
 *              Friday, Jan 30, 2004
 *
 * Modification:
 *
 *---------------------------------------------------------------------------
 */
void
H5FD_log_term(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5FD_log_term)

    /* Reset VFL ID */
    H5FD_LOG_g=0;

    FUNC_LEAVE_NOAPI_VOID
} /* end H5FD_log_term() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_fapl_log
 *
 * Purpose:	Modify the file access property list to use the H5FD_LOG
 *		driver defined in this source file.  There are no driver
 *		specific properties.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Thursday, February 19, 1998
 *
 * Modifications:
 *              We copy the LOGFILE value into our own access properties.
 *
 * 		Raymond Lu, 2001-10-25
 *		Changed the file access list to the new generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fapl_log(hid_t fapl_id, const char *logfile, unsigned flags, size_t buf_size)
{
    H5FD_log_fapl_t	fa;     /* File access property list information */
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value;

    FUNC_ENTER_API(H5Pset_fapl_log, FAIL)
    H5TRACE4("e", "i*sIuz", fapl_id, logfile, flags, buf_size);

    if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list")

    fa.logfile = (char *)logfile;
    fa.flags = flags;
    fa.buf_size = buf_size;
    ret_value = H5P_set_driver(plist, H5FD_LOG, &fa);

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_fapl_log() */


/*-------------------------------------------------------------------------
 * Function:	H5FD_log_fapl_get
 *
 * Purpose:	Returns a file access property list which indicates how the
 *		specified file is being accessed. The return list could be
 *		used to access another file the same way.
 *
 * Return:	Success:	Ptr to new file access property list with all
 *				members copied from the file struct.
 *
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Thursday, April 20, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5FD_log_fapl_get(H5FD_t *_file)
{
    H5FD_log_t	*file = (H5FD_log_t*)_file;
    void *ret_value;    /* Return value */

    FUNC_ENTER_NOAPI(H5FD_log_fapl_get, NULL)

    /* Set return value */
    ret_value= H5FD_log_fapl_copy(&(file->fa));

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_log_fapl_get() */


/*-------------------------------------------------------------------------
 * Function:	H5FD_log_fapl_copy
 *
 * Purpose:	Copies the log-specific file access properties.
 *
 * Return:	Success:	Ptr to a new property list
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Thursday, April 20, 2000
 *
 *-------------------------------------------------------------------------
 */
static void *
H5FD_log_fapl_copy(const void *_old_fa)
{
    const H5FD_log_fapl_t *old_fa = (const H5FD_log_fapl_t*)_old_fa;
    H5FD_log_fapl_t *new_fa = NULL;    /* New FAPL info */
    void *ret_value;    /* Return value */

    FUNC_ENTER_NOAPI(H5FD_log_fapl_copy, NULL)

    HDassert(old_fa);

    /* Allocate the new FAPL info */
    if(NULL == (new_fa = (H5FD_log_fapl_t *)H5MM_calloc(sizeof(H5FD_log_fapl_t))))
        HGOTO_ERROR(H5E_FILE, H5E_CANTALLOC, NULL, "unable to allocate log file FAPL")

    /* Copy the general information */
    HDmemcpy(new_fa, old_fa, sizeof(H5FD_log_fapl_t));

    /* Deep copy the log file name */
    if(old_fa->logfile != NULL)
        if(NULL == (new_fa->logfile = H5MM_xstrdup(old_fa->logfile)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "unable to allocate log file name")

    /* Set return value */
    ret_value = new_fa;

done:
    if(NULL == ret_value)
        if(new_fa) {
            if(new_fa->logfile)
                H5MM_free(new_fa->logfile);
            H5MM_free(new_fa);
        } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_log_fapl_copy() */


/*-------------------------------------------------------------------------
 * Function:	H5FD_log_fapl_free
 *
 * Purpose:	Frees the log-specific file access properties.
 *
 * Return:	Success:	0
 *		Failure:	-1
 *
 * Programmer:	Quincey Koziol
 *              Thursday, April 20, 2000
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_log_fapl_free(void *_fa)
{
    H5FD_log_fapl_t	*fa = (H5FD_log_fapl_t*)_fa;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5FD_log_fapl_free)

    /* Free the fapl information */
    if(fa->logfile)
        H5MM_xfree(fa->logfile);
    H5MM_xfree(fa);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5FD_log_fapl_free() */


/*-------------------------------------------------------------------------
 * Function:	H5FD_log_open
 *
 * Purpose:	Create and/or opens a Unix file as an HDF5 file.
 *
 * Return:	Success:	A pointer to a new file data structure. The
 *				public fields will be initialized by the
 *				caller, which is always H5FD_open().
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Thursday, July 29, 1999
 *
 *-------------------------------------------------------------------------
 */
static H5FD_t *
H5FD_log_open(const char *name, unsigned flags, hid_t fapl_id,
	       haddr_t maxaddr)
{
    int	        o_flags;
    int		fd = (-1);
    H5FD_log_t	*file = NULL;
    H5FD_log_fapl_t	*fa;     /* File access property list information */
#ifdef _WIN32
    HFILE filehandle;
    struct _BY_HANDLE_FILE_INFORMATION fileinfo;
#endif
    h5_stat_t   sb;
    H5P_genplist_t *plist;      /* Property list */
    H5FD_t	*ret_value;

    FUNC_ENTER_NOAPI(H5FD_log_open, NULL)

    /* Check arguments */
    if(!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid file name")
    if(0 == maxaddr || HADDR_UNDEF == maxaddr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, NULL, "bogus maxaddr")
    if(ADDR_OVERFLOW(maxaddr))
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, NULL, "bogus maxaddr")

    /* Build the open flags */
    o_flags = (H5F_ACC_RDWR & flags) ? O_RDWR : O_RDONLY;
    if(H5F_ACC_TRUNC & flags)
        o_flags |= O_TRUNC;
    if(H5F_ACC_CREAT & flags)
        o_flags |= O_CREAT;
    if(H5F_ACC_EXCL & flags)
        o_flags |= O_EXCL;

    /* Open the file */
    if((fd = HDopen(name, o_flags, 0666)) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to open file")
    if(HDfstat(fd, &sb) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_BADFILE, NULL, "unable to fstat file")

    /* Create the new file struct */
    if(NULL == (file = (H5FD_log_t *)H5MM_calloc(sizeof(H5FD_log_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "unable to allocate file struct")

    /* Get the driver specific information */
    if(NULL == (plist = H5P_object_verify(fapl_id, H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access property list")
    fa = (H5FD_log_fapl_t *)H5P_get_driver_info(plist);
    HDassert(fa);

    file->fd = fd;
    H5_ASSIGN_OVERFLOW(file->eof, sb.st_size, h5_stat_size_t, haddr_t);
    file->pos = HADDR_UNDEF;
    file->op = OP_UNKNOWN;
#ifdef _WIN32
    filehandle = _get_osfhandle(fd);
    (void)GetFileInformationByHandle((HANDLE)filehandle, &fileinfo);
    file->fileindexhi = fileinfo.nFileIndexHigh;
    file->fileindexlo = fileinfo.nFileIndexLow;
#else
    file->device = sb.st_dev;
    file->inode = sb.st_ino;
#endif

    /* Get the flags for logging */
    file->fa.flags = fa->flags;

    /* Check if we are doing any logging at all */
    if(file->fa.flags != 0) {
        file->iosize = fa->buf_size;
        if(file->fa.flags & H5FD_LOG_FILE_READ) {
            file->nread = (unsigned char *)H5MM_calloc(file->iosize);
            HDassert(file->nread);
        } /* end if */
        if(file->fa.flags & H5FD_LOG_FILE_WRITE) {
            file->nwrite = (unsigned char *)H5MM_calloc(file->iosize);
            HDassert(file->nwrite);
        } /* end if */
        if(file->fa.flags & H5FD_LOG_FLAVOR) {
            file->flavor = (unsigned char *)H5MM_calloc(file->iosize);
            HDassert(file->flavor);
        } /* end if */
        if(fa->logfile)
            file->logfp = HDfopen(fa->logfile, "w");
        else
            file->logfp = stderr;
    } /* end if */

    /* Set return value */
    ret_value = (H5FD_t*)file;

done:
    if(NULL == ret_value) {
        if(fd >= 0)
            HDclose(fd);
        file = (H5FD_log_t *)H5MM_xfree(file);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_log_open() */


/*-------------------------------------------------------------------------
 * Function:	H5FD_log_close
 *
 * Purpose:	Closes a Unix file.
 *
 * Return:	Success:	0
 *		Failure:	-1, file not closed.
 *
 * Programmer:	Robb Matzke
 *              Thursday, July 29, 1999
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_log_close(H5FD_t *_file)
{
    H5FD_log_t	*file = (H5FD_log_t*)_file;
#ifdef H5_HAVE_GETTIMEOFDAY
    struct timeval timeval_start,timeval_stop;
    struct timeval timeval_diff;
#endif /* H5_HAVE_GETTIMEOFDAY */
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_log_close, FAIL)

#ifdef H5_HAVE_GETTIMEOFDAY
    if(file->fa.flags&H5FD_LOG_TIME_CLOSE)
        HDgettimeofday(&timeval_start,NULL);
#endif /* H5_HAVE_GETTIMEOFDAY */
    if(HDclose(file->fd) < 0)
        HGOTO_ERROR(H5E_IO, H5E_CANTCLOSEFILE, FAIL, "unable to close file")
#ifdef H5_HAVE_GETTIMEOFDAY
    if(file->fa.flags&H5FD_LOG_TIME_CLOSE)
        HDgettimeofday(&timeval_stop,NULL);
#endif /* H5_HAVE_GETTIMEOFDAY */

    /* Dump I/O information */
    if(file->fa.flags != 0) {
        haddr_t addr;
        haddr_t last_addr;
        unsigned char last_val;

#ifdef H5_HAVE_GETTIMEOFDAY
        if(file->fa.flags & H5FD_LOG_TIME_CLOSE) {
             /* Calculate the elapsed gettimeofday time */
             timeval_diff.tv_usec = timeval_stop.tv_usec - timeval_start.tv_usec;
             timeval_diff.tv_sec = timeval_stop.tv_sec - timeval_start.tv_sec;
             if(timeval_diff.tv_usec < 0) {
                 timeval_diff.tv_usec += 1000000;
                 timeval_diff.tv_sec--;
             } /* end if */
            HDfprintf(file->logfp, "Close took: (%f s)\n", (double)timeval_diff.tv_sec + ((double)timeval_diff.tv_usec / (double)1000000.0));
        } /* end if */
#endif /* H5_HAVE_GETTIMEOFDAY */

        /* Dump the write I/O information */
        if(file->fa.flags & H5FD_LOG_FILE_WRITE) {
            HDfprintf(file->logfp, "Dumping write I/O information:\n");
            last_val = file->nwrite[0];
            last_addr = 0;
            addr = 1;
            while(addr < file->eoa) {
                if(file->nwrite[addr] != last_val) {
                    HDfprintf(file->logfp, "\tAddr %10a-%10a (%10lu bytes) written to %3d times\n", last_addr, (addr - 1), (unsigned long)(addr - last_addr), (int)last_val);
                    last_val = file->nwrite[addr];
                    last_addr = addr;
                } /* end if */
                addr++;
            } /* end while */
            HDfprintf(file->logfp, "\tAddr %10a-%10a (%10lu bytes) written to %3d times\n", last_addr, (addr - 1), (unsigned long)(addr - last_addr), (int)last_val);
        } /* end if */

        /* Dump the read I/O information */
        if(file->fa.flags & H5FD_LOG_FILE_READ) {
            HDfprintf(file->logfp, "Dumping read I/O information:\n");
            last_val = file->nread[0];
            last_addr = 0;
            addr = 1;
            while(addr < file->eoa) {
                if(file->nread[addr] != last_val) {
                    HDfprintf(file->logfp, "\tAddr %10a-%10a (%10lu bytes) read from %3d times\n", last_addr, (addr - 1), (unsigned long)(addr - last_addr), (int)last_val);
                    last_val = file->nread[addr];
                    last_addr = addr;
                } /* end if */
                addr++;
            } /* end while */
            HDfprintf(file->logfp, "\tAddr %10a-%10a (%10lu bytes) read from %3d times\n", last_addr, (addr - 1), (unsigned long)(addr - last_addr), (int)last_val);
        } /* end if */

        /* Dump the I/O flavor information */
        if(file->fa.flags & H5FD_LOG_FLAVOR) {
            HDfprintf(file->logfp, "Dumping I/O flavor information:\n");
            last_val = file->flavor[0];
            last_addr = 0;
            addr = 1;
            while(addr < file->eoa) {
                if(file->flavor[addr] != last_val) {
                    HDfprintf(file->logfp, "\tAddr %10a-%10a (%10lu bytes) flavor is %s\n", last_addr, (addr - 1), (unsigned long)(addr - last_addr), flavors[last_val]);
                    last_val = file->flavor[addr];
                    last_addr = addr;
                } /* end if */
                addr++;
            } /* end while */
            HDfprintf(file->logfp, "\tAddr %10a-%10a (%10lu bytes) flavor is %s\n", last_addr, (addr - 1), (unsigned long)(addr - last_addr), flavors[last_val]);
        } /* end if */

        /* Free the logging information */
        if(file->fa.flags & H5FD_LOG_FILE_WRITE)
            file->nwrite = (unsigned char *)H5MM_xfree(file->nwrite);
        if(file->fa.flags & H5FD_LOG_FILE_READ)
            file->nread = (unsigned char *)H5MM_xfree(file->nread);
        if(file->fa.flags & H5FD_LOG_FLAVOR)
            file->flavor = (unsigned char *)H5MM_xfree(file->flavor);
        if(file->logfp != stderr)
            fclose(file->logfp);
    } /* end if */

    H5MM_xfree(file);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_log_close() */


/*-------------------------------------------------------------------------
 * Function:	H5FD_log_cmp
 *
 * Purpose:	Compares two files belonging to this driver using an
 *		arbitrary (but consistent) ordering.
 *
 * Return:	Success:	A value like strcmp()
 *
 *		Failure:	never fails (arguments were checked by the
 *				caller).
 *
 * Programmer:	Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
H5FD_log_cmp(const H5FD_t *_f1, const H5FD_t *_f2)
{
    const H5FD_log_t	*f1 = (const H5FD_log_t*)_f1;
    const H5FD_log_t	*f2 = (const H5FD_log_t*)_f2;
    int ret_value=0;

    FUNC_ENTER_NOAPI(H5FD_log_cmp, H5FD_VFD_DEFAULT)

#ifdef _WIN32
    if (f1->fileindexhi < f2->fileindexhi) HGOTO_DONE(-1)
    if (f1->fileindexhi > f2->fileindexhi) HGOTO_DONE(1)

    if (f1->fileindexlo < f2->fileindexlo) HGOTO_DONE(-1)
    if (f1->fileindexlo > f2->fileindexlo) HGOTO_DONE(1)

#else
#ifdef H5_DEV_T_IS_SCALAR
    if (f1->device < f2->device) HGOTO_DONE(-1)
    if (f1->device > f2->device) HGOTO_DONE(1)
#else /* H5_DEV_T_IS_SCALAR */
    /* If dev_t isn't a scalar value on this system, just use memcmp to
     * determine if the values are the same or not.  The actual return value
     * shouldn't really matter...
     */
    if(HDmemcmp(&(f1->device),&(f2->device),sizeof(dev_t))<0) HGOTO_DONE(-1)
    if(HDmemcmp(&(f1->device),&(f2->device),sizeof(dev_t))>0) HGOTO_DONE(1)
#endif /* H5_DEV_T_IS_SCALAR */

    if (f1->inode < f2->inode) HGOTO_DONE(-1)
    if (f1->inode > f2->inode) HGOTO_DONE(1)
#endif

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_log_query
 *
 * Purpose:	Set the flags that this VFL driver is capable of supporting.
 *              (listed in H5FDpublic.h)
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Friday, August 25, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5FD_log_query(const H5FD_t UNUSED * _f, unsigned long *flags /* out */)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5FD_log_query)

    /* Set the VFL feature flags that this driver supports */
    if(flags) {
        *flags = 0;
        *flags |= H5FD_FEAT_AGGREGATE_METADATA; /* OK to aggregate metadata allocations */
        *flags |= H5FD_FEAT_ACCUMULATE_METADATA; /* OK to accumulate metadata for faster writes */
        *flags |= H5FD_FEAT_DATA_SIEVE;       /* OK to perform data sieving for faster raw data reads & writes */
        *flags |= H5FD_FEAT_AGGREGATE_SMALLDATA; /* OK to aggregate "small" raw data allocations */
        *flags |= H5FD_FEAT_POSIX_COMPAT_HANDLE; /* VFD handle is POSIX I/O call compatible */
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5FD_log_query() */


/*-------------------------------------------------------------------------
 * Function:	H5FD_log_alloc
 *
 * Purpose:	Allocate file memory.
 *
 * Return:	Success:	Address of new memory
 *
 *		Failure:	HADDR_UNDEF
 *
 * Programmer:	Quincey Koziol
 *              Monday, April 17, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static haddr_t
H5FD_log_alloc(H5FD_t *_file, H5FD_mem_t type, hid_t UNUSED dxpl_id, hsize_t size)
{
    H5FD_log_t	*file = (H5FD_log_t*)_file;
    haddr_t		addr;
    haddr_t ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5FD_log_alloc, HADDR_UNDEF)

    /* Compute the address for the block to allocate */
    addr = file->eoa;

    /* Check if we need to align this block */
    if(size>=file->pub.threshold) {
        /* Check for an already aligned block */
        if(addr%file->pub.alignment!=0)
            addr=((addr/file->pub.alignment)+1)*file->pub.alignment;
    } /* end if */

    file->eoa = addr+size;

    /* Retain the (first) flavor of the information written to the file */
    if(file->fa.flags!=0) {
        if(file->fa.flags&H5FD_LOG_FLAVOR) {
            assert(addr<file->iosize);
            H5_CHECK_OVERFLOW(size,hsize_t,size_t);
            HDmemset(&file->flavor[addr],(int)type,(size_t)size);
        } /* end if */

        if(file->fa.flags&H5FD_LOG_ALLOC)
            HDfprintf(file->logfp,"%10a-%10a (%10Hu bytes) (%s) Allocated\n",addr,addr+size-1,size,flavors[type]);
    } /* end if */

    /* Set return value */
    ret_value=addr;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5FD_log_alloc() */


/*-------------------------------------------------------------------------
 * Function:	H5FD_log_get_eoa
 *
 * Purpose:	Gets the end-of-address marker for the file. The EOA marker
 *		is the first address past the last byte allocated in the
 *		format address space.
 *
 * Return:	Success:	The end-of-address marker.
 *
 *		Failure:	HADDR_UNDEF
 *
 * Programmer:	Robb Matzke
 *              Monday, August  2, 1999
 *
 * Modifications:
 *              Raymond Lu
 *              21 Dec. 2006
 *              Added the parameter TYPE.  It's only used for MULTI driver.
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_log_get_eoa(const H5FD_t *_file, H5FD_mem_t UNUSED type)
{
    const H5FD_log_t	*file = (const H5FD_log_t*)_file;
    haddr_t ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5FD_log_get_eoa, HADDR_UNDEF)

    /* Set return value */
    ret_value=file->eoa;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_log_set_eoa
 *
 * Purpose:	Set the end-of-address marker for the file. This function is
 *		called shortly after an existing HDF5 file is opened in order
 *		to tell the driver where the end of the HDF5 data is located.
 *
 * Return:	Success:	0
 *
 *		Failure:	-1
 *
 * Programmer:	Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *              Raymond Lu
 *              21 Dec. 2006
 *              Added the parameter TYPE.  It's only used for MULTI driver.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_log_set_eoa(H5FD_t *_file, H5FD_mem_t UNUSED type, haddr_t addr)
{
    H5FD_log_t	*file = (H5FD_log_t*)_file;
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5FD_log_set_eoa, FAIL)

    file->eoa = addr;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_log_get_eof
 *
 * Purpose:	Returns the end-of-file marker, which is the greater of
 *		either the Unix end-of-file or the HDF5 end-of-address
 *		markers.
 *
 * Return:	Success:	End of file address, the first address past
 *				the end of the "file", either the Unix file
 *				or the HDF5 file.
 *
 *		Failure:	HADDR_UNDEF
 *
 * Programmer:	Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_log_get_eof(const H5FD_t *_file)
{
    const H5FD_log_t	*file = (const H5FD_log_t*)_file;
    haddr_t ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5FD_log_get_eof, HADDR_UNDEF)

    /* Set return value */
    ret_value=MAX(file->eof, file->eoa);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:       H5FD_log_get_handle
 *
 * Purpose:        Returns the file handle of LOG file driver.
 *
 * Returns:        Non-negative if succeed or negative if fails.
 *
 * Programmer:     Raymond Lu
 *                 Sept. 16, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5FD_log_get_handle(H5FD_t *_file, hid_t UNUSED fapl, void** file_handle)
{
    H5FD_log_t          *file = (H5FD_log_t *)_file;
    herr_t              ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_log_get_handle, FAIL)

    if(!file_handle)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file handle not valid")

    *file_handle = &(file->fd);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_log_read
 *
 * Purpose:	Reads SIZE bytes of data from FILE beginning at address ADDR
 *		into buffer BUF according to data transfer properties in
 *		DXPL_ID.
 *
 * Return:	Success:	Zero. Result is stored in caller-supplied
 *				buffer BUF.
 *
 *		Failure:	-1, Contents of buffer BUF are undefined.
 *
 * Programmer:	Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5FD_log_read(H5FD_t *_file, H5FD_mem_t type, hid_t UNUSED dxpl_id, haddr_t addr,
	       size_t size, void *buf/*out*/)
{
    H5FD_log_t		*file = (H5FD_log_t*)_file;
    ssize_t		nbytes;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_log_read, FAIL)

    assert(file && file->pub.cls);
    assert(buf);

    /* Check for overflow conditions */
    if (HADDR_UNDEF==addr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "addr undefined")
    if (REGION_OVERFLOW(addr, size))
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr overflow")
    if((addr + size) > file->eoa)
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr overflow")

    /* Log the I/O information about the read */
    if(file->fa.flags!=0) {
        size_t tmp_size=size;
        haddr_t tmp_addr=addr;

        /* Log information about the number of times these locations are read */
        if(file->fa.flags&H5FD_LOG_FILE_READ) {
            assert((addr+size)<file->iosize);
            while(tmp_size-->0)
                file->nread[tmp_addr++]++;
        } /* end if */

        /* Log information about the seek, if it's going to occur */
        if(file->fa.flags&H5FD_LOG_LOC_SEEK) {
            if(addr!=file->pos || OP_READ!=file->op)
                HDfprintf(file->logfp,"Seek: From %10a To %10a\n",file->pos,addr);
        } /* end if */

        /* Log information about the read */
        if(file->fa.flags&H5FD_LOG_LOC_READ) {
            HDfprintf(file->logfp,"%10a-%10a (%10Zu bytes) (%s) Read\n",addr,addr+size-1,size,flavors[type]);
/* XXX: Verify the flavor information, if we have it? */
        } /* end if */
    } /* end if */

    /* Seek to the correct location */
    if ((addr!=file->pos || OP_READ!=file->op) &&
            file_seek(file->fd, (file_offset_t)addr, SEEK_SET)<0) {
        file->pos = HADDR_UNDEF;
        file->op = OP_UNKNOWN;
        HGOTO_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "unable to seek to proper position")
    }

    /*
     * Read data, being careful of interrupted system calls, partial results,
     * and the end of the file.
     */
    while (size>0) {
        do {
            nbytes = HDread(file->fd, buf, size);
        } while (-1==nbytes && EINTR==errno);
        if (-1==nbytes) {
            /* error */
            file->pos = HADDR_UNDEF;
            file->op = OP_UNKNOWN;
            HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "file read failed")
        }
        if (0==nbytes) {
            /* end of file but not end of format address space */
            HDmemset(buf, 0, size);
            size = 0;
        }
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
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_log_write
 *
 * Purpose:	Writes SIZE bytes of data to FILE beginning at address ADDR
 *		from buffer BUF according to data transfer properties in
 *		DXPL_ID.
 *
 * Return:	Success:	Zero
 *
 *		Failure:	-1
 *
 * Programmer:	Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5FD_log_write(H5FD_t *_file, H5FD_mem_t type, hid_t UNUSED dxpl_id, haddr_t addr,
		size_t size, const void *buf)
{
    H5FD_log_t		*file = (H5FD_log_t*)_file;
    ssize_t		nbytes;
    size_t      orig_size=size; /* Save the original size for later */
    haddr_t     orig_addr=addr;
#ifdef H5_HAVE_GETTIMEOFDAY
    struct timeval timeval_start,timeval_stop;
    struct timeval timeval_diff;
#endif /* H5_HAVE_GETTIMEOFDAY */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_log_write, FAIL)

    assert(file && file->pub.cls);
    assert(size>0);
    assert(buf);

    /* Verify that we are writing out the type of data we allocated in this location */
    if(file->flavor) {
        assert(type==H5FD_MEM_DEFAULT || type==(H5FD_mem_t)file->flavor[addr] || (H5FD_mem_t)file->flavor[addr]==H5FD_MEM_DEFAULT);
        assert(type==H5FD_MEM_DEFAULT || type==(H5FD_mem_t)file->flavor[(addr+size)-1] || (H5FD_mem_t)file->flavor[(addr+size)-1]==H5FD_MEM_DEFAULT);
    } /* end if */

    /* Check for overflow conditions */
    if (HADDR_UNDEF==addr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "addr undefined")
    if (REGION_OVERFLOW(addr, size))
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr overflow")
    if (addr+size>file->eoa)
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr overflow")

    /* Log the I/O information about the write */
    if(file->fa.flags&H5FD_LOG_FILE_WRITE) {
        size_t tmp_size=size;
        haddr_t tmp_addr=addr;

        assert((addr+size)<file->iosize);
        while(tmp_size-->0)
            file->nwrite[tmp_addr++]++;
    } /* end if */

    /* Seek to the correct location */
    if (addr!=file->pos || OP_WRITE!=file->op) {
#ifdef H5_HAVE_GETTIMEOFDAY
        if(file->fa.flags&H5FD_LOG_TIME_SEEK)
            HDgettimeofday(&timeval_start,NULL);
#endif /* H5_HAVE_GETTIMEOFDAY */
        if(file_seek(file->fd, (file_offset_t)addr, SEEK_SET)<0) {
            file->pos = HADDR_UNDEF;
            file->op = OP_UNKNOWN;
            HGOTO_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "unable to seek to proper position")
        } /* end if */
#ifdef H5_HAVE_GETTIMEOFDAY
        if(file->fa.flags&H5FD_LOG_TIME_SEEK)
            HDgettimeofday(&timeval_stop,NULL);
#endif /* H5_HAVE_GETTIMEOFDAY */

        /* Log information about the seek */
        if(file->fa.flags&H5FD_LOG_LOC_SEEK) {
#ifdef H5_HAVE_GETTIMEOFDAY
            HDfprintf(file->logfp,"Seek: From %10a To %10a",file->pos,addr);
            if(file->fa.flags&H5FD_LOG_TIME_SEEK) {
                 /* Calculate the elapsed gettimeofday time */
                 timeval_diff.tv_usec=timeval_stop.tv_usec-timeval_start.tv_usec;
                 timeval_diff.tv_sec=timeval_stop.tv_sec-timeval_start.tv_sec;
                 if(timeval_diff.tv_usec<0) {
                     timeval_diff.tv_usec+=1000000;
                     timeval_diff.tv_sec--;
                 } /* end if */
                HDfprintf(file->logfp," (%f s)\n",(double)timeval_diff.tv_sec+((double)timeval_diff.tv_usec/(double)1000000.0));
            } /* end if */
            else
                HDfprintf(file->logfp,"\n");
#else /* H5_HAVE_GETTIMEOFDAY */
            HDfprintf(file->logfp,"Seek: From %10a To %10a\n",file->pos,addr);
#endif /* H5_HAVE_GETTIMEOFDAY */
        } /* end if */
    } /* end if */

    /*
     * Write the data, being careful of interrupted system calls and partial
     * results
     */
#ifdef H5_HAVE_GETTIMEOFDAY
    if(file->fa.flags&H5FD_LOG_TIME_WRITE)
        HDgettimeofday(&timeval_start,NULL);
#endif /* H5_HAVE_GETTIMEOFDAY */
    while (size>0) {
        do {
            nbytes = HDwrite(file->fd, buf, size);
        } while (-1==nbytes && EINTR==errno);
        if (-1==nbytes) {
            /* error */
            file->pos = HADDR_UNDEF;
            file->op = OP_UNKNOWN;
            if(file->fa.flags&H5FD_LOG_LOC_WRITE)
                HDfprintf(file->logfp,"Error! Writing: %10a-%10a (%10Zu bytes)\n",orig_addr,orig_addr+orig_size-1,orig_size);
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "file write failed")
        }
        assert(nbytes>0);
        assert((size_t)nbytes<=size);
        H5_CHECK_OVERFLOW(nbytes,ssize_t,size_t);
        size -= (size_t)nbytes;
        H5_CHECK_OVERFLOW(nbytes,ssize_t,haddr_t);
        addr += (haddr_t)nbytes;
        buf = (const char*)buf + nbytes;
    }
#ifdef H5_HAVE_GETTIMEOFDAY
    if(file->fa.flags&H5FD_LOG_TIME_WRITE)
        HDgettimeofday(&timeval_stop,NULL);
#endif /* H5_HAVE_GETTIMEOFDAY */

    /* Log information about the write */
    if(file->fa.flags&H5FD_LOG_LOC_WRITE) {
        HDfprintf(file->logfp,"%10a-%10a (%10Zu bytes) (%s) Written",orig_addr,orig_addr+orig_size-1,orig_size,flavors[type]);

        /* Check if this is the first write into a "default" section, grabbed by the metadata agregation algorithm */
        if(file->fa.flags&H5FD_LOG_FLAVOR) {
            if((H5FD_mem_t)file->flavor[orig_addr]==H5FD_MEM_DEFAULT)
                HDmemset(&file->flavor[orig_addr],(int)type,orig_size);
        } /* end if */

#ifdef H5_HAVE_GETTIMEOFDAY
        if(file->fa.flags&H5FD_LOG_TIME_WRITE) {
             /* Calculate the elapsed gettimeofday time */
             timeval_diff.tv_usec=timeval_stop.tv_usec-timeval_start.tv_usec;
             timeval_diff.tv_sec=timeval_stop.tv_sec-timeval_start.tv_sec;
             if(timeval_diff.tv_usec<0) {
                 timeval_diff.tv_usec+=1000000;
                 timeval_diff.tv_sec--;
             } /* end if */
            HDfprintf(file->logfp," (%f s)\n",(double)timeval_diff.tv_sec+((double)timeval_diff.tv_usec/(double)1000000.0));
        } /* end if */
        else
            HDfprintf(file->logfp,"\n");
#else /* H5_HAVE_GETTIMEOFDAY */
        HDfprintf(file->logfp,"\n");
#endif /* H5_HAVE_GETTIMEOFDAY */
    } /* end if */

    /* Update current position and eof */
    file->pos = addr;
    file->op = OP_WRITE;
    if (file->pos>file->eof)
        file->eof = file->pos;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_log_truncate
 *
 * Purpose:	Makes sure that the true file size is the same (or larger)
 *		than the end-of-address.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Wednesday, August  4, 1999
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5FD_log_truncate(H5FD_t *_file, hid_t UNUSED dxpl_id, unsigned UNUSED closing)
{
    H5FD_log_t	*file = (H5FD_log_t*)_file;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_log_truncate, FAIL)

    if(file->eoa>file->eof) {
        if(-1 == file_seek(file->fd, (file_offset_t)(file->eoa - 1), SEEK_SET))
            HGOTO_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "unable to seek to proper position")
        if(HDwrite(file->fd, "", (size_t)1) != 1)
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "file write failed")
        file->eof = file->eoa;
        file->pos = file->eoa;
        file->op = OP_WRITE;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_log_truncate() */

