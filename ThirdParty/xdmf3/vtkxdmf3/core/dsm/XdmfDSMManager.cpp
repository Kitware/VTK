/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfDSMManager.cpp                                                  */
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
  Module                  : H5FDdsmManger.cxx

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

#include <XdmfDSMManager.hpp>
#include <XdmfDSMBuffer.hpp>
#include <XdmfDSMCommMPI.hpp>
#include <XdmfError.hpp>
#include <mpi.h>

#ifndef _WIN32
  #include <unistd.h>
#endif

XdmfDSMManager::XdmfDSMManager()
{
  this->MpiComm                 = MPI_COMM_NULL;
  this->UpdatePiece             = 0;
  this->UpdateNumPieces         = 0;
  this->LocalBufferSizeMBytes   = 128;

  this->DsmBuffer               = NULL;
  this->DsmComm                 = NULL;
  this->IsServer                = true;
  this->DsmType                 = XDMF_DSM_TYPE_UNIFORM;
  this->BlockLength             = XDMF_DSM_DEFAULT_BLOCK_LENGTH;
  this->InterCommType           = XDMF_DSM_COMM_MPI;
}

XdmfDSMManager::~XdmfDSMManager()
{
  this->Destroy();
}

void
XdmfDSMManager::Connect(bool persist)
{
  int status;

  do {
    try {
      status = this->DsmBuffer->GetComm()->Connect();
    }
    catch (XdmfError & e) {
      throw e;
    }
    if (status == MPI_SUCCESS) {
      dynamic_cast<XdmfDSMBuffer*> (this->DsmBuffer)->SetIsConnected(true);
      try {
        this->DsmBuffer->ReceiveInfo();
      }
      catch (XdmfError & e) {
        throw e;
      }
    }
    else {
#ifdef _WIN32
  Sleep(1000);
  // Since windows has a different sleep command
#else
  sleep(1);
#endif
    }
  } while (persist && (status != MPI_SUCCESS));
}

void
XdmfDSMManager::Create(int startId, int endId)
{
  if (!this->DsmBuffer) {

    MPI_Comm_size(this->MpiComm, &this->UpdateNumPieces);
    MPI_Comm_rank(this->MpiComm, &this->UpdatePiece);
    //
    // Create DSM communicator
    //
    switch (this->GetInterCommType()) {
    case XDMF_DSM_COMM_MPI:
      this->DsmComm = new XdmfDSMCommMPI();
      break;
    default:
      try {
        XdmfError::message(XdmfError::FATAL, "DSM communication type not supported");
      }
      catch (XdmfError & e) {
        throw e;
      }
    }
    this->DsmComm->DupComm(this->MpiComm);
    this->DsmComm->Init();
    //
    // Create the DSM buffer
    //
    this->DsmBuffer = new XdmfDSMBuffer();
    //
    this->DsmBuffer->SetIsServer(this->IsServer);
    // Uniform Dsm : every node has a buffer the same size. (Addresses are sequential)
    long length = (long) (this->GetLocalBufferSizeMBytes())*1024LU*1024LU;
    switch (this->DsmType) {
    case XDMF_DSM_TYPE_UNIFORM:
    case XDMF_DSM_TYPE_UNIFORM_RANGE:
      this->DsmBuffer->ConfigureUniform(this->DsmComm, length, startId, endId);
      break;
    case XDMF_DSM_TYPE_BLOCK_CYCLIC:
      this->DsmBuffer->ConfigureUniform(this->DsmComm, length, startId, endId, this->BlockLength, false);
      break;
    case XDMF_DSM_TYPE_BLOCK_RANDOM:
      this->DsmBuffer->ConfigureUniform(this->DsmComm, length, startId, endId, this->BlockLength, true);
      break;
    default:
      try {
        XdmfError(XdmfError::FATAL, "DSM configuration type not supported");
      }
      catch (XdmfError & e) {
        throw e;
      }
    }
  }
}

void
XdmfDSMManager::Destroy()
{
  // Watch out that all processes have empty message queues
  // Should be already done during the disconnection
  if (this->DsmBuffer) {
    delete this->DsmBuffer;
    this->DsmBuffer = NULL;
    // Will be replaced by an Xdmf version
    // H5FD_dsm_set_manager(NULL);
  }
  if (this->DsmComm) {
    delete this->DsmComm;
    this->DsmComm = NULL;
  }
}

void
XdmfDSMManager::Disconnect()
{
  // Disconnecting is done manually
  try {
    this->DsmBuffer->GetComm()->Disconnect();
  }
  catch (XdmfError & e) {
    throw e;
  }
  dynamic_cast<XdmfDSMBuffer*> (this->DsmBuffer)->SetIsConnected(false);
}

long
XdmfDSMManager::GetBlockLength()
{
  return this->BlockLength;
}

XdmfDSMBuffer *
XdmfDSMManager::GetDsmBuffer()
{
  return this->DsmBuffer;
}

int
XdmfDSMManager::GetDsmType()
{
  return this->DsmType;
}

int
XdmfDSMManager::GetInterCommType()
{
  return this->InterCommType;
}

bool
XdmfDSMManager::GetIsConnected()
{
  if (this->DsmBuffer) {
    return this->DsmBuffer->GetIsConnected();
  }
  else {
    return false;
  }
}

bool
XdmfDSMManager::GetIsServer()
{
  return this->IsServer;
}

unsigned int
XdmfDSMManager::GetLocalBufferSizeMBytes()
{
  return this->LocalBufferSizeMBytes;
}

MPI_Comm
XdmfDSMManager::GetMpiComm()
{
  return this->MpiComm;
}

int
XdmfDSMManager::GetUpdatePiece()
{
  return this->UpdatePiece;
}

int
XdmfDSMManager::GetUpdateNumPieces()
{
  return this->UpdateNumPieces;
}

void
XdmfDSMManager::SetBlockLength(long newSize)
{
  this->BlockLength = newSize;
}

void
XdmfDSMManager::SetDsmBuffer(XdmfDSMBuffer * newBuffer)
{
  this->DsmBuffer = newBuffer;
}

void
XdmfDSMManager::SetDsmType(int newType)
{
  this->DsmType = newType;
}

void
XdmfDSMManager::SetIsServer(bool newStatus)
{
  this->IsServer = newStatus;
}

void
XdmfDSMManager::SetInterCommType(int newType)
{
  this->InterCommType = newType;
}

void
XdmfDSMManager::SetLocalBufferSizeMBytes(unsigned int newSize)
{
  this->LocalBufferSizeMBytes = newSize;
}

void
XdmfDSMManager::SetMpiComm(MPI_Comm comm)
{
  if (comm != this->MpiComm) {
    this->MpiComm = comm;
    if (this->MpiComm != MPI_COMM_NULL) {
      MPI_Comm_size(this->MpiComm, &this->UpdateNumPieces);
      MPI_Comm_rank(this->MpiComm, &this->UpdatePiece);
    }
  }
}
