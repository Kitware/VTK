/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPICommunicator.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkMPIGroup.h"
#include "vtkObjectFactory.h"

vtkMPICommunicator* vtkMPICommunicator::WorldCommunicator = 0;

vtkMPICommunicator* vtkMPICommunicator::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMPICommunicator");
  if(ret)
    {
    return (vtkMPICommunicator*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMPICommunicator;
}

// Return the world communicator (i.e. MPI_COMM_WORLD).
// Create one if necessary (singleton).
vtkMPICommunicator* vtkMPICommunicator::GetWorldCommunicator()
{
  int err, size;

  if (vtkMPICommunicator::WorldCommunicator == 0)
    {
    vtkMPICommunicator* comm = vtkMPICommunicator::New();
    comm->Handle = new MPI_Comm;
    *(comm->Handle) = MPI_COMM_WORLD;
    comm->Group = vtkMPIGroup::New();
    if ( (err = MPI_Comm_size(MPI_COMM_WORLD, &size)) != MPI_SUCCESS  )
      {
      char *msg = vtkMPIController::ErrorString(err);
      vtkGenericWarningMacro("MPI error occured: " << msg);
      delete[] msg;
      delete comm->Handle;
      comm->Handle = 0;
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

void vtkMPICommunicator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkCommunicator::PrintSelf(os,indent);
  os << indent << "Group: ";
  if (this->Group)
    {
    os << endl;
    this->Group->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(nil)";
    os << endl;
    }
  os << indent << "MPI Communicator handler: " ;
  if (this->Handle)
    {
    os << *(this->Handle);
    }
  else
    {
    os << "(nil)";
    }
  os << endl;
  os << indent << "Initialized: " << (this->Initialized ? "(yes)" : "(no)")
     << endl;
  os << indent << "Keep handle: " << (this->KeepHandle ? "(yes)" : "(no)")
     << endl;
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
      os << "(nil)";
      }
    os << endl;
    }
  return;
}

vtkMPICommunicator::vtkMPICommunicator()
{
  this->Handle = 0;
  this->Group = 0;
  this->Initialized = 0;
  this->KeepHandle = 0;
}

vtkMPICommunicator::~vtkMPICommunicator()
{
  // Free the handle if required and asked for.
  if (this->Handle && !this->KeepHandle )
    {
    if (*(this->Handle) != MPI_COMM_NULL)
      {
      MPI_Comm_free(this->Handle);
      }
    }
  delete this->Handle;
  this->SetGroup(0);
}

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
  if ( (err = MPI_Comm_group(*(mpiComm->Handle), &superGroup))
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

  this->Handle = new MPI_Comm;
  // Create the communicator from the group
  if ( (err  = MPI_Comm_create(*(mpiComm->Handle), subGroup, 
			       this->Handle) )
       != MPI_SUCCESS )
    {
    MPI_Group_free(&subGroup);
    delete this->Handle;
    this->Handle = 0;

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

// Start the copying process  
void vtkMPICommunicator::InitializeCopy(vtkMPICommunicator* source)
{
  if(!source)
    {
    return;
    }

  this->SetGroup(0);
  this->Group = vtkMPIGroup::New();
  this->Group->CopyFrom(source->Group);

  if (this->Handle && !this->KeepHandle)
    {
      MPI_Comm_free(this->Handle);
    }
  delete this->Handle;
  this->Handle = 0;

  this->Initialized = source->Initialized;
  this->Modified();
}

// Copy the MPI handle
void vtkMPICommunicator::CopyFrom(vtkMPICommunicator* source)
{
  this->InitializeCopy(source);

  if (source->Handle)
    {
    this->KeepHandleOn();
    this->Handle = new MPI_Comm;
    *(this->Handle) = *(source->Handle);
    }
}

// Duplicate the MPI handle
void vtkMPICommunicator::Duplicate(vtkMPICommunicator* source)
{
  this->InitializeCopy(source);

  this->KeepHandleOff();

  if (source->Handle)
    {
    this->Handle = new MPI_Comm;
    int err;
    if ( (err = MPI_Comm_dup(*(source->Handle), this->Handle))
	  != MPI_SUCCESS ) 
      {
      char *msg = vtkMPIController::ErrorString(err);
      vtkErrorMacro("MPI error occured: " << msg);
      delete[] msg;
      }                      
    }
}

//----------------------------------------------------------------------------
int vtkMPICommunicator::Send(int* data, int length, int remoteProcessId, 
			    int tag)
{
  int err = 
    MPI_Send(data, length, MPI_INT, remoteProcessId, tag, 
	     *(this->Handle));
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }

}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Send(unsigned long* data, int length, 
			     int remoteProcessId, int tag)
{
  int err = 
    MPI_Send(data, length, MPI_UNSIGNED_LONG, remoteProcessId, tag, 
	     *(this->Handle));
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Send(char* data, int length, 
			     int remoteProcessId, int tag)
{
  int err = 
    MPI_Send(data, length, MPI_CHAR, remoteProcessId, tag, 
	     *(this->Handle));
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Send(float* data, int length, 
			     int remoteProcessId, int tag)
{
  int err = 
    MPI_Send(data, length, MPI_FLOAT, remoteProcessId, tag, 
	     *(this->Handle));
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }
}

//----------------------------------------------------------------------------
int vtkMPICommunicator::Send(double* data, int length, 
			     int remoteProcessId, int tag)
{
  int err = 
    MPI_Send(data, length, MPI_DOUBLE, remoteProcessId, tag, 
	     *(this->Handle));
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }
}


//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockSend(int* data, int length, 
				    int remoteProcessId, int tag,
				    Request& req)
{
  int err = 
    MPI_Isend(data, length, MPI_INT, remoteProcessId, tag, 
	      *(this->Handle), &req.Req);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }

}
//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockSend(unsigned long* data, int length, 
				    int remoteProcessId, int tag,
				    Request& req)
{
  int err = 
    MPI_Isend(data, length, MPI_UNSIGNED_LONG, remoteProcessId, tag, 
	      *(this->Handle), &req.Req);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockSend(char* data, int length, 
				    int remoteProcessId, int tag, Request& req)
{
  int err = 
    MPI_Isend(data, length, MPI_CHAR, remoteProcessId, tag, 
	     *(this->Handle), &req.Req);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockSend(float* data, int length, 
				    int remoteProcessId, int tag, Request& req)
{
  int err = 
    MPI_Isend(data, length, MPI_FLOAT, remoteProcessId, tag, 
	     *(this->Handle), &req.Req);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }
}



//----------------------------------------------------------------------------
int vtkMPICommunicator::Receive(int* data, int length, 
				int remoteProcessId, int tag)
{
  MPI_Status status; 

  if (remoteProcessId == vtkMultiProcessController::ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  int err = 
    MPI_Recv(data, length, MPI_INT, remoteProcessId, tag, 
	     *(this->Handle),
	     &status);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }

}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Receive(unsigned long* data, int length, 
				int remoteProcessId, int tag)
{
  MPI_Status status; 

  if (remoteProcessId == vtkMultiProcessController::ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  int err = 
    MPI_Recv(data, length, MPI_UNSIGNED_LONG, remoteProcessId, 
	     tag, *(this->Handle), &status);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Receive(char* data, int length, 
				int remoteProcessId, int tag)
{
  MPI_Status status; 

  if (remoteProcessId == vtkMultiProcessController::ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  int err = 
    MPI_Recv(data, length, MPI_CHAR, remoteProcessId, tag, 
	     *(this->Handle),
	     &status);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::Receive(float* data, int length, 
				int remoteProcessId, int tag)
{
  MPI_Status status; 

  if (remoteProcessId == vtkMultiProcessController::ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  int err = 
    MPI_Recv(data, length, MPI_FLOAT, remoteProcessId, tag, 
	     *(this->Handle),
	     &status);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }
}

//----------------------------------------------------------------------------
int vtkMPICommunicator::Receive(double* data, int length, 
				int remoteProcessId, int tag)
{
  MPI_Status status; 

  if (remoteProcessId == vtkMultiProcessController::ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  int err = 
    MPI_Recv(data, length, MPI_DOUBLE, remoteProcessId, tag, 
	     *(this->Handle),
	     &status);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }
}

//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockReceive(int* data, int length, 
				       int remoteProcessId, int tag,
				       Request& req)
{

  if (remoteProcessId == vtkMultiProcessController::ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  int err = 
    MPI_Irecv(data, length, MPI_INT, remoteProcessId, tag, 
	     *(this->Handle), &req.Req);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }

}
//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockReceive(unsigned long* data, int length, 
				       int remoteProcessId, int tag,
				       Request& req)
{

  if (remoteProcessId == vtkMultiProcessController::ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  int err = 
    MPI_Irecv(data, length, MPI_UNSIGNED_LONG, remoteProcessId, 
	      tag, *(this->Handle), &req.Req);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockReceive(char* data, int length, 
				       int remoteProcessId, int tag,
				       Request& req)
{

  if (remoteProcessId == vtkMultiProcessController::ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  int err = 
    MPI_Irecv(data, length, MPI_CHAR, remoteProcessId, tag, 
	      *(this->Handle),&req.Req);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }
}
//----------------------------------------------------------------------------
int vtkMPICommunicator::NoBlockReceive(float* data, int length, 
				       int remoteProcessId, int tag,
				       Request& req)
{

  if (remoteProcessId == vtkMultiProcessController::ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  int err = 
    MPI_Irecv(data, length, MPI_FLOAT, remoteProcessId, tag, 
	     *(this->Handle), &req.Req);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }
}

int vtkMPICommunicator::Request::Test()
{
  MPI_Status status;
  int retVal;

  int err = MPI_Test(&this->Req, &retVal, &status);
  
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

void vtkMPICommunicator::Request::Wait()
{
  MPI_Status status;

  int err = MPI_Wait(&this->Req, &status);
  
  if ( err != MPI_SUCCESS )
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkGenericWarningMacro("MPI error occured: " << msg);
    delete[] msg;
    }
}

void vtkMPICommunicator::Request::Cancel()
{
  int err = MPI_Cancel(&this->Req);
  
  if ( err != MPI_SUCCESS )
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkGenericWarningMacro("MPI error occured: " << msg);
    delete[] msg;
    }
}
