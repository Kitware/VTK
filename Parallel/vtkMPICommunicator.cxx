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
//#include "vtkMPIGroup.h"
#include "vtkProcessGroup.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkToolkits.h"
#include "vtkMPI.h"
#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <vector>
#include <assert.h>

static inline void  vtkMPICommunicatorDebugBarrier(MPI_Comm* handle)
{
  // If NDEBUG is defined, do nothing.
#ifdef NDEBUG
  (void)handle; // to avoid warning about unused parameter
#else
  MPI_Barrier(*handle);
#endif
}

vtkStandardNewMacro(vtkMPICommunicator);

vtkMPICommunicator* vtkMPICommunicator::WorldCommunicator = 0;


vtkMPICommunicatorOpaqueComm::vtkMPICommunicatorOpaqueComm(MPI_Comm* handle)
{
  this->Handle = handle;
}

MPI_Comm* vtkMPICommunicatorOpaqueComm::GetHandle()
{
  return this->Handle;
}

//-----------------------------------------------------------------------------
// This MPI error handler basically does the same thing as the default error
// handler, but also provides a convenient place to attache a debugger
// breakpoint.
extern "C" void vtkMPICommunicatorMPIErrorHandler(MPI_Comm *comm,
                                                  int *errorcode, ...)
{
  char ErrorMessage[MPI_MAX_ERROR_STRING];
  int len;
  MPI_Error_string(*errorcode, ErrorMessage, &len);
  vtkGenericWarningMacro(<< "MPI had an error" << endl
                         << "------------------------------------------------"
                         << endl << ErrorMessage << endl
                         << "------------------------------------------------");
  MPI_Abort(*comm, *errorcode);
}

//----------------------------------------------------------------------------
// I wish I could think of a better way to convert a VTK type enum to an MPI
// type enum and back.

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

inline MPI_Datatype vtkMPICommunicatorGetMPIType(int vtkType)
{

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

inline int vtkMPICommunicatorGetVTKType(MPI_Datatype type)
{
  if (type == MPI_FLOAT)                return VTK_FLOAT;
  if (type == MPI_DOUBLE)               return VTK_DOUBLE;
  if (type == MPI_BYTE)                 return VTK_CHAR;
  if (type == MPI_CHAR)                 return VTK_CHAR;
  if (type == MPI_UNSIGNED_CHAR)        return VTK_UNSIGNED_CHAR;
#ifdef MPI_SIGNED_CHAR
  if (type == MPI_SIGNED_CHAR)          return VTK_SIGNED_CHAR;
#endif
  if (type == MPI_SHORT)                return VTK_SHORT;
  if (type == MPI_UNSIGNED_SHORT)       return VTK_UNSIGNED_SHORT;
  if (type == MPI_INT)                  return VTK_INT;
  if (type == MPI_UNSIGNED)             return VTK_UNSIGNED_INT;
  if (type == MPI_LONG)                 return VTK_LONG;
  if (type == MPI_UNSIGNED_LONG)        return VTK_UNSIGNED_LONG;
#ifdef MPI_LONG_LONG
  if (type == MPI_LONG_LONG)            return VTK_LONG_LONG;
#endif
#ifdef MPI_UNSIGNED_LONG_LONG
  if (type == MPI_UNSIGNED_LONG_LONG)   return VTK_UNSIGNED_LONG_LONG;
#endif

  vtkGenericWarningMacro("Received unrecognized MPI type.");
  return VTK_CHAR;
}

inline int vtkMPICommunicatorCheckSize(int vtkType, vtkIdType length)
{
  int typeSize;
  switch(vtkType)
    {
    vtkTemplateMacro(typeSize = sizeof(VTK_TT));
    default:
      typeSize = 1;
      break;
    }

  if (length*typeSize > VTK_INT_MAX)
    {
    vtkGenericWarningMacro(<< "This operation not yet supported for more than "
                           << VTK_INT_MAX << " bytes");
    return 0;
    }
  else
    {
    return 1;
    }
}

//----------------------------------------------------------------------------
template <class T>
int vtkMPICommunicatorSendData(const T* data, int length, int sizeoftype, 
                               int remoteProcessId, int tag, 
                               MPI_Datatype datatype, MPI_Comm *Handle, 
                               int useCopy,
                               int useSsend)
{
  if (useCopy)
    {
    int retVal;

    char* tmpData = vtkMPICommunicator::Allocate(length*sizeoftype);
    memcpy(tmpData, data, length*sizeoftype);
    if (useSsend)
      {
      retVal = MPI_Ssend(tmpData, length, datatype, remoteProcessId, tag, 
        *(Handle));
      }
    else
      {
      retVal = MPI_Send(tmpData, length, datatype, remoteProcessId, tag, 
        *(Handle));
      }
    vtkMPICommunicator::Free(tmpData);
    return retVal;
    }
  else
    {
    if (useSsend)
      {
      return MPI_Ssend(const_cast<T *>(data), length, datatype,
        remoteProcessId, tag, *(Handle));
      }
    else
      {
      return MPI_Send(const_cast<T *>(data), length, datatype,
        remoteProcessId, tag, *(Handle));
      }
    }
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::ReceiveDataInternal(
  char* data, int length, int sizeoftype,
  int remoteProcessId, int tag,
  vtkMPICommunicatorReceiveDataInfo* info,
  int useCopy, int& senderId)
{
  if (remoteProcessId == vtkMultiProcessController::ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  
  int retVal;

  if (useCopy)
    {
    char* tmpData = vtkMPICommunicator::Allocate(length*sizeoftype);
    retVal = MPI_Recv(tmpData, length, info->DataType, remoteProcessId, tag, 
                      *(info->Handle), &(info->Status));
    memcpy(data, tmpData, length*sizeoftype);
    vtkMPICommunicator::Free(tmpData);
    }
  else
    {
    retVal = MPI_Recv(data, length, info->DataType, remoteProcessId, tag, 
                      *(info->Handle), &(info->Status));
    }

  if (retVal == MPI_SUCCESS)
    {
    senderId = info->Status.MPI_SOURCE;
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
int vtkMPICommunicatorReduceData(const void *sendBuffer, void *recvBuffer,
                                 vtkIdType length, int type,
                                 MPI_Op operation, int destProcessId,
                                 MPI_Comm *comm)
{
  if (!vtkMPICommunicatorCheckSize(type, length)) return 0;
  MPI_Datatype mpiType = vtkMPICommunicatorGetMPIType(type);
  return MPI_Reduce(const_cast<void *>(sendBuffer), recvBuffer, length, mpiType,
                    operation, destProcessId, *comm);
}
//----------------------------------------------------------------------------
int vtkMPICommunicatorAllReduceData(const void *sendBuffer, void *recvBuffer,
                                    vtkIdType length, int type,
                                    MPI_Op operation, MPI_Comm *comm)
{
  if (!vtkMPICommunicatorCheckSize(type, length)) return 0;
  MPI_Datatype mpiType = vtkMPICommunicatorGetMPIType(type);
  return MPI_Allreduce(const_cast<void *>(sendBuffer), recvBuffer,
                       length, mpiType, operation, *comm);
}

//----------------------------------------------------------------------------
int vtkMPICommunicatorIprobe(int source, int tag, int* flag,
                             int* actualSource, MPI_Datatype datatype,
                             int* size, MPI_Comm * handle)
{
  if (source == vtkMultiProcessController::ANY_SOURCE)
    {
    source = MPI_ANY_SOURCE;
    }
  MPI_Status status;
  int retVal = MPI_Iprobe(source, tag, *handle,
                          flag, &status);
  if(retVal == MPI_SUCCESS && *flag == 1)
    {
    if(actualSource)
      {
      *actualSource = status.MPI_SOURCE;
      }
    if(size)
      {
      return MPI_Get_count(&status, datatype, size);
      }
    }
  return retVal;
}

//-----------------------------------------------------------------------------
// Method for converting an MPI operation to a
// vtkMultiProcessController::Operation.
// MPIAPI is defined in the microsoft mpi.h which decorates
// with the __stdcall decoration.
static vtkCommunicator::Operation *CurrentOperation;
#ifdef MPIAPI
extern "C" void MPIAPI vtkMPICommunicatorUserFunction(void *invec, void *inoutvec,
                                               int *len, MPI_Datatype *datatype)
#else
extern "C" void vtkMPICommunicatorUserFunction(void *invec, void *inoutvec,
                                               int *len, MPI_Datatype *datatype)
#endif
{
  int vtkType = vtkMPICommunicatorGetVTKType(*datatype);
  CurrentOperation->Function(invec, inoutvec, *len, vtkType);
}

//----------------------------------------------------------------------------
// Return the world communicator (i.e. MPI_COMM_WORLD).
// Create one if necessary (singleton).
vtkMPICommunicator* vtkMPICommunicator::GetWorldCommunicator()
{
  int err, size;

  if (vtkMPICommunicator::WorldCommunicator == 0)
    {
    // Install an error handler
    MPI_Errhandler errhandler;
    MPI_Errhandler_create(vtkMPICommunicatorMPIErrorHandler, &errhandler);
    MPI_Errhandler_set(MPI_COMM_WORLD, errhandler);
    MPI_Errhandler_free(&errhandler);

    vtkMPICommunicator* comm = vtkMPICommunicator::New();
    comm->MPIComm->Handle = new MPI_Comm;
    *(comm->MPIComm->Handle) = MPI_COMM_WORLD;
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
    comm->InitializeNumberOfProcesses();
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
  os << indent << "MPI Communicator handler: " ;
  if (this->MPIComm->Handle)
    {
    os << this->MPIComm->Handle << endl;
    }
  else
    {
    os << "(none)\n";
    }
  os << indent << "UseSsend: " << (this->UseSsend? "On" : " Off") << endl;
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
  this->Initialized = 0;
  this->KeepHandle = 0;
  this->LastSenderId = -1;
  this->UseSsend = 0;
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
}

//-----------------------------------------------------------------------------
//#ifndef VTK_LEGACY_REMOVE
//int vtkMPICommunicator::Initialize(vtkMPICommunicator  *mpiComm,
//                                   vtkMPIGroup *deprecatedGroup)
//{
//  VTK_LEGACY_REPLACED_BODY(Initialize(vtkMPICommunicator *, vtkMPIGroup *),
//                           "5.2", Initialize(vtkProcessGroup *));
//
//  VTK_CREATE(vtkProcessGroup, group);
//  deprecatedGroup->CopyInto(group, mpiComm);
//
//  return this->Initialize(group);
//}
//#endif

//-----------------------------------------------------------------------------
int vtkMPICommunicator::Initialize(vtkProcessGroup *group)
{
  if (this->Initialized)
    {
    return 0;
    }

  vtkMPICommunicator *mpiComm
    = vtkMPICommunicator::SafeDownCast(group->GetCommunicator());
  if (!mpiComm)
    {
    vtkErrorMacro("The group is not attached to an MPI communicator!");
    return 0;
    }

  // If mpiComm has been initialized, it is guaranteed (unless the MPI calls
  // return an error somewhere) to have valid Communicator.
  if (!mpiComm->Initialized)
    {
    vtkWarningMacro("The communicator passed has not been initialized!");
    return 0;
    }

  this->KeepHandleOff();

  // Select the new processes
  int nProcIds = group->GetNumberOfProcessIds();
  int* ranks = new int[nProcIds];
  for(int i=0; i<nProcIds; i++)
    {
    ranks[i] = group->GetProcessId(i);
    }

  MPI_Group superGroup;
  MPI_Group subGroup;

  // Get the super group
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

  // MPI is kind of funny in that in order to create a communicator from a
  // subgroup of another communicator, it is a collective operation involving
  // all of the processes in the original communicator, not just those belonging
  // to the group.  In any process not part of the group, the communicator is
  // created with MPI_COMM_NULL.  Check for that and only finish initialization
  // when the controller is not MPI_COMM_NULL.
  if (*(this->MPIComm->Handle) != MPI_COMM_NULL)
    {
    this->InitializeNumberOfProcesses();
    this->Initialized = 1;
    }

  this->Modified();

  return 1;
}

//-----------------------------------------------------------------------------
int vtkMPICommunicator::SplitInitialize(vtkCommunicator *oldcomm,
                                        int color, int key)
{
  if (this->Initialized) return 0;

  vtkMPICommunicator *mpiComm = vtkMPICommunicator::SafeDownCast(oldcomm);
  if (!mpiComm)
    {
    vtkErrorMacro("Split communicator must be an MPI communicator.");
    return 0;
    }

  // If mpiComm has been initialized, it is guaranteed (unless the MPI calls
  // return an error somewhere) to have valid Communicator.
  if (!mpiComm->Initialized)
    {
    vtkWarningMacro("The communicator passed has not been initialized!");
    return 0;
    }

  this->KeepHandleOff();

  this->MPIComm->Handle = new MPI_Comm;
  int err;
  if (   (err = MPI_Comm_split(*(mpiComm->MPIComm->Handle), color, key,
                               this->MPIComm->Handle))
      != MPI_SUCCESS )
    {
    delete this->MPIComm->Handle;
    this->MPIComm->Handle = 0;

    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;

    return 0;
    }

  this->InitializeNumberOfProcesses();
  this->Initialized = 1;

  this->Modified();

  return 1;
}

int vtkMPICommunicator::InitializeExternal (vtkMPICommunicatorOpaqueComm* comm)
{
  this->KeepHandleOn();

  if (this->MPIComm->Handle) 
    {
    delete this->MPIComm->Handle;
    }
  this->MPIComm->Handle = new MPI_Comm (*(comm->GetHandle()));
  this->InitializeNumberOfProcesses();
  this->Initialized = 1;

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

  if (this->MPIComm->Handle && !this->KeepHandle)
    {
    MPI_Comm_free(this->MPIComm->Handle);
    }
  delete this->MPIComm->Handle;
  this->MPIComm->Handle = 0;

  this->LocalProcessId = source->LocalProcessId;
  this->NumberOfProcesses = source->NumberOfProcesses;

  this->Initialized = source->Initialized;
  this->Modified();
}

//-----------------------------------------------------------------------------
// Set the number of processes and maximum number of processes
// to the size obtained from MPI.
int vtkMPICommunicator::InitializeNumberOfProcesses()
{
  int err;

  this->Modified();

  if ( (err = MPI_Comm_size(*(this->MPIComm->Handle), 
                            &(this->MaximumNumberOfProcesses))) 
       != MPI_SUCCESS  )
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }
  
  this->NumberOfProcesses = this->MaximumNumberOfProcesses;
  
  if ( (err = MPI_Comm_rank(*(this->MPIComm->Handle),&(this->LocalProcessId))) 
       != MPI_SUCCESS)
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }
  return 1;
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

  int maxSend = VTK_INT_MAX;
  while (length >= maxSend)
    {
    if (CheckForMPIError(
        vtkMPICommunicatorSendData(byteData, maxSend, sizeOfType, remoteProcessId,
          tag, mpiType, this->MPIComm->Handle,
          vtkCommunicator::UseCopy,
          this->UseSsend)) == 0)
      {
      // Failed to send.
      return 0;
      }
    byteData += maxSend*sizeOfType;
    length -= maxSend;
    }
  return CheckForMPIError
    (vtkMPICommunicatorSendData(byteData, length, sizeOfType, remoteProcessId,
                                tag, mpiType, this->MPIComm->Handle,
                                vtkCommunicator::UseCopy, this->UseSsend));
}

//-----------------------------------------------------------------------------
inline vtkIdType vtkMPICommunicatorMin(vtkIdType a, vtkIdType b)
{
  return (a > b)? b : a;
}

//-----------------------------------------------------------------------------
int vtkMPICommunicator::ReceiveVoidArray(void *data, vtkIdType maxlength, int type,
                                         int remoteProcessId, int tag)
{
  this->Count = 0;
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

  // maxReceive is the maximum size of data that can be fetched in a one atomic
  // receive. If when sending the data-length >= maxReceive, then the sender
  // splits it into multiple packets of at most maxReceive size each.  (Note
  // that when the sending exactly maxReceive length message, it is split into 2
  // packets of sizes maxReceive and 0 repectively).
  int maxReceive = VTK_INT_MAX;
  vtkMPICommunicatorReceiveDataInfo info;
  info.Handle = this->MPIComm->Handle;
  info.DataType = mpiType;
  while (CheckForMPIError(
      this->ReceiveDataInternal(byteData,
        vtkMPICommunicatorMin(maxlength, maxReceive),
        sizeOfType, remoteProcessId,
        tag, &info,
        vtkCommunicator::UseCopy,
        this->LastSenderId)) != 0)
    {
    remoteProcessId = this->LastSenderId;

    int words_received=0;
    if (CheckForMPIError(MPI_Get_count(&info.Status, mpiType, &words_received)) == 0)
      {
      // Failed.
      return 0;
      }
    this->Count += words_received;
    byteData += words_received*sizeOfType;
    maxlength -= words_received;
    if (words_received < maxReceive)
      {
      // words_received in this packet is exactly equal to maxReceive, then it
      // means that the sender is sending atleast one more packet for this
      // message. Otherwise, we have received all the packets for this message 
      // and we no longer need to iterate.
      return 1;
      }
    }
  return 0;
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
int vtkMPICommunicator::NoBlockSend(const double* data, int length, 
                                    int remoteProcessId, int tag, Request& req)
{

  return CheckForMPIError(
    vtkMPICommunicatorNoBlockSendData(data, 
                                      length, remoteProcessId, 
                                      tag, MPI_DOUBLE, req, 
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
//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockReceive(double* data, int length, 
                                       int remoteProcessId, int tag,
                                       Request& req)
{

  return CheckForMPIError(
    vtkMPICommunicatorNoBlockReceiveData(data, 
                                         length, remoteProcessId, 
                                         tag, MPI_DOUBLE, req, 
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

//-----------------------------------------------------------------------------
void vtkMPICommunicator::Barrier()
{
  CheckForMPIError(MPI_Barrier(*this->MPIComm->Handle));
}

//----------------------------------------------------------------------------
int vtkMPICommunicator::BroadcastVoidArray(void *data, vtkIdType length,
                                           int type, int root)
{
  vtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  if (!vtkMPICommunicatorCheckSize(type, length)) return 0;
  return CheckForMPIError(MPI_Bcast(data, length,
                                    vtkMPICommunicatorGetMPIType(type),
                                    root, *this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int vtkMPICommunicator::GatherVoidArray(const void *sendBuffer,
                                        void *recvBuffer,
                                        vtkIdType length, int type,
                                        int destProcessId)
{
  vtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  int numProc;
  MPI_Comm_size(*this->MPIComm->Handle, &numProc);
  if (!vtkMPICommunicatorCheckSize(type, length*numProc)) return 0;
  MPI_Datatype mpiType = vtkMPICommunicatorGetMPIType(type);
  return CheckForMPIError(MPI_Gather(const_cast<void *>(sendBuffer),
                                     length, mpiType,
                                     recvBuffer, length, mpiType,
                                     destProcessId, *this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int vtkMPICommunicator::GatherVVoidArray(const void *sendBuffer,
                                         void *recvBuffer,
                                         vtkIdType sendLength,
                                         vtkIdType *recvLengths,
                                         vtkIdType *offsets, int type,
                                         int destProcessId)
{
  vtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  if (!vtkMPICommunicatorCheckSize(type, sendLength)) return 0;
  MPI_Datatype mpiType = vtkMPICommunicatorGetMPIType(type);
  // We have to jump through several hoops to make sure vtkIdType arrays
  // become int arrays.
  int rank;
  MPI_Comm_rank(*this->MPIComm->Handle, &rank);
  if (rank == destProcessId)
    {
    int numProc;
    MPI_Comm_size(*this->MPIComm->Handle, &numProc);
#if defined(OPEN_MPI) && (OMPI_MAJOR_VERSION <= 1) && (OMPI_MINOR_VERSION <= 2)
    // I found a bug in OpenMPI 1.2 and earlier where MPI_Gatherv sometimes
    // fails with only one process.  I am told that they will fix it in a later
    // version, but here is a workaround for now.
    if (numProc == 1)
      {
      switch(type)
        {
        vtkTemplateMacro(std::copy(
                       reinterpret_cast<const VTK_TT*>(sendBuffer),
                       reinterpret_cast<const VTK_TT*>(sendBuffer) + sendLength,
                       reinterpret_cast<VTK_TT*>(recvBuffer) + offsets[0]));
        }
      return 1;
      }
#endif //OPEN_MPI
    std::vector<int> mpiRecvLengths, mpiOffsets;
    mpiRecvLengths.resize(numProc);  mpiOffsets.resize(numProc);
    for (int i = 0; i < numProc; i++)
      {
      if (!vtkMPICommunicatorCheckSize(type, recvLengths[i] + offsets[i]))
        {
        return 0;
        }
      mpiRecvLengths[i] = recvLengths[i];
      mpiOffsets[i] = offsets[i];
      }
    return CheckForMPIError(MPI_Gatherv(const_cast<void *>(sendBuffer),
                                        sendLength, mpiType,
                                        recvBuffer, &mpiRecvLengths[0],
                                        &mpiOffsets[0], mpiType,
                                        destProcessId, *this->MPIComm->Handle));
    }
  else
    {
    return CheckForMPIError(MPI_Gatherv(const_cast<void *>(sendBuffer),
                                        sendLength, mpiType,
                                        NULL, NULL, NULL, mpiType,
                                        destProcessId, *this->MPIComm->Handle));
    }
}

//-----------------------------------------------------------------------------
int vtkMPICommunicator::ScatterVoidArray(const void *sendBuffer,
                                         void *recvBuffer,
                                         vtkIdType length, int type,
                                         int srcProcessId)
{
  vtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  if (!vtkMPICommunicatorCheckSize(type, length)) return 0;
  MPI_Datatype mpiType = vtkMPICommunicatorGetMPIType(type);
  return CheckForMPIError(MPI_Scatter(const_cast<void *>(sendBuffer),
                                      length, mpiType,
                                      recvBuffer, length, mpiType,
                                      srcProcessId, *this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int vtkMPICommunicator::ScatterVVoidArray(const void *sendBuffer,
                                          void *recvBuffer,
                                          vtkIdType *sendLengths,
                                          vtkIdType *offsets,
                                          vtkIdType recvLength, int type,
                                          int srcProcessId)
{
  vtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  if (!vtkMPICommunicatorCheckSize(type, recvLength)) return 0;
  MPI_Datatype mpiType = vtkMPICommunicatorGetMPIType(type);
  // We have to jump through several hoops to make sure vtkIdType arrays
  // become int arrays.
  int rank;
  MPI_Comm_rank(*this->MPIComm->Handle, &rank);
  if (rank == srcProcessId)
    {
    int numProc;
    MPI_Comm_size(*this->MPIComm->Handle, &numProc);
#if defined(OPEN_MPI) && (OMPI_MAJOR_VERSION <= 1) && (OMPI_MINOR_VERSION <= 2)
    // I found a bug in OpenMPI 1.2 and earlier where MPI_Scatterv sometimes
    // fails with only one process.  I am told that they will fix it in a later
    // version, but here is a workaround for now.
    if (numProc == 1)
      {
      switch(type)
        {
        vtkTemplateMacro(std::copy(
                   reinterpret_cast<const VTK_TT*>(sendBuffer) + offsets[0],
                   reinterpret_cast<const VTK_TT*>(sendBuffer) + offsets[0]
                                                               + sendLengths[0],
                   reinterpret_cast<VTK_TT*>(recvBuffer)));
        }
      return 1;
      }
#endif //OPEN_MPI
    std::vector<int> mpiSendLengths, mpiOffsets;
    mpiSendLengths.resize(numProc);  mpiOffsets.resize(numProc);
    for (int i = 0; i < numProc; i++)
      {
      if (!vtkMPICommunicatorCheckSize(type, sendLengths[i] + offsets[i]))
        {
        return 0;
        }
      mpiSendLengths[i] = sendLengths[i];
      mpiOffsets[i] = offsets[i];
      }
    return CheckForMPIError(MPI_Scatterv(const_cast<void *>(sendBuffer),
                                         &mpiSendLengths[0], &mpiOffsets[0],
                                         mpiType,
                                         recvBuffer, recvLength, mpiType,
                                         srcProcessId, *this->MPIComm->Handle));
    }
  else
    {
    return CheckForMPIError(MPI_Scatterv(NULL, NULL, NULL, mpiType,
                                         recvBuffer, recvLength, mpiType,
                                         srcProcessId, *this->MPIComm->Handle));
    }
}

//-----------------------------------------------------------------------------
int vtkMPICommunicator::AllGatherVoidArray(const void *sendBuffer,
                                           void *recvBuffer,
                                           vtkIdType length, int type)
{
  vtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  int numProc;
  MPI_Comm_size(*this->MPIComm->Handle, &numProc);
  if (!vtkMPICommunicatorCheckSize(type, length*numProc)) return 0;
  MPI_Datatype mpiType = vtkMPICommunicatorGetMPIType(type);
  return CheckForMPIError(MPI_Allgather(const_cast<void *>(sendBuffer),
                                        length, mpiType,
                                        recvBuffer, length, mpiType,
                                        *this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int vtkMPICommunicator::AllGatherVVoidArray(const void *sendBuffer,
                                            void *recvBuffer,
                                            vtkIdType sendLength,
                                            vtkIdType *recvLengths,
                                            vtkIdType *offsets, int type)
{
  vtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  if (!vtkMPICommunicatorCheckSize(type, sendLength)) return 0;
  MPI_Datatype mpiType = vtkMPICommunicatorGetMPIType(type);
  // We have to jump through several hoops to make sure vtkIdType arrays
  // become int arrays.
  int numProc;
  MPI_Comm_size(*this->MPIComm->Handle, &numProc);
#if defined(OPEN_MPI) && (OMPI_MAJOR_VERSION <= 1) && (OMPI_MINOR_VERSION <= 2)
  // I found a bug in OpenMPI 1.2 and earlier where MPI_Allgatherv sometimes
  // fails with only one process.  I am told that they will fix it in a later
  // version, but here is a workaround for now.
  if (numProc == 1)
    {
    switch(type)
      {
      vtkTemplateMacro(std::copy(
                       reinterpret_cast<const VTK_TT*>(sendBuffer),
                       reinterpret_cast<const VTK_TT*>(sendBuffer) + sendLength,
                       reinterpret_cast<VTK_TT*>(recvBuffer) + offsets[0]));
      }
    return 1;
    }

  // I found another bug with Allgatherv where it sometimes fails to send data
  // to all processes when one process is sending nothing.  That is not
  // sufficient and I have not figured out what else needs to occur (although
  // the TestMPIController test's random tries eventually catches it if you
  // run it a bunch).  I sent a report to the OpenMPI mailing list.  Hopefully
  // they will have it fixed in a future version.
  for (int i = 0; i < this->NumberOfProcesses; i++)
    {
    if (recvLengths[i] < 1)
      {
      this->Superclass::AllGatherVVoidArray(sendBuffer, recvBuffer, sendLength,
                                            recvLengths, offsets, type);
      }
    }
#endif //OPEN_MPI
  std::vector<int> mpiRecvLengths, mpiOffsets;
  mpiRecvLengths.resize(numProc);  mpiOffsets.resize(numProc);
  for (int i = 0; i < numProc; i++)
    {
    if (!vtkMPICommunicatorCheckSize(type, recvLengths[i] + offsets[i]))
      {
      return 0;
      }
    mpiRecvLengths[i] = recvLengths[i];
    mpiOffsets[i] = offsets[i];
    }
  return CheckForMPIError(MPI_Allgatherv(const_cast<void *>(sendBuffer),
                                         sendLength, mpiType,
                                         recvBuffer, &mpiRecvLengths[0],
                                         &mpiOffsets[0], mpiType,
                                         *this->MPIComm->Handle));
}  

//-----------------------------------------------------------------------------
int vtkMPICommunicator::ReduceVoidArray(const void *sendBuffer,
                                        void *recvBuffer,
                                        vtkIdType length, int type,
                                        int operation, int destProcessId)
{
  vtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  MPI_Op mpiOp;
  switch (operation)
    {
    case MAX_OP:                mpiOp = MPI_MAX;        break;
    case MIN_OP:                mpiOp = MPI_MIN;        break;
    case SUM_OP:                mpiOp = MPI_SUM;        break;
    case PRODUCT_OP:            mpiOp = MPI_PROD;       break;
    case LOGICAL_AND_OP:        mpiOp = MPI_LAND;       break;
    case BITWISE_AND_OP:        mpiOp = MPI_BAND;       break;
    case LOGICAL_OR_OP:         mpiOp = MPI_LOR;        break;
    case BITWISE_OR_OP:         mpiOp = MPI_BOR;        break;
    case LOGICAL_XOR_OP:        mpiOp = MPI_LXOR;       break;
    case BITWISE_XOR_OP:        mpiOp = MPI_BXOR;       break;
    default:
      vtkWarningMacro(<< "Operation number " << operation << " not supported.");
      return 0;
    }
  return CheckForMPIError(vtkMPICommunicatorReduceData(sendBuffer, recvBuffer,
                                                       length, type,
                                                       mpiOp, destProcessId,
                                                       this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int vtkMPICommunicator::ReduceVoidArray(
                                const void *sendBuffer, void *recvBuffer,
                                vtkIdType length, int type,
                                Operation *operation, int destProcessId)
{
  vtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  MPI_Op mpiOp;
  MPI_Op_create(vtkMPICommunicatorUserFunction, operation->Commutative(),
                &mpiOp);
  // Setting a static global variable like this is not thread safe, but I
  // cannot think of another alternative.
  CurrentOperation = operation;

  int res;
  res = CheckForMPIError(vtkMPICommunicatorReduceData(sendBuffer, recvBuffer,
                                                      length, type,
                                                      mpiOp, destProcessId,
                                                      this->MPIComm->Handle));

  MPI_Op_free(&mpiOp);

  return res;
}

//-----------------------------------------------------------------------------
int vtkMPICommunicator::AllReduceVoidArray(const void *sendBuffer,
                                           void *recvBuffer,
                                           vtkIdType length, int type,
                                           int operation)
{
  vtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  MPI_Op mpiOp;
  switch (operation)
    {
    case MAX_OP:         mpiOp = MPI_MAX;     break;
    case MIN_OP:         mpiOp = MPI_MIN;     break;
    case SUM_OP:         mpiOp = MPI_SUM;     break;
    case PRODUCT_OP:     mpiOp = MPI_PROD;    break;
    case LOGICAL_AND_OP: mpiOp = MPI_LAND;    break;
    case BITWISE_AND_OP: mpiOp = MPI_BAND;    break;
    case LOGICAL_OR_OP:  mpiOp = MPI_LOR;     break;
    case BITWISE_OR_OP:  mpiOp = MPI_BOR;     break;
    case LOGICAL_XOR_OP: mpiOp = MPI_LXOR;    break;
    case BITWISE_XOR_OP: mpiOp = MPI_BXOR;    break;
    default:
      vtkWarningMacro(<< "Operation number " << operation << " not supported.");
      return 0;
    }
  return CheckForMPIError(vtkMPICommunicatorAllReduceData(sendBuffer,
                                                          recvBuffer,
                                                          length, type,
                                                          mpiOp,
                                                          this->MPIComm->Handle
                                                          ));
}

//-----------------------------------------------------------------------------
int vtkMPICommunicator::AllReduceVoidArray(
                                const void *sendBuffer, void *recvBuffer,
                                vtkIdType length, int type,
                                Operation *operation)
{
  vtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  MPI_Op mpiOp;
  MPI_Op_create(vtkMPICommunicatorUserFunction, operation->Commutative(),
                &mpiOp);
  // Setting a static global variable like this is not thread safe, but I
  // cannot think of another alternative.
  CurrentOperation = operation;

  int res;
  res = CheckForMPIError(vtkMPICommunicatorAllReduceData(sendBuffer, recvBuffer,
                                                         length, type,
                                                         mpiOp,
                                                         this->MPIComm->Handle
                                                         ));

  MPI_Op_free(&mpiOp);

  return res;
}
//-----------------------------------------------------------------------------
int vtkMPICommunicator::Iprobe(
  int source, int tag, int* flag, int* actualSource)
{
  return CheckForMPIError(
    vtkMPICommunicatorIprobe(source, tag, flag, actualSource,
                             MPI_INT, NULL, this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int vtkMPICommunicator::Iprobe(
  int source, int tag, int* flag, int* actualSource,
  int* vtkNotUsed(type), int* size)
{
  return CheckForMPIError(
    vtkMPICommunicatorIprobe(source, tag, flag, actualSource,
                             MPI_INT, size, this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int vtkMPICommunicator::Iprobe(
  int source, int tag, int* flag, int* actualSource,
  unsigned long* vtkNotUsed(type), int* size)
{
  return CheckForMPIError(
    vtkMPICommunicatorIprobe(source, tag, flag, actualSource,
                             MPI_UNSIGNED_LONG, size, this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int vtkMPICommunicator::Iprobe(
  int source, int tag, int* flag, int* actualSource,
  const char* vtkNotUsed(type), int* size)
{
  return CheckForMPIError(
    vtkMPICommunicatorIprobe(source, tag, flag, actualSource,
                             MPI_CHAR, size, this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int vtkMPICommunicator::Iprobe(
  int source, int tag, int* flag, int* actualSource,
  float* vtkNotUsed(type), int* size)
{
  return CheckForMPIError(
    vtkMPICommunicatorIprobe(source, tag, flag, actualSource,
                             MPI_FLOAT, size, this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int vtkMPICommunicator::Iprobe(
  int source, int tag, int* flag, int* actualSource,
  double* vtkNotUsed(type), int* size)
{
  return CheckForMPIError(
    vtkMPICommunicatorIprobe(source, tag, flag, actualSource,
                             MPI_DOUBLE, size, this->MPIComm->Handle));
}
