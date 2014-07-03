/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfHDF5WriterDSM.cpp                                               */
/*                                                                           */
/*  Author:                                                                  */
/*     Kenneth Leiter                                                        */
/*     kenneth.leiter@arl.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2011 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

#ifdef XDMF_BUILD_DSM_THREADS
  #include <H5FDdsm.h>
  #include <H5FDdsmManager.h>
  #include <H5FDdsmBuffer.h>
  #include <H5FDdsmBufferService.h>
  #include <H5FDdsmComm.h>
#endif
#include <hdf5.h>
#include <XdmfDSMCommMPI.hpp>
#include <XdmfDSMBuffer.hpp>
#include <XdmfDSMManager.hpp>
#include <XdmfDSMDriver.hpp>
#include "XdmfHDF5ControllerDSM.hpp"
#include "XdmfHDF5WriterDSM.hpp"
#include "XdmfError.hpp"

#ifdef XDMF_BUILD_DSM_THREADS
shared_ptr<XdmfHDF5WriterDSM>
XdmfHDF5WriterDSM::New(const std::string & filePath,
                       H5FDdsmBuffer * const dsmBuffer)
{
  shared_ptr<XdmfHDF5WriterDSM> p(new XdmfHDF5WriterDSM(filePath,
                                                        dsmBuffer));
  return p;
}
#endif

shared_ptr<XdmfHDF5WriterDSM>
XdmfHDF5WriterDSM::New(const std::string & filePath,
                       XdmfDSMBuffer * const dsmBuffer)
{
  shared_ptr<XdmfHDF5WriterDSM> p(new XdmfHDF5WriterDSM(filePath,
                                                        dsmBuffer));
  return p;
}

#ifdef XDMF_BUILD_DSM_THREADS
shared_ptr<XdmfHDF5WriterDSM>
XdmfHDF5WriterDSM::New(const std::string & filePath,
                       MPI_Comm comm,
                       unsigned int bufferSize)
{
  shared_ptr<XdmfHDF5WriterDSM> p(new XdmfHDF5WriterDSM(filePath,
                                                        comm,
                                                        bufferSize));
  return p;
}
#endif

shared_ptr<XdmfHDF5WriterDSM>
XdmfHDF5WriterDSM::New(const std::string & filePath,
                       MPI_Comm comm,
                       unsigned int bufferSize,
                       int startCoreIndex,
                       int endCoreIndex)
{
  shared_ptr<XdmfHDF5WriterDSM> p(new XdmfHDF5WriterDSM(filePath,
                                                        comm,
                                                        bufferSize,
                                                        startCoreIndex,
                                                        endCoreIndex));
  return p;
}

#ifdef XDMF_BUILD_DSM_THREADS
XdmfHDF5WriterDSM::XdmfHDF5WriterDSM(const std::string & filePath,
                                     H5FDdsmBuffer * const dsmBuffer) :
  XdmfHDF5Writer(filePath),
  mDSMManager(NULL),
  mDSMBuffer(dsmBuffer),
  mFAPL(-1),
  mDSMServerManager(NULL),
  mDSMServerBuffer(NULL),
  mWorkerComm(MPI_COMM_NULL),
  mServerMode(false)
{
}

XdmfHDF5WriterDSM::XdmfHDF5WriterDSM(const std::string & filePath,
                                     MPI_Comm comm,
                                     unsigned int bufferSize) :
  XdmfHDF5Writer(filePath),
  mFAPL(-1),
  mDSMServerManager(NULL),
  mDSMServerBuffer(NULL),
  mWorkerComm(MPI_COMM_NULL),
  mServerMode(false)
{
  H5FDdsmManager * newManager = new H5FDdsmManager();
  newManager->SetMpiComm(comm);
  newManager->SetLocalBufferSizeMBytes(bufferSize);
  newManager->SetIsStandAlone(H5FD_DSM_TRUE);
  newManager->Create();

  H5FD_dsm_set_manager(newManager);

  H5FD_dsm_set_options(H5FD_DSM_LOCK_ASYNCHRONOUS);

  H5FDdsmBuffer * newBuffer = newManager->GetDsmBuffer();
  mDSMManager = newManager;
  mDSMBuffer = newBuffer;
}
#endif

// The database/nonthreaded version
XdmfHDF5WriterDSM::XdmfHDF5WriterDSM(const std::string & filePath,
                                     XdmfDSMBuffer * const dsmBuffer) :
  XdmfHDF5Writer(filePath),
#ifdef XDMF_BUILD_DSM_THREADS
  mDSMManager(NULL),
  mDSMBuffer(NULL),
#endif
  mFAPL(-1),
  mDSMServerManager(NULL),
  mDSMServerBuffer(dsmBuffer),
  mServerMode(true)
{
  mWorkerComm = mDSMServerBuffer->GetComm()->GetIntraComm();
  if (xdmf_dsm_get_manager() == NULL) {
    mDSMServerManager = new XdmfDSMManager();
    mDSMServerManager->SetLocalBufferSizeMBytes(mDSMServerBuffer->GetLength());
    mDSMServerManager->SetInterCommType(XDMF_DSM_COMM_MPI);
    mDSMServerManager->SetIsServer(false);
    mDSMServerManager->SetMpiComm(mDSMServerBuffer->GetComm()->GetIntraComm());
    mDSMServerManager->SetDsmBuffer(mDSMServerBuffer);
    XDMF_dsm_set_manager(mDSMServerManager);
  }
  else {
    static_cast<XdmfDSMManager *>(xdmf_dsm_get_manager())->SetDsmBuffer(mDSMServerBuffer);
  }
}

XdmfHDF5WriterDSM::XdmfHDF5WriterDSM(const std::string & filePath,
                                     MPI_Comm comm,
                                     unsigned int bufferSize,
                                     int startCoreIndex,
                                     int endCoreIndex) :
  XdmfHDF5Writer(filePath),
  mFAPL(-1),
#ifdef XDMF_BUILD_DSM_THREADS
  mDSMManager(NULL),
  mDSMBuffer(NULL),
#endif
  mServerMode(true)
{
  int rank, size;

  MPI_Comm_size(comm, &size);
  MPI_Comm_rank(comm, &rank);

  // Negative values will be changed to maximum range
  if (startCoreIndex < 0) {
    startCoreIndex = 0;
  }
  if (endCoreIndex < 0) {
    endCoreIndex = size - 1;
  }

  // Ensure start index is less than end index
  if (startCoreIndex > endCoreIndex) {
    int tempholder = startCoreIndex;
    startCoreIndex = endCoreIndex;
    endCoreIndex = tempholder;
  }

  MPI_Comm serverComm;

  MPI_Group workers, dsmgroup, serversplit, servergroup;

  int * ServerIds = (int *)calloc((endCoreIndex - startCoreIndex + 1), sizeof(int));
  unsigned int index = 0;
  for(int i=startCoreIndex ; i <= endCoreIndex ; ++i) {
    ServerIds[index++] = i;
  }

  MPI_Comm_group(comm, &serversplit);
  MPI_Group_incl(serversplit, index, ServerIds, &servergroup);
  MPI_Comm_create(comm, servergroup, &serverComm);
  MPI_Comm_group(comm, &dsmgroup);
  MPI_Group_excl(dsmgroup, index, ServerIds, &workers);
  MPI_Comm_create(comm, workers, &mWorkerComm);
  cfree(ServerIds);

  // Create the manager

  mDSMServerManager = new XdmfDSMManager();

  mDSMServerManager->SetLocalBufferSizeMBytes(bufferSize);
  mDSMServerManager->SetInterCommType(XDMF_DSM_COMM_MPI);

  MPI_Barrier(comm);

  if (rank >= startCoreIndex && rank <= endCoreIndex) {
    mDSMServerManager->SetMpiComm(serverComm);
    mDSMServerManager->Create();
  }
  else {
    mDSMServerManager->SetMpiComm(mWorkerComm);
    mDSMServerManager->SetIsServer(false);
    mDSMServerManager->Create(startCoreIndex, endCoreIndex);
  }

  XDMF_dsm_set_manager(mDSMServerManager);

  mDSMServerBuffer = mDSMServerManager->GetDsmBuffer();

  mDSMServerBuffer->GetComm()->DupInterComm(comm);
  mDSMServerBuffer->SetIsConnected(true);

  if (startCoreIndex < size) {
    if (rank >= startCoreIndex && rank <= endCoreIndex) {
      mDSMServerManager->GetDsmBuffer()->ReceiveInfo();
    }
    else {
      mDSMServerManager->GetDsmBuffer()->SendInfo();
    }
  }

  MPI_Barrier(comm);

  // Loop needs to be started before anything can be done to the file
  // since the service is what sets up the file

  if (rank < startCoreIndex || rank > endCoreIndex) {
    // Turn off the server designation
    mDSMServerBuffer->SetIsServer(false);
    // If this is set to false then the buffer will attempt to connect
    // to the intercomm for DSM communications
    mDSMServerManager->SetIsServer(false);
  }
  else {
    // On cores where memory is set up, start the service loop
    // This should iterate infinitely until a value to end the loop is passed
    int returnOpCode;
    try {
      mDSMServerBuffer->BufferServiceLoop(&returnOpCode);
    }
    catch (XdmfError e) {
      throw e;
    }
  }
}

XdmfHDF5WriterDSM::~XdmfHDF5WriterDSM()
{
  
}

shared_ptr<XdmfHeavyDataController>
XdmfHDF5WriterDSM::createController(const std::string & hdf5FilePath,
                                    const std::string & dataSetPath,
                                    const shared_ptr<const XdmfArrayType> type,
                                    const std::vector<unsigned int> & start,
                                    const std::vector<unsigned int> & stride,
                                    const std::vector<unsigned int> & dimensions,
                                    const std::vector<unsigned int> & dataspaceDimensions)
{
  if (mDSMServerBuffer != NULL) {
        return XdmfHDF5ControllerDSM::New(hdf5FilePath,
                                      dataSetPath,
                                      type,
                                      start,
                                      stride,
                                      dimensions,
                                      dataspaceDimensions,
                                      mDSMServerBuffer);
  }
#ifdef XDMF_BUILD_DSM_THREADS
  else if (mDSMBuffer != NULL) {
    return XdmfHDF5ControllerDSM::New(hdf5FilePath,
                                      dataSetPath,
                                      type,
                                      start,
                                      stride,
                                      dimensions,
                                      dataspaceDimensions,
                                      mDSMBuffer);
  }
#endif
  else {
    return shared_ptr<XdmfHDF5ControllerDSM>();
  }
}

void XdmfHDF5WriterDSM::deleteManager()
{
#ifdef XDMF_BUILD_DSM_THREADS
  if (mDSMManager != NULL)
  {
    delete mDSMManager;
  }
#endif
  if (mDSMServerManager != NULL)
  {
    closeFile();
    delete mDSMServerManager;
  }
}

void
XdmfHDF5WriterDSM::closeFile()
{
  if(mFAPL >= 0) {
    herr_t status = H5Pclose(mFAPL);
    mFAPL = -1;
  }
  XdmfHDF5Writer::closeFile();
}

#ifdef XDMF_BUILD_DSM_THREADS
H5FDdsmBuffer * XdmfHDF5WriterDSM::getBuffer()
{
  return mDSMBuffer;
}

H5FDdsmManager * XdmfHDF5WriterDSM::getManager()
{
  return mDSMManager;
}
#endif

XdmfDSMBuffer * XdmfHDF5WriterDSM::getServerBuffer()
{
  return mDSMServerBuffer;
}

XdmfDSMManager * XdmfHDF5WriterDSM::getServerManager()
{
  return mDSMServerManager;
}

bool XdmfHDF5WriterDSM::getServerMode()
{
  return mServerMode;
}

MPI_Comm XdmfHDF5WriterDSM::getWorkerComm()
{
  MPI_Comm returnComm;
  int status = MPI_Comm_dup(mWorkerComm, &returnComm);
  return returnComm;
}

void XdmfHDF5WriterDSM::setAllowSetSplitting(bool newAllow)
{
  //overrides to disable the parent version
  XdmfHDF5Writer::setAllowSetSplitting(false); 
}

#ifdef XDMF_BUILD_DSM_THREADS
void XdmfHDF5WriterDSM::setBuffer(H5FDdsmBuffer * newBuffer)
{
  mDSMBuffer = newBuffer;
}
#endif

void XdmfHDF5WriterDSM::setBuffer(XdmfDSMBuffer * newBuffer)
{
  mDSMServerBuffer = newBuffer;
}

#ifdef XDMF_BUILD_DSM_THREADS
void XdmfHDF5WriterDSM::setManager(H5FDdsmManager * newManager)
{
  H5FDdsmBuffer * newBuffer = newManager->GetDsmBuffer();
  mDSMManager = newManager;
  mDSMBuffer = newBuffer;
}
#endif

void XdmfHDF5WriterDSM::setManager(XdmfDSMManager * newManager)
{
  XdmfDSMBuffer * newBuffer = newManager->GetDsmBuffer();
  mDSMServerManager = newManager;
  mDSMServerBuffer = newBuffer;
}

void XdmfHDF5WriterDSM::setServerMode(bool newMode)
{
  mServerMode = newMode;
}

void XdmfHDF5WriterDSM::setWorkerComm(MPI_Comm comm)
{
  int status;
#ifndef OPEN_MPI
  if (mWorkerComm != MPI_COMM_NULL) {
    status = MPI_Comm_free(&mWorkerComm);
    if (status != MPI_SUCCESS) {
      try {
        XdmfError::message(XdmfError::FATAL, "Failed to disconnect Comm");
      }
      catch (XdmfError e) {
        throw e;
      }
    }
  }
#endif
  if (comm != MPI_COMM_NULL) {
    status = MPI_Comm_dup(comm, &mWorkerComm);
    if (status != MPI_SUCCESS) {
      try {
        XdmfError::message(XdmfError::FATAL, "Failed to duplicate Comm");
      }
      catch (XdmfError e) {
        throw e;
      }
    }
  }
  mDSMServerBuffer->GetComm()->DupComm(comm);
}

void XdmfHDF5WriterDSM::stopDSM()
{
  if (mServerMode) {
    // Send manually
    for (int i = mDSMServerBuffer->GetStartServerId();
         i <= mDSMServerBuffer->GetEndServerId();
         ++i) {
      try {
        mDSMServerBuffer->SendCommandHeader(XDMF_DSM_OPCODE_DONE, i, 0, 0, XDMF_DSM_INTER_COMM);
      }
      catch (XdmfError e) {
        throw e;
      }
    }
  }
  else {
    try {
      XdmfError::message(XdmfError::FATAL, "Error: Stopping DSM manually only available in server mode.");
    }
    catch (XdmfError e) {
      throw e;
    }
  }
}

void XdmfHDF5WriterDSM::restartDSM()
{
  if (mServerMode) {
    if (mDSMServerBuffer->GetComm()->GetInterId() >=
          mDSMServerBuffer->GetStartServerId() &&
        mDSMServerBuffer->GetComm()->GetInterId() <=
          mDSMServerBuffer->GetEndServerId()) {
      int returnOpCode;
      try {
        mDSMServerBuffer->BufferServiceLoop(&returnOpCode);
      }
      catch (XdmfError e) {
        throw e;
      }
    }
  }
  else {
    try {
      XdmfError::message(XdmfError::FATAL, "Error: Restarting DSM only available in server mode.");
    }
    catch (XdmfError e) {
      throw e;
    }
  }
}

void 
XdmfHDF5WriterDSM::openFile()
{
  if(mFAPL >= 0) {
    this->closeFile();
  }

  // Set file access property list for DSM
  mFAPL = H5Pcreate(H5P_FILE_ACCESS);

  if (mServerMode) {
    if (mWorkerComm != MPI_COMM_NULL) {
      XDMFH5Pset_fapl_dsm(mFAPL, mWorkerComm, mDSMServerBuffer, 0);
    }
  }
  else {
#ifdef XDMF_BUILD_DSM_THREADS
    H5Pset_fapl_dsm(mFAPL, MPI_COMM_WORLD, mDSMBuffer, 0);
#else
    try {
      XdmfError::message(XdmfError::FATAL, "Error: Threaded DSM not enabled.");
    }
    catch (XdmfError e) {
      throw e;
    }
#endif
  }
  XdmfHDF5Writer::openFile(mFAPL);
}

void XdmfHDF5WriterDSM::visit(XdmfArray & array,
                              const shared_ptr<XdmfBaseVisitor>)
{
  bool closeFAPL = false;

  if(mFAPL < 0) {
    // Set file access property list for DSM
    mFAPL = H5Pcreate(H5P_FILE_ACCESS);
    // Use DSM driver
    if (mServerMode) {
      if (mWorkerComm != MPI_COMM_NULL) {
        XDMFH5Pset_fapl_dsm(mFAPL, mWorkerComm, mDSMServerBuffer, 0);
      }
    }
    else {
#ifdef XDMF_BUILD_DSM_THREADS
      H5Pset_fapl_dsm(mFAPL, MPI_COMM_WORLD, mDSMBuffer, 0);
#else
      try {
        XdmfError::message(XdmfError::FATAL, "Error: Threaded DSM not enabled.");
      }
      catch (XdmfError e) {
        throw e;
      }
#endif
    }

    closeFAPL = true;
  }

  // Write to DSM Buffer
  this->write(array, mFAPL);

  if(closeFAPL) {
    // Close file access property list
    herr_t status = H5Pclose(mFAPL);
    mFAPL = -1;
  }

}
