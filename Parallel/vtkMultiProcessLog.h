/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiProcessLog.h
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
// .NAME vtkMultiProcessLog - Manages logs for multiple threads/processes.
// .SECTION Description
// Needs to be thread safe in the future (vtkTimerLog is not).

#ifndef __vtkMultiProcessLog_h
#define __vtkMultiProcessLog_h


#include "vtkObject.h"
#include "vtkThreadSafeLog.h"


#define VTK_MULTI_PROCESS_LOG_MAX 1000

class VTK_EXPORT vtkMultiProcessLog : public vtkObject
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
  vtkMultiProcessLog(const vtkMultiProcessLog&);
  void operator=(const vtkMultiProcessLog&);

};



#endif
