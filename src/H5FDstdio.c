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
 * Programmer:	Robb Matzke <matzke@llnl.gov>
 *	      	Wednesday, October 22, 1997
 *
 * Purpose:   	This is the Posix stdio.h I/O subclass of H5Flow.
 *		It also serves as an example of coding a simple file driver,
 *		therefore, it should not use any non-public definitions.
 *
 * Notes:  Ported to the new H5FD architecture on 10/18/99 - QAK
 *
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Disable certain warnings in PC-Lint: */
/*lint --emacro( {534, 830}, H5P_FILE_ACCESS) */
/*lint --emacro( {534, 830}, H5F_ACC_RDWR, H5F_ACC_EXCL) */
/*lint -esym( 534, H5Eclear2, H5Epush2) */

#include "hdf5.h"

#ifdef H5_HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef H5_HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <io.h>

/* This is not defined in the Windows header files */
#ifndef F_OK
#define F_OK 00
#endif

#endif


#ifdef MAX
#undef MAX
#endif /* MAX */
#define MAX(X,Y)	((X)>(Y)?(X):(Y))

/* The driver identification number, initialized at runtime */
static hid_t H5FD_STDIO_g = 0;

/* File operations */
typedef enum {
    H5FD_STDIO_OP_UNKNOWN=0,
    H5FD_STDIO_OP_READ=1,
    H5FD_STDIO_OP_WRITE=2,
    H5FD_STDIO_OP_SEEK=3
} H5FD_stdio_file_op;

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
typedef struct H5FD_stdio_t {
    H5FD_t	pub;			/*public stuff, must be first	*/
    FILE *	fp;			    /*the file handle */
    haddr_t	eoa;			/*end of allocated region	*/
    haddr_t	eof;			/*end of file; current file size*/
    haddr_t	pos;			/*current file I/O position	*/
    H5FD_stdio_file_op op;	/*last operation		*/
    unsigned write_access;  /* Flag to indicate the file was opened with write access */
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
} H5FD_stdio_t;

#ifdef H5_HAVE_LSEEK64
#   define file_offset_t	off64_t
#   define file_truncate	ftruncate64
#elif defined (_WIN32) && !defined(__MWERKS__)
# /*MSVC*/
#   define file_offset_t __int64
#   define file_truncate	_chsize
#else
#   define file_offset_t	off_t
#   define file_truncate	ftruncate
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
/* adding for windows NT filesystem support. */
#define MAXADDR (((haddr_t)1<<(8*sizeof(file_offset_t)-1))-1)
#define ADDR_OVERFLOW(A)	(HADDR_UNDEF==(A) || ((A) & ~(haddr_t)MAXADDR))
#define SIZE_OVERFLOW(Z)	((Z) & ~(hsize_t)MAXADDR)
#define REGION_OVERFLOW(A,Z)	(ADDR_OVERFLOW(A) || SIZE_OVERFLOW(Z) || \
    HADDR_UNDEF==(A)+(Z) || (file_offset_t)((A)+(Z))<(file_offset_t)(A))

#ifndef H5_HAVE_FSEEKO
/* Define big file as 2GB */
#define BIG_FILE 0x80000000UL
#endif

/* Prototypes */
static H5FD_t *H5FD_stdio_open(const char *name, unsigned flags,
                 hid_t fapl_id, haddr_t maxaddr);
static herr_t H5FD_stdio_close(H5FD_t *lf);
static int H5FD_stdio_cmp(const H5FD_t *_f1, const H5FD_t *_f2);
static herr_t H5FD_stdio_query(const H5FD_t *_f1, unsigned long *flags);
static haddr_t H5FD_stdio_alloc(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id, hsize_t size);
static haddr_t H5FD_stdio_get_eoa(const H5FD_t *_file, H5FD_mem_t type);
static herr_t H5FD_stdio_set_eoa(H5FD_t *_file, H5FD_mem_t type, haddr_t addr);
static haddr_t H5FD_stdio_get_eof(const H5FD_t *_file);
static herr_t  H5FD_stdio_get_handle(H5FD_t *_file, hid_t fapl, void** file_handle);
static herr_t H5FD_stdio_read(H5FD_t *lf, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
                size_t size, void *buf);
static herr_t H5FD_stdio_write(H5FD_t *lf, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
                size_t size, const void *buf);
static herr_t H5FD_stdio_flush(H5FD_t *_file, hid_t dxpl_id, unsigned closing);
static herr_t H5FD_stdio_truncate(H5FD_t *_file, hid_t dxpl_id, hbool_t closing);

static const H5FD_class_t H5FD_stdio_g = {
    "stdio",				        /*name			*/
    MAXADDR,				        /*maxaddr		*/
    H5F_CLOSE_WEAK,				/* fc_degree		*/
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
    H5FD_stdio_open,		                /*open			*/
    H5FD_stdio_close,		                /*close			*/
    H5FD_stdio_cmp,			        /*cmp			*/
    H5FD_stdio_query,		                /*query			*/
    NULL,					/*get_type_map		*/
    H5FD_stdio_alloc,				/*alloc			*/
    NULL,					/*free			*/
    H5FD_stdio_get_eoa,		                /*get_eoa		*/
    H5FD_stdio_set_eoa, 	                /*set_eoa		*/
    H5FD_stdio_get_eof,		                /*get_eof		*/
    H5FD_stdio_get_handle,                      /*get_handle            */
    H5FD_stdio_read,		                /*read			*/
    H5FD_stdio_write,		                /*write			*/
    H5FD_stdio_flush,		                /*flush			*/
    H5FD_stdio_truncate,			/*truncate		*/
    NULL,                                       /*lock                  */
    NULL,                                       /*unlock                */
    H5FD_FLMAP_SINGLE 		                /*fl_map		*/
};


/*-------------------------------------------------------------------------
 * Function:	H5FD_stdio_init
 *
 * Purpose:	Initialize this driver by registering the driver with the
 *		library.
 *
 * Return:	Success:	The driver ID for the stdio driver.
 *
 *		Failure:	Negative.
 *
 * Programmer:	Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *      Stolen from the sec2 driver - QAK, 10/18/99
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FD_stdio_init(void)
{
    /* Clear the error stack */
    H5Eclear2(H5E_DEFAULT);

    if (H5I_VFL!=H5Iget_type(H5FD_STDIO_g))
        H5FD_STDIO_g = H5FDregister(&H5FD_stdio_g);
    return(H5FD_STDIO_g);
}


/*---------------------------------------------------------------------------
 * Function:	H5FD_stdio_term
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
H5FD_stdio_term(void)
{
    /* Reset VFL ID */
    H5FD_STDIO_g=0;

} /* end H5FD_stdio_term() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_fapl_stdio
 *
 * Purpose:	Modify the file access property list to use the H5FD_STDIO
 *		driver defined in this source file.  There are no driver
 *		specific properties.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Thursday, February 19, 1998
 *
 * Modifications:
 *      Stolen from the sec2 driver - QAK, 10/18/99
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fapl_stdio(hid_t fapl_id)
{
    static const char *func="H5FDset_fapl_stdio";  /*for error reporting*/

    /*NO TRACE*/

    /* Clear the error stack */
    H5Eclear2(H5E_DEFAULT);

    if(0 == H5Pisa_class(fapl_id, H5P_FILE_ACCESS))
        H5Epush_ret(func, H5E_ERR_CLS, H5E_PLIST, H5E_BADTYPE, "not a file access property list", -1)

    return H5Pset_driver(fapl_id, H5FD_STDIO, NULL);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_stdio_open
 *
 * Purpose:	Create and/or opens a Standard C file as an HDF5 file.
 *
 * Bugs:	H5F_ACC_EXCL has a race condition. (? -QAK)
 *
 * Errors:
 *		IO	  CANTOPENFILE	File doesn't exist and CREAT wasn't
 *					specified.
 *		IO	  CANTOPENFILE	Fopen failed.
 *		IO	  FILEEXISTS	File exists but CREAT and EXCL were
 *					specified.
 *
 * Return:	Success:	A pointer to a new file data structure. The
 *				public fields will be initialized by the
 *				caller, which is always H5FD_open().
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *		Wednesday, October 22, 1997
 *
 * Modifications:
 *      Ported to VFL/H5FD layer - QAK, 10/18/99
 *
 *-------------------------------------------------------------------------
 */
static H5FD_t *
H5FD_stdio_open( const char *name, unsigned flags, hid_t fapl_id,
    haddr_t maxaddr)
{
    FILE		   *f = NULL;
    unsigned    write_access=0;     /* File opened with write access? */
    H5FD_stdio_t	*file=NULL;
    static const char *func="H5FD_stdio_open";  /* Function Name for error reporting */
#ifdef _WIN32
	HFILE filehandle;
	struct _BY_HANDLE_FILE_INFORMATION fileinfo;
        int fd;
#else /* _WIN32 */
    struct stat		    sb;
#endif  /* _WIN32 */

    /* Sanity check on file offsets */
    assert(sizeof(file_offset_t)>=sizeof(size_t));

    /* Shut compiler up */
    fapl_id=fapl_id;

    /* Clear the error stack */
    H5Eclear2(H5E_DEFAULT);

    /* Check arguments */
    if (!name || !*name)
        H5Epush_ret(func, H5E_ERR_CLS, H5E_ARGS, H5E_BADVALUE, "invalid file name", NULL)
    if (0==maxaddr || HADDR_UNDEF==maxaddr)
        H5Epush_ret(func, H5E_ERR_CLS, H5E_ARGS, H5E_BADRANGE, "bogus maxaddr", NULL)
    if (ADDR_OVERFLOW(maxaddr))
        H5Epush_ret(func, H5E_ERR_CLS, H5E_ARGS, H5E_OVERFLOW, "maxaddr too large", NULL)

    if (access(name, F_OK) < 0) {
        if ((flags & H5F_ACC_CREAT) && (flags & H5F_ACC_RDWR)) {
            f = fopen(name, "wb+");
            write_access=1;     /* Note the write access */
        }
        else
            H5Epush_ret(func, H5E_ERR_CLS, H5E_IO, H5E_CANTOPENFILE, "file doesn't exist and CREAT wasn't specified", NULL)
    } else if ((flags & H5F_ACC_CREAT) && (flags & H5F_ACC_EXCL)) {
        H5Epush_ret(func, H5E_ERR_CLS, H5E_IO, H5E_FILEEXISTS, "file exists but CREAT and EXCL were specified", NULL)
    } else if (flags & H5F_ACC_RDWR) {
        if (flags & H5F_ACC_TRUNC)
            f = fopen(name, "wb+");
        else
            f = fopen(name, "rb+");
        write_access=1;     /* Note the write access */
    } else {
        f = fopen(name, "rb");
    }
    if (!f)
        H5Epush_ret(func, H5E_ERR_CLS, H5E_IO, H5E_CANTOPENFILE, "fopen failed", NULL)

    /* Build the return value */
    if(NULL == (file = (H5FD_stdio_t *)calloc((size_t)1, sizeof(H5FD_stdio_t)))) {
        fclose(f);
        H5Epush_ret(func, H5E_ERR_CLS, H5E_RESOURCE, H5E_NOSPACE, "memory allocation failed", NULL)
    } /* end if */
    file->fp = f;
    file->op = H5FD_STDIO_OP_SEEK;
    file->pos = HADDR_UNDEF;
    file->write_access=write_access;    /* Note the write_access for later */
#ifdef H5_HAVE_FSEEKO
    if(fseeko(file->fp, (off_t)0, SEEK_END) < 0) {
#else
    if(fseek(file->fp, (long)0L, SEEK_END) < 0) {
#endif
        file->op = H5FD_STDIO_OP_UNKNOWN;
    } else {
#ifdef H5_HAVE_FTELLO
        off_t x = ftello (file->fp);
#else
        long x = ftell (file->fp);
#endif
        assert (x>=0);
        file->eof = (haddr_t)x;
    }

    /* The unique key */
#ifdef _WIN32
/*#error "Needs correct fileindexhi & fileindexlo, code below is from sec2 driver"*/
    fd = _fileno(f);
    filehandle = _get_osfhandle(fd);
    (void)GetFileInformationByHandle((HANDLE)filehandle, &fileinfo);
    file->fileindexhi = fileinfo.nFileIndexHigh;
    file->fileindexlo = fileinfo.nFileIndexLow;
#else
    fstat(fileno(file->fp), &sb);
    file->device = sb.st_dev;
    file->inode = sb.st_ino;
#endif
    return((H5FD_t*)file);
}   /* end H5FD_stdio_open() */


/*-------------------------------------------------------------------------
 * Function:	H5F_stdio_close
 *
 * Purpose:	Closes a file.
 *
 * Errors:
 *		IO	  CLOSEERROR	Fclose failed.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Wednesday, October 22, 1997
 *
 * Modifications:
 *      Ported to VFL/H5FD layer - QAK, 10/18/99
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_stdio_close(H5FD_t *_file)
{
    H5FD_stdio_t	*file = (H5FD_stdio_t*)_file;
    static const char *func="H5FD_stdio_close";  /* Function Name for error reporting */

    /* Clear the error stack */
    H5Eclear2(H5E_DEFAULT);

    if (fclose(file->fp) < 0)
        H5Epush_ret(func, H5E_ERR_CLS, H5E_IO, H5E_CLOSEERROR, "fclose failed", -1)

    free(file);

    return(0);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_stdio_cmp
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
 *      Stolen from the sec2 driver - QAK, 10/18/99
 *
 *-------------------------------------------------------------------------
 */
static int
H5FD_stdio_cmp(const H5FD_t *_f1, const H5FD_t *_f2)
{
    const H5FD_stdio_t	*f1 = (const H5FD_stdio_t*)_f1;
    const H5FD_stdio_t	*f2 = (const H5FD_stdio_t*)_f2;

    /* Clear the error stack */
    H5Eclear2(H5E_DEFAULT);

#ifdef _WIN32
    if (f1->fileindexhi < f2->fileindexhi) return -1;
    if (f1->fileindexhi > f2->fileindexhi) return 1;

    if (f1->fileindexlo < f2->fileindexlo) return -1;
    if (f1->fileindexlo > f2->fileindexlo) return 1;

#else
#ifdef H5_DEV_T_IS_SCALAR
    if (f1->device < f2->device) return -1;
    if (f1->device > f2->device) return 1;
#else /* H5_DEV_T_IS_SCALAR */
    /* If dev_t isn't a scalar value on this system, just use memcmp to
     * determine if the values are the same or not.  The actual return value
     * shouldn't really matter...
     */
    if(memcmp(&(f1->device),&(f2->device),sizeof(dev_t))<0) return -1;
    if(memcmp(&(f1->device),&(f2->device),sizeof(dev_t))>0) return 1;
#endif /* H5_DEV_T_IS_SCALAR */

    if (f1->inode < f2->inode) return -1;
    if (f1->inode > f2->inode) return 1;
#endif
    return 0;
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_stdio_query
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
static herr_t
H5FD_stdio_query(const H5FD_t *_f, unsigned long *flags /* out */)
{
    /* Shut compiler up */
    _f=_f;

    /* Set the VFL feature flags that this driver supports */
    if(flags) {
        *flags = 0;
        *flags|=H5FD_FEAT_AGGREGATE_METADATA; /* OK to aggregate metadata allocations */
        *flags|=H5FD_FEAT_ACCUMULATE_METADATA; /* OK to accumulate metadata for faster writes */
        *flags|=H5FD_FEAT_DATA_SIEVE;       /* OK to perform data sieving for faster raw data reads & writes */
        *flags|=H5FD_FEAT_AGGREGATE_SMALLDATA; /* OK to aggregate "small" raw data allocations */
    }

    return(0);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_stdio_alloc
 *
 * Purpose:	Allocates file memory. If fseeko isn't available, makes
 *              sure the file size isn't bigger than 2GB because the
 *              parameter OFFSET of fseek is of the type LONG INT, limiting
 *              the file size to 2GB.
 *
 * Return:	Success:	Address of new memory
 *
 *		Failure:	HADDR_UNDEF
 *
 * Programmer:	Raymond Lu
 *              30 March 2007
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_stdio_alloc(H5FD_t *_file, H5FD_mem_t /*UNUSED*/ type, hid_t /*UNUSED*/ dxpl_id, hsize_t size)
{
    H5FD_stdio_t	*file = (H5FD_stdio_t*)_file;
    haddr_t		addr;
#ifndef H5_HAVE_FSEEKO
    static const char   *func = "H5FD_stdio_alloc";  /* Function Name for error reporting */
#endif
    haddr_t ret_value;          /* Return value */

    /* Shut compiler up */
    type = type;
    dxpl_id = dxpl_id;

    /* Clear the error stack */
    H5Eclear2(H5E_DEFAULT);

    /* Compute the address for the block to allocate */
    addr = file->eoa;

    /* Check if we need to align this block */
    if(size >= file->pub.threshold) {
        /* Check for an already aligned block */
        if((addr % file->pub.alignment) != 0)
            addr = ((addr / file->pub.alignment) + 1) * file->pub.alignment;
    } /* end if */

#ifndef H5_HAVE_FSEEKO
    /* If fseeko isn't available, big files (>2GB) won't be supported. */
    if((addr + size) > BIG_FILE)
        H5Epush_ret(func, H5E_ERR_CLS, H5E_IO, H5E_SEEKERROR, "can't write file bigger than 2GB because fseeko isn't available", HADDR_UNDEF)
#endif

    file->eoa = addr + size;

    /* Set return value */
    ret_value = addr;

    return(ret_value);
}   /* H5FD_stdio_alloc() */


/*-------------------------------------------------------------------------
 * Function:	H5FD_stdio_get_eoa
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
 *              Stolen from the sec2 driver - QAK, 10/18/99
 *
 *              Raymond Lu
 *              21 Dec. 2006
 *              Added the parameter TYPE.  It's only used for MULTI driver.
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_stdio_get_eoa(const H5FD_t *_file, H5FD_mem_t /*unused*/ type)
{
    const H5FD_stdio_t	*file = (const H5FD_stdio_t *)_file;

    /* Clear the error stack */
    H5Eclear2(H5E_DEFAULT);

    /* Shut compiler up */
    type = type;

    return(file->eoa);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_stdio_set_eoa
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
 *              Stolen from the sec2 driver - QAK, 10/18/99
 *
 *              Raymond Lu
 *              21 Dec. 2006
 *              Added the parameter TYPE.  It's only used for MULTI driver.
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_stdio_set_eoa(H5FD_t *_file, H5FD_mem_t /*unused*/ type, haddr_t addr)
{
    H5FD_stdio_t	*file = (H5FD_stdio_t*)_file;

    /* Clear the error stack */
    H5Eclear2(H5E_DEFAULT);

    /* Shut compiler up */
    type = type;

    file->eoa = addr;

    return(0);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_stdio_get_eof
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
 *      Stolen from the sec2 driver - QAK, 10/18/99
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_stdio_get_eof(const H5FD_t *_file)
{
    const H5FD_stdio_t	*file = (const H5FD_stdio_t *)_file;

    /* Clear the error stack */
    H5Eclear2(H5E_DEFAULT);

    return(MAX(file->eof, file->eoa));
}


/*-------------------------------------------------------------------------
 * Function:       H5FD_stdio_get_handle
 *
 * Purpose:        Returns the file handle of stdio file driver.
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
static herr_t
H5FD_stdio_get_handle(H5FD_t *_file, hid_t fapl, void** file_handle)
{
    H5FD_stdio_t       *file = (H5FD_stdio_t *)_file;
    static const char  *func="H5FD_stdio_get_handle";  /* Function Name for error reporting */

    /* Shut compiler up */
    fapl=fapl;

    /* Clear the error stack */
    H5Eclear2(H5E_DEFAULT);

    *file_handle = &(file->fp);
    if(*file_handle==NULL)
        H5Epush_ret(func, H5E_ERR_CLS, H5E_IO, H5E_WRITEERROR, "get handle failed", -1)
    return(0);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_stdio_read
 *
 * Purpose:	Reads SIZE bytes beginning at address ADDR in file LF and
 *		places them in buffer BUF.  Reading past the logical or
 *		physical end of file returns zeros instead of failing.
 *
 * Errors:
 *		IO	  READERROR	Fread failed.
 *		IO	  SEEKERROR	Fseek failed.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Wednesday, October 22, 1997
 *
 * Modifications:
 *		June 2, 1998	Albert Cheng
 *		Added xfer_mode argument
 *
 *      Ported to VFL/H5FD layer - QAK, 10/18/99
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_stdio_read(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, size_t size,
    void *buf/*out*/)
{
    size_t		n;
    H5FD_stdio_t		*file = (H5FD_stdio_t*)_file;
    static const char *func="H5FD_stdio_read";  /* Function Name for error reporting */

    /* Shut compiler up */
    type=type;
    dxpl_id=dxpl_id;

    /* Clear the error stack */
    H5Eclear2(H5E_DEFAULT);

    /* Check for overflow */
    if (HADDR_UNDEF==addr)
        H5Epush_ret (func, H5E_ERR_CLS, H5E_IO, H5E_OVERFLOW, "file address overflowed", -1)
    if (REGION_OVERFLOW(addr, size))
        H5Epush_ret (func, H5E_ERR_CLS, H5E_IO, H5E_OVERFLOW, "file address overflowed", -1)
    if((addr + size) > file->eoa)
        H5Epush_ret(func, H5E_ERR_CLS, H5E_IO, H5E_OVERFLOW, "file address overflowed", -1)

    /* Check easy cases */
    if (0 == size)
        return(0);
    if ((haddr_t)addr >= file->eof) {
        memset(buf, 0, size);
        return(0);
    }

    /*
     * Seek to the correct file position.
     */
    if (!(file->op == H5FD_STDIO_OP_READ || file->op==H5FD_STDIO_OP_SEEK) ||
            file->pos != addr) {
#ifdef H5_HAVE_FSEEKO
        if (fseeko(file->fp, (off_t)addr, SEEK_SET) < 0) {
#else
        if (fseek(file->fp, (long)addr, SEEK_SET) < 0) {
#endif
            file->op = H5FD_STDIO_OP_UNKNOWN;
            file->pos = HADDR_UNDEF;
            H5Epush_ret(func, H5E_ERR_CLS, H5E_IO, H5E_SEEKERROR, "fseek failed", -1)
        }
        file->pos = addr;
    }

    /*
     * Read zeros past the logical end of file (physical is handled below)
     */
    if (addr + size > file->eof) {
        size_t nbytes = (size_t) (addr + size - file->eof);
        memset((unsigned char *)buf + size - nbytes, 0, nbytes);
        size -= nbytes;
    }

    /*
     * Read the data.  Since we're reading single-byte values, a partial read
     * will advance the file position by N.  If N is zero or an error
     * occurs then the file position is undefined.
     */
    n = fread(buf, (size_t)1, size, file->fp);
    if(n == 0 && ferror(file->fp)) {
        file->op = H5FD_STDIO_OP_UNKNOWN;
        file->pos = HADDR_UNDEF;
        H5Epush_ret(func, H5E_ERR_CLS, H5E_IO, H5E_READERROR, "fread failed", -1)
    } else if (n < size) {
        memset((unsigned char *)buf + n, 0, (size - n));
    }

    /*
     * Update the file position data.
     */
    file->op = H5FD_STDIO_OP_READ;
    file->pos = addr+n; /*checked for overflow above*/
    return(0);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_stdio_write
 *
 * Purpose:	Writes SIZE bytes from the beginning of BUF into file LF at
 *		file address ADDR.
 *
 * Errors:
 *		IO	  SEEKERROR	Fseek failed.
 *		IO	  WRITEERROR	Fwrite failed.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Wednesday, October 22, 1997
 *
 * Modifications:
 *		June 2, 1998	Albert Cheng
 *		Added xfer_mode argument
 *
 *      Ported to VFL/H5FD layer - QAK, 10/18/99
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_stdio_write(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr,
		size_t size, const void *buf)
{
    H5FD_stdio_t		*file = (H5FD_stdio_t*)_file;
    static const char *func="H5FD_stdio_write";  /* Function Name for error reporting */

    /* Shut compiler up */
    dxpl_id=dxpl_id;
    type=type;

    /* Clear the error stack */
    H5Eclear2(H5E_DEFAULT);

    /* Check for overflow conditions */
    if (HADDR_UNDEF==addr)
        H5Epush_ret (func, H5E_ERR_CLS, H5E_IO, H5E_OVERFLOW, "file address overflowed", -1)
    if (REGION_OVERFLOW(addr, size))
        H5Epush_ret (func, H5E_ERR_CLS, H5E_IO, H5E_OVERFLOW, "file address overflowed", -1)
    if (addr+size>file->eoa)
        H5Epush_ret (func, H5E_ERR_CLS, H5E_IO, H5E_OVERFLOW, "file address overflowed", -1)

    /*
     * Seek to the correct file position.
     */
    if ((file->op != H5FD_STDIO_OP_WRITE && file->op != H5FD_STDIO_OP_SEEK) ||
                file->pos != addr) {
#ifdef H5_HAVE_FSEEKO
        if (fseeko(file->fp, (off_t)addr, SEEK_SET) < 0) {
#else
        if (fseek(file->fp, (long)addr, SEEK_SET) < 0) {
#endif
            file->op = H5FD_STDIO_OP_UNKNOWN;
            file->pos = HADDR_UNDEF;
            H5Epush_ret(func, H5E_ERR_CLS, H5E_IO, H5E_SEEKERROR, "fseek failed", -1)
        }
        file->pos = addr;
    }

    /*
     * Write the buffer.  On successful return, the file position will be
     * advanced by the number of bytes read.  Otherwise nobody knows where it
     * is.
     */
    if(size != fwrite(buf, (size_t)1, size, file->fp)) {
        file->op = H5FD_STDIO_OP_UNKNOWN;
        file->pos = HADDR_UNDEF;
        H5Epush_ret(func, H5E_ERR_CLS, H5E_IO, H5E_WRITEERROR, "fwrite failed", -1)
    }

    /*
     * Update seek optimizing data.
     */
    file->op = H5FD_STDIO_OP_WRITE;
    file->pos = addr + size;

    /* Update EOF if necessary */
    if (file->pos>file->eof)
        file->eof = file->pos;

    return(0);
}


/*-------------------------------------------------------------------------
 * Function:	H5F_stdio_flush
 *
 * Purpose:	Makes sure that all data is on disk.
 *
 * Errors:
 *		IO	  SEEKERROR     fseek failed.
 *		IO	  WRITEERROR    fflush or fwrite failed.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Wednesday, October 22, 1997
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_stdio_flush(H5FD_t *_file, hid_t dxpl_id, unsigned closing)
{
    H5FD_stdio_t	*file = (H5FD_stdio_t*)_file;
    static const char *func = "H5FD_stdio_flush";  /* Function Name for error reporting */

    /* Shut compiler up */
    dxpl_id = dxpl_id;

    /* Clear the error stack */
    H5Eclear2(H5E_DEFAULT);

    /* Only try to flush the file if we have write access */
    if(file->write_access) {
        /* Flush */
        if(!closing) {
            if(fflush(file->fp) < 0)
                H5Epush_ret(func, H5E_ERR_CLS, H5E_IO, H5E_WRITEERROR, "fflush failed", -1)

            /* Reset last file I/O information */
            file->pos = HADDR_UNDEF;
            file->op = H5FD_STDIO_OP_UNKNOWN;
        } /* end if */
    } /* end if */

    return(0);
} /* end H5FD_stdio_flush() */


/*-------------------------------------------------------------------------
 * Function:	H5F_stdio_truncate
 *
 * Purpose:	Makes sure that the true file size is the same (or larger)
 *		than the end-of-address.
 *
 * Errors:
 *		IO	  SEEKERROR     fseek failed.
 *		IO	  WRITEERROR    fflush or fwrite failed.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Thursday, January 31, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_stdio_truncate(H5FD_t *_file, hid_t dxpl_id, hbool_t closing)
{
    H5FD_stdio_t	*file = (H5FD_stdio_t*)_file;
    static const char *func = "H5FD_stdio_truncate";  /* Function Name for error reporting */

    /* Shut compiler up */
    dxpl_id = dxpl_id;
    closing = closing;

    /* Clear the error stack */
    H5Eclear2(H5E_DEFAULT);

    /* Only try to flush the file if we have write access */
    if(file->write_access) {
        /* Makes sure that the true file size is the same as the end-of-address. */
        if(file->eoa != file->eof) {
            int fd = fileno(file->fp);     /* File descriptor for HDF5 file */

#ifdef _WIN32
            HFILE filehandle;   /* Windows file handle */
            LARGE_INTEGER li;   /* 64-bit integer for SetFilePointer() call */

            /* Map the posix file handle to a Windows file handle */
            filehandle = _get_osfhandle(fd);

            /* Translate 64-bit integers into form Windows wants */
            /* [This algorithm is from the Windows documentation for SetFilePointer()] */
            li.QuadPart = (LONGLONG)file->eoa;
            (void)SetFilePointer((HANDLE)filehandle, li.LowPart, &li.HighPart, FILE_BEGIN);
            if(SetEndOfFile((HANDLE)filehandle) == 0)
                H5Epush_ret(func, H5E_ERR_CLS, H5E_IO, H5E_SEEKERROR, "unable to truncate/extend file properly", -1)
#else /* _WIN32 */
            /* Reset seek offset to beginning of file, so that file isn't re-extended later */
            rewind(file->fp);

            /* Truncate file to proper length */
            if(-1 == file_truncate(fd, (file_offset_t)file->eoa))
                H5Epush_ret(func, H5E_ERR_CLS, H5E_IO, H5E_SEEKERROR, "unable to truncate/extend file properly", -1)
#endif /* _WIN32 */

            /* Update the eof value */
            file->eof = file->eoa;

            /* Reset last file I/O information */
            file->pos = HADDR_UNDEF;
            file->op = H5FD_STDIO_OP_UNKNOWN;
        } /* end if */
    } /* end if */
    else {
        /* Double-check for problems */
        if(file->eoa > file->eof)
            H5Epush_ret(func, H5E_ERR_CLS, H5E_IO, H5E_TRUNCATED, "eoa>eof!", -1)
      } /* end else */

    return(0);
} /* end H5FD_stdio_truncate() */


#ifdef _H5private_H
/*
 * This is not related to the functionality of the driver code.
 * It is added here to trigger warning if HDF5 private definitions are included
 * by mistake.  The code should use only HDF5 public API and definitions.
 */
#error "Do not use HDF5 private definitions"
#endif

