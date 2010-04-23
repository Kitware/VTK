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

#include "vtkByteSwap.h"
#include "vtkCollection.h"
#include "vtkDummyController.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkProcessGroup.h"
#include "vtkSubCommunicator.h"
#include "vtkToolkits.h"
#include "vtkProcess.h"

#ifdef VTK_USE_MPI
#include "vtkMPIController.h"
#endif

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <vtkstd/list>
#include <vtkstd/vector>
#include <vtksys/hash_map.hxx>

//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkMultiProcessController);

//-----------------------------------------------------------------------------
// Stores internal members that cannot or should not be exposed in the header
// file (for example, because they use templated types).
class vtkMultiProcessController::vtkInternal
{
public:
  vtksys::hash_map<int, vtkProcessFunctionType> MultipleMethod;
  vtksys::hash_map<int, void *> MultipleData;
};

//----------------------------------------------------------------------------
// Helper class to contain the RMI information.  
// A subclass of vtkObject so that I can keep them in a collection.
class VTK_PARALLEL_EXPORT vtkMultiProcessControllerRMI : public vtkObject
{
public:
  static vtkMultiProcessControllerRMI *New(); 
  vtkTypeMacro(vtkMultiProcessControllerRMI, vtkObject);
  
  int Tag;
  vtkRMIFunctionType Function;
  void *LocalArgument;

  unsigned long Id;
  
protected:
  vtkMultiProcessControllerRMI() {};
  vtkMultiProcessControllerRMI(const vtkMultiProcessControllerRMI&);
  void operator=(const vtkMultiProcessControllerRMI&);
};

vtkStandardNewMacro(vtkMultiProcessControllerRMI);


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

// ----------------------------------------------------------------------------
// Single method used when launching a single process.
static void vtkMultiProcessControllerRun(vtkMultiProcessController *c,
                                         void *arg)
{
  vtkProcess *p=reinterpret_cast<vtkProcess *>(arg);
  p->SetController(c);
  p->Execute();
}

//----------------------------------------------------------------------------
vtkMultiProcessController::vtkMultiProcessController()
{
  this->Internal = new vtkInternal;

  this->RMICount = 1;
  
  this->RMIs = vtkCollection::New();
  
  this->SingleMethod = 0;
  this->SingleData = 0;   

  this->Communicator = 0;
  this->RMICommunicator = 0;

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

  delete this->Internal;
}

 
//----------------------------------------------------------------------------
void vtkMultiProcessController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  vtkMultiProcessControllerRMI *rmi;
  vtkIndent nextIndent = indent.GetNextIndent();
  
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
  if(this->Communicator)
    {
    this->Communicator->SetNumberOfProcesses(num);
    }
  else
    {
    vtkErrorMacro("Communicator not set.");
    }
}

//----------------------------------------------------------------------------
int vtkMultiProcessController::GetNumberOfProcesses()
{
  if(this->Communicator)
    {
    return this->Communicator->GetNumberOfProcesses();
    }
  else
    {
    vtkErrorMacro("Communicator not set.");
    return 0;
    }
}

//----------------------------------------------------------------------------
int vtkMultiProcessController::GetLocalProcessId()
{
  if(this->Communicator)
    {
    return this->Communicator->GetLocalProcessId();
    }
  else
    {
    vtkErrorMacro("Communicator not set.");
    return -1;
    }
}

//----------------------------------------------------------------------------
void vtkMultiProcessController::SetSingleMethod( vtkProcessFunctionType f, 
                                                 void *data )
{
  this->SingleMethod = f;
  this->SingleData   = data;
}

// ----------------------------------------------------------------------------
void vtkMultiProcessController::SetSingleProcessObject(vtkProcess *p)
{
  this->SetSingleMethod(vtkMultiProcessControllerRun,p);
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
  if ( index >= this->GetNumberOfProcesses() )
    {
    vtkErrorMacro( << "Can't set method " << index << 
      " with a processes count of " << this->GetNumberOfProcesses() );
    }
  else
    {
    this->Internal->MultipleMethod[index] = f;
    this->Internal->MultipleData[index]   = data;
    }
}

//-----------------------------------------------------------------------------
void vtkMultiProcessController::GetMultipleMethod(int index,
                                                  vtkProcessFunctionType &func,
                                                  void *&data)
{
  if (   this->Internal->MultipleMethod.find(index)
      != this->Internal->MultipleMethod.end() )
    {
    func = this->Internal->MultipleMethod[index];
    data = this->Internal->MultipleData[index];
    }
  else
    {
    func = NULL;
    data = NULL;
    }
}

//-----------------------------------------------------------------------------
vtkMultiProcessController *vtkMultiProcessController::CreateSubController(
                                                         vtkProcessGroup *group)
{
  if (group->GetCommunicator() != this->Communicator)
    {
    vtkErrorMacro(<< "Invalid group for creating a sub controller.");
    return NULL;
    }

  if (group->FindProcessId(this->GetLocalProcessId()) < 0)
    {
    // The group does not contain this process.  Just return NULL.
    return NULL;
    }

  vtkSubCommunicator *subcomm = vtkSubCommunicator::New();
  subcomm->SetGroup(group);

  // We only need a basic implementation of vtkMultiProcessController for the
  // subgroup, so we just use vtkDummyController here.  It's a bit of a misnomer
  // and may lead to confusion, but I think it's better than creating yet
  // another class we have to maintain.
  vtkDummyController *subcontroller = vtkDummyController::New();
  subcontroller->SetCommunicator(subcomm);
  subcontroller->SetRMICommunicator(subcomm);

  subcomm->Delete();

  return subcontroller;
}

//-----------------------------------------------------------------------------
vtkMultiProcessController *vtkMultiProcessController::PartitionController(
                                                                 int localColor,
                                                                 int localKey)
{
  vtkMultiProcessController *subController = NULL;

  int numProc = this->GetNumberOfProcesses();

  vtkstd::vector<int> allColors(numProc);
  this->AllGather(&localColor, &allColors[0], 1);

  vtkstd::vector<int> allKeys(numProc);
  this->AllGather(&localKey, &allKeys[0], 1);

  vtkstd::vector<bool> inPartition;
  inPartition.assign(numProc, false);

  for (int i = 0; i < numProc; i++)
    {
    if (inPartition[i]) continue;
    int targetColor = allColors[i];
    vtkstd::list<int> partitionIds;     // Make sorted list, then put in group.
    for (int j = i; j < numProc; j++)
      {
      if (allColors[j] != targetColor) continue;
      inPartition[j] = true;
      vtkstd::list<int>::iterator iter = partitionIds.begin();
      while ((iter != partitionIds.end()) && (allKeys[*iter] <= allKeys[j]))
        {
        iter++;
        }
      partitionIds.insert(iter, j);
      }
    // Copy list into process group.
    VTK_CREATE(vtkProcessGroup, group);
    group->Initialize(this);
    group->RemoveAllProcessIds();
    for (vtkstd::list<int>::iterator iter = partitionIds.begin();
         iter != partitionIds.end(); iter++)
      {
      group->AddProcessId(*iter);
      }
    // Use group to create controller.
    vtkMultiProcessController *sc = this->CreateSubController(group);
    if (sc)
      {
      subController = sc;
      }
    }

  return subController;
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
int vtkMultiProcessController::RemoveRMI(unsigned long id)
{
  vtkMultiProcessControllerRMI *rmi = NULL;
  this->RMIs->InitTraversal();
  while ( (rmi = (vtkMultiProcessControllerRMI*)(this->RMIs->GetNextItemAsObject())) )
    {
    if (rmi->Id == id)
      {
      this->RMIs->RemoveItem(rmi);
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
unsigned long vtkMultiProcessController::AddRMI(vtkRMIFunctionType f, 
                                       void *localArg, int tag)
{
  vtkMultiProcessControllerRMI *rmi = vtkMultiProcessControllerRMI::New();

  // Remove any previously registered RMI handler for the tag.
  this->RemoveFirstRMI(tag);

  rmi->Tag = tag;
  rmi->Function = f;
  rmi->LocalArgument = localArg;
  rmi->Id = this->RMICount;
  this->RMICount++;

  this->RMIs->AddItem(rmi);
  rmi->Delete();

  return rmi->Id;
}

//----------------------------------------------------------------------------
void vtkMultiProcessController::TriggerRMIOnAllChildren(
  void *arg, int argLength, int rmiTag)
{
  int myid = this->GetLocalProcessId();
  int childid = 2 * myid + 1; 
  int numProcs = this->GetNumberOfProcesses();
  if (numProcs > childid)
    {
    this->TriggerRMIInternal(childid, arg, argLength, rmiTag, true);
    }
  childid++;
  if (numProcs > childid)
    {
    this->TriggerRMIInternal(childid, arg, argLength, rmiTag, true);
    }
}

//----------------------------------------------------------------------------
void vtkMultiProcessController::TriggerRMI(int remoteProcessId, 
                                           void *arg, int argLength,
                                           int rmiTag)
{
  // Deal with sending RMI to ourself here for now.
  if (remoteProcessId == this->GetLocalProcessId())
    {
    this->ProcessRMI(remoteProcessId, arg, argLength, rmiTag);
    return;
    }

  this->TriggerRMIInternal(remoteProcessId, arg, argLength, rmiTag, false);
}

//----------------------------------------------------------------------------
void vtkMultiProcessController::TriggerRMIInternal(int remoteProcessId, 
    void* arg, int argLength, int rmiTag, bool propagate)
{
  int triggerMessage[128];
  triggerMessage[0] = rmiTag;
  triggerMessage[1] = argLength;
  
  // It is important for the remote process to know what process invoked it.
  // Multiple processes might try to invoke the method at the same time.
  // The remote method will know where to get additional args.
  triggerMessage[2] = this->GetLocalProcessId();
  
  // Pass the propagate flag.
  triggerMessage[3] = propagate? 1 : 0;

  // We send the header in Little Endian order.
  vtkByteSwap::SwapLERange(triggerMessage, 4);

  // If the message is small, we will try to get the message sent over using a
  // single Send(), rather than two. This helps speed up communication
  // significantly, since sending multiple small messages is generally slower
  // than sending a single large message.
  if (argLength >= 0 && static_cast<unsigned int>(argLength) < sizeof(int)*(128-4))
    {
    if (argLength > 0)
      {
      memcpy(&triggerMessage[4], arg, argLength);
      }
    int num_bytes = static_cast<int>(4*sizeof(int)) + argLength;
    this->RMICommunicator->Send(reinterpret_cast<unsigned char*>(triggerMessage),
      num_bytes, remoteProcessId, RMI_TAG);
    }
  else
    {
    this->RMICommunicator->Send(
      reinterpret_cast<unsigned char*>(triggerMessage), 
      static_cast<int>(4*sizeof(int)), remoteProcessId, RMI_TAG);
    if (argLength > 0)
      {
      this->RMICommunicator->Send((char*)arg, argLength, remoteProcessId,  
        RMI_ARG_TAG);
      }
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
  return this->ProcessRMIs(1, 0);
}

//----------------------------------------------------------------------------
int vtkMultiProcessController::ProcessRMIs(int reportErrors, int dont_loop)
{
  int triggerMessage[128];
  unsigned char *arg = NULL;
  int error = RMI_NO_ERROR;
  
  do 
    {
    if (!this->RMICommunicator->Receive(
        reinterpret_cast<unsigned char*>(triggerMessage), 
        static_cast<vtkIdType>(128*sizeof(int)), ANY_SOURCE, RMI_TAG) ||
      this->RMICommunicator->GetCount() < static_cast<int>(4*sizeof(int)))
      {
      if (reportErrors)
        {
        vtkErrorMacro("Could not receive RMI trigger message.");
        }
      error = RMI_TAG_ERROR;
      break;
      }
#ifdef VTK_WORDS_BIGENDIAN
    // Header is sent in little-endian form. We need to convert it to  big
    // endian.
    vtkByteSwap::SwapLERange(triggerMessage, 4);
#endif

    if (triggerMessage[1] > 0)
      {
      arg = new unsigned char[triggerMessage[1]];
      // If the message length is small enough, the TriggerRMIInternal() call
      // packs the message data inline. So depending on the message length we
      // use the inline data or make a second receive to fetch the data.
      if (static_cast<unsigned int>(triggerMessage[1]) < sizeof(int)*(128-4))
        {
        int num_bytes = static_cast<int>(4 *sizeof(int)) + triggerMessage[1];
        if (this->RMICommunicator->GetCount() != num_bytes)
          {
          if (reportErrors)
            {
            vtkErrorMacro("Could not receive the RMI argument in its entirety.");
            }
          error= RMI_ARG_ERROR;
          break;
          }
        memcpy(arg, &triggerMessage[4], triggerMessage[1]);
        }
      else
        {
        if (!this->RMICommunicator->Receive((char*)(arg), triggerMessage[1], 
            triggerMessage[2], RMI_ARG_TAG) ||
          this->RMICommunicator->GetCount() != triggerMessage[1])
          {
          if (reportErrors)
            {
            vtkErrorMacro("Could not receive RMI argument.");
            }
          error = RMI_ARG_ERROR;
          break;
          }
        }
      }
    if (triggerMessage[3] == 1 && this->GetNumberOfProcesses() > 3)//propagate==true
      {
      this->TriggerRMIOnAllChildren(arg, triggerMessage[1], triggerMessage[0]);
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
    } while (!dont_loop);

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
vtkMultiProcessController *vtkMultiProcessController::GetLocalController()
{
  return VTK_GLOBAL_MULTI_PROCESS_CONTROLLER;
}
//----------------------------------------------------------------------------
// This can be overridden in the subclass to translate controllers.
void vtkMultiProcessController::SetGlobalController(
                                   vtkMultiProcessController *controller)
{
  VTK_GLOBAL_MULTI_PROCESS_CONTROLLER = controller;
}


