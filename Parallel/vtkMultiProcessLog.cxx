/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiProcessLog.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/

#include "vtkMultiProcessLog.h"
#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"

// Is there a thread safe way to initialize this array?
// A log for each process.
vtkThreadSafeLog *VTK_TIMER_LOGS[VTK_MULTI_PROCESS_LOG_MAX];


//----------------------------------------------------------------------------
vtkMultiProcessLog* vtkMultiProcessLog::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMultiProcessLog");
  if(ret)
    {
    return (vtkMultiProcessLog*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMultiProcessLog;
}


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

  





