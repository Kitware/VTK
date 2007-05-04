/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPICommunicator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMPICommunicator.h"

#include "vtkImageData.h"
#include "vtkMPIController.h"
#include "vtkMPIGroup.h"
#include "vtkMPIGroup.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkToolkits.h"

#include "vtkMPI.h"

vtkCxxRevisionMacro(vtkMPICommunicator, "1.42");
vtkStandardNewMacro(vtkMPICommunicator);

vtkCxxSetObjectMacro(vtkMPICommunicator,Group,vtkMPIGroup);

vtkMPICommunicator* vtkMPICommunicator::WorldCommunicator = 0;

class vtkMPICommunicatorOpaqueRequest
{
public:
  MPI_Request Handle;
};

vtkMPICommunicatorOpaqueComm::vtkMPICommunicatorOpaqueComm()
{
  this->Handle = 0;
}

MPI_Comm* vtkMPICommunicatorOpaqueComm::GetHandle()
{
  return this->Handle;
}

//----------------------------------------------------------------------------
// I wish I could think of a better way to convert a VTK type enum to an MPI
// type enum.
inline MPI_Datatype vtkMPICommunicatorGetMPIType(int vtkType)
{
  // Make sure MPI_LONG_LONG and MPI_UNSIGNED_LONG_LONG are defined, if at all
  // possible.
#ifndef MPI_LONG_LONG
#ifdef MPI_LONG_LONG_INT
  // lampi only has MPI_LONG_LONG_INT, not MPI_LONG_LONG
#define MPI_LONG_LONG MPI_LONG_LONG_INT
#endif
#endif

#ifndef MPI_UNSIGNED_LONG_LONG
#ifdef MPI_UNSIGNED_LONG_LONG_INT
#define MPI_UNSIGNED_LONG_LONG MPI_UNSIGNED_LONG_LONG_INT
#elif defined(MPI_LONG_LONG)
  // mpich does not have an unsigned long long.  Just using signed should
  // be OK.  Everything uses 2's complement nowadays, right?
#define MPI_UNSIGNED_LONG_LONG MPI_LONG_LONG
#endif
#endif

  switch (vtkType)
    {
    case VTK_CHAR:              return MPI_CHAR;
#ifdef MPI_SIGNED_CHAR
    case VTK_SIGNED_CHAR:       return MPI_SIGNED_CHAR;
#else
    case VTK_SIGNED_CHAR:       return MPI_CHAR;
#endif
    case VTK_UNSIGNED_CHAR:     return MPI_UNSIGNED_CHAR;
    case VTK_SHORT:             return MPI_SHORT;
    case VTK_UNSIGNED_SHORT:    return MPI_UNSIGNED_SHORT;
    case VTK_INT:               return MPI_INT;
    case VTK_UNSIGNED_INT:      return MPI_UNSIGNED;
    case VTK_LONG:              return MPI_LONG;
    case VTK_UNSIGNED_LONG:     return MPI_UNSIGNED_LONG;
    case VTK_FLOAT:             return MPI_FLOAT;
    case VTK_DOUBLE:            return MPI_DOUBLE;

#ifdef VTK_USE_64BIT_IDS
#if VTK_SIZEOF_LONG == 8
    case VTK_ID_TYPE:           return MPI_LONG;
#elif defined(MPI_LONG_LONG)
    case VTK_ID_TYPE:           return MPI_LONG_LONG;
#else
    case VTK_ID_TYPE:
      vtkGenericWarningMacro("This systems MPI doesn't seem to support 64 bit ids and you have 64 bit IDs turned on. Please contact VTK mailing list.");
      return MPI_LONG;
#endif
#else //VTK_USE_64BIT_IDS
    case VTK_ID_TYPE:           return MPI_INT;
#endif //VTK_USE_64BIT_IDS

#ifdef MPI_LONG_LONG
    case VTK_LONG_LONG:         return MPI_LONG_LONG;
    case VTK_UNSIGNED_LONG_LONG:return MPI_UNSIGNED_LONG_LONG;
#endif

#if VTK_SIZEOF_LONG == 8
    case VTK___INT64:           return MPI_LONG;
    case VTK_UNSIGNED___INT64:  return MPI_UNSIGNED_LONG;
#elif defined(MPI_LONG_LONG)
    case VTK___INT64:           return MPI_LONG_LONG;
    case VTK_UNSIGNED___INT64:  return MPI_UNSIGNED_LONG_LONG;
#endif

    default:
      vtkGenericWarningMacro("Could not find a supported MPI type for VTK type " << vtkType);
      return MPI_BYTE;
    }
}

//----------------------------------------------------------------------------
template <class T>
int vtkMPICommunicatorSendData(const T* data, int length, int sizeoftype, 
                               int remoteProcessId, int tag, 
                               MPI_Datatype datatype, MPI_Comm *Handle, 
                               int useCopy) 
{
  if (useCopy)
    {
    int retVal;
    
    char* tmpData = vtkMPICommunicator::Allocate(length*sizeoftype);
    memcpy(tmpData, data, length*sizeoftype);
    retVal = MPI_Send(tmpData, length, datatype, remoteProcessId, tag, 
                      *(Handle));
    vtkMPICommunicator::Free(tmpData);
    return retVal;
    }
  else
    {
    return MPI_Send(const_cast<T *>(data), length, datatype,
                    remoteProcessId, tag, *(Handle));
    }
}
//----------------------------------------------------------------------------
template <class T>
int vtkMPICommunicatorReceiveData(T* data, int length, int sizeoftype, 
                                  int remoteProcessId, int tag, 
                                  MPI_Datatype datatype, MPI_Comm *Handle, 
                                  int useCopy, int& senderId)
{
  MPI_Status status; 
  
  if (remoteProcessId == vtkMultiProcessController::ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  
  int retVal;

  if (useCopy)
    {
    char* tmpData = vtkMPICommunicator::Allocate(length*sizeoftype);
    retVal = MPI_Recv(tmpData, length, datatype, remoteProcessId, tag, 
                      *(Handle), &status);
    memcpy(data, tmpData, length*sizeoftype);
    vtkMPICommunicator::Free(tmpData);
    }
  else
    {
    retVal = MPI_Recv(data, length, datatype, remoteProcessId, tag, 
                      *(Handle), &status);
    }

  if (retVal == MPI_SUCCESS)
    {
    senderId = status.MPI_SOURCE;
    }
  return retVal;
}
//----------------------------------------------------------------------------
template <class T>
int vtkMPICommunicatorNoBlockSendData(const T* data, int length, 
                                      int remoteProcessId, int tag, 
                                      MPI_Datatype datatype, 
                                      vtkMPICommunicator::Request& req, 
                                      MPI_Comm *Handle)
{
    return MPI_Isend(const_cast<T*>(data), length, datatype,
                     remoteProcessId, tag, 
                     *(Handle), &req.Req->Handle);
}
//----------------------------------------------------------------------------
template <class T>
int vtkMPICommunicatorNoBlockReceiveData(T* data, int length,
                                         int remoteProcessId, int tag, 
                                         MPI_Datatype datatype, 
                                         vtkMPICommunicator::Request& req, 
                                         MPI_Comm *Handle)
{
  if (remoteProcessId == vtkMultiProcessController::ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  
  return MPI_Irecv(data, length, datatype, remoteProcessId, tag, 
                   *(Handle), &req.Req->Handle);
}
//----------------------------------------------------------------------------
template <class T>
int vtkMPICommunicatorBroadcastData(T* data, int length, 
                                    int root, MPI_Datatype datatype, 
                                    MPI_Comm *Handle)
{
  return MPI_Bcast(data, length, datatype, root, *(Handle));
}
//----------------------------------------------------------------------------
template <class T>
int vtkMPICommunicatorGatherData(T* data, T* to,
                                 int sendlength, int root,
                                 MPI_Datatype datatype,
                                 MPI_Comm *Handle)
{

  return MPI_Gather(data, sendlength, datatype, to, 
                    sendlength, datatype, root, *(Handle));
}
//----------------------------------------------------------------------------
template <class T>
int vtkMPICommunicatorGatherVData(T* data, T* to,
                                  int sendlength, int* recvlengths,
                                  int* offsets, int root,
                                  MPI_Datatype datatype,
                                  MPI_Comm *Handle)
{
  return MPI_Gatherv(data, sendlength, datatype, 
                     to, recvlengths, offsets, datatype, 
                     root, *(Handle));
}
//----------------------------------------------------------------------------
template <class T>
int vtkMPICommunicatorAllGatherData(T* data, T* to,
                                    int sendlength,
                                    MPI_Datatype datatype,
                                    MPI_Comm *Handle)
{
  return MPI_Allgather(data, sendlength, datatype, to, 
                    sendlength, datatype, *(Handle));
}
//----------------------------------------------------------------------------
template <class T>
int vtkMPICommunicatorAllGatherVData(T* data, T* to,
                                     int sendlength, int* recvlengths,
                                     int* offsets,
                                     MPI_Datatype datatype,
                                     MPI_Comm *Handle)
{
  return MPI_Allgatherv(data, sendlength, datatype, 
                     to, recvlengths, offsets, datatype, 
                     *(Handle));
}
//----------------------------------------------------------------------------
template <class T>
int vtkMPICommunicatorReduceData(T* data, T* to, int root,
                                 int sendlength, MPI_Datatype datatype,
                                 MPI_Op op, MPI_Comm *Handle)
{
  return MPI_Reduce(data, to, sendlength, datatype, 
                    op, root, *(Handle));
}

//----------------------------------------------------------------------------
// Return the world communicator (i.e. MPI_COMM_WORLD).
// Create one if necessary (singleton).
vtkMPICommunicator* vtkMPICommunicator::GetWorldCommunicator()
{
  int err, size;

  if (vtkMPICommunicator::WorldCommunicator == 0)
    {
    vtkMPICommunicator* comm = vtkMPICommunicator::New();
    vtkMPIGroup* group = vtkMPIGroup::New();
    comm->MPIComm->Handle = new MPI_Comm;
    *(comm->MPIComm->Handle) = MPI_COMM_WORLD;
    comm->SetGroup(group);
    group->Delete();
    group = NULL;
    if ( (err = MPI_Comm_size(MPI_COMM_WORLD, &size)) != MPI_SUCCESS  )
      {
      char *msg = vtkMPIController::ErrorString(err);
      vtkGenericWarningMacro("MPI error occured: " << msg);
      delete[] msg;
      delete comm->MPIComm->Handle;
      comm->MPIComm = 0;
      comm->Delete();
      return 0;
      }
    comm->Group->Initialize(size);
    for(int i=0; i<size; i++)
      {
      comm->Group->AddProcessId(i);
      }
    comm->Initialized = 1;
    comm->KeepHandleOn();
    vtkMPICommunicator::WorldCommunicator = comm;
    }
  return vtkMPICommunicator::WorldCommunicator;
}

//----------------------------------------------------------------------------
void vtkMPICommunicator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Group: ";
  if (this->Group)
    {
    os << endl;
    this->Group->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }
  os << indent << "MPI Communicator handler: " ;
  if (this->MPIComm->Handle)
    {
    os << this->MPIComm->Handle << endl;
    }
  else
    {
    os << "(none)\n";
    }
  os << indent << "Initialized: " << (this->Initialized ? "On\n" : "Off\n");
  os << indent << "Keep handle: " << (this->KeepHandle ? "On\n" : "Off\n");
  if ( this != vtkMPICommunicator::WorldCommunicator )
    {
    os << indent << "World communicator: ";
    if (vtkMPICommunicator::WorldCommunicator)
      {
      os << endl;
      vtkMPICommunicator::WorldCommunicator->PrintSelf(os, indent.GetNextIndent());
      }
    else
      {
      os << "(none)";
      }
    os << endl;
    }
  return;
}

//----------------------------------------------------------------------------
vtkMPICommunicator::vtkMPICommunicator()
{
  this->MPIComm = new vtkMPICommunicatorOpaqueComm;
  this->Group = 0;
  this->Initialized = 0;
  this->KeepHandle = 0;
  this->LastSenderId = -1;
}

//----------------------------------------------------------------------------
vtkMPICommunicator::~vtkMPICommunicator()
{
  // Free the handle if required and asked for.
  if (this->MPIComm)
    {
    if (this->MPIComm->Handle && !this->KeepHandle )
      {
      if (*(this->MPIComm->Handle) != MPI_COMM_NULL)
        {
        MPI_Comm_free(this->MPIComm->Handle);
        }
      }
    delete this->MPIComm->Handle;
    delete this->MPIComm;
    }
  this->SetGroup(0);
}

//----------------------------------------------------------------------------
int vtkMPICommunicator::Initialize(vtkMPICommunicator* mpiComm, 
                                   vtkMPIGroup* group)
{
  if (this->Initialized)
    {
    return 0;
    }

  // If mpiComm has been initialized, it is guaranteed (unless
  // the MPI calls return an error somewhere) to have valid 
  // Communicator and Group
  if (!mpiComm->Initialized)
    {
    vtkWarningMacro("The communicator passed has not been initialized!");
    return 0;
    }

  this->KeepHandleOff();

  int nProcIds = group->GetNumberOfProcessIds();
  // The new group has to be a sub-class
  if ( ( nProcIds <= 0) ||
       ( mpiComm->Group == 0 ) ||
       ( nProcIds > mpiComm->Group->GetNumberOfProcessIds() ) )
    {
    vtkWarningMacro("The group or the communicator has " 
                    << "invalid number of ids.");
    return 0;
    }

  
  // Select the new processes
  int* ranks = new int[nProcIds];
  for(int i=0; i<nProcIds; i++)
    {
    ranks[i] = group->GetProcessId(i);
    }

  MPI_Group superGroup;
  MPI_Group subGroup;

  // Get the group from the argument
  int err;
  if ( (err = MPI_Comm_group(*(mpiComm->MPIComm->Handle), &superGroup))
       != MPI_SUCCESS )
    {
    delete[] ranks;
    MPI_Group_free(&superGroup);

    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;

    return 0;
    }
  
  // Create a new group by including the process ids in group
  if ( (err = MPI_Group_incl(superGroup, nProcIds, ranks, &subGroup))
       != MPI_SUCCESS )
    {
    delete[] ranks;
    MPI_Group_free(&superGroup);
    MPI_Group_free(&subGroup);

    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;

    return 0;
    }

  delete[] ranks;
  MPI_Group_free(&superGroup);

  this->MPIComm->Handle = new MPI_Comm;
  // Create the communicator from the group
  if ( (err  = MPI_Comm_create(*(mpiComm->MPIComm->Handle), subGroup, 
                               this->MPIComm->Handle) )
       != MPI_SUCCESS )
    {
    MPI_Group_free(&subGroup);
    delete this->MPIComm->Handle;
    this->MPIComm->Handle = 0;

    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;

    return 0;
    }
       
  MPI_Group_free(&subGroup);

  this->Initialized = 1;

  // Store the group so that this communicator can be used
  // to create new ones
  this->SetGroup(group);

  this->Modified();

  return 1;

}

//----------------------------------------------------------------------------
// Start the copying process  
void vtkMPICommunicator::InitializeCopy(vtkMPICommunicator* source)
{
  if(!source)
    {
    return;
    }

  this->SetGroup(0);
  vtkMPIGroup* group = vtkMPIGroup::New();
  this->SetGroup(group);
  group->Delete();
  group = 0;
  this->Group->CopyFrom(source->Group);

  if (this->MPIComm->Handle && !this->KeepHandle)
    {
    MPI_Comm_free(this->MPIComm->Handle);
    }
  delete this->MPIComm->Handle;
  this->MPIComm->Handle = 0;

  this->Initialized = source->Initialized;
  this->Modified();
}

//----------------------------------------------------------------------------
// Copy the MPI handle
void vtkMPICommunicator::CopyFrom(vtkMPICommunicator* source)
{
  this->InitializeCopy(source);

  if (source->MPIComm->Handle)
    {
    this->KeepHandleOn();
    this->MPIComm->Handle = new MPI_Comm;
    *(this->MPIComm->Handle) = *(source->MPIComm->Handle);
    }
}

//----------------------------------------------------------------------------
// Duplicate the MPI handle
void vtkMPICommunicator::Duplicate(vtkMPICommunicator* source)
{
  this->InitializeCopy(source);

  this->KeepHandleOff();

  if (source->MPIComm->Handle)
    {
    this->MPIComm->Handle = new MPI_Comm;
    int err;
    if ( (err = MPI_Comm_dup(*(source->MPIComm->Handle), this->MPIComm->Handle))
          != MPI_SUCCESS ) 
      {
      char *msg = vtkMPIController::ErrorString(err);
      vtkErrorMacro("MPI error occured: " << msg);
      delete[] msg;
      }                      
    }
}

//----------------------------------------------------------------------------
char* vtkMPICommunicator::Allocate(size_t size)
{
#ifdef MPIPROALLOC
  char* ptr;
  MPI_Alloc_mem(size, NULL, &ptr);
  return ptr;
#else
  return new char[size];
#endif
}

//----------------------------------------------------------------------------
void vtkMPICommunicator::Free(char* ptr)
{
#ifdef MPIPROALLOC
  MPI_Free_mem(ptr);
#else
  delete[] ptr;
#endif
}

//----------------------------------------------------------------------------
int vtkMPICommunicator::CheckForMPIError(int err)
{

  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkGenericWarningMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }

}

//-----------------------------------------------------------------------------
int vtkMPICommunicator::SendVoidArray(const void *data, vtkIdType length,
                                      int type, int remoteProcessId, int tag)
{
  const char *byteData = static_cast<const char *>(data);
  MPI_Datatype mpiType = vtkMPICommunicatorGetMPIType(type);
  int sizeOfType;
  switch(type)
    {
    vtkTemplateMacro(sizeOfType = sizeof(VTK_TT));
    default:
      vtkWarningMacro(<< "Invalid data type " << type);
      sizeOfType = 1;
      break;
    }

  int maxSend = VTK_INT_MAX/sizeOfType;
  while (length > maxSend)
    {
    vtkMPICommunicatorSendData(byteData, maxSend, sizeOfType, remoteProcessId,
                               tag, mpiType, this->MPIComm->Handle,
                               vtkCommunicator::UseCopy);
    byteData += maxSend*sizeOfType;
    length -= maxSend;
    }
  return CheckForMPIError
    (vtkMPICommunicatorSendData(byteData, length, sizeOfType, remoteProcessId,
                                tag, mpiType, this->MPIComm->Handle,
                                vtkCommunicator::UseCopy));
}

//-----------------------------------------------------------------------------
int vtkMPICommunicator::ReceiveVoidArray(void *data, vtkIdType length, int type,
                                         int remoteProcessId, int tag)
{
  char *byteData = static_cast<char *>(data);
  MPI_Datatype mpiType = vtkMPICommunicatorGetMPIType(type);
  int sizeOfType;
  switch(type)
    {
    vtkTemplateMacro(sizeOfType = sizeof(VTK_TT));
    default:
      vtkWarningMacro(<< "Invalid data type " << type);
      sizeOfType = 1;
      break;
    }

  int maxReceive = VTK_INT_MAX/sizeOfType;
  while (length > maxReceive)
    {
    vtkMPICommunicatorReceiveData(byteData, maxReceive, sizeOfType,
                                  remoteProcessId,
                                  tag, mpiType, this->MPIComm->Handle,
                                  vtkCommunicator::UseCopy,
                                  this->LastSenderId);
    remoteProcessId = this->LastSenderId;
    byteData += maxReceive*sizeOfType;
    length -= maxReceive;
    }
  return CheckForMPIError
    (vtkMPICommunicatorReceiveData(byteData, length, sizeOfType,
                                   remoteProcessId,
                                   tag, mpiType, this->MPIComm->Handle,
                                   vtkCommunicator::UseCopy,
                                   this->LastSenderId));
}

//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockSend(const int* data, int length, 
                                    int remoteProcessId, int tag,
                                    Request& req)
{

  return CheckForMPIError(
    vtkMPICommunicatorNoBlockSendData(data, 
                                      length, remoteProcessId, 
                                      tag, MPI_INT, req, this->MPIComm->Handle));
  
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockSend(const unsigned long* data, int length, 
                                    int remoteProcessId, int tag,
                                    Request& req)
{

  return CheckForMPIError(
    vtkMPICommunicatorNoBlockSendData(data, 
                                      length, remoteProcessId, 
                                      tag, MPI_UNSIGNED_LONG, req, 
                                      this->MPIComm->Handle));


}
//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockSend(const char* data, int length, 
                                    int remoteProcessId, int tag, Request& req)
{

  return CheckForMPIError(
    vtkMPICommunicatorNoBlockSendData(data, 
                                      length,  remoteProcessId,
                                      tag, MPI_CHAR, req, this->MPIComm->Handle));

}
//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockSend(const float* data, int length, 
                                    int remoteProcessId, int tag, Request& req)
{

  return CheckForMPIError(
    vtkMPICommunicatorNoBlockSendData(data, 
                                      length, remoteProcessId, 
                                      tag, MPI_FLOAT, req, 
                                      this->MPIComm->Handle));
}

//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockReceive(int* data, int length, 
                                       int remoteProcessId, int tag,
                                       Request& req)
{

  return CheckForMPIError(
    vtkMPICommunicatorNoBlockReceiveData(data, 
                                         length, remoteProcessId, 
                                         tag, MPI_INT, req, 
                                         this->MPIComm->Handle));
  
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockReceive(unsigned long* data, int length, 
                                       int remoteProcessId, int tag,
                                       Request& req)
{

  return CheckForMPIError(
    vtkMPICommunicatorNoBlockReceiveData(data, 
                                         length, remoteProcessId, 
                                         tag, MPI_UNSIGNED_LONG, req, 
                                         this->MPIComm->Handle));

}
//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockReceive(char* data, int length, 
                                       int remoteProcessId, int tag,
                                       Request& req)
{

  return CheckForMPIError(
    vtkMPICommunicatorNoBlockReceiveData(data, 
                                         length, remoteProcessId, 
                                         tag, MPI_CHAR, req, 
                                         this->MPIComm->Handle));

}
//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockReceive(float* data, int length, 
                                       int remoteProcessId, int tag,
                                       Request& req)
{

  return CheckForMPIError(
    vtkMPICommunicatorNoBlockReceiveData(data, 
                                         length, remoteProcessId, 
                                         tag, MPI_FLOAT, req, 
                                         this->MPIComm->Handle));

}
#ifdef VTK_USE_64BIT_IDS
//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockReceive(vtkIdType* data, int length, 
                                       int remoteProcessId, int tag,
                                       Request& req)
{

  return CheckForMPIError(
    vtkMPICommunicatorNoBlockReceiveData(data, 
                                         length, remoteProcessId, 
                                         tag,
                                         vtkMPICommunicatorGetMPIType(VTK_ID_TYPE),
                                         req, 
                                         this->MPIComm->Handle));

}
#endif

//----------------------------------------------------------------------------
vtkMPICommunicator::Request::Request()
{
  this->Req = new vtkMPICommunicatorOpaqueRequest;
}

//----------------------------------------------------------------------------
vtkMPICommunicator::Request::Request( const vtkMPICommunicator::Request& src )
{
  this->Req = new vtkMPICommunicatorOpaqueRequest;
  this->Req->Handle = src.Req->Handle;
}

//----------------------------------------------------------------------------
vtkMPICommunicator::Request& vtkMPICommunicator::Request::operator = ( const vtkMPICommunicator::Request& src )
{
  if ( this == &src )
    {
    return *this;
    }
  this->Req->Handle = src.Req->Handle;
  return *this;
}

//----------------------------------------------------------------------------
vtkMPICommunicator::Request::~Request()
{
  delete this->Req;
}

//----------------------------------------------------------------------------
int vtkMPICommunicator::Request::Test()
{
  MPI_Status status;
  int retVal;

  int err = MPI_Test(&this->Req->Handle, &retVal, &status);
  
  if ( err == MPI_SUCCESS )
    {
    return retVal;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkGenericWarningMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkMPICommunicator::Request::Wait()
{
  MPI_Status status;

  int err = MPI_Wait(&this->Req->Handle, &status);
  
  if ( err != MPI_SUCCESS )
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkGenericWarningMacro("MPI error occured: " << msg);
    delete[] msg;
    }
}

//----------------------------------------------------------------------------
void vtkMPICommunicator::Request::Cancel()
{
  int err = MPI_Cancel(&this->Req->Handle);
  
  if ( err != MPI_SUCCESS )
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkGenericWarningMacro("MPI error occured: " << msg);
    delete[] msg;
    }

  err = MPI_Request_free(&this->Req->Handle);

  if ( err != MPI_SUCCESS )
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkGenericWarningMacro("MPI error occured: " << msg);
    delete[] msg;
    }
}

//----------------------------------------------------------------------------
int vtkMPICommunicator::Broadcast(int* data, int length, int root)
{

  return CheckForMPIError(
    vtkMPICommunicatorBroadcastData(data, length, root, MPI_INT, 
                                    this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Broadcast(unsigned long* data, int length, int root)
{

  return CheckForMPIError(
    vtkMPICommunicatorBroadcastData(data, length, root, MPI_UNSIGNED_LONG, 
                                    this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Broadcast(char* data, int length, int root)
{

  return CheckForMPIError(
    vtkMPICommunicatorBroadcastData(data, length, root, MPI_CHAR, 
                                    this->MPIComm->Handle));
  
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Broadcast(float* data, int length, int root)
{

  return CheckForMPIError(
    vtkMPICommunicatorBroadcastData(data, length, root, MPI_FLOAT, 
                                    this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Broadcast(double* data, int length, int root)
{

  return CheckForMPIError(
    vtkMPICommunicatorBroadcastData(data, length, root, MPI_DOUBLE, 
                                    this->MPIComm->Handle));
}

//----------------------------------------------------------------------------
int vtkMPICommunicator::Gather(int* data, int* to, int length, int root)
{

  return CheckForMPIError(
    vtkMPICommunicatorGatherData(data, to, length, root, MPI_INT, 
                                 this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Gather(unsigned long* data, unsigned long* to, 
                               int length, int root)
{

  return CheckForMPIError(
    vtkMPICommunicatorGatherData(data, to, length, root, MPI_UNSIGNED_LONG, 
                                 this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Gather(char* data, char* to, int length, int root)
{

  return CheckForMPIError(
    vtkMPICommunicatorGatherData(data, to, length, root, MPI_CHAR, 
                                 this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Gather(float* data, float* to, int length, int root)
{

  return CheckForMPIError(
    vtkMPICommunicatorGatherData(data, to, length, root, MPI_FLOAT, 
                                 this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Gather(double* data, double* to, int length, int root)
{

  return CheckForMPIError(
    vtkMPICommunicatorGatherData(data, to, length, root, MPI_DOUBLE, 
                                 this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::GatherV(int* data, int* to, 
                                int sendlength, int* recvlengths, 
                                int* offsets, int root)
{

  return CheckForMPIError(
    vtkMPICommunicatorGatherVData(data, to, 
                                  sendlength, recvlengths, offsets,
                                  root, MPI_INT, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::GatherV(unsigned long* data, unsigned long* to, 
                                int sendlength, int* recvlengths,
                                int* offsets, int root)
{

  return CheckForMPIError(
    vtkMPICommunicatorGatherVData(data, to, 
                                  sendlength, recvlengths, offsets,
                                  root, MPI_UNSIGNED_LONG, 
                                  this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::GatherV(char* data, char* to,
                                int sendlength, int* recvlengths,
                                int* offsets, int root)
{

  return CheckForMPIError(
    vtkMPICommunicatorGatherVData(data, to, 
                                  sendlength, recvlengths, offsets,
                                  root, MPI_CHAR, 
                                  this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::GatherV(float* data, float* to, 
                                int sendlength, int* recvlengths,
                                int* offsets, int root)
{

  return CheckForMPIError(
    vtkMPICommunicatorGatherVData(data, to, 
                                  sendlength, recvlengths, offsets,
                                  root, MPI_FLOAT, 
                                  this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::GatherV(double* data, double* to,
                                int sendlength, int* recvlengths,
                                int* offsets, int root)
{

  return CheckForMPIError(
    vtkMPICommunicatorGatherVData(data, to, 
                                  sendlength, recvlengths, offsets,
                                  root, MPI_DOUBLE, 
                                  this->MPIComm->Handle));
}

//----------------------------------------------------------------------------
int vtkMPICommunicator::AllGather(int* data, int* to, int length)
{

  return CheckForMPIError(
    vtkMPICommunicatorAllGatherData(data, to, length, MPI_INT, 
                                 this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::AllGather(unsigned long* data, unsigned long* to, 
                               int length)
{

  return CheckForMPIError(
    vtkMPICommunicatorAllGatherData(data, to, length, MPI_UNSIGNED_LONG, 
                                 this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::AllGather(char* data, char* to, int length)
{

  return CheckForMPIError(
    vtkMPICommunicatorAllGatherData(data, to, length, MPI_CHAR, 
                                 this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::AllGather(float* data, float* to, int length)
{

  return CheckForMPIError(
    vtkMPICommunicatorAllGatherData(data, to, length, MPI_FLOAT, 
                                 this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::AllGather(double* data, double* to, int length)
{

  return CheckForMPIError(
    vtkMPICommunicatorAllGatherData(data, to, length, MPI_DOUBLE, 
                                 this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::AllGatherV(int* data, int* to, 
                                int sendlength, int* recvlengths, 
                                int* offsets)
{

  return CheckForMPIError(
    vtkMPICommunicatorAllGatherVData(data, to, 
                                  sendlength, recvlengths, offsets,
                                  MPI_INT, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::AllGatherV(unsigned long* data, unsigned long* to, 
                                int sendlength, int* recvlengths,
                                int* offsets)
{

  return CheckForMPIError(
    vtkMPICommunicatorAllGatherVData(data, to, 
                                  sendlength, recvlengths, offsets,
                                  MPI_UNSIGNED_LONG, 
                                  this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::AllGatherV(char* data, char* to,
                                int sendlength, int* recvlengths,
                                int* offsets)
{

  return CheckForMPIError(
    vtkMPICommunicatorAllGatherVData(data, to, 
                                  sendlength, recvlengths, offsets,
                                  MPI_CHAR, 
                                  this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::AllGatherV(float* data, float* to, 
                                int sendlength, int* recvlengths,
                                int* offsets)
{

  return CheckForMPIError(
    vtkMPICommunicatorAllGatherVData(data, to, 
                                  sendlength, recvlengths, offsets,
                                  MPI_FLOAT, 
                                  this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::AllGatherV(double* data, double* to,
                                int sendlength, int* recvlengths,
                                int* offsets)
{

  return CheckForMPIError(
    vtkMPICommunicatorAllGatherVData(data, to, 
                                  sendlength, recvlengths, offsets,
                                  MPI_DOUBLE, 
                                  this->MPIComm->Handle));
}



//----------------------------------------------------------------------------
int vtkMPICommunicator::ReduceMax(int* data, int* to,
                                  int sendlength, int root)
{
  return CheckForMPIError(
    vtkMPICommunicatorReduceData(data, to, 
                                 root, sendlength, MPI_INT, 
                                 MPI_MAX, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::ReduceMax(unsigned long* data, unsigned long* to,
                                  int sendlength, int root)
{
  return CheckForMPIError(
    vtkMPICommunicatorReduceData(data, to, 
                                 root, sendlength, MPI_LONG, 
                                 MPI_MAX, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::ReduceMax(float* data, float* to,
                                  int sendlength, int root)
{
  return CheckForMPIError(
    vtkMPICommunicatorReduceData(data, to,
                                 root, sendlength, MPI_FLOAT, 
                                 MPI_MAX, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::ReduceMax(double* data, double* to,
                                  int sendlength, int root)
{
  return CheckForMPIError(
    vtkMPICommunicatorReduceData(data, to,
                                 root, sendlength, MPI_DOUBLE, 
                                 MPI_MAX, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::ReduceMin(int* data, int* to,
                                  int sendlength, int root)
{
  return CheckForMPIError(
    vtkMPICommunicatorReduceData(data, to,
                                 root, sendlength, MPI_INT, 
                                 MPI_MIN, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::ReduceMin(unsigned long* data, unsigned long* to,
                                  int sendlength, int root)
{
  return CheckForMPIError(
    vtkMPICommunicatorReduceData(data, to, 
                                 root, sendlength, MPI_LONG, 
                                 MPI_MIN, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::ReduceMin(float* data, float* to,
                                  int sendlength, int root)
{
  return CheckForMPIError(
    vtkMPICommunicatorReduceData(data, to,
                                 root, sendlength, MPI_FLOAT, 
                                 MPI_MIN, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::ReduceMin(double* data, double* to,
                                  int sendlength, int root)
{
  return CheckForMPIError(
    vtkMPICommunicatorReduceData(data, to,
                                 root, sendlength, MPI_DOUBLE, 
                                 MPI_MIN, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::ReduceSum(int* data, int* to,
                                  int sendlength, int root)
{
  return CheckForMPIError(
    vtkMPICommunicatorReduceData(data, to,
                                 root, sendlength, MPI_INT, 
                                 MPI_SUM, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::ReduceSum(unsigned long* data, unsigned long* to,
                                  int sendlength, int root)
{
  return CheckForMPIError(
    vtkMPICommunicatorReduceData(data, to, 
                                 root, sendlength, MPI_LONG, 
                                 MPI_SUM, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::ReduceSum(float* data, float* to,
                                  int sendlength, int root)
{
  return CheckForMPIError(
    vtkMPICommunicatorReduceData(data, to,
                                 root, sendlength, MPI_FLOAT, 
                                 MPI_SUM, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::ReduceSum(double* data, double* to,
                                  int sendlength, int root)
{
  return CheckForMPIError(
    vtkMPICommunicatorReduceData(data, to,
                                 root, sendlength, MPI_DOUBLE, 
                                 MPI_SUM, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
// There is no MPI data type bool in the C binding of MPI 
// -> convert to int!
int vtkMPICommunicator::ReduceAnd(bool* data, bool* to,
                                  int size, int root)
{
  int i;

  int *intsbuffer;
  int *intrbuffer;

  intsbuffer = new int[size]; 
  intrbuffer = new int[size]; 

  for (i = 0; i < size; ++i)
    {
    intsbuffer[i] = (data[i] ? 1 : 0);
    }

  int err = CheckForMPIError(
    vtkMPICommunicatorReduceData(intsbuffer, intrbuffer,
                                 root, size, MPI_INT, 
                                 MPI_LAND, this->MPIComm->Handle));

  for (i = 0; i < size; ++i)
    {
    to[i] = (intrbuffer[i] == 1);
    }

  delete [] intsbuffer;
  delete [] intrbuffer;

  return err;
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::ReduceOr(bool* data, bool* to,
                                 int size, int root)
{
  int *intsbuffer;
  int *intrbuffer;
  int i;

  intsbuffer = new int[size]; 
  intrbuffer = new int[size]; 

  for (i = 0; i < size; ++i)
    {
    intsbuffer[i] = (data[i] ? 1 : 0);
    }

  int err = CheckForMPIError(
    vtkMPICommunicatorReduceData(intsbuffer, intrbuffer,
                                 root, size, MPI_INT, 
                                 MPI_LOR, this->MPIComm->Handle));

  for (i = 0; i < size; ++i)
    {
    to[i] = (intrbuffer[i] == 1);
    }

  delete [] intsbuffer;
  delete [] intrbuffer;

  return err;
}
