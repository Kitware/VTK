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

// .SECTION See Also
// vtkComputingResources vtkThreadedStreamingPipeline

#ifndef __vtkExecutionScheduler_h
#define __vtkExecutionScheduler_h

#include "vtkObject.h"

class vtkExecutive;
class vtkComputingResources;
class vtkMultiThreader;
class vtkMutexLock;
class vtkThreadMessager;
class vtkInformation;
class vtkInformationIntegerKey;
class vtkExecutiveCollection;

class VTK_FILTERING_EXPORT vtkExecutionScheduler : public vtkObject
{
public:
  static vtkExecutionScheduler* New();
  vtkTypeMacro(vtkExecutionScheduler,vtkObject);
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Return the global instance of the scheduler 
  static vtkExecutionScheduler *GetGlobalScheduler();

 // Description:
  // Key to store the priority of a task
  static vtkInformationIntegerKey* TASK_PRIORITY();

  // Description:  
  // Put the current set of executives (modules) to the be scheduled given its
  // dependency graph which will be used to compute the set
  // topological orders
  void Schedule(vtkExecutiveCollection *execs, vtkInformation *info);
  
  // Description:
  // Put the current set of executives (modules) to the be scheduled
  // given its dependency graph which will be used to compute the set
  // topological orders. Then wait for their execution to be complete
  void SchedulePropagate(vtkExecutiveCollection *execs, vtkInformation *info);
  
  // Description:
  // Wait until the current set of executives (modules) have finished executing
  void WaitUntilDone(vtkExecutiveCollection *execs);
  
  // Description:  
  // Wait until the current set of executives (modules) have their inputs released
  void WaitUntilReleased(vtkExecutiveCollection *execs);

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

protected:
  vtkExecutionScheduler();
  ~vtkExecutionScheduler();

  vtkComputingResources       *Resources;
  vtkThreadMessager           *ScheduleMessager;
  vtkThreadMessager           *ResourceMessager;
  vtkMutexLock                *ScheduleLock;
  vtkMultiThreader            *ScheduleThreader;
  int                          ScheduleThreadId;

//BTX
  class implementation;
  implementation* const Implementation;
  friend class implementation;

  // Description:  
  // The scheduling thread that is responsible for queueing up module
  // execution in the right order
  friend void * vtkExecutionScheduler_ScheduleThread(void *data);

  // Description:  
  // Execute thread function that is responsible for forking process
  // for each module
  friend void * vtkExecutionScheduler_ExecuteThread(void *data);

//ETX

private:
  vtkExecutionScheduler(const vtkExecutionScheduler&);  // Not implemented.
  void operator=(const vtkExecutionScheduler&);  // Not implemented.
  
};

#endif
