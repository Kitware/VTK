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

#include <hdf5.h>
#include <XdmfArray.hpp>
#include <XdmfDSMCommMPI.hpp>
#include <XdmfDSMBuffer.hpp>
#include <XdmfDSMDriver.hpp>
#include "XdmfHDF5ControllerDSM.hpp"
#include "XdmfHDF5WriterDSM.hpp"
#include "XdmfSystemUtils.hpp"
#include "XdmfError.hpp"
#include <string.h>
#include <iostream>

XdmfHDF5WriterDSM::XdmfHDF5WriterDSMImpl::XdmfHDF5WriterDSMImpl():
  XdmfHDF5WriterImpl(),
  mDSMIsInit(false),
  mDSMLocked(false)
{
};

XdmfHDF5WriterDSM::XdmfHDF5WriterDSMImpl::~XdmfHDF5WriterDSMImpl()
{
  closeFile();
};

int
XdmfHDF5WriterDSM::XdmfHDF5WriterDSMImpl::openFile(const std::string & filePath,
                                             const int mDataSetId)
{
  if(mHDF5Handle >= 0) {
    // Perhaps we should throw a warning.
    closeFile();
  }
  // Save old error handler and turn off error handling for now
  H5E_auto_t old_func;
  void * old_client_data;
  H5Eget_auto(0, &old_func, &old_client_data);
  H5Eset_auto2(0, NULL, NULL);

  int toReturn = 0;

  mOpenFile.assign(filePath);

  std::vector<unsigned int> pages;
  haddr_t start, end;
  unsigned int numPages;

  if (((XdmfDSMBuffer *)xdmf_dsm_get_manager())->GetComm()->GetId() == 0 && !mDSMLocked)
  {
    ((XdmfDSMBuffer *)xdmf_dsm_get_manager())->Lock(strdup(filePath.c_str()));
    mDSMLocked = true;
  }


//  if (mDSMIsInit) {//  if(XdmfDsmFileInDSM(filePath.c_str()) > 0) {
  if (((XdmfDSMBuffer *)xdmf_dsm_get_manager())->RequestFileDescription(strdup(filePath.c_str()),
                                                                               pages,
                                                                               numPages,
                                                                               start,
                                                                               end)
      == XDMF_DSM_SUCCESS)
  {
    mHDF5Handle = H5Fopen(filePath.c_str(),
                          H5F_ACC_RDWR,
                          mFapl);
    if(mDataSetId == 0) {
      hsize_t numObjects;
      /*herr_t status = */H5Gget_num_objs(mHDF5Handle,
                                          &numObjects);
      toReturn = numObjects;
    }
    else {
      toReturn = mDataSetId;
    }
  }
  else {
    mHDF5Handle = H5Fcreate(filePath.c_str(),
                            H5F_ACC_TRUNC,
                            H5P_DEFAULT,
                            mFapl);
    mDSMIsInit = true;
  }

  // Restore previous error handler
  H5Eset_auto2(0, old_func, old_client_data);

  return toReturn;
};

void
XdmfHDF5WriterDSM::XdmfHDF5WriterDSMImpl::closeFile()
{
  if(mHDF5Handle >= 0) {
    H5Fclose(mHDF5Handle);
    mHDF5Handle = -1;
  }

  if (mDSMLocked)
  {
    if (((XdmfDSMBuffer *)xdmf_dsm_get_manager())->GetComm()->GetId() == 0)
    {
      ((XdmfDSMBuffer *)xdmf_dsm_get_manager())->Unlock(strdup(mOpenFile.c_str()));
      mDSMLocked = false;
    }
  }
  mOpenFile = "";
};

shared_ptr<XdmfHDF5WriterDSM>
XdmfHDF5WriterDSM::New(const std::string & filePath,
                       XdmfDSMBuffer * const dsmBuffer)
{
  shared_ptr<XdmfHDF5WriterDSM> p(new XdmfHDF5WriterDSM(filePath,
                                                        dsmBuffer));
  return p;
}

shared_ptr<XdmfHDF5WriterDSM>
XdmfHDF5WriterDSM::New(const std::string & filePath,
                       MPI_Comm comm,
                       unsigned int bufferSize,
                       int startCoreIndex,
                       int endCoreIndex,
                       std::string applicationName)
{
  shared_ptr<XdmfHDF5WriterDSM> p(new XdmfHDF5WriterDSM(filePath,
                                                        comm,
                                                        bufferSize,
                                                        startCoreIndex,
                                                        endCoreIndex,
                                                        applicationName));
  return p;
}

shared_ptr<XdmfHDF5WriterDSM>
XdmfHDF5WriterDSM::New(const std::string & filePath,
                       MPI_Comm comm,
                       unsigned int bufferSize,
                       unsigned int blockSize,
                       double resizeFactor,
                       int startCoreIndex,
                       int endCoreIndex,
                       std::string applicationName)
{
  shared_ptr<XdmfHDF5WriterDSM> p(new XdmfHDF5WriterDSM(filePath,
                                                        comm,
                                                        bufferSize,
                                                        blockSize,
                                                        resizeFactor,
                                                        startCoreIndex,
                                                        endCoreIndex,
                                                        applicationName));
  return p;
}

shared_ptr<XdmfHDF5WriterDSM>
XdmfHDF5WriterDSM::New(const std::string & filePath,
                       MPI_Comm comm,
                       std::string applicationName)
{
  shared_ptr<XdmfHDF5WriterDSM> p(new XdmfHDF5WriterDSM(filePath,
                                                        comm,
                                                        applicationName));
  return p;
}

// The database/nonthreaded version
XdmfHDF5WriterDSM::XdmfHDF5WriterDSM(const std::string & filePath,
                                     XdmfDSMBuffer * const dsmBuffer) :
  XdmfHDF5Writer(filePath),
  mDSMServerBuffer(dsmBuffer),
  mServerMode(true),
  mNotifyOnWrite(true)
{
  if (mImpl)
  {
    delete mImpl;
  }
  mImpl = new XdmfHDF5WriterDSMImpl();

  mImpl->mFapl = -1;
  mWorkerComm = mDSMServerBuffer->GetComm()->GetIntraComm();
  if (xdmf_dsm_get_manager() == NULL) {
    XDMF_dsm_set_manager(mDSMServerBuffer);
  }
  else {
    xdmf_dsm_set_manager(mDSMServerBuffer);
  }
}

XdmfHDF5WriterDSM::XdmfHDF5WriterDSM(const std::string & filePath,
                                     MPI_Comm comm,
                                     unsigned int bufferSize,
                                     int startCoreIndex,
                                     int endCoreIndex,
                                     std::string applicationName) :
  XdmfHDF5Writer(filePath),
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
    assert(coreTag);
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

  if (mImpl)
  {
    delete mImpl;
  }
  mImpl = new XdmfHDF5WriterDSMImpl();

  mImpl->mFapl = -1;

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
  mDSMServerBuffer = new XdmfDSMBuffer();
  mDSMServerBuffer->SetLocalBufferSizeMBytes(bufferSize);
  mDSMServerBuffer->SetInterCommType(XDMF_DSM_COMM_MPI);
  mDSMServerBuffer->SetDsmType(XDMF_DSM_TYPE_UNIFORM);
  MPI_Barrier(comm);

  if (rank >= startCoreIndex && rank <= endCoreIndex) {
    mDSMServerBuffer->Create(serverComm);
  }
  else {
    mDSMServerBuffer->Create(mWorkerComm, startCoreIndex, endCoreIndex);
    mDSMServerBuffer->SetIsServer(false);
  }

  XDMF_dsm_set_manager(mDSMServerBuffer);

#ifdef XDMF_DSM_IS_CRAY
  mDSMServerBuffer->GetComm()->DupInterComm(InterComm);
#else
  mDSMServerBuffer->GetComm()->DupInterComm(comm);
#endif
  if (rank >= startCoreIndex && rank <= endCoreIndex)
  {
    mDSMServerBuffer->GetComm()->SetApplicationName("Server");
  }
  else
  {
    mDSMServerBuffer->GetComm()->SetApplicationName(applicationName);
  }
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
    // If this is set to false then the buffer will attempt to connect
    // to the intercomm for DSM communications
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

XdmfHDF5WriterDSM::XdmfHDF5WriterDSM(const std::string & filePath,
                                     MPI_Comm comm,
                                     unsigned int bufferSize,
                                     unsigned int blockSize,
                                     double resizeFactor,
                                     int startCoreIndex,
                                     int endCoreIndex,
                                     std::string applicationName) :
  XdmfHDF5Writer(filePath),
  mServerMode(true)
{
  int rank, size;

#ifdef XDMF_DSM_IS_CRAY

  MPI_Comm InterComm = comm;

  // Cray needs to be launched via the colon notation so that it
  // can properly create a merged communicator

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  int currentCore = 0;
  int * checkstatus = new int[size]();
  int localCheck = 0;

  char * coreTag;
  int tagSize = 0;

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
    assert(coreTag);
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

  if (mImpl)
  {
    delete mImpl;
  }
  mImpl = new XdmfHDF5WriterDSMImpl();

  mImpl->mFapl = -1;

// Cray Specific stuff has to occur here

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
  mDSMServerBuffer = new XdmfDSMBuffer();
  mDSMServerBuffer->SetLocalBufferSizeMBytes(bufferSize);
  mDSMServerBuffer->SetInterCommType(XDMF_DSM_COMM_MPI);
  mDSMServerBuffer->SetBlockLength(blockSize);
  mDSMServerBuffer->SetDsmType(XDMF_DSM_TYPE_BLOCK_CYCLIC);
  mDSMServerBuffer->SetResizeFactor(resizeFactor);
  MPI_Barrier(comm);

  if (rank >= startCoreIndex && rank <= endCoreIndex) {
    mDSMServerBuffer->Create(serverComm);
  }
  else {
    mDSMServerBuffer->Create(mWorkerComm, startCoreIndex, endCoreIndex);
    mDSMServerBuffer->SetIsServer(false);
  }

  XDMF_dsm_set_manager(mDSMServerBuffer);

#ifdef XDMF_DSM_IS_CRAY
  mDSMServerBuffer->GetComm()->DupInterComm(InterComm);
#else
  mDSMServerBuffer->GetComm()->DupInterComm(comm);
#endif
  if (rank >= startCoreIndex && rank <= endCoreIndex)
  {
    mDSMServerBuffer->GetComm()->SetApplicationName("Server");
  }
  else
  {
    mDSMServerBuffer->GetComm()->SetApplicationName(applicationName);
  }
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
    // If this is set to false then the buffer will attempt to connect
    // to the intercomm for DSM communications
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

XdmfHDF5WriterDSM::XdmfHDF5WriterDSM(const std::string & filePath,
                                     MPI_Comm comm,
                                     std::string applicationName) :
  XdmfHDF5Writer(filePath),
  mServerMode(true),
  mNotifyOnWrite(true)
{
  int rank, size;

#ifdef XDMF_DSM_IS_CRAY

  MPI_Comm InterComm = comm;

  // Cray needs to be launched via the colon notation so that it
  // can properly create a merged communicator

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  int currentCore = 0;
  int * checkstatus = new int[size]();
  int localCheck = 0;

  char * coreTag;
  int tagSize = 0;

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
    assert(coreTag);
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

  if (mImpl)
  {
    delete mImpl;
  }
  mImpl = new XdmfHDF5WriterDSMImpl();

  mImpl->mFapl = -1;

  // Retrieve the Buffer
  mDSMServerBuffer = new XdmfDSMBuffer();
  mDSMServerBuffer->SetIsServer(false);
  mDSMServerBuffer->SetInterCommType(XDMF_DSM_COMM_MPI);

  // Create a Comm object
  XdmfDSMCommMPI * newComm = new XdmfDSMCommMPI();
  newComm->DupComm(comm);
#ifdef XDMF_DSM_IS_CRAY
  newComm->DupInterComm(InterComm);
#else
  newComm->DupInterComm(comm);
#endif
  newComm->Init();
  newComm->SetApplicationName(applicationName);

  // Set the Comm to the buffer
  mDSMServerBuffer->SetComm(newComm);

  // Register the manager with the driver
  XDMF_dsm_set_manager(mDSMServerBuffer);

#ifdef XDMF_DSM_IS_CRAY
  mDSMServerBuffer->ReceiveInfo();
#endif

  MPI_Barrier(comm);
}

XdmfHDF5WriterDSM::XdmfHDF5WriterDSM(XdmfHDF5WriterDSM & refWriter):
  XdmfHDF5Writer(refWriter),
  mDSMServerBuffer(refWriter.getServerBuffer()),
  mWorkerComm(refWriter.getWorkerComm()),
  mServerMode(refWriter.getServerMode()),
  mNotifyOnWrite(refWriter.mNotifyOnWrite)
{
  mImpl->mFapl = -1;
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
  else {
    return shared_ptr<XdmfHDF5ControllerDSM>();
  }
}

void
XdmfHDF5WriterDSM::closeFile()
{
  if(mImpl->mFapl >= 0) {
    H5Pclose(mImpl->mFapl);
    mImpl->mFapl = -1;
  }
  XdmfHDF5Writer::closeFile();
}

int
XdmfHDF5WriterDSM::getDataSetSize(const std::string & fileName, const std::string & dataSetName)
{
  bool closeFAPL = false;

  if(mImpl->mFapl < 0) {
    // Set file access property list for DSM
    mImpl->mFapl = H5Pcreate(H5P_FILE_ACCESS);
    // Use DSM driver
    if (mWorkerComm != MPI_COMM_NULL) {
      XDMFH5Pset_fapl_dsm(mImpl->mFapl, mWorkerComm, mDSMServerBuffer, 0);
    }

    closeFAPL = true;
  }

  hid_t handle = -1;
  H5E_auto_t old_func;
  void * old_client_data;
  herr_t status;
  H5Eget_auto(0, &old_func, &old_client_data);
  H5Eset_auto2(0, NULL, NULL);
  bool mustClose = false;
  if (XdmfSystemUtils::getRealPath(fileName) != mImpl->mOpenFile) {
    // Save old error handler and turn off error handling for now
      handle = H5Fopen(fileName.c_str(),
                       H5F_ACC_RDWR,
                       mImpl->mFapl);
    mustClose = true;
  }
  else {
    handle = mImpl->mHDF5Handle;
  }

  // Restore previous error handler
  H5Eset_auto2(0, old_func, old_client_data);

  if (!H5Lexists(handle,
                 dataSetName.c_str(),
                 H5P_DEFAULT))
  {
    if (handle != mImpl->mHDF5Handle) {
      H5Fclose(handle);
    }

    if(closeFAPL) {
      // Close file access property list
      H5Pclose(mImpl->mFapl);
      mImpl->mFapl = -1;
    }

     return 0;
  }

  hid_t checkset = H5Dopen(handle,
                           dataSetName.c_str(),
                           H5P_DEFAULT);

  hid_t checkspace = H5S_ALL;
  checkspace = H5Dget_space(checkset);
  hssize_t checksize = H5Sget_simple_extent_npoints(checkspace);

  if(checkspace != H5S_ALL) {
    status = H5Sclose(checkspace);
  }
  status = H5Dclose(checkset);

  if (handle != mImpl->mHDF5Handle || mustClose) {
    H5Fclose(handle);
  }

  if(closeFAPL) {
    // Close file access property list
    H5Pclose(mImpl->mFapl);
    mImpl->mFapl = -1;
  }

  return checksize;
}

bool
XdmfHDF5WriterDSM::getNotifyOnWrite()
{
  return mNotifyOnWrite;
}

XdmfDSMBuffer * XdmfHDF5WriterDSM::getServerBuffer()
{
  return mDSMServerBuffer;
}

bool XdmfHDF5WriterDSM::getServerMode()
{
  return mServerMode;
}

MPI_Comm XdmfHDF5WriterDSM::getWorkerComm()
{
  MPI_Comm returnComm = MPI_COMM_NULL;
  if (mWorkerComm != MPI_COMM_NULL) {
    MPI_Comm_dup(mWorkerComm, &returnComm);
  }
  return returnComm;
}

void XdmfHDF5WriterDSM::setAllowSetSplitting(bool newAllow)
{
  //overrides to disable the parent version
  XdmfHDF5Writer::setAllowSetSplitting(false); 
}

void XdmfHDF5WriterDSM::setBuffer(XdmfDSMBuffer * newBuffer)
{
  mDSMServerBuffer = newBuffer;
}

void XdmfHDF5WriterDSM::setNotifyOnWrite(bool status)
{
  mNotifyOnWrite = status;
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

void 
XdmfHDF5WriterDSM::openFile()
{
  if(mImpl->mFapl >= 0) {
    this->closeFile();
  }

  // Set file access property list for DSM
  mImpl->mFapl = H5Pcreate(H5P_FILE_ACCESS);

  if (mWorkerComm != MPI_COMM_NULL) {
    XDMFH5Pset_fapl_dsm(mImpl->mFapl, mWorkerComm, mDSMServerBuffer, 0);
  }
  XdmfHDF5Writer::openFile();
}

void XdmfHDF5WriterDSM::visit(XdmfArray & array,
                              const shared_ptr<XdmfBaseVisitor>)
{
  bool closeFAPL = false;

  if(mImpl->mFapl < 0) {
    // Set file access property list for DSM
    mImpl->mFapl = H5Pcreate(H5P_FILE_ACCESS);
    // Use DSM driver
    if (mWorkerComm != MPI_COMM_NULL) {
      XDMFH5Pset_fapl_dsm(mImpl->mFapl, mWorkerComm, mDSMServerBuffer, 0);
    }

    closeFAPL = true;
  }

  // Write to DSM Buffer
  this->write(array);

  if(closeFAPL) {
    // Close file access property list
    H5Pclose(mImpl->mFapl);
    mImpl->mFapl = -1;
  }

  if (mNotifyOnWrite)
  {
    for (unsigned int i = 0; i < array.getNumberHeavyDataControllers(); ++i)
    {
      if (array.getHeavyDataController(i)->getName().compare("HDFDSM") == 0)
      {
        this->waitRelease(array.getHeavyDataController(i)->getFilePath(),
                          shared_dynamic_cast<XdmfHDF5ControllerDSM>(array.getHeavyDataController(i))->getDataSetPath());
      }
    }
  }
}

void
XdmfHDF5WriterDSM::waitRelease(std::string fileName, std::string datasetName, int code)
{
  mDSMServerBuffer->WaitRelease(fileName, datasetName, code);
}

int
XdmfHDF5WriterDSM::waitOn(std::string fileName, std::string datasetName)
{
  return mDSMServerBuffer->WaitOn(fileName, datasetName);
}

// C Wrappers

XDMFHDF5WRITERDSM * XdmfHDF5WriterDSMNewFromServerBuffer(char * filePath,
                                                         void * dsmBuffer,
                                                         int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<XdmfHDF5WriterDSM> createdWriter = XdmfHDF5WriterDSM::New(std::string(filePath), (XdmfDSMBuffer *)dsmBuffer);
  return (XDMFHDF5WRITERDSM *)((void *)(new XdmfHDF5WriterDSM(* createdWriter.get())));
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

XDMFHDF5WRITERDSM * XdmfHDF5WriterDSMNew(char * filePath,
                                         MPI_Comm comm,
                                         unsigned int bufferSize,
                                         int startCoreIndex,
                                         int endCoreIndex,
                                         char * applicationName,
                                         int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<XdmfHDF5WriterDSM> createdWriter = XdmfHDF5WriterDSM::New(std::string(filePath), comm, bufferSize, startCoreIndex, endCoreIndex, std::string(applicationName));
  return (XDMFHDF5WRITERDSM *)((void *)(new XdmfHDF5WriterDSM(*createdWriter.get())));
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

XDMFHDF5WRITERDSM * XdmfHDF5WriterDSMNewPaged(char * filePath,
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
  shared_ptr<XdmfHDF5WriterDSM> createdWriter = XdmfHDF5WriterDSM::New(std::string(filePath), comm, bufferSize, blockSize, resizeFactor, startCoreIndex, endCoreIndex, std::string(applicationName));
  return (XDMFHDF5WRITERDSM *)((void *)(new XdmfHDF5WriterDSM(*createdWriter.get())));
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

XDMFHDF5WRITERDSM *
XdmfHDF5WriterDSMNewConnectRequired(char * filePath,
                                    MPI_Comm comm,
                                    char * applicationName,
                                    int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<XdmfHDF5WriterDSM> createdWriter = XdmfHDF5WriterDSM::New(std::string(filePath), comm, std::string(applicationName));
  return (XDMFHDF5WRITERDSM *)((void *)(new XdmfHDF5WriterDSM(*createdWriter.get())));
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

int XdmfHDF5WriterDSMGetDataSetSize(XDMFHDF5WRITERDSM * writer, char * fileName, char * dataSetName)
{
  return ((XdmfHDF5WriterDSM *) writer)->getDataSetSize(std::string(fileName), std::string(dataSetName));
}

XDMFDSMBUFFER * XdmfHDF5WriterDSMGetServerBuffer(XDMFHDF5WRITERDSM * writer)
{
  return (XDMFDSMBUFFER *)((void *)(((XdmfHDF5WriterDSM *) writer)->getServerBuffer()));
}

int XdmfHDF5WriterDSMGetServerMode(XDMFHDF5WRITERDSM * writer)
{
  return ((XdmfHDF5WriterDSM *) writer)->getServerMode();
}

MPI_Comm XdmfHDF5WriterDSMGetWorkerComm(XDMFHDF5WRITERDSM * writer)
{
  return ((XdmfHDF5WriterDSM *) writer)->getWorkerComm();
}

void XdmfHDF5WriterDSMSetServerBuffer(XDMFHDF5WRITERDSM * writer, XDMFDSMBUFFER * newBuffer)
{
  ((XdmfHDF5WriterDSM *) writer)->setBuffer((XdmfDSMBuffer *)newBuffer);
}

void XdmfHDF5WriterDSMSetServerMode(XDMFHDF5WRITERDSM * writer, int newMode)
{
  ((XdmfHDF5WriterDSM *) writer)->setServerMode(newMode);
}

void XdmfHDF5WriterDSMSetWorkerComm(XDMFHDF5WRITERDSM * writer, MPI_Comm comm, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfHDF5WriterDSM *) writer)->setWorkerComm(comm);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfHDF5WriterDSMStopDSM(XDMFHDF5WRITERDSM * writer, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfHDF5WriterDSM *) writer)->stopDSM();
  XDMF_ERROR_WRAP_END(status)
}

void XdmfHDF5WriterDSMRestartDSM(XDMFHDF5WRITERDSM * writer, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfHDF5WriterDSM *) writer)->restartDSM();
  XDMF_ERROR_WRAP_END(status)
}

void XdmfHDF5WriterDSMWaitRelease(XDMFHDF5WRITERDSM * writer, char * fileName, char * datasetName, int code)
{
  ((XdmfHDF5WriterDSM *) writer)->waitRelease(std::string(fileName), std::string(datasetName), code);
}

int XdmfHDF5WriterDSMWaitOn(XDMFHDF5WRITERDSM * writer, char * fileName, char * datasetName)
{
  return ((XdmfHDF5WriterDSM *) writer)->waitOn(std::string(fileName), std::string(datasetName));
}

XDMF_HDF5WRITER_C_CHILD_WRAPPER(XdmfHDF5WriterDSM, XDMFHDF5WRITERDSM)
XDMF_HEAVYWRITER_C_CHILD_WRAPPER(XdmfHDF5WriterDSM, XDMFHDF5WRITERDSM)
