/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiProcessLog.cxx
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

#include "vtkMultiProcessLog.h"
#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"

// Is there a thread safe way to initialize this array?
// A log for each process.
vtkThreadSafeLog *VTK_TIMER_LOGS[VTK_MULTI_PROCESS_LOG_MAX];

vtkCxxRevisionMacro(vtkMultiProcessLog, "1.4");
vtkStandardNewMacro(vtkMultiProcessLog);

//----------------------------------------------------------------------------
void vtkMultiProcessLog::SetTimerLog(vtkThreadSafeLog *log)
{
  vtkMultiProcessController *controller;
  int myid;

  controller = vtkMultiProcessController::GetGlobalController();
  myid = controller->GetLocalProcessId();
  // Array not initialized: Is there any way to reference the log?
  VTK_TIMER_LOGS[myid] = log;
}
//----------------------------------------------------------------------------
vtkThreadSafeLog *vtkMultiProcessLog::GetTimerLog()
{
  vtkMultiProcessController *controller;
  vtkThreadSafeLog *log;
  int myid;

  controller = vtkMultiProcessController::GetGlobalController();
  myid = controller->GetLocalProcessId();
  log = VTK_TIMER_LOGS[myid];

  if (strcmp(log->GetClassName(), "vtkThreadSafeLog") != 0)
    {
    vtkGenericWarningMacro("Class does not match: Was the log set for this process?");
    return NULL;
    }

  return log;
}





//----------------------------------------------------------------------------
void vtkMultiProcessLog::DumpLog(char *filename)
{
  vtkMultiProcessController *controller;
  vtkThreadSafeLog *log;
  int myid, numProcs, tmp;

  controller = vtkMultiProcessController::GetGlobalController();
  myid = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();

  log = vtkMultiProcessLog::GetTimerLog();

  // force sequential dump
  if (myid == 0)
    {
    log->DumpLog(filename);
    }
  else
    {
    // receive blocks until myid-1 sends (is finished).
    controller->Receive(&tmp, 1, myid-1, 9877234);
    log->DumpLog(filename, ios::app);
    }
  if ( myid < numProcs-1)
    {
    // junk message to signal next process to go.
    tmp = 1;
    controller->Send(&tmp, 1, myid+1, 9877234);
    }

}

  





