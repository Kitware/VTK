/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTimerLog.h
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
// .NAME vtkTimerLog - Timer support and logging
// .SECTION Description
// vtkTimerLog contains walltime and cputime measurements associated
// with a given event.  These results can be later analyzed when
// "dumping out" the table.
//
// In addition, vtkTimerLog allows the user to simply get the current
// time, and to start/stop a simple timer separate from the timing
// table logging.

#ifndef __vtkTimerLog_h
#define __vtkTimerLog_h

#include "vtkSystemIncludes.h"

#ifdef _WIN32
#include <sys/types.h>
#include <sys/timeb.h>
#else
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/times.h>
#endif

// var args
#ifndef _WIN32
#include <unistd.h>
#endif
#include <stdarg.h>

// select stuff here is for sleep method
#ifndef NO_FD_SET
#   define SELECT_MASK fd_set
#else
#   ifndef _AIX
	typedef long fd_mask;
#   endif
#   if defined(_IBMR2)
#	define SELECT_MASK void
#   else
#	define SELECT_MASK int
#   endif
#endif


#include "vtkObject.h"
#define VTK_LOG_EVENT_LENGTH 40

//BTX
typedef struct
{
  float WallTime;
  int CpuTicks;
  char Event[VTK_LOG_EVENT_LENGTH];
} vtkTimerLogEntry;
//ETX

// The microsoft compiler defines this as a macro, so
// undefine it here
#undef GetCurrentTime

class VTK_EXPORT vtkTimerLog : public vtkObject 
{
public:
  static vtkTimerLog *New();

  vtkTypeMacro(vtkTimerLog,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the maximum number of entries allowed in the timer log
  static void SetMaxEntries(int a);
  static int  GetMaxEntries();

//BTX
  // Description:
  // Record a timing event.  The event is represented by a formatted
  // string.
  static void FormatAndMarkEvent(char *EventString, ...);
//ETX
  
  // Description:
  // Write the timing table out to a file.  Calculate some helpful
  // statistics (deltas and  percentages) in the process.
  static void DumpLog(char *filename);

  // Description:
  // Record a timing event and capture wall time and cpu ticks.
  static void MarkEvent(char *EventString);

  // Description:
  // Clear the timing table.  walltime and cputime will also be set
  // to zero when the first new event is recorded.
  static void ResetLog();

  // Description:
  // Allocate timing table with MaxEntries elements.
  static void AllocateLog();

  // Description:
  // Returns the elapsed number of seconds since January 1, 1970. This
  // is also called Universal Coordinated Time.
  static double GetCurrentTime();

  // Description:
  // Returns the CPU time for this process
  // On Win32 platforms this actually returns wall time.
  static double GetCPUTime();

  // Description:
  // Set the StartTime to the current time. Used with GetElapsedTime().
  void StartTimer();

  // Description:
  // Sets EndTime to the current time. Used with GetElapsedTime().
  void StopTimer();

  // Description:
  // Returns the difference between StartTime and EndTime as 
  // a floating point value indicating the elapsed time in seconds.
  double GetElapsedTime();

protected:
  vtkTimerLog() {this->StartTime=0;}; //insure constructor/destructor protected
  ~vtkTimerLog() {};
  vtkTimerLog(const vtkTimerLog&);
  void operator=(const vtkTimerLog&);

  static int               MaxEntries;
  static int               NextEntry;
  static int               WrapFlag;
  static int               TicksPerSecond;
  static vtkTimerLogEntry *TimerLog;

#ifdef _WIN32
  static timeb             FirstWallTime;
  static timeb             CurrentWallTime;
#else
  static timeval           FirstWallTime;
  static timeval           CurrentWallTime;
  static tms               FirstCpuTicks;
  static tms               CurrentCpuTicks;
#endif

  // instance variables to support simple timing functionality,
  // separate from timer table logging.
  double StartTime;
  double EndTime;

  //BTX
  static void DumpEntry(ostream& os, int index, float time, float deltatime,
                        int tick, int deltatick, char *event);
  //ETX

};


//
// Set built-in type.  Creates member Set"name"() (e.g., SetVisibility());
//
#define vtkTimerLogMacro(string) \
  { \
      vtkTimerLog::FormatAndMarkEvent("Mark: In %s, line %d, class %s: %s", \
			      __FILE__, __LINE__, this->GetClassName(), string); \
  }

#endif
