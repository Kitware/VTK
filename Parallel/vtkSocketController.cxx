/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkSocketController.cxx
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
#include "vtkSocketController.h"
#include "vtkObjectFactory.h"

#ifdef _WIN32
#define WSA_VERSION MAKEWORD(1,1)
#define vtkCloseSocketMacro(sock) (closesocket(sock))
#else
#define vtkCloseSocketMacro(sock) (close(sock))
#endif

int vtkSocketController::Initialized = 0;

//------------------------------------------------------------------------------
vtkSocketController* vtkSocketController::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSocketController");
  if(ret)
    {
    return (vtkSocketController*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSocketController;
}

//----------------------------------------------------------------------------
vtkSocketController::vtkSocketController()
{
  this->NumberOfProcesses = 2;
  this->Communicator = vtkSocketCommunicator::New();
  this->RMICommunicator = this->Communicator;
}

//----------------------------------------------------------------------------
vtkSocketController::~vtkSocketController()
{
  this->Communicator->Delete();
  this->Communicator = this->RMICommunicator = 0;
}

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

void vtkSocketController::SetNumberOfProcesses(int num)
{
  vtkErrorMacro("Can not change the number of processes.");
  return;
}

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
  vtkMultiProcessController::PrintSelf(os,indent);
}


