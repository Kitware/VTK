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
 * Programmer:  Quincey Koziol <koziol@hdfgroup.org>
 *              Monday, April 17, 2000
 *
 * Purpose:     The POSIX unbuffered file driver using only the HDF5 public
 *              API and with a few optimizations: the lseek() call is made
 *              only when the current file position is unknown or needs to be
 *              changed based on previous I/O through this driver (don't mix
 *              I/O from this driver with I/O from other parts of the
 *              application to the same file).
 *              With custom modifications...
 */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5FD_log_init_interface


#include "H5private.h"      /* Generic Functions    */
#include "H5Eprivate.h"     /* Error handling       */
#include "H5Fprivate.h"     /* File access          */
#include "H5FDprivate.h"    /* File drivers         */
#include "H5FDlog.h"        /* Logging file driver  */
#include "H5FLprivate.h"    /* Free Lists           */
#include "H5Iprivate.h"     /* IDs                  */
#include "H5MMprivate.h"    /* Memory management    */
#include "H5Pprivate.h"     /* Property lists       */

/* The driver identification number, initialized at runtime */
static hid_t H5FD_LOG_g = 0;

/* Driver-specific file access properties */
typedef struct H5FD_log_fapl_t {
    char *logfile;              /* Allocated log file name */
    unsigned long long flags;   /* Flags for logging behavior */
    size_t buf_size;            /* Size of buffers for track flavor and number of times each byte is accessed */
} H5FD_log_fapl_t;

/* Define strings for the different file memory types
 * These are defined in the H5F_mem_t enum from H5Fpublic.h
 * Note that H5FD_MEM_NOLIST is not listed here since it has
 * a negative value.
 */
static const char *flavors[]={
    "H5FD_MEM_DEFAULT",
    "H5FD_MEM_SUPER",
    "H5FD_MEM_BTREE",
    "H5FD_MEM_DRAW",
    "H5FD_MEM_GHEAP",
    "H5FD_MEM_LHEAP",
    "H5FD_MEM_OHDR",
};

/* The description of a file belonging to this driver. The `eoa' and `eof'
 * determine the amount of hdf5 address space in use and the high-water mark
 * of the file (the current size of the underlying filesystem file). The
 * `pos' value is used to eliminate file position updates when they would be a
 * no-op. Unfortunately we've found systems that use separate file position
 * indicators for reading and writing so the lseek can only be eliminated if
 * the current operation is the same as the previous operation.  When opening
 * a file the `eof' will be set to the current file size, `eoa' will be set
 * to zero, `pos' will be set to H5F_ADDR_UNDEF (as it is when an error
 * occurs), and `op' will be set to H5F_OP_UNKNOWN.
 */
typedef struct H5FD_log_t {
    H5FD_t          pub;    /* public stuff, must be first      */
    int             fd;     /* the unix file                    */
    haddr_t         eoa;    /* end of allocated region          */
    haddr_t         eof;    /* end of file; current file size   */
    haddr_t         pos;    /* current file I/O position        */
    H5FD_file_op_t  op;     /* last operation                   */
    char            filename[H5FD_MAX_FILENAME_LEN];    /* Copy of file name from open operation */
#ifndef H5_HAVE_WIN32_API
    /* On most systems the combination of device and i-node number uniquely
     * identify a file.  Note that Cygwin, MinGW and other Windows POSIX
     * environments have the stat function (which fakes inodes)
     * and will use the 'device + inodes' scheme as opposed to the
     * Windows code further below.
     */
    dev_t           device;         /* file device number   */
#ifdef H5_VMS
    ino_t           inode[3];       /* file i-node number   */
#else
    ino_t           inode;          /* file i-node number   */
#endif /*H5_VMS*/
#else
    /* Files in windows are uniquely identified by the volume serial
     * number and the file index (both low and high parts).
     *
     * There are caveats where these numbers can change, especially
     * on FAT file systems.  On NTFS, however, a file should keep
     * those numbers the same until renamed or deleted (though you
     * can use ReplaceFile() on NTFS to keep the numbers the same
     * while renaming).
     *
     * See the MSDN "BY_HANDLE_FILE_INFORMATION Structure" entry for
     * more information.
     *
     * http://msdn.microsoft.com/en-us/library/aa363788(v=VS.85).aspx
     */
    DWORD           nFileIndexLow;
    DWORD           nFileIndexHigh;
    DWORD           dwVolumeSerialNumber;
    
    HANDLE          hFile;      /* Native windows file handle */
#endif  /* H5_HAVE_WIN32_API */

    /* Information from properties set by 'h5repart' tool
     *
     * Whether to eliminate the family driver info and convert this file to
     * a single file
     */
    hbool_t     fam_to_sec2;

    /* Fields for tracking I/O operations */
    unsigned char       *nread;                 /* Number of reads from a file location             */
    unsigned char       *nwrite;                /* Number of write to a file location               */
    unsigned char       *flavor;                /* Flavor of information written to file location   */
    unsigned long long  total_read_ops;         /* Total number of read operations                  */
    unsigned long long  total_write_ops;        /* Total number of write operations                 */
    unsigned long long  total_seek_ops;         /* Total number of seek operations                  */
    unsigned long long  total_truncate_ops;     /* Total number of truncate operations              */
    double              total_read_time;        /* Total time spent in read operations              */
    double              total_write_time;       /* Total time spent in write operations             */
    double              total_seek_time;        /* Total time spent in seek operations              */
    size_t              iosize;                 /* Size of I/O information buffers                  */
    FILE                *logfp;                 /* Log file pointer                                 */
    H5FD_log_fapl_t     fa;                     /* Driver-specific file access properties           */
} H5FD_log_t;

/*
 * These macros check for overflow of various quantities.  These macros
 * assume that HDoff_t is signed and haddr_t and size_t are unsigned.
 *
 * ADDR_OVERFLOW:   Checks whether a file address of type `haddr_t'
 *                  is too large to be represented by the second argument
 *                  of the file seek function.
 *
 * SIZE_OVERFLOW:   Checks whether a buffer size of type `hsize_t' is too
 *                  large to be represented by the `size_t' type.
 *
 * REGION_OVERFLOW: Checks whether an address and size pair describe data
 *                  which can be addressed entirely by the second
 *                  argument of the file seek function.
 */
#define MAXADDR                 (((haddr_t)1<<(8*sizeof(HDoff_t)-1))-1)
#define ADDR_OVERFLOW(A)        (HADDR_UNDEF==(A) || ((A) & ~(haddr_t)MAXADDR))
#define SIZE_OVERFLOW(Z)        ((Z) & ~(hsize_t)MAXADDR)
#define REGION_OVERFLOW(A,Z)    (ADDR_OVERFLOW(A) || SIZE_OVERFLOW(Z) ||    \
                                 HADDR_UNDEF==(A)+(Z) ||                    \
                                (HDoff_t)((A)+(Z))<(HDoff_t)(A))

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
    H5FD_FLMAP_DICHOTOMY			/*fl_map		*/
};

/* Declare a free list to manage the H5FD_log_t struct */
H5FL_DEFINE_STATIC(H5FD_log_t);


/*-------------------------------------------------------------------------
 * Function:    H5FD_log_init_interface
 *
 * Purpose:     Initializes any interface-specific data or routines.
 *
 * Return:      Success:    The driver ID for the log driver.
 *              Failure:    Negative.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_log_init_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    FUNC_LEAVE_NOAPI(H5FD_log_init())
} /* H5FD_log_init_interface() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_log_init
 *
 * Purpose:     Initialize this driver by registering the driver with the
 *              library.
 *
 * Return:      Success:    The driver ID for the log driver.
 *              Failure:    Negative.
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FD_log_init(void)
{
    hid_t ret_value;            /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if(H5I_VFL != H5I_get_type(H5FD_LOG_g))
        H5FD_LOG_g = H5FD_register(&H5FD_log_g, sizeof(H5FD_class_t), FALSE);

    /* Set return value */
    ret_value = H5FD_LOG_g;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_log_init() */


/*---------------------------------------------------------------------------
 * Function:    H5FD_log_term
 *
 * Purpose:     Shut down the VFD
 *
 * Returns:     <none>
 *
 * Programmer:  Quincey Koziol
 *              Friday, Jan 30, 2004
 *
 *---------------------------------------------------------------------------
 */
void
H5FD_log_term(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Reset VFL ID */
    H5FD_LOG_g = 0;

    FUNC_LEAVE_NOAPI_VOID
} /* end H5FD_log_term() */


/*-------------------------------------------------------------------------
 * Function:    H5Pset_fapl_log
 *
 * Purpose:     Modify the file access property list to use the H5FD_LOG
 *              driver defined in this source file.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Robb Matzke
 *              Thursday, February 19, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fapl_log(hid_t fapl_id, const char *logfile, unsigned long long flags, size_t buf_size)
{
    H5FD_log_fapl_t     fa;         /* File access property list information */
    H5P_genplist_t      *plist;     /* Property list pointer */
    herr_t              ret_value;  /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "i*sULz", fapl_id, logfile, flags, buf_size);

    /* Check arguments */
    if(NULL == (plist = H5P_object_verify(fapl_id, H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list")

    /* This shallow copy is correct! The string will be properly 
     * copied deep down in the H5P code. 
     */
    fa.logfile = (char *)logfile;

    fa.flags = flags;
    fa.buf_size = buf_size;
    ret_value = H5P_set_driver(plist, H5FD_LOG, &fa);

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_fapl_log() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_log_fapl_get
 *
 * Purpose:     Returns a file access property list which indicates how the
 *              specified file is being accessed. The return list could be
 *              used to access another file the same way.
 *
 * Return:      Success:    Ptr to new file access property list with all
 *                          members copied from the file struct.
 *              Failure:    NULL
 *
 * Programmer:  Quincey Koziol
 *              Thursday, April 20, 2000
 *
 *-------------------------------------------------------------------------
 */
static void *
H5FD_log_fapl_get(H5FD_t *_file)
{
    H5FD_log_t  *file = (H5FD_log_t *)_file;
    void        *ret_value;                     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Set return value */
    ret_value = H5FD_log_fapl_copy(&(file->fa));

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_log_fapl_get() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_log_fapl_copy
 *
 * Purpose:     Copies the log-specific file access properties.
 *
 * Return:      Success:    Ptr to a new property list
 *              Failure:    NULL
 *
 * Programmer:  Quincey Koziol
 *              Thursday, April 20, 2000
 *
 *-------------------------------------------------------------------------
 */
static void *
H5FD_log_fapl_copy(const void *_old_fa)
{
    const H5FD_log_fapl_t   *old_fa = (const H5FD_log_fapl_t*)_old_fa;
    H5FD_log_fapl_t         *new_fa = NULL;     /* New FAPL info */
    void                    *ret_value;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

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
 * Function:    H5FD_log_fapl_free
 *
 * Purpose:     Frees the log-specific file access properties.
 *
 * Return:      SUCCEED (Can't fail)
 *
 * Programmer:  Quincey Koziol
 *              Thursday, April 20, 2000
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_log_fapl_free(void *_fa)
{
    H5FD_log_fapl_t	*fa = (H5FD_log_fapl_t*)_fa;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Free the fapl information */
    if(fa->logfile)
        H5MM_xfree(fa->logfile);
    H5MM_xfree(fa);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5FD_log_fapl_free() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_log_open
 *
 * Purpose:     Create and/or opens a file as an HDF5 file.
 *
 * Return:      Success:    A pointer to a new file data structure. The
 *                          public fields will be initialized by the
 *                          caller, which is always H5FD_open().
 *              Failure:    NULL
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 *-------------------------------------------------------------------------
 */
static H5FD_t *
H5FD_log_open(const char *name, unsigned flags, hid_t fapl_id, haddr_t maxaddr)
{
    H5FD_log_t      *file = NULL;
    H5P_genplist_t  *plist;         /* Property list */
    H5FD_log_fapl_t *fa;            /* File access property list information */
    int             fd = -1;        /* File descriptor */
    int             o_flags;        /* Flags for open() call */
#ifdef H5_HAVE_WIN32_API
    struct _BY_HANDLE_FILE_INFORMATION fileinfo;
#endif
#ifdef H5_HAVE_GETTIMEOFDAY
    struct timeval timeval_start;
    struct timeval open_timeval_diff;
    struct timeval stat_timeval_diff;
#endif /* H5_HAVE_GETTIMEOFDAY */
    h5_stat_t       sb;
    H5FD_t          *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check on file offsets */
    HDcompile_assert(sizeof(HDoff_t) >= sizeof(size_t));

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

    /* Get the driver specific information */
    if(NULL == (plist = H5P_object_verify(fapl_id, H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access property list")
    if(NULL == (fa = (H5FD_log_fapl_t *)H5P_get_driver_info(plist)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, NULL, "bad VFL driver info")

#ifdef H5_HAVE_GETTIMEOFDAY
    if(fa->flags & H5FD_LOG_TIME_OPEN)
        HDgettimeofday(&timeval_start, NULL);
#endif /* H5_HAVE_GETTIMEOFDAY */
    /* Open the file */
    if((fd = HDopen(name, o_flags, 0666)) < 0) {
        int myerrno = errno;

        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to open file: name = '%s', errno = %d, error message = '%s', flags = %x, o_flags = %x", name, myerrno, HDstrerror(myerrno), flags, (unsigned)o_flags);
    } /* end if */
#ifdef H5_HAVE_GETTIMEOFDAY
    if(fa->flags & H5FD_LOG_TIME_OPEN) {
        struct timeval timeval_stop;

        HDgettimeofday(&timeval_stop, NULL);

         /* Calculate the elapsed gettimeofday time */
         open_timeval_diff.tv_usec = timeval_stop.tv_usec - timeval_start.tv_usec;
         open_timeval_diff.tv_sec = timeval_stop.tv_sec - timeval_start.tv_sec;
         if(open_timeval_diff.tv_usec < 0) {
             open_timeval_diff.tv_usec += 1000000;
             open_timeval_diff.tv_sec--;
         } /* end if */
    } /* end if */
#endif /* H5_HAVE_GETTIMEOFDAY */

#ifdef H5_HAVE_GETTIMEOFDAY
    if(fa->flags & H5FD_LOG_TIME_STAT)
        HDgettimeofday(&timeval_start, NULL);
#endif /* H5_HAVE_GETTIMEOFDAY */
    /* Get the file stats */
    if(HDfstat(fd, &sb) < 0)
        HSYS_GOTO_ERROR(H5E_FILE, H5E_BADFILE, NULL, "unable to fstat file")
#ifdef H5_HAVE_GETTIMEOFDAY
    if(fa->flags & H5FD_LOG_TIME_STAT) {
        struct timeval timeval_stop;

        HDgettimeofday(&timeval_stop, NULL);

         /* Calculate the elapsed gettimeofday time */
         stat_timeval_diff.tv_usec = timeval_stop.tv_usec - timeval_start.tv_usec;
         stat_timeval_diff.tv_sec = timeval_stop.tv_sec - timeval_start.tv_sec;
         if(stat_timeval_diff.tv_usec < 0) {
             stat_timeval_diff.tv_usec += 1000000;
             stat_timeval_diff.tv_sec--;
         } /* end if */
    } /* end if */
#endif /* H5_HAVE_GETTIMEOFDAY */

    /* Create the new file struct */
    if(NULL == (file = H5FL_CALLOC(H5FD_log_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "unable to allocate file struct")

    file->fd = fd;
    H5_ASSIGN_OVERFLOW(file->eof, sb.st_size, h5_stat_size_t, haddr_t);
    file->pos = HADDR_UNDEF;
    file->op = OP_UNKNOWN;
#ifdef H5_HAVE_WIN32_API
    file->hFile = (HANDLE)_get_osfhandle(fd);
    if(INVALID_HANDLE_VALUE == file->hFile)
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to get Windows file handle")

    if(!GetFileInformationByHandle((HANDLE)file->hFile, &fileinfo))
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to get Windows file information")

    file->nFileIndexHigh = fileinfo.nFileIndexHigh;
    file->nFileIndexLow = fileinfo.nFileIndexLow;
    file->dwVolumeSerialNumber = fileinfo.dwVolumeSerialNumber;
#else /* H5_HAVE_WIN32_API */
    file->device = sb.st_dev;
#ifdef H5_VMS
    file->inode[0] = sb.st_ino[0];
    file->inode[1] = sb.st_ino[1];
    file->inode[2] = sb.st_ino[2];
#else
    file->inode = sb.st_ino;
#endif /*H5_VMS*/

#endif /* H5_HAVE_WIN32_API */

    /* Retain a copy of the name used to open the file, for possible error reporting */
    HDstrncpy(file->filename, name, sizeof(file->filename));
    file->filename[sizeof(file->filename) - 1] = '\0';

    /* Get the flags for logging */
    file->fa.flags = fa->flags;

    /* Check if we are doing any logging at all */
    if(file->fa.flags != 0) {
        /* Allocate buffers for tracking file accesses and data "flavor" */
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

        /* Set the log file pointer */
        if(fa->logfile)
            file->logfp = HDfopen(fa->logfile, "w");
        else
            file->logfp = stderr;

#ifdef H5_HAVE_GETTIMEOFDAY
        if(file->fa.flags & H5FD_LOG_TIME_OPEN)
            HDfprintf(file->logfp, "Open took: (%f s)\n", (double)open_timeval_diff.tv_sec + ((double)open_timeval_diff.tv_usec / (double)1000000.0f));
        if(file->fa.flags & H5FD_LOG_TIME_STAT)
            HDfprintf(file->logfp, "Stat took: (%f s)\n", (double)stat_timeval_diff.tv_sec + ((double)stat_timeval_diff.tv_usec / (double)1000000.0f));
#endif /* H5_HAVE_GETTIMEOFDAY */

    } /* end if */

    /* Check for non-default FAPL */
    if(H5P_FILE_ACCESS_DEFAULT != fapl_id) {
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
    ret_value = (H5FD_t*)file;

done:
    if(NULL == ret_value) {
        if(fd >= 0)
            HDclose(fd);
        if(file)
            file = H5FL_FREE(H5FD_log_t, file);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_log_open() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_log_close
 *
 * Purpose:     Closes an HDF5 file.
 *
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL, file not closed.
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_log_close(H5FD_t *_file)
{
    H5FD_log_t	*file = (H5FD_log_t *)_file;
#ifdef H5_HAVE_GETTIMEOFDAY
    struct timeval timeval_start, timeval_stop;
#endif /* H5_HAVE_GETTIMEOFDAY */
    herr_t ret_value = SUCCEED;                 /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(file);

#ifdef H5_HAVE_GETTIMEOFDAY
    if(file->fa.flags&H5FD_LOG_TIME_CLOSE)
        HDgettimeofday(&timeval_start, NULL);
#endif /* H5_HAVE_GETTIMEOFDAY */
    /* Close the underlying file */
    if(HDclose(file->fd) < 0)
        HSYS_GOTO_ERROR(H5E_IO, H5E_CANTCLOSEFILE, FAIL, "unable to close file")
#ifdef H5_HAVE_GETTIMEOFDAY
    if(file->fa.flags&H5FD_LOG_TIME_CLOSE)
        HDgettimeofday(&timeval_stop, NULL);
#endif /* H5_HAVE_GETTIMEOFDAY */

    /* Dump I/O information */
    if(file->fa.flags != 0) {
        haddr_t addr;
        haddr_t last_addr;
        unsigned char last_val;

#ifdef H5_HAVE_GETTIMEOFDAY
        if(file->fa.flags & H5FD_LOG_TIME_CLOSE) {
            struct timeval timeval_diff;

             /* Calculate the elapsed gettimeofday time */
             timeval_diff.tv_usec = timeval_stop.tv_usec - timeval_start.tv_usec;
             timeval_diff.tv_sec = timeval_stop.tv_sec - timeval_start.tv_sec;
             if(timeval_diff.tv_usec < 0) {
                 timeval_diff.tv_usec += 1000000;
                 timeval_diff.tv_sec--;
             } /* end if */
            HDfprintf(file->logfp, "Close took: (%f s)\n", (double)timeval_diff.tv_sec + ((double)timeval_diff.tv_usec / (double)1000000.0f));
        } /* end if */
#endif /* H5_HAVE_GETTIMEOFDAY */

        /* Dump the total number of seek/read/write operations */
        if(file->fa.flags & H5FD_LOG_NUM_READ)
            HDfprintf(file->logfp, "Total number of read operations: %llu\n", file->total_read_ops);
        if(file->fa.flags & H5FD_LOG_NUM_WRITE)
            HDfprintf(file->logfp, "Total number of write operations: %llu\n", file->total_write_ops);
        if(file->fa.flags & H5FD_LOG_NUM_SEEK)
            HDfprintf(file->logfp, "Total number of seek operations: %llu\n", file->total_seek_ops);
        if(file->fa.flags & H5FD_LOG_NUM_TRUNCATE)
            HDfprintf(file->logfp, "Total number of truncate operations: %llu\n", file->total_truncate_ops);

        /* Dump the total time in seek/read/write */
        if(file->fa.flags & H5FD_LOG_TIME_READ)
            HDfprintf(file->logfp, "Total time in read operations: %f s\n", file->total_read_time);
        if(file->fa.flags & H5FD_LOG_TIME_WRITE)
            HDfprintf(file->logfp, "Total time in write operations: %f s\n", file->total_write_time);
        if(file->fa.flags & H5FD_LOG_TIME_SEEK)
            HDfprintf(file->logfp, "Total time in seek operations: %f s\n", file->total_seek_time);

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
            HDfclose(file->logfp);
    } /* end if */

    /* Release the file info */
    file = H5FL_FREE(H5FD_log_t, file);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_log_close() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_log_cmp
 *
 * Purpose:     Compares two files belonging to this driver using an
 *              arbitrary (but consistent) ordering.
 *
 * Return:      Success:    A value like strcmp()
 *              Failure:    never fails (arguments were checked by the
 *                          caller).
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 *-------------------------------------------------------------------------
 */
static int
H5FD_log_cmp(const H5FD_t *_f1, const H5FD_t *_f2)
{
    const H5FD_log_t    *f1 = (const H5FD_log_t *)_f1;
    const H5FD_log_t    *f2 = (const H5FD_log_t *)_f2;
    int ret_value = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

#ifdef H5_HAVE_WIN32_API
    if(f1->dwVolumeSerialNumber < f2->dwVolumeSerialNumber) HGOTO_DONE(-1)
    if(f1->dwVolumeSerialNumber > f2->dwVolumeSerialNumber) HGOTO_DONE(1)

    if(f1->nFileIndexHigh < f2->nFileIndexHigh) HGOTO_DONE(-1)
    if(f1->nFileIndexHigh > f2->nFileIndexHigh) HGOTO_DONE(1)

    if(f1->nFileIndexLow < f2->nFileIndexLow) HGOTO_DONE(-1)
    if(f1->nFileIndexLow > f2->nFileIndexLow) HGOTO_DONE(1)
#else
#ifdef H5_DEV_T_IS_SCALAR
    if(f1->device < f2->device) HGOTO_DONE(-1)
    if(f1->device > f2->device) HGOTO_DONE(1)
#else /* H5_DEV_T_IS_SCALAR */
    /* If dev_t isn't a scalar value on this system, just use memcmp to
     * determine if the values are the same or not.  The actual return value
     * shouldn't really matter...
     */
    if(HDmemcmp(&(f1->device),&(f2->device),sizeof(dev_t)) < 0) HGOTO_DONE(-1)
    if(HDmemcmp(&(f1->device),&(f2->device),sizeof(dev_t)) > 0) HGOTO_DONE(1)
#endif /* H5_DEV_T_IS_SCALAR */

#ifndef H5_VMS
    if(f1->inode < f2->inode) HGOTO_DONE(-1)
    if(f1->inode > f2->inode) HGOTO_DONE(1)
#else
    if(HDmemcmp(&(f1->inode), &(f2->inode), 3 * sizeof(ino_t)) < 0) HGOTO_DONE(-1)
    if(HDmemcmp(&(f1->inode), &(f2->inode), 3 * sizeof(ino_t)) > 0) HGOTO_DONE(1)
#endif /*H5_VMS*/

#endif

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_log_cmp() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_log_query
 *
 * Purpose:     Set the flags that this VFL driver is capable of supporting.
 *              (listed in H5FDpublic.h)
 *
 * Return:      SUCCEED (Can't fail)
 *
 * Programmer:  Quincey Koziol
 *              Friday, August 25, 2000
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_log_query(const H5FD_t *_file, unsigned long *flags /* out */)
{
    const H5FD_log_t    *file = (const H5FD_log_t *)_file;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Set the VFL feature flags that this driver supports */
    if(flags) {
        *flags = 0;
        *flags |= H5FD_FEAT_AGGREGATE_METADATA; /* OK to aggregate metadata allocations */
        *flags |= H5FD_FEAT_ACCUMULATE_METADATA; /* OK to accumulate metadata for faster writes */
        *flags |= H5FD_FEAT_DATA_SIEVE;       /* OK to perform data sieving for faster raw data reads & writes */
        *flags |= H5FD_FEAT_AGGREGATE_SMALLDATA; /* OK to aggregate "small" raw data allocations */
        *flags |= H5FD_FEAT_POSIX_COMPAT_HANDLE; /* VFD handle is POSIX I/O call compatible */

        /* Check for flags that are set by h5repart */
        if(file && file->fam_to_sec2)
            *flags |= H5FD_FEAT_IGNORE_DRVRINFO; /* Ignore the driver info when file is opened (which eliminates it) */
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5FD_log_query() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_log_alloc
 *
 * Purpose:     Allocate file memory.
 *
 * Return:      Success:    Address of new memory
 *              Failure:    HADDR_UNDEF
 *
 * Programmer:  Quincey Koziol
 *              Monday, April 17, 2000
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_log_alloc(H5FD_t *_file, H5FD_mem_t type, hid_t UNUSED dxpl_id, hsize_t size)
{
    H5FD_log_t	*file = (H5FD_log_t *)_file;
    haddr_t addr;
    haddr_t ret_value;          /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Compute the address for the block to allocate */
    addr = file->eoa;

    /* Check if we need to align this block */
    if(size >= file->pub.threshold) {
        /* Check for an already aligned block */
        if(addr % file->pub.alignment != 0)
            addr = ((addr / file->pub.alignment) + 1) * file->pub.alignment;
    } /* end if */

    file->eoa = addr + size;

    /* Retain the (first) flavor of the information written to the file */
    if(file->fa.flags != 0) {
        if(file->fa.flags & H5FD_LOG_FLAVOR) {
            HDassert(addr < file->iosize);
            H5_CHECK_OVERFLOW(size, hsize_t, size_t);
            HDmemset(&file->flavor[addr], (int)type, (size_t)size);
        } /* end if */

        if(file->fa.flags & H5FD_LOG_ALLOC)
            HDfprintf(file->logfp, "%10a-%10a (%10Hu bytes) (%s) Allocated\n", addr, (addr + size) - 1, size, flavors[type]);
    } /* end if */

    /* Set return value */
    ret_value = addr;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_log_alloc() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_log_get_eoa
 *
 * Purpose:     Gets the end-of-address marker for the file. The EOA marker
 *              is the first address past the last byte allocated in the
 *              format address space.
 *
 * Return:      Success:    The end-of-address marker.
 *              Failure:    HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              Monday, August  2, 1999
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_log_get_eoa(const H5FD_t *_file, H5FD_mem_t UNUSED type)
{
    const H5FD_log_t    *file = (const H5FD_log_t *)_file;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    FUNC_LEAVE_NOAPI(file->eoa)
} /* end H5FD_log_get_eoa() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_log_set_eoa
 *
 * Purpose:     Set the end-of-address marker for the file. This function is
 *              called shortly after an existing HDF5 file is opened in order
 *              to tell the driver where the end of the HDF5 data is located.
 *
 * Return:      SUCCEED (Can't fail)
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_log_set_eoa(H5FD_t *_file, H5FD_mem_t type, haddr_t addr)
{
    H5FD_log_t  *file = (H5FD_log_t *)_file;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(file->fa.flags != 0) {
        if(H5F_addr_gt(addr, file->eoa) && H5F_addr_gt(addr, 0)) {
            hsize_t size = addr - file->eoa;

            /* Retain the flavor of the space allocated by the extension */
            if(file->fa.flags & H5FD_LOG_FLAVOR) {
                HDassert(addr < file->iosize);
                H5_CHECK_OVERFLOW(size, hsize_t, size_t);
                HDmemset(&file->flavor[file->eoa], (int)type, (size_t)size);
            } /* end if */

            /* Log the extension like an allocation */
            if(file->fa.flags & H5FD_LOG_ALLOC)
                HDfprintf(file->logfp, "%10a-%10a (%10Hu bytes) (%s) Allocated\n", file->eoa, addr, size, flavors[type]);
        } /* end if */
    } /* end if */

    file->eoa = addr;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5FD_log_set_eoa() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_log_get_eof
 *
 * Purpose:     Returns the end-of-file marker, which is the greater of
 *              either the filesystem end-of-file or the HDF5 end-of-address
 *              markers.
 *
 * Return:      Success:    End of file address, the first address past
 *                          the end of the "file", either the filesystem file
 *                          or the HDF5 file.
 *              Failure:    HADDR_UNDEF
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_log_get_eof(const H5FD_t *_file)
{
    const H5FD_log_t	*file = (const H5FD_log_t *)_file;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    FUNC_LEAVE_NOAPI(MAX(file->eof, file->eoa))
} /* end H5FD_log_get_eof() */


/*-------------------------------------------------------------------------
 * Function:       H5FD_log_get_handle
 *
 * Purpose:        Returns the file handle of LOG file driver.
 *
 * Returns:        SUCCEED/FAIL
 *
 * Programmer:     Raymond Lu
 *                 Sept. 16, 2002
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_log_get_handle(H5FD_t *_file, hid_t UNUSED fapl, void **file_handle)
{
    H5FD_log_t          *file = (H5FD_log_t *)_file;
    herr_t              ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

    if(!file_handle)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file handle not valid")

    *file_handle = &(file->fd);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_log_get_handle() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_log_read
 *
 * Purpose:     Reads SIZE bytes of data from FILE beginning at address ADDR
 *              into buffer BUF according to data transfer properties in
 *              DXPL_ID.
 *
 * Return:      Success:    SUCCEED. Result is stored in caller-supplied
 *                          buffer BUF.
 *              Failure:    FAIL, Contents of buffer BUF are undefined.
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_log_read(H5FD_t *_file, H5FD_mem_t type, hid_t UNUSED dxpl_id, haddr_t addr,
            size_t size, void *buf/*out*/)
{
    H5FD_log_t          *file = (H5FD_log_t *)_file;
    size_t              orig_size = size; /* Save the original size for later */
    haddr_t             orig_addr = addr;
#ifdef H5_HAVE_GETTIMEOFDAY
    struct timeval      timeval_start, timeval_stop;
#endif /* H5_HAVE_GETTIMEOFDAY */
    herr_t              ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(file && file->pub.cls);
    HDassert(buf);

    /* Check for overflow conditions */
    if(!H5F_addr_defined(addr))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "addr undefined, addr = %llu", (unsigned long long)addr)
    if(REGION_OVERFLOW(addr, size))
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr overflow, addr = %llu", (unsigned long long)addr)
    if((addr + size) > file->eoa)
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr overflow, addr = %llu", (unsigned long long)addr)

    /* Log the I/O information about the read */
    if(file->fa.flags != 0) {
        size_t tmp_size = size;
        haddr_t tmp_addr = addr;

        /* Log information about the number of times these locations are read */
        if(file->fa.flags & H5FD_LOG_FILE_READ) {
            HDassert((addr + size) < file->iosize);
            while(tmp_size-- > 0)
                file->nread[tmp_addr++]++;
        } /* end if */
    } /* end if */

    /* Seek to the correct location */
    if(addr != file->pos || OP_READ != file->op) {
#ifdef H5_HAVE_GETTIMEOFDAY
        if(file->fa.flags & H5FD_LOG_TIME_SEEK)
            HDgettimeofday(&timeval_start, NULL);
#endif /* H5_HAVE_GETTIMEOFDAY */
        if(HDlseek(file->fd, (HDoff_t)addr, SEEK_SET) < 0)
            HSYS_GOTO_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "unable to seek to proper position")
#ifdef H5_HAVE_GETTIMEOFDAY
        if(file->fa.flags & H5FD_LOG_TIME_SEEK)
            HDgettimeofday(&timeval_stop, NULL);
#endif /* H5_HAVE_GETTIMEOFDAY */

        /* Log information about the seek */
        if(file->fa.flags & H5FD_LOG_NUM_SEEK)
            file->total_seek_ops++;
        if(file->fa.flags & H5FD_LOG_LOC_SEEK) {
            HDfprintf(file->logfp, "Seek: From %10a To %10a", file->pos, addr);
#ifdef H5_HAVE_GETTIMEOFDAY
            if(file->fa.flags & H5FD_LOG_TIME_SEEK) {
                struct timeval timeval_diff;
                double time_diff;

                /* Calculate the elapsed gettimeofday time */
                timeval_diff.tv_usec = timeval_stop.tv_usec - timeval_start.tv_usec;
                timeval_diff.tv_sec = timeval_stop.tv_sec - timeval_start.tv_sec;
                if(timeval_diff.tv_usec < 0) {
                    timeval_diff.tv_usec += 1000000;
                    timeval_diff.tv_sec--;
                } /* end if */
                time_diff = (double)timeval_diff.tv_sec + ((double)timeval_diff.tv_usec / (double)1000000.0f);
                HDfprintf(file->logfp, " (%f s)\n", time_diff);

                /* Add to total seek time */
                file->total_seek_time += time_diff;
            } /* end if */
            else
                HDfprintf(file->logfp, "\n");
#else /* H5_HAVE_GETTIMEOFDAY */
            HDfprintf(file->logfp, "\n");
#endif /* H5_HAVE_GETTIMEOFDAY */
        } /* end if */
    } /* end if */

    /*
     * Read data, being careful of interrupted system calls, partial results,
     * and the end of the file.
     */
#ifdef H5_HAVE_GETTIMEOFDAY
    if(file->fa.flags & H5FD_LOG_TIME_READ)
        HDgettimeofday(&timeval_start, NULL);
#endif /* H5_HAVE_GETTIMEOFDAY */
    while(size > 0) {

        h5_posix_io_t       bytes_in        = 0;    /* # of bytes to read       */
        h5_posix_io_ret_t   bytes_read      = -1;   /* # of bytes actually read */ 

        /* Trying to read more bytes than the return type can handle is
         * undefined behavior in POSIX.
         */
        if(size > H5_POSIX_MAX_IO_BYTES)
            bytes_in = H5_POSIX_MAX_IO_BYTES;
        else
            bytes_in = (h5_posix_io_t)size;

        do {
            bytes_read = HDread(file->fd, buf, bytes_in);
        } while(-1 == bytes_read && EINTR == errno);

        if(-1 == bytes_read) { /* error */
            int myerrno = errno;
            time_t mytime = HDtime(NULL);
            HDoff_t myoffset = HDlseek(file->fd, (HDoff_t)0, SEEK_CUR);

            if(file->fa.flags & H5FD_LOG_LOC_READ)
                HDfprintf(file->logfp, "Error! Reading: %10a-%10a (%10Zu bytes)\n", orig_addr, (orig_addr + orig_size) - 1, orig_size);

            HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "file read failed: time = %s, filename = '%s', file descriptor = %d, errno = %d, error message = '%s', buf = %p, total read size = %llu, bytes this sub-read = %llu, bytes actually read = %llu, offset = %llu", HDctime(&mytime), file->filename, file->fd, myerrno, HDstrerror(myerrno), buf, (unsigned long long)size, (unsigned long long)bytes_in, (unsigned long long)bytes_read, (unsigned long long)myoffset);
        } /* end if */

        if(0 == bytes_read) {
            /* end of file but not end of format address space */
            HDmemset(buf, 0, size);
            break;
        } /* end if */

        HDassert(bytes_read >= 0);
        HDassert((size_t)bytes_read <= size);
        
        size -= (size_t)bytes_read;
        addr += (haddr_t)bytes_read;
        buf = (char *)buf + bytes_read;

    } /* end while */
#ifdef H5_HAVE_GETTIMEOFDAY
    if(file->fa.flags & H5FD_LOG_TIME_READ)
        HDgettimeofday(&timeval_stop, NULL);
#endif /* H5_HAVE_GETTIMEOFDAY */

    /* Log information about the read */
    if(file->fa.flags & H5FD_LOG_NUM_READ)
        file->total_read_ops++;
    if(file->fa.flags & H5FD_LOG_LOC_READ) {
        HDfprintf(file->logfp, "%10a-%10a (%10Zu bytes) (%s) Read", orig_addr, (orig_addr + orig_size) - 1, orig_size, flavors[type]);

        /* XXX: Verify the flavor information, if we have it? */

#ifdef H5_HAVE_GETTIMEOFDAY
        if(file->fa.flags & H5FD_LOG_TIME_READ) {
            struct timeval timeval_diff;
            double time_diff;

            /* Calculate the elapsed gettimeofday time */
            timeval_diff.tv_usec = timeval_stop.tv_usec - timeval_start.tv_usec;
            timeval_diff.tv_sec = timeval_stop.tv_sec - timeval_start.tv_sec;
            if(timeval_diff.tv_usec < 0) {
                timeval_diff.tv_usec += 1000000;
                timeval_diff.tv_sec--;
            } /* end if */
            time_diff = (double)timeval_diff.tv_sec + ((double)timeval_diff.tv_usec / (double)1000000.0f);
            HDfprintf(file->logfp, " (%f s)\n", time_diff);

            /* Add to total read time */
            file->total_read_time += time_diff;
        } /* end if */
        else
            HDfprintf(file->logfp, "\n");
#else /* H5_HAVE_GETTIMEOFDAY */
        HDfprintf(file->logfp, "\n");
#endif /* H5_HAVE_GETTIMEOFDAY */
    } /* end if */

    /* Update current position */
    file->pos = addr;
    file->op = OP_READ;

done:
    if(ret_value < 0) {
        /* Reset last file I/O information */
        file->pos = HADDR_UNDEF;
        file->op = OP_UNKNOWN;
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_log_read() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_log_write
 *
 * Purpose:     Writes SIZE bytes of data to FILE beginning at address ADDR
 *              from buffer BUF according to data transfer properties in
 *              DXPL_ID.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_log_write(H5FD_t *_file, H5FD_mem_t type, hid_t UNUSED dxpl_id, haddr_t addr,
            size_t size, const void *buf)
{
    H5FD_log_t          *file = (H5FD_log_t *)_file;
    size_t              orig_size = size; /* Save the original size for later */
    haddr_t             orig_addr = addr;
#ifdef H5_HAVE_GETTIMEOFDAY
    struct timeval      timeval_start, timeval_stop;
#endif /* H5_HAVE_GETTIMEOFDAY */
    herr_t              ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(file && file->pub.cls);
    HDassert(size > 0);
    HDassert(buf);

    /* Verify that we are writing out the type of data we allocated in this location */
    if(file->flavor) {
        HDassert(type == H5FD_MEM_DEFAULT || type == (H5FD_mem_t)file->flavor[addr] || (H5FD_mem_t)file->flavor[addr] == H5FD_MEM_DEFAULT);
        HDassert(type == H5FD_MEM_DEFAULT || type == (H5FD_mem_t)file->flavor[(addr + size) - 1] || (H5FD_mem_t)file->flavor[(addr + size) - 1] == H5FD_MEM_DEFAULT);
    } /* end if */

    /* Check for overflow conditions */
    if(!H5F_addr_defined(addr))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "addr undefined, addr = %llu", (unsigned long long)addr)
    if(REGION_OVERFLOW(addr, size))
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr overflow, addr = %llu, size = %llu", (unsigned long long)addr, (unsigned long long)size)
    if((addr + size) > file->eoa)
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "addr overflow, addr = %llu, size = %llu, eoa = %llu", (unsigned long long)addr, (unsigned long long)size, (unsigned long long)file->eoa)

    /* Log the I/O information about the write */
    if(file->fa.flags & H5FD_LOG_FILE_WRITE) {
        size_t tmp_size = size;
        haddr_t tmp_addr = addr;

        /* Log information about the number of times these locations are read */
        HDassert((addr + size) < file->iosize);
        while(tmp_size-- > 0)
            file->nwrite[tmp_addr++]++;
    } /* end if */

    /* Seek to the correct location */
    if(addr != file->pos || OP_WRITE != file->op) {
#ifdef H5_HAVE_GETTIMEOFDAY
        if(file->fa.flags & H5FD_LOG_TIME_SEEK)
            HDgettimeofday(&timeval_start, NULL);
#endif /* H5_HAVE_GETTIMEOFDAY */
        if(HDlseek(file->fd, (HDoff_t)addr, SEEK_SET) < 0)
            HSYS_GOTO_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "unable to seek to proper position")
#ifdef H5_HAVE_GETTIMEOFDAY
        if(file->fa.flags & H5FD_LOG_TIME_SEEK)
            HDgettimeofday(&timeval_stop, NULL);
#endif /* H5_HAVE_GETTIMEOFDAY */

        /* Log information about the seek */
        if(file->fa.flags & H5FD_LOG_NUM_SEEK)
            file->total_seek_ops++;
        if(file->fa.flags & H5FD_LOG_LOC_SEEK) {
            HDfprintf(file->logfp, "Seek: From %10a To %10a", file->pos, addr);
#ifdef H5_HAVE_GETTIMEOFDAY
            if(file->fa.flags & H5FD_LOG_TIME_SEEK) {
                struct timeval timeval_diff;
                double time_diff;

                /* Calculate the elapsed gettimeofday time */
                timeval_diff.tv_usec = timeval_stop.tv_usec - timeval_start.tv_usec;
                timeval_diff.tv_sec = timeval_stop.tv_sec - timeval_start.tv_sec;
                if(timeval_diff.tv_usec < 0) {
                    timeval_diff.tv_usec += 1000000;
                    timeval_diff.tv_sec--;
                } /* end if */
                time_diff = (double)timeval_diff.tv_sec + ((double)timeval_diff.tv_usec / (double)1000000.0f);
                HDfprintf(file->logfp, " (%f s)\n", time_diff);

                /* Add to total seek time */
                file->total_seek_time += time_diff;
            } /* end if */
            else
                HDfprintf(file->logfp, "\n");
#else /* H5_HAVE_GETTIMEOFDAY */
            HDfprintf(file->logfp, "\n");
#endif /* H5_HAVE_GETTIMEOFDAY */
        } /* end if */
    } /* end if */

    /*
     * Write the data, being careful of interrupted system calls and partial
     * results
     */
#ifdef H5_HAVE_GETTIMEOFDAY
    if(file->fa.flags&H5FD_LOG_TIME_WRITE)
        HDgettimeofday(&timeval_start, NULL);
#endif /* H5_HAVE_GETTIMEOFDAY */
    while(size > 0) {

        h5_posix_io_t       bytes_in        = 0;    /* # of bytes to write  */
        h5_posix_io_ret_t   bytes_wrote     = -1;   /* # of bytes written   */ 

        /* Trying to write more bytes than the return type can handle is
         * undefined behavior in POSIX.
         */
        if(size > H5_POSIX_MAX_IO_BYTES)
            bytes_in = H5_POSIX_MAX_IO_BYTES;
        else
            bytes_in = (h5_posix_io_t)size;

        do {
            bytes_wrote = HDwrite(file->fd, buf, bytes_in);
        } while(-1 == bytes_wrote && EINTR == errno);

        if(-1 == bytes_wrote) { /* error */
            int myerrno = errno;
            time_t mytime = HDtime(NULL);
            HDoff_t myoffset = HDlseek(file->fd, (HDoff_t)0, SEEK_CUR);

            if(file->fa.flags & H5FD_LOG_LOC_WRITE)
                HDfprintf(file->logfp, "Error! Writing: %10a-%10a (%10Zu bytes)\n", orig_addr, (orig_addr + orig_size) - 1, orig_size);

            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "file write failed: time = %s, filename = '%s', file descriptor = %d, errno = %d, error message = '%s', buf = %p, total write size = %llu, bytes this sub-write = %llu, bytes actually written = %llu, offset = %llu", HDctime(&mytime), file->filename, file->fd, myerrno, HDstrerror(myerrno), buf, (unsigned long long)size, (unsigned long long)bytes_in, (unsigned long long)bytes_wrote, (unsigned long long)myoffset);
        } /* end if */

        HDassert(bytes_wrote > 0);
        HDassert((size_t)bytes_wrote <= size);

        size -= (size_t)bytes_wrote;
        addr += (haddr_t)bytes_wrote;
        buf = (const char *)buf + bytes_wrote;
    } /* end while */
#ifdef H5_HAVE_GETTIMEOFDAY
    if(file->fa.flags & H5FD_LOG_TIME_WRITE)
        HDgettimeofday(&timeval_stop, NULL);
#endif /* H5_HAVE_GETTIMEOFDAY */

    /* Log information about the write */
    if(file->fa.flags & H5FD_LOG_NUM_WRITE)
        file->total_write_ops++;
    if(file->fa.flags & H5FD_LOG_LOC_WRITE) {
        HDfprintf(file->logfp, "%10a-%10a (%10Zu bytes) (%s) Written", orig_addr, (orig_addr + orig_size) - 1, orig_size, flavors[type]);

        /* Check if this is the first write into a "default" section, grabbed by the metadata agregation algorithm */
        if(file->fa.flags & H5FD_LOG_FLAVOR) {
            if((H5FD_mem_t)file->flavor[orig_addr] == H5FD_MEM_DEFAULT)
                HDmemset(&file->flavor[orig_addr], (int)type, orig_size);
        } /* end if */

#ifdef H5_HAVE_GETTIMEOFDAY
        if(file->fa.flags & H5FD_LOG_TIME_WRITE) {
            struct timeval timeval_diff;
            double time_diff;

            /* Calculate the elapsed gettimeofday time */
            timeval_diff.tv_usec = timeval_stop.tv_usec - timeval_start.tv_usec;
            timeval_diff.tv_sec = timeval_stop.tv_sec - timeval_start.tv_sec;
            if(timeval_diff.tv_usec < 0) {
                timeval_diff.tv_usec += 1000000;
                timeval_diff.tv_sec--;
            } /* end if */
            time_diff = (double)timeval_diff.tv_sec + ((double)timeval_diff.tv_usec / (double)1000000.0f);
            HDfprintf(file->logfp, " (%f s)\n", time_diff);

            /* Add to total write time */
            file->total_write_time += time_diff;
        } /* end if */
        else
            HDfprintf(file->logfp, "\n");
#else /* H5_HAVE_GETTIMEOFDAY */
        HDfprintf(file->logfp, "\n");
#endif /* H5_HAVE_GETTIMEOFDAY */
    } /* end if */

    /* Update current position and eof */
    file->pos = addr;
    file->op = OP_WRITE;
    if(file->pos > file->eof)
        file->eof = file->pos;

done:
    if(ret_value < 0) {
        /* Reset last file I/O information */
        file->pos = HADDR_UNDEF;
        file->op = OP_UNKNOWN;
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_log_write() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_log_truncate
 *
 * Purpose:     Makes sure that the true file size is the same (or larger)
 *              than the end-of-address.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, August  4, 1999
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_log_truncate(H5FD_t *_file, hid_t UNUSED dxpl_id, hbool_t UNUSED closing)
{
    H5FD_log_t  *file = (H5FD_log_t *)_file;
    herr_t      ret_value = SUCCEED;                /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(file);

    /* Extend the file to make sure it's large enough */
    if(!H5F_addr_eq(file->eoa, file->eof)) {
#ifdef H5_HAVE_WIN32_API
        LARGE_INTEGER   li;         /* 64-bit (union) integer for SetFilePointer() call */
        DWORD           dwPtrLow;   /* Low-order pointer bits from SetFilePointer()
                                     * Only used as an error code here.
                                     */
        DWORD           dwError;    /* DWORD error code from GetLastError() */
        BOOL            bError;     /* Boolean error flag */

        /* Windows uses this odd QuadPart union for 32/64-bit portability */
        li.QuadPart = (__int64)file->eoa;

        /* Extend the file to make sure it's large enough.
         *
         * Since INVALID_SET_FILE_POINTER can technically be a valid return value
         * from SetFilePointer(), we also need to check GetLastError().
         */
        dwPtrLow = SetFilePointer(file->hFile, li.LowPart, &li.HighPart, FILE_BEGIN);
        if(INVALID_SET_FILE_POINTER == dwPtrLow) {
            dwError = GetLastError();
            if(dwError != NO_ERROR )
                HGOTO_ERROR(H5E_FILE, H5E_FILEOPEN, FAIL, "unable to set file pointer")
        }

        bError = SetEndOfFile(file->hFile);
        if(0 == bError)
            HGOTO_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "unable to extend file properly")
#else /* H5_HAVE_WIN32_API */
#ifdef H5_VMS
        /* Reset seek offset to the beginning of the file, so that the file isn't
         * re-extended later.  This may happen on Open VMS. */
        if(-1 == HDlseek(file->fd, (HDoff_t)0, SEEK_SET))
            HSYS_GOTO_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "unable to seek to proper position")
#endif

        if(-1 == HDftruncate(file->fd, (HDoff_t)file->eoa))
            HSYS_GOTO_ERROR(H5E_IO, H5E_SEEKERROR, FAIL, "unable to extend file properly")
#endif /* H5_HAVE_WIN32_API */

        /* Log information about the truncate */
        if(file->fa.flags & H5FD_LOG_NUM_TRUNCATE)
            file->total_truncate_ops++;

        /* Update the eof value */
        file->eof = file->eoa;

        /* Reset last file I/O information */
        file->pos = HADDR_UNDEF;
        file->op = OP_UNKNOWN;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD_log_truncate() */
