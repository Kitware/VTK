/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPIEventLog.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkMPIEventLog - Class for logging and timing.

// .SECTION Description
// This class is wrapper around MPE event logging functions
// (available from Argonne National Lab/Missippi State
// University). It allows users to create events with names
// and log them. Different log file formats can be generated
// by changing MPE's configuration. Some of these formats are
// binary (for examples SLOG and CLOG) and can be analyzed with
// viewers from ANL. ALOG is particularly useful since it is
// text based and can be processed with simple scripts.

// .SECTION See Also
// vtkTimerLog vtkMPIController vtkMPICommunicator

#ifndef __vtkMPIEventLog_h
#define __vtkMPIEventLog_h

#include "vtkObject.h"
#include "vtkMPIController.h"

class VTK_PARALLEL_EXPORT vtkMPIEventLog : public vtkObject
{

public:

  vtkTypeMacro(vtkMPIEventLog,vtkObject);
  
  // Description:
  // Construct a vtkMPIEventLog with the following initial state:
  // Processes = 0, MaximumNumberOfProcesses = 0.
  static vtkMPIEventLog* New();
  
  // Description:
  // Used to initialize the underlying mpe event.
  // HAS TO BE CALLED BY ALL PROCESSES before any event 
  // logging is done.
  // It takes a name and a description for the graphical
  // representation, for example, "red:vlines3". See
  // mpe documentation for details.
  // Returns 0 on MPI failure (or aborts depending on
  // MPI error handlers)
  int SetDescription(const char* name, const char* desc);

  // Description:
  // These methods have to be called once on all processors
  // before and after invoking any logging events.
  // The name of the logfile is given by fileName.
  // See mpe documentation for file formats.
  static void InitializeLogging();
  static void FinalizeLogging(const char* fileName);

  // Description:
  // Issue start and stop events for this log entry.
  void StartLogging();
  void StopLogging();

  virtual void PrintSelf(ostream& os, vtkIndent indent);

protected:

  vtkMPIEventLog();
  ~vtkMPIEventLog();

  static int LastEventId;
  int Active;
  int BeginId;
  int EndId;
private:
  vtkMPIEventLog(const vtkMPIEventLog&);  // Not implemented.
  void operator=(const vtkMPIEventLog&);  // Not implemented.
};

#endif




