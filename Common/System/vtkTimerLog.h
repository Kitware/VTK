/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTimerLog.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

#ifndef vtkTimerLog_h
#define vtkTimerLog_h

#include "vtkCommonSystemModule.h" // For export macro
#include "vtkObject.h"

#ifdef _WIN32
#include <sys/types.h> // Needed for Win32 implementation of timer
#include <sys/timeb.h> // Needed for Win32 implementation of timer
#else
#include <time.h>      // Needed for unix implementation of timer
#include <sys/time.h>  // Needed for unix implementation of timer
#include <sys/types.h> // Needed for unix implementation of timer
#include <sys/times.h> // Needed for unix implementation of timer
#endif

// var args
#ifndef _WIN32
#include <unistd.h>    // Needed for unix implementation of timer
#endif

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


#define VTK_LOG_EVENT_LENGTH 40

//BTX
typedef struct
{
  double WallTime;
  int CpuTicks;
  char Event[VTK_LOG_EVENT_LENGTH];
  unsigned char Indent;
} vtkTimerLogEntry;
//ETX

class VTKCOMMONSYSTEM_EXPORT vtkTimerLog : public vtkObject
{
public:
  static vtkTimerLog *New();

  vtkTypeMacro(vtkTimerLog,vtkObject);
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
  static void FormatAndMarkEvent(const char *EventString, ...);
//ETX

  // Description:
  // Write the timing table out to a file.  Calculate some helpful
  // statistics (deltas and  percentages) in the process.
  static void DumpLog(const char *filename);

  // Description:
  // I want to time events, so I am creating this interface to
  // mark events that have a start and an end.  These events can be,
  // nested. The standard Dumplog ignores the indents.
  static void MarkStartEvent(const char *EventString);
  static void MarkEndEvent(const char *EventString);
//BTX
  static void DumpLogWithIndents(ostream *os, double threshold);
//ETX

  // Description:
  // Programatic access to events.  Indexed from 0 to num-1.
  static int GetNumberOfEvents();
  static int GetEventIndent(int i);
  static double GetEventWallTime(int i);
  static const char* GetEventString(int i);

  // Description:
  // Record a timing event and capture wall time and cpu ticks.
  static void MarkEvent(const char *EventString);

  // Description:
  // Clear the timing table.  walltime and cputime will also be set
  // to zero when the first new event is recorded.
  static void ResetLog();

  // Description:
  // Allocate timing table with MaxEntries elements.
  static void AllocateLog();

  // Description:
  // Remove timer log.
  static void CleanupLog();

  // Description:
  // Returns the elapsed number of seconds since January 1, 1970. This
  // is also called Universal Coordinated Time.
  static double GetUniversalTime();

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
  // a doubleing point value indicating the elapsed time in seconds.
  double GetElapsedTime();

protected:
  vtkTimerLog() {this->StartTime=0; this->EndTime = 0;}; //insure constructor/destructor protected
  virtual ~vtkTimerLog() { };

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
  static void DumpEntry(ostream& os, int index, double time, double deltatime,
                        int tick, int deltatick, const char *event);
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
