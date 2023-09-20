// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTimerLog
 * @brief   Timer support and logging
 *
 * vtkTimerLog contains walltime and cputime measurements associated
 * with a given event.  These results can be later analyzed when
 * "dumping out" the table.
 *
 * In addition, vtkTimerLog allows the user to simply get the current
 * time, and to start/stop a simple timer separate from the timing
 * table logging.
 */

#ifndef vtkTimerLog_h
#define vtkTimerLog_h

#include "vtkCommonSystemModule.h" // For export macro
#include "vtkObject.h"

#include <string> // STL Header

#ifdef _WIN32
#include <sys/timeb.h> // Needed for Win32 implementation of timer
#include <sys/types.h> // Needed for Win32 implementation of timer
#else
#include <sys/time.h>  // Needed for unix implementation of timer
#include <sys/times.h> // Needed for unix implementation of timer
#include <sys/types.h> // Needed for unix implementation of timer
#include <time.h>      // Needed for unix implementation of timer
#endif

// var args
#ifndef _WIN32
#include <unistd.h> // Needed for unix implementation of timer
#endif

// select stuff here is for sleep method
#ifndef NO_FD_SET
#define SELECT_MASK fd_set
#else
#ifndef _AIX
typedef long fd_mask;
#endif
#if defined(_IBMR2)
#define SELECT_MASK void
#else
#define SELECT_MASK int
#endif
#endif

VTK_ABI_NAMESPACE_BEGIN
struct vtkTimerLogEntry
{
  enum LogEntryType
  {
    INVALID = -1,
    STANDALONE, // an individual, marked event
    START,      // start of a timed event
    END,        // end of a timed event
    INSERTED    // externally timed value
  };
  double WallTime;
  int CpuTicks;
  std::string Event;
  LogEntryType Type;
  unsigned char Indent;
  vtkTimerLogEntry()
    : WallTime(0)
    , CpuTicks(0)
    , Type(INVALID)
    , Indent(0)
  {
  }
};

class VTKCOMMONSYSTEM_EXPORT vtkTimerLog : public vtkObject
{
public:
  static vtkTimerLog* New();

  vtkTypeMacro(vtkTimerLog, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This flag will turn logging of events off or on.
   * By default, logging is on.
   */
  static void SetLogging(int v) { vtkTimerLog::Logging = v; }
  static int GetLogging() { return vtkTimerLog::Logging; }
  static void LoggingOn() { vtkTimerLog::SetLogging(1); }
  static void LoggingOff() { vtkTimerLog::SetLogging(0); }

  ///@{
  /**
   * Set/Get the maximum number of entries allowed in the timer log
   */
  static void SetMaxEntries(int a);
  static int GetMaxEntries();
  ///@}

  /**
   * Record a timing event.  The event is represented by a formatted
   * string.  The internal buffer is 4096 bytes and will truncate anything longer.
   */
#ifndef __VTK_WRAP__
  static void FormatAndMarkEvent(const char* format, ...) VTK_FORMAT_PRINTF(1, 2);
#endif

  ///@{
  /**
   * Write the timing table out to a file.  Calculate some helpful
   * statistics (deltas and percentages) in the process.
   */
  static void DumpLog(VTK_FILEPATH const char* filename);
  ///@}

  ///@{
  /**
   * I want to time events, so I am creating this interface to
   * mark events that have a start and an end.  These events can be,
   * nested. The standard Dumplog ignores the indents.
   */
  static void MarkStartEvent(const char* EventString);
  static void MarkEndEvent(const char* EventString);
  ///@}

  ///@{
  /**
   * Insert an event with a known wall time value (in seconds)
   * and cpuTicks.
   */
  static void InsertTimedEvent(const char* EventString, double time, int cpuTicks);
  ///@}

  static void DumpLogWithIndents(ostream* os, double threshold);
  static void DumpLogWithIndentsAndPercentages(ostream* os);

  ///@{
  /**
   * Programmatic access to events.  Indexed from 0 to num-1.
   */
  static int GetNumberOfEvents();
  static int GetEventIndent(int i);
  static double GetEventWallTime(int i);
  static const char* GetEventString(int i);
  static vtkTimerLogEntry::LogEntryType GetEventType(int i);
  ///@}

  /**
   * Record a timing event and capture wall time and cpu ticks.
   */
  static void MarkEvent(const char* EventString);

  /**
   * Clear the timing table.  walltime and cputime will also be set
   * to zero when the first new event is recorded.
   */
  static void ResetLog();

  /**
   * Remove timer log.
   */
  static void CleanupLog();

  /**
   * Returns the elapsed number of seconds since 00:00:00 Coordinated Universal
   * Time (UTC), Thursday, 1 January 1970. This is also called Unix Time.
   */
  static double GetUniversalTime();

  /**
   * Returns the CPU time for this process
   * On Win32 platforms this actually returns wall time.
   */
  static double GetCPUTime();

  /**
   * Set the StartTime to the current time. Used with GetElapsedTime().
   */
  void StartTimer();

  /**
   * Sets EndTime to the current time. Used with GetElapsedTime().
   */
  void StopTimer();

  /**
   * Returns the difference between StartTime and EndTime as
   * a doubleing point value indicating the elapsed time in seconds.
   */
  double GetElapsedTime();

protected:
  vtkTimerLog()
  {
    this->StartTime = 0;
    this->EndTime = 0;
  } // ensure constructor/destructor protected
  ~vtkTimerLog() override = default;

  static int Logging;
  static int Indent;
  static int MaxEntries;
  static int NextEntry;
  static int WrapFlag;
  static int TicksPerSecond;

#ifdef _WIN32
#ifndef _WIN32_WCE
  static timeb FirstWallTime;
  static timeb CurrentWallTime;
#else
  static FILETIME FirstWallTime;
  static FILETIME CurrentWallTime;
#endif
#else
  static timeval FirstWallTime;
  static timeval CurrentWallTime;
  static tms FirstCpuTicks;
  static tms CurrentCpuTicks;
#endif

  /**
   * Record a timing event and capture wall time and cpu ticks.
   */
  static void MarkEventInternal(const char* EventString, vtkTimerLogEntry::LogEntryType type,
    vtkTimerLogEntry* entry = nullptr);

  // instance variables to support simple timing functionality,
  // separate from timer table logging.
  double StartTime;
  double EndTime;

  static vtkTimerLogEntry* GetEvent(int i);

  static void DumpEntry(ostream& os, int index, double time, double deltatime, int tick,
    int deltatick, const char* event);

private:
  vtkTimerLog(const vtkTimerLog&) = delete;
  void operator=(const vtkTimerLog&) = delete;
};

/**
 * Helper class to log time within scope
 */
class vtkTimerLogScope
{
public:
  vtkTimerLogScope(const char* eventString)
  {
    if (eventString)
    {
      this->EventString = eventString;
    }
    vtkTimerLog::MarkStartEvent(eventString);
  }

  ~vtkTimerLogScope() { vtkTimerLog::MarkEndEvent(this->EventString.c_str()); }

protected:
  std::string EventString;

private:
  vtkTimerLogScope(const vtkTimerLogScope&) = delete;
  void operator=(const vtkTimerLogScope&) = delete;
};

//
// Set built-in type.  Creates member Set"name"() (e.g., SetVisibility());
//
#define vtkTimerLogMacro(string)                                                                   \
  {                                                                                                \
    vtkTimerLog::FormatAndMarkEvent(                                                               \
      "Mark: In %s, line %d, class %s: %s", __FILE__, __LINE__, this->GetClassName(), string);     \
  }

// Implementation detail for Schwarz counter idiom.
class VTKCOMMONSYSTEM_EXPORT vtkTimerLogCleanup
{
public:
  vtkTimerLogCleanup();
  ~vtkTimerLogCleanup();

private:
  vtkTimerLogCleanup(const vtkTimerLogCleanup&) = delete;
  void operator=(const vtkTimerLogCleanup&) = delete;
};
static vtkTimerLogCleanup vtkTimerLogCleanupInstance;

VTK_ABI_NAMESPACE_END
#endif
