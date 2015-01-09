/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfDSMDriver.hpp                                                   */
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

/*=========================================================================

  Project                 : H5FDdsm
  Module                  : H5FDdsmDriver.h H5FDdsm.h

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
  Framework Programme (FP7/2007-2013) under grant agreement 225967 âxtMuSEâOB
=========================================================================*/

#ifndef XDMFDSMDRIVER_HPP_
#define XDMFDSMDRIVER_HPP_

// Forward Declarations

// Includes
#include <XdmfDSMBuffer.hpp>
#include <XdmfDSMCommMPI.hpp>
#include <XdmfDSMManager.hpp>
#include <XdmfDSM.hpp>
#include <mpi.h>

#include "H5Ipublic.h"
#include "H5FDpublic.h"

#ifndef H5_HAVE_PARALLEL
  #pragma Error : The XdmfDSM virtual File Driver for HDF5 can only be compiled against an HDF5 library with parallel IO support
#endif

#define XDMF_DSM_NOTIFY_NONE         0x0000
#define XDMF_DSM_NOTIFY_DATA         0x0001 /* this is the default */
#define XDMF_DSM_NOTIFY_INFORMATION  0x0002
/* Internal notifications */
#define XDMF_DSM_NOTIFY_WAIT         0x0003
#define XDMF_DSM_NOTIFY_CONNECTED    0x0004
#define XDMF_DSM_NOTIFY_DISCONNECTED 0x0005
/* User notifications */
#define XDMF_DSM_NOTIFY_USER         0x0010 

#define XDMF_DSM (XDMF_dsm_init())

#define IS_XDMF_DSM(f) /* (H5F_t *f) */    \
    (XDMF_DSM==H5F_DRIVER_ID(f))

#include "H5FDmpiposix.h"

#ifndef H5FD_FEAT_HAS_MPI
  // This is a temporary solution to allow interface with standard hdf5 in addition to hdf5vfd
  // Hopefully a better solution will be made in the future
  #define XDMF_dsm_init H5FD_mpiposix_init
  //#pragma message(": warning Xdmf: H5FD mpiposix file driver replaced to enable dsm compatibility with hdf5")
#endif

extern "C" {
  XDMFDSM_EXPORT hid_t  XDMF_dsm_init(void);
#if H5_VERSION_GE(1,9,0)
  XDMFDSM_EXPORT herr_t XDMF_dsm_term(void);
#else
  XDMFDSM_EXPORT void XDMF_dsm_term(void);
#endif
  // lock and unlock are currently disabled the user has to make allowances
  // to prevent race conditions
  XDMFDSM_EXPORT herr_t XDMF_dsm_lock(void);
  XDMFDSM_EXPORT herr_t XDMF_dsm_unlock(unsigned long flag);
  // Currently no options to set
  XDMFDSM_EXPORT herr_t XDMF_dsm_set_options(unsigned long flags);
  XDMFDSM_EXPORT herr_t XDMF_dsm_set_manager(void *manager);
  XDMFDSM_EXPORT herr_t XDMFH5Pset_fapl_dsm(hid_t fapl_id, MPI_Comm intra_comm,
      void *local_buf_ptr, size_t local_buf_len);
  XDMFDSM_EXPORT herr_t XDMFH5Pget_fapl_dsm(hid_t fapl_id, MPI_Comm *intra_comm /* out */,
      void **local_buf_ptr_ptr /* out */, size_t *local_buf_len_ptr /* out */);




  XDMFDSM_EXPORT void   *xdmf_dsm_get_manager();
  XDMFDSM_EXPORT herr_t  xdmf_dsm_get_properties(MPI_Comm *intra_comm,
      void **buf_ptr_ptr, size_t *buf_len_ptr);
  XDMFDSM_EXPORT void    xdmf_dsm_set_manager(void *manager);

/*
  // Probably a bad idea to create managers automatically for the non-threaded version
  XDMFDSM_EXPORT herr_t  xdmf_dsm_alloc(MPI_Comm intra_comm, void *buf_ptr, size_t buf_len);
*/
  XDMFDSM_EXPORT herr_t  xdmf_dsm_free();

  XDMFDSM_EXPORT hbool_t xdmf_dsm_is_server();
  // Currently no options to set
  XDMFDSM_EXPORT herr_t  xdmf_dsm_set_options(unsigned long flags);

  XDMFDSM_EXPORT hbool_t xdmf_dsm_is_connected();
  XDMFDSM_EXPORT herr_t  xdmf_dsm_connect();

  XDMFDSM_EXPORT herr_t  xdmf_dsm_update_entry(haddr_t start, haddr_t end);
  XDMFDSM_EXPORT herr_t  xdmf_dsm_get_entry(haddr_t *start_ptr, haddr_t *end_ptr);

  XDMFDSM_EXPORT herr_t  xdmf_dsm_lock();
  XDMFDSM_EXPORT herr_t  xdmf_dsm_unlock(unsigned long flag);

  XDMFDSM_EXPORT herr_t  xdmf_dsm_read(haddr_t addr, size_t len, void *buf_ptr);
  XDMFDSM_EXPORT herr_t  xdmf_dsm_write(haddr_t addr, size_t len, const void *buf_ptr);
}

#endif /* XDMFDSMDRIVER_HPP_ */
