/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadSafeLog.h
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
// .NAME vtkThreadSafeLog - Saves tag/float entries
// .SECTION Description
// For timing Port stuff. Float values are saved with associated string tags.

#ifndef __vtkThreadSafeLog_h
#define __vtkThreadSafeLog_h


#include "vtkObject.h"
#include "vtkTimerLog.h"


#define VTK_THREAD_SAFE_LOG_MAX 1000

class VTK_EXPORT vtkThreadSafeLog : public vtkObject
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
  vtkThreadSafeLog(const vtkThreadSafeLog&);
  void operator=(const vtkThreadSafeLog&);


  char *Tags[VTK_THREAD_SAFE_LOG_MAX];
  float Values[VTK_THREAD_SAFE_LOG_MAX];
  int NumberOfEntries;

  vtkTimerLog *Timer;
};



#endif
