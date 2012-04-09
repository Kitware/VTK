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
#include "vtkCommand.h"
#include "vtkComputingResources.h"
#include "vtkExecutiveCollection.h"
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

#include <set>
#include <vtksys/hash_map.hxx>
#include <vector>
#include <vtksys/hash_set.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkExecutionScheduler);

vtkInformationKeyMacro(vtkExecutionScheduler, TASK_PRIORITY, Integer);

//----------------------------------------------------------------------------
class Task 
{
public:
  Task(int _priority = -1, vtkExecutive *_exec = NULL, vtkInformation *_info = NULL) 
  {
    this->priority = _priority;
    this->exec = _exec;
    this->info = _info;
  }
  
  int             priority;
  vtkExecutive   *exec;
  vtkInformation *info;
};
  
//----------------------------------------------------------------------------
class TaskWeakOrdering 
{
public:
  bool operator()(const Task& t1,
                  const Task& t2) const 
  {
    return t1.priority < t2.priority;
  }
};

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
typedef std::vector<vtkExecutive*>                       vtkExecutiveVector;

//----------------------------------------------------------------------------
class vtkExecutionScheduler::implementation
{
public:
  // The containing object
  vtkExecutionScheduler*  Scheduler;

  // Some convenient type definitions for STL containers
  typedef vtksys::hash_map<vtkExecutive*, int, 
    vtkExecutiveHasher>                                 ExecutiveIntHashMap;
  typedef std::pair<int, int>                        Edge;
  class                                                 EdgeHasher;
  typedef vtksys::hash_set<Edge, EdgeHasher>            EdgeSet;
  typedef std::multiset<Task, TaskWeakOrdering>      TaskPriorityQueue;
  typedef std::vector<vtkMutexLock*>                 MutexLockVector;
  typedef std::vector<vtkThreadMessager*>            MessagerVector;
  class EdgeHasher 
  {
  public:
    size_t operator()(const Edge &e) const 
    {
      return (size_t)((e.first << 16) +  e.second);
    }
  };

  vtkExecutiveSet           ExecutingTasks;
  TaskPriorityQueue         PrioritizedTasks;
  ExecutiveIntHashMap       DependencyNodes;
  EdgeSet                   DependencyEdges;
  MessagerVector            TaskDoneMessagers;
  MutexLockVector           InputsReleasedLocks;
  MessagerVector            InputsReleasedMessagers;
  int                       CurrentPriority;
  
  // Description:
  // Start from the exec and go all the way up to the sources (modules
  // without any inputs), then call TraverseDownToSink to update edges
  void FindAndTraverseFromSources(vtkExecutive *exec, vtkExecutiveSet &visited);
  
  // Description:
  // Actual traverse down the network, for each nodes, construct and
  // add edges connecting all of its upstream modules to itself to the
  // dependency graph
  void TraverseDownToSink(vtkExecutive *exec, vtkExecutiveSet &upstream,
                          vtkExecutiveSet &visited);
  
  // Description:
  // Actual traverse down the network, for each nodes, construct and
  // add edges connecting all of its upstream modules to itself to the
  // dependency graph
  void CollectDownToSink(vtkExecutive *exec, vtkExecutiveSet &visited,
                         vtkExecutiveVector &graph);
  
  // Description:
  // A task can be executed if none of its predecessor tasks are still
  // on the queue. This only makes sense for tasks that are currently
  // on the queue, thus, an iterator is provided instead of the task
  // itself.
  bool CanExecuteTask(TaskPriorityQueue::const_iterator ti);
  // Description:
  // Spawn a thread to execute a module
  void Execute(const Task &task);

  // Description:
  // Check if the given exec is a new module or not. If it is then
  // traverse the network to update dependency edges for its connected
  // subgraph
  void UpdateDependencyGraph(vtkExecutive *exec);

  // Description:
  // Add the module exec to the set of dependency nodes if it is not
  // already there and return its node id number
  int AddToDependencyGraph(vtkExecutive *exec);

  // Description:
  // Add the given executive to the execution queue for later
  // execution
  void AddToQueue(vtkExecutive *exec, vtkInformation *info);
  
  // Description:
  // Obtain the priority from the information object if it is given,
  // otherwise, use a priority assigned from the scheduler
  int AcquirePriority(vtkInformation *info);
};

//----------------------------------------------------------------------------
static vtkExecutionScheduler *globalScheduler = NULL;

//----------------------------------------------------------------------------
void * vtkExecutionScheduler_ScheduleThread(void *data);
void * vtkExecutionScheduler_ExecuteThread(void *data);

//----------------------------------------------------------------------------
vtkExecutionScheduler* vtkExecutionScheduler::GetGlobalScheduler()
{
  if (!globalScheduler)
    {
    globalScheduler = vtkExecutionScheduler::New();
    }
  return globalScheduler;
}

//----------------------------------------------------------------------------
vtkExecutionScheduler::vtkExecutionScheduler()
  : Implementation(new implementation)
{
  this->Resources = vtkComputingResources::New();
  this->Resources->ObtainMaximumResources();
  this->ResourceMessager = vtkThreadMessager::New();
  this->ScheduleLock = vtkMutexLock::New();
  this->ScheduleMessager = vtkThreadMessager::New();
  this->ScheduleThreader = vtkMultiThreader::New();
  this->ScheduleThreader->SetNumberOfThreads(1);
  this->ScheduleThreadId = -1;
  this->Implementation->Scheduler = this;
  this->Implementation->CurrentPriority = 0;
}

//----------------------------------------------------------------------------
vtkExecutionScheduler::~vtkExecutionScheduler()
{
  this->Resources->Delete();
  this->ResourceMessager->Delete();
  this->ScheduleLock->Delete();
  this->ScheduleMessager->Delete();
  this->ScheduleThreader->Delete();
  delete this->Implementation;
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::SchedulePropagate(vtkExecutiveCollection *execs, vtkInformation* vtkNotUsed(info))
{
  execs->InitTraversal();
  vtkExecutiveSet    visited;
  vtkExecutiveVector graph;
  for (vtkExecutive *e = execs->GetNextItem(); e != 0; e = execs->GetNextItem())
    {
    this->Implementation->CollectDownToSink(e, visited, graph);
    }
  
  for (vtkExecutiveVector::iterator vi=graph.begin();
       vi!=graph.end(); vi++)
    {
    (*vi)->Update();
    vtkAlgorithm *rep =(*vi)->GetAlgorithm();
    if (rep->IsA("vtkDataRepresentation"))
      {
      rep->InvokeEvent(vtkCommand::UpdateEvent, NULL);
      }
    }
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::Schedule(vtkExecutiveCollection *execs, vtkInformation *info)
{
  if (this->ScheduleThreadId == -1)
    {
    this->ScheduleThreadId = this->ScheduleThreader->
      SpawnThread((vtkThreadFunctionType)(vtkExecutionScheduler_ScheduleThread), this);
    }
  this->ScheduleLock->Lock();
  vtkExecutiveVector G;
  execs->InitTraversal();
  for (vtkExecutive *e = execs->GetNextItem(); e != 0; e = execs->GetNextItem()) 
    {
    if (this->Implementation->ExecutingTasks.find(e) != this->Implementation->ExecutingTasks.end())
      {
      return;
      }
    if (this->Implementation->DependencyNodes.find(e) == this->Implementation->DependencyNodes.end()) 
      {
      this->Implementation->UpdateDependencyGraph(e);
      }
    G.push_back(e);
    }

  // Create an adjacency matrix
  unsigned i, j, k, p;
  unsigned N = (unsigned)G.size();
  int *A = (int*)malloc(N*N*sizeof(int));
  int *degree = (int*)malloc(N*sizeof(int));
  memset(A, 0, N*N*sizeof(int));
  memset(degree, 0, N*sizeof(int));
  for (i = 0; i < N; i++) 
    {
    int src = (*(this->Implementation->DependencyNodes.find(G[i]))).second;
    for (j = 0; j < N; j++) 
      {
      int dst = (*(this->Implementation->DependencyNodes.find(G[j]))).second;
      if (this->Implementation->DependencyEdges.find(implementation::Edge(src, dst)) !=
          this->Implementation->DependencyEdges.end()) 
        {
        A[i*N+j] = 1;
        degree[j]++;
        }
      }
    }
  
  unsigned *S = (unsigned*)malloc(N*sizeof(unsigned));
  k = 0;
  for (j = 0; j < N; j++)
    {
    if (degree[j] == 0)
      {
      S[k++] = j;
      }
    }
  p = 0;
  while (p < k) 
    {
    i = S[p++];
    this->Implementation->AddToQueue(G[i], info);
    for (j = 0; j < N; j++)
      if (A[i*N+j]) 
        {
        degree[j]--;
        A[i*N+j] = 0;
        if (degree[j] == 0)
          {
          S[k++] = j;
          }
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
void vtkExecutionScheduler::implementation::UpdateDependencyGraph(vtkExecutive *exec)
{
  vtkExecutiveSet visited;
  this->FindAndTraverseFromSources(exec, visited);
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::implementation::FindAndTraverseFromSources
(vtkExecutive *exec, vtkExecutiveSet &visited)
{
  if (visited.find(exec) != visited.end())
    {
    return;
    }
  visited.insert(exec);
  bool isSource = true;
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
      if (e) 
        {
        isSource = false;
        this->FindAndTraverseFromSources(e, visited);
        }
      }
    }
  if (isSource) 
    {
    vtkExecutiveSet upstream;
    vtkExecutiveSet downVisited;
    this->TraverseDownToSink(exec, upstream, downVisited);
    }
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::implementation::TraverseDownToSink
(vtkExecutive *exec, vtkExecutiveSet &upstream, vtkExecutiveSet &visited)
{
  if (visited.find(exec)!=visited.end())
    {
    return;
    }
  
  // Now mark all edges from upstream modules to exec as dependency edges
  int vId = this->AddToDependencyGraph(exec);
  for (vtkExecutiveSet::const_iterator it = upstream.begin(); it != upstream.end(); it++)
    {
    implementation::ExecutiveIntHashMap::iterator hit = this->DependencyNodes.find(*it);
    this->DependencyEdges.insert(Edge((*hit).second, vId));
    }
  
  // Mark as visited
  visited.insert(exec);

  // Then insert it to the upstream list for going down
  upstream.insert(exec);
  for(int i = 0; i < exec->GetNumberOfOutputPorts(); ++i) 
    {
    vtkInformation* info = exec->GetOutputInformation(i);
    int consumerCount = vtkExecutive::CONSUMERS()->Length(info);
    vtkExecutive** e = vtkExecutive::CONSUMERS()->GetExecutives(info);
    for (int j = 0; j < consumerCount; j++)
      if (e[j]) 
        {
        this->TraverseDownToSink(e[j], upstream, visited);
        }
    }
  
  // Take it out of the upstream and prepare for back-tracking
  upstream.erase(exec);
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::implementation::CollectDownToSink
(vtkExecutive *exec, vtkExecutiveSet &visited, vtkExecutiveVector &graph)
{
  if (visited.find(exec)!=visited.end())
    {
    return;
    }
  
  // Mark as visited
  visited.insert(exec);

  // Add it to the ordered graph
  graph.push_back(exec);

  // Then traverse down
  for(int i = 0; i < exec->GetNumberOfOutputPorts(); ++i) 
    {
    vtkInformation* info = exec->GetOutputInformation(i);
    int consumerCount = vtkExecutive::CONSUMERS()->Length(info);
    vtkExecutive** e = vtkExecutive::CONSUMERS()->GetExecutives(info);
    for (int j = 0; j < consumerCount; j++)
      {
      if (e[j]) 
        {
        this->CollectDownToSink(e[j], visited, graph);
        }
      }
    }
}

//----------------------------------------------------------------------------
int vtkExecutionScheduler::implementation::AddToDependencyGraph(vtkExecutive *exec)
{
  // We never remove vertices, it's ok to just return the size of
  // DependencyGraph as a node id
  implementation::ExecutiveIntHashMap::iterator hit = this->DependencyNodes.find(exec);
  // Check if this is a new module or an untouched sub-network
  int vId = -1;
  if (hit == this->DependencyNodes.end()) 
    {
    vId = (int)this->DependencyNodes.size();
    this->DependencyNodes[exec] = vId;
    
    // Make sure that we have enough thread messagers for this vId
    while (this->TaskDoneMessagers.size()<=(size_t)vId)
      {
      this->TaskDoneMessagers.push_back(vtkThreadMessager::New());
      }
    
    while (this->InputsReleasedMessagers.size()<=(size_t)vId) 
      {
      this->InputsReleasedMessagers.push_back(vtkThreadMessager::New());
      this->InputsReleasedLocks.push_back(vtkMutexLock::New());
      }
    }
  // We have this module in our cache before
  else 
    {
    vId = (*hit).second;
    }
  return vId;
}

//----------------------------------------------------------------------------
int vtkExecutionScheduler::implementation::AcquirePriority(vtkInformation * info)
{
  int priority;
  if (info && info->Has(TASK_PRIORITY())) 
    {
    priority = info->Get(TASK_PRIORITY());
    }
  else 
    {
    priority = this->CurrentPriority++;
    }
  return priority;
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::implementation::AddToQueue(vtkExecutive *exec, vtkInformation * info)
{
  int priority = this->AcquirePriority(info);
  this->PrioritizedTasks.insert(Task(priority, exec, info));
  vtkMutexLock *lock = this->Scheduler->GetInputsReleasedLock(exec);
  if (lock) 
    {
//     fprintf(stderr, "A %s\n", exec->GetAlgorithm()->GetClassName());
    lock->Lock();
    }
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::WaitUntilDone(vtkExecutiveCollection *execs)
{
  vtkExecutiveSet::const_iterator it;
  execs->InitTraversal();
  for (vtkExecutive *e = execs->GetNextItem(); e != 0; e = execs->GetNextItem()) 
    {
    this->WaitForTaskDone(e);
    }
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::WaitUntilAllDone()
{
  while (true) 
    {
    vtkExecutive *exec = NULL;
    this->ScheduleLock->Lock();
    if (this->Implementation->PrioritizedTasks.size() > 0)
      exec = (*(this->Implementation->PrioritizedTasks.begin())).exec;
    this->ScheduleLock->Unlock();
    if (exec)
      this->WaitForTaskDone(exec);
    else
      break;
    }
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::WaitUntilReleased(vtkExecutiveCollection *execs)
{
  vtkExecutiveSet::const_iterator it;
  execs->InitTraversal();
  for (vtkExecutive *e = execs->GetNextItem(); e != 0; e = execs->GetNextItem())
    {
    this->WaitForInputsReleased(e);
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
  if (lock) 
    {
    lock->Lock();
    lock->Unlock();
    }
//   vtkThreadMessager *messager = this->GetInputsReleasedMessager(exec);
//   if (messager)
//     messager->WaitForMessage();
}

//----------------------------------------------------------------------------
vtkThreadMessager* vtkExecutionScheduler::GetTaskDoneMessager(vtkExecutive *exec)
{
  implementation::ExecutiveIntHashMap::iterator hit = 
    this->Implementation->DependencyNodes.find(exec);
  if (hit != this->Implementation->DependencyNodes.end()) 
    {
    implementation::TaskPriorityQueue::const_iterator ti;
    for (ti = this->Implementation->PrioritizedTasks.begin();
         ti != this->Implementation->PrioritizedTasks.end(); ti++) 
      {
      if ((*ti).exec==exec)
        {
        return this->Implementation->TaskDoneMessagers[(*hit).second];
        }
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkMutexLock* vtkExecutionScheduler::GetInputsReleasedLock(vtkExecutive *exec)
{
  implementation::ExecutiveIntHashMap::iterator hit = 
    this->Implementation->DependencyNodes.find(exec);
  if (hit != this->Implementation->DependencyNodes.end()) 
    {
    implementation::TaskPriorityQueue::const_iterator ti;
    for (ti = this->Implementation->PrioritizedTasks.begin();
         ti != this->Implementation->PrioritizedTasks.end(); ti++) 
      {
      if ((*ti).exec == exec)
        {
        return this->Implementation->InputsReleasedLocks[(*hit).second];
        }
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkThreadMessager* vtkExecutionScheduler::GetInputsReleasedMessager(vtkExecutive *exec)
{
  implementation::ExecutiveIntHashMap::iterator hit = 
    this->Implementation->DependencyNodes.find(exec);
  if (hit != this->Implementation->DependencyNodes.end()) 
    {
    implementation::TaskPriorityQueue::const_iterator ti;
    for (ti = this->Implementation->PrioritizedTasks.begin();
         ti != this->Implementation->PrioritizedTasks.end(); ti++) 
      {
      if ((*ti).exec==exec)
        {
        return this->Implementation->InputsReleasedMessagers[(*hit).second];
        }
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::ReleaseResources(vtkExecutive *exec)
{
  vtkThreadedStreamingPipeline *threadedExec = vtkThreadedStreamingPipeline::
    SafeDownCast(exec);
  if (threadedExec) 
    {
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
  if (threadedExec) 
    {
    while (!this->Resources->Reserve(threadedExec->GetResources())) 
      {
      this->ResourceMessager->WaitForMessage();
      }
    }
}

//----------------------------------------------------------------------------
bool vtkExecutionScheduler::implementation::CanExecuteTask
(TaskPriorityQueue::const_iterator taskIter)
{
  if (this->ExecutingTasks.find((*taskIter).exec) != this->ExecutingTasks.end())
    {
    return false;
    }
  ExecutiveIntHashMap::iterator hit = this->DependencyNodes.find((*taskIter).exec);
  if (hit == this->DependencyNodes.end())
    {
    return true;
    }
  int dst = (*hit).second;
  TaskPriorityQueue::const_iterator ti;
  for (ti = this->PrioritizedTasks.begin();
       ti != taskIter; ti++)
    {
    if ((*ti).priority > (*taskIter).priority)
      {
      break;
      }
    hit = this->DependencyNodes.find((*ti).exec);
    int src = (*hit).second;
    if (this->DependencyEdges.find(Edge(src, dst)) != this->DependencyEdges.end())
      {
      return false;
      }
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::RescheduleFrom(vtkExecutive *exec,
                                           vtkComputingResources *resources) 
{
  vtkExecutiveVector upstream;
  // Compute the total time
  float totalUpStreamTime = 0.0;
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
      if (e && vtkThreadedStreamingPipeline::SafeDownCast(e))
        {
        upstream.push_back(e);
        }
      }
    }
  for (size_t i = 0; i < upstream.size(); i++)
    {
    totalUpStreamTime += vtkThreadedStreamingPipeline::SafeDownCast(upstream[i])->LastDataRequestTimeFromSource;
    }

  // Then distribute the resources based on those ratios
  vtkProcessingUnitResource *totalResources[] = 
    {
      resources->GetResourceFor(vtkThreadedStreamingPipeline::PROCESSING_UNIT_CPU),
      resources->GetResourceFor(vtkThreadedStreamingPipeline::PROCESSING_UNIT_GPU),
    };
  for (size_t i = 0; i < upstream.size(); i++) 
    {
    float ratio = vtkThreadedStreamingPipeline::
      SafeDownCast(upstream[i])->LastDataRequestTimeFromSource/totalUpStreamTime;
    for (size_t j = 0; j < sizeof(totalResources)/sizeof(vtkProcessingUnitResource*); j++) 
      {
      vtkProcessingUnitResource* moduleResource = vtkThreadedStreamingPipeline::
        SafeDownCast(upstream[i])->GetResources()->
        GetResourceFor(totalResources[j]->ProcessingUnit());
      moduleResource->IncreaseByRatio(ratio, totalResources[j]);
      }
    }
  
  // Try to reserve upstream
  for (size_t i = 0; i < upstream.size(); i++)
    {
    this->RescheduleFrom(upstream[i],
                         vtkThreadedStreamingPipeline::
                         SafeDownCast(upstream[i])->GetResources());
    }
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::RescheduleNetwork(vtkExecutive *sink) 
{
  this->Resources->ObtainMaximumResources();
  this->RescheduleFrom(sink, this->Resources);
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::ClassInitialize()
{
  // Currently empty - initialize late when required.
}

//----------------------------------------------------------------------------
void vtkExecutionScheduler::ClassFinalize()
{
  // Clean up our singleton (if it was ever initalized).
  if (globalScheduler)
    {
    globalScheduler->FastDelete();
    }
}

//----------------------------------------------------------------------------
typedef struct 
{
  vtkExecutionScheduler *scheduler;
  Task task;
} ExecutionData;
//----------------------------------------------------------------------------
void vtkExecutionScheduler::implementation::Execute(const Task &task)
{
  ExecutionData *eData = new ExecutionData();
  eData->scheduler = this->Scheduler;
  eData->task = task;
  this->Scheduler->ScheduleThreader->SpawnThread((vtkThreadFunctionType)(vtkExecutionScheduler_ExecuteThread), eData);
}

//----------------------------------------------------------------------------
void * vtkExecutionScheduler_ScheduleThread(void *data)
{
  vtkExecutionScheduler *self = static_cast<vtkExecutionScheduler*>
    (static_cast<vtkMultiThreader::ThreadInfo*>(data)->UserData);
  while (true) 
    {
    self->ScheduleLock->Lock();
    bool needToWait = true;
    vtkExecutionScheduler::implementation::TaskPriorityQueue::iterator ti;
    for (ti = self->Implementation->PrioritizedTasks.begin();
         ti != self->Implementation->PrioritizedTasks.end(); ti++) 
      {
      if (self->Implementation->CanExecuteTask(ti)) 
        {
        vtkThreadedStreamingPipeline *exec = vtkThreadedStreamingPipeline::
          SafeDownCast((*ti).exec);
        if (self->Resources->Reserve(exec->GetResources())) 
          {
          needToWait = false;
          self->Implementation->ExecutingTasks.insert(exec);
          self->ScheduleLock->Unlock();
          self->Implementation->Execute(*ti);
          break;
          }
        }
      }
    if (needToWait) 
      {
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
  Task task = eData->task;
  vtkThreadedStreamingPipeline *exec = vtkThreadedStreamingPipeline::
    SafeDownCast(task.exec);
  vtkThreadMessager *messager = self->GetTaskDoneMessager(task.exec);
  vtkMutexLock *lock = self->GetInputsReleasedLock(task.exec);
  exec->GetResources()->Deploy(exec, task.info);
  self->ScheduleLock->Lock();
  self->Implementation->PrioritizedTasks.erase(task);
  self->Implementation->ExecutingTasks.erase(exec);
  self->Resources->Collect(exec->GetResources());
  self->ResourceMessager->SendWakeMessage();
  self->ScheduleLock->Unlock();
  exec->ReleaseInputs();
  self->ScheduleMessager->SendWakeMessage();
  if (task.info && task.info->Has(vtkThreadedStreamingPipeline::AUTO_PROPAGATE()))
    {
      fprintf(stderr, "Push DOWN from %s\n", exec->GetAlgorithm()->GetClassName());
    exec->Push(task.info);
    }
  if (messager)
    {
    messager->SendWakeMessage();
    }
  delete eData;
  fprintf(stderr, "Release now\n");
  lock->Unlock();
  return NULL;
}
