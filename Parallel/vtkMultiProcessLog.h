/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiProcessLog.h
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
// .NAME vtkMultiProcessLog - Manages logs for multiple threads/processes.
// .SECTION Description
// Needs to be thread safe in the future (vtkTimerLog is not).

#ifndef __vtkMultiProcessLog_h
#define __vtkMultiProcessLog_h

#include "vtkThreadSafeLog.h"

#define VTK_MULTI_PROCESS_LOG_MAX 1000

class VTK_PARALLEL_EXPORT vtkMultiProcessLog : public vtkObject
{
public:
  static vtkMultiProcessLog *New();
  const char *GetClassName() {return "vtkMultiProcessLog";};

  // Description:
  // I want all the events in one process to share a log.
  // Note: Each process must set its own log.
  // Since I cannot initialize the array in a thread safe manner,
  // the logs are not reference counted.
  static void SetTimerLog(vtkThreadSafeLog *log);
  static vtkThreadSafeLog *GetTimerLog();

  // Decription:
  // Dumps logs sequentially into a file.  
  // Each process should call this.
  static void DumpLog(char *filename);

protected:

  vtkMultiProcessLog() {}; //insure constructur/destructor protected
  ~vtkMultiProcessLog() {};

private:
  vtkMultiProcessLog(const vtkMultiProcessLog&);  // Not implemented.
  void operator=(const vtkMultiProcessLog&);  // Not implemented.
};



#endif
