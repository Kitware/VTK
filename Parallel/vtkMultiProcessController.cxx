/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiProcessController.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This will be the default.
#include "vtkMultiProcessController.h"
#include "vtkDummyController.h"
#include "vtkThreadedController.h"
#include "vtkToolkits.h"

#ifdef VTK_USE_MPI
#include "vtkMPIController.h"
#endif

#include "vtkCollection.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"

//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkMultiProcessController);

//----------------------------------------------------------------------------

// Helper class to contain the RMI information.  
// A subclass of vtkObject so that I can keep them in a collection.
class VTK_PARALLEL_EXPORT vtkMultiProcessControllerRMI : public vtkObject
{
public:
  static vtkMultiProcessControllerRMI *New(); 
  vtkTypeRevisionMacro(vtkMultiProcessControllerRMI, vtkObject);
  
  int Tag;
  vtkRMIFunctionType Function;
  void *LocalArgument;
  
protected:
  vtkMultiProcessControllerRMI() {};
  vtkMultiProcessControllerRMI(const vtkMultiProcessControllerRMI&);
  void operator=(const vtkMultiProcessControllerRMI&);
};

vtkCxxRevisionMacro(vtkMultiProcessControllerRMI, "1.22");
vtkStandardNewMacro(vtkMultiProcessControllerRMI);

vtkCxxRevisionMacro(vtkMultiProcessController, "1.22");

//----------------------------------------------------------------------------
// An RMI function that will break the "ProcessRMIs" loop.
void vtkMultiProcessControllerBreakRMI(void *localArg, 
                                       void *remoteArg, int remoteArgLength, 
                                       int vtkNotUsed(remoteId))
{
  vtkMultiProcessController *controller;
  remoteArg = remoteArg;
  remoteArgLength = remoteArgLength;
  controller = (vtkMultiProcessController*)(localArg);
  controller->SetBreakFlag(1);
}



//----------------------------------------------------------------------------
vtkMultiProcessController::vtkMultiProcessController()
{
  int i;

  // If some one is using multiple processes, 
  // limit the number of threads to 1
  vtkMultiThreader::SetGlobalDefaultNumberOfThreads(1);
  
  this->LocalProcessId = 0;
  this->NumberOfProcesses = 1;
  this->MaximumNumberOfProcesses = MAX_PROCESSES;
  
  this->RMIs = vtkCollection::New();
  
  this->SingleMethod = 0;
  this->SingleData = 0;   

  this->Communicator = 0;
  this->RMICommunicator = 0;
  
  for ( i = 0; i < VTK_MAX_THREADS; i++ )
    {
    this->MultipleMethod[i] = NULL;
    this->MultipleData[i] = NULL;
    }

  this->BreakFlag = 0;
  this->ForceDeepCopy = 1;

  this->OutputWindow = 0;

  // Define an rmi internally to exit from the processing loop.
  this->AddRMI(vtkMultiProcessControllerBreakRMI, this, BREAK_RMI_TAG);
}

//----------------------------------------------------------------------------
// This is an old comment that I do not know is true:
// (We need to have a "GetNetReferenceCount" to avoid memory leaks.)
vtkMultiProcessController::~vtkMultiProcessController()
{
  if ( this->OutputWindow &&
       (this->OutputWindow == vtkOutputWindow::GetInstance()) )
    {
    vtkOutputWindow::SetInstance(0);
    }

  if (this->OutputWindow)
    {
    this->OutputWindow->Delete();
    }

  this->RMIs->Delete();
  this->RMIs = NULL;
}

 
//----------------------------------------------------------------------------
vtkMultiProcessController *vtkMultiProcessController::New()
{
  const char *temp;
  
  // first check the environment variable
  temp = getenv("VTK_CONTROLLER");
  
#ifdef VTK_USE_MPI
  if ( temp == NULL || !strcmp("MPI",temp))
    {
    return vtkMPIController::New();
    }
#endif

#if defined(VTK_USE_SPROC) || defined(VTK_USE_WIN32_THREADS) || defined(VTK_USE_PTHREADS)
  if ( temp == NULL || !strcmp("Threaded",temp))
    {
    return vtkThreadedController::New();
    }
#endif

  vtkGenericWarningMacro("No valid parallel library was found. "
                         "Creating dummy controller.");
  return vtkDummyController::New();
}



//----------------------------------------------------------------------------
void vtkMultiProcessController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  vtkMultiProcessControllerRMI *rmi;
  vtkIndent nextIndent = indent.GetNextIndent();
  
  os << indent << "MaximumNumberOfProcesses: " 
     << this->MaximumNumberOfProcesses << endl;
  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
  os << indent << "LocalProcessId: " << this->LocalProcessId << endl;
  os << indent << "Break flag: " << (this->BreakFlag ? "(yes)" : "(no)") 
     << endl;
  os << indent << "Force deep copy: " << (this->ForceDeepCopy ? "(yes)" : "(no)") 
     << endl;
  os << indent << "Output window: ";
  if (this->OutputWindow)
    {
    os << endl;
    this->OutputWindow->PrintSelf(os, nextIndent);
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "Communicator: ";
  if (this->Communicator)
    {
    os << endl;
    this->Communicator->PrintSelf(os, nextIndent);
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "RMI communicator: ";
  if (this->RMICommunicator)
    {
    os << endl;
    this->RMICommunicator->PrintSelf(os, nextIndent);
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "RMIs: \n";
  
  this->RMIs->InitTraversal();
  while ( (rmi = (vtkMultiProcessControllerRMI*)(this->RMIs->GetNextItemAsObject())) )
    {
    os << nextIndent << rmi->Tag << endl;
    }
  
}

//----------------------------------------------------------------------------
void vtkMultiProcessController::SetNumberOfProcesses(int num)
{
  if (num == this->NumberOfProcesses)
    {
    return;
    }
  
  if (num < 1 || num > this->MaximumNumberOfProcesses)
    {
    vtkErrorMacro( << num 
          << " is an invalid number of processes try a number from 1 to " 
          << this->NumberOfProcesses );
    return;
    }
  
  this->NumberOfProcesses = num;
  this->Modified();
}


//----------------------------------------------------------------------------
// Set the user defined method that will be run on NumberOfThreads threads
// when SingleMethodExecute is called.
void vtkMultiProcessController::SetSingleMethod( vtkProcessFunctionType f, 
                                                 void *data )
{
  this->SingleMethod = f;
  this->SingleData   = data;
}

//----------------------------------------------------------------------------
// Set one of the user defined methods that will be run on NumberOfProcesses
// processes when MultipleMethodExecute is called. This method should be
// called with index = 0, 1, ..,  NumberOfProcesses-1 to set up all the
// required user defined methods
void vtkMultiProcessController::SetMultipleMethod( int index,
                                 vtkProcessFunctionType f, void *data )
{ 
  // You can only set the method for 0 through NumberOfProcesses-1
  if ( index >= this->NumberOfProcesses )
    {
    vtkErrorMacro( << "Can't set method " << index << 
      " with a processes count of " << this->NumberOfProcesses );
    }
  else
    {
    this->MultipleMethod[index] = f;
    this->MultipleData[index]   = data;
    }
}

//----------------------------------------------------------------------------
int vtkMultiProcessController::RemoveFirstRMI(int tag)
{
  vtkMultiProcessControllerRMI *rmi = NULL;
  this->RMIs->InitTraversal();
  while ( (rmi = (vtkMultiProcessControllerRMI*)(this->RMIs->GetNextItemAsObject())) )
    {
    if (rmi->Tag == tag)
      {
      this->RMIs->RemoveItem(rmi);
      return 1;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkMultiProcessController::AddRMI(vtkRMIFunctionType f, 
                                       void *localArg, int tag)
{
  vtkMultiProcessControllerRMI *rmi = vtkMultiProcessControllerRMI::New();

  rmi->Tag = tag;
  rmi->Function = f;
  rmi->LocalArgument = localArg;
  this->RMIs->AddItem(rmi);
  rmi->Delete();
}

//----------------------------------------------------------------------------
void vtkMultiProcessController::TriggerRMI(int remoteProcessId, 
                                           void *arg, int argLength,
                                           int rmiTag)
{
  int triggerMessage[3];

  // Deal with sending RMI to ourself here for now.
  if (remoteProcessId == this->GetLocalProcessId())
    {
    this->ProcessRMI(remoteProcessId, arg, argLength, rmiTag);
    return;
    }
  
  triggerMessage[0] = rmiTag;
  triggerMessage[1] = argLength;
  
  // It is important for the remote process to know what process invoked it.
  // Multiple processes might try to invoke the method at the same time.
  // The remote method will know where to get additional args.
  triggerMessage[2] = this->GetLocalProcessId();

  this->RMICommunicator->Send(triggerMessage, 3, remoteProcessId, RMI_TAG);
  if (argLength > 0)
    {
    this->RMICommunicator->Send((char*)arg, argLength, remoteProcessId,  
                                 RMI_ARG_TAG);
    } 
}

//----------------------------------------------------------------------------
void vtkMultiProcessController::TriggerBreakRMIs()
{
  int idx, num;

  if (this->GetLocalProcessId() != 0)
    {
    vtkErrorMacro("Break should be triggered from process 0.");
    return;
    }

  num = this->GetNumberOfProcesses();
  for (idx = 1; idx < num; ++idx)
    {
    this->TriggerRMI(idx, NULL, 0, BREAK_RMI_TAG);
    }
}

//----------------------------------------------------------------------------
int vtkMultiProcessController::ProcessRMIs()
{
  return this->ProcessRMIs(1);
}

//----------------------------------------------------------------------------
int vtkMultiProcessController::ProcessRMIs(int reportErrors)
{
  int triggerMessage[3];
  unsigned char *arg = NULL;
  int error = RMI_NO_ERROR;
  
  while (1)
    {
    if (!this->RMICommunicator->Receive(triggerMessage, 3, ANY_SOURCE, RMI_TAG))
      {
      if (reportErrors)
        {
        vtkErrorMacro("Could not receive RMI trigger message.");
        }
      error = RMI_TAG_ERROR;
      break;
      }
    if (triggerMessage[1] > 0)
      {
      arg = new unsigned char[triggerMessage[1]];
      if (!this->RMICommunicator->Receive((char*)(arg), triggerMessage[1], 
                                          triggerMessage[2], RMI_ARG_TAG))
        {
        if (reportErrors)
          {
          vtkErrorMacro("Could not receive RMI argument.");
          }
        error = RMI_ARG_ERROR;
        break;
        }
      }
    this->ProcessRMI(triggerMessage[2], arg, triggerMessage[1], 
                     triggerMessage[0]);
    if (arg)
      {
      delete [] arg;
      arg = NULL;
      }
    
    // check for break
    if (this->BreakFlag)
      {
      this->BreakFlag = 0;
      return error;
      }
    }

  return error;
}


//----------------------------------------------------------------------------
void vtkMultiProcessController::ProcessRMI(int remoteProcessId, 
                                           void *arg, int argLength,
                                           int rmiTag)
{
  vtkMultiProcessControllerRMI *rmi = NULL;
  int found = 0;

  // find the rmi
  this->RMIs->InitTraversal();
  while ( !found &&
   (rmi = (vtkMultiProcessControllerRMI*)(this->RMIs->GetNextItemAsObject())) )
    {
    if (rmi->Tag == rmiTag)
      {
      found = 1;
      }
    }
  
  if ( ! found)
    {
    vtkErrorMacro("Process " << this->GetLocalProcessId() << 
                  " Could not find RMI with tag " << rmiTag);
    }
  else
    {
    if ( rmi->Function )
      {
      (*rmi->Function)(rmi->LocalArgument, arg, argLength, remoteProcessId);
      }     
    }
}


//============================================================================
// The intent is to give access to a processes controller from a static method.

vtkMultiProcessController *VTK_GLOBAL_MULTI_PROCESS_CONTROLLER = NULL;
//----------------------------------------------------------------------------
vtkMultiProcessController *vtkMultiProcessController::GetGlobalController()
{
  if (VTK_GLOBAL_MULTI_PROCESS_CONTROLLER == NULL)
    {
    return NULL;
    }
  
  return VTK_GLOBAL_MULTI_PROCESS_CONTROLLER->GetLocalController();
}
//----------------------------------------------------------------------------
// This can be overridden in the subclass to translate controllers.
// (Threads have to share a global variable.)
vtkMultiProcessController *vtkMultiProcessController::GetLocalController()
{
  return VTK_GLOBAL_MULTI_PROCESS_CONTROLLER;
}
//----------------------------------------------------------------------------
// This can be overridden in the subclass to translate controllers.
// (Threads have to share a global variable.)
void vtkMultiProcessController::SetGlobalController(
                                   vtkMultiProcessController *controller)
{
  VTK_GLOBAL_MULTI_PROCESS_CONTROLLER = controller;
}


