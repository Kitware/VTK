/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPIController.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMPIController.h"

#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"

int vtkMPIController::Initialized = 0;
char vtkMPIController::ProcessorName[MPI_MAX_PROCESSOR_NAME] = "";

// Output window which prints out the process id
// with the error or warning messages
class VTK_PARALLEL_EXPORT vtkMPIOutputWindow : public vtkOutputWindow
{
public:
  vtkTypeRevisionMacro(vtkMPIOutputWindow,vtkOutputWindow);

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

  friend class vtkMPIController;

protected:

  vtkMPIController* Controller;
  vtkMPIOutputWindow(const vtkMPIOutputWindow&);
  void operator=(const vtkMPIOutputWindow&);

};

void vtkMPIController::CreateOutputWindow()
{
  vtkMPIOutputWindow* window = new vtkMPIOutputWindow;
  window->Controller = this;
  this->OutputWindow = window;
  vtkOutputWindow::SetInstance(this->OutputWindow);
}

vtkCxxRevisionMacro(vtkMPIOutputWindow, "1.13");

vtkCxxRevisionMacro(vtkMPIController, "1.13");
vtkStandardNewMacro(vtkMPIController);

//----------------------------------------------------------------------------
vtkMPIController::vtkMPIController()
{
  // If MPI was already initialized obtain rank and size.
  if (vtkMPIController::Initialized)
    {
    this->InitializeCommunicator(vtkMPICommunicator::GetWorldCommunicator());
    // Copy vtkMPIController::WorldRMICommunicataor which is created when
    // MPI is initialized
    vtkMPICommunicator* comm = vtkMPICommunicator::New();
    comm->CopyFrom(vtkMPIController::WorldRMICommunicator);
    this->RMICommunicator = comm;
    }

  this->OutputWindow = 0;
}

//----------------------------------------------------------------------------
vtkMPIController::~vtkMPIController()
{
  this->SetCommunicator(0);
  if (this->RMICommunicator)
    {
    this->RMICommunicator->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkMPIController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Initialized: " << ( vtkMPIController::Initialized ? "(yes)" : "(no)" ) << endl;
}

// Set the number of processes and maximum number of processes
// to the size obtained from MPI.
int vtkMPIController::InitializeNumberOfProcesses()
{
  int err;

  this->Modified();

  vtkMPICommunicator* comm = (vtkMPICommunicator*)this->Communicator;
  if ( (err = MPI_Comm_size(*(comm->Handle), 
                            &(this->MaximumNumberOfProcesses))) 
       != MPI_SUCCESS  )
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }

  if (this->MaximumNumberOfProcesses > MAX_PROCESSES)
    {
    vtkWarningMacro("Maximum of " << MAX_PROCESSES);
    this->MaximumNumberOfProcesses = MAX_PROCESSES;
    }
  
  this->NumberOfProcesses = this->MaximumNumberOfProcesses;
  
  if ( (err = MPI_Comm_rank(*(comm->Handle),&(this->LocalProcessId))) 
       != MPI_SUCCESS)
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }
  return 1;
}

vtkMPICommunicator* vtkMPIController::WorldRMICommunicator=0;

//----------------------------------------------------------------------------
void vtkMPIController::Initialize(int* argc, char*** argv)
{
  if (vtkMPIController::Initialized)
    {
    vtkWarningMacro("Already initialized.");
    return;
    }
  
  // Can be done once in the program.
  vtkMPIController::Initialized = 1;
  MPI_Init(argc, argv);
  this->InitializeCommunicator(vtkMPICommunicator::GetWorldCommunicator());
  this->InitializeNumberOfProcesses();

  int tmp;
  MPI_Get_processor_name(ProcessorName, &tmp);
  // Make a copy of MPI_COMM_WORLD creating a new context.
  // This is used in the creating of the communicators after
  // Initialize() has been called. It has to be done here
  // because for this to work, all processes have to call
  // MPI_Comm_dup and this is the only method which is
  // guaranteed to be called by all processes.
  vtkMPIController::WorldRMICommunicator = vtkMPICommunicator::New();
  vtkMPIController::WorldRMICommunicator->Duplicate((vtkMPICommunicator*)this->Communicator);
  this->RMICommunicator = vtkMPIController::WorldRMICommunicator;
  // Since we use Delete to get rid of the reference, we should use NULL to register.
  this->RMICommunicator->Register(NULL);

  this->Modified();
}

const char* vtkMPIController::GetProcessorName()
{
  return ProcessorName;
}

// Good-bye world
// There should be no MPI calls after this.
// (Except maybe MPI_XXX_free())
void vtkMPIController::Finalize()
{
  if (vtkMPIController::Initialized)
    { 
    vtkMPIController::WorldRMICommunicator->Delete();
    vtkMPIController::WorldRMICommunicator = 0;
    vtkMPICommunicator::WorldCommunicator->Delete();
    MPI_Finalize();
    vtkMPIController::Initialized = 0;
    this->Modified();
    }  
  
}

// Called by SetCommunicator and constructor. It frees but does 
// not set RMIHandle (which should not be set by using MPI_Comm_dup
// during construction).
void vtkMPIController::InitializeCommunicator(vtkMPICommunicator* comm)
{
  if (this->Communicator != comm) 
    { 
    if (this->Communicator != 0) 
      { 
      this->Communicator->UnRegister(this); 
      }
    this->Communicator = comm; 
    if (this->Communicator != 0) 
      { 
      this->Communicator->Register(this); 
      } 

    vtkMPICommunicator* comm = (vtkMPICommunicator*)this->Communicator;
    if (comm && comm->Handle)
      {
      this->InitializeNumberOfProcesses();
      }
    this->Modified(); 
    }  


}

// Delete the previous RMI communicator and creates a new one
// by duplicating the user communicator.
void vtkMPIController::InitializeRMICommunicator()
{
  if ( this->RMICommunicator )
    {
    this->RMICommunicator->Delete();
    this->RMICommunicator = 0;
    }
  if (this->Communicator)
    {
    this->RMICommunicator = vtkMPICommunicator::New();
    ((vtkMPICommunicator*)this->RMICommunicator)->Duplicate((vtkMPICommunicator*)this->Communicator);
    }
}

void vtkMPIController::SetCommunicator(vtkMPICommunicator* comm)
{
  this->InitializeCommunicator(comm);
  this->InitializeRMICommunicator();
}

  
void vtkMPIController::Barrier()
{
  vtkMPICommunicator* comm = (vtkMPICommunicator*)this->Communicator;
  int err;
  if ( (err = MPI_Barrier(*(comm->Handle)) ) 
       != MPI_SUCCESS ) 
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    }
}

//----------------------------------------------------------------------------
// Execute the method set as the SingleMethod.
void vtkMPIController::SingleMethodExecute()
{
  if(!vtkMPIController::Initialized)
    {
    vtkWarningMacro("MPI has to be initialized first.");
    return;
    }

  if (this->LocalProcessId < this->NumberOfProcesses)
    {
    if (this->SingleMethod)
      {
      vtkMultiProcessController::SetGlobalController(this);
      (this->SingleMethod)(this, this->SingleData);
      }
    else
      {
      vtkWarningMacro("SingleMethod not set.");
      }
    }
}

//----------------------------------------------------------------------------
// Execute the methods set as the MultipleMethods.
void vtkMPIController::MultipleMethodExecute()
{
  if(!vtkMPIController::Initialized)
    {
    vtkWarningMacro("MPI has to be initialized first.");
    return;
    }

  int i = this->LocalProcessId;
  
  if (this->LocalProcessId < this->NumberOfProcesses)
    {
    if (this->MultipleMethod[i])
      {
      vtkMultiProcessController::SetGlobalController(this);
      (this->MultipleMethod[i])(this, this->MultipleData[i]);
      }
    else
      {
      vtkWarningMacro("MultipleMethod " << i << " not set.");
      }
    }
}

char* vtkMPIController::ErrorString(int err)
{
  char* buffer = new char[MPI_MAX_ERROR_STRING];
  int resLen;
  MPI_Error_string(err, buffer, &resLen);
  return buffer;
}

