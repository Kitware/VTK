/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : Id  */
/*  Date : $Date$ */
/*  Version : $Revision$ */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2007 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
/*
 *
 * Purpose:  A driver which stores the HDF5 data in DSM using
 *    only the HDF5 public API. This driver is useful for distributed
 *    access to  hdf5 files.
 *    Derived from the "core" driver.
 */
#include "XdmfH5Driver.h"
#include "XdmfDsmBuffer.h"
#include "XdmfDsmComm.h"
#include "assert.h"
#include "hdf5.h"

#include <cstring>
#include <stdlib.h>

#define HDF_IO_DEBUG 1
#undef HDF_IO_DEBUG

#ifndef FAIL
#define FAIL -1
#endif

#undef MAX
#define MAX(X,Y)  ((X)>(Y)?(X):(Y))

#undef MIN
#define MIN(X,Y)  ((X)<(Y)?(X):(Y))

namespace xdmf2
{

/* The driver identification number, initialized at runtime */
static hid_t H5FD_DSM_g = 0;


/*
 * The description of a file belonging to this driver. The `eoa' and `eof'
 * determine the amount of hdf5 address space in use and the high-water mark
 * of the file (the current size of the underlying memory).
 */
typedef struct H5FD_dsm_t {
    H5FD_t  pub;      /*public stuff, must be first  */
    char  *name;      /*for equivalence testing  */
    haddr_t  eoa;      /*end of allocated region  */
    haddr_t  eof;      /*current allocated size  */
    size_t  increment;    /*multiples for mem allocation  */
    XdmfInt64  entry_addr;    /* DSM Address of this entry*/
    XdmfInt64  start;      /* Current DSM Start Address */
    XdmfInt64  end;      /* Current DSM End Address */
    char  entry_name[80];    /* Should be the same as name */
    char  entry[80];
    int    dirty;
    XdmfDsmBuffer   *DsmBuffer;
} H5FD_dsm_t;

/* Driver-specific file access properties */
typedef struct H5FD_dsm_fapl_t {
    size_t  increment;    /*how much to grow memory  */
    XdmfDsmBuffer *buffer;      /* Default DSM Buffer*/
} H5FD_dsm_fapl_t;

/*
 * These macros check for overflow of various quantities.  These macros
 * assume that file_offset_t is signed and haddr_t and size_t are unsigned.
 * 
 * ADDR_OVERFLOW:  Checks whether a file address of type `haddr_t'
 *      is too large to be represented by the second argument
 *      of the file seek function.
 *
 * SIZE_OVERFLOW:  Checks whether a buffer size of type `hsize_t' is too
 *      large to be represented by the `size_t' type.
 *
 * REGION_OVERFLOW:  Checks whether an address and size pair describe data
 *      which can be addressed entirely in memory.
 */
#if (H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=6))
#ifdef H5_HAVE_LSEEK64
#   define file_offset_t        off64_t
#   define file_seek            lseek64
#   define file_truncate        ftruncate64
#elif defined (WIN32) && !defined(__MWERKS__)
# /*MSVC*/
#   define file_offset_t __int64
#   define file_seek _lseeki64
#   define file_truncate        _ftruncatei64
#else
#   define file_offset_t        off_t
#   define file_seek            lseek
#   define file_truncate        HDftruncate
#endif
#define MAXADDR (((haddr_t)1<<(8*sizeof(file_offset_t)-1))-1)
#define DSM_HSIZE_T size_t
#else
#define MAXADDR     ((haddr_t)~(size_t)0 - 1)
#define DSM_HSIZE_T hsize_t
#endif
#define ADDR_OVERFLOW(A)  (HADDR_UNDEF==(A) ||            \
         ((A) & ~(haddr_t)MAXADDR))
#define SIZE_OVERFLOW(Z)  ((Z) & ~(hsize_t)MAXADDR)
#define REGION_OVERFLOW(A,Z)  (ADDR_OVERFLOW(A) || SIZE_OVERFLOW(Z) ||      \
                                 HADDR_UNDEF==(A)+(Z) ||          \
         (size_t)((A)+(Z))<(size_t)(A))

/* Prototypes */
extern "C" {
static void *H5FD_dsm_fapl_get(H5FD_t *_file);
static H5FD_t *H5FD_dsm_open(const char *name, unsigned flags, hid_t fapl_id,
            haddr_t maxaddr);
static herr_t H5FD_dsm_close(H5FD_t *_file);
#ifdef XDMF_NOT_USED
static herr_t H5FD_dsm_flush(H5FD_t *_file);
#endif
static int H5FD_dsm_cmp(const H5FD_t *_f1, const H5FD_t *_f2);
#if (H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))
static haddr_t H5FD_dsm_get_eoa(const H5FD_t *_file, H5FD_mem_t type);
static herr_t H5FD_dsm_set_eoa(H5FD_t *_file, H5FD_mem_t type, haddr_t addr);
static haddr_t H5FD_dsm_get_eof(const H5FD_t *_file);
#else
static haddr_t H5FD_dsm_get_eoa(H5FD_t *_file);
static herr_t H5FD_dsm_set_eoa(H5FD_t *_file, haddr_t addr);
static haddr_t H5FD_dsm_get_eof(H5FD_t *_file);
#endif
static herr_t H5FD_dsm_read(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
           DSM_HSIZE_T size, void *buf);
static herr_t H5FD_dsm_write(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
            DSM_HSIZE_T size, const void *buf);
}

static const H5FD_class_t H5FD_dsm_g = {
    "dsm",                      /*name          */
    MAXADDR,                    /*maxaddr       */
    H5F_CLOSE_WEAK,             /*fc_degree     */
    NULL,                       /*sb_size       */
    NULL,                       /*sb_encode     */
    NULL,                       /*sb_decode     */
    sizeof(H5FD_dsm_fapl_t),    /*fapl_size     */
    H5FD_dsm_fapl_get,          /*fapl_get      */
    NULL,                       /*fapl_copy     */
    NULL,                       /*fapl_free     */
    0,                          /*dxpl_size     */
    NULL,                       /*dxpl_copy     */
    NULL,                       /*dxpl_free     */
    H5FD_dsm_open,              /*open          */
    H5FD_dsm_close,             /*close         */
    H5FD_dsm_cmp,               /*cmp           */
    NULL,                       /*query         */
#if (H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))
    NULL,                       /* get_type_map */
#endif
    NULL,                       /*alloc         */
    NULL,                       /*free          */
    H5FD_dsm_get_eoa,           /*get_eoa       */
    H5FD_dsm_set_eoa,           /*set_eoa       */
    H5FD_dsm_get_eof,           /*get_eof       */
    NULL,                       /*get_handle    */
    H5FD_dsm_read,              /*read          */
    H5FD_dsm_write,             /*write         */
    NULL,                       /*flush         */
#if (H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))
    NULL,                       /* truncate     */
#endif
    NULL,                       /*lock          */
    NULL,                       /*unlock        */
    H5FD_FLMAP_SINGLE           /*fl_map        */
};


#define WORD_CMP(a, b)    ((b) == NULL ? 1 : strncmp((a), (b), strlen(b)))

/**********************************/
/********** Support ***************/
/**********************************/
#define XDMF_DSM_MAGIC  0xDEFBABE
typedef struct {
    XdmfInt64   magic;
    XdmfInt64   start;
    XdmfInt64   end;
} DsmEntry;

XdmfInt32
DsmUpdateEntry(H5FD_dsm_t *file){
    XdmfInt32   status;
    XdmfInt64   addr;
    DsmEntry    entry;

#ifdef HDF_IO_DEBUG
cout << "(" << file->DsmBuffer->GetComm()->GetId() << ") DsmUpdateEntry()" << endl;
#endif
    if(!file->DsmBuffer) return(XDMF_FAIL);
    file->end = MAX(((XdmfInt64)(file->start + file->eof)), file->end);
    file->eof = file->end - file->start;
    entry.magic = XDMF_DSM_MAGIC;
    entry.start = file->start;
    entry.end = file->end;
    addr = file->DsmBuffer->GetTotalLength() - sizeof(entry) - sizeof(XdmfInt64);
#ifdef HDF_IO_DEBUG
cout << "(" << file->DsmBuffer->GetComm()->GetId() << ") DsmUpdateEntry start " <<
        file->start <<
        " end " << file->end <<
        " addr " << addr << 
        endl;
#endif
    status = file->DsmBuffer->Put(addr, sizeof(entry), &entry); 
    if(status == XDMF_SUCCESS){
        // Send is non blocking, make sure it's there
        status = file->DsmBuffer->Get(addr, sizeof(entry), &entry); 
    }
    return(status);
}

XdmfInt32
DsmGetEntry(H5FD_dsm_t *file){
    XdmfInt32   status;
    XdmfInt64   addr;
    DsmEntry    entry;

#ifdef HDF_IO_DEBUG
cout << "(" << file->DsmBuffer->GetComm()->GetId() << ") DsmGetEntry()" << endl;
#endif
    if(!file->DsmBuffer) return(XDMF_FAIL);
    addr = file->DsmBuffer->GetTotalLength() - sizeof(entry) - sizeof(XdmfInt64);
    status = file->DsmBuffer->Get(addr, sizeof(entry), &entry); 
    if((status != XDMF_SUCCESS) || (entry.magic != XDMF_DSM_MAGIC)){
#ifdef HDF_IO_DEBUG
        cout << "(" << file->DsmBuffer->GetComm()->GetId() << ") DsmGetEntry Magic = " << entry.magic << endl;
#endif
        return(XDMF_FAIL);
    }
    file->start = entry.start;
    file->end = entry.end;
#ifdef HDF_IO_DEBUG
cout << "(" << file->DsmBuffer->GetComm()->GetId() << ") DsmGetEntry start " <<
        file->start <<
        " end " << file->end <<
        " addr " << addr  <<
        endl;
#endif
    return(XDMF_SUCCESS);
}

/**********************************/
/********** End Support ***********/
/**********************************/

/*-------------------------------------------------------------------------
 * Function:  H5FD_dsm_init
 *
 * Purpose:  Initialize this driver by registering the driver with the
 *    library.
 *
 * Return:  Success:  The driver ID for the dsm driver.
 *
 *    Failure:  Negative.
 *
 * Programmer:  Jerry Clarke
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FD_dsm_init(void)
{
#ifdef HDF_IO_DEBUG
cout << "In H5FD_dsm_init()" << endl;
#endif
    if (H5I_VFL!=H5Iget_type(H5FD_DSM_g)) {
  H5FD_DSM_g = H5FDregister(&H5FD_dsm_g);
    }
    return H5FD_DSM_g;
}

/*-------------------------------------------------------------------------
 * Function:  H5Pset_fapl_dsm
 *
 * Purpose:  Modify the file access property list to use the H5FD_DSM
 *    driver defined in this source file.  The INCREMENT specifies
 *    how much to grow the memory each time we need more.
 *    
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Jerry Clarke
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fapl_dsm(hid_t fapl_id, size_t increment, XdmfDsmBuffer *buffer )
{
    H5FD_dsm_fapl_t  fa;

#ifdef HDF_IO_DEBUG
#endif
    fa.increment = increment;
    fa.buffer = buffer;

    return H5Pset_driver(fapl_id, H5FD_DSM, &fa);
}

/*-------------------------------------------------------------------------
 * Function:  H5Pget_fapl_dsm
 *
 * Purpose:  Queries properties set by the H5Pset_fapl_dsm() function.
 *
 * Return:  Success:  Non-negative
 *
 *    Failure:  Negative
 *
 * Programmer:  Jerry Clarke
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_fapl_dsm(hid_t fapl_id, size_t *increment/*out*/, XdmfDsmBuffer **buffer /* out */ )
{
    H5FD_dsm_fapl_t  *fa;

#ifdef HDF_IO_DEBUG
printf("Getting fapl\n");
#endif
    /*NO TRACE*/
    if (H5P_FILE_ACCESS!=H5Pget_class(fapl_id)) return -1;
    if (H5FD_DSM!=H5Pget_driver(fapl_id)) return -1;
    if (NULL==(fa=(H5FD_dsm_fapl_t *)H5Pget_driver_info(fapl_id))) return -1;
    if (increment) *increment = fa->increment;
    if( buffer ) *buffer = fa->buffer;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:  H5FD_dsm_fapl_get
 *
 * Purpose:  Returns a copy of the file access properties.
 *
 * Return:  Success:  Ptr to new file access properties.
 *
 *    Failure:  NULL
 *
 * Programmer:  Jerry Clarke
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5FD_dsm_fapl_get(H5FD_t *_file)
{
    H5FD_dsm_t    *file = (H5FD_dsm_t*)_file;
    H5FD_dsm_fapl_t  *fa = (H5FD_dsm_fapl_t *)calloc(1, sizeof(H5FD_dsm_fapl_t));

#ifdef HDF_IO_DEBUG
printf("Fapl Get\n");
#endif
    fa->increment = file->increment;
    return fa;
}

/*-------------------------------------------------------------------------
 * Function:  H5FD_dsm_open
 *
 * Purpose:  Create memory as an HDF5 file.
 *
 * Return:  Success:  A pointer to a new file data structure. The
 *        public fields will be initialized by the
 *        caller, which is always H5FD_open().
 *
 *    Failure:  NULL
 *
 * Programmer:  Jerry Clarke
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5FD_t *
H5FD_dsm_open(const char *name, unsigned flags/*unused*/, hid_t fapl_id,
         haddr_t maxaddr)
{
    H5FD_dsm_t    *file=NULL;
    H5FD_dsm_fapl_t  *fa=NULL;
    XdmfInt32       status;
    //XdmfInt64    addr;
    
    /* Check arguments */
    if (0==maxaddr || HADDR_UNDEF==maxaddr) return NULL;
#ifdef HDF_IO_DEBUG
// printf("Opening %s ... Checking maxaddr \n", name);
#endif
    if (ADDR_OVERFLOW(maxaddr)) return NULL;
#ifdef HDF_IO_DEBUG
// printf("Opening %s .... Setting Driver\n", name);
#endif
    if (H5P_DEFAULT!=fapl_id) fa = (H5FD_dsm_fapl_t *)H5Pget_driver_info(fapl_id);

#ifdef HDF_IO_DEBUG
// printf("Opening %s\n", name);
#endif

// One HDF5 file @ Address 0

    /* Create the new file struct */
    file = (H5FD_dsm_t *)calloc(1, sizeof(H5FD_dsm_t));
    if (name && *name) {
        XDMF_STRING_DUPLICATE(file->name, name);
    }

    /* See if it exists */
    file->DsmBuffer = fa->buffer;
#ifdef HDF_IO_DEBUG
cout << "(" << file->DsmBuffer->GetComm()->GetId() << ") Opening " << name << endl;
#endif
    status = DsmGetEntry(file);
#ifdef HDF_IO_DEBUG
if( status == XDMF_SUCCESS ){
    cout << "(" << file->DsmBuffer->GetComm()->GetId() << ") Opened from Entry  " << name <<
        " Start " << file->start <<
        " End " << file->end <<
        endl;
}
#endif
  if(H5F_ACC_CREAT & flags){
    if (status == XDMF_FAIL){
#ifdef HDF_IO_DEBUG
    cout << "(" << file->DsmBuffer->GetComm()->GetId() << ") Creating  " << name << endl;
#endif
      DsmUpdateEntry(file);
    }
    file->eof = file->end - file->start;
  } else {
    /* Must Already Exist */
    if(status == XDMF_FAIL){
      free( file );
      return( NULL );
    }
    if(H5F_ACC_RDWR & flags) {
      /* Read and Write */
      file->eof = file->end - file->start;
    } else {
      /* Read Only */
      file->eof = file->end - file->start;
    }
  }

    /*
     * The increment comes from either the file access property list or the
     * default value. But if the file access property list was zero then use
     * the default value instead.
     */
    file->increment = (fa && fa->increment>0) ?
          fa->increment : H5FD_DSM_INCREMENT;
#ifdef HDF_IO_DEBUG
cout << "(" << file->DsmBuffer->GetComm()->GetId() << ") Opened " << name <<
    " Start " << file->start <<
    " End " << file->end <<
    " Eoa " << file->eoa <<
    " Eof  " << file->eof <<
    endl;
#endif

  file->dirty = 0;
#ifdef HDF_IO_DEBUG
// fprintf(stderr, "Returning (H5FD_t*)fil = <%p>\n", file);
#endif
    return (H5FD_t*)file;
}

/*-------------------------------------------------------------------------
 * Function:  H5FD_dsm_flush
 *
 * Purpose:  Flush the file.
 *
 * Return:  Success:  0
 *
 *    Failure:  -1
 *
 * Programmer:  Jerry Clarke
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifdef XDMF_NOT_USED
static herr_t
H5FD_dsm_flush(H5FD_t *_file)
{
    H5FD_dsm_t  *file = (H5FD_dsm_t*)_file;
    XdmfInt64 size;
    int status;

  file->dirty = 0;
return( 0 );
}
#endif
/*-------------------------------------------------------------------------
 * Function:  H5FD_dsm_close
 *
 * Purpose:  Closes the file.
 *
 * Return:  Success:  0
 *
 *    Failure:  -1
 *
 * Programmer:  Jerry Clarke
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_dsm_close(H5FD_t *_file)
{
    H5FD_dsm_t  *file = (H5FD_dsm_t*)_file;
    int status;

#ifdef HDF_IO_DEBUG
cout << "(" << file->DsmBuffer->GetComm()->GetId() << ") Closing Start " << file->start <<
    " End " << file->end <<
    " Eoa " << file->eoa <<
    " Eof " << file->eof <<
    endl;
#endif
        status = DsmUpdateEntry( file );
  if( status != XDMF_SUCCESS) return -1;
    if (file->name) free(file->name);
    memset(file, 0, sizeof(H5FD_dsm_t));
    free(file);
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:  H5FD_dsm_cmp
 *
 * Purpose:  Compares two files belonging to this driver by name. If one
 *    file doesn't have a name then it is less than the other file.
 *    If neither file has a name then the comparison is by file
 *    address.
 *
 * Return:  Success:  A value like strcmp()
 *
 *    Failure:  never fails (arguments were checked by the
 *        caller).
 *
 * Programmer:  Jerry Clarke
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
H5FD_dsm_cmp(const H5FD_t *_f1, const H5FD_t *_f2)
{
    const H5FD_dsm_t  *f1 = (const H5FD_dsm_t*)_f1;
    const H5FD_dsm_t  *f2 = (const H5FD_dsm_t*)_f2;

    if (NULL==f1->name && NULL==f2->name) {
  if (f1<f2) return -1;
  if (f1>f2) return 1;
  return 0;
    }
    
    if (NULL==f1->name) return -1;
    if (NULL==f2->name) return 1;

    return strcmp(f1->name, f2->name);
}

/*-------------------------------------------------------------------------
 * Function:  H5FD_dsm_get_eoa
 *
 * Purpose:  Gets the end-of-address marker for the file. The EOA marker
 *    is the first address past the last byte allocated in the
 *    format address space.
 *
 * Return:  Success:  The end-of-address marker.
 *
 *    Failure:  HADDR_UNDEF
 *
 * Programmer:  Jerry Clarke
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
#if (H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))
H5FD_dsm_get_eoa(const H5FD_t *_file, H5FD_mem_t type)
#else
H5FD_dsm_get_eoa(H5FD_t *_file)
#endif
{
    H5FD_dsm_t  *file = (H5FD_dsm_t*)_file;

#ifdef HDF_IO_DEBUG
// printf("H5FD_dsm_get_eoa Called\n");
#endif
    return file->eoa;
}

/*-------------------------------------------------------------------------
 * Function:  H5FD_dsm_set_eoa
 *
 * Purpose:  Set the end-of-address marker for the file. This function is
 *    called shortly after an existing HDF5 file is opened in order
 *    to tell the driver where the end of the HDF5 data is located.
 *
 * Return:  Success:  0
 *
 *    Failure:  -1
 *
 * Programmer:  Jerry Clarke
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
#if (H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))
H5FD_dsm_set_eoa(H5FD_t *_file, H5FD_mem_t type, haddr_t addr)
#else
H5FD_dsm_set_eoa(H5FD_t *_file, haddr_t addr)
#endif
{
    H5FD_dsm_t  *file = (H5FD_dsm_t*)_file;


#ifdef HDF_IO_DEBUG
// printf("H5FD_dsm_set_eoa Called %ld \n", addr);
#endif
    if (ADDR_OVERFLOW(addr)){
#ifdef HDF_IO_DEBUG
// printf("H5FD_dsm_set_eoa Address OverFLow at %ld \n", addr);
// printf("H5FD_dsm_set_eoa MAXADDR = %ld \n", MAXADDR);
// printf("H5FD_dsm_set_eoa Address (addr) & ~(haddr_t)MAXADDR) = %ld \n", (addr) & ~(haddr_t)MAXADDR);
#endif
     return -1;
    }
    file->eof = file->eoa = addr;
    DsmUpdateEntry( file );
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:  H5FD_dsm_get_eof
 *
 * Purpose:  Returns the end-of-file marker, which is the greater of
 *    either the size of the underlying memory or the HDF5
 *    end-of-address markers.
 *
 * Return:  Success:  End of file address, the first address past
 *        the end of the "file", either the memory
 *        or the HDF5 file.
 *
 *    Failure:  HADDR_UNDEF
 *
 * Programmer:  Jerry Clarke
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
#if (H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))
H5FD_dsm_get_eof(const H5FD_t *_file)
#else
H5FD_dsm_get_eof(H5FD_t *_file)
#endif
{
    H5FD_dsm_t  *file = (H5FD_dsm_t*)_file;

#ifdef HDF_IO_DEBUG
// printf("H5FD_dsm_get_eoa Called %ld \n", MAX(file->eof, file->eoa) );
#endif
    return MAX(file->eof, file->eoa);
}

/*-------------------------------------------------------------------------
 * Function:  H5FD_dsm_read
 *
 * Purpose:  Reads SIZE bytes of data from FILE beginning at address ADDR
 *    into buffer BUF according to data transfer properties in
 *    DXPL_ID.
 *
 * Return:  Success:  Zero. Result is stored in caller-supplied
 *        buffer BUF.
 *
 *    Failure:  -1, Contents of buffer BUF are undefined.
 *
 * Programmer:  Jerry Clarke
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_dsm_read(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id/*unused*/, haddr_t addr,
         DSM_HSIZE_T size, void *buf/*out*/)
{
    H5FD_dsm_t    *file = (H5FD_dsm_t*)_file;
    ssize_t    nbytes;
    herr_t    status;

(void)type;
(void)dxpl_id;
    
#ifdef HDF_IO_DEBUG
cout << "(" << file->DsmBuffer->GetComm()->GetId() << ") Read Start " << file->start <<
    " End " << file->end <<
    " Addr " << addr  <<
    " Size " << size  <<
    " Eoa " << file->eoa <<
    " Eof " << file->eof <<
     endl;
#endif
    assert(file && file->pub.cls);
    assert(buf);

    /* Check for overflow conditions */
    if (HADDR_UNDEF==addr) return -1;
    if (REGION_OVERFLOW(addr, size)) return -1;
    if (addr+size>file->eoa) return -1;

    /* Read the part which is before the EOF marker */
#ifdef HDF_IO_DEBUG
/*
printf("check addr ( %ld )  < file->eof ( %ld )\n", addr, file->eof);
*/
#endif
    if (addr<file->eof) {
  nbytes = MIN(size, file->eof-addr);
  status = file->DsmBuffer->Get(file->start + addr, nbytes, buf);
  if( status <= 0 ) return -1;
  size -= nbytes;
  addr += nbytes;
  buf = (char*)buf + nbytes;
    }

    /* Read zeros for the part which is after the EOF markers */
    if (size>0) {
  memset(buf, 0, size);
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:  H5FD_dsm_write
 *
 * Purpose:  Writes SIZE bytes of data to FILE beginning at address ADDR
 *    from buffer BUF according to data transfer properties in
 *    DXPL_ID.
 *
 * Return:  Success:  Zero
 *
 *    Failure:  -1
 *
 * Programmer:  Jerry Clarke
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_dsm_write(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id/*unused*/, haddr_t addr,
    DSM_HSIZE_T size, const void *buf)
{
    H5FD_dsm_t    *file = (H5FD_dsm_t*)_file;
    herr_t    status;

/*
    const char    *bufp = buf;
*/
    
    (void)type;
    (void)dxpl_id;
    
    assert(file && file->pub.cls);
    assert(buf);

#ifdef HDF_IO_DEBUG
cout << "(" << file->DsmBuffer->GetComm()->GetId() << ") Write Start " << file->start <<
    " End " << file->end <<
    " Addr " << addr  <<
    " Size " << size  <<
    " Eoa " << file->eoa <<
    " Eof " << file->eof <<
    endl;
#endif
    /* Check for overflow conditions */
    if (REGION_OVERFLOW(addr, size)) return -1;
    if (addr+size>file->eoa) return -1;

    if (addr+size>file->eof) {
  size_t new_eof = file->increment * ((addr+size)/file->increment);
  if ((addr+size) % file->increment) new_eof += file->increment;
#ifdef HDF_IO_DEBUG
// printf("HDF::Write New eof %ld\n", new_eof);
#endif
  /* Blindly Grab more DSM for now */
  file->end = file->start + new_eof;
  file->eof = new_eof;
  /* Write it out to DSM */
  status = DsmUpdateEntry( file );
  if( status != XDMF_SUCCESS ) return -1;

    }

    /* Write from BUF to DSM */
  status = file->DsmBuffer->Put(file->start + addr, size, (void *)buf);
  if( status <= 0 ) return -1;
  return 0;
}
}
