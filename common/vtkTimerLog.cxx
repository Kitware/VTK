
// .NAME vtkTimerLog - Maintains timing table for performance analysis
// .SECTION Description
// vtkTimerLog contains walltime and cputime measurements associated
// with a given event.  These results can be later analyzed when
// "dumping out" the table.

#include "vtkTimerLog.h"
#ifndef _WIN32
#include <limits.h>	// for CLK_TCK
#endif

// Description:
// Construct a new TimerLog, setting MaxEntries to a default size.
// The internal timing table structure will be allocated later (when
// the first event is recorded).  Thus, to allocate a timing table
// of a different size, use SetMaxEntries() before MarkEvent() is
// ever called.
vtkTimerLog::vtkTimerLog()
{
  MaxEntries = DEFAULT_NUM_TIMER_LOG_ELEMS;
  cout << "MaxEntries: " << MaxEntries << "\n";
  WrapFlag = 0;
  NextEntry = 0;
  TimerLog = NULL;

#ifdef _WIN32
  TicksPerSecond = 60;  // ???
#else
  TicksPerSecond = CLK_TCK;
#endif
}

// Description:
// Delete the TimerLog and its internal timing table structure.
vtkTimerLog::~vtkTimerLog()
{
  if (TimerLog != NULL)
    delete[] TimerLog;
}

// Description:
// Allocate timing table with MaxEntries elements.
void vtkTimerLog::AllocateLog()
{
  if (TimerLog != NULL)
    delete[] TimerLog;
  TimerLog = new TIMER_LOG_ENTRY[MaxEntries];
}


// Description:
// Clear the timing table.  walltime and cputime will also be set
// to zero when the first new event is recorded.
void vtkTimerLog::ResetLog()
{
  WrapFlag = 0;
  NextEntry = 0;
  // may want to free TimerLog to force realloc so
  // that user can resize the table by changing MaxEntries.
}


// Description:
// Record a timing event.  The event is represented by a formatted
// string.
void vtkTimerLog::FormatAndMarkEvent(char *format, ...)
{
  char event[80];

  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  this->MarkEvent(event);
}


// Description:
// Record a timing event and capture walltime and cputicks.
void vtkTimerLog::MarkEvent(char *event)
{
  int strsize;
  double time_diff;
  int ticks_diff;

  strsize = (strlen(event)) > TIMER_LOG_EVENT_LEN-1 ? TIMER_LOG_EVENT_LEN-1 : strlen(event);

  // If this the first event we're recording, allocate the
  // internal timing table and initialize WallTime and CpuTicks
  // for this first event to zero.
  if (NextEntry == 0 && ! WrapFlag)
  {
    if (TimerLog == NULL)
    {
      AllocateLog();
    }

#ifdef _WIN32
    ftime( &(this->FirstWallTime) );
#else
    gettimeofday( &(this->FirstWallTime), NULL );
    times(&FirstCpuTicks);
#endif

    TimerLog[0].WallTime = 0.0;
    TimerLog[0].CpuTicks = 0.0;
    strncpy(TimerLog[0].Event, event, strsize);
    TimerLog[0].Event[strsize] = '\0';
    NextEntry = 1;
    return;
  }

#ifdef _WIN32

  static double scale = 1.0/1000.0;
  ftime( &(this->CurrentWallTime) );
  time_diff  =  this->CurrentWallTime.time - this->FirstWallTime.time;
  time_diff += (this->CurrentWallTime.millitm - this->FirstWallTime.millitm) * scale;

  ticks_diff = 0;

#else

  static double scale = 1.0/1000000.0;
  gettimeofday( &(this->CurrentWallTime), NULL );
  time_diff  =  this->CurrentWallTime.tv_sec - this->FirstWallTime.tv_sec;
  time_diff += (this->CurrentWallTime.tv_usec - this->FirstWallTime.tv_usec) * scale;

  times(&CurrentCpuTicks);
  ticks_diff = (CurrentCpuTicks.tms_utime + CurrentCpuTicks.tms_stime) -
		(FirstCpuTicks.tms_utime + FirstCpuTicks.tms_stime);

#endif

  TimerLog[NextEntry].WallTime = (float)time_diff;
  TimerLog[NextEntry].CpuTicks = ticks_diff;
  strncpy(TimerLog[NextEntry].Event, event, strsize);
  TimerLog[NextEntry].Event[strsize] = '\0';

  NextEntry++;
  if (NextEntry == MaxEntries)
  {
    NextEntry = 0;
    WrapFlag = 1;
  }
}


// Description:
// Write the timing table out to a file.  Calculate some helpful
// statistics (deltas and  percentages) in the process.
void vtkTimerLog::DumpLog(char *filename)
{
  ofstream os(filename);
  int i;

  os << " Entry   Wall Time (sec)  Delta   CPU Time (sec)  Delta  %CPU   Event\n";
  os << "----------------------------------------------------------------------\n";

  if ( WrapFlag )
  {
    DumpEntry(os, 0, TimerLog[NextEntry].WallTime, 0,
              TimerLog[NextEntry].CpuTicks, 0, TimerLog[NextEntry].Event);
    for (i=NextEntry+1; i<MaxEntries; i++)
    {
      DumpEntry(os, i-NextEntry, TimerLog[i].WallTime,
                TimerLog[i].WallTime - TimerLog[i-1].WallTime,
                TimerLog[i].CpuTicks,
                TimerLog[i].CpuTicks - TimerLog[i-1].CpuTicks,
                TimerLog[i].Event);
    }
    DumpEntry(os, MaxEntries-NextEntry, TimerLog[0].WallTime,
              TimerLog[0].WallTime - TimerLog[MaxEntries-1].WallTime,
              TimerLog[0].CpuTicks,
              TimerLog[0].CpuTicks - TimerLog[MaxEntries-1].CpuTicks,
              TimerLog[0].Event);
    for (i=1; i<NextEntry; i++)
    {
      DumpEntry(os, MaxEntries-NextEntry+i, TimerLog[i].WallTime,
                TimerLog[i].WallTime - TimerLog[i-1].WallTime,
                TimerLog[i].CpuTicks,
                TimerLog[i].CpuTicks - TimerLog[i-1].CpuTicks,
                TimerLog[i].Event);
    }
  }
  else
  {
    DumpEntry(os, 0, TimerLog[0].WallTime, 0,
              TimerLog[0].CpuTicks, 0, TimerLog[0].Event);
    for (i=1; i<NextEntry; i++)
    {
      DumpEntry(os, i, TimerLog[i].WallTime,
                TimerLog[i].WallTime - TimerLog[i-1].WallTime,
                TimerLog[i].CpuTicks,
                TimerLog[i].CpuTicks - TimerLog[i-1].CpuTicks,
                TimerLog[i].Event);
    }
  }

  os.close();
}


// Description:
// Print method for vtkTimerLog.
void vtkTimerLog::PrintSelf(ostream& os, vtkIndent indent)
{
  int i;

  vtkObject::PrintSelf(os, indent);

  os << "TimerLog\n";
  os << "MaxEntries: " << MaxEntries << "\n";
  os << "NextEntry: " << NextEntry << "\n";
  os << "WrapFlag: " << WrapFlag << "\n";
  os << "TicksPerSecond: " << TicksPerSecond << "\n";
  os << "\n";

  os << " Entry   Wall Time   CpuTicks   Event\n";
  os << "--------------------------------------\n";

  if ( WrapFlag )
  {
    for (i=NextEntry; i<MaxEntries; i++)
    {
      os << i << " " << TimerLog[i].WallTime << " " << TimerLog[i].CpuTicks
         << " " << TimerLog[i].Event << "\n";
    }
  }

  for (i=0; i<NextEntry; i++)
  {
    os << i << " " << TimerLog[i].WallTime << " " << TimerLog[i].CpuTicks
       << " " << TimerLog[i].Event << "\n";
  }
}

