/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTimerLog.h
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
#ifndef _WIN32_WCE
#include <sys/types.h>
#include <sys/timeb.h>
#endif
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
#       define SELECT_MASK void
#   else
#       define SELECT_MASK int
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
  unsigned char Indent;
} vtkTimerLogEntry;
//ETX

// The microsoft compiler defines this as a macro, so
// undefine it here
#undef GetCurrentTime

class VTK_COMMON_EXPORT vtkTimerLog : public vtkObject 
{
public:
  static vtkTimerLog *New();

  vtkTypeRevisionMacro(vtkTimerLog,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This flag will turn loging of events off or on.  
  // By default, logging is on.
  static void SetLogging(int v) {vtkTimerLog::Logging = v;}
  static int GetLogging() {return vtkTimerLog::Logging;}
  static void LoggingOn() {vtkTimerLog::SetLogging(1);}
  static void LoggingOff() {vtkTimerLog::SetLogging(0);}

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
  // I want to time events, so I am creating this interface to
  // mark events that have a start and an end.  These events can be,
  // nested. The standard Dumplog ignores the indents.
  static void MarkStartEvent(char *EventString);
  static void MarkEndEvent(char *EventString);
//BTX
  static void DumpLogWithIndents(ostream *os, float threshold);
//ETX

  // Description:
  // Programatic access to events.  Indexed from 0 to num-1.
  static int GetNumberOfEvents();
  static int GetEventIndent(int i);
  static float GetEventWallTime(int i);
  static const char* GetEventString(int i);

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
  vtkTimerLog() {this->StartTime=0; this->EndTime = 0;}; //insure constructor/destructor protected
  ~vtkTimerLog() {};

  static vtkTimerLogEntry* GetEvent(int i);

  static int               Logging;
  static int               Indent;
  static int               MaxEntries;
  static int               NextEntry;
  static int               WrapFlag;
  static int               TicksPerSecond;
  static vtkTimerLogEntry *TimerLog;

#ifdef _WIN32
#ifndef _WIN32_WCE
  static timeb             FirstWallTime;
  static timeb             CurrentWallTime;
#else
  static FILETIME FirstWallTime;
  static FILETIME CurrentWallTime;
#endif
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

private:
  vtkTimerLog(const vtkTimerLog&);  // Not implemented.
  void operator=(const vtkTimerLog&);  // Not implemented.
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
