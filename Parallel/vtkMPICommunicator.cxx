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

#include "vtkMPIController.h"
#include "vtkMPIGroup.h"
#include "vtkMPIGroup.h"
#include "vtkObjectFactory.h"
#include "vtkToolkits.h"

#include "vtkMPI.h"

vtkCxxRevisionMacro(vtkMPICommunicator, "1.37");
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
// overloaded functions for vtkIdType
#ifdef VTK_HAS_ID_TYPE
# ifdef VTK_USE_64BIT_IDS
MPI_Datatype vtkMPICommunicatorGetMPIType()
{
#if VTK_SIZEOF_LONG == 8
  return MPI_LONG;
#elif defined(MPI_LONG_LONG)
  return MPI_LONG_LONG;
#elif defined(MPI_LONG_LONG_INT)
  // lampi only has MPI_LONG_LONG_INT, not MPI_LONG_LONG
  return MPI_LONG_LONG_INT;
#else
  vtkGenericWarningMacro("This systems MPI doesnt seem to support 64 bit ids and you have 64 bit IDs turned on. Please contact VTK mailing list.");
  return MPI_INT;
#endif
}
# endif
#else
MPI_Datatype vtkMPICommunicatorGetMPIType()
{
  return MPI_INT;
}
#endif

//----------------------------------------------------------------------------
template <class T>
int vtkMPICommunicatorSendData(T* data, int length, int sizeoftype, 
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
    return MPI_Send(data, length, datatype, remoteProcessId, tag, *(Handle));
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
int vtkMPICommunicatorNoBlockSendData(T* data, int length, 
                                      int remoteProcessId, int tag, 
                                      MPI_Datatype datatype, 
                                      vtkMPICommunicator::Request& req, 
                                      MPI_Comm *Handle)
{
    return MPI_Isend(data, length, datatype, remoteProcessId, tag, 
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

//----------------------------------------------------------------------------
int vtkMPICommunicator::Send(int* data, int length, int remoteProcessId, 
                            int tag)
{

  return CheckForMPIError(
     vtkMPICommunicatorSendData(data, length, 
                                sizeof(int), remoteProcessId, tag, 
                                MPI_INT, this->MPIComm->Handle, 
                                vtkCommunicator::UseCopy));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Send(unsigned long* data, int length, 
                             int remoteProcessId, int tag)
{

  return CheckForMPIError(
    vtkMPICommunicatorSendData(data, length, 
                              sizeof(unsigned long), remoteProcessId, tag, 
                              MPI_UNSIGNED_LONG, this->MPIComm->Handle,
                              vtkCommunicator::UseCopy));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Send(char* data, int length, 
                             int remoteProcessId, int tag)
{

  return CheckForMPIError(
    vtkMPICommunicatorSendData(data, length, 
                               sizeof(char), remoteProcessId, tag, 
                               MPI_CHAR, this->MPIComm->Handle, 
                               vtkCommunicator::UseCopy));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Send(unsigned char* data, int length, 
                             int remoteProcessId, int tag)
{

  return CheckForMPIError(
    vtkMPICommunicatorSendData(data, length, 
                               sizeof(unsigned char), remoteProcessId, tag, 
                               MPI_UNSIGNED_CHAR, this->MPIComm->Handle,
                               vtkCommunicator::UseCopy));
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Send(float* data, int length, 
                             int remoteProcessId, int tag)
{

  return CheckForMPIError(
    vtkMPICommunicatorSendData(data, length, 
                               sizeof(float), remoteProcessId, tag, 
                               MPI_FLOAT, this->MPIComm->Handle, 
                               vtkCommunicator::UseCopy));
}

//----------------------------------------------------------------------------
int vtkMPICommunicator::Send(double* data, int length, 
                             int remoteProcessId, int tag)
{

  return CheckForMPIError(
    vtkMPICommunicatorSendData(data, length, 
                               sizeof(double), remoteProcessId, tag, 
                               MPI_DOUBLE, this->MPIComm->Handle, 
                               vtkCommunicator::UseCopy));
}

//----------------------------------------------------------------------------
#ifdef VTK_USE_64BIT_IDS
int vtkMPICommunicator::Send(vtkIdType* data, int length, 
                             int remoteProcessId, int tag)
{

  return CheckForMPIError(
    vtkMPICommunicatorSendData(data, length, 
                               sizeof(vtkIdType), remoteProcessId, tag, 
                               vtkMPICommunicatorGetMPIType(), 
                               this->MPIComm->Handle, vtkCommunicator::UseCopy));
}
#endif

//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockSend(int* data, int length, 
                                    int remoteProcessId, int tag,
                                    Request& req)
{

  return CheckForMPIError(
    vtkMPICommunicatorNoBlockSendData(data, 
                                      length, remoteProcessId, 
                                      tag, MPI_INT, req, this->MPIComm->Handle));
  
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockSend(unsigned long* data, int length, 
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
int vtkMPICommunicator::NoBlockSend(char* data, int length, 
                                    int remoteProcessId, int tag, Request& req)
{

  return CheckForMPIError(
    vtkMPICommunicatorNoBlockSendData(data, 
                                      length,  remoteProcessId,
                                      tag, MPI_CHAR, req, this->MPIComm->Handle));

}
//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockSend(float* data, int length, 
                                    int remoteProcessId, int tag, Request& req)
{

  return CheckForMPIError(
    vtkMPICommunicatorNoBlockSendData(data, 
                                      length, remoteProcessId, 
                                      tag, MPI_FLOAT, req, 
                                      this->MPIComm->Handle));
}

//----------------------------------------------------------------------------
int vtkMPICommunicator::Receive(int* data, int length, 
                                int remoteProcessId, int tag)
{

  return CheckForMPIError(
    vtkMPICommunicatorReceiveData(data, length, 
                                  sizeof(int), remoteProcessId, tag, 
                                  MPI_INT, this->MPIComm->Handle, 
                                  vtkCommunicator::UseCopy,
                                  this->LastSenderId));

}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Receive(unsigned long* data, int length, 
                                int remoteProcessId, int tag)
{

  return CheckForMPIError(
    vtkMPICommunicatorReceiveData(data, length,
                                  sizeof(unsigned long),
                                  remoteProcessId, tag, 
                                  MPI_UNSIGNED_LONG, this->MPIComm->Handle,
                                  vtkCommunicator::UseCopy,
                                  this->LastSenderId));

}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Receive(char* data, int length, 
                                int remoteProcessId, int tag)
{

  return CheckForMPIError(
    vtkMPICommunicatorReceiveData(data, length, 
                                  sizeof(char), remoteProcessId, tag, 
                                  MPI_CHAR, this->MPIComm->Handle, 
                                  vtkCommunicator::UseCopy,
                                  this->LastSenderId));

}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Receive(unsigned char* data, int length, 
                                int remoteProcessId, int tag)
{

  return CheckForMPIError(
    vtkMPICommunicatorReceiveData(data, length,
                                  sizeof(unsigned char), remoteProcessId, 
                                  tag, MPI_UNSIGNED_CHAR, this->MPIComm->Handle,
                                  vtkCommunicator::UseCopy,
                                  this->LastSenderId));

}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Receive(float* data, int length, 
                                int remoteProcessId, int tag)
{

  return CheckForMPIError(
    vtkMPICommunicatorReceiveData(data, length, 
                                  sizeof(float), remoteProcessId, tag, 
                                  MPI_FLOAT, this->MPIComm->Handle, 
                                  vtkCommunicator::UseCopy,
                                  this->LastSenderId));

}

//----------------------------------------------------------------------------
int vtkMPICommunicator::Receive(double* data, int length, 
                                int remoteProcessId, int tag)
{

  return CheckForMPIError(
    vtkMPICommunicatorReceiveData(data, length,
                                  sizeof(double), remoteProcessId, tag, 
                                  MPI_DOUBLE, this->MPIComm->Handle, 
                                  vtkCommunicator::UseCopy,
                                  this->LastSenderId));

}

//----------------------------------------------------------------------------
#ifdef VTK_USE_64BIT_IDS
int vtkMPICommunicator::Receive(vtkIdType* data, int length, 
                                int remoteProcessId, int tag)
{

  return CheckForMPIError(
    vtkMPICommunicatorReceiveData(data, length, 
                                  sizeof(vtkIdType), remoteProcessId, tag, 
                                  vtkMPICommunicatorGetMPIType(), 
                                  this->MPIComm->Handle, 
                                  vtkCommunicator::UseCopy,
                                  this->LastSenderId));
}
#endif

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

//----------------------------------------------------------------------------
vtkMPICommunicator::Request::Request()
{
  this->Req = new vtkMPICommunicatorOpaqueRequest;
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

//----------------------------------------------------------------------------
int vtkMPICommunicator::Receive(vtkDataObject* data, 
                                int remoteHandle, 
                                int tag)
{
  int dataLength;

  // First receive the data length.
  if (!this->Receive( &dataLength, 1, remoteHandle, tag))
    {
    vtkErrorMacro("Could not receive data!");
    return 0;
    }
  
  int senderId = this->LastSenderId;

  if (dataLength < 0)
    {
    vtkErrorMacro("Bad data length");
    return 0;
    }
  
  if (dataLength == 0)
    { // This indicates a NULL object was sent. Do nothing.
    return 1;   
    }
  
  // if we cannot reuse the string, allocate a new one.
  if (dataLength > this->MarshalStringLength)
    {
    char *str = new char[dataLength + 10]; // maybe a little extra?
    this->DeleteAndSetMarshalString(str, dataLength + 10);
    }
  
  // Receive the string from the same processor as the
  // data length.
  if (!this->Receive(this->MarshalString, dataLength, 
                     senderId, tag))
    {
    return 0;
    }

  this->MarshalDataLength = dataLength;

  this->ReadObject(data);

  // we should really look at status to determine success
  return 1;
}
