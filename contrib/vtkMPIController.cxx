/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkMPIController.cxx
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
#include "vtkMPIController.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"

int vtkMPIController::Initialized = 0;

// Output window which prints out the process id
// with the error or warning messages
class VTK_EXPORT vtkMPIOutputWindow : public vtkOutputWindow
{
public:
  vtkTypeMacro(vtkMPIOutputWindow,vtkOutputWindow);

  void DisplayText(const char* t)
  {
    if (this->Controller)
      {
      cout << "Process id: " << this->Controller->GetLocalProcessId()
	   << " >> ";
      }
    cout << t;
  }

  vtkMPIOutputWindow()
  {
    vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMPIOutputWindow");
    if (ret)
      ret->Delete();
    this->Controller = 0;
  }

  friend vtkMPIController;

protected:

  vtkMPIController* Controller;
};

void vtkMPIController::CreateOutputWindow()
{
  this->OutputWindow = new vtkMPIOutputWindow;
  this->OutputWindow->Controller = this;
  vtkOutputWindow::SetInstance(this->OutputWindow);
}

//------------------------------------------------------------------------------
vtkMPIController* vtkMPIController::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMPIController");
  if(ret)
    {
    return (vtkMPIController*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMPIController;
}

//----------------------------------------------------------------------------
vtkMPIController::vtkMPIController()
{
  // If MPI was already initialized obtain rank and size.
  if (vtkMPIController::Initialized)
    {
    this->InitializeNumberOfProcesses();
    }
  this->OutputWindow = 0;
}

//----------------------------------------------------------------------------
vtkMPIController::~vtkMPIController()
{
  if ( this->OutputWindow == vtkOutputWindow::GetInstance() )
    vtkOutputWindow::SetInstance(0);
  if (this->OutputWindow)
    this->OutputWindow->Delete();
}

//----------------------------------------------------------------------------
void vtkMPIController::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMultiProcessController::PrintSelf(os,indent);
}

void vtkMPIController::InitializeNumberOfProcesses()
{
  MPI_Comm_size(MPI_COMM_WORLD, &(this->MaximumNumberOfProcesses));
  if (this->MaximumNumberOfProcesses > VTK_MP_CONTROLLER_MAX_PROCESSES)
    {
    vtkWarningMacro("Maximum of " << VTK_MP_CONTROLLER_MAX_PROCESSES);
    this->MaximumNumberOfProcesses = VTK_MP_CONTROLLER_MAX_PROCESSES;
    }
  
  this->NumberOfProcesses = this->MaximumNumberOfProcesses;
  
  MPI_Comm_rank(MPI_COMM_WORLD, &(this->LocalProcessId));  

}
//----------------------------------------------------------------------------
void vtkMPIController::Initialize(int* argc, char*** argv)
{
  if (vtkMPIController::Initialized)
    {
    vtkErrorMacro("Already initialized.");
    return;
    }
  
  // Can be done once in the program.
  vtkMPIController::Initialized = 1;
  MPI_Init(argc, argv);
  this->InitializeNumberOfProcesses();

  this->Modified();
}

void vtkMPIController::Finalize()
{
  if (vtkMPIController::Initialized)
    { 
    MPI_Finalize();
    vtkMPIController::Initialized = 0;
    }  
  
}

  
//----------------------------------------------------------------------------
// Execute the method set as the SingleMethod on NumberOfThreads threads.
void vtkMPIController::SingleMethodExecute()
{
  if(!vtkMPIController::Initialized)
    {
    vtkErrorMacro("MPI has to be initialized first.");
    return;
    }

  if (this->LocalProcessId < this->NumberOfProcesses)
    {
    if (this->SingleMethod)
      {
      this->SetGlobalController(this);
      (this->SingleMethod)(this, this->SingleData);
      }
    else
      {
      vtkErrorMacro("SingleMethod not set.");
      }
    }
}


//----------------------------------------------------------------------------
// Execute the methods set as the MultipleMethods.
void vtkMPIController::MultipleMethodExecute()
{
  if(!vtkMPIController::Initialized)
    {
    vtkErrorMacro("MPI has to be initialized first.");
    return;
    }

  int i = this->LocalProcessId;
  
  if (this->LocalProcessId < this->NumberOfProcesses)
    {
    if (this->MultipleMethod[i])
      {
      this->SetGlobalController(this);
      (this->MultipleMethod[i])(this, this->MultipleData[i]);
      }
    else
      {
      vtkErrorMacro("MultipleMethod " << i << " not set.");
      }
    }
}


//----------------------------------------------------------------------------
int vtkMPIController::Send(int *data, int length, int remoteProcessId, 
			    int tag)
{
  int err = 
    MPI_Send(data, length, MPI_INT, remoteProcessId, tag, MPI_COMM_WORLD);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    vtkErrorMacro("MPI error: " << err);
    return 0;
    }

}
//----------------------------------------------------------------------------
int vtkMPIController::Send(unsigned long *data, int length, 
			   int remoteProcessId, int tag)
{
  int err = 
    MPI_Send(data, length, MPI_UNSIGNED_LONG, remoteProcessId, tag, 
	     MPI_COMM_WORLD);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    vtkErrorMacro("MPI error: " << err);
    return 0;
    }
}
//----------------------------------------------------------------------------
int vtkMPIController::Send(char *data, int length, 
			   int remoteProcessId, int tag)
{
  int err = 
    MPI_Send(data, length, MPI_CHAR, remoteProcessId, tag, MPI_COMM_WORLD);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    vtkErrorMacro("MPI error: " << err);
    return 0;
    }
}
//----------------------------------------------------------------------------
int vtkMPIController::Send(float *data, int length, 
			   int remoteProcessId, int tag)
{
  int err = 
    MPI_Send(data, length, MPI_FLOAT, remoteProcessId, tag, MPI_COMM_WORLD);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    vtkErrorMacro("MPI error: " << err);
    return 0;
    }
}



//----------------------------------------------------------------------------
int vtkMPIController::Receive(int *data, int length, int remoteProcessId, 
			      int tag)
{
  MPI_Status status; 

  if (remoteProcessId == VTK_MP_CONTROLLER_ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  int err = 
    MPI_Recv(data, length, MPI_INT, remoteProcessId, tag, MPI_COMM_WORLD,
	     &status);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    vtkErrorMacro("MPI error: " << err);
    return 0;
    }

}
//----------------------------------------------------------------------------
int vtkMPIController::Receive(unsigned long *data, int length, 
			      int remoteProcessId, int tag)
{
  MPI_Status status; 

  if (remoteProcessId == VTK_MP_CONTROLLER_ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  int err = 
    MPI_Recv(data, length, MPI_UNSIGNED_LONG, remoteProcessId, 
	     tag, MPI_COMM_WORLD, &status);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    vtkErrorMacro("MPI error: " << err);
    return 0;
    }
}
//----------------------------------------------------------------------------
int vtkMPIController::Receive(char *data, int length, 
			      int remoteProcessId, int tag)
{
  MPI_Status status; 

  if (remoteProcessId == VTK_MP_CONTROLLER_ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  int err = 
    MPI_Recv(data, length, MPI_CHAR, remoteProcessId, tag, MPI_COMM_WORLD,
	   &status);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    vtkErrorMacro("MPI error: " << err);
    return 0;
    }
}
//----------------------------------------------------------------------------
int vtkMPIController::Receive(float *data, int length, 
			      int remoteProcessId, int tag)
{
  MPI_Status status; 

  if (remoteProcessId == VTK_MP_CONTROLLER_ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  int err = 
    MPI_Recv(data, length, MPI_FLOAT, remoteProcessId, tag, MPI_COMM_WORLD,
	     &status);
  if ( err == MPI_SUCCESS )
    {
    return 1;
    }
  else
    {
    vtkErrorMacro("MPI error: " << err);
    return 0;
    }
}











