/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfDSMDriver.cpp                                                   */
/*                                                                           */
/*  Author:                                                                  */
/*     Andrew Burns                                                          */
/*     andrew.j.burns2@us.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2013 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

/*=========================================================================
  This code is derived from an earlier work and is distributed
  with permission from, and thanks to ...
=========================================================================*/

/*============================================================================

  Project                 : H5FDdsm
  Module                  : H5FDdsmDriver.cxx H5FDdsm.c

  Authors:
     John Biddiscombe     Jerome Soumagne
     biddisco@cscs.ch     soumagne@cscs.ch

  Copyright (C) CSCS - Swiss National Supercomputing Centre.
  You may use modify and and distribute this code freely providing
  1) This copyright notice appears on all copies of source code
  2) An acknowledgment appears with any substantial usage of the code
  3) If this code is contributed to any other open source project, it
  must not be reformatted such that the indentation, bracketing or
  overall style is modified significantly.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  This work has received funding from the European Community's Seventh
  Framework Programme (FP7/2007-2013) under grant agreement 225967 âxtMuSEâOC

============================================================================*/

#include <XdmfDSMBuffer.hpp>
#include <XdmfDSMCommMPI.hpp>
#include <XdmfError.hpp>
#include <mpi.h>

#include <H5public.h>
#include <hdf5.h>

#include <sstream>
#include <map>

typedef struct XDMF_dsm_t
{
  H5FD_t pub;            /* public stuff, must be first             */
  char *name;            /* for equivalence testing                 */
  void *local_buf_ptr;   /* underlying local DSM buffer             */
  size_t local_buf_len;  /* local DSM buffer length                 */
  haddr_t eoa;           /* end of address marker                   */
  haddr_t eof;           /* end of file marker                      */
  haddr_t start;         /* current DSM start address               */
  haddr_t end;           /* current DSM end address                 */
  hbool_t read_only;     /* file access is read-only                */
  hbool_t dirty;         /* dirty marker                            */
  unsigned int numPages; /* number of pages assigned to the file    */
} XDMF_dsm_t;

typedef struct XDMF_dsm_fapl_t
{
  void  *local_buf_ptr;     /* local buffer pointer        */
  size_t local_buf_len;     /* local buffer length         */
} XDMF_dsm_fapl_t;

typedef struct
{
  long start;
  long end;
} XdmfDSMEntry;

/* The driver identification number, initialized at runtime */
static hid_t XDMF_DSM_g = 0;

//from driver
XdmfDSMBuffer *dsmBuffer = NULL;
std::map<std::string, unsigned int> fileEOF; // holding previously created files
std::map<std::string, std::vector<unsigned int> > filePages;

#define MAXADDR                 ((haddr_t)((~(size_t)0)-1))
#define ADDR_OVERFLOW(A)        (HADDR_UNDEF==(A) || (A) > (haddr_t)MAXADDR)
#define SIZE_OVERFLOW(Z)        ((Z) > (hsize_t)MAXADDR)
#define REGION_OVERFLOW(A,Z)    (ADDR_OVERFLOW(A) || SIZE_OVERFLOW(Z) ||      \
                                 HADDR_UNDEF==(A)+(Z) ||                      \
                                 (size_t)((A)+(Z))<(size_t)(A))

extern "C" {

#include "H5FDprivate.h"  /* File drivers           */
#include "H5private.h"    /* Generic Functions      */
#include "H5ACprivate.h"  /* Metadata cache         */
#include "H5Dprivate.h"   /* Dataset functions      */
#include "H5Eprivate.h"   /* Error handling         */
#include "H5Fprivate.h"   /* File access            */
#include "H5FDmpi.h"      /* MPI-based file drivers */
#include "H5Iprivate.h"   /* IDs                    */
#include "H5Pprivate.h"   /* Property lists         */

#include <XdmfDSMDriver.hpp>

/* Private Prototypes */
static void    *XDMF_dsm_fapl_get(H5FD_t *_file);
static void    *XDMF_dsm_fapl_copy(const void *_old_fa);
static herr_t   XDMF_dsm_fapl_free(void *_fa);
static H5FD_t  *XDMF_dsm_open(const char *name, unsigned flags, hid_t fapl_id,
    haddr_t maxaddr);
static herr_t   XDMF_dsm_close(H5FD_t *_file);
static herr_t   XDMF_dsm_query(const H5FD_t *_f1, unsigned long *flags);
static haddr_t  XDMF_dsm_get_eoa(const H5FD_t *_file, H5FD_mem_t type);
static herr_t   XDMF_dsm_set_eoa(H5FD_t * _file, H5FD_mem_t type, haddr_t addr);
static haddr_t  XDMF_dsm_get_eof(const H5FD_t *_file);
static herr_t   XDMF_dsm_read(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id,
    haddr_t addr, size_t size, void *buf);
static herr_t   XDMF_dsm_write(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id,
    haddr_t addr, size_t size, const void *buf);
static herr_t   XDMF_dsm_flush(H5FD_t *_file, hid_t dxpl_id, unsigned closing);
static int      XDMF_dsm_mpi_rank(const H5FD_t *_file);
static int      XDMF_dsm_mpi_size(const H5FD_t *_file);
static MPI_Comm XDMF_dsm_communicator(const H5FD_t *_file);

/* The DSM file driver information */
static const H5FD_class_mpi_t XDMF_dsm_g = {
    {
        "dsm",                                  /* name                 */
        MAXADDR,                                /* maxaddr              */
        H5F_CLOSE_SEMI,                         /* fc_degree            */
#if H5_VERSION_GE(1,9,0)
        XDMF_dsm_term,                          /* terminate            */
#endif
        NULL,                                   /* sb_size              */
        NULL,                                   /* sb_encode            */
        NULL,                                   /* sb_decode            */
        sizeof(XDMF_dsm_fapl_t),                /* fapl_size            */
        XDMF_dsm_fapl_get,                      /* fapl_get             */
        XDMF_dsm_fapl_copy,                     /* fapl_copy            */
        XDMF_dsm_fapl_free,                     /* fapl_free            */
        0,                                      /* dxpl_size            */
        NULL,                                   /* dxpl_copy            */
        NULL,                                   /* dxpl_free            */
        XDMF_dsm_open,                          /* open                 */
        XDMF_dsm_close,                         /* close                */
        NULL,                                   /* cmp                  */
        XDMF_dsm_query,                         /* query                */
        NULL,                                   /* get_type_map         */
        NULL,                                   /* alloc                */
        NULL,                                   /* free                 */
#ifdef H5_HAVE_VFD_EXTENSIONS
        XDMF_dsm_term,                          /* terminate            */
#endif
        XDMF_dsm_get_eoa,                       /* get_eoa              */
        XDMF_dsm_set_eoa,                       /* set_eoa              */
        XDMF_dsm_get_eof,                       /* get_eof              */
        NULL,                                   /* get_handle           */
        XDMF_dsm_read,                          /* read                 */
        XDMF_dsm_write,                         /* write                */
        XDMF_dsm_flush,                         /* flush                */
        NULL,                                   /* truncate             */
        NULL,                                   /* lock                 */
        NULL,                                   /* unlock               */
        H5FD_FLMAP_SINGLE                       /* fl_map               */
    },
    XDMF_dsm_mpi_rank,                          /* get_rank             */
    XDMF_dsm_mpi_size,                          /* get_size             */
    XDMF_dsm_communicator                       /* get_comm             */
};

#define H5_INTERFACE_INIT_FUNC  XDMF_dsm_init_interface

static herr_t
XDMF_dsm_init_interface(void)
{
#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI_NOINIT_NOERR
#else
  FUNC_ENTER_NOAPI_NOINIT_NOFUNC(XDMF_dsm_init_interface)
#endif

  FUNC_LEAVE_NOAPI(XDMF_dsm_init())
}

void
XdmfUnused(void)
{
  //To remove warning about XDMF_dsm_init_interface being unused
  XDMF_dsm_init_interface();
}

hid_t
XDMF_dsm_init(void)
{
  hid_t ret_value; /* Return value */

#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI(FAIL)
#else
  FUNC_ENTER_NOAPI(XDMF_dsm_init, FAIL)
#endif

  if (H5I_VFL != H5Iget_type(XDMF_DSM_g)) {// registering the driver
    XDMF_DSM_g = H5FD_register(&XDMF_dsm_g, sizeof(H5FD_class_mpi_t), FALSE);
  }

  /* Set return value */
  ret_value = XDMF_DSM_g;// return value is the new id of the driver

// Removed because error handling isn't called in this function
//done:
  if (err_occurred) {
    /* Nothing */
  }

  FUNC_LEAVE_NOAPI(ret_value)
}


#if H5_VERSION_GE(1,9,0)
herr_t
#else
void
#endif
XDMF_dsm_term(void)
{
#if H5_VERSION_GE(1,9,0)
  herr_t ret_value = SUCCEED;

  FUNC_ENTER_NOAPI(FAIL)
#else
#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI_NOINIT_NOERR
#else
  FUNC_ENTER_NOAPI_NOINIT_NOFUNC(XDMF_dsm_term)
#endif
#endif

  if (SUCCEED != xdmf_dsm_free()) {
#if H5_VERSION_GE(1,9,0)
    HGOTO_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "dsm_free failed");
#endif
  }

  /* Reset VFL ID */
  XDMF_DSM_g = 0;

#if H5_VERSION_GE(1,9,0)
done:
  FUNC_LEAVE_NOAPI(ret_value)
#else
  FUNC_LEAVE_NOAPI_VOID
#endif
}

herr_t
XDMF_dsm_set_options(unsigned long flags)
{
  herr_t ret_value = SUCCEED;

#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI(FAIL)
#else
  FUNC_ENTER_NOAPI(XDMF_dsm_set_options, FAIL)
#endif

  if (SUCCEED != xdmf_dsm_set_options(flags))
    HGOTO_ERROR(H5E_VFL, H5E_CANTMODIFY, FAIL, "cannot set options")

done:
  if (err_occurred) {
    /* Nothing */
  }

  FUNC_LEAVE_NOAPI(ret_value)
}

herr_t
XDMF_dsm_set_manager(void *manager)
{
  herr_t ret_value = SUCCEED;

#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI(FAIL)
#else
  FUNC_ENTER_NOAPI(XDMF_dsm_set_manager, FAIL)
#endif

  xdmf_dsm_set_manager(manager);

// Removed because error handling isn't called in this function
//done:
  if (err_occurred) {
    /* Nothing */
  }

  FUNC_LEAVE_NOAPI(ret_value)
}

herr_t
XDMFH5Pset_fapl_dsm(hid_t fapl_id, MPI_Comm intra_comm, void *local_buf_ptr,
                    size_t local_buf_len)
{
//printf("XDMFH5Pset_fapl_dsm\n");
  XDMF_dsm_fapl_t fa;
  herr_t ret_value = SUCCEED;
  H5P_genplist_t *plist; /* Property list pointer */

#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_API(FAIL)
#else
  FUNC_ENTER_API(XDMFH5Pset_fapl_dsm, FAIL)
#endif
  /* Check arguments */
  if (NULL == (plist = H5P_object_verify(fapl_id, H5P_FILE_ACCESS)))
    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list")

  if (!xdmf_dsm_get_manager()) {
	// throw error here instead of calling alloc
        XdmfError::message(XdmfError::FATAL, "Error: In set_fapl_dsm No manager set");
  }

  if (SUCCEED != xdmf_dsm_get_properties(NULL, &fa.local_buf_ptr, &fa.local_buf_len))
    HGOTO_ERROR(H5E_PLIST, H5E_CANTALLOC, FAIL, "cannot get DSM properties")

  if (!xdmf_dsm_is_server() && !xdmf_dsm_is_connected()) {

    //it should already be connected when this is called
    //potentially the connect will connect via port
    xdmf_dsm_connect();
  }

  /* duplication is done during driver setting */
  ret_value = H5P_set_driver(plist, XDMF_DSM, &fa);

done:
  FUNC_LEAVE_API(ret_value)
}

herr_t
XDMFH5Pget_fapl_dsm(hid_t fapl_id, MPI_Comm *intra_comm /* out */,
    void **local_buf_ptr_ptr /* out */, size_t *local_buf_len_ptr /* out */)
{
  XDMF_dsm_fapl_t *fa;
  MPI_Comm intra_comm_tmp = MPI_COMM_NULL;
  H5P_genplist_t *plist; /* Property list pointer */
  herr_t ret_value = SUCCEED;
  int mpi_code;

#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_API(FAIL)
#else
  FUNC_ENTER_API(XDMFH5Pget_fapl_dsm, FAIL)
#endif

  if (NULL == (plist = (H5P_genplist_t*) H5P_object_verify(fapl_id, H5P_FILE_ACCESS)))
    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list")
  if (XDMF_DSM != H5P_get_driver(plist))
    HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "incorrect VFL driver")
  if (NULL == (fa = (XDMF_dsm_fapl_t*) H5P_get_driver_info(plist)))
    HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "bad VFL driver info")

  if (intra_comm) {
    if (MPI_SUCCESS != (mpi_code = MPI_Comm_dup(dsmBuffer->GetComm()->GetIntraComm(), &intra_comm_tmp)))
      HMPI_GOTO_ERROR(FAIL, "MPI_Comm_dup failed", mpi_code)
    *intra_comm = intra_comm_tmp;
  }

  if (local_buf_ptr_ptr) *local_buf_ptr_ptr = fa->local_buf_ptr;

  if (local_buf_len_ptr) *local_buf_len_ptr = fa->local_buf_len;

done:
  if (FAIL == ret_value) {
    /* need to free anything created here */
  }

  FUNC_LEAVE_API(ret_value)
}

static void *
XDMF_dsm_fapl_get(H5FD_t *_file)
{
//printf("XDMF_dsm_fapl_get\n");
  XDMF_dsm_t *file = (XDMF_dsm_t*) _file;
  XDMF_dsm_fapl_t *fa = NULL;
  void *ret_value = NULL;
  int mpi_code;

#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI_NOINIT
#else
  FUNC_ENTER_NOAPI_NOINIT(XDMF_dsm_fapl_get)
#endif

  HDassert(file);
  HDassert(XDMF_DSM == file->pub.driver_id);

  if (NULL == (fa = (XDMF_dsm_fapl_t *) calloc((size_t)1, sizeof(XDMF_dsm_fapl_t))))
    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

  fa->local_buf_ptr = file->local_buf_ptr;
  fa->local_buf_len = file->local_buf_len;

  /* Set return value */
  ret_value = fa;

done:
  if ((NULL == ret_value) && err_occurred) {
    /* need to free anything created here */
  }

  FUNC_LEAVE_NOAPI(ret_value)
}

static void *
XDMF_dsm_fapl_copy(const void *_old_fa)
{
//printf("XDMF_dsm_fapl_copy\n");
  void *ret_value = NULL;
  const XDMF_dsm_fapl_t *old_fa = (const XDMF_dsm_fapl_t*)_old_fa;
  XDMF_dsm_fapl_t *new_fa = NULL;
  int mpi_code;

#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI_NOINIT
#else
  FUNC_ENTER_NOAPI_NOINIT(XDMF_dsm_fapl_copy)
#endif

  if(NULL == (new_fa = (XDMF_dsm_fapl_t *) calloc((size_t)1, sizeof(XDMF_dsm_fapl_t))))
    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

  /* Copy the general information */
  HDmemcpy(new_fa, old_fa, sizeof(XDMF_dsm_fapl_t));

  ret_value = new_fa;

done:
  if ((NULL == ret_value) && err_occurred) {
    /* cleanup */
    if (new_fa) free(new_fa);
  }

  FUNC_LEAVE_NOAPI(ret_value)
}

static herr_t
XDMF_dsm_fapl_free(void *_fa)
{
  herr_t ret_value = SUCCEED;
  XDMF_dsm_fapl_t *fa = (XDMF_dsm_fapl_t*)_fa;

#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI_NOINIT_NOERR
#else
  FUNC_ENTER_NOAPI_NOINIT_NOFUNC(XDMF_dsm_fapl_free)
#endif

  assert(fa);

  free(fa);

  FUNC_LEAVE_NOAPI(ret_value)
}

static H5FD_t *
XDMF_dsm_open(const char *name, unsigned flags, hid_t fapl_id, haddr_t maxaddr)
{
  XDMF_dsm_t *file = NULL;
  int mpi_rank; /* MPI rank of this process */
  int mpi_size; /* Total number of MPI processes */
  int mpi_code;  /* mpi return code */
  const XDMF_dsm_fapl_t *fa = NULL;
  H5P_genplist_t *plist; /* Property list pointer */
  H5FD_t *ret_value = NULL;
  herr_t dsm_code = SUCCEED;

  unsigned int * newpages = NULL;

#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI_NOINIT
#else
  FUNC_ENTER_NOAPI_NOINIT(XDMF_dsm_open)
#endif

  /* Check arguments */
  if(!name || !*name)
    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid file name")
  if (0 == maxaddr || HADDR_UNDEF == maxaddr)
    HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, NULL, "bogus maxaddr")
  if (ADDR_OVERFLOW(maxaddr))
    HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, NULL, "maxaddr overflow")


  if (H5P_DEFAULT != fapl_id) {

    if (NULL == (plist = (H5P_genplist_t*) H5I_object(fapl_id)))
      HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access property list")
    fa = (const XDMF_dsm_fapl_t *) H5P_get_driver_info(plist);
    assert(fa);
  }

  /* Get the MPI rank of this process and the total number of processes */
  mpi_rank = dsmBuffer->GetComm()->GetId();
  mpi_size = dsmBuffer->GetComm()->GetIntraSize();

  /* Build the return value and initialize it */
  if (NULL == (file = (XDMF_dsm_t *) calloc((size_t)1, sizeof(XDMF_dsm_t))))
    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

  if (name && *name) {
    file->name = HDstrdup(name);
  }

  /* Get address information from DSM */
  if (!xdmf_dsm_get_manager())
    HGOTO_ERROR(H5E_VFL, H5E_NOTFOUND, NULL, "DSM buffer not found")

  file->local_buf_ptr = fa->local_buf_ptr;
  file->local_buf_len = fa->local_buf_len;

  // Poll DSM for file information
  // If file exists fill data from the DSM's results
  // Otherwise reset data.
  // For now, multiple files are only supported by paged mode.
  if (dsmBuffer->GetComm()->GetId() == 0)
  {
    if (dsmBuffer->RequestFileDescription(file->name,
                                          filePages[file->name],
                                          file->numPages,
                                          file->start,
                                          file->end) == XDMF_DSM_FAIL)
    {
      // File not found
      file->numPages = 0;
      file->start = 0;
      file->end = 0;
    }
  }



  dsmBuffer->GetComm()->Broadcast(&file->start,
                                  sizeof(haddr_t),
                                  0,
                                  XDMF_DSM_INTRA_COMM);

  dsmBuffer->GetComm()->Broadcast(&file->end,
                                  sizeof(haddr_t),
                                  0,
                                  XDMF_DSM_INTRA_COMM);

  dsmBuffer->GetComm()->Broadcast(&file->numPages,
                                  sizeof(unsigned int),
                                  0,
                                  XDMF_DSM_INTRA_COMM);

  if (file->numPages > 0)
  {
    if (dsmBuffer->GetComm()->GetId() != 0)
    {
      filePages[file->name].clear();
      for (unsigned int i = 0; i < file->numPages; ++i)
      {
        filePages[file->name].push_back(0);
      }
    }
    dsmBuffer->GetComm()->Broadcast(&(filePages[file->name][0]),
                                    sizeof(unsigned int) * file->numPages,
                                    0,
                                    XDMF_DSM_INTRA_COMM);
  }

  if (H5F_ACC_RDWR & flags) {
    file->read_only = FALSE;
  } else {
    file->read_only = TRUE;
  }

  if (H5F_ACC_CREAT & flags) {
    /* Reset start and end markers */
    if (fileEOF.count(name) == 0) // Need to store the eof and eoa of the files here
    {
      file->start = 0;
      file->end = 0;
      file->eof = 0;
      fileEOF[name] = 0;
    }
    else
    {
      file->eof = fileEOF[name];
    }

  } else {
    file->eof = file->end - file->start;
  }

  // Poll DSM for page info if in paged mode

  /* Don't let any proc return until all have created the file. */
  if ((H5F_ACC_CREAT & flags)) {
    dsmBuffer->GetComm()->Barrier(XDMF_DSM_INTRA_COMM);
  }

  /* Set return value */
  ret_value = (H5FD_t *) file;

done:
  if((ret_value == NULL) && err_occurred) {
    if (file && file->name) HDfree(file->name);
    if (file) free(file);
  } /* end if */


  FUNC_LEAVE_NOAPI(ret_value)
}

static herr_t
XDMF_dsm_close(H5FD_t *_file)
{
  XDMF_dsm_t *file = (XDMF_dsm_t*) _file;
  herr_t ret_value = SUCCEED; /* Return value */
  int mpi_code;
  herr_t dsm_code = SUCCEED;
  unsigned long unlock_flag;

#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI_NOINIT
#else
  FUNC_ENTER_NOAPI_NOINIT(XDMF_dsm_close)
#endif

  assert(file);
  assert(XDMF_DSM == file->pub.driver_id);

  if (!file->read_only) {
    if (dsmBuffer->GetComm()->GetId() == 0)
    {
      // Update DSM file description
      if (dsmBuffer->RegisterFile(file->name,
                              &(filePages[file->name][0]),
                              file->numPages,
                              file->start,
                              file->end) == XDMF_DSM_FAIL)
      {
        dsm_code = FAIL;
      }
    }

    /* Wait for the DSM entry to be updated */
    dsmBuffer->GetComm()->Broadcast(&dsm_code,
                                    sizeof(herr_t),
                                    0,
                                    XDMF_DSM_INTRA_COMM);

    if (SUCCEED != dsm_code)
      HGOTO_ERROR(H5E_VFL, H5E_CANTUPDATE, FAIL, "cannot update DSM entries")

      /*
       * Be sure that everyone's here before releasing resources (done with
       * collective op). Gather all the dirty flags because some processes may
       * not have written yet.
       */
      hbool_t * dirtyaccum = new hbool_t[dsmBuffer->GetComm()->GetIntraSize()]();

      dsmBuffer->GetComm()->AllGather(&file->dirty,
                                      sizeof(hbool_t),
                                      dirtyaccum,
                                      sizeof(hbool_t),
                                      XDMF_DSM_INTRA_COMM);

      for (unsigned int i = 0; i < dsmBuffer->GetComm()->GetIntraSize(); ++i)
      {
        if (dirtyaccum[i] > file->dirty)
        {
          file->dirty = dirtyaccum[i];
        }
      }

      delete dirtyaccum;
  }

  /* if ReleaseLockOnClose was set, unlocks using whatever notification
   * unlock_flag was set.
   */
  unlock_flag = (file->dirty) ? XDMF_DSM_NOTIFY_DATA : XDMF_DSM_NOTIFY_NONE;

  /* Release resources */
  if (file->name) HDfree(file->name);
  HDmemset(file, 0, sizeof(XDMF_dsm_t));
  free(file);

done:
  if (err_occurred) {
    /* Nothing */
  }

  FUNC_LEAVE_NOAPI(ret_value)
}

static herr_t
XDMF_dsm_query(const H5FD_t UNUSED *_file, unsigned long *flags /* out */)
{
#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI_NOINIT_NOERR
#else
  FUNC_ENTER_NOAPI_NOINIT_NOFUNC(XDMF_dsm_query)
#endif

  /* Set the VFL feature flags that this driver supports */
  if (flags) {
    *flags = 0;
    *flags |= H5FD_FEAT_AGGREGATE_METADATA;  /* OK to aggregate metadata allocations */
    *flags |= H5FD_FEAT_AGGREGATE_SMALLDATA; /* OK to aggregate "small" raw data allocations */
#ifdef H5FD_FEAT_HAS_MPI
    *flags |= H5FD_FEAT_HAS_MPI;             /* This driver uses MPI */
#endif
#ifdef H5FD_FEAT_ALLOCATE_EARLY
    *flags |= H5FD_FEAT_ALLOCATE_EARLY;      /* Allocate space early instead of late */
#endif

  } /* end if */

  FUNC_LEAVE_NOAPI(SUCCEED)
}

static haddr_t
XDMF_dsm_get_eoa(const H5FD_t *_file, H5FD_mem_t UNUSED type)
{
  const XDMF_dsm_t *file = (const XDMF_dsm_t*) _file;

#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI_NOINIT_NOERR
#else
  FUNC_ENTER_NOAPI_NOINIT_NOFUNC(XDMF_dsm_get_eoa)
#endif

  assert(file);
  assert(XDMF_DSM == file->pub.driver_id);

  FUNC_LEAVE_NOAPI(file->eoa)
}

static herr_t
XDMF_dsm_set_eoa(H5FD_t * _file, H5FD_mem_t UNUSED type, haddr_t addr)
{
  XDMF_dsm_t *file = (XDMF_dsm_t*) _file;
  herr_t ret_value = SUCCEED; /* Return value */
  int mpi_code;
  herr_t dsm_code = SUCCEED;

#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI_NOINIT
#else
  FUNC_ENTER_NOAPI_NOINIT(XDMF_dsm_set_eoa)
#endif

  assert(file);
  assert(XDMF_DSM == file->pub.driver_id);

  if (ADDR_OVERFLOW(addr))
    HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "address overflow")

  file->eoa = addr;

  // If EoA is smaller than the end of the file the address is within page boundaries
  // If this is not the case then we need to reserve more pages.
  if ((file->start + file->eoa) > file->end &&
      (((XdmfDSMBuffer *)xdmf_dsm_get_manager())->GetDsmType() == XDMF_DSM_TYPE_BLOCK_CYCLIC ||
      ((XdmfDSMBuffer *)xdmf_dsm_get_manager())->GetDsmType() == XDMF_DSM_TYPE_BLOCK_RANDOM) &&
      !file->read_only)
  {
    unsigned int pageCount = file->numPages;
    unsigned int * newpages;
    if (dsmBuffer->GetComm()->GetId() == 0)
    {
      // Request additional pages to store data
      dsmBuffer->RequestPages(file->name,
                              file->start + file->eoa - file->end,
                              filePages[file->name],
                              file->numPages,
                              file->start,
                              file->end);
    }

    // If requesting pages resized the pointer
    // Reset the total length to match the new size.
    unsigned int currentLength = dsmBuffer->GetLength();
    dsmBuffer->GetComm()->Broadcast(&currentLength,
                                    sizeof(unsigned int),
                                    0,
                                    XDMF_DSM_INTRA_COMM);
    if (currentLength != dsmBuffer->GetLength()) {
      dsmBuffer->UpdateLength(currentLength);
    }

    dsmBuffer->GetComm()->Broadcast(&file->end,
                                    sizeof(haddr_t),
                                    0,
                                    XDMF_DSM_INTRA_COMM);

    dsmBuffer->GetComm()->Broadcast(&file->numPages,
                                    sizeof(unsigned int),
                                    0,
                                    XDMF_DSM_INTRA_COMM);

    if (pageCount != file->numPages) 
    {
      if (dsmBuffer->GetComm()->GetId() != 0)
      {
        filePages[file->name].clear();
        for (unsigned int i = 0; i < file->numPages; ++i)
        {
          filePages[file->name].push_back(0);
        }
      }

      dsmBuffer->GetComm()->Broadcast(&(filePages[file->name][0]),
                                      sizeof(unsigned int) * file->numPages,
                                      0,
                                      XDMF_DSM_INTRA_COMM);
    }
  }

  file->end = MAX((file->start + file->eoa), file->end);
  file->eof = file->end - file->start;
  fileEOF[file->name] = file->eof;
  if (!file->read_only) {

    if (dsmBuffer->GetComm()->GetId() == 0)
    {
      // Update DSM file description
      if (dsmBuffer->RegisterFile(file->name,
                                  &(filePages[file->name][0]),
                                  file->numPages,
                                  file->start,
                                  file->end) == XDMF_DSM_FAIL)
      {
        dsm_code = FAIL;
      }

    }

    /* Wait for the DSM entry to be updated */
      dsmBuffer->GetComm()->Broadcast(&dsm_code,
                                      sizeof(herr_t),
                                      0,
                                      XDMF_DSM_INTRA_COMM);
    if (SUCCEED != dsm_code)
      HGOTO_ERROR(H5E_VFL, H5E_CANTUPDATE, FAIL, "cannot update DSM entries")
  }
done:
  if (err_occurred) {
    /* Nothing */
  }

  FUNC_LEAVE_NOAPI(ret_value)
}

static haddr_t
XDMF_dsm_get_eof(const H5FD_t *_file)
{
  const XDMF_dsm_t *file = (const XDMF_dsm_t*) _file;

#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI_NOINIT_NOERR
#else
  FUNC_ENTER_NOAPI_NOINIT_NOFUNC(XDMF_dsm_get_eof)
#endif

  assert(file);
  assert(XDMF_DSM == file->pub.driver_id);

  FUNC_LEAVE_NOAPI(MAX(file->eof, file->eoa))
}

static herr_t
XDMF_dsm_read(H5FD_t *_file, H5FD_mem_t UNUSED type, hid_t UNUSED dxpl_id,
    haddr_t addr, size_t size, void *buf /* out */)
{
  XDMF_dsm_t *file = (XDMF_dsm_t*) _file;
  herr_t ret_value = SUCCEED; /* Return value */

#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI_NOINIT
#else
  FUNC_ENTER_NOAPI_NOINIT(XDMF_dsm_read)
#endif

  assert(file);
  assert(XDMF_DSM == file->pub.driver_id);
  assert(buf);

  /* Check for overflow conditions */
  if (HADDR_UNDEF == addr)
    HGOTO_ERROR(H5E_IO, H5E_OVERFLOW, FAIL, "file address overflowed")
  if (REGION_OVERFLOW(addr, size))
    HGOTO_ERROR(H5E_IO, H5E_OVERFLOW, FAIL, "file address overflowed")
  if (addr + size > file->eoa)
    HGOTO_ERROR(H5E_IO, H5E_OVERFLOW, FAIL, "file address overflowed")

  /* Read the part which is before the EOF marker */
  if (addr < file->eof) {
    size_t  nbytes;
    hsize_t temp_nbytes;

    temp_nbytes = file->eof - addr;
    H5_CHECK_OVERFLOW(temp_nbytes,hsize_t,size_t);
    nbytes = MIN(size,(size_t)temp_nbytes);


    if (((XdmfDSMBuffer *)xdmf_dsm_get_manager())->GetDsmType() == XDMF_DSM_TYPE_BLOCK_CYCLIC ||
        ((XdmfDSMBuffer *)xdmf_dsm_get_manager())->GetDsmType() == XDMF_DSM_TYPE_BLOCK_RANDOM)
    {
      xdmf_dsm_read_pages(&(filePages[file->name][0]), file->numPages, addr, nbytes, buf);
      size -= nbytes;
      addr += nbytes;
      buf = (char*) buf + nbytes;
    }
    else if (((XdmfDSMBuffer *)xdmf_dsm_get_manager())->GetDsmType() == XDMF_DSM_TYPE_UNIFORM ||
             ((XdmfDSMBuffer *)xdmf_dsm_get_manager())->GetDsmType() == XDMF_DSM_TYPE_UNIFORM_RANGE)
    {
      /* Read from DSM to BUF */
      if (SUCCEED != xdmf_dsm_read(file->start + addr, nbytes, buf)) {
        HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "cannot read from DSM")
      } else {
        size -= nbytes;
        addr += nbytes;
        buf = (char*) buf + nbytes;
      }
    }
    else
    {
      HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "invalid DSM type")
    }
  }
  /* Read zeros for the part which is after the EOF markers */
  if (size > 0) HDmemset(buf, 0, size);

done:
  if (err_occurred) {
    /* Nothing */
  }

  FUNC_LEAVE_NOAPI(ret_value)
}

static herr_t
XDMF_dsm_write(H5FD_t *_file, H5FD_mem_t type, hid_t UNUSED dxpl_id,
    haddr_t addr, size_t size, const void *buf)
{
  XDMF_dsm_t *file = (XDMF_dsm_t*) _file;
  herr_t ret_value = SUCCEED; /* Return value */

#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI_NOINIT
#else
  FUNC_ENTER_NOAPI_NOINIT(XDMF_dsm_write)
#endif

  assert(file);
  assert(XDMF_DSM == file->pub.driver_id);
  assert(buf);

  if (file->read_only)
    HGOTO_ERROR(H5E_IO, H5E_RESOURCE, FAIL, "cannot write to DSM open in read-only")

  /* Check for overflow conditions */
  if (REGION_OVERFLOW(addr, size))
    HGOTO_ERROR(H5E_IO, H5E_OVERFLOW, FAIL, "file address overflowed")
  if (addr + size > file->eoa)
    HGOTO_ERROR(H5E_IO, H5E_OVERFLOW, FAIL, "file address overflowed")

  /* For now, do not allow dynamic reallocation of the DSM */
  if (addr + size > file->eof)
    HGOTO_ERROR(H5E_IO, H5E_NOSPACE, FAIL, "not enough space in DSM")

  if (((XdmfDSMBuffer *)xdmf_dsm_get_manager())->GetDsmType() == XDMF_DSM_TYPE_BLOCK_CYCLIC ||
      ((XdmfDSMBuffer *)xdmf_dsm_get_manager())->GetDsmType() == XDMF_DSM_TYPE_BLOCK_RANDOM)
  {
    xdmf_dsm_write_pages(&(filePages[file->name][0]), file->numPages, addr, size, buf);
  }
  else if (((XdmfDSMBuffer *)xdmf_dsm_get_manager())->GetDsmType() == XDMF_DSM_TYPE_UNIFORM ||
           ((XdmfDSMBuffer *)xdmf_dsm_get_manager())->GetDsmType() == XDMF_DSM_TYPE_UNIFORM_RANGE)
  {
    /* Write from BUF to DSM */
    if (SUCCEED != xdmf_dsm_write(file->start + addr, size, buf))
      HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "cannot write to DSM")
  }
  else
  {
    HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "invalid DSM type")
  }
  /* Set dirty flag so that we know someone has written something */
  file->dirty = TRUE;
done:
  if (err_occurred) {
    /* Nothing */
  }

  FUNC_LEAVE_NOAPI(ret_value)
}

static herr_t
XDMF_dsm_flush(H5FD_t *_file, hid_t UNUSED dxpl_id, unsigned UNUSED closing)
{
  herr_t ret_value = SUCCEED; /* Return value */

#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI_NOINIT_NOERR
#else
  FUNC_ENTER_NOAPI_NOINIT_NOFUNC(XDMF_dsm_flush)
#endif

  FUNC_LEAVE_NOAPI(ret_value)
}

static int
XDMF_dsm_mpi_rank(const H5FD_t *_file)
{
  const XDMF_dsm_t *file = (const XDMF_dsm_t*) _file;

#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI_NOINIT_NOERR
#else
  FUNC_ENTER_NOAPI_NOINIT_NOFUNC(XDMF_dsm_mpi_rank)
#endif

  assert(file);
  assert(XDMF_DSM == file->pub.driver_id);

  FUNC_LEAVE_NOAPI(dsmBuffer->GetComm()->GetId())
}

static int
XDMF_dsm_mpi_size(const H5FD_t *_file)
{
  const XDMF_dsm_t *file = (const XDMF_dsm_t*) _file;

#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI_NOINIT_NOERR
#else
  FUNC_ENTER_NOAPI_NOINIT_NOFUNC(XDMF_dsm_mpi_size)
#endif

  assert(file);
  assert(XDMF_DSM == file->pub.driver_id);

  FUNC_LEAVE_NOAPI(dsmBuffer->GetComm()->GetIntraSize())
}

static MPI_Comm
XDMF_dsm_communicator(const H5FD_t *_file)
{
  const XDMF_dsm_t *file = (const XDMF_dsm_t*) _file;

#if H5_VERSION_GE(1,8,9)
  FUNC_ENTER_NOAPI_NOINIT_NOERR
#else
  FUNC_ENTER_NOAPI_NOINIT_NOFUNC(XDMF_dsm_communicator)
#endif

  assert(file);
  assert(XDMF_DSM == file->pub.driver_id);

  FUNC_LEAVE_NOAPI(dsmBuffer->GetComm()->GetIntraComm())
}

}

void*
xdmf_dsm_get_manager()
{
  void *ret_value = NULL;
  if (dsmBuffer) ret_value = static_cast <void*> (dsmBuffer);  
  return(ret_value);
}

herr_t
xdmf_dsm_get_properties(MPI_Comm *intra_comm, void **buf_ptr_ptr, size_t *buf_len_ptr)
{
  if (!dsmBuffer) {
    try {
      XdmfError::message(XdmfError::FATAL, "No DSM manager found");
    }
    catch (XdmfError & e) {
      return FAIL;
    }
  }

  if (dsmBuffer->GetIsServer()) {
    if (buf_ptr_ptr) *buf_ptr_ptr =
        dsmBuffer->GetDataPointer();
    if (buf_len_ptr) *buf_len_ptr =
        dsmBuffer->GetLength();
  } else {
    if (buf_ptr_ptr) *buf_ptr_ptr = NULL;
    if (buf_len_ptr) *buf_len_ptr = 0;
  }

  return(SUCCEED);
}

void
xdmf_dsm_set_manager(void *manager)
{
  dsmBuffer = static_cast <XdmfDSMBuffer*> (manager);
}

herr_t
xdmf_dsm_free()
{
  if (dsmBuffer) {
    /* probably not required, since the autoallocation is not on
    */
  }

  return(SUCCEED);
}

hbool_t
xdmf_dsm_is_server()
{
  hbool_t ret_value = TRUE;

  if (!dsmBuffer) {
    XdmfError::message(XdmfError::FATAL, "No DSM manager found");
  }

  ret_value = dsmBuffer->GetIsServer();

  return(ret_value);
}

herr_t
xdmf_dsm_set_options(unsigned long flags)
{
  if (!dsmBuffer) {
    try {
      XdmfError::message(XdmfError::FATAL, "No DSM manager found");
    }
    catch (XdmfError & e) {
      return FAIL;
    }
  }

  // Currently no options to set
  // All options are set via the dsmBuffer during the dsm's creation

  return(SUCCEED);
}

hbool_t
xdmf_dsm_is_connected()
{
  hbool_t ret_value = TRUE;

  if (!dsmBuffer) {
    XdmfError::message(XdmfError::FATAL, "No DSM manager found");
  }

  ret_value = dsmBuffer->GetIsConnected();

  return(ret_value);
}

herr_t
xdmf_dsm_connect()
{
  if (!dsmBuffer) {
    try {
      XdmfError::message(XdmfError::FATAL, "No DSM manager found");
    }
    catch (XdmfError & e) {
      return FAIL;
    }
  }

  // Initialize the connection if it has not been done already
  if (dsmBuffer->GetIsConnected()) {
    try {
      XdmfError::message(XdmfError::FATAL, "Already Connected");
    }
    catch (XdmfError & e) {
      return FAIL;
    }
  }

  try {
    dsmBuffer->Connect();
  }
  catch (XdmfError & e) {
    return FAIL;
  }

  return(SUCCEED);
}

herr_t
xdmf_dsm_lock(char * filename)
{
  if (!dsmBuffer) {
    try {
      XdmfError::message(XdmfError::FATAL, "No DSM manager found");
    }
    catch (XdmfError & e) {
      return FAIL;
    }
  }

  return(SUCCEED);
}

herr_t
xdmf_dsm_unlock(char * filename, unsigned long flag)
{
  if (!dsmBuffer) {
    try {
      XdmfError::message(XdmfError::FATAL, "No DSM manager found");
    }
    catch (XdmfError & e) {
      return FAIL;
    }
  }

  return(SUCCEED);
}


// When writing and reading, we want to provide a list of pages that the file contains.
// The appropriate subsections can be retrieved from the pages this way.
// We may be able to eliminate these calls and just call straight from the buffer in the file driver.

herr_t
xdmf_dsm_read(haddr_t addr, size_t len, void *buf_ptr)
{
  if (!dsmBuffer) {
    try {
      XdmfError::message(XdmfError::FATAL, "No DSM manager found");
    }
    catch (XdmfError & e) {
      return FAIL;
    }
  }

  try {
    dsmBuffer->Get(addr, len, buf_ptr);
  }
  catch (XdmfError & e) {
    return FAIL;
  }
  return(SUCCEED);
}

herr_t
xdmf_dsm_read_pages(unsigned int * pages, unsigned int numPages,  haddr_t addr, size_t len, void *buf_ptr)
{
  if (!dsmBuffer) {
    try {
      XdmfError::message(XdmfError::FATAL, "No DSM manager found");
    }
    catch (XdmfError & e) {
      return FAIL;
    }
  }

  try {
    dsmBuffer->Get(pages, numPages, addr, len, buf_ptr);
  }
  catch (XdmfError & e) {
    return FAIL;
  }
  return(SUCCEED);
}

herr_t
xdmf_dsm_write(haddr_t addr, size_t len, const void *buf_ptr)
{
  if (!dsmBuffer) {
    try {
      XdmfError::message(XdmfError::FATAL, "No DSM manager found");
    }
    catch (XdmfError & e) {
      return FAIL;
    }
  }

  try {
    dsmBuffer->Put(addr, len, buf_ptr);
  }
  catch (XdmfError & e) {
    return FAIL;
  }
  return(SUCCEED);
}

herr_t
xdmf_dsm_write_pages(unsigned int * pages, unsigned int numPages, haddr_t addr, size_t len, const void *buf_ptr)
{
  if (!dsmBuffer) {
    try {
      XdmfError::message(XdmfError::FATAL, "No DSM manager found");
    }
    catch (XdmfError & e) {
      return FAIL;
    }
  }

  try {
    dsmBuffer->Put(pages, numPages, addr, len, buf_ptr);
  }
  catch (XdmfError & e) {
    return FAIL;
  }
  return(SUCCEED);
}
