/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadSafeLog.h
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
// .NAME vtkThreadSafeLog - Saves tag/float entries
// .SECTION Description
// For timing Port stuff. Float values are saved with associated string tags.

#ifndef __vtkThreadSafeLog_h
#define __vtkThreadSafeLog_h


#include "vtkObject.h"

#define VTK_THREAD_SAFE_LOG_MAX 1000

class VTK_PARALLEL_EXPORT vtkThreadSafeLog : public vtkObject
{
public:
  static vtkThreadSafeLog *New();
  const char *GetClassName() {return "vtkThreadSafeLog";};

  // Description:
  // some of the timeing features of vtkTimerLog.
  void StartTimer() { this->Timer->StartTimer();}
  void StopTimer() { this->Timer->StopTimer();}
  double GetElapsedTime() { return this->Timer->GetElapsedTime();}

  // Decription:
  // Save a tag/value pair. 
  void AddEntry(char *tag, float value);

  // Description:
  // Write the timing table out to a file.  
  // if mode is out, then two lines are printed: 
  // Tags on first, values on second.
  // If mode is iso::app, then only the line with values is printed.
  void DumpLog(char *filename, int mode = ios::out);

protected:

  vtkThreadSafeLog(); //insure constructur/destructor protected
  ~vtkThreadSafeLog();


  char *Tags[VTK_THREAD_SAFE_LOG_MAX];
  float Values[VTK_THREAD_SAFE_LOG_MAX];
  int NumberOfEntries;

  vtkTimerLog *Timer;
private:
  vtkThreadSafeLog(const vtkThreadSafeLog&);  // Not implemented.
  void operator=(const vtkThreadSafeLog&);  // Not implemented.
};



#endif
