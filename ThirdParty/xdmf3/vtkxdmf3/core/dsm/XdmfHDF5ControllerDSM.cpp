/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfHDF5ControllerDSM.cpp                                           */
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
#endif
#include <H5public.h>
#include <hdf5.h>
#include "XdmfArray.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfHDF5ControllerDSM.hpp"
#include "XdmfDSMManager.hpp"
#include "XdmfDSMBuffer.hpp"
#include "XdmfDSMCommMPI.hpp"
#include "XdmfDSMDriver.hpp"
#include "XdmfError.hpp"

#ifdef XDMF_BUILD_DSM_THREADS

shared_ptr<XdmfHDF5ControllerDSM>
XdmfHDF5ControllerDSM::New(const std::string & hdf5FilePath,
                           const std::string & dataSetPath,
                           const shared_ptr<const XdmfArrayType> type,
                           const std::vector<unsigned int> & start,
                           const std::vector<unsigned int> & stride,
                           const std::vector<unsigned int> & dimensions,
                           const std::vector<unsigned int> & datspaceDimensions,
                           H5FDdsmBuffer * const dsmBuffer)
{
  shared_ptr<XdmfHDF5ControllerDSM> 
    p(new XdmfHDF5ControllerDSM(hdf5FilePath,
                                dataSetPath,
                                type,
                                start,
                                stride,
                                dimensions,
                                datspaceDimensions,
                                dsmBuffer));
  return p;
}

shared_ptr<XdmfHDF5ControllerDSM>
XdmfHDF5ControllerDSM::New(const std::string & hdf5FilePath,
                           const std::string & dataSetPath,
                           const shared_ptr<const XdmfArrayType> type,
                           const std::vector<unsigned int> & start,
                           const std::vector<unsigned int> & stride,
                           const std::vector<unsigned int> & dimensions,
                           const std::vector<unsigned int> & datspaceDimensions,
                           MPI_Comm comm,
                           unsigned int bufferSize)
{
  shared_ptr<XdmfHDF5ControllerDSM>
    p(new XdmfHDF5ControllerDSM(hdf5FilePath,
                                dataSetPath,
                                type,
                                start,
                                stride,
                                dimensions,
                                datspaceDimensions,
                                comm,
                                bufferSize));
  return p;
}

#endif

// Server/ nonthreaded versions
shared_ptr<XdmfHDF5ControllerDSM>
XdmfHDF5ControllerDSM::New(const std::string & hdf5FilePath,
                           const std::string & dataSetPath,
                           const shared_ptr<const XdmfArrayType> type,
                           const std::vector<unsigned int> & start,
                           const std::vector<unsigned int> & stride,
                           const std::vector<unsigned int> & dimensions,
                           const std::vector<unsigned int> & datspaceDimensions,
                           XdmfDSMBuffer * const dsmBuffer)
{
  shared_ptr<XdmfHDF5ControllerDSM>
    p(new XdmfHDF5ControllerDSM(hdf5FilePath,
                                dataSetPath,
                                type,
                                start,
                                stride,
                                dimensions,
                                datspaceDimensions,
                                dsmBuffer));
  return p;
}

shared_ptr<XdmfHDF5ControllerDSM>
XdmfHDF5ControllerDSM::New(const std::string & hdf5FilePath,
                           const std::string & dataSetPath,
                           const shared_ptr<const XdmfArrayType> type,
                           const std::vector<unsigned int> & start,
                           const std::vector<unsigned int> & stride,
                           const std::vector<unsigned int> & dimensions,
                           const std::vector<unsigned int> & datspaceDimensions,
                           MPI_Comm comm,
                           unsigned int bufferSize,
                           int startCoreIndex,
                           int endCoreIndex)
{
  shared_ptr<XdmfHDF5ControllerDSM>
    p(new XdmfHDF5ControllerDSM(hdf5FilePath,
                                dataSetPath,
                                type,
                                start,
                                stride,
                                dimensions,
                                datspaceDimensions,
                                comm,
                                bufferSize,
                                startCoreIndex,
                                endCoreIndex));
  return p;
}

#ifdef XDMF_BUILD_DSM_THREADS

XdmfHDF5ControllerDSM::XdmfHDF5ControllerDSM(const std::string & hdf5FilePath,
                                             const std::string & dataSetPath,
                                             const shared_ptr<const XdmfArrayType> type,
                                             const std::vector<unsigned int> & start,
                                             const std::vector<unsigned int> & stride,
                                             const std::vector<unsigned int> & dimensions,
                                             const std::vector<unsigned int> & dataspaceDimensions,
                                             H5FDdsmBuffer * const dsmBuffer) :
  XdmfHDF5Controller(hdf5FilePath, 
                     dataSetPath, 
                     type, 
                     start,
                     stride,
                     dimensions, 
                     dataspaceDimensions),
  mDSMManager(NULL),
  mDSMBuffer(dsmBuffer),
  mDSMServerBuffer(NULL),
  mDSMServerManager(NULL),
  mWorkerComm(MPI_COMM_NULL),
  mServerMode(false)
{
}

XdmfHDF5ControllerDSM::XdmfHDF5ControllerDSM(const std::string & hdf5FilePath,
                                             const std::string & dataSetPath,
                                             const shared_ptr<const XdmfArrayType> type,
                                             const std::vector<unsigned int> & start,
                                             const std::vector<unsigned int> & stride,
                                             const std::vector<unsigned int> & dimensions,
                                             const std::vector<unsigned int> & dataspaceDimensions,
                                             MPI_Comm comm,
                                             unsigned int bufferSize) :
  XdmfHDF5Controller(hdf5FilePath,
                     dataSetPath,
                     type,
                     start,
                     stride,
                     dimensions,
                     dataspaceDimensions),
  mDSMServerBuffer(NULL),
  mDSMServerManager(NULL),
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

XdmfHDF5ControllerDSM::XdmfHDF5ControllerDSM(const std::string & hdf5FilePath,
                                             const std::string & dataSetPath,
                                             const shared_ptr<const XdmfArrayType> type,
                                             const std::vector<unsigned int> & start,
                                             const std::vector<unsigned int> & stride,
                                             const std::vector<unsigned int> & dimensions,
                                             const std::vector<unsigned int> & dataspaceDimensions,
                                             XdmfDSMBuffer * const dsmBuffer) :
  XdmfHDF5Controller(hdf5FilePath,
                     dataSetPath,
                     type,
                     start,
                     stride,
                     dimensions,
                     dataspaceDimensions),
#ifdef XDMF_BUILD_DSM_THREADS
  mDSMManager(NULL),
  mDSMBuffer(NULL),
#endif
  mDSMServerBuffer(dsmBuffer),
  mDSMServerManager(NULL),
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

XdmfHDF5ControllerDSM::XdmfHDF5ControllerDSM(const std::string & hdf5FilePath,
                                             const std::string & dataSetPath,
                                             const shared_ptr<const XdmfArrayType> type,
                                             const std::vector<unsigned int> & start,
                                             const std::vector<unsigned int> & stride,
                                             const std::vector<unsigned int> & dimensions,
                                             const std::vector<unsigned int> & dataspaceDimensions,
                                             MPI_Comm comm,
                                             unsigned int bufferSize,
                                             int startCoreIndex,
                                             int endCoreIndex) :
  XdmfHDF5Controller(hdf5FilePath,
                     dataSetPath,
                     type,
                     start,
                     stride,
                     dimensions,
                     dataspaceDimensions),
#ifdef XDMF_BUILD_DSM_THREADS
  mDSMBuffer(NULL),
  mDSMManager(NULL),
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

  int * ServerIds = (int *)calloc((3), sizeof(int));
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
    // If this is set to false then the buffer will attempt to
    // connect to the intercomm for DSM stuff
    mDSMServerManager->SetIsServer(false);
  }
  else {
    // On cores where memory is set up, start the service loop
    // This should iterate infinitely until a value to end the loop is passed
    int returnOpCode;
    mDSMServerBuffer->BufferServiceLoop(&returnOpCode);
  }
}

XdmfHDF5ControllerDSM::~XdmfHDF5ControllerDSM()
{
}

void XdmfHDF5ControllerDSM::deleteManager()
{
#ifdef XDMF_BUILD_DSM_THREADS
  if (mDSMManager != NULL) {
    delete mDSMManager;
  }
#endif
  if (mDSMServerManager != NULL) {
    delete mDSMServerManager;
  }
}

std::string XdmfHDF5ControllerDSM::getName() const
{
  return "HDFDSM";
}

#ifdef XDMF_BUILD_DSM_THREADS

H5FDdsmBuffer * XdmfHDF5ControllerDSM::getBuffer()
{
  return mDSMBuffer;
}

H5FDdsmManager * XdmfHDF5ControllerDSM::getManager()
{
  return mDSMManager;
}

#endif

XdmfDSMBuffer * XdmfHDF5ControllerDSM::getServerBuffer()
{
  return mDSMServerBuffer;
}

XdmfDSMManager * XdmfHDF5ControllerDSM::getServerManager()
{
  return mDSMServerManager;
}

bool XdmfHDF5ControllerDSM::getServerMode() const
{
  return mServerMode;
}

MPI_Comm XdmfHDF5ControllerDSM::getWorkerComm()
{
  MPI_Comm returnComm = MPI_COMM_NULL;
  if (mWorkerComm != MPI_COMM_NULL) {
    MPI_Comm_dup(mWorkerComm, &returnComm);
  }
  return returnComm;
}

void XdmfHDF5ControllerDSM::setManager(XdmfDSMManager * newManager)
{
  XdmfDSMBuffer * newBuffer = newManager->GetDsmBuffer();
  mDSMServerManager = newManager;
  mDSMServerBuffer = newBuffer;
}

#ifdef XDMF_BUILD_DSM_THREADS

void XdmfHDF5ControllerDSM::setManager(H5FDdsmManager * newManager)
{
  H5FDdsmBuffer * newBuffer = newManager->GetDsmBuffer();
  mDSMManager = newManager;
  mDSMBuffer = newBuffer;
}

#endif

void XdmfHDF5ControllerDSM::setBuffer(XdmfDSMBuffer * newBuffer)
{
  mDSMServerBuffer = newBuffer;
}

#ifdef XDMF_BUILD_DSM_THREADS

void XdmfHDF5ControllerDSM::setBuffer(H5FDdsmBuffer * newBuffer)
{
  mDSMBuffer = newBuffer;
}

#endif

void XdmfHDF5ControllerDSM::setServerMode(bool newMode)
{
  mServerMode = newMode;
}

void XdmfHDF5ControllerDSM::setWorkerComm(MPI_Comm comm)
{
  int status;
#ifndef OPEN_MPI
  if (mWorkerComm != MPI_COMM_NULL) {
    status = MPI_Comm_free(&mWorkerComm);
    if (status != MPI_SUCCESS) {
      XdmfError::message(XdmfError::FATAL, "Failed to disconnect Comm");
    }
  }
#endif
  if (comm != MPI_COMM_NULL) {
    status = MPI_Comm_dup(comm, &mWorkerComm);
    if (status != MPI_SUCCESS) {
      XdmfError::message(XdmfError::FATAL, "Failed to duplicate Comm");
    }
  }
  mDSMServerBuffer->GetComm()->DupComm(comm);
}

void XdmfHDF5ControllerDSM::stopDSM()
{
  if (mServerMode) {
    // Send manually
    for (int i = mDSMServerBuffer->GetStartServerId();
         i <= mDSMServerBuffer->GetEndServerId();
         ++i) {
      mDSMServerBuffer->SendCommandHeader(XDMF_DSM_OPCODE_DONE, i, 0, 0, XDMF_DSM_INTER_COMM);
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Stopping DSM manually only available in server mode.");
  }
}

void XdmfHDF5ControllerDSM::restartDSM()
{
  if (mServerMode) {
    if (mDSMServerBuffer->GetComm()->GetInterId() >=
          mDSMServerBuffer->GetStartServerId() &&
        mDSMServerBuffer->GetComm()->GetInterId() <=
          mDSMServerBuffer->GetEndServerId()) {
      int returnOpCode;
      mDSMServerBuffer->BufferServiceLoop(&returnOpCode);
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Restarting DSM only available in server mode.");
  }
}

void XdmfHDF5ControllerDSM::read(XdmfArray * const array)
{
  // Set file access property list for DSM
  hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);

  // Use DSM driver
  if (mServerMode) {
    if (mWorkerComm != MPI_COMM_NULL) {
      XDMFH5Pset_fapl_dsm(fapl, mWorkerComm, mDSMServerBuffer, 0);
    }
  }
  else {
#ifdef XDMF_BUILD_DSM_THREADS
    H5Pset_fapl_dsm(fapl, MPI_COMM_WORLD, mDSMBuffer, 0);
#else
    XdmfError::message(XdmfError::FATAL, "Error: Threaded DSM not enabled");
#endif
  }

  // Read from DSM Buffer
  XdmfHDF5Controller::read(array, fapl);

  // Close file access property list
  H5Pclose(fapl);
}
