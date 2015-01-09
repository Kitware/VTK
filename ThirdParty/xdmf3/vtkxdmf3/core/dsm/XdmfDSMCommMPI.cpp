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
#include <fstream>
#include <iostream>

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
  InterCommType = XDMF_DSM_COMM_MPI;
  HasOpenedPort = false;
}

XdmfDSMCommMPI::~XdmfDSMCommMPI()
{
#ifndef OPEN_MPI
  if (InterComm != MPI_COMM_NULL) {
    int status = MPI_Comm_free(&InterComm);
    if (status != MPI_SUCCESS) {
      XdmfError::message(XdmfError::FATAL, "Failed to free intercomm Comm");
    }
  }
  if (IntraComm != MPI_COMM_NULL) {
    int status = MPI_Comm_free(&IntraComm);
    if (status != MPI_SUCCESS) {
      XdmfError::message(XdmfError::FATAL, "Failed to free intercomm Comm");
    }
  }
#endif
}

void
XdmfDSMCommMPI::Accept(unsigned int numConnections)
{
  while (numConnections > 0) {
    if (InterComm == MPI_COMM_NULL) {
      // If there is no InterComm, then accept from IntraComm and merge into InterComm
      MPI_Comm tempComm;
      int * portCheck = new int[GetInterSize()]();
      int portStatus;
      portStatus = 0;
      if (HasOpenedPort) {
        portStatus = 1;
      }
      MPI_Allgather(&portStatus, 1, MPI_INT, &(portCheck[0]), 1, MPI_INT, InterComm);
      unsigned int index = 0;
      for (index = 0; index < GetInterSize(); ++index) {
        if (portCheck[index] == 1) {
          break;
        }
      }
      int status = MPI_Comm_accept(DsmPortName, MPI_INFO_NULL, index, IntraComm, &tempComm);
      if (status != MPI_SUCCESS) {
        std::string message = "Failed to accept port ";
        message = message + DsmPortName;
        XdmfError::message(XdmfError::FATAL, message);
      }
      // False is specified for high so that the index of the cores doesn't change
      status = MPI_Intercomm_merge(tempComm, false, &InterComm);
      if (status != MPI_SUCCESS) {
        XdmfError::message(XdmfError::FATAL, "Failed to merge intercomm");
      }
      else {
        MPI_Comm_rank(InterComm, &InterId);
        MPI_Comm_size(InterComm, &InterSize);
      }
    }
    else {
      // If there is an InterComm, accept into the InterComm and merge
      MPI_Comm tempComm;
      int * portCheck = new int[GetInterSize()]();
      int portStatus;
      portStatus = 0;
      if (HasOpenedPort) {
        portStatus = 1;
      }
      MPI_Allgather(&portStatus, 1, MPI_INT, &(portCheck[0]), 1, MPI_INT, InterComm);
      unsigned int index = 0;
      for (index = 0; index < GetInterSize(); ++index) {
        if (portCheck[index] == 1) {
          break;
        }
      }
      int status = MPI_Comm_accept(DsmPortName, MPI_INFO_NULL, index, InterComm, &tempComm);
      if (status != MPI_SUCCESS) {
        std::string message = "Failed to accept port ";
        message = message + DsmPortName;
        XdmfError::message(XdmfError::FATAL, message);
      }
      // False is specified for high so that the index of the cores doesn't change
      status = MPI_Intercomm_merge(tempComm, false, &InterComm);
      if (status != MPI_SUCCESS) {
        XdmfError::message(XdmfError::FATAL, "Failed to merge InterComm");
      }
      else {
        MPI_Comm_rank(InterComm, &InterId);
        MPI_Comm_size(InterComm, &InterSize);
      }
    }
    --numConnections;
    MPI_Bcast(&numConnections, 1, MPI_INT, 0, InterComm);
  }
}

void
XdmfDSMCommMPI::ClosePort()
{
  if (Id == 0) {
    int status;
    status = MPI_Close_port(DsmPortName);
    if (status != MPI_SUCCESS) {
      std::string message = "Failed to close port ";
      message = message + DsmPortName;
      XdmfError::message(XdmfError::FATAL, message);
    }
  }
  HasOpenedPort = false;
}

int
XdmfDSMCommMPI::Connect()
{
  if (InterComm == MPI_COMM_NULL) {
    MPI_Comm tempComm;
    MPI_Errhandler_set(IntraComm, MPI_ERRORS_RETURN);
    int status = MPI_Comm_connect(DsmPortName, MPI_INFO_NULL, 0, IntraComm, &tempComm);
    MPI_Errhandler_set(IntraComm, MPI_ERRORS_ARE_FATAL);
    if (status != MPI_SUCCESS) {
      std::string message = "Failed to connect to port ";
      message = message + DsmPortName;
      XdmfError::message(XdmfError::FATAL, message);
    }
    status = MPI_Intercomm_merge(tempComm, true, &InterComm);
    if (status != MPI_SUCCESS) {
      XdmfError::message(XdmfError::FATAL, "Failed to merge InterComm");
    }
    else {
      status = MPI_Comm_rank(InterComm, &InterId);
      status = MPI_Comm_size(InterComm, &InterSize);
    }
  }
  else {
    MPI_Comm tempComm;
    MPI_Errhandler_set(InterComm, MPI_ERRORS_RETURN);
    int status = MPI_Comm_connect(DsmPortName, MPI_INFO_NULL, 0, InterComm, &tempComm);
    MPI_Errhandler_set(InterComm, MPI_ERRORS_ARE_FATAL);
    if (status != MPI_SUCCESS) {
      std::string message = "Failed to connect to port ";
      message = message + DsmPortName;
      XdmfError::message(XdmfError::FATAL, message);
    }
    status = MPI_Intercomm_merge(tempComm, true, &InterComm);
    if (status != MPI_SUCCESS) {
      XdmfError::message(XdmfError::FATAL, "Failed to merge InterComm");
    }
    else {
      status = MPI_Comm_rank(InterComm, &InterId);
      status = MPI_Comm_size(InterComm, &InterSize);
    }
  }
  int numAccepts = 0;
  MPI_Bcast(&numAccepts, 1, MPI_INT, 0, InterComm);
  Accept(numAccepts);
  return MPI_SUCCESS;
}

void
XdmfDSMCommMPI::Disconnect()
{
#ifndef OPEN_MPI
  if (InterComm != MPI_COMM_NULL) {
    int status = MPI_Comm_free(&InterComm);
    if (status != MPI_SUCCESS) {
      XdmfError::message(XdmfError::FATAL, "Failed to disconnect Comm");
    }
  }
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
        XdmfError::message(XdmfError::FATAL, "Failed to disconnect Comm");
      }
    }
#endif
    if (comm != MPI_COMM_NULL) {
      status = MPI_Comm_dup(comm, &IntraComm);
      if (status != MPI_SUCCESS) {
        XdmfError::message(XdmfError::FATAL, "Failed to duplicate Comm");
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
        XdmfError::message(XdmfError::FATAL, "Failed to disconnect Comm");
      }
    }
#endif
    if (comm != MPI_COMM_NULL) {
      status = MPI_Comm_dup(comm, &InterComm);
      if (status != MPI_SUCCESS) {
        XdmfError::message(XdmfError::FATAL, "Failed to duplicate Comm");
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
XdmfDSMCommMPI::GetDsmFileName()
{
  return DsmFileName;
}

char *
XdmfDSMCommMPI::GetDsmPortName()
{
  return DsmPortName;
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

void
XdmfDSMCommMPI::Init()
{
  int size, rank;
  if (MPI_Comm_size(this->IntraComm, &size) != MPI_SUCCESS) {
    XdmfError::message(XdmfError::FATAL, "Failed to initialize size");
  }
  if (MPI_Comm_rank(this->IntraComm, &rank) != MPI_SUCCESS) {
    XdmfError::message(XdmfError::FATAL, "Failed to initialize rank");
  }

  this->Id = rank;
  this->IntraSize = size;
}

void
XdmfDSMCommMPI::OpenPort()
{
  if (Id == 0) {
    int status = MPI_Open_port(MPI_INFO_NULL, DsmPortName);
    if (status != MPI_SUCCESS) {
      std::string message = "Failed to open port ";
      message = message + DsmPortName;
      XdmfError::message(XdmfError::FATAL, message);
    }
    std::ofstream connectFile (DsmFileName.c_str());
    if (connectFile.is_open()) {
      connectFile << DsmPortName;
      connectFile.close();
    }
    else {
      XdmfError::message(XdmfError::FATAL, "Failed to write port to file");
    }
    HasOpenedPort = true;
  }
  MPI_Bcast(DsmPortName, MPI_MAX_PORT_NAME, MPI_CHAR, 0, IntraComm);
}

void
XdmfDSMCommMPI::ReadDsmPortName()
{
  std::ifstream connectFile(DsmFileName.c_str());
  std::string connectLine;
  if (connectFile.is_open()) {
    getline(connectFile, connectLine);
  }
  strcpy(DsmPortName, connectLine.c_str());
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
