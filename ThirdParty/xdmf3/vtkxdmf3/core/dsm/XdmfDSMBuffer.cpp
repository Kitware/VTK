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
  this->DsmType = XDMF_DSM_TYPE_UNIFORM;
  this->IsServer = true;
  this->StartAddress = this->EndAddress = 0;
  this->StartServerId = this->EndServerId = -1;
  this->LocalBufferSizeMBytes = 128;
  this->Length = 0;
  this->TotalLength = 0;
  this->BlockLength = XDMF_DSM_DEFAULT_BLOCK_LENGTH;
  this->NumPages = 0;
  this->PagesAssigned = 0;
  this->Comm = NULL;
  this->DataPointer = NULL;
  this->InterCommType = XDMF_DSM_COMM_MPI;
  this->IsConnected = false;
  this->ResizeFactor = 1;
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
      // Block based allocation should use PageToId
      // All Servers have same length
      // This finds out which server the address provided starts on
      ServerId = this->StartServerId + (Address / this->Length);
      if(ServerId > this->EndServerId ){
        try {
          std::stringstream message;
          message << "ServerId " << ServerId << " for Address "
                  << Address << " is larger than EndServerId "
                  << this->EndServerId;
          XdmfError::message(XdmfError::FATAL, message.str());
        }
        catch (XdmfError & e) {
          throw e;
        }
      }
      break;
    default :
      // Not Implemented
      try {
        std::stringstream message;
        message << "DsmType " << this->DsmType << " not yet implemented or not uniform";
        XdmfError::message(XdmfError::FATAL, message.str());
      }
      catch (XdmfError & e) {
        throw e;
      }
      break;
    }
    return(ServerId);
}

void
XdmfDSMBuffer::BroadcastComm(int *comm, int root)
{
  int status;

  this->Comm->Broadcast(comm,
                        sizeof(int),
                        root,
                        XDMF_DSM_INTRA_COMM);
  if (status != MPI_SUCCESS) {
    try {
      XdmfError(XdmfError::FATAL, "Broadcast of Comm failed");
    }
    catch (XdmfError & e) {
      throw e;
    }
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
      try {
        this->Comm->Probe(&this->CommChannel);
      }
      catch (XdmfError & e) {
        throw e;
      }
    }
    try {
      this->BroadcastComm(&this->CommChannel, 0);
    }
    catch (XdmfError & e) {
      throw e;
    }
  }

  try {
    this->ReceiveCommandHeader(&opcode,
                               &who,
                               &address,
                               &aLength,
                               this->CommChannel,
                               syncId);
  }
  catch (XdmfError & e) {
    throw e;
  }

  // Connection is an ID for client or server,
//  int communicatorId = this->CommChannel;

  switch(opcode) {

  // H5FD_DSM_OPCODE_PUT
  case XDMF_DSM_OPCODE_PUT:
    if (((unsigned int) aLength + address) > this->Length) {
      try {
        std::stringstream message;
        message << "Length " << aLength << " too long for Address " << address 
                << "\n" << "Server Start = " << this->StartAddress << " End = "
                << this->EndAddress;
        XdmfError::message(XdmfError::FATAL, message.str());
      }
      catch (XdmfError & e) {
        throw e;
      }
    }
    if ((datap = this->DataPointer) == NULL) {
       try {
         XdmfError::message(XdmfError::FATAL,
                            "Null Data Pointer when trying to put data");
       }
       catch (XdmfError & e) {
         throw e;
       }
    }
    datap += address;
    try {
      this->ReceiveData(who,
                        datap,
                        aLength,
                        XDMF_DSM_PUT_DATA_TAG,
                        0,
                        this->CommChannel);
    }
    catch (XdmfError & e) {
      throw e;
    }
    break;

  // H5FD_DSM_OPCODE_GET
  case XDMF_DSM_OPCODE_GET:
    if (((unsigned int) aLength + address) > this->Length) {
      try {
        std::stringstream message;
        message << "Length " << aLength << " too long for Address " << address
                << "\n" << "Server Start = " << this->StartAddress << " End = "
                << this->EndAddress;
        XdmfError::message(XdmfError::FATAL, message.str());
      }
      catch (XdmfError & e) {
        throw e;
      }
    }
    if ((datap = this->DataPointer) == NULL) {
      try {
         XdmfError::message(XdmfError::FATAL,
                            "Null Data Pointer when trying to put data");
       }
       catch (XdmfError & e) {
         throw e;
       }
    }
    datap += address;
    try {
      this->SendData(who,
                     datap,
                     aLength,
                     XDMF_DSM_GET_DATA_TAG,
                     0,
                     this->CommChannel);
    }
    catch (XdmfError & e) {
      throw e;
    }
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
  // Comes from client, requests a notifcation when a file is touched.
  // The notification is sent out when clear is called.
  case XDMF_DSM_SET_NOTIFY:
  {
    // Send the notify info to all cores.
    int strlength = 0;
    char * notifystring;
    int waitingCore = 0;
    if (this->Comm->GetId() == 0)
    {
      waitingCore = who;
      this->ReceiveAcknowledgment(who,
                                  strlength,
                                  XDMF_DSM_EXCHANGE_TAG,
                                  this->CommChannel);
      notifystring = new char[strlength+1]();
      this->ReceiveData(who,
                        notifystring,
                        strlength,
                        XDMF_DSM_EXCHANGE_TAG,
                        0,
                        this->CommChannel);
      notifystring[strlength] = 0;
      WaitingMap[std::string(notifystring)].push_back(who);
      // Send XDMF_DSM_SET_NOTIFY to all server cores in order of increasing id
      for (int i = this->GetStartServerId() + 1; // Since this is core 0 sending it
           i <= this->GetEndServerId();
           ++i) {
        if (i != this->Comm->GetInterId())
        {
          this->SendCommandHeader(XDMF_DSM_SET_NOTIFY, i, 0, 0, XDMF_DSM_INTER_COMM);
        }
      }
    }
    // broadcast to the other server cores
    // BCAST strlen
    this->Comm->Broadcast(&strlength,
                          sizeof(int),
                          0,
                          XDMF_DSM_INTRA_COMM);
    // BCAST notifystring
    if  (this->Comm->GetId() != 0)
    {
      notifystring = new char[strlength + 1]();
    }
    this->Comm->Broadcast(&notifystring,
                          strlength,
                          0,
                          XDMF_DSM_INTRA_COMM);
    notifystring[strlength] = 0;
    // BCAST locked core
    this->Comm->Broadcast(&waitingCore,
                          sizeof(int),
                          0,
                          XDMF_DSM_INTRA_COMM);

    if (this->Comm->GetId() != 0)
    {
      WaitingMap[std::string(notifystring)].push_back(waitingCore);
    }

    break;
  }
  // sends out and clears the notifcations that are stored for a specific file.
  case XDMF_DSM_CLEAR_NOTIFY:
  {
    // send a command to other cores to clear this notification
    int strlength = 0;
    char * notifystring;
    int clearCode = 0;
    if (this->Comm->GetId() == 0)
    {
      this->ReceiveAcknowledgment(who,
                                  strlength,
                                  XDMF_DSM_EXCHANGE_TAG,
                                  this->CommChannel);
      notifystring = new char[strlength+1]();
      this->ReceiveData(who,
                        notifystring,
                        strlength,
                        XDMF_DSM_EXCHANGE_TAG,
                        0,
                        this->CommChannel);
      notifystring[strlength] = 0;
      this->ReceiveAcknowledgment(who,
                                  clearCode,
                                  XDMF_DSM_EXCHANGE_TAG,
                                  this->CommChannel);
    }
    // broad cast string to be notified
    if (WaitingMap[std::string(notifystring)].size() > 0)
    {
      // Request the help of the rest of the server
      // Send XDMF_DSM_SET_NOTIFY to all server cores in order of increasing id
      for (int i = this->GetStartServerId() + 1; // Since this is core 0 sending it
           i <= this->GetEndServerId();
           ++i) {
        if (i != this->Comm->GetInterId())
        {
          this->SendCommandHeader(XDMF_DSM_CLEAR_NOTIFY, i, 0, 0, XDMF_DSM_INTER_COMM);
        }
      }

      // BCAST strlen and code
      this->Comm->Broadcast(&strlength,
                            sizeof(int),
                            0,
                            XDMF_DSM_INTRA_COMM);
      this->Comm->Broadcast(&clearCode,
                            sizeof(int),
                            0,
                            XDMF_DSM_INTRA_COMM);
      // BCAST notifystring
      if  (this->Comm->GetId() != 0)
      {
        notifystring = new char[strlength+1]();
      }
      this->Comm->Broadcast(&notifystring,
                            strlength,
                            0,
                            XDMF_DSM_INTRA_COMM);
      notifystring[strlength] = 0;
      // cores notify based on their index, in order to split up the work
      std::vector<unsigned int> notifiedCores = WaitingMap[std::string(notifystring)];
      for (unsigned int i = this->Comm->GetId(); i < notifiedCores.size(); i+=this->Comm->GetIntraSize())
      {
        unsigned int recvCore = notifiedCores[i];
        this->SendAcknowledgment(recvCore,
                                 clearCode,
                                 XDMF_DSM_EXCHANGE_TAG,
                                 this->CommChannel);
      }
      // Then all cores remove the string from the map of notifications
      WaitingMap.erase(std::string(notifystring));
    }
    break;
  }
  case XDMF_DSM_REGISTER_FILE:
  {
    // save file description
    XdmfDSMBuffer::XDMF_file_desc * newfile = new XdmfDSMBuffer::XDMF_file_desc();

    int strlength = 0;

    this->ReceiveAcknowledgment(who,
                                strlength,
                                XDMF_DSM_EXCHANGE_TAG,
                                this->CommChannel);

    newfile->name = new char[strlength + 1];

    this->ReceiveData(who,
                      newfile->name,
                      strlength,
                      XDMF_DSM_EXCHANGE_TAG,
                      0,
                      this->CommChannel);

    newfile->name[strlength] = 0;

    this->ReceiveData(who,
                      (char *)&newfile->start,
                      sizeof(haddr_t),
                      XDMF_DSM_EXCHANGE_TAG,
                      0,
                      this->CommChannel);

    this->ReceiveData(who,
                      (char *)&newfile->end,
                      sizeof(haddr_t),
                      XDMF_DSM_EXCHANGE_TAG,
                      0,
                      this->CommChannel);

    int recvNumPages = 0;

    this->ReceiveAcknowledgment(who,
                                recvNumPages,
                                XDMF_DSM_EXCHANGE_TAG,
                                this->CommChannel);

    newfile->numPages = recvNumPages;

    if (newfile->numPages > 0)
    {
      newfile->pages = new unsigned int[newfile->numPages]();

      this->ReceiveData(who,
                        (char *)newfile->pages,
                        newfile->numPages * sizeof(unsigned int),
                        XDMF_DSM_EXCHANGE_TAG,
                        0,
                        this->CommChannel);
    }
    else
    {
      newfile->pages = NULL;
    }

    // If old description exists, overwrite it.

    FileDefinitions[std::string(newfile->name)] = newfile;

    break;
  }
  case XDMF_DSM_REQUEST_PAGES:
  {
    // set aside pages to a file
    char * requestfile;
    int strlength = 0;

    this->ReceiveAcknowledgment(who,
                                strlength,
                                XDMF_DSM_EXCHANGE_TAG,
                                this->CommChannel);

    requestfile = new char[strlength + 1];

    this->ReceiveData(who,
                      requestfile,
                      strlength,
                      XDMF_DSM_EXCHANGE_TAG,
                      0,
                      this->CommChannel);

    requestfile[strlength] = 0;

    // This file will have its pages appended to.
    XdmfDSMBuffer::XDMF_file_desc * filedesc;

    if (FileDefinitions.count(std::string(requestfile)) > 0)
    {
      filedesc = FileDefinitions[std::string(requestfile)];
    }
    else
    {
      filedesc = new XDMF_file_desc();
      filedesc->start = 0;
      filedesc->end = 0;
      filedesc->numPages = 0;
      filedesc->pages = NULL;
    }

    int datasize = 0;

    // Request size required for the file
    this->ReceiveAcknowledgment(who,
                                datasize,
                                XDMF_DSM_EXCHANGE_TAG,
                                this->CommChannel);

    // TODO Error handling block length must be greater than 0
    // If Block size = 0 then do nothing?
    // Then return blank data?

    int requestedblocks = ((double)datasize) / this->BlockLength;

    // Round up
    if (requestedblocks * this->BlockLength != datasize)
    {
      ++requestedblocks;
    }

    while (requestedblocks + PagesAssigned >= this->NumPages * this->Comm->GetIntraSize())
    {
      // If requested blocks are out of range, resize
      for (int i = this->GetStartServerId() + 1; // Since this is core 0 sending it
           i <= this->GetEndServerId();
           ++i) {
        if (i != this->Comm->GetInterId())
        {
          this->SendCommandHeader(XDMF_DSM_OPCODE_RESIZE, i, 0, 0, XDMF_DSM_INTER_COMM);
        }
      }
      this->SetLength(this->Length + (this->Length * this->ResizeFactor));
    }

    unsigned int * newpagelist = new unsigned int[filedesc->numPages + requestedblocks]();

    unsigned int index = 0;

    for (unsigned int i = 0; i < filedesc->numPages; ++i)
    {
      newpagelist[index] = filedesc->pages[index];
      ++index;
    }

    for (;index < filedesc->numPages + requestedblocks; ++index)
    {
      // The number of pages assigned is incremented after the page is added.
      // The value added is simply an index
      newpagelist[index] = PagesAssigned++;
    }

    filedesc->numPages = filedesc->numPages + requestedblocks;
    unsigned int * oldpointer = filedesc->pages;
    filedesc->pages = newpagelist;

    if (oldpointer != NULL)
    {
      delete oldpointer;
    }

    // Send back new page allocation pointer

    this->SendAcknowledgment(who,
                             filedesc->numPages,
                             XDMF_DSM_EXCHANGE_TAG,
                             this->CommChannel);

    this->SendData(who,
                   (char *)filedesc->pages,
                   filedesc->numPages * sizeof(unsigned int),
                   XDMF_DSM_EXCHANGE_TAG,
                   0,
                   this->CommChannel);

    this->SendData(who,
                   (char*)&filedesc->start,
                   sizeof(haddr_t),
                   XDMF_DSM_EXCHANGE_TAG,
                   0,
                   this->CommChannel);

    filedesc->end = filedesc->start + (filedesc->numPages * this->BlockLength);

    this->SendData(who,
                   (char*)&(filedesc->end),
                   sizeof(haddr_t),
                   XDMF_DSM_EXCHANGE_TAG,
                   0,
                   this->CommChannel);

    // Notify the current size of the buffer
    int currentLength = this->Length;
    this->SendAcknowledgment(who,
                             currentLength,
                             XDMF_DSM_EXCHANGE_TAG,
                             this->CommChannel);

    break;
  }
  case XDMF_DSM_REQUEST_FILE:
  {
    char * requestfile;
    int strlength = 0;

    this->ReceiveAcknowledgment(who,
                                strlength,
                                XDMF_DSM_EXCHANGE_TAG,
                                this->CommChannel);

    requestfile = new char[strlength + 1];

    this->ReceiveData(who,
                      requestfile,
                      strlength,
                      XDMF_DSM_EXCHANGE_TAG,
                      0,
                      this->CommChannel);

    requestfile[strlength] = 0;

    // This file will be returned.
    XdmfDSMBuffer::XDMF_file_desc * filedesc;

    if (FileDefinitions.count(std::string(requestfile)) > 0)
    {
      this->SendAcknowledgment(who,
                               XDMF_DSM_SUCCESS,
                               XDMF_DSM_EXCHANGE_TAG,
                               this->CommChannel);

      filedesc = FileDefinitions[std::string(requestfile)];

      this->SendData(who,
                     (char*)&filedesc->start,
                     sizeof(haddr_t),
                     XDMF_DSM_EXCHANGE_TAG,
                     0,
                     this->CommChannel);

      this->SendData(who,
                     (char*)&filedesc->end,
                     sizeof(haddr_t),
                     XDMF_DSM_EXCHANGE_TAG,
                     0,
                     this->CommChannel);

      int sendNumPages = filedesc->numPages;

      this->SendAcknowledgment(who,
                               sendNumPages,
                               XDMF_DSM_EXCHANGE_TAG,
                               this->CommChannel);

      this->SendData(who,
                     (char *)filedesc->pages,
                     filedesc->numPages * sizeof(unsigned int),
                     XDMF_DSM_EXCHANGE_TAG,
                     0,
                     this->CommChannel);
    }
    else
    {
      this->SendAcknowledgment(who,
                               XDMF_DSM_FAIL,
                               XDMF_DSM_EXCHANGE_TAG,
                               this->CommChannel);
    }

    break;
  }
  case XDMF_DSM_OPCODE_RESIZE:
    this->SetLength(this->Length + (this->Length * this->ResizeFactor));
    break;
  case XDMF_DSM_REQUEST_ACCESS:
  {
    int isLocked = 0;

    char * requestfile;
    int strlength = 0;

    this->ReceiveAcknowledgment(who,
                                strlength,
                                XDMF_DSM_EXCHANGE_TAG,
                                this->CommChannel);

    requestfile = new char[strlength + 1];

    this->ReceiveData(who,
                      requestfile,
                      strlength,
                      XDMF_DSM_EXCHANGE_TAG,
                      0,
                      this->CommChannel);

    requestfile[strlength] = 0;

    // If the requesting core is the one who
    // already locked the file then tell it that there is not lock.
    std::map<std::string, int>::iterator isOwner = FileOwners.find(std::string(requestfile));

    if (LockedMap.count(std::string(requestfile)) > 0)
    {
      if (isOwner->second != who)
      {
        // If the file is locked notify the requesting core and add it to the queue.
        isLocked = 1;
        LockedMap[std::string(requestfile)].push(who);
      }
    }
    else
    {
      LockedMap[std::string(requestfile)] = std::queue<unsigned int>();
      FileOwners[std::string(requestfile)] = who;
    }

    this->SendAcknowledgment(who,
                             isLocked,
                             XDMF_DSM_EXCHANGE_TAG,
                             this->CommChannel);

    break;
  }
  case XDMF_DSM_UNLOCK_FILE:
  {
    char * requestfile;
    int strlength = 0;

    this->ReceiveAcknowledgment(who,
                                strlength,
                                XDMF_DSM_EXCHANGE_TAG,
                                this->CommChannel);

    requestfile = new char[strlength + 1];

    this->ReceiveData(who,
                      requestfile,
                      strlength,
                      XDMF_DSM_EXCHANGE_TAG,
                      0,
                      this->CommChannel);

    requestfile[strlength] = 0;

    // If file isn't locked do nothing
    if (LockedMap.count(std::string(requestfile)) > 0)
    {
      // Remove the queue if there are no more waiting
      if (LockedMap[std::string(requestfile)].size() > 0)
      {
        // Pop the next process waiting off the queue
        unsigned int nextCore = LockedMap[std::string(requestfile)].front();
        LockedMap[std::string(requestfile)].pop();
        FileOwners[std::string(requestfile)] = nextCore;
        this->SendAcknowledgment(nextCore,
                                 1,
                                 XDMF_DSM_EXCHANGE_TAG,
                                 this->CommChannel);
      }
      if(LockedMap[std::string(requestfile)].size() == 0)
      {
        LockedMap.erase(std::string(requestfile));
        FileOwners.erase(std::string(requestfile));
      }
    }

    break;
  }
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
    try {
      std::stringstream message;
      message << "Error: Unknown Opcode " << opcode;
      XdmfError::message(XdmfError::FATAL, message.str());
    }
    catch (XdmfError & e) {
      throw e;
    }
  }

  if (returnOpcode) *returnOpcode = opcode;
  return(XDMF_DSM_SUCCESS);
}

void
XdmfDSMBuffer::BufferServiceLoop(int *returnOpcode)
{
  int op, status = XDMF_DSM_SUCCESS;
  while (status == XDMF_DSM_SUCCESS) {
    try {
      status = this->BufferService(&op);
    }
    catch (XdmfError & e) {
      throw e;
    }
    if (returnOpcode) *returnOpcode = op;
    if (op == XDMF_DSM_OPCODE_DONE) {
      break;
    }
  }
}

void
XdmfDSMBuffer::Create(MPI_Comm newComm, int startId, int endId)
{
  //
  // Create DSM communicator
  //
  switch (this->InterCommType) {
    case XDMF_DSM_COMM_MPI:
      this->Comm = new XdmfDSMCommMPI();
      break;
    default:
      try {
        XdmfError::message(XdmfError::FATAL, "DSM communication type not supported");
      }
      catch (XdmfError & e) {
        throw e;
      }
  }

  this->Comm->DupComm(newComm);
  this->Comm->Init();

  // Uniform Dsm : every node has a buffer the same size. (Addresses are sequential)
  // Block DSM : nodes are set up using paging
  long length = (long) (this->LocalBufferSizeMBytes)*1024LU*1024LU;
  switch (this->DsmType) {
    case XDMF_DSM_TYPE_UNIFORM:
    case XDMF_DSM_TYPE_UNIFORM_RANGE:
      this->ConfigureUniform(this->Comm, length, startId, endId);
      break;
    case XDMF_DSM_TYPE_BLOCK_CYCLIC:
      this->ConfigureUniform(this->Comm, length, startId, endId, this->BlockLength, false);
      break;
    case XDMF_DSM_TYPE_BLOCK_RANDOM:
      this->ConfigureUniform(this->Comm, length, startId, endId, this->BlockLength, true);
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
    try {
      if (aBlockLength) {
        // For optimization we make the DSM length fit to a multiple of block size
        this->SetLength(((long)(aLength / aBlockLength)) * aBlockLength);
        this->NumPages = ((long)(aLength / aBlockLength));
      }
      else {
        this->SetLength(aLength);
      }
    }
    catch (XdmfError & e) {
      throw e;
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
  // Set DSM structure
  std::vector<std::pair<std::string, unsigned int> > newStructure;
  // determine size of application before the server
  if (startId > 0)
  {
    newStructure.push_back(std::pair<std::string, unsigned int>(aComm->GetApplicationName(), startId));
  }
  newStructure.push_back(std::pair<std::string, unsigned int>("Server", (endId + 1) - startId));
  if(aComm->GetInterSize() - (startId +((endId + 1) - startId)) > 0)
  {
    newStructure.push_back(std::pair<std::string, unsigned int>(aComm->GetApplicationName(), aComm->GetInterSize() - (startId +((endId + 1) - startId))));
  }
  aComm->SetDsmProcessStructure(newStructure);
}

void
XdmfDSMBuffer::Connect(bool persist)
{
  int status;

  do {
    try {
      status = this->GetComm()->Connect();
    }
    catch (XdmfError & e) {
      throw e;
    }
    if (status == MPI_SUCCESS) {
      this->SetIsConnected(true);
      try {
        this->ReceiveInfo();
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
XdmfDSMBuffer::Disconnect()
{
  // Disconnecting is done manually
  try {
    this->GetComm()->Disconnect();
  }
  catch (XdmfError & e) {
    throw e;
  }
  this->SetIsConnected(false);
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
      try {
        XdmfError::message(XdmfError::FATAL, "Address Error");
      }
      catch (XdmfError & e) {
        throw e;
      }
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
      try {
        this->SendCommandHeader(XDMF_DSM_OPCODE_GET, who, Address - astart, len, dataComm);
      }
      catch (XdmfError & e) {
        throw e;
      }
      try {
        this->ReceiveData(who, datap, len, XDMF_DSM_GET_DATA_TAG, Address - astart, dataComm);
      }
      catch (XdmfError & e) {
        throw e;
      }
    }
    // Shift all the numbers by the length of the data written
    // Until aLength = 0
    aLength -= len;
    Address += len;
    datap += len;
  }
}

void
XdmfDSMBuffer::Get(unsigned int * pages, unsigned int numPages, long Address, long aLength, void *Data)
{
  char * currentStart;
  unsigned int currentPageId = Address / this->BlockLength;
  unsigned int dsmPage;
  long startingAddress = Address % this->BlockLength;
  unsigned int tranferedLength;
  unsigned int dataPage = 0;

  long pointeroffset = 0;

  int serverCore;
  int writeAddress;

  while (aLength) {
    if (dataPage == 0) {
      tranferedLength = this->BlockLength - startingAddress;
    }
    else {
      tranferedLength = this->BlockLength;
    }
    if (tranferedLength > aLength) {
      tranferedLength = aLength;
    }

    dsmPage = pages[currentPageId];

    currentStart = (char *)Data + pointeroffset;;

    // Write page to DSM
    // page to DSM server Id
    // page to address
    // write to location

    serverCore = PageToId(dsmPage);
    writeAddress = PageToAddress(dsmPage);

    if (serverCore == XDMF_DSM_FAIL) {
      XdmfError::message(XdmfError::FATAL,
                         "Error: Unable to determine server core.");
    }

    if (writeAddress == XDMF_DSM_FAIL) {
      XdmfError::message(XdmfError::FATAL,
                         "Error: Unable to determine write address.");
    }

    if (dataPage == 0)
    {
      writeAddress += startingAddress;
    }

    // If the data is on the core running this code, then the put is simple
    if(serverCore == this->Comm->GetInterId()){
      char *dp;
      dp = this->DataPointer;
      dp += writeAddress;
      memcpy(currentStart, dp, tranferedLength);
    }
    else{
      // Otherwise send it to the appropriate core to deal with
      int dataComm = XDMF_DSM_INTRA_COMM;
      if (this->Comm->GetInterComm() != MPI_COMM_NULL) {
        dataComm = XDMF_DSM_INTER_COMM;
      }
      try {
        this->SendCommandHeader(XDMF_DSM_OPCODE_GET,
                                serverCore,
                                writeAddress,
                                tranferedLength,
                                dataComm);
      }
      catch (XdmfError & e) {
        throw e;
      }
      try {
        this->ReceiveData(serverCore,
                          currentStart,
                          tranferedLength,
                          XDMF_DSM_GET_DATA_TAG,
                          writeAddress,
                          dataComm);
      }
      catch (XdmfError & e) {
        throw e;
      }
    }

    aLength -= tranferedLength;
    pointeroffset += tranferedLength;
    // move to the next page
    ++currentPageId;
    ++dataPage;
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
        try {
          std::stringstream message;
          message << "DsmType " << this->DsmType << " not yet implemented";
          XdmfError::message(XdmfError::FATAL, message.str());
        }
        catch (XdmfError & e) {
          throw e;
        }
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

int
XdmfDSMBuffer::GetInterCommType()
{
  return this->InterCommType;
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
  // This is the length of the pointer on the current core.
  // Different from local buffer size as that value is
  // the starting size.
  return this->Length;
}

unsigned int
XdmfDSMBuffer::GetLocalBufferSizeMBytes()
{
  // This is the starting value, so it is not updated as the pointer is expanded.
  return this->LocalBufferSizeMBytes;
}

double
XdmfDSMBuffer::GetResizeFactor()
{
  return this->ResizeFactor;
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
XdmfDSMBuffer::Lock(char * filename)
{
  int strlength = std::string(filename).size();
  // Request access to the file
  this->SendCommandHeader(XDMF_DSM_REQUEST_ACCESS,
                          this->GetStartServerId(),
                          0,
                          0,
                          XDMF_DSM_INTER_COMM);

  this->SendAcknowledgment(this->GetStartServerId(),
                           strlength,
                           XDMF_DSM_EXCHANGE_TAG,
                           XDMF_DSM_INTER_COMM);

  this->SendData(this->GetStartServerId(),
                 filename,
                 strlength,
                 XDMF_DSM_EXCHANGE_TAG,
                 0,
                 XDMF_DSM_INTER_COMM);

  int isLocked = 0;

  this->ReceiveAcknowledgment(this->GetStartServerId(),
                              isLocked,
                              XDMF_DSM_EXCHANGE_TAG,
                              XDMF_DSM_INTER_COMM);

  if (isLocked == 1)
  {
    // If locked wait for notification that the file is available.
    this->ReceiveAcknowledgment(this->GetStartServerId(),
                            isLocked,
                            XDMF_DSM_EXCHANGE_TAG,
                            XDMF_DSM_INTER_COMM);
  }
}

int
XdmfDSMBuffer::PageToId(int pageId)
{
  int   ServerId = XDMF_DSM_FAIL;

  switch(this->DsmType) {
    case XDMF_DSM_TYPE_BLOCK_CYCLIC :
    case XDMF_DSM_TYPE_BLOCK_RANDOM :
    {
      // Block based allocation should use PageToId
      // All Servers have same length
      // This finds out which server the address provided starts on
      int serversize = (this->EndServerId - this->StartServerId);
      if (serversize < 1)
      {
        serversize = 1;
      }
      ServerId = pageId % serversize;// This should only be called by the server
      ServerId += this->StartServerId; // Apply the offset of the server if required.
      break;
    }
    default :
      // Not Implemented
      try {
        std::stringstream message;
        message << "DsmType " << this->DsmType << " not yet implemented or not paged";
        XdmfError::message(XdmfError::FATAL, message.str());
      }
      catch (XdmfError & e) {
        throw e;
      }
      break;
    }
    return(ServerId);
}

int
XdmfDSMBuffer::PageToAddress(int pageId)
{
  int   resultAddress = XDMF_DSM_FAIL;

  switch(this->DsmType) {
    case XDMF_DSM_TYPE_BLOCK_CYCLIC :
    case XDMF_DSM_TYPE_BLOCK_RANDOM :
    {
      // Block based allocation should use PageToId
      // All Servers have same length
      // This finds out which server the address provided starts on
      // Since this is integers being divided the result is truncated.
      int serversize = (this->EndServerId - this->StartServerId);
      if (serversize < 1)
      {
        serversize = 1;
      }
      resultAddress = this->BlockLength * (pageId / serversize);
      break;
    }
    default :
      // Not Implemented
      try {
        std::stringstream message;
        message << "DsmType " << this->DsmType << " not yet implemented or not paged";
        XdmfError::message(XdmfError::FATAL, message.str());
      }
      catch (XdmfError & e) {
        throw e;
      }
      break;
    }
    return(resultAddress);
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
      try {
        XdmfError::message(XdmfError::FATAL, "Address Error");
      }
      catch (XdmfError & e) {
        throw e;
      }
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
      int   dataComm = XDMF_DSM_INTRA_COMM;
      if (this->Comm->GetInterComm() != MPI_COMM_NULL) {
        dataComm = XDMF_DSM_INTER_COMM;
      }
      try {
        this->SendCommandHeader(XDMF_DSM_OPCODE_PUT,
                                who,
                                Address - astart,
                                len,
                                dataComm);
      }
      catch (XdmfError & e) {
        throw e;
      }
      try {
        this->SendData(who,
                       datap, 
                       len,
                       XDMF_DSM_PUT_DATA_TAG,
                       Address - astart,
                       dataComm);
      }
      catch (XdmfError & e) {
        throw e;
      }
    }
    // Shift all the numbers by the length of the data written
    // Until aLength = 0
    aLength -= len;
    Address += len;
    datap += len;
  }
}

void
XdmfDSMBuffer::Put(unsigned int * pages, unsigned int numPages, haddr_t Address, haddr_t aLength, const void *Data)
{

  char * currentStart;
  unsigned int currentPageId = Address / this->BlockLength;
  unsigned int dsmPage = 0;
  long startingAddress = Address % this->BlockLength;
  unsigned int tranferedLength;

  long pointeroffset = 0;

  unsigned int dataPage = 0;

  int serverCore = 0;
  int writeAddress = 0;

  while (aLength) {
    if (dataPage == 0) {
      tranferedLength = this->BlockLength - startingAddress;
    }
    else {
      tranferedLength = this->BlockLength;
    }
    if (tranferedLength > aLength) {
      tranferedLength = aLength;
    }

    dsmPage = pages[currentPageId];

    currentStart = (char *)Data + pointeroffset;

    // Write page to DSM
    // page to DSM server Id
    // page to address
    // write to location
    serverCore = PageToId(dsmPage);

    writeAddress = PageToAddress(dsmPage);

    if (serverCore == XDMF_DSM_FAIL) {
      XdmfError::message(XdmfError::FATAL, "Error: Unable to determine page server core.");
    }
    if (writeAddress == XDMF_DSM_FAIL) {
      XdmfError::message(XdmfError::FATAL, "Error: Unable to determine page address.");
    }

    if (dataPage == 0)
    {
      writeAddress += startingAddress;
    }

    // If the data is on the core running this code, then the put is simple
    if(serverCore == this->Comm->GetInterId()) {
      char *dp;
      dp = this->DataPointer;
      dp += writeAddress;
      memcpy(dp, currentStart, tranferedLength);
    }
    else{
      // Otherwise send it to the appropriate core to deal with
      int dataComm = XDMF_DSM_INTRA_COMM;
      if (this->Comm->GetInterComm() != MPI_COMM_NULL) {
        dataComm = XDMF_DSM_INTER_COMM;
      }
      try {
        this->SendCommandHeader(XDMF_DSM_OPCODE_PUT,
                                serverCore,
                                writeAddress,
                                tranferedLength,
                                dataComm);
      }
      catch (XdmfError & e) {
        throw e;
      }
      try {
        this->SendData(serverCore,
                       currentStart,
                       tranferedLength,
                       XDMF_DSM_PUT_DATA_TAG,
                       writeAddress,
                       dataComm);
      }
      catch (XdmfError & e) {
        throw e;
      }
    }

    aLength -= tranferedLength;
    pointeroffset += tranferedLength;
    // move to the next page
    ++currentPageId;
    ++dataPage;
  }
}

void
XdmfDSMBuffer::ReceiveAcknowledgment(int source, int &data, int tag, int comm)
{
  int status;
  MPI_Status signalStatus;

  this->Comm->Receive(&data,
                      sizeof(int),
                      source,
                      comm,
                      tag);
  status = MPI_SUCCESS;

  if (status != MPI_SUCCESS) {
    try {
      XdmfError::message(XdmfError::FATAL, "Error: Failed to receive data");
    }
    catch (XdmfError & e) {
      throw e;
    }
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

  this->Comm->Receive(&cmd,
                      sizeof(CommandMsg),
                      remoteSource,
                      comm,
                      XDMF_DSM_COMMAND_TAG);

  status = MPI_SUCCESS;

  if (status != MPI_SUCCESS) {
    try {
      XdmfError::message(XdmfError::FATAL, "Error: Failed to receive command header");
    }
    catch (XdmfError & e) {
      throw e;
    }
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
  this->Comm->Receive(data,
                      aLength,
                      source,
                      comm,
                      tag);
  status = MPI_SUCCESS;
  if (status != MPI_SUCCESS) {
    try {
      XdmfError::message(XdmfError::FATAL, "Error: Failed to receive data");
    }
    catch (XdmfError & e) {
      throw e;
    }
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

  this->Comm->AllGather(&infoStatus,
                        sizeof(int),
                        &(groupInfoStatus[0]),
                        sizeof(int),
                        XDMF_DSM_INTER_COMM);

  int sendCore = 0;

  for (int i = 0; i < this->Comm->GetInterSize(); ++i) {
    if (groupInfoStatus[i] == 2) {
      sendCore = i;
    }
  }

  status = MPI_SUCCESS;

  this->Comm->Broadcast(&dsmInfo,
                        sizeof(InfoMsg),
                        sendCore,
                        XDMF_DSM_INTER_COMM);
  if (status != MPI_SUCCESS) {
    try {
      XdmfError::message(XdmfError::FATAL, "Error: Failed to broadcast info");
    }
    catch (XdmfError & e) {
      throw e;
    }
  }
  this->SetDsmType(dsmInfo.type);
  // We are a client so don't allocate anything but only set a virtual remote length
  this->SetLength(dsmInfo.length);
  this->TotalLength = dsmInfo.total_length;
  this->SetBlockLength(dsmInfo.block_length);
  this->StartServerId = dsmInfo.start_server_id;
  this->EndServerId = dsmInfo.end_server_id;

  MPI_Comm comm = this->Comm->GetInterComm();

  // Cray needs to be launched via the colon notation so that it
  // can properly create a merged communicator

  int rank = this->Comm->GetInterId();
  int size = this->Comm->GetInterSize();

  int currentCore = 0;
  int * checkstatus = new int[size]();
  int localCheck = 0;
  std::string applicationName = this->Comm->GetApplicationName();

  char * coreTag;
  int tagSize = 0;

  std::vector<int> coreSplit;
  unsigned int splitid = 0;

  std::vector<std::pair<std::string, unsigned int> > newStructure;

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
  this->Comm->SetDsmProcessStructure(newStructure);
}

int
XdmfDSMBuffer::RegisterFile(char * name, unsigned int * pages, unsigned int numPages, haddr_t start, haddr_t end)
{
  this->SendCommandHeader(XDMF_DSM_REGISTER_FILE,
                          this->GetStartServerId(),
                          0,
                          0,
                          XDMF_DSM_INTER_COMM);

  int strlength = std::string(name).size();

  this->SendAcknowledgment(this->GetStartServerId(),
                           strlength,
                           XDMF_DSM_EXCHANGE_TAG,
                           XDMF_DSM_INTER_COMM);

  this->SendData(this->GetStartServerId(),
                 name,
                 strlength,
                 XDMF_DSM_EXCHANGE_TAG,
                 0,
                 XDMF_DSM_INTER_COMM);

  this->SendData(this->GetStartServerId(),
                 (char *)&start,
                 sizeof(haddr_t),
                 XDMF_DSM_EXCHANGE_TAG,
                 0,
                 XDMF_DSM_INTER_COMM);

  this->SendData(this->GetStartServerId(),
                 (char *)&end,
                 sizeof(haddr_t),
                 XDMF_DSM_EXCHANGE_TAG,
                 0,
                 XDMF_DSM_INTER_COMM);

  this->SendAcknowledgment(this->GetStartServerId(),
                           numPages,
                           XDMF_DSM_EXCHANGE_TAG,
                           XDMF_DSM_INTER_COMM);

  if (numPages > 0)
  {
    this->SendData(this->GetStartServerId(),
                   (char *)pages,
                   numPages * sizeof(unsigned int),
                   XDMF_DSM_EXCHANGE_TAG,
                   0,
                   XDMF_DSM_INTER_COMM);
  }

  return XDMF_DSM_SUCCESS;
}

int
XdmfDSMBuffer::RequestFileDescription(char * name, std::vector<unsigned int> & pages, unsigned int & numPages, haddr_t & start, haddr_t & end)
{
  this->SendCommandHeader(XDMF_DSM_REQUEST_FILE,
                          this->GetStartServerId(),
                          0,
                          0,
                          XDMF_DSM_INTER_COMM);

  int strlength = std::string(name).size();

  this->SendAcknowledgment(this->GetStartServerId(),
                           strlength,
                           XDMF_DSM_EXCHANGE_TAG,
                           XDMF_DSM_INTER_COMM);

  this->SendData(this->GetStartServerId(),
                 name,
                 strlength,
                 XDMF_DSM_EXCHANGE_TAG,
                 0,
                 XDMF_DSM_INTER_COMM);

  int fileExists = XDMF_DSM_SUCCESS;

  this->ReceiveAcknowledgment(this->GetStartServerId(),
                              fileExists,
                              XDMF_DSM_EXCHANGE_TAG,
                              XDMF_DSM_INTER_COMM);

  if (fileExists == XDMF_DSM_SUCCESS)
  {
    this->ReceiveData(this->GetStartServerId(),
                      (char*)&start,
                      sizeof(haddr_t),
                      XDMF_DSM_EXCHANGE_TAG,
                      0,
                      XDMF_DSM_INTER_COMM);

    this->ReceiveData(this->GetStartServerId(),
                      (char*)&end,
                      sizeof(haddr_t),
                      XDMF_DSM_EXCHANGE_TAG,
                      0,
                      XDMF_DSM_INTER_COMM);

    int recvNumPages = 0;

    this->ReceiveAcknowledgment(this->GetStartServerId(),
                                recvNumPages,
                                XDMF_DSM_EXCHANGE_TAG,
                                XDMF_DSM_INTER_COMM);

    numPages = recvNumPages;

    // Reallocate pointer
    pages.clear();

    unsigned int * pagelist = new unsigned int[numPages];

    this->ReceiveData(this->GetStartServerId(),
                      (char *)pagelist,
                      numPages * sizeof(unsigned int),
                      XDMF_DSM_EXCHANGE_TAG,
                      0,
                      XDMF_DSM_INTER_COMM);

    for (unsigned int i = 0; i < numPages; ++i)
    {
      pages.push_back(pagelist[i]);
    }

    return XDMF_DSM_SUCCESS;
  }
  else
  {
    return XDMF_DSM_FAIL;
  }
}


void
XdmfDSMBuffer::RequestPages(char * name,
                            haddr_t spaceRequired,
                            std::vector<unsigned int> & pages,
                            unsigned int & numPages,
                            haddr_t & start,
                            haddr_t & end)
{
  this->SendCommandHeader(XDMF_DSM_REQUEST_PAGES,
                          this->GetStartServerId(),
                          0,
                          0,
                          XDMF_DSM_INTER_COMM);

  // set aside pages to a file
  int strlength = std::string(name).size();

  this->SendAcknowledgment(this->GetStartServerId(),
                           strlength,
                           XDMF_DSM_EXCHANGE_TAG,
                           XDMF_DSM_INTER_COMM);

  this->SendData(this->GetStartServerId(),
                 name,
                 strlength,
                 XDMF_DSM_EXCHANGE_TAG,
                 0,
                 XDMF_DSM_INTER_COMM);

  // Request size required for the file
  this->SendAcknowledgment(this->GetStartServerId(),
                           spaceRequired,
                           XDMF_DSM_EXCHANGE_TAG,
                           XDMF_DSM_INTER_COMM);

  // Send back new page allocation pointer

  int newPageCount = 0;

  this->ReceiveAcknowledgment(this->GetStartServerId(),
                              newPageCount,
                              XDMF_DSM_EXCHANGE_TAG,
                              XDMF_DSM_INTER_COMM);

  numPages = newPageCount;

  unsigned int * pagelist = new unsigned int[numPages]();
  pages.clear();

  this->ReceiveData(this->GetStartServerId(),
                    (char *) pagelist,
                    numPages * sizeof(unsigned int),
                    XDMF_DSM_EXCHANGE_TAG,
                    0,
                    XDMF_DSM_INTER_COMM);

  for (unsigned int i = 0; i < numPages; ++i)
  {
    pages.push_back(pagelist[i]);
  }

  // Recieve the new start and end addresses
  this->ReceiveData(this->GetStartServerId(),
                    (char*)&start,
                    sizeof(haddr_t),
                    XDMF_DSM_EXCHANGE_TAG,
                    0,
                    XDMF_DSM_INTER_COMM);

  this->ReceiveData(this->GetStartServerId(),
                    (char*)&end,
                    sizeof(haddr_t),
                    XDMF_DSM_EXCHANGE_TAG,
                    0,
                    XDMF_DSM_INTER_COMM);

  // If resized, set up the reset the total length.
  int currentLength = 0;
  this->ReceiveAcknowledgment(this->GetStartServerId(),
                              currentLength,
                              XDMF_DSM_EXCHANGE_TAG,
                              XDMF_DSM_INTER_COMM);

  this->UpdateLength(currentLength);
}

void
XdmfDSMBuffer::SendAccept(unsigned int numConnections)
{
#ifndef XDMF_DSM_IS_CRAY
  for (int i = this->StartServerId; i <= this->EndServerId; ++i) {
    if (i != this->Comm->GetInterId()){
      this->SendCommandHeader(XDMF_DSM_ACCEPT, i, 0, 0, XDMF_DSM_INTER_COMM);
      this->SendAcknowledgment(i, numConnections, XDMF_DSM_EXCHANGE_TAG, XDMF_DSM_INTER_COMM);
    }
  }
  this->Comm->Accept(numConnections);
  this->SendInfo();
#endif
}

void
XdmfDSMBuffer::SendAcknowledgment(int dest, int data, int tag, int comm)
{
  int status;
  this->Comm->Send(&data,
                   sizeof(int),
                   dest,
                   comm,
                   tag);
  status = MPI_SUCCESS;
  if (status != MPI_SUCCESS) {
    try {
      XdmfError::message(XdmfError::FATAL, "Error: Failed to receive data");
    }
    catch (XdmfError & e) {
      throw e;
    }
  }
}

void
XdmfDSMBuffer::SendCommandHeader(int opcode, int dest, int address, int aLength, int comm)
{
  int status;
  CommandMsg cmd;
  memset(&cmd, 0, sizeof(CommandMsg));
  cmd.Opcode = opcode;
  if (comm == XDMF_DSM_INTRA_COMM) {
    cmd.Source = this->Comm->GetId();
  }
  else if (comm == XDMF_DSM_INTER_COMM) {
    cmd.Source = this->Comm->GetInterId();
  }
  cmd.Target = dest;
  cmd.Address = address;
  cmd.Length = aLength;

  this->Comm->Send(&cmd,
                   sizeof(CommandMsg),
                   dest,
                   comm,
                   XDMF_DSM_COMMAND_TAG);
  status = MPI_SUCCESS;
  if (status != MPI_SUCCESS) {
    try {
      XdmfError::message(XdmfError::FATAL, "Error: Failed to send command header");
    }
    catch (XdmfError & e) {
      throw e;
    }
  }
}

void
XdmfDSMBuffer::SendData(int dest, char * data, int aLength, int tag, int aAddress, int comm)
{
  int status;

  this->Comm->Send(data,
                   aLength,
                   dest,
                   comm,
                   tag);
  status = MPI_SUCCESS;
  if (status != MPI_SUCCESS) {
    try {
      XdmfError::message(XdmfError::FATAL, "Error: Failed to send data");
    }
    catch (XdmfError & e) {
      throw e;
    }
  }
}

void
XdmfDSMBuffer::SendDone()
{
  try {
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
  catch (XdmfError & e) {
    throw e;
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

  this->Comm->AllGather(&infoStatus,
                        sizeof(int),
                        &(groupInfoStatus[0]),
                        sizeof(int),
                        XDMF_DSM_INTER_COMM);

  int sendCore = 0;

  for (int i = 0; i < this->Comm->GetInterSize(); ++i) {
    if (groupInfoStatus[i] == 2) {
      sendCore = i;
    }
  }

  status = MPI_SUCCESS;

  this->Comm->Broadcast(&dsmInfo,
                        sizeof(InfoMsg),
                        sendCore,
                        XDMF_DSM_INTER_COMM);
  if (status != MPI_SUCCESS) {
    try {
      XdmfError::message(XdmfError::FATAL, "Error: Failed to send info");
    }
    catch (XdmfError & e) {
      throw e;
    }
  }

  MPI_Comm comm = this->Comm->GetInterComm();

  // Cray needs to be launched via the colon notation so that it
  // can properly create a merged communicator

  int rank = this->Comm->GetInterId();
  int size = this->Comm->GetInterSize();

  int currentCore = 0;
  int * checkstatus = new int[size]();
  int localCheck = 0;
  std::string applicationName = this->Comm->GetApplicationName();

  char * coreTag;
  int tagSize = 0;

  std::vector<int> coreSplit;
  unsigned int splitid = 0;

  std::vector<std::pair<std::string, unsigned int> > newStructure;

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
  this->Comm->SetDsmProcessStructure(newStructure);
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
XdmfDSMBuffer::SetInterCommType(int newType)
{
  this->InterCommType = newType;
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
XdmfDSMBuffer::SetResizeFactor(double newFactor)
{
  if (newFactor >= 0)
  {
    this->ResizeFactor = newFactor;
  }
  else
  {
    this->ResizeFactor = newFactor * -1;
  }
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

  if (this->BlockLength > 0)
  {
    this->NumPages = this->Length / this->BlockLength;
  }
}

void
XdmfDSMBuffer::SetLocalBufferSizeMBytes(unsigned int newSize)
{
  this->LocalBufferSizeMBytes = newSize;
}

void
XdmfDSMBuffer::Unlock(char * filename)
{
  int strlength = std::string(filename).size();
  this->SendCommandHeader(XDMF_DSM_UNLOCK_FILE,
                          this->GetStartServerId(),
                          0,
                          0,
                          XDMF_DSM_INTER_COMM);

  this->SendAcknowledgment(this->GetStartServerId(),
                           strlength,
                           XDMF_DSM_EXCHANGE_TAG,
                           XDMF_DSM_INTER_COMM);

  this->SendData(this->GetStartServerId(),
                 filename,
                 strlength,
                 XDMF_DSM_EXCHANGE_TAG,
                 0,
                 XDMF_DSM_INTER_COMM);
}

void
XdmfDSMBuffer::UpdateLength(unsigned int newLength)
{
  this->Length = newLength;
  this->TotalLength = this->Length * (this->EndServerId - this->StartServerId);
}

void
XdmfDSMBuffer::WaitRelease(std::string filename, std::string datasetname, int code)
{
  // Send Command Header
  this->SendCommandHeader(XDMF_DSM_CLEAR_NOTIFY,
                          this->GetStartServerId(),
                          0,
                          0,
                          XDMF_DSM_INTER_COMM);
  // Send string size
  this->SendAcknowledgment(this->GetStartServerId(),
                           filename.size() + datasetname.size(),
                           XDMF_DSM_EXCHANGE_TAG,
                           XDMF_DSM_INTER_COMM);
  // Send string
  char * sendPointer = new char[filename.size() + datasetname.size()]();
  unsigned int placementIndex = 0;
  for (unsigned int i = 0; i < filename.size(); ++i)
  {
    sendPointer[placementIndex++] = filename[i];
  }
  for (unsigned int i = 0; i < datasetname.size(); ++i)
  {
    sendPointer[placementIndex++] = datasetname[i];
  }
  this->SendData(this->GetStartServerId(),
                 sendPointer,
                 filename.size() + datasetname.size(),
                 XDMF_DSM_EXCHANGE_TAG,
                 0,
                 XDMF_DSM_INTER_COMM);
  delete sendPointer;
  // Send Release Code
  this->SendAcknowledgment(this->GetStartServerId(),
                           code,
                           XDMF_DSM_EXCHANGE_TAG,
                           XDMF_DSM_INTER_COMM);
}

int
XdmfDSMBuffer::WaitOn(std::string filename, std::string datasetname)
{
  // Send Command Header
  this->SendCommandHeader(XDMF_DSM_SET_NOTIFY,
                          this->GetStartServerId(),
                          0,
                          0,
                          XDMF_DSM_INTER_COMM);
  // Send string size
  this->SendAcknowledgment(this->GetStartServerId(),
                           filename.size() + datasetname.size(),
                           XDMF_DSM_EXCHANGE_TAG,
                           XDMF_DSM_INTER_COMM);
  // Send string
  char * sendPointer = new char[filename.size() + datasetname.size()]();
  unsigned int placementIndex = 0;
  for (unsigned int i = 0; i < filename.size(); ++i)
  {
    sendPointer[placementIndex++] = filename[i];
  }
  for (unsigned int i = 0; i < datasetname.size(); ++i)
  {
    sendPointer[placementIndex++] = datasetname[i];
  }  
  this->SendData(this->GetStartServerId(),
                 sendPointer,
                 filename.size() + datasetname.size(),
                 XDMF_DSM_EXCHANGE_TAG,
                 0,
                 XDMF_DSM_INTER_COMM);

  // Wait for Release
  int code = 0;
  this->ReceiveAcknowledgment(MPI_ANY_SOURCE,
                              code,
                              XDMF_DSM_EXCHANGE_TAG,
                              this->CommChannel);
  delete sendPointer;
  // Return Code from Notification
  return code;
}

// C Wrappers

XDMFDSMBUFFER * XdmfDSMBufferNew()
{
  try
  {
    return (XDMFDSMBUFFER *)((void *)(new XdmfDSMBuffer()));
  }
  catch (...)
  {
    return (XDMFDSMBUFFER *)((void *)(new XdmfDSMBuffer()));
  }
}

void XdmfDSMBufferFree(XDMFDSMBUFFER * item)
{
  if (item != NULL) {
    delete ((XdmfDSMBuffer *)item);
  }
  item = NULL;
}

void XdmfDSMBufferBroadcastComm(XDMFDSMBUFFER * buffer, int *comm, int root, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMBuffer *)buffer)->BroadcastComm(comm, root);
  XDMF_ERROR_WRAP_END(status)
}

int XdmfDSMBufferBufferService(XDMFDSMBUFFER * buffer, int *returnOpcode, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  return ((XdmfDSMBuffer *)buffer)->BufferService(returnOpcode);
  XDMF_ERROR_WRAP_END(status)
  return -1;
}

void XdmfDSMBufferBufferServiceLoop(XDMFDSMBUFFER * buffer, int *returnOpcode, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMBuffer *)buffer)->BufferServiceLoop(returnOpcode);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMBufferConfigureUniform(XDMFDSMBUFFER * buffer,
                                   XDMFDSMCOMMMPI * Comm,
                                   long Length,
                                   int StartId,
                                   int EndId,
                                   long aBlockLength,
                                   int random,
                                   int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMBuffer *)buffer)->ConfigureUniform((XdmfDSMCommMPI *)Comm, Length, StartId, EndId, aBlockLength, random);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMBufferConnect(XDMFDSMBUFFER * buffer, int persist, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMBuffer *)buffer)->Connect(persist);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMBufferCreate(XDMFDSMBUFFER * buffer, MPI_Comm comm, int startId, int endId, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMBuffer *)buffer)->Create(comm, startId, endId);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMBufferDisconnect(XDMFDSMBUFFER * buffer, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMBuffer *)buffer)->Disconnect();
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMBufferGet(XDMFDSMBUFFER * buffer, long Address, long aLength, void * Data, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMBuffer *)buffer)->Get(Address, aLength, Data);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMBufferGetAddressRangeForId(XDMFDSMBUFFER * buffer, int Id, int * Start, int * End, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMBuffer *)buffer)->GetAddressRangeForId(Id, Start, End);
  XDMF_ERROR_WRAP_END(status)
}

long XdmfDSMBufferGetBlockLength(XDMFDSMBUFFER * buffer)
{
  try
  {
    return ((XdmfDSMBuffer *)buffer)->GetBlockLength();
  }
  catch (...)
  {
    return ((XdmfDSMBuffer *)buffer)->GetBlockLength();
  }
}

XDMFDSMCOMMMPI * XdmfDSMBufferGetComm(XDMFDSMBUFFER * buffer)
{
  try
  {
    return (XDMFDSMCOMMMPI *)((void *)(((XdmfDSMBuffer *)buffer)->GetComm()));
  }
  catch (...)
  {
    return (XDMFDSMCOMMMPI *)((void *)(((XdmfDSMBuffer *)buffer)->GetComm()));
  }
}

char * XdmfDSMBufferGetDataPointer(XDMFDSMBUFFER * buffer)
{
  try
  {
    return ((XdmfDSMBuffer *)buffer)->GetDataPointer();
  }
  catch (...)
  {
    return ((XdmfDSMBuffer *)buffer)->GetDataPointer();
  }
}

int XdmfDSMBufferGetDsmType(XDMFDSMBUFFER * buffer)
{
  try
  {
    return ((XdmfDSMBuffer *)buffer)->GetDsmType();
  }
  catch (...)
  {
    return ((XdmfDSMBuffer *)buffer)->GetDsmType();
  }
}

int XdmfDSMBufferGetEndAddress(XDMFDSMBUFFER * buffer)
{
  try
  {
    return ((XdmfDSMBuffer *)buffer)->GetEndAddress();
  }
  catch (...)
  {
    return ((XdmfDSMBuffer *)buffer)->GetEndAddress();
  }
}

int XdmfDSMBufferGetEndServerId(XDMFDSMBUFFER * buffer)
{
  try
  {
    return ((XdmfDSMBuffer *)buffer)->GetEndServerId();
  }
  catch (...)
  {
    return ((XdmfDSMBuffer *)buffer)->GetEndServerId();
  }
}

int XdmfDSMBufferGetInterCommType(XDMFDSMBUFFER * buffer)
{
  try
  {
    return ((XdmfDSMBuffer *)buffer)->GetInterCommType();
  }
  catch (...)
  {
    return ((XdmfDSMBuffer *)buffer)->GetInterCommType();
  }
}

int XdmfDSMBufferGetIsConnected(XDMFDSMBUFFER * buffer)
{
  try
  {
    return ((XdmfDSMBuffer *)buffer)->GetIsConnected();
  }
  catch (...)
  {
    return ((XdmfDSMBuffer *)buffer)->GetIsConnected();
  }
}

int XdmfDSMBufferGetIsServer(XDMFDSMBUFFER * buffer)
{
  try
  {
    return ((XdmfDSMBuffer *)buffer)->GetIsServer();
  }
  catch (...)
  {
    return ((XdmfDSMBuffer *)buffer)->GetIsServer();
  }
}

long XdmfDSMBufferGetLength(XDMFDSMBUFFER * buffer)
{
  try
  {
    return ((XdmfDSMBuffer *)buffer)->GetLength();
  }
  catch (...)
  {
    return ((XdmfDSMBuffer *)buffer)->GetLength();
  }
}

unsigned int XdmfDSMBufferGetLocalBufferSizeMBytes(XDMFDSMBUFFER * buffer)
{
  try
  {
    return ((XdmfDSMBuffer *)buffer)->GetLocalBufferSizeMBytes();
  }
  catch (...)
  {
    return ((XdmfDSMBuffer *)buffer)->GetLocalBufferSizeMBytes();
  }
}

double XdmfDSMBufferGetResizeFactor(XDMFDSMBUFFER * buffer)
{
  try
  {
    return ((XdmfDSMBuffer *)buffer)->GetResizeFactor();
  }
  catch (...)
  {
    return ((XdmfDSMBuffer *)buffer)->GetResizeFactor();
  }
}

int XdmfDSMBufferGetStartAddress(XDMFDSMBUFFER * buffer)
{
  try
  {
    return ((XdmfDSMBuffer *)buffer)->GetStartAddress();
  }
  catch (...)
  {
    return ((XdmfDSMBuffer *)buffer)->GetStartAddress();
  }
}

int XdmfDSMBufferGetStartServerId(XDMFDSMBUFFER * buffer)
{
  try
  {
    return ((XdmfDSMBuffer *)buffer)->GetStartServerId();
  }
  catch (...)
  {
    return ((XdmfDSMBuffer *)buffer)->GetStartServerId();
  }
}

long XdmfDSMBufferGetTotalLength(XDMFDSMBUFFER * buffer)
{
  try
  {
    return ((XdmfDSMBuffer *)buffer)->GetTotalLength();
  }
  catch (...)
  {
    return ((XdmfDSMBuffer *)buffer)->GetTotalLength();
  }
}

void XdmfDSMBufferProbeCommandHeader(XDMFDSMBUFFER * buffer, int * comm, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMBuffer *)buffer)->ProbeCommandHeader(comm);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMBufferPut(XDMFDSMBUFFER * buffer, long Address, long aLength, void * Data, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMBuffer *)buffer)->Put(Address, aLength, Data);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMBufferReceiveAcknowledgment(XDMFDSMBUFFER * buffer,
                                        int source,
                                        int * data,
                                        int tag,
                                        int comm,
                                        int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMBuffer *)buffer)->ReceiveAcknowledgment(source, *data, tag, comm);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMBufferReceiveCommandHeader(XDMFDSMBUFFER * buffer,
                                       int * opcode,
                                       int * source,
                                       int * address,
                                       int * aLength,
                                       int comm,
                                       int remoteSource,
                                       int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMBuffer *)buffer)->ReceiveCommandHeader(opcode, source, address, aLength, comm, remoteSource);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMBufferReceiveData(XDMFDSMBUFFER * buffer,
                              int source,
                              char * data,
                              int aLength,
                              int tag,
                              int aAddress,
                              int comm,
                              int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMBuffer *)buffer)->ReceiveData(source, data, aLength, tag, aAddress, comm);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMBufferReceiveInfo(XDMFDSMBUFFER * buffer, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMBuffer *)buffer)->ReceiveInfo();
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMBufferSendAccept(XDMFDSMBUFFER * buffer, unsigned int numConnects)
{
  ((XdmfDSMBuffer *)buffer)->SendAccept(numConnects);
}

void XdmfDSMBufferSendAcknowledgment(XDMFDSMBUFFER * buffer,
                                     int dest,
                                     int data,
                                     int tag,
                                     int comm,
                                     int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMBuffer *)buffer)->SendAcknowledgment(dest, data, tag, comm);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMBufferSendCommandHeader(XDMFDSMBUFFER * buffer,
                                    int opcode,
                                    int dest,
                                    int address,
                                    int aLength,
                                    int comm,
                                    int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMBuffer *)buffer)->SendCommandHeader(opcode, dest, address, aLength, comm);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMBufferSendData(XDMFDSMBUFFER * buffer,
                           int dest,
                           char * data,
                           int aLength,
                           int tag,
                           int aAddress,
                           int comm,
                           int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMBuffer *)buffer)->SendData(dest, data, aLength, tag, aAddress, comm);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMBufferSendDone(XDMFDSMBUFFER * buffer, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMBuffer *)buffer)->SendDone();
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMBufferSendInfo(XDMFDSMBUFFER * buffer, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfDSMBuffer *)buffer)->SendInfo();
  XDMF_ERROR_WRAP_END(status)
}

void XdmfDSMBufferSetBlockLength(XDMFDSMBUFFER * buffer, long newBlock)
{
  try
  {
    ((XdmfDSMBuffer *)buffer)->SetBlockLength(newBlock);
  }
  catch (...)
  {
    ((XdmfDSMBuffer *)buffer)->SetBlockLength(newBlock);
  }
}

void XdmfDSMBufferSetComm(XDMFDSMBUFFER * buffer, XDMFDSMCOMMMPI * newComm)
{
  try
  {
    ((XdmfDSMBuffer *)buffer)->SetComm((XdmfDSMCommMPI *)newComm);
  }
  catch (...)
  {
    ((XdmfDSMBuffer *)buffer)->SetComm((XdmfDSMCommMPI *)newComm);
  }
}

void XdmfDSMBufferSetDsmType(XDMFDSMBUFFER * buffer, int newDsmType)
{
  try
  {
    ((XdmfDSMBuffer *)buffer)->SetDsmType(newDsmType);
  }
  catch (...)
  {
    ((XdmfDSMBuffer *)buffer)->SetDsmType(newDsmType);
  }
}

void XdmfDSMBufferSetInterCommType(XDMFDSMBUFFER * buffer, int newType)
{
  try
  {
    ((XdmfDSMBuffer *)buffer)->SetInterCommType(newType);
  }
  catch (...)
  {
    ((XdmfDSMBuffer *)buffer)->SetInterCommType(newType);
  }
}

void XdmfDSMBufferSetIsConnected(XDMFDSMBUFFER * buffer, int newStatus)
{
  try
  {
    ((XdmfDSMBuffer *)buffer)->SetIsConnected(newStatus);
  }
  catch (...)
  {
    ((XdmfDSMBuffer *)buffer)->SetIsConnected(newStatus);
  }
}

void XdmfDSMBufferSetIsServer(XDMFDSMBUFFER * buffer, int newIsServer)
{
  try
  {
    ((XdmfDSMBuffer *)buffer)->SetIsServer(newIsServer);
  }
  catch (...)
  {
    ((XdmfDSMBuffer *)buffer)->SetIsServer(newIsServer);
  }
}

void XdmfDSMBufferSetLocalBufferSizeMBytes(XDMFDSMBUFFER * buffer, unsigned int newSize)
{
  try
  {
    ((XdmfDSMBuffer *)buffer)->SetLocalBufferSizeMBytes(newSize);
  }
  catch (...)
  {
    ((XdmfDSMBuffer *)buffer)->SetLocalBufferSizeMBytes(newSize);
  }
}

void XdmfDSMBufferSetResizeFactor(XDMFDSMBUFFER * buffer, double newFactor)
{
  try
  {
    ((XdmfDSMBuffer *)buffer)->SetResizeFactor(newFactor);
  }
  catch (...)
  {
    ((XdmfDSMBuffer *)buffer)->SetResizeFactor(newFactor);
  }
}

void XdmfDSMBufferWaitRelease(XDMFDSMBUFFER * buffer, char * filename, char * datasetname, int code)
{
  try
  {
    ((XdmfDSMBuffer *)buffer)->WaitRelease(std::string(filename), std::string(datasetname), code);
  }
  catch (...)
  {
    ((XdmfDSMBuffer *)buffer)->WaitRelease(std::string(filename), std::string(datasetname), code);
  }
}

int XdmfDSMBufferWaitOn(XDMFDSMBUFFER * buffer, char * filename, char * datasetname)
{
  try
  {
    return ((XdmfDSMBuffer *)buffer)->WaitOn(std::string(filename), std::string(datasetname));
  }
  catch (...)
  {
    return ((XdmfDSMBuffer *)buffer)->WaitOn(std::string(filename), std::string(datasetname));
  }
}
