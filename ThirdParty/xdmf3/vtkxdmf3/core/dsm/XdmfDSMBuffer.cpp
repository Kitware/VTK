/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfDSMBuffer.hpp                                                   */
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
  Module                  : H5FDdsmBufferService.cxx H5FDdsmBuffer.cxx

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
#include <string.h>
#include <stdlib.h>
#include <algorithm>

#ifndef _WIN32
  #include <unistd.h>
#endif

XdmfDSMBuffer::XdmfDSMBuffer()
{
  this->CommChannel = XDMF_DSM_INTER_COMM;
  this->IsServer = true;
  this->StartAddress = this->EndAddress = 0;
  this->StartServerId = this->EndServerId = -1;
  this->Length = 0;
  this->TotalLength = 0;
  this->BlockLength = 0;
  this->Comm = NULL;
  this->DataPointer = NULL;
  this->IsConnected = false;
}

XdmfDSMBuffer::~XdmfDSMBuffer()
{
  if (this->DataPointer) {
    free(this->DataPointer);
  }
  this->DataPointer = NULL;
}

class XdmfDSMBuffer::CommandMsg
{
  public:
    int Opcode;
    int Source;
    int  Target;
    int Address;
    int Length;
};

class XdmfDSMBuffer::InfoMsg
{
  public:
    int type;
    unsigned int length;
    unsigned int total_length;
    unsigned int block_length;
    int start_server_id;
    int end_server_id;
};

int
XdmfDSMBuffer::AddressToId(int Address)
{
  int   ServerId = XDMF_DSM_FAIL;

  switch(this->DsmType) {
    case XDMF_DSM_TYPE_UNIFORM :
    case XDMF_DSM_TYPE_UNIFORM_RANGE :
      // All Servers have same length
      // This finds out which server the address provided starts on
      ServerId = this->StartServerId + (Address / this->Length);
      if(ServerId > this->EndServerId ){
        std::stringstream message;
        message << "ServerId " << ServerId << " for Address "
                << Address << " is larger than EndServerId "
                << this->EndServerId;
        XdmfError::message(XdmfError::FATAL, message.str());
      }
      break;
    default :
      // Not Implemented
      std::stringstream message;
      message << "DsmType " << this->DsmType << " not yet implemented";
      XdmfError::message(XdmfError::FATAL, message.str());
      break;
    }
    return(ServerId);
}

void
XdmfDSMBuffer::BroadcastComm(int *comm, int root)
{
  int status;

  status = MPI_Bcast(comm, sizeof(int), MPI_UNSIGNED_CHAR, root, this->Comm->GetIntraComm());
  if (status != MPI_SUCCESS) {
    XdmfError(XdmfError::FATAL, "Broadcast of Comm failed");
  }
}

int
XdmfDSMBuffer::BufferService(int *returnOpcode)
{
  int        opcode, who;
  int        aLength;
  int          address;
  char        *datap;
  static int syncId      = -1;

  if (this->CommChannel == XDMF_DSM_ANY_COMM) {
    if (this->Comm->GetId() == 0) {
      this->ProbeCommandHeader(&this->CommChannel);
    }
    this->BroadcastComm(&this->CommChannel, 0);
  }

  this->ReceiveCommandHeader(&opcode,
                             &who,
                             &address,
                             &aLength,
                             this->CommChannel,
                             syncId);

  // Connection is an ID for client or server,
//  int communicatorId = this->CommChannel;

  switch(opcode) {

  // H5FD_DSM_OPCODE_PUT
  case XDMF_DSM_OPCODE_PUT:
    if (((unsigned int) aLength + address) > this->Length) {
      std::stringstream message;
      message << "Length " << aLength << " too long for Address " << address 
              << "\n" << "Server Start = " << this->StartAddress << " End = "
              << this->EndAddress;
      XdmfError::message(XdmfError::FATAL, message.str());
    }
    if ((datap = this->DataPointer) == NULL) {
       XdmfError::message(XdmfError::FATAL,
                          "Null Data Pointer when trying to put data");
    }
    datap += address;
    this->ReceiveData(who,
                      datap,
                      aLength,
                      XDMF_DSM_PUT_DATA_TAG,
                      0,
                      this->CommChannel);
    break;

  // H5FD_DSM_OPCODE_GET
  case XDMF_DSM_OPCODE_GET:
    if (((unsigned int) aLength + address) > this->Length) {
      std::stringstream message;
      message << "Length " << aLength << " too long for Address " << address
              << "\n" << "Server Start = " << this->StartAddress << " End = "
              << this->EndAddress;
      XdmfError::message(XdmfError::FATAL, message.str());
    }
    if ((datap = this->DataPointer) == NULL) {
       XdmfError::message(XdmfError::FATAL,
                          "Null Data Pointer when trying to put data");
    }
    datap += address;
    this->SendData(who,
                   datap,
                   aLength,
                   XDMF_DSM_GET_DATA_TAG,
                   0,
                   this->CommChannel);
    break;

// H5FD_DSM_ACCEPT
  // Comes from client
  case XDMF_DSM_ACCEPT:
  {
    int numConnections;	
    this->ReceiveAcknowledgment(who,
                                numConnections,
                                XDMF_DSM_EXCHANGE_TAG,
                                XDMF_DSM_INTER_COMM);
    this->Comm->Accept(numConnections);
    this->SendInfo();
    break;
  }
  // H5FD_DSM_LOCK_ACQUIRE
  // Comes from client or server depending on communicator
  case XDMF_DSM_LOCK_ACQUIRE:
    // Currently unsupported
    break;

  // H5FD_DSM_LOCK_RELEASE
  // Comes from client or server depending on communicator
  case XDMF_DSM_LOCK_RELEASE:
    // Currently unsupported
    break;

  // H5FD_DSM_OPCODE_DONE
  // Always received on server
  case XDMF_DSM_OPCODE_DONE:
    break;

  // DEFAULT
  default :
    std::stringstream message;
    message << "Error: Unknown Opcode " << opcode;
    XdmfError::message(XdmfError::FATAL, message.str());
  }

  if (returnOpcode) *returnOpcode = opcode;
  return(XDMF_DSM_SUCCESS);
}

void
XdmfDSMBuffer::BufferServiceLoop(int *returnOpcode)
{
  int op, status = XDMF_DSM_SUCCESS;
  while (status == XDMF_DSM_SUCCESS) {
    status = this->BufferService(&op);
    if (returnOpcode) *returnOpcode = op;
    if (op == XDMF_DSM_OPCODE_DONE) {
      break;
    }
  }
}

void
XdmfDSMBuffer::ConfigureUniform(XdmfDSMCommMPI *aComm, long aLength,
                                int startId, int endId, long aBlockLength,
                                bool random)
{
  if (startId < 0) {
    startId = 0;
  }
  if (endId < 0) {
    endId = aComm->GetIntraSize() - 1;
  }
  this->SetDsmType(XDMF_DSM_TYPE_UNIFORM_RANGE);
  if ((startId == 0) && (endId == aComm->GetIntraSize() - 1)) {
    this->SetDsmType(XDMF_DSM_TYPE_UNIFORM);
  }
  if (aBlockLength) {
    if (!random) {
      this->SetDsmType(XDMF_DSM_TYPE_BLOCK_CYCLIC);
    }
    else {
      this->SetDsmType(XDMF_DSM_TYPE_BLOCK_RANDOM);
    }
    this->SetBlockLength(aBlockLength);
  }
  this->StartServerId = startId;
  this->EndServerId = endId;
  this->SetComm(aComm);
  if ((aComm->GetId() >= startId) &&
      (aComm->GetId() <= endId) &&
      this->IsServer) {
    if (aBlockLength) {
      // For optimization we make the DSM length fit to a multiple of block size
      this->SetLength(((long)(aLength / aBlockLength)) * aBlockLength);
    }
    else {
      this->SetLength(aLength);
    }
    this->StartAddress = (aComm->GetId() - startId) * aLength;
    this->EndAddress = this->StartAddress + aLength - 1;
  }
  else {
    if (aBlockLength) {
      this->Length = ((long)(aLength / aBlockLength)) * aBlockLength;
    }
    else {
      this->Length = aLength;
    }
  }
  this->TotalLength = this->GetLength() * (endId - startId + 1);
}

void
XdmfDSMBuffer::Get(long Address, long aLength, void *Data)
{
  int   who, MyId = this->Comm->GetInterId();
  int   astart, aend, len;
  char   *datap = (char *)Data;

  // While there is length left
  while(aLength) {
    // Figure out what server core the address is located on
    who = this->AddressToId(Address);
    if(who == XDMF_DSM_FAIL){
      XdmfError::message(XdmfError::FATAL, "Address Error");
    }
    // Get the start and end of the block listed
    this->GetAddressRangeForId(who, &astart, &aend);
    // Determine the amount of data to be written to that core
    // Basically, it's how much data will fit from
    // the starting point of the address to the end
    len = std::min(aLength, aend - Address + 1);
    // If the data is on the core running this code, then the put is simple
    if(who == MyId){
      char *dp;
      dp = this->DataPointer;
      dp += Address - this->StartAddress;
      memcpy(datap, dp, len);
    }
    else{
      // Otherwise send it to the appropriate core to deal with
      int   dataComm = XDMF_DSM_INTRA_COMM;
      if (this->Comm->GetInterComm() != MPI_COMM_NULL) {
        dataComm = XDMF_DSM_INTER_COMM;
      }
      this->SendCommandHeader(XDMF_DSM_OPCODE_GET, who, Address - astart, len, dataComm);
      this->ReceiveData(who, datap, len, XDMF_DSM_GET_DATA_TAG, Address - astart, dataComm);
    }
    // Shift all the numbers by the length of the data written
    // Until aLength = 0
    aLength -= len;
    Address += len;
    datap += len;
  }
}

void
XdmfDSMBuffer::GetAddressRangeForId(int Id, int *Start, int *End){
    switch(this->DsmType) {
      case XDMF_DSM_TYPE_UNIFORM :
      case XDMF_DSM_TYPE_UNIFORM_RANGE :
        // All Servers have same length
        // Start index is equal to the id inside the servers times
        // the length of the block per server
        // It is the starting index of the server's data block relative
        // to the entire block
        *Start = (Id - this->StartServerId) * this->Length;
        // End index is simply the start index + the length of the
        // server's data block.
        // The range produced is the start of the server's data block to its end.
        *End = *Start + Length - 1;
        break;
      default :
        // Not Implemented
        std::stringstream message;
        message << "DsmType " << this->DsmType << " not yet implemented";
        XdmfError::message(XdmfError::FATAL, message.str());
        break;
    }
}

long
XdmfDSMBuffer::GetBlockLength()
{
  return this->BlockLength;
}

XdmfDSMCommMPI *
XdmfDSMBuffer::GetComm()
{
  return this->Comm;
}

char *
XdmfDSMBuffer::GetDataPointer()
{
  return this->DataPointer;
}

int
XdmfDSMBuffer::GetDsmType()
{
  return this->DsmType;
}

int
XdmfDSMBuffer::GetEndAddress()
{
  return this->EndAddress;
}

int
XdmfDSMBuffer::GetEndServerId()
{
  return this->EndServerId;
}

bool
XdmfDSMBuffer::GetIsConnected()
{
  return IsConnected;
}

bool
XdmfDSMBuffer::GetIsServer()
{
  return this->IsServer;
}

long
XdmfDSMBuffer::GetLength()
{
  return this->Length;
}

int
XdmfDSMBuffer::GetStartAddress()
{
  return this->StartAddress;
}

int
XdmfDSMBuffer::GetStartServerId()
{
  return this->StartServerId;
}

long
XdmfDSMBuffer::GetTotalLength()
{
  return this->TotalLength;
}

void
XdmfDSMBuffer::ProbeCommandHeader(int *comm)
{
  // Used for finding a comm that has a waiting command, then sets the comm
  int status = XDMF_DSM_FAIL;
  MPI_Status signalStatus;

  int flag;
  MPI_Comm probeComm =
    static_cast<XdmfDSMCommMPI *>(this->Comm)->GetIntraComm();

  // Spin until a message is found on one of the communicators
  while (status != XDMF_DSM_SUCCESS) {
    status = MPI_Iprobe(XDMF_DSM_ANY_SOURCE,
                        XDMF_DSM_ANY_TAG,
                        probeComm,
                        &flag,
                        &signalStatus);
    if (status != MPI_SUCCESS)
    {
       XdmfError::message(XdmfError::FATAL,
                          "Error: Failed to probe for command header");
    }
    if (flag) {
      status = XDMF_DSM_SUCCESS;
    }
    else {
      if (static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm() != MPI_COMM_NULL) {
        if (probeComm == static_cast<XdmfDSMCommMPI *>(this->Comm)->GetIntraComm()) {
          probeComm = static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm();
  }
  else {
          probeComm = static_cast<XdmfDSMCommMPI *>(this->Comm)->GetIntraComm();
  }
      }
    }
  }
  if (probeComm == static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm()) {
    *comm = XDMF_DSM_INTER_COMM;
  }
  else
  {
    *comm = XDMF_DSM_INTRA_COMM;
  }

  probeComm = MPI_COMM_NULL;
}

void
XdmfDSMBuffer::Put(long Address, long aLength, const void *Data)
{
  int   who, MyId = this->Comm->GetInterId();
  int   astart, aend, len;
  char    *datap = (char *)Data;

  // While there is length left
  while(aLength){
    // Figure out what server core the address is located on
    who = this->AddressToId(Address);
    if(who == XDMF_DSM_FAIL){
      XdmfError::message(XdmfError::FATAL, "Address Error");
    }
    // Get the start and end of the block listed
    this->GetAddressRangeForId(who, &astart, &aend);
    // Determine the amount of data to be written to that core
    // Basically, it's how much data will fit from the starting point of
    // the address to the end
    len = std::min(aLength, aend - Address + 1);
    // If the data is on the core running this code, then the put is simple
    if(who == MyId){
      char *dp;
      dp = this->DataPointer;
      dp += Address - this->StartAddress;
      memcpy(dp, datap, len);
    }
    else{
      // Otherwise send it to the appropriate core to deal with
      int   status;
      int   dataComm = XDMF_DSM_INTRA_COMM;
      if (this->Comm->GetInterComm() != MPI_COMM_NULL) {
        dataComm = XDMF_DSM_INTER_COMM;
      }
      this->SendCommandHeader(XDMF_DSM_OPCODE_PUT,
                              who,
                              Address - astart,
                              len,
                              dataComm);
      this->SendData(who,
                     datap, 
                     len,
                     XDMF_DSM_PUT_DATA_TAG,
                     Address - astart,
                     dataComm);
    }
    // Shift all the numbers by the length of the data written
    // Until aLength = 0
    aLength -= len;
    Address += len;
    datap += len;
  }
}

void
XdmfDSMBuffer::ReceiveAcknowledgment(int source, int &data, int tag, int comm)
{
  int status;
  MPI_Status signalStatus;
  if (comm == XDMF_DSM_INTRA_COMM) {
    status = MPI_Recv(&data, 
                      sizeof(int), 
                      MPI_UNSIGNED_CHAR, 
                      source, 
                      tag, 
                      static_cast<XdmfDSMCommMPI *>(this->Comm)->GetIntraComm(),
                      &signalStatus);
  }
  else if (comm == XDMF_DSM_INTER_COMM) {
    status = MPI_Recv(&data,
                      sizeof(int),
                      MPI_UNSIGNED_CHAR,
                      source,
                      tag,
                      static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm(),
                      &signalStatus);
  }
  else {
    MPI_Comm tempComm = MPI_Comm_f2c(comm);
    status = MPI_Recv(&data,
                      sizeof(int),
                      MPI_UNSIGNED_CHAR,
                      source,
                      tag,
                      tempComm,
                      &signalStatus);
  }

  if (status != MPI_SUCCESS) {
    XdmfError::message(XdmfError::FATAL, "Error: Failed to receive data");
  }
}

void
XdmfDSMBuffer::ReceiveCommandHeader(int *opcode, int *source, int *address, int *aLength, int comm, int remoteSource)
{
  CommandMsg cmd;
  memset(&cmd, 0, sizeof(CommandMsg));
  int status = MPI_ERR_OTHER;
  MPI_Status signalStatus;

  if (remoteSource < 0) {
    remoteSource = MPI_ANY_SOURCE;
  }

  if (comm == XDMF_DSM_INTRA_COMM) {
    status = MPI_Recv(&cmd,
                      sizeof(CommandMsg),
                      MPI_UNSIGNED_CHAR,
                      remoteSource,
                      XDMF_DSM_COMMAND_TAG,
                      static_cast<XdmfDSMCommMPI *>(this->Comm)->GetIntraComm(),
                      &signalStatus);
  }
  else if (comm == XDMF_DSM_INTER_COMM) {
    status = MPI_Recv(&cmd,
                      sizeof(CommandMsg),
                      MPI_UNSIGNED_CHAR,
                      remoteSource,
                      XDMF_DSM_COMMAND_TAG,
                      static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm(),
                      &signalStatus);
  }
  else {
    // In this case the integer is probably a pointer to an MPI_Comm object
    MPI_Comm tempComm = MPI_Comm_f2c(comm);
    status = MPI_Recv(&cmd,
                      sizeof(CommandMsg),
                      MPI_UNSIGNED_CHAR,
                      remoteSource,
                      XDMF_DSM_COMMAND_TAG,
                      tempComm,
                      &signalStatus);
  }

  if (status != MPI_SUCCESS) {
    XdmfError::message(XdmfError::FATAL, "Error: Failed to receive command header");
  }
  else {
    *opcode  = cmd.Opcode;
                *source  = cmd.Source;
    *address = cmd.Address;
    *aLength = cmd.Length;
  }
}

void
XdmfDSMBuffer::ReceiveData(int source, char * data, int aLength, int tag, int aAddress, int comm)
{
  int status;
  MPI_Status signalStatus;
  if (comm == XDMF_DSM_INTRA_COMM) {
    status = MPI_Recv(data,
                      aLength,
                      MPI_UNSIGNED_CHAR,
                      source,
                      tag,
                      static_cast<XdmfDSMCommMPI *>(this->Comm)->GetIntraComm(),
                      &signalStatus);
  }
  else if (comm == XDMF_DSM_INTER_COMM) {
    status = MPI_Recv(data,
                      aLength,
                      MPI_UNSIGNED_CHAR,
                      source,
                      tag,
                      static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm(),
                      &signalStatus);
  }
  else {
    MPI_Comm tempComm = MPI_Comm_f2c(comm);
    status = MPI_Recv(data,
                      aLength,
                      MPI_UNSIGNED_CHAR,
                      source,
                      tag,
                      tempComm,
                      &signalStatus);
  }
  if (status != MPI_SUCCESS) {
    XdmfError::message(XdmfError::FATAL, "Error: Failed to receive data");
  }
}

void
XdmfDSMBuffer::ReceiveInfo()
{
  InfoMsg  dsmInfo;
  int status;

  memset(&dsmInfo, 0, sizeof(InfoMsg));

  int infoStatus = 0;

  if (this->Comm->GetId() == 0) {
    infoStatus = 1;
  }

  int * groupInfoStatus = new int[this->Comm->GetInterSize()]();

  MPI_Allgather(&infoStatus, 1, MPI_INT, &(groupInfoStatus[0]), 1, MPI_INT, this->Comm->GetInterComm());

  int sendCore = 0;

  for (int i = 0; i < this->Comm->GetInterSize(); ++i) {
    if (groupInfoStatus[i] == 2) {
      sendCore = i;
    }
  }
  status = MPI_Bcast(&dsmInfo, 
                     sizeof(InfoMsg),
                     MPI_UNSIGNED_CHAR,
                     sendCore,
                     static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm());
  if (status != MPI_SUCCESS) {
    XdmfError::message(XdmfError::FATAL, "Error: Failed to broadcast info");
  }
  this->SetDsmType(dsmInfo.type);
  // We are a client so don't allocate anything but only set a virtual remote length
  this->SetLength(dsmInfo.length);
  this->TotalLength = dsmInfo.total_length;
  this->SetBlockLength(dsmInfo.block_length);
  this->StartServerId = dsmInfo.start_server_id;
  this->EndServerId = dsmInfo.end_server_id;
}

void
XdmfDSMBuffer::SendAccept(unsigned int numConnections)
{
  for (int i = this->StartServerId; i <= this->EndServerId; ++i) {
    if (i != this->Comm->GetInterId()){
      this->SendCommandHeader(XDMF_DSM_ACCEPT, i, 0, 0, XDMF_DSM_INTER_COMM);
      this->SendAcknowledgment(i, numConnections, XDMF_DSM_EXCHANGE_TAG, XDMF_DSM_INTER_COMM);
    }
  }
  this->Comm->Accept(numConnections);
  this->SendInfo();
}

void
XdmfDSMBuffer::SendAcknowledgment(int dest, int data, int tag, int comm)
{
  int status;

  if (comm == XDMF_DSM_INTRA_COMM) {
    status = MPI_Send(&data,
             sizeof(int),
             MPI_UNSIGNED_CHAR,
             dest,
             tag,
             static_cast<XdmfDSMCommMPI *>(this->Comm)->GetIntraComm());
  }
  else if (comm == XDMF_DSM_INTER_COMM) {
    status = MPI_Send(&data,
                      sizeof(int),
                      MPI_UNSIGNED_CHAR,
                      dest,
                      tag,
                      static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm());
  }
  else {
    MPI_Comm tempComm = MPI_Comm_f2c(comm);
    status = MPI_Send(&data,
                      sizeof(int),
                      MPI_UNSIGNED_CHAR,
                      dest,
                      tag,
                      tempComm);
  }
  if (status != MPI_SUCCESS) {
    XdmfError::message(XdmfError::FATAL, "Error: Failed to receive data");
  }
}

void
XdmfDSMBuffer::SendCommandHeader(int opcode, int dest, int address, int aLength, int comm)
{
  int status;
  CommandMsg cmd;
  memset(&cmd, 0, sizeof(CommandMsg));
  cmd.Opcode = opcode;
  cmd.Source = this->Comm->GetId();
  cmd.Target = dest;
  cmd.Address = address;
  cmd.Length = aLength;

  if (comm == XDMF_DSM_INTRA_COMM) {
    status = MPI_Send(&cmd,
                      sizeof(CommandMsg),
                      MPI_UNSIGNED_CHAR,
                      dest,
                      XDMF_DSM_COMMAND_TAG,
                      static_cast<XdmfDSMCommMPI *>(this->Comm)->GetIntraComm());
  }
  else if (comm == XDMF_DSM_INTER_COMM) {
    int interSource = 0;
    MPI_Comm_rank(static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm(), &interSource);
    cmd.Source = interSource;
    status = MPI_Send(&cmd,
                      sizeof(CommandMsg),
                      MPI_UNSIGNED_CHAR,
                      dest,
                      XDMF_DSM_COMMAND_TAG,
                      static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm());
  }
  else {
    // In this case the comm should be a pointer to an MPI_Comm object
    MPI_Comm tempComm = MPI_Comm_f2c(comm);
    status = MPI_Send(&cmd,
                      sizeof(CommandMsg),
                      MPI_UNSIGNED_CHAR,
                      dest,
                      XDMF_DSM_COMMAND_TAG,
                      tempComm);
  }
  if (status != MPI_SUCCESS) {
    XdmfError::message(XdmfError::FATAL, "Error: Failed to send command header");
  }
}

void
XdmfDSMBuffer::SendData(int dest, char * data, int aLength, int tag, int aAddress, int comm)
{
  int status;
  if (comm == XDMF_DSM_INTRA_COMM) {
    status = MPI_Send(data,
                      aLength,
                      MPI_UNSIGNED_CHAR,
                      dest,
                      tag,
                       static_cast<XdmfDSMCommMPI *>(this->Comm)->GetIntraComm());
  }
  else if (comm == XDMF_DSM_INTER_COMM) {
    status = MPI_Send(data,
                      aLength,
                      MPI_UNSIGNED_CHAR,
                      dest,
                      tag,
                      static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm());
  }
  else {
    MPI_Comm tempComm = MPI_Comm_f2c(comm);
    status = MPI_Send(data, aLength, MPI_UNSIGNED_CHAR, dest, tag, tempComm);
  }
  if (status != MPI_SUCCESS) {
    XdmfError::message(XdmfError::FATAL, "Error: Failed to send data");
  }
}

void
XdmfDSMBuffer::SendDone()
{
  if (static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm() == MPI_COMM_NULL)
  {
    for (int i = this->StartServerId; i <= this->EndServerId; ++i) {
      if (i != this->Comm->GetId()){
        this->SendCommandHeader(XDMF_DSM_OPCODE_DONE, i, 0, 0, XDMF_DSM_INTRA_COMM);
      }
    }
  }
  else
  {
    for (int i = this->StartServerId; i <= this->EndServerId; ++i) {
      if (i != this->Comm->GetId()){
        this->SendCommandHeader(XDMF_DSM_OPCODE_DONE, i, 0, 0, XDMF_DSM_INTER_COMM);
      }
    }
  }
}

void
XdmfDSMBuffer::SendInfo()
{
  InfoMsg  dsmInfo;
  int status;

  memset(&dsmInfo, 0, sizeof(InfoMsg));
  dsmInfo.type = this->GetDsmType();
  dsmInfo.length = this->GetLength();
  dsmInfo.total_length = this->GetTotalLength();
  dsmInfo.block_length = this->GetBlockLength();
  dsmInfo.start_server_id = this->GetStartServerId();
  dsmInfo.end_server_id = this->GetEndServerId();

  int infoStatus = 3;
  if (this->Comm->GetId() == 0) {
    infoStatus = 2;
  }

  int * groupInfoStatus = new int[this->Comm->GetInterSize()]();

  MPI_Allgather(&infoStatus,
                1,
                MPI_INT,
                &(groupInfoStatus[0]),
                1,
                MPI_INT,
                this->Comm->GetInterComm());

  int sendCore = 0;

  for (int i = 0; i < this->Comm->GetInterSize(); ++i) {
    if (groupInfoStatus[i] == 2) {
      sendCore = i;
    }
  }

  status = MPI_Bcast(&dsmInfo,
                     sizeof(InfoMsg),
                     MPI_UNSIGNED_CHAR,
                     sendCore,
                     static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm());
  if (status != MPI_SUCCESS) {
    XdmfError::message(XdmfError::FATAL, "Error: Failed to send info");
  }
}

void
XdmfDSMBuffer::SetBlockLength(long newBlock)
{
  this->BlockLength = newBlock;
}

void
XdmfDSMBuffer::SetComm(XdmfDSMCommMPI * newComm)
{
  this->Comm = newComm;
}

void
XdmfDSMBuffer::SetDsmType(int newDsmType)
{
  this->DsmType = newDsmType;
}

void
XdmfDSMBuffer::SetIsConnected(bool newStatus)
{
  IsConnected =  newStatus;
}

void
XdmfDSMBuffer::SetIsServer(bool newIsServer)
{
  this->IsServer = newIsServer;
}

void
XdmfDSMBuffer::SetLength(long aLength)
{
  this->Length = aLength;
  if (this->DataPointer) {
    // Try to reallocate
    // This should not be called in most cases
    this->DataPointer =
      static_cast<char *>(realloc(this->DataPointer, this->Length*sizeof(char)));
  }
  else {
#ifdef _WIN32
    this->DataPointer = calloc(this->Length, sizeof(char));
#else
  #ifdef _SC_PAGESIZE
    posix_memalign((void **)(&this->DataPointer), sysconf(_SC_PAGESIZE), this->Length);
    memset(this->DataPointer, 0, this->Length);
  #else
    // Older linux variation, for backwards compatibility
    posix_memalign((void **)(&this->DataPointer), getpagesize(), this->Length);
    memset(this->DataPointer, 0, this->Length);
  #endif
#endif
  }

  if (this->DataPointer == NULL) {
    std::stringstream message;
    message << "Allocation Failed, unable to allocate " << this->Length;
    XdmfError::message(XdmfError::FATAL, message.str());
  }
}
