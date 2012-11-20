/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadedStreamingPipeline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright (c) 2008, 2009 by SCI Institute, University of Utah.

  This is part of the Parallel Dataflow System originally developed by
  Huy T. Vo and Claudio T. Silva. For more information, see:

  "Parallel Dataflow Scheme for Streaming (Un)Structured Data" by Huy
  T. Vo, Daniel K. Osmari, Brian Summa, Joao L.D. Comba, Valerio
  Pascucci and Claudio T. Silva, SCI Institute, University of Utah,
  Technical Report #UUSCI-2009-004, 2009.

  "Multi-Threaded Streaming Pipeline For VTK" by Huy T. Vo and Claudio
  T. Silva, SCI Institute, University of Utah, Technical Report
  #UUSCI-2009-005, 2009.
-------------------------------------------------------------------------*/
#include "vtkThreadedStreamingPipeline.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkComputingResources.h"
#include "vtkExecutionScheduler.h"
#include "vtkExecutive.h"
#include "vtkExecutiveCollection.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationExecutivePortVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationVector.h"
#include "vtkMultiThreader.h"
#include "vtkMutexLock.h"
#include "vtkObjectFactory.h"
#include "vtkThreadMessager.h"
#include "vtkTimerLog.h"
#include <vtksys/hash_set.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkThreadedStreamingPipeline);

vtkInformationKeyMacro(vtkThreadedStreamingPipeline, AUTO_PROPAGATE, Integer);
vtkInformationKeyRestrictedMacro(vtkThreadedStreamingPipeline,
                                 EXTRA_INFORMATION, ObjectBase,
                                 "vtkInformation");

//----------------------------------------------------------------------------
// Convinient definitions of vector/set of vtkExecutive
class vtkExecutiveHasher
{
public:
  size_t operator()(const vtkExecutive* e) const
  {
    return (size_t)e;
  }
};
typedef vtksys::hash_set<vtkExecutive*, vtkExecutiveHasher> vtkExecutiveSet;

//----------------------------------------------------------------------------
vtkThreadedStreamingPipeline::vtkThreadedStreamingPipeline()
{
  this->LastDataRequestTime = 0.0f;
  this->LastDataRequestTimeFromSource = 0.0f;
  this->ForceDataRequest = NULL;
  this->Resources = NULL;
  this->Scheduler = NULL;
}

//----------------------------------------------------------------------------
vtkThreadedStreamingPipeline::~vtkThreadedStreamingPipeline()
{
  if (this->ForceDataRequest)
    {
    this->ForceDataRequest->Delete();
    }
  if (this->Resources)
    {
    this->Resources->Delete();
    }
  if (this->Scheduler)
    {
    this->Scheduler->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkThreadedStreamingPipeline::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
static bool MultiThreadedEnabled = false;
static bool AutoPropagatePush = false;

//----------------------------------------------------------------------------
void vtkThreadedStreamingPipeline::SetMultiThreadedEnabled(bool enabled)
{
  MultiThreadedEnabled = enabled;
}

//----------------------------------------------------------------------------
void vtkThreadedStreamingPipeline::SetAutoPropagatePush(bool enabled)
{
  AutoPropagatePush = enabled;
}

//----------------------------------------------------------------------------
static void
CollectUpstreamModules(vtkExecutive *exec, vtkExecutiveSet &eSet)
{
  for(int i = 0; i < exec->GetNumberOfInputPorts(); ++i)
    {
    int nic = exec->GetAlgorithm()->GetNumberOfInputConnections(i);
    vtkInformationVector* inVector = exec->GetInputInformation()[i];
    for(int j = 0; j < nic; ++j)
      {
      vtkInformation* inInfo = inVector->GetInformationObject(j);
      vtkExecutive* e;
      int producerPort;
      vtkExecutive::PRODUCER()->Get(inInfo, e, producerPort);
      if (eSet.find(e)!=eSet.end())
        {
        continue;
        }
      eSet.insert(e);
      CollectUpstreamModules(e, eSet);
      }
    }
}

//----------------------------------------------------------------------------
void vtkThreadedStreamingPipeline::Pull(vtkExecutive *exec)
{
  vtkThreadedStreamingPipeline::Pull(exec, static_cast<vtkInformation*>(NULL));
}

//----------------------------------------------------------------------------
void vtkThreadedStreamingPipeline::Pull(vtkExecutive *exec, vtkInformation *info)
{
  vtkExecutiveCollection *execs = vtkExecutiveCollection::New();
  execs->AddItem(exec);
  vtkThreadedStreamingPipeline::Pull(execs, info);
  execs->Delete();
}

//----------------------------------------------------------------------------
void vtkThreadedStreamingPipeline::Pull(vtkExecutiveCollection *execs)
{
  vtkThreadedStreamingPipeline::Pull(execs, static_cast<vtkInformation*>(NULL));
}

//----------------------------------------------------------------------------
void vtkThreadedStreamingPipeline::Pull(vtkExecutiveCollection *execs, vtkInformation *info)
{
  vtkExecutiveSet eSet;
  execs->InitTraversal();
  for (vtkExecutive *e = execs->GetNextItem(); e != 0; e = execs->GetNextItem())
    {
    eSet.insert(e);
    CollectUpstreamModules(e, eSet);
    }
  vtkExecutiveCollection *collectedExecs = vtkExecutiveCollection::New();
  for (vtkExecutiveSet::iterator ti = eSet.begin(); ti != eSet.end(); ti++)
    {
    collectedExecs->AddItem(*ti);
    }
  vtkExecutionScheduler::GetGlobalScheduler()->Schedule(collectedExecs, info);
  vtkExecutionScheduler::GetGlobalScheduler()->WaitUntilDone(collectedExecs);
  collectedExecs->Delete();
}

//----------------------------------------------------------------------------
void vtkThreadedStreamingPipeline::Push(vtkExecutive *exec)
{
  vtkThreadedStreamingPipeline::Push(exec, static_cast<vtkInformation*>(NULL));
}

//----------------------------------------------------------------------------
void vtkThreadedStreamingPipeline::Push(vtkExecutive *exec, vtkInformation *info)
{
  vtkExecutiveCollection *execs = vtkExecutiveCollection::New();
  execs->AddItem(exec);
  vtkThreadedStreamingPipeline::Push(execs, info);
  execs->Delete();
}

//----------------------------------------------------------------------------
void vtkThreadedStreamingPipeline::Push(vtkExecutiveCollection *execs)
{
  vtkThreadedStreamingPipeline::Push(execs, static_cast<vtkInformation*>(NULL));
}

//----------------------------------------------------------------------------
void vtkThreadedStreamingPipeline::Push(vtkExecutiveCollection *execs, vtkInformation *info)
{
  vtkExecutiveSet eSet;
  execs->InitTraversal();
  for (vtkExecutive *e = execs->GetNextItem(); e != 0; e = execs->GetNextItem())
    {
    eSet.insert(e);
    e->GetAlgorithm()->GetInformation()->Set(EXTRA_INFORMATION(), info);
    }
  if (AutoPropagatePush)
    {
    if (info==NULL)
      {
      info = vtkInformation::New();
      }
    info->Set(vtkThreadedStreamingPipeline::AUTO_PROPAGATE(), 1);
    }
  vtkExecutionScheduler::GetGlobalScheduler()->Schedule(execs, info);
  fprintf(stderr, "OK 2 %d\n", execs->GetNumberOfItems());
  vtkExecutionScheduler::GetGlobalScheduler()->WaitUntilReleased(execs);
  fprintf(stderr, "OK 3\n");
}

//----------------------------------------------------------------------------
void vtkThreadedStreamingPipeline::Pull()
{
  this->Pull(static_cast<vtkInformation*>(NULL));
}

//----------------------------------------------------------------------------
void vtkThreadedStreamingPipeline::Pull(vtkInformation *info)
{
  vtkExecutiveSet eSet;
  CollectUpstreamModules(this, eSet);
  vtkExecutiveCollection *execs = vtkExecutiveCollection::New();
  for (vtkExecutiveSet::iterator ti = eSet.begin(); ti != eSet.end(); ti++)
    {
    execs->AddItem(*ti);
    }
  vtkExecutionScheduler::GetGlobalScheduler()->Schedule(execs, info);
  vtkExecutionScheduler::GetGlobalScheduler()->ReleaseResources(this);
  vtkExecutionScheduler::GetGlobalScheduler()->WaitUntilDone(execs);
  vtkExecutionScheduler::GetGlobalScheduler()->ReacquireResources(this);
  execs->Delete();
}

//----------------------------------------------------------------------------
void vtkThreadedStreamingPipeline::Push()
{
  this->Push(static_cast<vtkInformation*>(NULL));
}

//----------------------------------------------------------------------------
void vtkThreadedStreamingPipeline::Push(vtkInformation *info)
{
  vtkExecutiveSet eSet;
  for(int i = 0; i < this->GetNumberOfOutputPorts(); ++i) {
    vtkInformation* outInfo = this->GetOutputInformation(i);
    int consumerCount = vtkExecutive::CONSUMERS()->Length(outInfo);
    vtkExecutive** e = vtkExecutive::CONSUMERS()->GetExecutives(outInfo);
    for (int j=0; j<consumerCount; j++)
      {
      eSet.insert(e[j]);
      e[j]->GetAlgorithm()->GetInformation()->Set(EXTRA_INFORMATION(), info);
      }
  }
  vtkExecutiveCollection *execs = vtkExecutiveCollection::New();
  for (vtkExecutiveSet::iterator ti=eSet.begin(); ti!=eSet.end(); ti++)
    {
    execs->AddItem(*ti);
    }
  if (AutoPropagatePush)
    {
    vtkExecutionScheduler::GetGlobalScheduler()->SchedulePropagate(execs, info);
    }
  else
    {
    vtkExecutionScheduler::GetGlobalScheduler()->Schedule(execs, info);
    vtkExecutionScheduler::GetGlobalScheduler()->ReleaseResources(this);
    vtkExecutionScheduler::GetGlobalScheduler()->WaitUntilReleased(execs);
    vtkExecutionScheduler::GetGlobalScheduler()->ReacquireResources(this);
    }
  execs->Delete();
}

//----------------------------------------------------------------------------
void vtkThreadedStreamingPipeline::ReleaseInputs()
{
  vtkThreadMessager *messager = vtkExecutionScheduler::
    GetGlobalScheduler()->GetInputsReleasedMessager(this);
  if (messager)
    {
    messager->SendWakeMessage();
    }
}

//----------------------------------------------------------------------------
int vtkThreadedStreamingPipeline
::ProcessRequest(vtkInformation* request,
                 vtkInformationVector** inInfoVec,
                 vtkInformationVector* outInfoVec)
{
  int result = 0;
  if (request->Has(REQUEST_DATA()))
    {
    double startTime = vtkTimerLog::GetUniversalTime();
    result = this->Superclass::ProcessRequest(request, inInfoVec, outInfoVec);
    this->LastDataRequestTime = vtkTimerLog::GetUniversalTime() - startTime;
    }
  else
    {
    result = this->Superclass::ProcessRequest(request, inInfoVec, outInfoVec);
    }
  return result;
}

//----------------------------------------------------------------------------
int vtkThreadedStreamingPipeline::ForceUpdateData(int vtkNotUsed(processingUnit), vtkInformation *info)
{
  if (this->ForceDataRequest==NULL)
    {
    this->ForceDataRequest = vtkInformation::New();
    }
  if (info)
    {
    this->ForceDataRequest->Copy(info);
    }
  else
    {
    this->ForceDataRequest->Clear();
    }
  this->ForceDataRequest->Set(vtkDemandDrivenPipeline::REQUEST_DATA());
  this->ForceDataRequest->Set(vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);
  // Algorithms process this request after it is forwarded.
  this->ForceDataRequest->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);
//   this->ForceDataRequest->
//     Set(vtkThreadedStreamingPipeline::PROCESSING_UNIT(), processingUnit);
  double startTime = vtkTimerLog::GetUniversalTime();
  int result =  this->CallAlgorithm(this->ForceDataRequest, vtkExecutive::RequestDownstream,
                                    this->GetInputInformation(),
                                    this->GetOutputInformation());
  this->LastDataRequestTime = vtkTimerLog::GetUniversalTime() - startTime;
  return result;
}

//----------------------------------------------------------------------------
void vtkThreadedStreamingPipeline::UpdateRequestDataTimeFromSource()
{
  float maxUpStreamTime = 0.0f;
  for(int i = 0; i < this->GetNumberOfInputPorts(); ++i)
    {
    int nic = this->GetAlgorithm()->GetNumberOfInputConnections(i);
    vtkInformationVector* inVector = this->GetInputInformation()[i];
    for(int j=0; j < nic; ++j)
      {
      vtkInformation* inInfo = inVector->GetInformationObject(j);
      vtkExecutive* e;
      int producerPort;
      vtkExecutive::PRODUCER()->Get(inInfo, e, producerPort);
      if (e)
        {
        vtkThreadedStreamingPipeline *te = vtkThreadedStreamingPipeline::
          SafeDownCast(e);
        if (te && maxUpStreamTime<te->LastDataRequestTimeFromSource)
          maxUpStreamTime = te->LastDataRequestTimeFromSource;
        }
      }
    }
  this->LastDataRequestTimeFromSource = maxUpStreamTime + this->LastDataRequestTime;
}

//----------------------------------------------------------------------------
vtkComputingResources *vtkThreadedStreamingPipeline::GetResources() {
  if (!this->Resources)
    {
    this->Resources = vtkComputingResources::New();
    }
  return this->Resources;
}

//----------------------------------------------------------------------------
int vtkThreadedStreamingPipeline::ForwardUpstream(vtkInformation* request)
{
  if (MultiThreadedEnabled && request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    this->Pull();
    return 1;
    }
  else
    {
    return this->Superclass::ForwardUpstream(request);
    }
}
//----------------------------------------------------------------------------
int vtkThreadedStreamingPipeline::ForwardUpstream(int i, int j, vtkInformation* request)
{
  return this->Superclass::ForwardUpstream(i, j, request);
}
