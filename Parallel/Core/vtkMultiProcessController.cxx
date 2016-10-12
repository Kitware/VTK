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
#include "vtkCommand.h"
#include "vtkDummyController.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkProcessGroup.h"
#include "vtkSubCommunicator.h"
#include "vtkToolkits.h"
#include "vtkProcess.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <list>
#include <vector>
#include <vtksys/hash_map.hxx>

//-----------------------------------------------------------------------------
// Stores internal members that cannot or should not be exposed in the header
// file (for example, because they use templated types).
class vtkMultiProcessController::vtkInternal
{
public:
  vtksys::hash_map<int, vtkProcessFunctionType> MultipleMethod;
  vtksys::hash_map<int, void *> MultipleData;

  class vtkRMICallback
  {
  public:
    unsigned long Id;
    vtkRMIFunctionType Function;
    void* LocalArgument;
  };

  typedef std::vector<vtkRMICallback> RMICallbackVector;

  // key == tag, value == vector of vtkRMICallback instances.
  typedef vtksys::hash_map<int, RMICallbackVector> RMICallbackMap;
  RMICallbackMap RMICallbacks;
};

//----------------------------------------------------------------------------
// An RMI function that will break the "ProcessRMIs" loop.
static void vtkMultiProcessControllerBreakRMI(void *localArg,
                                              void *remoteArg, int remoteArgLength,
                                              int vtkNotUsed(remoteId))
{
  (void)remoteArg;
  (void)remoteArgLength;
  vtkMultiProcessController *controller = (vtkMultiProcessController*)(localArg);
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

  this->SingleMethod = 0;
  this->SingleData = 0;

  this->Communicator = 0;
  this->RMICommunicator = 0;

  this->BreakFlag = 0;
  this->ForceDeepCopy = 1;

  this->BroadcastTriggerRMI = false;

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

  delete this->Internal;
}


//----------------------------------------------------------------------------
void vtkMultiProcessController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  vtkIndent nextIndent = indent.GetNextIndent();

  os << indent << "Break flag: " << (this->BreakFlag ? "(yes)" : "(no)")
     << endl;
  os << indent << "Force deep copy: " << (this->ForceDeepCopy ? "(yes)" : "(no)")
     << endl;
  os << indent << "Output window: ";
  os << indent << "BroadcastTriggerRMI: " <<
      (this->BroadcastTriggerRMI ? "(yes)" : "(no)");
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

  std::vector<int> allColors(numProc);
  this->AllGather(&localColor, &allColors[0], 1);

  std::vector<int> allKeys(numProc);
  this->AllGather(&localKey, &allKeys[0], 1);

  std::vector<bool> inPartition;
  inPartition.assign(numProc, false);

  for (int i = 0; i < numProc; i++)
  {
    if (inPartition[i]) continue;
    int targetColor = allColors[i];
    std::list<int> partitionIds;     // Make sorted list, then put in group.
    for (int j = i; j < numProc; j++)
    {
      if (allColors[j] != targetColor) continue;
      inPartition[j] = true;
      std::list<int>::iterator iter = partitionIds.begin();
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
    for (std::list<int>::iterator iter = partitionIds.begin();
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
unsigned long vtkMultiProcessController::AddRMICallback(
  vtkRMIFunctionType callback, void* localArg, int tag)
{
  vtkInternal::vtkRMICallback callbackInfo;
  callbackInfo.Id = this->RMICount++;
  callbackInfo.Function = callback;
  callbackInfo.LocalArgument = localArg;
  this->Internal->RMICallbacks[tag].push_back(callbackInfo);
  return callbackInfo.Id;
}

//----------------------------------------------------------------------------
void vtkMultiProcessController::RemoveAllRMICallbacks(int tag)
{
  vtkInternal::RMICallbackMap::iterator iter =
    this->Internal->RMICallbacks.find(tag);
  if (iter != this->Internal->RMICallbacks.end())
  {
    this->Internal->RMICallbacks.erase(iter);
  }
}

//----------------------------------------------------------------------------
bool vtkMultiProcessController::RemoveRMICallback(unsigned long id)
{
  vtkInternal::RMICallbackMap::iterator iterMap;
  for (iterMap = this->Internal->RMICallbacks.begin();
    iterMap != this->Internal->RMICallbacks.end(); ++iterMap)
  {
    vtkInternal::RMICallbackVector::iterator iterVec;
    for (iterVec = iterMap->second.begin();
      iterVec != iterMap->second.end(); ++iterVec)
    {
      if (iterVec->Id == id)
      {
        iterMap->second.erase(iterVec);
        return true;
      }
    }
  }
  return false;
}

//----------------------------------------------------------------------------
int vtkMultiProcessController::RemoveFirstRMI(int tag)
{
  vtkInternal::RMICallbackMap::iterator iter =
    this->Internal->RMICallbacks.find(tag);
  if (iter != this->Internal->RMICallbacks.end())
  {
    if (iter->second.begin() != iter->second.end())
    {
      iter->second.erase(iter->second.begin());
      return 1;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkMultiProcessController::RemoveRMI(unsigned long id)
{
  return this->RemoveRMICallback(id)? 1 : 0;
}

//----------------------------------------------------------------------------
unsigned long vtkMultiProcessController::AddRMI(vtkRMIFunctionType f,
                                       void *localArg, int tag)
{
  // Remove any previously registered RMI handler for the tag.
  this->RemoveAllRMICallbacks(tag);
  return this->AddRMICallback(f, localArg, tag);
}

//----------------------------------------------------------------------------
void vtkMultiProcessController::TriggerRMIOnAllChildren(
  void *arg, int argLength, int rmiTag)
{

  if( this->BroadcastTriggerRMI )
  {
    this->BroadcastTriggerRMIOnAllChildren(arg,argLength,rmiTag);
  } // END if use broadcast method
  else
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
  } // END else
}

//----------------------------------------------------------------------------
void vtkMultiProcessController::TriggerRMI(int remoteProcessId,
                                           void *arg, int argLength,
                                           int rmiTag)
{
  if( this->BroadcastTriggerRMI  )
  {
    vtkErrorMacro(
        "TriggerRMI should not be called when BroadcastTriggerRMI is ON");
  }

  // Deal with sending RMI to ourself here for now.
  if (remoteProcessId == this->GetLocalProcessId())
  {
    this->ProcessRMI(remoteProcessId, arg, argLength, rmiTag);
    return;
  }

  this->TriggerRMIInternal(remoteProcessId, arg, argLength, rmiTag, false);
}

//----------------------------------------------------------------------------
void vtkMultiProcessController::BroadcastTriggerRMIOnAllChildren(
    void *arg, int argLength, int rmiTag)
{
  // This is called by the root process, namely rank 0. The sattelite ranks
  // call BroadcastProcessRMIs().

  int triggerMessage[128];
  triggerMessage[0] = rmiTag;
  triggerMessage[1] = argLength;

  // Note: We don't need to set the local process ID & propagate

  // We send the header in Little Endian order.
  vtkByteSwap::SwapLERange(triggerMessage, 2);

  // If possible, send the data over a single broadcast
  int buffer_size = sizeof(int)*(126 /*128-2*/);
  if( (argLength >= 0) && (argLength < buffer_size) )
  {
    if( argLength > 0 )
    {
      memcpy(&triggerMessage[2],arg,argLength);
    }
    this->RMICommunicator->Broadcast(triggerMessage,128,0);
  }
  else
  {
    this->RMICommunicator->Broadcast(triggerMessage,128,0);
    this->RMICommunicator->Broadcast(
        reinterpret_cast<unsigned char*>(arg),argLength,0);
  }
}

//----------------------------------------------------------------------------
int vtkMultiProcessController::BroadcastProcessRMIs(
    int vtkNotUsed(reportErrors), int dont_loop )
{
  int triggerMessage[128];
  int rmiTag;
  int argLength;
  unsigned char *arg = NULL;
  int error = RMI_NO_ERROR;

  this->InvokeEvent(vtkCommand::StartEvent);

  do
  {
    this->RMICommunicator->Broadcast(triggerMessage,128,0);

#ifdef VTK_WORDS_BIGENDIAN
    // Header is sent in little-endian form. We need to convert it to  big
    // endian.
    vtkByteSwap::SwapLERange(triggerMessage, 2);
#endif

    rmiTag    = triggerMessage[0];
    argLength = triggerMessage[1];

    if( argLength > 0 )
    {
      arg = new unsigned char[ argLength ];
      int buffer_size = sizeof(int)*(126 /*128-2*/);

      if( argLength < buffer_size )
      {
        memcpy(arg,&triggerMessage[2],argLength);
      } // END if message is inline
      else
      {
        this->RMICommunicator->Broadcast(arg,argLength,0);
      } // END else message was not inline

    } // END if non-null arguments

    this->ProcessRMI(0 /*we broadcast from rank 0*/, arg, argLength,rmiTag);

    delete [] arg;
    arg = NULL;

    // check for break
    if (this->BreakFlag)
    {
      this->BreakFlag = 0;
      this->InvokeEvent(vtkCommand::EndEvent);
      return error;
    }

  } while( !dont_loop );

  this->InvokeEvent(vtkCommand::EndEvent);
  return error;
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

  if( this->BroadcastTriggerRMI == 1 )
  {
    this->BroadcastTriggerRMIOnAllChildren(NULL,0,BREAK_RMI_TAG);
    return;
  }

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
  if( this->BroadcastTriggerRMI )
  {
    return this->BroadcastProcessRMIs(reportErrors,dont_loop);
  }


  this->InvokeEvent(vtkCommand::StartEvent);
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
    delete [] arg;
    arg = NULL;

    // check for break
    if (this->BreakFlag)
    {
      this->BreakFlag = 0;
      this->InvokeEvent(vtkCommand::EndEvent);
      return error;
    }
  } while (!dont_loop);

  this->InvokeEvent(vtkCommand::EndEvent);
  return error;
}


//----------------------------------------------------------------------------
void vtkMultiProcessController::ProcessRMI(int remoteProcessId,
                                           void *arg, int argLength,
                                           int rmiTag)
{
  // we build the list of callbacks to call and then invoke them to handle the
  // case where the callback removes the callback.
  std::vector<vtkInternal::vtkRMICallback> callbacks;

  vtkInternal::RMICallbackMap::iterator iter =
    this->Internal->RMICallbacks.find(rmiTag);
  if (iter != this->Internal->RMICallbacks.end())
  {
    vtkInternal::RMICallbackVector::iterator iterVec;
    for (iterVec = iter->second.begin();
      iterVec != iter->second.end(); iterVec++)
    {
      if (iterVec->Function)
      {
        callbacks.push_back(*iterVec);
      }
    }
  }

  if (callbacks.size()==0)
  {
    vtkErrorMacro("Process " << this->GetLocalProcessId() <<
                  " Could not find RMI with tag " << rmiTag);
  }

  std::vector<vtkInternal::vtkRMICallback>::iterator citer;
  for (citer = callbacks.begin(); citer != callbacks.end(); citer++)
  {
    (*citer->Function)(citer->LocalArgument, arg, argLength, remoteProcessId);
  }
}

//============================================================================
// The intent is to give access to a processes controller from a static method.
static vtkWeakPointer<vtkMultiProcessController> VTK_GLOBAL_MULTI_PROCESS_CONTROLLER;
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


