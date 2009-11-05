/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExecutionScheduler.cxx

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
#include "vtkExecutionScheduler.h"

#include "vtkAlgorithm.h"
#include "vtkComputingResources.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationExecutivePortVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkMultiThreader.h"
#include "vtkMutexLock.h"
#include "vtkObjectFactory.h"
#include "vtkThreadedStreamingPipeline.h"
#include "vtkThreadMessager.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkExecutionScheduler, "1.2");
vtkStandardNewMacro(vtkExecutionScheduler);

vtkInformationKeyMacro(vtkExecutionScheduler, TASK_PRIORITY, Integer);

//----------------------------------------------------------------------------
static vtkExecutionScheduler *globalScheduler = vtkExecutionScheduler::New();
//----------------------------------------------------------------------------
vtkExecutionScheduler* vtkExecutionScheduler::GetGlobalScheduler()
{
  return globalScheduler;
}

//----------------------------------------------------------------------------
void * vtkExecutionScheduler_ScheduleThread(void *data);
//----------------------------------------------------------------------------
vtkExecutionScheduler::vtkExecutionScheduler()
{
  this->CurrentPriority = 0;
  this->Resources = vtkComputingResources::New();
  this->Resources->ObtainMaximumResources();
  this->ResourceMessager = vtkThreadMessager::New();
  this->ScheduleLock = vtkMutexLock::New();
  this->ScheduleMessager = vtkThreadMessager::New();
  this->ScheduleThreader = vtkMultiThreader::New();
  this->ScheduleThreader->SetNumberOfThreads(1);
  this->ScheduleThreadId = -1;
}

//----------------------------------------------------------------------------
vtkExecutionScheduler::~vtkExecutionScheduler()
{
  this->Resources->Delete();
  this->ResourceMessager->Delete();
  this->ScheduleLock->Delete();
  this->ScheduleMessager->Delete();
  this->ScheduleThreader->Delete();
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::Schedule
(const vtkThreadedStreamingPipeline::vtkExecutiveSet &eSet,
 vtkInformation *info)
{
  if (this->ScheduleThreadId==-1)
    this->ScheduleThreadId = this->ScheduleThreader->
      SpawnThread((vtkThreadFunctionType)(vtkExecutionScheduler_ScheduleThread), this);
  this->ScheduleLock->Lock();
  vtkThreadedStreamingPipeline::vtkExecutiveVector G;
  vtkThreadedStreamingPipeline::vtkExecutiveSet::const_iterator it;
  for (it=eSet.begin(); it!=eSet.end(); it++) {
    if (this->ExecutingTasks.find(*it)!=this->ExecutingTasks.end())
      return;
    if (this->DependencyNodes.find(*it)==this->DependencyNodes.end()) {
      this->UpdateDependencyGraph(*it);
    }
    G.push_back(*it);
  }

  // Create a adjacency matrix
  int i, j, k, p;
  int N = G.size();
  int *A = (int*)malloc(N*N*sizeof(int));
  int *degree = (int*)malloc(N*sizeof(int));
  memset(A, 0, N*N*sizeof(int));
  memset(degree, 0, N*sizeof(int));
  for (i=0; i<N; i++) {
    int src = (*(this->DependencyNodes.find(G[i]))).second;
    for (j=0; j<N; j++) {
      int dst = (*(this->DependencyNodes.find(G[j]))).second;
      if (this->DependencyEdges.find(Edge(src, dst))!=this->DependencyEdges.end()) {
        A[i*N+j] = 1;
        degree[j]++;
      }
    }
  }
  
  int *S = (int*)malloc(N*sizeof(int));
  k = 0;
  for (j=0; j<N; j++)
    if (degree[j]==0)
      S[k++] = j;
  p = 0;
  while (p<k) {
    i = S[p++];
    this->AddToQueue(G[i], info);
    for (j=0; j<N; j++)
      if (A[i*N+j]) {
        degree[j]--;
        A[i*N+j] = 0;
        if (degree[j]==0)
          S[k++] = j;
      }
  }
  free(S);
  free(degree);
  free(A);
        
  // Wake the scheduling thread up if it is currently waiting for tasks
  this->ScheduleMessager->SendWakeMessage();
  this->ScheduleLock->Unlock();

}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::UpdateDependencyGraph(vtkExecutive *exec)
{
  vtkThreadedStreamingPipeline::vtkExecutiveSet visited;
  this->FindAndTraverseFromSources(exec, visited);
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::FindAndTraverseFromSources
(vtkExecutive *exec,
 vtkThreadedStreamingPipeline::vtkExecutiveSet &visited)
{
  if (visited.find(exec)!=visited.end())
    return;
  visited.insert(exec);
  bool isSource = true;
  for(int i=0; i < exec->GetNumberOfInputPorts(); ++i) {
    int nic = exec->GetAlgorithm()->GetNumberOfInputConnections(i);
    vtkInformationVector* inVector = exec->GetInputInformation()[i];
    for(int j=0; j < nic; ++j) {
      vtkInformation* inInfo = inVector->GetInformationObject(j);
      vtkExecutive* e;
      int producerPort;
      vtkExecutive::PRODUCER()->Get(inInfo, e, producerPort);
      if (e) {
        isSource = false;
        this->FindAndTraverseFromSources(e, visited);
      }
    }
  }
  if (isSource) {
    vtkThreadedStreamingPipeline::vtkExecutiveSet upstream;
    vtkThreadedStreamingPipeline::vtkExecutiveSet downVisited;
    this->TraverseDownToSink(exec, upstream, downVisited);
  }
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::TraverseDownToSink(vtkExecutive *exec,
                                               vtkThreadedStreamingPipeline::vtkExecutiveSet &upstream,
                                               vtkThreadedStreamingPipeline::vtkExecutiveSet &visited)
{
  if (visited.find(exec)!=visited.end())
    return;
  
  // Now mark all edges from upstream modules to exec as dependency edges
  int vId = this->AddToDependencyGraph(exec);
  for (vtkThreadedStreamingPipeline::vtkExecutiveSet::const_iterator it=upstream.begin();
       it!=upstream.end(); it++) {
    ExecutiveIntHashMap::iterator hit = this->DependencyNodes.find(*it);
    this->DependencyEdges.insert(Edge((*hit).second, vId));
  }
  
  // Mark as visited
  visited.insert(exec);

  // Then insert it to the upstream list for going down
  upstream.insert(exec);
  for(int i=0; i < exec->GetNumberOfOutputPorts(); ++i) {
    vtkInformation* info = exec->GetOutputInformation(i);
    int consumerCount = vtkExecutive::CONSUMERS()->Length(info);
    vtkExecutive** e = vtkExecutive::CONSUMERS()->GetExecutives(info);
    for (int j=0; j<consumerCount; j++)
      if (e[j]) {
        this->TraverseDownToSink(e[j], upstream, visited);
      }
  }
  
  // Take it out of the upstream and prepare for back-tracking
  upstream.erase(exec);
}

//----------------------------------------------------------------------------
int vtkExecutionScheduler::AddToDependencyGraph(vtkExecutive *exec)
{
  // We never remove vertices, it's ok to just return the size of
  // DependencyGraph as a node id
  ExecutiveIntHashMap::iterator hit = this->DependencyNodes.find(exec);
  // Check if this is a new module or an untouched sub-network
  int vId = -1;
  if (hit==this->DependencyNodes.end()) {
    vId = this->DependencyNodes.size();
    this->DependencyNodes[exec] = vId;
    
    // Make sure that we have enough thread messagers for this vId
    while (this->TaskDoneMessagers.size()<=vId)
      this->TaskDoneMessagers.push_back(vtkThreadMessager::New());
    while (this->InputsReleasedMessagers.size()<=vId) {
      this->InputsReleasedMessagers.push_back(vtkThreadMessager::New());
      this->InputsReleasedLocks.push_back(vtkMutexLock::New());
    }
  }
  // We have this module in our cache before
  else {
    vId = (*hit).second;
  }
  return vId;
}

//----------------------------------------------------------------------------
int vtkExecutionScheduler::AcquirePriority(vtkInformation * info)
{
  int priority;
  if (info && info->Has(TASK_PRIORITY())) {
    priority = info->Get(TASK_PRIORITY());
  }
  else {
    priority = this->CurrentPriority++;
  }
  return priority;
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::AddToQueue(vtkExecutive *exec, vtkInformation * info)
{
  int priority = this->AcquirePriority(info);
  this->PrioritizedTasks.insert(Task(priority, exec, info));
  vtkMutexLock *lock = this->GetInputsReleasedLock(exec);
  if (lock) {
//     fprintf(stderr, "A %s\n", exec->GetAlgorithm()->GetClassName());
    lock->Lock();
  }
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::WaitUntilDone(const vtkThreadedStreamingPipeline::vtkExecutiveSet &eSet)
{
  vtkThreadedStreamingPipeline::vtkExecutiveSet::const_iterator it;
  for (it=eSet.begin(); it!=eSet.end(); it++) {
    this->WaitForTaskDone(*it);
  }
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::WaitUntilAllDone()
{
  while (true) {
    vtkExecutive *exec = NULL;
    this->ScheduleLock->Lock();
    if (this->PrioritizedTasks.size()>0)
      exec = (*(this->PrioritizedTasks.begin())).exec;
    this->ScheduleLock->Unlock();
    if (exec)
      this->WaitForTaskDone(exec);
    else
      break;
  }
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::WaitUntilReleased(const vtkThreadedStreamingPipeline::vtkExecutiveSet &eSet)
{
  vtkThreadedStreamingPipeline::vtkExecutiveSet::const_iterator it;
  for (it=eSet.begin(); it!=eSet.end(); it++) {
    this->WaitForInputsReleased(*it);
  }
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::WaitForTaskDone(vtkExecutive *exec)
{
  vtkThreadMessager *messager = this->GetTaskDoneMessager(exec);
  if (messager)
    messager->WaitForMessage();
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::WaitForInputsReleased(vtkExecutive *exec)
{
  vtkMutexLock *lock = this->GetInputsReleasedLock(exec);
  if (lock) {
//     fprintf(stderr, "B\n");
    lock->Lock();
//     fprintf(stderr, "C\n");
    lock->Unlock();
  }
//   vtkThreadMessager *messager = this->GetInputsReleasedMessager(exec);
//   if (messager)
//     messager->WaitForMessage();
}

//----------------------------------------------------------------------------
vtkThreadMessager* vtkExecutionScheduler::GetTaskDoneMessager(vtkExecutive *exec)
{
  ExecutiveIntHashMap::iterator hit = this->DependencyNodes.find(exec);
  if (hit!=this->DependencyNodes.end()) {
    TaskPriorityQueue::const_iterator ti;
    for (ti=this->PrioritizedTasks.begin();
         ti!=this->PrioritizedTasks.end(); ti++) {
      if ((*ti).exec==exec)
        return this->TaskDoneMessagers[(*hit).second];
    }
  }
  return NULL;
}

//----------------------------------------------------------------------------
vtkMutexLock* vtkExecutionScheduler::GetInputsReleasedLock(vtkExecutive *exec)
{
  ExecutiveIntHashMap::iterator hit = this->DependencyNodes.find(exec);
  if (hit!=this->DependencyNodes.end()) {
    TaskPriorityQueue::const_iterator ti;
    for (ti=this->PrioritizedTasks.begin();
         ti!=this->PrioritizedTasks.end(); ti++) {
      if ((*ti).exec==exec)
        return this->InputsReleasedLocks[(*hit).second];
    }
  }
  return NULL;
}

//----------------------------------------------------------------------------
vtkThreadMessager* vtkExecutionScheduler::GetInputsReleasedMessager(vtkExecutive *exec)
{
  ExecutiveIntHashMap::iterator hit = this->DependencyNodes.find(exec);
  if (hit!=this->DependencyNodes.end()) {
    TaskPriorityQueue::const_iterator ti;
    for (ti=this->PrioritizedTasks.begin();
         ti!=this->PrioritizedTasks.end(); ti++) {
      if ((*ti).exec==exec)
        return this->InputsReleasedMessagers[(*hit).second];
    }
  }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::ReleaseResources(vtkExecutive *exec)
{
  vtkThreadedStreamingPipeline *threadedExec = vtkThreadedStreamingPipeline::
    SafeDownCast(exec);
  if (threadedExec) {
    this->ScheduleLock->Lock();
    this->Resources->Collect(threadedExec->GetResources());
    this->ResourceMessager->SendWakeMessage();
    this->ScheduleLock->Unlock();
  }
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::ReacquireResources(vtkExecutive *exec)
{
  vtkThreadedStreamingPipeline *threadedExec = vtkThreadedStreamingPipeline::
    SafeDownCast(exec);
  if (threadedExec) {
    while (!this->Resources->Reserve(threadedExec->GetResources())) {
      this->ResourceMessager->WaitForMessage();
    }
  }
}

//----------------------------------------------------------------------------
bool vtkExecutionScheduler::CanExecuteTask(TaskPriorityQueue::const_iterator taskIter)
{
  if (this->ExecutingTasks.find((*taskIter).exec)!=this->ExecutingTasks.end())
    return false;
  ExecutiveIntHashMap::iterator hit = this->DependencyNodes.find((*taskIter).exec);
  if (hit==this->DependencyNodes.end())
    return true;
  int dst = (*hit).second;
  TaskPriorityQueue::const_iterator ti;
  for (ti=this->PrioritizedTasks.begin();
       ti!=taskIter; ti++) {
    if ((*ti).priority>(*taskIter).priority)
      break;
    hit = this->DependencyNodes.find((*ti).exec);
    int src = (*hit).second;
    if (this->DependencyEdges.find(Edge(src, dst))!=this->DependencyEdges.end())
      return false;
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::RescheduleFrom(vtkExecutive *exec,
                                           vtkComputingResources *resources) {
  vtkThreadedStreamingPipeline::vtkExecutiveVector upstream;
  // Compute the total time
  float totalUpStreamTime = 0.0;
  for(int i=0; i < exec->GetNumberOfInputPorts(); ++i) {
    int nic = exec->GetAlgorithm()->GetNumberOfInputConnections(i);
    vtkInformationVector* inVector = exec->GetInputInformation()[i];
    for(int j=0; j < nic; ++j) {
      vtkInformation* inInfo = inVector->GetInformationObject(j);
      vtkExecutive* e;
      int producerPort;
      vtkExecutive::PRODUCER()->Get(inInfo, e, producerPort);
      if (e && vtkThreadedStreamingPipeline::SafeDownCast(e))
        upstream.push_back(e);
    }
  }
  for (size_t i=0; i<upstream.size(); i++) {    
    totalUpStreamTime += vtkThreadedStreamingPipeline::SafeDownCast(upstream[i])->LastDataRequestTimeFromSource;
  }

  // Then distribute the resources based on those ratios
  vtkProcessingUnitResource *totalResources[] = {
    resources->GetResourceFor(vtkThreadedStreamingPipeline::PROCESSING_UNIT_CPU),
    resources->GetResourceFor(vtkThreadedStreamingPipeline::PROCESSING_UNIT_GPU),
  };
  for (size_t i=0; i<upstream.size(); i++) {
    float ratio = vtkThreadedStreamingPipeline::
        SafeDownCast(upstream[i])->LastDataRequestTimeFromSource/totalUpStreamTime;
    for (int j=0; j<sizeof(totalResources)/sizeof(vtkProcessingUnitResource*); j++) {
      vtkProcessingUnitResource* moduleResource = vtkThreadedStreamingPipeline::
        SafeDownCast(upstream[i])->GetResources()->
        GetResourceFor(totalResources[j]->ProcessingUnit());
      moduleResource->IncreaseByRatio(ratio, totalResources[j]);
    }
  }
  
  // Try to reserve upstream
  for (size_t i=0; i<upstream.size(); i++)
    this->RescheduleFrom(upstream[i],
                         vtkThreadedStreamingPipeline::
                         SafeDownCast(upstream[i])->GetResources());
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::RescheduleNetwork(vtkExecutive *sink) {
  this->Resources->ObtainMaximumResources();
  this->RescheduleFrom(sink, this->Resources);
}

//----------------------------------------------------------------------------
void * vtkExecutionScheduler_ExecuteThread(void *data);
typedef struct {
  vtkExecutionScheduler *scheduler;
  vtkExecutionScheduler::Task task;
} ExecutionData;
//----------------------------------------------------------------------------
void vtkExecutionScheduler::Execute(const Task &task)
{
  ExecutionData *eData = new ExecutionData();
//   fprintf(stderr, ">>>>>>>>> EXECUTING %s\n", task.exec->GetAlgorithm()->GetClassName());
  eData->scheduler = this;
  eData->task = task;
  this->ScheduleThreader->SpawnThread((vtkThreadFunctionType)(vtkExecutionScheduler_ExecuteThread), eData);
}

//----------------------------------------------------------------------------
void * vtkExecutionScheduler_ScheduleThread(void *data)
{
  vtkExecutionScheduler *self = static_cast<vtkExecutionScheduler*>
    (static_cast<vtkMultiThreader::ThreadInfo*>(data)->UserData);
  while (true) {
    self->ScheduleLock->Lock();
    bool needToWait = true;
    vtkExecutionScheduler::TaskPriorityQueue::iterator ti;
//     fprintf(stderr, "STACK ");
//     for (ti =self->PrioritizedTasks.begin();
//          ti!=self->PrioritizedTasks.end(); ti++) {
//       fprintf(stderr, "%s ", (*ti).exec->GetAlgorithm()->GetClassName());
//     }
//     fprintf(stderr, "\n");
    for (ti =self->PrioritizedTasks.begin();
         ti!=self->PrioritizedTasks.end(); ti++) {
      if (self->CanExecuteTask(ti)) {
        vtkThreadedStreamingPipeline *exec = vtkThreadedStreamingPipeline::
          SafeDownCast((*ti).exec);
        if (self->Resources->Reserve(exec->GetResources())) {
          needToWait = false;
          self->ExecutingTasks.insert(exec);
          self->ScheduleLock->Unlock();
          self->Execute(*ti);
          break;
        }
      }
    }
    if (needToWait) {
      self->ScheduleLock->Unlock();
      self->ScheduleMessager->WaitForMessage();
    }
  }
  return NULL;
}

//----------------------------------------------------------------------------
void * vtkExecutionScheduler_ExecuteThread(void *data)
{
  ExecutionData *eData = static_cast<ExecutionData*>
    (static_cast<vtkMultiThreader::ThreadInfo*>(data)->UserData);
  vtkExecutionScheduler *self = eData->scheduler;
  vtkExecutionScheduler::Task task = eData->task;
  vtkThreadedStreamingPipeline *exec = vtkThreadedStreamingPipeline::
    SafeDownCast(task.exec);
  vtkThreadMessager *messager = self->GetTaskDoneMessager(task.exec);
  vtkMutexLock *lock = self->GetInputsReleasedLock(task.exec);
  exec->GetResources()->Deploy(exec, task.info);
  self->ScheduleLock->Lock();
  self->PrioritizedTasks.erase(task);
  self->ExecutingTasks.erase(exec);
  self->Resources->Collect(exec->GetResources());
  self->ResourceMessager->SendWakeMessage();
  self->ScheduleLock->Unlock();
  exec->ReleaseInputs();
  self->ScheduleMessager->SendWakeMessage();
  if (task.info && task.info->Has(vtkThreadedStreamingPipeline::AUTO_PROPAGATE()))
    exec->Push(task.info);
  if (messager)
    messager->SendWakeMessage();
  delete eData;
  lock->Unlock();
//   fprintf(stderr, "DONE EXECUTING %s\n", exec->GetAlgorithm()->GetClassName());
//   if (task.info && task.info->Has(vtkThreadedStreamingPipeline::AUTO_PROPAGATE())) {
//     fprintf(stderr, "BINGO!!!!\n");
//     exec->Push(task.info);
//     for(int i=0; i < exec->GetNumberOfOutputPorts(); ++i) {
//       vtkInformation* info = exec->GetOutputInformation(i);
//       int consumerCount = vtkExecutive::CONSUMERS()->Length(info);
//       vtkExecutive** e = vtkExecutive::CONSUMERS()->GetExecutives(info);
//       for (int j=0; j<consumerCount; j++)
//         if (e[j] && self->ExecutingTasks.find(e[j])==self->ExecutingTasks.end()) {
//           fprintf(stderr, ">>>>>>>>>>>>>>>>>>>>>>>>>> %s\n", e[j]->GetAlgorithm()->GetClassName());
//           vtkThreadedStreamingPipeline::Push(e[j], task.info);
//         }
//     }
//   }
  return NULL;
}
