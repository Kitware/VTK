/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTimerLog.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTimerLog - Maintains timing table for performance analysis
// .SECTION Description
// vtkTimerLog contains walltime and cputime measurements associated
// with a given event.  These results can be later analyzed when
// "dumping out" the table.
//
// In addition, vtkTimerLog allows the user to simply get the current
// time, and to start/stop a simple timer separate from the timing
// table logging.

#include "vtkTimerLog.h"

#include "vtkMath.h"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iterator>
#include <stdarg.h>
#include <string>
#include <vector>

#ifndef _WIN32
#include <climits>     // for CLK_TCK
#include <sys/time.h>
#include <unistd.h>
#endif

#ifndef _WIN32_WCE
#include <sys/types.h>
#include <ctime>
#endif
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkTimerLog);

// initialze the class variables
int vtkTimerLog::Logging = 1;
int vtkTimerLog::Indent = 0;
int vtkTimerLog::MaxEntries = 100;
int vtkTimerLog::NextEntry = 0;
int vtkTimerLog::WrapFlag = 0;
std::vector<vtkTimerLogEntry> vtkTimerLog::TimerLog;

#ifdef CLK_TCK
int vtkTimerLog::TicksPerSecond = CLK_TCK;
#else
int vtkTimerLog::TicksPerSecond = 60;
#endif

#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC (vtkTimerLog::TicksPerSecond)
#endif


#ifdef _WIN32
#ifndef _WIN32_WCE
timeb vtkTimerLog::FirstWallTime;
timeb vtkTimerLog::CurrentWallTime;
#else
FILETIME vtkTimerLog::FirstWallTime;
FILETIME vtkTimerLog::CurrentWallTime;
#endif
#else
timeval vtkTimerLog::FirstWallTime;
timeval vtkTimerLog::CurrentWallTime;
tms     vtkTimerLog::FirstCpuTicks;
tms     vtkTimerLog::CurrentCpuTicks;
#endif

//----------------------------------------------------------------------------
// Allocate timing table with MaxEntries elements.
void vtkTimerLog::AllocateLog()
{
  vtkTimerLog::TimerLog.resize(vtkTimerLog::MaxEntries);
}

//----------------------------------------------------------------------------
// Remove timer log.
void vtkTimerLog::CleanupLog()
{
  vtkTimerLog::TimerLog.clear();
}

//----------------------------------------------------------------------------
// Clear the timing table.  walltime and cputime will also be set
// to zero when the first new event is recorded.
void vtkTimerLog::ResetLog()
{
  vtkTimerLog::WrapFlag = 0;
  vtkTimerLog::NextEntry = 0;
  // may want to free TimerLog to force realloc so
  // that user can resize the table by changing MaxEntries.
}


//----------------------------------------------------------------------------
// Record a timing event.  The event is represented by a formatted
// string.
void vtkTimerLog::FormatAndMarkEvent(const char *format, ...)
{
  if (! vtkTimerLog::Logging)
  {
    return;
  }

  static  char event[4096];
  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  vtkTimerLog::MarkEventInternal(event, vtkTimerLogEntry::STANDALONE);
}

//----------------------------------------------------------------------------
// Record a timing event and capture walltime and cputicks.
void vtkTimerLog::MarkEvent(const char *event)
{
  vtkTimerLog::MarkEventInternal(event, vtkTimerLogEntry::STANDALONE);
}

//----------------------------------------------------------------------------
// Record a timing event and capture walltime and cputicks.
void vtkTimerLog::MarkEventInternal(
  const char *event, vtkTimerLogEntry::LogEntryType type, vtkTimerLogEntry* entry)
{
  if (! vtkTimerLog::Logging)
  {
    return;
  }

  double time_diff;
  int ticks_diff;

  // If this the first event we're recording, allocate the
  // internal timing table and initialize WallTime and CpuTicks
  // for this first event to zero.
  if (vtkTimerLog::NextEntry == 0 && ! vtkTimerLog::WrapFlag)
  {
    if (vtkTimerLog::TimerLog.empty())
    {
      vtkTimerLog::AllocateLog();
    }

#ifdef _WIN32
#ifdef _WIN32_WCE
    SYSTEMTIME st;
    GetLocalTime(&st);
    SystemTimeToFileTime(&st, &(vtkTimerLog::FirstWallTime));
#else
    ::ftime( &(vtkTimerLog::FirstWallTime) );
#endif
#else
    gettimeofday( &(vtkTimerLog::FirstWallTime), NULL );
    times(&FirstCpuTicks);
#endif

    if (entry)
    {
      vtkTimerLog::TimerLog[0] = *entry;
    }
    else
    {
      vtkTimerLog::TimerLog[0].Indent = vtkTimerLog::Indent;
      vtkTimerLog::TimerLog[0].WallTime = 0.0;
      vtkTimerLog::TimerLog[0].CpuTicks = 0;
      if (event)
      {
        vtkTimerLog::TimerLog[0].Event = event;
      }
      vtkTimerLog::TimerLog[0].Type = type;
      vtkTimerLog::NextEntry = 1;
    }
    return;
  }

  if (entry)
  {
    vtkTimerLog::TimerLog[vtkTimerLog::NextEntry] = *entry;
  }
  else
  {
#ifdef _WIN32
#ifdef _WIN32_WCE
    SYSTEMTIME st;
    GetLocalTime(&st);
    SystemTimeToFileTime(&st, &(vtkTimerLog::CurrentWallTime));
    time_diff = (vtkTimerLog::CurrentWallTime.dwHighDateTime -
      vtkTimerLog::FirstWallTime.dwHighDateTime);
    time_diff = time_diff * 429.4967296;
    time_diff = time_diff + ((vtkTimerLog::CurrentWallTime.dwLowDateTime -
      vtkTimerLog::FirstWallTime.dwLowDateTime) / 10000000.0);
#else
    static double scale = 1.0/1000.0;
    ::ftime( &(vtkTimerLog::CurrentWallTime) );
    time_diff =
      vtkTimerLog::CurrentWallTime.time - vtkTimerLog::FirstWallTime.time;
    time_diff +=
      (vtkTimerLog::CurrentWallTime.millitm
       - vtkTimerLog::FirstWallTime.millitm) * scale;
#endif
    ticks_diff = 0;
#else
    static double scale = 1.0/1000000.0;
    gettimeofday( &(vtkTimerLog::CurrentWallTime), NULL );
    time_diff  =  vtkTimerLog::CurrentWallTime.tv_sec
      - vtkTimerLog::FirstWallTime.tv_sec;
    time_diff +=
      (vtkTimerLog::CurrentWallTime.tv_usec
       - vtkTimerLog::FirstWallTime.tv_usec) * scale;

    times(&CurrentCpuTicks);
    ticks_diff = (CurrentCpuTicks.tms_utime + CurrentCpuTicks.tms_stime) -
      (FirstCpuTicks.tms_utime + FirstCpuTicks.tms_stime);
#endif

    vtkTimerLog::TimerLog[vtkTimerLog::NextEntry].Indent = vtkTimerLog::Indent;
    vtkTimerLog::TimerLog[vtkTimerLog::NextEntry].WallTime =
      static_cast<double>(time_diff);
    vtkTimerLog::TimerLog[vtkTimerLog::NextEntry].CpuTicks = ticks_diff;
    if (event)
    {
      vtkTimerLog::TimerLog[vtkTimerLog::NextEntry].Event = event;
    }
    vtkTimerLog::TimerLog[vtkTimerLog::NextEntry].Type = type;
  }

  vtkTimerLog::NextEntry++;
  if (vtkTimerLog::NextEntry == vtkTimerLog::MaxEntries)
  {
    vtkTimerLog::NextEntry = 0;
    vtkTimerLog::WrapFlag = 1;
  }
}

//----------------------------------------------------------------------------
// Record a timing event and capture walltime and cputicks.
// Increments indent after mark.
void vtkTimerLog::MarkStartEvent(const char *event)
{
  if (! vtkTimerLog::Logging)
  { // Maybe we should still change the Indent ...
    return;
  }

  vtkTimerLog::MarkEventInternal(event, vtkTimerLogEntry::START);
  ++vtkTimerLog::Indent;
}

//----------------------------------------------------------------------------
// Record a timing event and capture walltime and cputicks.
// Decrements indent after mark.
void vtkTimerLog::MarkEndEvent(const char *event)
{
  if (! vtkTimerLog::Logging)
  { // Maybe we should still change the Indent ...
    return;
  }

  vtkTimerLog::MarkEventInternal(event, vtkTimerLogEntry::END);
  --vtkTimerLog::Indent;
}

//----------------------------------------------------------------------------
// Record a timing event with known walltime and cputicks.
void vtkTimerLog::InsertTimedEvent(
  const char *event, double time, int cpuTicks)
{
  if (! vtkTimerLog::Logging)
  {
    return;
  }
  // manually create both the start and end event and then
  // change the start events values to appear like other events
  vtkTimerLogEntry entry;
  entry.WallTime = time;
  entry.CpuTicks = cpuTicks;
  if (event)
  {
    entry.Event = event;
  }
  entry.Type = vtkTimerLogEntry::INSERTED;
  entry.Indent = vtkTimerLog::Indent;

  vtkTimerLog::MarkEventInternal(event, vtkTimerLogEntry::INSERTED, &entry);
}

//----------------------------------------------------------------------------
// Record a timing event and capture walltime and cputicks.
int vtkTimerLog::GetNumberOfEvents()
{
  if (vtkTimerLog::WrapFlag)
  {
    return vtkTimerLog::MaxEntries;
  }
   else
   {
    return vtkTimerLog::NextEntry;
   }
}

//----------------------------------------------------------------------------
vtkTimerLogEntry *vtkTimerLog::GetEvent(int idx)
{
  int num = vtkTimerLog::GetNumberOfEvents();
  int start = 0;
  if (vtkTimerLog::WrapFlag)
  {
    start = vtkTimerLog::NextEntry;
  }

  if (idx < 0 || idx >= num)
  {
    cerr << "Bad entry index " << idx << endl;
    return nullptr;
  }
  idx = (idx + start) % vtkTimerLog::MaxEntries;

  return &(vtkTimerLog::TimerLog[idx]);
}

//----------------------------------------------------------------------------
int vtkTimerLog::GetEventIndent(int idx)
{
  if (vtkTimerLogEntry *tmp = vtkTimerLog::GetEvent(idx))
  {
    return tmp->Indent;
  }
  return 0;
}

//----------------------------------------------------------------------------
double vtkTimerLog::GetEventWallTime(int idx)
{
  if (vtkTimerLogEntry *tmp = vtkTimerLog::GetEvent(idx))
  {
    return tmp->WallTime;
  }
  return 0.0;
}

//----------------------------------------------------------------------------
const char* vtkTimerLog::GetEventString(int idx)
{
  if (vtkTimerLogEntry *tmp = vtkTimerLog::GetEvent(idx))
  {
    return tmp->Event.c_str();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
vtkTimerLogEntry::LogEntryType vtkTimerLog::GetEventType(int idx)
{
  if (vtkTimerLogEntry *tmp = vtkTimerLog::GetEvent(idx))
  {
    return tmp->Type;
  }
  return vtkTimerLogEntry::INVALID;
}

//----------------------------------------------------------------------------
// Write the timing table out to a file.  Calculate some helpful
// statistics (deltas and  percentages) in the process.
void vtkTimerLog::DumpLogWithIndents(ostream *os, double threshold)
{
#ifndef _WIN32_WCE
  int num = vtkTimerLog::GetNumberOfEvents();
  std::vector<bool> handledEvents(num, false);

  for (int w=0; w < vtkTimerLog::WrapFlag+1; w++)
  {
    int start = 0;
    int end = vtkTimerLog::NextEntry;
    if (vtkTimerLog::WrapFlag != 0 && w == 0)
    {
      start = vtkTimerLog::NextEntry;
      end = vtkTimerLog::MaxEntries;
    }
    for (int i1=start; i1 < end; i1++)
    {
      int indent1 = vtkTimerLog::GetEventIndent(i1);
      vtkTimerLogEntry::LogEntryType eventType = vtkTimerLog::GetEventType(i1);
      int endEvent = -1; // only modified if this is a START event
      if (eventType == vtkTimerLogEntry::END && handledEvents[i1] == true)
      {
        continue; // this END event is handled by the corresponding START event
      }
      if (eventType == vtkTimerLogEntry::START)
      {
        // Search for an END event. it may be before the START event if we've wrapped.
        int counter = 1;
        while (counter < num && vtkTimerLog::GetEventIndent((i1+counter)%num) > indent1)
        {
          counter++;
        }
        if (vtkTimerLog::GetEventIndent((i1+counter)%num) == indent1)
        {
          counter--;
          endEvent = (i1+counter)%num;
          handledEvents[endEvent] = true;
        }
      }
      double dtime = threshold;
      if (eventType == vtkTimerLogEntry::START)
      {
        dtime = vtkTimerLog::GetEventWallTime(endEvent) - vtkTimerLog::GetEventWallTime(i1);
      }
      if (dtime >= threshold)
      {
        int j = indent1;
        while (j-- > 0)
        {
          *os << "    ";
        }
        *os << vtkTimerLog::GetEventString(i1);
        if (endEvent != -1)
        { // Start event.
          *os << ",  " << dtime << " seconds";
        }
        else if (eventType == vtkTimerLogEntry::INSERTED)
        {
          *os << ",  " << vtkTimerLog::GetEventWallTime(i1) << " seconds (inserted time)";
        }
        else if (eventType == vtkTimerLogEntry::END)
        {
          *os << " (END event without matching START event)";
        }
        *os << endl;
      }
    }
  }
#endif
}

//----------------------------------------------------------------------------
void vtkTimerLog::DumpLogWithIndentsAndPercentages(std::ostream *os)
{
  assert(os);
  // Use indents to pair start/end events. Indents work like this:
  // Indent | Event
  // ------ | -----
  //   0    | Event 1 Start
  //   1    | SubEvent 1 Start
  //   2    | SubEvent 1 End
  //   1    | SubEvent 2 Start
  //   2    | SubSubEvent 1 Start
  //   3    | SubSubEvent 1 End
  //   2    | SubEvent 2 End
  //   1    | Event 1 End

  // If we've wrapped the entry buffer, the indent information will be
  // nonsensical. I don't trust the parsing logic below to not do bizarre,
  // possibly crashy things in this case, so let's just error out and let the
  // dev know how to fix the issue.
  if (vtkTimerLog::WrapFlag)
  {
    *os << "Error: Event log has exceeded vtkTimerLog::MaxEntries.\n"
           "Call vtkTimerLog::SetMaxEntries to increase the log buffer size.\n"
           "Current vtkTimerLog::MaxEntries: " << vtkTimerLog::MaxEntries
        << ".\n";
    return;
  }

  // Store previous 'scopes' in a LIFO buffer
  typedef std::pair<int, double> IndentTime;
  std::vector<IndentTime> parentInfo;

  // Find the longest event string:
  int numEvents = vtkTimerLog::GetNumberOfEvents();
  int longestString = 0;
  for (int i = 0; i < numEvents; ++i)
  {
    longestString = std::max(longestString, static_cast<int>(
                               strlen(vtkTimerLog::GetEventString(i))));
  }

  // Loop to numEvents - 1, since the last event must be an end event.
  for (int startIdx = 0; startIdx < numEvents - 1; ++startIdx)
  {
    int curIndent = vtkTimerLog::GetEventIndent(startIdx);

    vtkTimerLogEntry::LogEntryType logEntryType = vtkTimerLog::GetEventType(startIdx);
    if (logEntryType == vtkTimerLogEntry::END)
    { // Skip this event if it is an end event:
      assert(!parentInfo.empty());
      parentInfo.pop_back();
      continue;
    }
    else if (logEntryType == vtkTimerLogEntry::STANDALONE)
    { // Skip this event if it is just to mark that an event occured
      continue;
    }

    // Find the event that follows the end event:
    int endIdx = startIdx + 1;
    for (; endIdx < numEvents; ++endIdx)
    {
      if (vtkTimerLog::GetEventIndent(endIdx) == curIndent)
      {
        break;
      }
    }

    // Move back one event to get our end event (also works when we've reached
    // the end of the event log).
    endIdx--;

    // Get the current event time:
    double elapsedTime = logEntryType == vtkTimerLogEntry::START ?
      vtkTimerLog::GetEventWallTime(endIdx) - vtkTimerLog::GetEventWallTime(startIdx) :
      vtkTimerLog::GetEventWallTime(startIdx);

    // The total time the parent took to execute. If empty, this is the first
    // event, and just set to 100%.
    IndentTime parent = parentInfo.empty() ? IndentTime(-1, elapsedTime)
                                           : parentInfo.back();

    // Percentage of parent exec time, rounded to a single decimal:
    float percentage =
        vtkMath::Round(elapsedTime / parent.second * 1000.) / 10.f;

    *os << std::setw(8) << std::setprecision(6) << std::fixed
        << elapsedTime
        << std::setw(0) << "s"
        << std::setw(curIndent * 2) << " "
        << std::setprecision(1) << std::setw(5) << std::right << percentage
        << std::setw(0) << std::left << "% "
        << std::setw(longestString) << vtkTimerLog::GetEventString(startIdx);
    if (logEntryType == vtkTimerLogEntry::INSERTED)
    {
      *os << " (inserted time)";
    }
    *os << "\n";

    // Add our parent info if this was time with a START and END event:
    if (logEntryType == vtkTimerLogEntry::START)
    {
      parentInfo.push_back(IndentTime(curIndent, elapsedTime));
    }
  }
}

//----------------------------------------------------------------------------
// Write the timing table out to a file. This is meant for non-timed events,
// i.e. event type = STANDALONE. All other event types besides the first
// are ignored.
void vtkTimerLog::DumpLog(const char *filename)
{
#ifndef _WIN32_WCE
  ofstream os_with_warning_C4701(filename);
  int i;

  if ( vtkTimerLog::WrapFlag )
  {
    vtkTimerLog::DumpEntry(
      os_with_warning_C4701, 0,
      vtkTimerLog::TimerLog[vtkTimerLog::NextEntry].WallTime, 0,
      vtkTimerLog::TimerLog[vtkTimerLog::NextEntry].CpuTicks, 0,
      vtkTimerLog::TimerLog[vtkTimerLog::NextEntry].Event.c_str());
    int previousEvent = vtkTimerLog::NextEntry;
    for (i=vtkTimerLog::NextEntry+1; i<vtkTimerLog::MaxEntries; i++)
    {
      if (vtkTimerLog::TimerLog[i].Type == vtkTimerLogEntry::STANDALONE)
      {
        vtkTimerLog::DumpEntry(
          os_with_warning_C4701, i-vtkTimerLog::NextEntry,
          vtkTimerLog::TimerLog[i].WallTime,
          vtkTimerLog::TimerLog[i].WallTime
          - vtkTimerLog::TimerLog[previousEvent].WallTime,
          vtkTimerLog::TimerLog[i].CpuTicks,
          vtkTimerLog::TimerLog[i].CpuTicks
          - vtkTimerLog::TimerLog[previousEvent].CpuTicks,
          vtkTimerLog::TimerLog[i].Event.c_str());
        previousEvent = i;
      }
    }
    for (i=0; i<vtkTimerLog::NextEntry; i++)
    {
      if (vtkTimerLog::TimerLog[i].Type == vtkTimerLogEntry::STANDALONE)
      {
        vtkTimerLog::DumpEntry(
          os_with_warning_C4701, vtkTimerLog::MaxEntries-vtkTimerLog::NextEntry+i,
          vtkTimerLog::TimerLog[i].WallTime,
          vtkTimerLog::TimerLog[i].WallTime
          - vtkTimerLog::TimerLog[previousEvent].WallTime,
          vtkTimerLog::TimerLog[i].CpuTicks,
          vtkTimerLog::TimerLog[i].CpuTicks
          - vtkTimerLog::TimerLog[previousEvent].CpuTicks,
          vtkTimerLog::TimerLog[i].Event.c_str());
        previousEvent = i;
      }
    }
  }
  else
  {
    vtkTimerLog::DumpEntry(
      os_with_warning_C4701, 0, vtkTimerLog::TimerLog[0].WallTime, 0,
      vtkTimerLog::TimerLog[0].CpuTicks, 0,
      vtkTimerLog::TimerLog[0].Event.c_str());
    int previousEvent = 0;
    for (i=1; i<vtkTimerLog::NextEntry; i++)
    {
      if (vtkTimerLog::TimerLog[i].Type == vtkTimerLogEntry::STANDALONE)
      {
        vtkTimerLog::DumpEntry(
          os_with_warning_C4701, i, vtkTimerLog::TimerLog[i].WallTime,
          vtkTimerLog::TimerLog[i].WallTime
          - vtkTimerLog::TimerLog[previousEvent].WallTime,
          vtkTimerLog::TimerLog[i].CpuTicks,
          vtkTimerLog::TimerLog[i].CpuTicks
          - vtkTimerLog::TimerLog[previousEvent].CpuTicks,
          vtkTimerLog::TimerLog[i].Event.c_str());
        previousEvent = i;
      }
    }
  }

  os_with_warning_C4701.close();
#endif
}

//----------------------------------------------------------------------------
// Print method for vtkTimerLog.
void vtkTimerLog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "MaxEntries: " << vtkTimerLog::MaxEntries << "\n";
  os << indent << "NextEntry: " << vtkTimerLog::NextEntry << "\n";
  os << indent << "WrapFlag: " << vtkTimerLog::WrapFlag << "\n";
  os << indent << "TicksPerSecond: " << vtkTimerLog::TicksPerSecond << "\n";
  os << "\n";

  os << indent << "Entry \tWall Time\tCpuTicks\tEvent\n";
  os << indent << "----------------------------------------------\n";

  if ( vtkTimerLog::WrapFlag )
  {
    for (int i=vtkTimerLog::NextEntry; i<vtkTimerLog::MaxEntries; i++)
    {
      os << indent << i << "\t\t" << TimerLog[i].WallTime << "\t\t" <<
        TimerLog[i].CpuTicks << "\t\t" << TimerLog[i].Event << "\n";
    }
  }

  for (int i=0; i<vtkTimerLog::NextEntry; i++)
  {
    os << indent << i << "\t\t" << TimerLog[i].WallTime << "\t\t" <<
      TimerLog[i].CpuTicks << "\t\t" << TimerLog[i].Event << "\n";
  }

  os << "\n" << indent << "StartTime: " << this->StartTime << "\n";
}


// Methods to support simple timer functionality, separate from
// timer table logging.

//----------------------------------------------------------------------------
// Returns the elapsed number of seconds since January 1, 1970. This
// is also called Universal Coordinated Time.
double vtkTimerLog::GetUniversalTime()
{
  double currentTimeInSeconds;

#ifdef _WIN32
#ifdef _WIN32_WCE
  FILETIME CurrentTime;
  SYSTEMTIME st;
  GetLocalTime(&st);
  SystemTimeToFileTime(&st, &CurrentTime);
  currentTimeInSeconds = CurrentTime.dwHighDateTime;
  currentTimeInSeconds *= 429.4967296;
  currentTimeInSeconds = currentTimeInSeconds +
        CurrentTime.dwLowDateTime / 10000000.0;
#else
  timeb CurrentTime;
  static double scale = 1.0/1000.0;
  ::ftime( &CurrentTime );
  currentTimeInSeconds = CurrentTime.time + scale * CurrentTime.millitm;
#endif
#else
  timeval CurrentTime;
  static double scale = 1.0/1000000.0;
  gettimeofday( &CurrentTime, NULL );
  currentTimeInSeconds = CurrentTime.tv_sec + scale * CurrentTime.tv_usec;
#endif

  return currentTimeInSeconds;
}

//----------------------------------------------------------------------------
double vtkTimerLog::GetCPUTime()
{
#ifndef _WIN32_WCE
  return static_cast<double>(clock()) /static_cast<double>(CLOCKS_PER_SEC);
#else
  return 1.0;
#endif
}

//----------------------------------------------------------------------------
// Set the StartTime to the current time. Used with GetElapsedTime().
void vtkTimerLog::StartTimer()
{
  this->StartTime = vtkTimerLog::GetUniversalTime();
}

//----------------------------------------------------------------------------
// Sets EndTime to the current time. Used with GetElapsedTime().
void vtkTimerLog::StopTimer()
{
  this->EndTime = vtkTimerLog::GetUniversalTime();
}

//----------------------------------------------------------------------------
// Returns the difference between StartTime and EndTime as
// a floating point value indicating the elapsed time in seconds.
double vtkTimerLog::GetElapsedTime()
{
  return (this->EndTime - this->StartTime);
}

//----------------------------------------------------------------------------
void vtkTimerLog::DumpEntry(ostream& os, int index, double ttime,
                            double deltatime, int tick, int deltatick,
                            const char *event)
{
  os << index << "   "
     << ttime << "  "
     << deltatime << "   "
     << static_cast<double>(tick)/vtkTimerLog::TicksPerSecond << "  "
     << static_cast<double>(deltatick)/vtkTimerLog::TicksPerSecond << "  ";
  if (deltatime == 0.0)
  {
    os << "0.0   ";
  }
  else
  {
    os << 100.0*deltatick/vtkTimerLog::TicksPerSecond/deltatime << "   ";
  }
  os << event << "\n";
}

//----------------------------------------------------------------------------
void vtkTimerLog::SetMaxEntries(int a)
{
  if (a == vtkTimerLog::MaxEntries)
  {
    return;
  }
  int numEntries = vtkTimerLog::GetNumberOfEvents();
  if (vtkTimerLog::WrapFlag)
  { // if we've wrapped events, reorder them
    std::vector<vtkTimerLogEntry> tmp;
    tmp.reserve(vtkTimerLog::MaxEntries);
    std::copy(vtkTimerLog::TimerLog.begin() + vtkTimerLog::NextEntry,
              vtkTimerLog::TimerLog.end(),
              std::back_inserter(tmp));
    std::copy(vtkTimerLog::TimerLog.begin(),
              vtkTimerLog::TimerLog.begin() + vtkTimerLog::NextEntry,
              std::back_inserter(tmp));
    vtkTimerLog::TimerLog = tmp;
    vtkTimerLog::WrapFlag = 0;
  }
  if (numEntries <= a)
  {
    vtkTimerLog::TimerLog.resize(a);
    vtkTimerLog::NextEntry = numEntries;
    vtkTimerLog::WrapFlag = 0;
    vtkTimerLog::MaxEntries = a;
    return;
  }
  // Reduction so we need to get rid of the first bunch of events
  int offset = numEntries - a;
  assert(offset >= 0);
  vtkTimerLog::TimerLog.erase(vtkTimerLog::TimerLog.begin(),
                              vtkTimerLog::TimerLog.begin()+offset);
  vtkTimerLog::MaxEntries = a;
  vtkTimerLog::NextEntry = 0;
  vtkTimerLog::WrapFlag = 1;
}

//----------------------------------------------------------------------------
int vtkTimerLog::GetMaxEntries()
{
  return vtkTimerLog::MaxEntries;
}
