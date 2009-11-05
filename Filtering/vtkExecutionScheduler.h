/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExecutionScheduler.h

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
// .NAME vtkExecutionScheduler - Scheduling execution with
// thread/computing resources distributing
// .SECTION Description
// This is a class for balancing the computing resources throughout
// the network

#ifndef __vtkExecutionScheduler_h
#define __vtkExecutionScheduler_h

#include "vtkObject.h"
#include "vtkThreadedStreamingPipeline.h"
#include "vtkThreadedStreamingTypes.i"

class vtkComputingResources;
class vtkMultiThreader;
class vtkMutexLock;
class vtkThreadMessager;
class vtkInformation;

class VTK_FILTERING_EXPORT vtkExecutionScheduler : public vtkObject
{
public:
  static vtkExecutionScheduler* New();
  vtkTypeRevisionMacro(vtkExecutionScheduler,vtkObject);

  // Description:
  // Return the global instance of the scheduler 
  static vtkExecutionScheduler *GetGlobalScheduler();

  // Description:
  // Erase all cache information about the current execution network
  static vtkExecutionScheduler ClearDependencyCache();
  
 // Description:
  // Key to store the priority of a task
  static vtkInformationIntegerKey* TASK_PRIORITY();

  //BTX
  // Description:  
  // Put a set of executives (modules) to the be scheduled given its
  // dependency graph which will be used to compute the set
  // topological orders
  void Schedule(const vtkThreadedStreamingPipeline::vtkExecutiveSet &eSet,
                vtkInformation *info);
  
  // Description:
  // Wait until a set of executives (modules) have finished executing
  void WaitUntilDone(const vtkThreadedStreamingPipeline::vtkExecutiveSet &eSet);
  
  // Description:  
  // Wait until a set of executives (modules) have their inputs released
  void WaitUntilReleased(const vtkThreadedStreamingPipeline::vtkExecutiveSet &eSet);
  //ETX

  // Description:
  // Wait for all tasks to be done
  void WaitUntilAllDone();
  
  // Description:
  // Wait for a task that is on the scheduling queue to be done. If
  // the task is not there, this will return immediately. If the exec
  // is NULL, any task that is done will trigger this the return
  void WaitForTaskDone(vtkExecutive *exec);
  
  // Description:
  // Similar to WaitForTaskDone but return whenever input connections
  // of a task are released instead of done computing. But exec cannot
  // be NULL.
  void WaitForInputsReleased(vtkExecutive *exec);
  
  // Description:
  // Return the thread messager reserved for the given exec to notify
  // when it is done
  vtkThreadMessager* GetTaskDoneMessager(vtkExecutive *exec);

  // Description:
  // Return the thread messager reserved for the given exec to notify
  // when it releases its inputs
  vtkThreadMessager* GetInputsReleasedMessager(vtkExecutive *exec);

  // Description:
  // Return the mutex lock reserved for the given exec to notify
  // when it releases its inputs
  vtkMutexLock* GetInputsReleasedLock(vtkExecutive *exec);

  // Description:
  // Release the resources that are being used by the given exec
  void ReleaseResources(vtkExecutive *exec);

  // Description:
  // Re-acquire the resource released earlier by ReleaseResource
  void ReacquireResources(vtkExecutive *exec);

  // Description:  
  // Redistribute the thread resources over the network from a sink
  // with a maximum resource
  void RescheduleNetwork(vtkExecutive *sink);
  
  // Description:  
  // Redistribute the thread resources from a sink given a certain
  // amount of resource
  void RescheduleFrom(vtkExecutive *sink, vtkComputingResources *resources);
  
  vtkComputingResources       *Resources;
  vtkThreadMessager           *ScheduleMessager;
  vtkThreadMessager           *ResourceMessager;
  vtkMutexLock                *ScheduleLock;
  vtkMultiThreader            *ScheduleThreader;
  int                          ScheduleThreadId;

  //BTX
  class Task {
  public:
    Task(int _priority=-1, vtkExecutive *_exec=NULL, vtkInformation *_info=NULL) {
      this->priority = _priority;
      this->exec = _exec;
      this->info = _info;
    }
    int             priority;
    vtkExecutive   *exec;
    vtkInformation *info;
  };
  
  class TaskWeakOrdering {
  public:
    bool operator()(const Task& t1,
                    const Task& t2) const {
      return t1.priority < t2.priority;
    }
  };
  //ETX
  
protected:
  vtkExecutionScheduler();
  ~vtkExecutionScheduler();

  //BTX
  // Some convenient type definitions for STL containers
  typedef vtksys::hash_map<vtkExecutive*, int,
    vtkThreadedStreamingPipeline::vtkExecutiveHasher>   ExecutiveIntHashMap;
  typedef vtkstd::pair<int, int>                        Edge;
  class                                                 EdgeHasher;
  typedef vtksys::hash_set<Edge, EdgeHasher>            EdgeSet;
  typedef vtkstd::multiset<Task, TaskWeakOrdering>      TaskPriorityQueue;
  typedef vtkstd::vector<vtkMutexLock*>                 MutexLockVector;
  typedef vtkstd::vector<vtkThreadMessager*>            MessagerVector;
  class EdgeHasher {
  public:
    size_t operator()(const Edge &e) const {
      return (size_t)((e.first << 16) +  e.second);
    };
  };

  vtkThreadedStreamingPipeline::
    vtkExecutiveSet         ExecutingTasks;
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
  void FindAndTraverseFromSources(vtkExecutive *exec,
                                  vtkThreadedStreamingPipeline::vtkExecutiveSet &visited);
  
  // Description:
  // Actual traverse down the network, for each nodes, construct and
  // add edges connecting all of its upstream modules to itself to the
  // dependency graph
  void TraverseDownToSink(vtkExecutive *exec,
                          vtkThreadedStreamingPipeline::vtkExecutiveSet &upstream,
                          vtkThreadedStreamingPipeline::vtkExecutiveSet &visited);
  
  // Description:
  // A task can be executed if none of its predecessor tasks are still
  // on the queue. This only makes sense for tasks that are currently
  // on the queue, thus, an iterator is provided instead of the task
  // itself.
  bool CanExecuteTask(TaskPriorityQueue::const_iterator ti);
  //ETX

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
  
  // Description:
  // Spawn a thread to execute a module
  void Execute(const Task &task);
  

  // Description:  
  // The scheduling thread that is responsible for queueing up module
  // execution in the right order
  friend void * vtkExecutionScheduler_ScheduleThread(void *data);

  // Description:  
  // Execute thread function that is responsible for forking process
  // for each module
  friend void * vtkExecutionScheduler_ExecuteThread(void *data);

private:
  vtkExecutionScheduler(const vtkExecutionScheduler&);  // Not implemented.
  void operator=(const vtkExecutionScheduler&);  // Not implemented.
};

#endif
