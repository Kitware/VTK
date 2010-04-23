/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSocketController.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSocketController.h"

#include "vtkObjectFactory.h"
#include "vtkProcessGroup.h"
#include "vtkSocketCommunicator.h"

#if defined(_WIN32) && !defined(__CYGWIN__)
# define VTK_WINDOWS_FULL
# include "vtkWindows.h"
# define WSA_VERSION MAKEWORD(1,1)
#endif

int vtkSocketController::Initialized = 0;

vtkStandardNewMacro(vtkSocketController);

//----------------------------------------------------------------------------
vtkSocketController::vtkSocketController()
{
  this->Communicator = vtkSocketCommunicator::New();
  this->RMICommunicator = this->Communicator;
}

//----------------------------------------------------------------------------
vtkSocketController::~vtkSocketController()
{
  this->Communicator->Delete();
  this->Communicator = this->RMICommunicator = 0;
}

//----------------------------------------------------------------------------
void vtkSocketController::Initialize(int* , char***)
{
  if (vtkSocketController::Initialized)
    {
    vtkWarningMacro("Already initialized.");
    return;
    }

#if defined(_WIN32) && !defined(__CYGWIN__)
  WSAData wsaData;  
  if (WSAStartup(WSA_VERSION, &wsaData))
    {
    vtkErrorMacro("Could not initialize sockets !");
    }
#endif
  vtkSocketController::Initialized = 1;

}

//----------------------------------------------------------------------------
void vtkSocketController::SetCommunicator(vtkSocketCommunicator* comm)
{
  if (comm == this->Communicator)
    {
    return;
    }
  if (this->Communicator)
    {
    this->Communicator->UnRegister(this);
    }
  this->Communicator = comm;
  this->RMICommunicator = comm;
  if (comm)
    {
    comm->Register(this);
    }
}

//----------------------------------------------------------------------------
void vtkSocketController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


//----------------------------------------------------------------------------
int vtkSocketController::WaitForConnection(int port)
{ 
  return vtkSocketCommunicator::SafeDownCast(this->Communicator)->
    WaitForConnection(port); 
}

//----------------------------------------------------------------------------
void vtkSocketController::CloseConnection()
{ 
  vtkSocketCommunicator::SafeDownCast(this->Communicator)->
    CloseConnection(); 
}

//----------------------------------------------------------------------------
int vtkSocketController::ConnectTo( char* hostName, int port )
{ 
  return vtkSocketCommunicator::SafeDownCast(this->Communicator)->
    ConnectTo(hostName, port); 
}

//----------------------------------------------------------------------------
int vtkSocketController::GetSwapBytesInReceivedData()
{
  return vtkSocketCommunicator::SafeDownCast(this->Communicator)->
    GetSwapBytesInReceivedData();
}

//-----------------------------------------------------------------------------
vtkMultiProcessController *vtkSocketController::CreateCompliantController()
{
  vtkProcessGroup *group = vtkProcessGroup::New();
  group->Initialize(this->Communicator);
  group->RemoveAllProcessIds();

  // This hack creates sub controllers with differing orders of the processes
  // that will map the ids to be unique on each process.
  if (vtkSocketCommunicator::SafeDownCast(this->Communicator)->GetIsServer())
    {
    group->AddProcessId(1);
    group->AddProcessId(0);
    }
  else
    {
    group->AddProcessId(0);
    group->AddProcessId(1);
    }

  vtkMultiProcessController *compliantController
    = this->CreateSubController(group);

  group->Delete();

  return compliantController;
}
