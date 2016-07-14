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

#include <H5public.h>
#include <hdf5.h>
#include "XdmfArray.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfHDF5ControllerDSM.hpp"
#include "XdmfDSMBuffer.hpp"
#include "XdmfDSMCommMPI.hpp"
#include "XdmfDSMDriver.hpp"
#include "XdmfError.hpp"

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
                           int endCoreIndex,
                           std::string applicationName)
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
                                endCoreIndex,
                                applicationName));
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
                           unsigned int blockSize,
                           double resizeFactor,
                           int startCoreIndex,
                           int endCoreIndex,
                           std::string applicationName)
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
                                blockSize,
                                resizeFactor,
                                startCoreIndex,
                                endCoreIndex,
                                applicationName));
  return p;
}

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
  mDSMServerBuffer(dsmBuffer),
  mServerMode(true)
{
  mWorkerComm = mDSMServerBuffer->GetComm()->GetIntraComm();
  if (xdmf_dsm_get_manager() == NULL) {
    XDMF_dsm_set_manager(mDSMServerBuffer);
  }
  else {
    xdmf_dsm_set_manager(mDSMServerBuffer);
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
                                             int endCoreIndex,
                                             std::string applicationName) :
  XdmfHDF5Controller(hdf5FilePath,
                     dataSetPath,
                     type,
                     start,
                     stride,
                     dimensions,
                     dataspaceDimensions),
  mServerMode(true)

{
  int rank, size;
#ifdef XDMF_DSM_IS_CRAY

  MPI_Comm InterComm = comm;
  // Cray needs to be launched via the colon notation so that it
  // can properly create a merged communicator

  MPI_Comm_size(comm, &size);
  MPI_Comm_rank(comm, &rank);

  int currentCore = 0;
  int * checkstatus = new int[size]();
  int localCheck = 0;

  char * coreTag;
  int tagSize = 0;

  std::vector<std::pair<std::string, unsigned int> > newStructure;

  std::vector<int> coreSplit;
  unsigned int splitid = 0;

  int * splitIds;
  unsigned int splitsize = 0;

  while (currentCore < size)
  {
    if (rank == currentCore)
    {
      tagSize = applicationName.size();
    }
    MPI_Bcast(&tagSize, 1, MPI_INT, currentCore, comm);
    coreTag = new char[tagSize+1]();

    if (rank == currentCore)
    {
      strcpy(coreTag, applicationName.c_str());
    }
    MPI_Bcast(coreTag, tagSize, MPI_CHAR, currentCore, comm);

    coreTag[tagSize] = 0;

    if (strcmp(coreTag, applicationName.c_str()) == 0)
    {
      localCheck = 1;
    }
    else
    {
      localCheck = 0;
    }

    checkstatus[rank] = localCheck;

    MPI_Allgather(&localCheck, 1, MPI_INT,
                  checkstatus, 1, MPI_INT,
                  comm);

    bool insplit = false;
    while (checkstatus[currentCore])
    {
      if (rank == currentCore)
      {
        insplit = true;
      }
      coreSplit.push_back(currentCore);
      ++currentCore;
      if (currentCore >= size)
      {
        break;
      }
    }
    if (insplit)
    {
      splitIds = (int *)calloc(coreSplit.size(), sizeof(int));
      memcpy(splitIds, &(coreSplit[0]), coreSplit.size() * sizeof(int));
      splitsize = coreSplit.size();
    }
    newStructure.push_back(std::pair<std::string, unsigned int>(std::string(coreTag), coreSplit.size()));
    coreSplit.clear();
    ++splitid;
  }


  // Use MPI_Comm_split
  MPI_Group IntraGroup, InterGroup;
  MPI_Comm IntraComm;
  MPI_Comm_group(comm, &InterGroup);
  MPI_Group_incl(InterGroup, splitsize, splitIds, &IntraGroup);
  MPI_Comm_create(comm, IntraGroup, &IntraComm);
  cfree(splitIds);

  int intraid = 0;
  int intrasize = 0;

  MPI_Comm_rank(IntraComm, &intraid);
  MPI_Comm_size(IntraComm, &intrasize);

  comm = IntraComm;
#endif

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
  mDSMServerBuffer = new XdmfDSMBuffer();

  mDSMServerBuffer->SetLocalBufferSizeMBytes(bufferSize);
  mDSMServerBuffer->SetInterCommType(XDMF_DSM_COMM_MPI);
  mDSMServerBuffer->SetDsmType(XDMF_DSM_TYPE_UNIFORM);
  if (rank >= startCoreIndex && rank <= endCoreIndex)
  {
    mDSMServerBuffer->GetComm()->SetApplicationName("Server");
  }
  else
  {
    mDSMServerBuffer->GetComm()->SetApplicationName(applicationName);
  }

  if (rank >= startCoreIndex && rank <= endCoreIndex) {
    mDSMServerBuffer->Create(serverComm);
  }
  else {
    mDSMServerBuffer->Create(mWorkerComm, startCoreIndex, endCoreIndex);
  }

  XDMF_dsm_set_manager(mDSMServerBuffer);

#ifdef XDMF_DSM_IS_CRAY
  mDSMServerBuffer->GetComm()->DupInterComm(InterComm);
#else
  mDSMServerBuffer->GetComm()->DupInterComm(comm);
#endif
  mDSMServerBuffer->SetIsConnected(true);

  if (startCoreIndex < size) {
    if (rank >= startCoreIndex && rank <= endCoreIndex) {
      mDSMServerBuffer->ReceiveInfo();
    }
    else {
      mDSMServerBuffer->SendInfo();
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
  }
  else {
    // On cores where memory is set up, start the service loop
    // This should iterate infinitely until a value to end the loop is passed
    int returnOpCode;
    try {
      mDSMServerBuffer->BufferServiceLoop(&returnOpCode);
    }
    catch (XdmfError & e) {
      throw e;
    }
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
                                             unsigned int blockSize,
                                             double resizeFactor,
                                             int startCoreIndex,
                                             int endCoreIndex,
                                             std::string applicationName) :
  XdmfHDF5Controller(hdf5FilePath,
                     dataSetPath,
                     type,
                     start,
                     stride,
                     dimensions,
                     dataspaceDimensions),
  mServerMode(true)

{
  int rank, size;
#ifdef XDMF_DSM_IS_CRAY

  MPI_Comm InterComm = comm;
  // Cray needs to be launched via the colon notation so that it
  // can properly create a merged communicator

  MPI_Comm_size(comm, &size);
  MPI_Comm_rank(comm, &rank);

  int currentCore = 0;
  int * checkstatus = new int[size]();
  int localCheck = 0;

  char * coreTag;
  int tagSize = 0;

  std::vector<std::pair<std::string, unsigned int> > newStructure;

  std::vector<int> coreSplit;
  unsigned int splitid = 0;

  int * splitIds;
  unsigned int splitsize = 0;

  while (currentCore < size)
  {
    if (rank == currentCore)
    {
      tagSize = applicationName.size();
    }
    MPI_Bcast(&tagSize, 1, MPI_INT, currentCore, comm);
    coreTag = new char[tagSize+1]();

    if (rank == currentCore)
    {
      strcpy(coreTag, applicationName.c_str());
    }
    MPI_Bcast(coreTag, tagSize, MPI_CHAR, currentCore, comm);

    coreTag[tagSize] = 0;

    if (strcmp(coreTag, applicationName.c_str()) == 0)
    {
      localCheck = 1;
    }
    else
    {
      localCheck = 0;
    }

    checkstatus[rank] = localCheck;

    MPI_Allgather(&localCheck, 1, MPI_INT,
                  checkstatus, 1, MPI_INT,
                  comm);

    bool insplit = false;
    while (checkstatus[currentCore])
    {
      if (rank == currentCore)
      {
        insplit = true;
      }
      coreSplit.push_back(currentCore);
      ++currentCore;
      if (currentCore >= size)
      {
        break;
      }
    }
    if (insplit)
    {
      splitIds = (int *)calloc(coreSplit.size(), sizeof(int));
      memcpy(splitIds, &(coreSplit[0]), coreSplit.size() * sizeof(int));
      splitsize = coreSplit.size();
    }
    newStructure.push_back(std::pair<std::string, unsigned int>(std::string(coreTag), coreSplit.size()));
    coreSplit.clear();
    ++splitid;
  }


  // Use MPI_Comm_split
  MPI_Group IntraGroup, InterGroup;
  MPI_Comm IntraComm;
  MPI_Comm_group(comm, &InterGroup);
  MPI_Group_incl(InterGroup, splitsize, splitIds, &IntraGroup);
  MPI_Comm_create(comm, IntraGroup, &IntraComm);
  cfree(splitIds);

  int intraid = 0;
  int intrasize = 0;

  MPI_Comm_rank(IntraComm, &intraid);
  MPI_Comm_size(IntraComm, &intrasize);

  comm = IntraComm;

#endif

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
  mDSMServerBuffer = new XdmfDSMBuffer();

  mDSMServerBuffer->SetLocalBufferSizeMBytes(bufferSize);
  mDSMServerBuffer->SetInterCommType(XDMF_DSM_COMM_MPI);
  mDSMServerBuffer->SetBlockLength(blockSize);
  mDSMServerBuffer->SetDsmType(XDMF_DSM_TYPE_BLOCK_CYCLIC);
  mDSMServerBuffer->SetResizeFactor(resizeFactor);
  if (rank >= startCoreIndex && rank <= endCoreIndex)
  {
    mDSMServerBuffer->GetComm()->SetApplicationName("Server");
  }
  else
  {
    mDSMServerBuffer->GetComm()->SetApplicationName(applicationName);
  }

  if (rank >= startCoreIndex && rank <= endCoreIndex) {
    mDSMServerBuffer->Create(serverComm);
  }
  else {
    mDSMServerBuffer->Create(mWorkerComm, startCoreIndex, endCoreIndex);
  }

  XDMF_dsm_set_manager(mDSMServerBuffer);

#ifdef XDMF_DSM_IS_CRAY
  mDSMServerBuffer->GetComm()->DupInterComm(InterComm);
#else
  mDSMServerBuffer->GetComm()->DupInterComm(comm);
#endif
  mDSMServerBuffer->SetIsConnected(true);

  if (startCoreIndex < size) {
    if (rank >= startCoreIndex && rank <= endCoreIndex) {
      mDSMServerBuffer->ReceiveInfo();
    }
    else {
      mDSMServerBuffer->SendInfo();
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
  }
  else {
    // On cores where memory is set up, start the service loop
    // This should iterate infinitely until a value to end the loop is passed
    int returnOpCode;
    try {
      mDSMServerBuffer->BufferServiceLoop(&returnOpCode);
    }
    catch (XdmfError & e) {
      throw e;
    }
  }
}

XdmfHDF5ControllerDSM::XdmfHDF5ControllerDSM(XdmfHDF5ControllerDSM & refController):
  XdmfHDF5Controller(refController),
  mDSMServerBuffer(refController.getServerBuffer()),
  mServerMode(refController.getServerMode())
{
  mWorkerComm = mDSMServerBuffer->GetComm()->GetIntraComm();
}

XdmfHDF5ControllerDSM::~XdmfHDF5ControllerDSM()
{
}

std::string XdmfHDF5ControllerDSM::getName() const
{
  return "HDFDSM";
}

XdmfDSMBuffer * XdmfHDF5ControllerDSM::getServerBuffer()
{
  return mDSMServerBuffer;
}

bool XdmfHDF5ControllerDSM::getServerMode() const
{
  return mServerMode;
}

MPI_Comm XdmfHDF5ControllerDSM::getWorkerComm() const
{
  MPI_Comm returnComm = MPI_COMM_NULL;
  if (mWorkerComm != MPI_COMM_NULL) {
    MPI_Comm_dup(mWorkerComm, &returnComm);
  }
  return returnComm;
}

void XdmfHDF5ControllerDSM::setBuffer(XdmfDSMBuffer * newBuffer)
{
  mDSMServerBuffer = newBuffer;
}

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
      try {
        XdmfError::message(XdmfError::FATAL, "Failed to disconnect Comm");
      }
      catch (XdmfError & e) {
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
      catch (XdmfError & e) {
        throw e;
      }
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
      try {
        mDSMServerBuffer->SendCommandHeader(XDMF_DSM_OPCODE_DONE, i, 0, 0, XDMF_DSM_INTER_COMM);
      }
      catch (XdmfError & e) {
        throw e;
      }
    }
  }
  else {
    try {
      XdmfError::message(XdmfError::FATAL, "Error: Stopping DSM manually only available in server mode.");
    }
    catch (XdmfError & e) {
      throw e;
    }
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
      try {
        mDSMServerBuffer->BufferServiceLoop(&returnOpCode);
      }
      catch (XdmfError & e) {
        throw e;
      }
    }
  }
  else {
    try {
      XdmfError::message(XdmfError::FATAL, "Error: Restarting DSM only available in server mode.");
    }
    catch (XdmfError & e) {
      throw e;
    }
  }
}

void XdmfHDF5ControllerDSM::read(XdmfArray * const array)
{
  // Set file access property list for DSM
  hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);

  // Use DSM driver
  if (mWorkerComm != MPI_COMM_NULL) {
    XDMFH5Pset_fapl_dsm(fapl, mWorkerComm, mDSMServerBuffer, 0);
  }

  // Read from DSM Buffer
  XdmfHDF5Controller::read(array, fapl);

  // Close file access property list
  H5Pclose(fapl);
}

// C Wrappers

XDMFHDF5CONTROLLERDSM * XdmfHDF5ControllerDSMNewFromServerBuffer(char * hdf5FilePath,
                                                                 char * dataSetPath,
                                                                 int type,
                                                                 unsigned int * start,
                                                                 unsigned int * stride,
                                                                 unsigned int * dimensions,
                                                                 unsigned int * dataspaceDimensions,
                                                                 unsigned int numDims,
                                                                 void * dsmBuffer,
                                                                 int * status)
{
  XDMF_ERROR_WRAP_START(status)
  std::vector<unsigned int> startVector(start, start + numDims);
  std::vector<unsigned int> strideVector(stride, stride + numDims);
  std::vector<unsigned int> dimVector(dimensions, dimensions + numDims);
  std::vector<unsigned int> dataspaceVector(dataspaceDimensions, dataspaceDimensions + numDims);
  shared_ptr<const XdmfArrayType> buildType = shared_ptr<XdmfArrayType>();
  switch (type) {
    case XDMF_ARRAY_TYPE_UINT8:
      buildType = XdmfArrayType::UInt8();
      break;
    case XDMF_ARRAY_TYPE_UINT16:
      buildType = XdmfArrayType::UInt16();
      break;
    case XDMF_ARRAY_TYPE_UINT32:
      buildType = XdmfArrayType::UInt32();
      break;
    case XDMF_ARRAY_TYPE_INT8:
      buildType = XdmfArrayType::Int8();
      break;
    case XDMF_ARRAY_TYPE_INT16:
      buildType = XdmfArrayType::Int16();
      break;
    case XDMF_ARRAY_TYPE_INT32:
      buildType = XdmfArrayType::Int32();
      break;
    case XDMF_ARRAY_TYPE_INT64:
      buildType = XdmfArrayType::Int64();
      break;
    case XDMF_ARRAY_TYPE_FLOAT32:
      buildType = XdmfArrayType::Float32();
      break;
    case XDMF_ARRAY_TYPE_FLOAT64:
      buildType = XdmfArrayType::Float64();
      break;
    default:
      try {
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid ArrayType.");
      }
      catch (XdmfError & e) {
        throw e;
      }
      break;
  }
  shared_ptr<XdmfHDF5ControllerDSM> generatedController = XdmfHDF5ControllerDSM::New(std::string(hdf5FilePath),
                                                                                     std::string(dataSetPath),
                                                                                     buildType,
                                                                                     startVector,
                                                                                     strideVector,
                                                                                     dimVector,
                                                                                     dataspaceVector,
                                                                                     (XdmfDSMBuffer *) dsmBuffer);
  return (XDMFHDF5CONTROLLERDSM *)((void *)(new XdmfHDF5ControllerDSM(*generatedController.get())));
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

XDMFHDF5CONTROLLERDSM * XdmfHDF5ControllerDSMNew(char * hdf5FilePath,
                                                 char * dataSetPath,
                                                 int type,
                                                 unsigned int * start,
                                                 unsigned int * stride,
                                                 unsigned int * dimensions,
                                                 unsigned int * dataspaceDimensions,
                                                 unsigned int numDims,
                                                 MPI_Comm comm,
                                                 unsigned int bufferSize,
                                                 int startCoreIndex,
                                                 int endCoreIndex,
                                                 char * applicationName,
                                                 int * status)
{
  XDMF_ERROR_WRAP_START(status)
  std::vector<unsigned int> startVector(start, start + numDims);
  std::vector<unsigned int> strideVector(stride, stride + numDims);
  std::vector<unsigned int> dimVector(dimensions, dimensions + numDims);
  std::vector<unsigned int> dataspaceVector(dataspaceDimensions, dataspaceDimensions + numDims);
  shared_ptr<const XdmfArrayType> buildType = shared_ptr<XdmfArrayType>();
  switch (type) {
    case XDMF_ARRAY_TYPE_UINT8:
      buildType = XdmfArrayType::UInt8();
      break;
    case XDMF_ARRAY_TYPE_UINT16:
      buildType = XdmfArrayType::UInt16();
      break;
    case XDMF_ARRAY_TYPE_UINT32:
      buildType = XdmfArrayType::UInt32();
      break;
    case XDMF_ARRAY_TYPE_INT8:
      buildType = XdmfArrayType::Int8();
      break;
    case XDMF_ARRAY_TYPE_INT16:
      buildType = XdmfArrayType::Int16();
      break;
    case XDMF_ARRAY_TYPE_INT32:
      buildType = XdmfArrayType::Int32();
      break;
    case XDMF_ARRAY_TYPE_INT64:
      buildType = XdmfArrayType::Int64();
      break;
    case XDMF_ARRAY_TYPE_FLOAT32:
      buildType = XdmfArrayType::Float32();
      break;
    case XDMF_ARRAY_TYPE_FLOAT64:
      buildType = XdmfArrayType::Float64();
      break;
    default:
      try {
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid ArrayType.");
      }
      catch (XdmfError & e) {
        throw e;
      }
      break;
  }
  shared_ptr<XdmfHDF5ControllerDSM> generatedController = XdmfHDF5ControllerDSM::New(std::string(hdf5FilePath),
                                                                                     std::string(dataSetPath),
                                                                                     buildType,
                                                                                     startVector,
                                                                                     strideVector,
                                                                                     dimVector,
                                                                                     dataspaceVector,
                                                                                     comm,
                                                                                     bufferSize,
                                                                                     startCoreIndex,
                                                                                     endCoreIndex,
                                                                                     std::string(applicationName));
  return (XDMFHDF5CONTROLLERDSM *)((void *)(new XdmfHDF5ControllerDSM(*generatedController.get())));
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

XDMFHDF5CONTROLLERDSM * XdmfHDF5ControllerDSMNewPaged(char * hdf5FilePath,
                                                      char * dataSetPath,
                                                      int type,
                                                      unsigned int * start,
                                                      unsigned int * stride,
                                                      unsigned int * dimensions,
                                                      unsigned int * dataspaceDimensions,
                                                      unsigned int numDims,
                                                      MPI_Comm comm,
                                                      unsigned int bufferSize,
                                                      unsigned int blockSize,
                                                      double resizeFactor,
                                                      int startCoreIndex,
                                                      int endCoreIndex,
                                                      char * applicationName,
                                                      int * status)
{
  XDMF_ERROR_WRAP_START(status)
  std::vector<unsigned int> startVector(start, start + numDims);
  std::vector<unsigned int> strideVector(stride, stride + numDims);
  std::vector<unsigned int> dimVector(dimensions, dimensions + numDims);
  std::vector<unsigned int> dataspaceVector(dataspaceDimensions, dataspaceDimensions + numDims);
  shared_ptr<const XdmfArrayType> buildType = shared_ptr<XdmfArrayType>();
  switch (type) {
    case XDMF_ARRAY_TYPE_UINT8:
      buildType = XdmfArrayType::UInt8();
      break;
    case XDMF_ARRAY_TYPE_UINT16:
      buildType = XdmfArrayType::UInt16();
      break;
    case XDMF_ARRAY_TYPE_UINT32:
      buildType = XdmfArrayType::UInt32();
      break;
    case XDMF_ARRAY_TYPE_INT8:
      buildType = XdmfArrayType::Int8();
      break;
    case XDMF_ARRAY_TYPE_INT16:
      buildType = XdmfArrayType::Int16();
      break;
    case XDMF_ARRAY_TYPE_INT32:
      buildType = XdmfArrayType::Int32();
      break;
    case XDMF_ARRAY_TYPE_INT64:
      buildType = XdmfArrayType::Int64();
      break;
    case XDMF_ARRAY_TYPE_FLOAT32:
      buildType = XdmfArrayType::Float32();
      break;
    case XDMF_ARRAY_TYPE_FLOAT64:
      buildType = XdmfArrayType::Float64();
      break;
    default:
      try {
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid ArrayType.");
      }
      catch (XdmfError & e) {
        throw e;
      }
      break;
  }
  shared_ptr<XdmfHDF5ControllerDSM> generatedController = XdmfHDF5ControllerDSM::New(std::string(hdf5FilePath),
                                                                                     std::string(dataSetPath),
                                                                                     buildType,
                                                                                     startVector,
                                                                                     strideVector,
                                                                                     dimVector,
                                                                                     dataspaceVector,
                                                                                     comm,
                                                                                     bufferSize,
                                                                                     blockSize,
                                                                                     resizeFactor,
                                                                                     startCoreIndex,
                                                                                     endCoreIndex,
                                                                                     std::string(applicationName));
  return (XDMFHDF5CONTROLLERDSM *)((void *)(new XdmfHDF5ControllerDSM(*generatedController.get())));
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

XDMFDSMBUFFER * XdmfHDF5ControllerDSMGetServerBuffer(XDMFHDF5CONTROLLERDSM * controller)
{
  return (XDMFDSMBUFFER *)((void *)(((XdmfHDF5ControllerDSM *)controller)->getServerBuffer()));
}

int XdmfHDF5ControllerDSMGetServerMode(XDMFHDF5CONTROLLERDSM * controller)
{
  return ((XdmfHDF5ControllerDSM *)controller)->getServerMode();
}

MPI_Comm XdmfHDF5ControllerDSMGetWorkerComm(XDMFHDF5CONTROLLERDSM * controller)
{
  return ((XdmfHDF5ControllerDSM *)controller)->getWorkerComm();
}

void XdmfHDF5ControllerDSMSetServerBuffer(XDMFHDF5CONTROLLERDSM * controller, XDMFDSMBUFFER * newBuffer)
{
  ((XdmfHDF5ControllerDSM *)controller)->setBuffer((XdmfDSMBuffer *)newBuffer);
}

void XdmfHDF5ControllerDSMSetServerMode(XDMFHDF5CONTROLLERDSM * controller, int newMode)
{
  ((XdmfHDF5ControllerDSM *)controller)->setServerMode(newMode);
}

void XdmfHDF5ControllerDSMSetWorkerComm(XDMFHDF5CONTROLLERDSM * controller, MPI_Comm comm, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfHDF5ControllerDSM *)controller)->setWorkerComm(comm);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfHDF5ControllerDSMStopDSM(XDMFHDF5CONTROLLERDSM * controller, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfHDF5ControllerDSM *)controller)->stopDSM();
  XDMF_ERROR_WRAP_END(status)
}

void XdmfHDF5ControllerDSMRestartDSM(XDMFHDF5CONTROLLERDSM * controller, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfHDF5ControllerDSM *)controller)->restartDSM();
  XDMF_ERROR_WRAP_END(status)
}

// C Wrappers for parent classes are generated by macros

XDMF_HEAVYCONTROLLER_C_CHILD_WRAPPER(XdmfHDF5ControllerDSM, XDMFHDF5CONTROLLERDSM)
XDMF_HDF5CONTROLLER_C_CHILD_WRAPPER(XdmfHDF5ControllerDSM, XDMFHDF5CONTROLLERDSM)
