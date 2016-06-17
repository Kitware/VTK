/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfDSMCommMPI.cpp                                                  */
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
  Module                  : H5FDdsmCommMpi.cxx

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

#include <XdmfDSMCommMPI.hpp>
#include <XdmfError.hpp>
#include <mpi.h>
#include <string.h>
#include <cstdlib>
#include <fstream>
#include <iostream>

bool XdmfDSMCommMPI::UseEnvFileName = false;

XdmfDSMCommMPI::XdmfDSMCommMPI()
{
  IntraComm = MPI_COMM_NULL;
  Id = -1;
  IntraSize = -1;
  InterComm = MPI_COMM_NULL;
  InterId = -1;
  InterSize = -1;
  SetDsmPortName("");
  // This is the default file name for the config file.
  DsmFileName = "dsmconnect.cfg";
  if (XdmfDSMCommMPI::UseEnvFileName)
  {
    // Grab from ENV
    if (std::getenv("XDMFDSM_CONFIG_FILE") != NULL)
    {
      DsmFileName = std::getenv("XDMFDSM_CONFIG_FILE");
    }
  }
  InterCommType = XDMF_DSM_COMM_MPI;
  HasOpenedPort = false;
  ApplicationName = "Application";
}

XdmfDSMCommMPI::~XdmfDSMCommMPI()
{
#ifndef OPEN_MPI
  if (InterComm != MPI_COMM_NULL) {
    int status = MPI_Comm_free(&InterComm);
    if (status != MPI_SUCCESS) {
      try {
        XdmfError::message(XdmfError::FATAL, "Failed to free intercomm Comm");
      }
      catch (XdmfError & e) {
        throw e;
      }
    }
  }
  if (IntraComm != MPI_COMM_NULL) {
    int status = MPI_Comm_free(&IntraComm);
    if (status != MPI_SUCCESS) {
      try {
        XdmfError::message(XdmfError::FATAL, "Failed to free intercomm Comm");
      }
      catch (XdmfError & e) {
        throw e;
      }
    }
  }
#endif
}

void
XdmfDSMCommMPI::Accept(unsigned int numConnections)
{
#ifndef XDMF_DSM_IS_CRAY
  int status;
  int acceptingLeadId;
  while (numConnections > 0) {
    if (InterComm == MPI_COMM_NULL) {
      acceptingLeadId = this->IntraSize;
      // If there is no InterComm, then accept from IntraComm and merge into InterComm
      MPI_Comm tempComm;
      int * portCheck = new int[GetInterSize()]();
      int portStatus;
      portStatus = 0;
      if (HasOpenedPort) {
        portStatus = 1;
      }
      MPI_Allgather(&portStatus, 1, MPI_INT, &(portCheck[0]), 1, MPI_INT, InterComm);
      int index = 0;
      for (index = 0; index < GetInterSize(); ++index) {
        if (portCheck[index] == 1) {
          break;
        }
      }
      int status = MPI_Comm_accept(DsmPortName, MPI_INFO_NULL, index, IntraComm, &tempComm);
      if (status != MPI_SUCCESS) {
        try {
          std::string message = "Failed to accept port ";
          message = message + DsmPortName;
          XdmfError::message(XdmfError::FATAL, message);
        }
        catch (XdmfError & e) {
          throw e;
        }
      }
      // False is specified for high so that the index of the cores doesn't change
      MPI_Comm mergedComm;
      status = MPI_Intercomm_merge(tempComm, false, &mergedComm);
      this->DupInterComm(mergedComm);
      if (status != MPI_SUCCESS) {
        try {
          XdmfError::message(XdmfError::FATAL, "Failed to merge intercomm");
        }
        catch (XdmfError & e) {
          throw e;
        }
      }
      else {
        MPI_Comm_rank(InterComm, &InterId);
        MPI_Comm_size(InterComm, &InterSize);
      }
    }
    else {
      acceptingLeadId = this->InterSize;
      // If there is an InterComm, accept into the InterComm and merge
      MPI_Comm tempComm;
      int * portCheck = new int[GetInterSize()]();
      int portStatus;
      portStatus = 0;
      if (HasOpenedPort) {
        portStatus = 1;
      }
      MPI_Allgather(&portStatus, 1, MPI_INT, &(portCheck[0]), 1, MPI_INT, InterComm);
      int index = 0;
      for (index = 0; index < GetInterSize(); ++index) {
        if (portCheck[index] == 1) {
          break;
        }
      }
      int status = MPI_Comm_accept(DsmPortName, MPI_INFO_NULL, index, InterComm, &tempComm);
      if (status != MPI_SUCCESS) {
        try {
          std::string message = "Failed to accept port ";
          message = message + DsmPortName;
          XdmfError::message(XdmfError::FATAL, message);
        }
        catch (XdmfError & e) {
          throw e;
        }
      }
      // False is specified for high so that the index of the cores doesn't change
      MPI_Comm mergedComm;
      status = MPI_Intercomm_merge(tempComm, false, &mergedComm);
      this->DupInterComm(mergedComm);
      if (status != MPI_SUCCESS) {
        try {
          XdmfError::message(XdmfError::FATAL, "Failed to merge InterComm");
        }
        catch (XdmfError & e) {
          throw e;
        }
      }
      else {
        MPI_Comm_rank(InterComm, &InterId);
        MPI_Comm_size(InterComm, &InterSize);
      }
    }

    //regen Intra comm from Inter Comm
    MPI_Group IntraGroup, InterGroup;
    MPI_Comm_group(InterComm, &InterGroup);
    int * ServerIds = (int *)calloc(this->IntraSize, sizeof(int));
    unsigned int index = 0;
    for(int i=this->InterId - this->Id; i < this->InterId - this->Id + this->IntraSize; ++i)
    {
      ServerIds[index++] = i;
    }
    MPI_Group_incl(InterGroup, this->IntraSize, ServerIds, &IntraGroup);
    MPI_Comm_create(InterComm, IntraGroup, &IntraComm);
    cfree(ServerIds);

    // Since this is accept, we will be recieving the local data from the new core and
    // sending the overarching data to the connecting cores
    unsigned int length;
    char * appname;
    unsigned int appsize;
    if (DsmProcessStructure.size() == 0)
    {
      DsmProcessStructure.push_back(std::pair<std::string, unsigned int>(this->ApplicationName, this->IntraSize));
    }
    int numSections = DsmProcessStructure.size();
    if (InterId == 0)
    {
      // Loop in the applicaiton names of the already existing sections of the DSM
      // Get the number of application sections
      MPI_Send(&numSections, 1, MPI_INT, acceptingLeadId, XDMF_DSM_EXCHANGE_TAG, this->InterComm);
      for (unsigned int i = 0; i < numSections; ++i)
      {
        length = DsmProcessStructure[i].first.size();
        // Get the length of the name
        MPI_Send(&length, 1, MPI_UNSIGNED, acceptingLeadId, XDMF_DSM_EXCHANGE_TAG, InterComm);
        // Get the string of characters
        appname = new char[length]();
        strcpy(appname, DsmProcessStructure[i].first.c_str());
        MPI_Send(appname, length, MPI_CHAR, acceptingLeadId, XDMF_DSM_EXCHANGE_TAG, InterComm);
        delete appname;
        // Get associated numprocs
        appsize = DsmProcessStructure[i].second;
        MPI_Send(&appsize, 1, MPI_UNSIGNED, acceptingLeadId, XDMF_DSM_EXCHANGE_TAG, InterComm);
      }
    }
    // Add the information for the newly added application
    // For each application in the connecting set.
    MPI_Bcast(&numSections, 1, MPI_UNSIGNED, acceptingLeadId, InterComm);
    for (unsigned int i = 0; i < numSections; ++i)
    {
      MPI_Bcast(&length, 1, MPI_UNSIGNED, acceptingLeadId, InterComm);
      appname = new char[length+1]();
      MPI_Bcast(appname, length, MPI_CHAR, acceptingLeadId, InterComm);
      appname[length] = 0;
      MPI_Bcast(&appsize, 1, MPI_UNSIGNED, acceptingLeadId, InterComm);
      DsmProcessStructure.push_back(std::pair<std::string, unsigned int>(std::string(appname), appsize));
    }
    --numConnections;
    MPI_Bcast(&numConnections, 1, MPI_INT, 0, InterComm);
  }
#endif
}

void
XdmfDSMCommMPI::AllGather(void *sendbuf,
                          int sendbytes,
                          void *recvbuf,
                          int recvbytes,
                          int comm)
{
  if (comm == XDMF_DSM_INTRA_COMM)
  {
    MPI_Allgather(sendbuf,
                  sendbytes,
                  MPI_UNSIGNED_CHAR,
                  recvbuf,
                  recvbytes,
                  MPI_UNSIGNED_CHAR,
                  IntraComm);
  }
  else if (comm == XDMF_DSM_INTER_COMM)
  {
    MPI_Allgather(sendbuf,
                  sendbytes,
                  MPI_UNSIGNED_CHAR,
                  recvbuf,
                  recvbytes,
                  MPI_UNSIGNED_CHAR,
                  InterComm);
  }
}

void
XdmfDSMCommMPI::Barrier(int comm)
{
  int status;

  if (comm == XDMF_DSM_INTRA_COMM)
  {
    status = MPI_Barrier(IntraComm);
  }
  else if (comm == XDMF_DSM_INTER_COMM)
  {
    status = MPI_Barrier(InterComm);
  }
}

void
XdmfDSMCommMPI::Broadcast(void * pointer,
                          int sizebytes,
                          int root,
                          int comm)
{
  int status;

  if (comm == XDMF_DSM_INTRA_COMM)
  {
    status = MPI_Bcast(pointer, sizebytes, MPI_UNSIGNED_CHAR, root, IntraComm);
  }
  else if (comm == XDMF_DSM_INTER_COMM)
  {
    status = MPI_Bcast(pointer, sizebytes, MPI_UNSIGNED_CHAR, root, InterComm);
  }
}

void
XdmfDSMCommMPI::ClosePort()
{
#ifndef XDMF_DSM_IS_CRAY
  if (Id == 0) {
    int status;
    for (unsigned int i = 0; i < PreviousDsmPortNames.size(); ++i)
    {
      status = MPI_Close_port(PreviousDsmPortNames[i]);
      if (status != MPI_SUCCESS) {
        try {// OpenMPI iterate through open ports in order to close the multiple needed
          std::string message = "Failed to close port ";
          message = message + PreviousDsmPortNames[i];
          XdmfError::message(XdmfError::FATAL, message);
        }
        catch (XdmfError & e) {
          throw e;
        }
      }
    }
  }
#endif
  HasOpenedPort = false;
}

int
XdmfDSMCommMPI::Connect()
{
#ifndef XDMF_DSM_IS_CRAY
  int status;
  MPI_Status mpistatus;
  if (InterComm == MPI_COMM_NULL) {
    this->DupInterComm(IntraComm);
  }
  MPI_Comm tempComm;
  MPI_Comm tempConnectComm;
  MPI_Comm_dup(InterComm, &tempConnectComm);
  MPI_Errhandler_set(InterComm, MPI_ERRORS_RETURN);
  status = MPI_Comm_connect(DsmPortName, MPI_INFO_NULL, 0, tempConnectComm, &tempComm);
  MPI_Errhandler_set(InterComm, MPI_ERRORS_ARE_FATAL);
  if (status != MPI_SUCCESS) {
    try {
      std::string message = "Failed to connect to port ";
      message = message + DsmPortName;
      XdmfError::message(XdmfError::FATAL, message);
    }
    catch (XdmfError & e) {
      throw e;
    }
  }
  MPI_Comm mergedComm;
  status = MPI_Intercomm_merge(tempComm, true, &mergedComm);
  this->DupInterComm(mergedComm);
  if (status != MPI_SUCCESS) {
    try {
      XdmfError::message(XdmfError::FATAL, "Failed to merge InterComm");
    }
    catch (XdmfError & e) {
      throw e;
    }
  }
  else {
    status = MPI_Comm_rank(InterComm, &InterId);
    status = MPI_Comm_size(InterComm, &InterSize);
  }

  //regen Intra comm from Inter Comm
  MPI_Group IntraGroup, InterGroup;
  MPI_Comm_group(InterComm, &InterGroup);
  int * ServerIds = (int *)calloc(this->IntraSize, sizeof(int));
  unsigned int index = 0;
  for(int i=this->InterId - this->Id; i < this->InterId - this->Id + this->IntraSize; ++i)
  {
    ServerIds[index++] = i;
  }
  MPI_Group_incl(InterGroup, this->IntraSize, ServerIds, &IntraGroup);
  MPI_Comm_create(InterComm, IntraGroup, &IntraComm);
  cfree(ServerIds);

  // Here the process will send information about itself to the server and
  // the server will diseminate that info across all the connected processes
  std::vector<std::pair<std::string, unsigned int> > structureArchive;
  // Archive old structure if it exists
  if (DsmProcessStructure.size() > 0)
  {
    for (unsigned int i = 0; i < DsmProcessStructure.size(); ++i)
    {
      structureArchive.push_back(DsmProcessStructure[i]);
    }
    DsmProcessStructure.clear();
  }
  // Loop in the applicaiton names of the already existing sections of the DSM
  // Get the number of application sections
  int numSections;
  if (this->Id == 0)
  {
    MPI_Recv(&numSections, 1, MPI_INT, 0, XDMF_DSM_EXCHANGE_TAG, this->InterComm, &mpistatus);
  }
  MPI_Bcast(&numSections, 1, MPI_INT, 0, IntraComm);
  unsigned int length;
  char * appname;
  unsigned int appsize;
  for (unsigned int i = 0; i < numSections; ++i)
  {
    if (this->Id == 0)
    {
      // Get the length of the name
      MPI_Recv(&length, 1, MPI_UNSIGNED, 0, XDMF_DSM_EXCHANGE_TAG, InterComm, &mpistatus);
      // Get the string of characters
      appname = new char[length+1]();
      MPI_Recv(appname, length, MPI_CHAR, 0, XDMF_DSM_EXCHANGE_TAG, InterComm, &mpistatus);
      appname[length] = 0;
      // Get associated numprocs
      MPI_Recv(&appsize, 1, MPI_UNSIGNED, 0, XDMF_DSM_EXCHANGE_TAG, InterComm, &mpistatus);
    }
    // Broadcast to local comm
    MPI_Bcast(&length, 1, MPI_UNSIGNED, 0, IntraComm);
    if (this->Id != 0)
    {
      appname = new char[length+1]();
    }
    MPI_Bcast(appname, length+1, MPI_CHAR, 0, IntraComm);
    MPI_Bcast(&appsize, 1, MPI_UNSIGNED, 0, IntraComm);

    DsmProcessStructure.push_back(std::pair<std::string, unsigned int>(std::string(appname), appsize));
  }
  if (structureArchive.size() == 0)
  {
    numSections = 1;
    MPI_Bcast(&numSections, 1, MPI_INT, InterId-Id, InterComm);
    length = ApplicationName.size();
    appsize = this->IntraSize;
    MPI_Bcast(&length, 1, MPI_UNSIGNED, InterId-Id, InterComm);
    appname = new char[length]();
    strcpy(appname, ApplicationName.c_str());
    MPI_Bcast(appname, length, MPI_CHAR, InterId-Id, InterComm);
    delete appname;
    MPI_Bcast(&appsize, 1, MPI_UNSIGNED, InterId-Id, InterComm);
    DsmProcessStructure.push_back(std::pair<std::string, unsigned int>(ApplicationName, appsize));
  }
  else
  {
    numSections = structureArchive.size();
    MPI_Bcast(&numSections, 1, MPI_UNSIGNED, InterId-Id, InterComm);
    for (unsigned int i = 0; i < numSections; ++i)
    {
      length = structureArchive[i].first.size();
      appsize = structureArchive[i].second;
      MPI_Bcast(&length, 1, MPI_UNSIGNED, InterId-Id, InterComm);
      appname = new char[length]();
      strcpy(appname, structureArchive[i].first.c_str());
      MPI_Bcast(appname, length, MPI_CHAR, InterId-Id, InterComm);
      delete appname;
      MPI_Bcast(&appsize, 1, MPI_UNSIGNED, InterId-Id, InterComm);
      DsmProcessStructure.push_back(std::pair<std::string, unsigned int>(structureArchive[i].first, appsize));
    }
  }
  int numAccepts = 0;
  MPI_Bcast(&numAccepts, 1, MPI_INT, 0, InterComm);
#ifdef OPEN_MPI
    if (numAccepts > 0) {
      MPI_Bcast(DsmPortName, MPI_MAX_PORT_NAME, MPI_CHAR, 0, InterComm);
    }
#endif
  Accept(numAccepts);
  return MPI_SUCCESS;
#endif
  return MPI_SUCCESS;
}

void
XdmfDSMCommMPI::Disconnect()
{
#ifndef XDMF_DSM_IS_CRAY
#ifndef OPEN_MPI
  if (InterComm != MPI_COMM_NULL) {
    int status = MPI_Comm_free(&InterComm);
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
#endif
  InterComm = MPI_COMM_NULL;
}

void
XdmfDSMCommMPI::DupComm(MPI_Comm comm)
{
  if (IntraComm != comm) {
    int status;
#ifndef OPEN_MPI
    if (IntraComm != MPI_COMM_NULL) {
      status = MPI_Comm_free(&IntraComm);
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
      status = MPI_Comm_dup(comm, &IntraComm);
      if (status != MPI_SUCCESS) {
        try {
          XdmfError::message(XdmfError::FATAL, "Failed to duplicate Comm");
        }
        catch (XdmfError & e) {
          throw e;
        }
      }
      else {
        status = MPI_Comm_size(IntraComm, &IntraSize);
        status = MPI_Comm_rank(IntraComm, &Id);
      }
    }
  }
}

void
XdmfDSMCommMPI::DupInterComm(MPI_Comm comm)
{
  if (InterComm != comm) {
    int status;
#ifndef OPEN_MPI
    if (InterComm != MPI_COMM_NULL) {
      status = MPI_Comm_free(&InterComm);
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
      status = MPI_Comm_dup(comm, &InterComm);
      if (status != MPI_SUCCESS) {
        try {
          XdmfError::message(XdmfError::FATAL, "Failed to duplicate Comm");
        }
        catch (XdmfError & e) {
          throw e;
        }
      }
      else {
        status = MPI_Comm_rank(InterComm, &InterId);
        status = MPI_Comm_size(InterComm, &InterSize);
      }
    }
    else {
      InterId = -1;
      InterSize = -1;
    }
  }
}

std::string
XdmfDSMCommMPI::GetApplicationName()
{
  return ApplicationName;
}

std::string
XdmfDSMCommMPI::GetDsmFileName()
{
  return DsmFileName;
}

char *
XdmfDSMCommMPI::GetDsmPortName()
{
  return DsmPortName;
}

std::vector<std::pair<std::string, unsigned int> >
XdmfDSMCommMPI::GetDsmProcessStructure()
{
  return DsmProcessStructure;
}

int
XdmfDSMCommMPI::GetId()
{
  return this->Id;
}

MPI_Comm
XdmfDSMCommMPI::GetInterComm()
{
  return InterComm;
}

int
XdmfDSMCommMPI::GetInterCommType()
{
  return this->InterCommType;
}

int
XdmfDSMCommMPI::GetInterId()
{
  return this->InterId;
}

int
XdmfDSMCommMPI::GetInterSize()
{
  return this->InterSize;
}

MPI_Comm
XdmfDSMCommMPI::GetIntraComm()
{
  return IntraComm;
}

int
XdmfDSMCommMPI::GetIntraSize()
{
  return this->IntraSize;
}

bool
XdmfDSMCommMPI::GetUseEnvFileName()
{
  return XdmfDSMCommMPI::UseEnvFileName;
}

void
XdmfDSMCommMPI::Init()
{
  int size, rank;
  if (MPI_Comm_size(this->IntraComm, &size) != MPI_SUCCESS) {
    try {
      XdmfError::message(XdmfError::FATAL, "Failed to initialize size");
    }
    catch (XdmfError & e) {
      throw e;
    }
  }
  if (MPI_Comm_rank(this->IntraComm, &rank) != MPI_SUCCESS) {
    try {
      XdmfError::message(XdmfError::FATAL, "Failed to initialize rank");
    }
    catch (XdmfError & e) {
      throw e;
    }
  }

  this->Id = rank;
  this->IntraSize = size;
}

void
XdmfDSMCommMPI::OpenPort()
{
  if (Id == 0) {
#ifndef XDMF_DSM_IS_CRAY
    int status = MPI_Open_port(MPI_INFO_NULL, DsmPortName);
    if (status != MPI_SUCCESS) {
      try {
        std::string message = "Failed to open port ";
        message = message + DsmPortName;
        XdmfError::message(XdmfError::FATAL, message);
      }
      catch (XdmfError & e) {
        throw e;
      }
    }
    PreviousDsmPortNames.push_back(DsmPortName);
#endif
    std::ofstream connectFile (DsmFileName.c_str());
    if (connectFile.is_open()) {
      connectFile << DsmPortName;
      connectFile.close();
    }
    else {
      try {
        XdmfError::message(XdmfError::FATAL, "Failed to write port to file");
      }
      catch (XdmfError & e) {
        throw e;
      }
    }
    HasOpenedPort = true;
  }
#ifndef XDMF_DSM_IS_CRAY
  MPI_Bcast(DsmPortName, MPI_MAX_PORT_NAME, MPI_CHAR, 0, IntraComm);
#endif
}

void
XdmfDSMCommMPI::Probe(int *comm)
{
  // Used for finding a comm that has a waiting command, then sets the comm
  int status = XDMF_DSM_FAIL;
  MPI_Status signalStatus;

  int flag;
  MPI_Comm probeComm =
    this->GetIntraComm();

  // Spin until a message is found on one of the communicators
  while (status != XDMF_DSM_SUCCESS) {
    status = MPI_Iprobe(XDMF_DSM_ANY_SOURCE,
                        XDMF_DSM_ANY_TAG,
                        probeComm,
                        &flag,
                        &signalStatus);
    if (status != MPI_SUCCESS)
    {
       try {
         XdmfError::message(XdmfError::FATAL,
                            "Error: Failed to probe for command header");
       }
       catch (XdmfError & e) {
         throw e;
       }
    }
    if (flag) {
      status = XDMF_DSM_SUCCESS;
    }
    else {
      if (this->GetInterComm() != MPI_COMM_NULL) {
        if (probeComm == this->GetIntraComm()) {
          probeComm = this->GetInterComm();
        }
        else {
          probeComm = this->GetIntraComm();
        }
      }
    }
  }
  if (probeComm == this->GetInterComm()) {
    *comm = XDMF_DSM_INTER_COMM;
  }
  else
  {
    *comm = XDMF_DSM_INTRA_COMM;
  }

  probeComm = MPI_COMM_NULL;
}

void
XdmfDSMCommMPI::ReadDsmPortName()
{
#ifndef XDMF_DSM_IS_CRAY
  std::ifstream connectFile(DsmFileName.c_str());
  std::string connectLine;
  if (connectFile.is_open()) {
    getline(connectFile, connectLine);
  }
  strcpy(DsmPortName, connectLine.c_str());
#endif
}

void
XdmfDSMCommMPI::Send(void * pointer,
                     int sizebytes,
                     int coreTo,
                     int comm,
                     int tag)
{
  int status;
  if (comm == XDMF_DSM_INTRA_COMM) {
    status = MPI_Send(pointer,
                      sizebytes,
                      MPI_UNSIGNED_CHAR,
                      coreTo,
                      tag,
                      IntraComm);
  }
  else if (comm == XDMF_DSM_INTER_COMM) {
    status = MPI_Send(pointer,
                      sizebytes,
                      MPI_UNSIGNED_CHAR,
                      coreTo,
                      tag,
                      InterComm);
  }
}

void
XdmfDSMCommMPI::Receive(void * pointer,
                        int sizebytes,
                        int coreFrom,
                        int comm,
                        int tag)
{
  int status;
  MPI_Status signalStatus;
  if (comm == XDMF_DSM_INTRA_COMM) {
    status = MPI_Recv(pointer,
                      sizebytes,
                      MPI_UNSIGNED_CHAR,
                      coreFrom,
                      tag,
                      IntraComm,
                      &signalStatus);
  }
  else if (comm == XDMF_DSM_INTER_COMM) {
    status = MPI_Recv(pointer,
                      sizebytes,
                      MPI_UNSIGNED_CHAR,
                      coreFrom,
                      tag,
                      InterComm,
                      &signalStatus);
  }
}

void
XdmfDSMCommMPI::SetApplicationName(std::string newName)
{
  ApplicationName = newName;
}

void
XdmfDSMCommMPI::SetDsmFileName(std::string filename)
{
  DsmFileName = filename;
}

void
XdmfDSMCommMPI::SetDsmPortName(const char *hostName)
{
  strcpy(DsmPortName, hostName);
}

void
XdmfDSMCommMPI::SetDsmProcessStructure(std::vector<std::pair<std::string, unsigned int> > & newStructure)
{
  DsmProcessStructure = newStructure;
}

void
XdmfDSMCommMPI::SetUseEnvFileName(bool status)
{
  XdmfDSMCommMPI::UseEnvFileName = status;
}

// C Wrappers

XDMFDSMCOMMMPI * XdmfDSMCommMPINew()
{
  try
  {
    return (XDMFDSMCOMMMPI *)((void *)(new XdmfDSMCommMPI()));
  }
  catch (...)
  {
    return (XDMFDSMCOMMMPI *)((void *)(new XdmfDSMCommMPI()));
  }
}

void XdmfDSMCommMPIFree(XDMFDSMCOMMMPI * item)
{
  if (item != NULL) {
    delete ((XdmfDSMCommMPI *)item);
  }
  item = NULL;
}

void XdmfDSMCommMPIAccept(XDMFDSMCOMMMPI * dsmComm, unsigned int numConnections, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMCommMPI *)dsmComm)->Accept(numConnections);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMCommMPIClosePort(XDMFDSMCOMMMPI * dsmComm, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMCommMPI *)dsmComm)->ClosePort();
  XDMF_ERROR_WRAP_END(status)
}

int XdmfDSMCommMPIConnect(XDMFDSMCOMMMPI * dsmComm, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  return ((XdmfDSMCommMPI *)dsmComm)->Connect();
  XDMF_ERROR_WRAP_END(status)
  return -1;
}

void XdmfDSMCommMPIDisconnect(XDMFDSMCOMMMPI * dsmComm, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMCommMPI *)dsmComm)->Disconnect();
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMCommMPIDupComm(XDMFDSMCOMMMPI * dsmComm, MPI_Comm comm, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMCommMPI *)dsmComm)->DupComm(comm);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMCommMPIDupInterComm(XDMFDSMCOMMMPI * dsmComm, MPI_Comm comm, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMCommMPI *)dsmComm)->DupInterComm(comm);
  XDMF_ERROR_WRAP_END(status)
}

char * XdmfDSMCommMPIGetApplicationName(XDMFDSMCOMMMPI * dsmComm)
{
  try
  {
    char * returnPointer = strdup(((XdmfDSMCommMPI *)dsmComm)->GetApplicationName().c_str());
    return returnPointer;
  }
  catch (...)
  {
    char * returnPointer = strdup(((XdmfDSMCommMPI *)dsmComm)->GetApplicationName().c_str());
    return returnPointer;
  }
}

char * XdmfDSMCommMPIGetDsmFileName(XDMFDSMCOMMMPI * dsmComm)
{
  try
  {
    char * returnPointer = strdup(((XdmfDSMCommMPI *)dsmComm)->GetDsmFileName().c_str());
    return returnPointer;
  }
  catch (...)
  {
    char * returnPointer = strdup(((XdmfDSMCommMPI *)dsmComm)->GetDsmFileName().c_str());
    return returnPointer;
  }
}

char * XdmfDSMCommMPIGetDsmPortName(XDMFDSMCOMMMPI * dsmComm)
{
  try
  {
    return ((XdmfDSMCommMPI *)dsmComm)->GetDsmPortName();
  }
  catch (...)
  {
    return ((XdmfDSMCommMPI *)dsmComm)->GetDsmPortName();
  }
}

void XdmfDSMCommMPIGetDsmProcessStructure(XDMFDSMCOMMMPI * dsmComm,
                                          char ** names,
                                          unsigned int * coreCount,
                                          int * numApplications)
{
  try
  {
    std::vector<std::pair<std::string, unsigned int> > structure =
      ((XdmfDSMCommMPI *)dsmComm)->GetDsmProcessStructure();
    *numApplications = structure.size();
    coreCount = new unsigned int[*numApplications]();
    names = new char *[*numApplications]();
    for (unsigned int i = 0; i < *numApplications; ++i)
    {
      coreCount[i] = structure[i].second;
      names[i] = strdup(structure[i].first.c_str());
    }
  }
  catch (...)
  {
    std::vector<std::pair<std::string, unsigned int> > structure =
      ((XdmfDSMCommMPI *)dsmComm)->GetDsmProcessStructure();
    *numApplications = structure.size();
    coreCount = new unsigned int[*numApplications]();
    names = new char *[*numApplications]();
    for (unsigned int i = 0; i < *numApplications; ++i)
    {
      coreCount[i] = structure[i].second;
      names[i] = strdup(structure[i].first.c_str());
    }
  }
}

int XdmfDSMCommMPIGetId(XDMFDSMCOMMMPI * dsmComm)
{
  try
  {
    return ((XdmfDSMCommMPI *)dsmComm)->GetId();
  }
  catch (...)
  {
    return ((XdmfDSMCommMPI *)dsmComm)->GetId();
  }
}

MPI_Comm XdmfDSMCommMPIGetInterComm(XDMFDSMCOMMMPI * dsmComm)
{
  try
  {
    return ((XdmfDSMCommMPI *)dsmComm)->GetInterComm();
  }
  catch (...)
  {
    return ((XdmfDSMCommMPI *)dsmComm)->GetInterComm();
  }
}

int XdmfDSMCommMPIGetInterCommType(XDMFDSMCOMMMPI * dsmComm)
{
  try
  {
    return ((XdmfDSMCommMPI *)dsmComm)->GetInterCommType();
  }
  catch (...)
  {
    return ((XdmfDSMCommMPI *)dsmComm)->GetInterCommType();
  }
}

int XdmfDSMCommMPIGetInterId(XDMFDSMCOMMMPI * dsmComm)
{
  try
  {
    return ((XdmfDSMCommMPI *)dsmComm)->GetInterId();
  }
  catch (...)
  {
    return ((XdmfDSMCommMPI *)dsmComm)->GetInterId();
  }
}

int XdmfDSMCommMPIGetInterSize(XDMFDSMCOMMMPI * dsmComm)
{
  try
  {
    return ((XdmfDSMCommMPI *)dsmComm)->GetInterSize();
  }
  catch (...)
  {
    return ((XdmfDSMCommMPI *)dsmComm)->GetInterSize();
  }
}

MPI_Comm XdmfDSMCommMPIGetIntraComm(XDMFDSMCOMMMPI * dsmComm)
{
  try
  {
    return ((XdmfDSMCommMPI *)dsmComm)->GetIntraComm();
  }
  catch (...)
  {
    return ((XdmfDSMCommMPI *)dsmComm)->GetIntraComm();
  }
}

int XdmfDSMCommMPIGetIntraSize(XDMFDSMCOMMMPI * dsmComm)
{
  try
  {
    return ((XdmfDSMCommMPI *)dsmComm)->GetIntraSize();
  }
  catch (...)
  {
    return ((XdmfDSMCommMPI *)dsmComm)->GetIntraSize();
  }
}

int XdmfDSMCommMPIGetUseEnvFileName(XDMFDSMCOMMMPI * dsmComm)
{
  try
  {
    return ((XdmfDSMCommMPI *)dsmComm)->GetUseEnvFileName();
  }
  catch (...)
  {
    return ((XdmfDSMCommMPI *)dsmComm)->GetUseEnvFileName();
  }
}

void XdmfDSMCommMPIInit(XDMFDSMCOMMMPI * dsmComm, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMCommMPI *)dsmComm)->Init();
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMCommMPIOpenPort(XDMFDSMCOMMMPI * dsmComm, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMCommMPI *)dsmComm)->OpenPort();
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMCommMPIReadDsmPortName(XDMFDSMCOMMMPI * dsmComm)
{
  try
  {
    ((XdmfDSMCommMPI *)dsmComm)->ReadDsmPortName();
  }
  catch (...)
  {
    ((XdmfDSMCommMPI *)dsmComm)->ReadDsmPortName();
  }
}

void XdmfDSMCommMPISetApplicationName(XDMFDSMCOMMMPI * dsmComm, char * newName)
{
  try
  {
    ((XdmfDSMCommMPI *)dsmComm)->SetApplicationName(std::string(newName));
  }
  catch (...)
  {
    ((XdmfDSMCommMPI *)dsmComm)->SetApplicationName(std::string(newName));
  }
}

void XdmfDSMCommMPISetDsmFileName(XDMFDSMCOMMMPI * dsmComm, char * filename)
{
  try
  {
    ((XdmfDSMCommMPI *)dsmComm)->SetDsmFileName(std::string(filename));
  }
  catch (...)
  {
    ((XdmfDSMCommMPI *)dsmComm)->SetDsmFileName(std::string(filename));
  }
}

void XdmfDSMCommMPISetDsmPortName(XDMFDSMCOMMMPI * dsmComm, char * hostName)
{
  try
  {
    ((XdmfDSMCommMPI *)dsmComm)->SetDsmPortName(hostName);
  }
  catch (...)
  {
    ((XdmfDSMCommMPI *)dsmComm)->SetDsmPortName(hostName);
  }
}

void XdmfDSMCommMPISetUseEnvFileName(XDMFDSMCOMMMPI * dsmComm, int status)
{
  try
  {
    ((XdmfDSMCommMPI *)dsmComm)->SetUseEnvFileName(status);
  }
  catch (...)
  {
    ((XdmfDSMCommMPI *)dsmComm)->SetUseEnvFileName(status);
  }
}
