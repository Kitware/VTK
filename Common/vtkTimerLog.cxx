/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTimerLog.cxx
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
#ifndef _WIN32
#include <limits.h>     // for CLK_TCK
#include <sys/time.h>
#include <unistd.h>
#endif

#ifndef _WIN32_WCE
#include <sys/types.h>
#include <time.h>
#endif
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkTimerLog, "1.30");
vtkStandardNewMacro(vtkTimerLog);

// initialze the class variables
int vtkTimerLog::MaxEntries = 100;
int vtkTimerLog::NextEntry = 0;
int vtkTimerLog::WrapFlag = 0;
vtkTimerLogEntry *vtkTimerLog::TimerLog = NULL;

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

// Allocate timing table with MaxEntries elements.
void vtkTimerLog::AllocateLog()
{
  if (vtkTimerLog::TimerLog != NULL)
    {
    delete [] vtkTimerLog::TimerLog;
    }
  vtkTimerLog::TimerLog = new vtkTimerLogEntry[vtkTimerLog::MaxEntries];
}


// Clear the timing table.  walltime and cputime will also be set
// to zero when the first new event is recorded.
void vtkTimerLog::ResetLog()
{
  vtkTimerLog::WrapFlag = 0;
  vtkTimerLog::NextEntry = 0;
  // may want to free TimerLog to force realloc so
  // that user can resize the table by changing MaxEntries.
}


// Record a timing event.  The event is represented by a formatted
// string.
void vtkTimerLog::FormatAndMarkEvent(char *format, ...)
{
static  char event[4096];

  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  vtkTimerLog::MarkEvent(event);
}


// Record a timing event and capture walltime and cputicks.
void vtkTimerLog::MarkEvent(char *event)
{
  int strsize;
  double time_diff;
  int ticks_diff;

  strsize = (strlen(event)) > VTK_LOG_EVENT_LENGTH - 1
    ? VTK_LOG_EVENT_LENGTH-1 : static_cast<int>(strlen(event));

  // If this the first event we're recording, allocate the
  // internal timing table and initialize WallTime and CpuTicks
  // for this first event to zero.
  if (vtkTimerLog::NextEntry == 0 && ! vtkTimerLog::WrapFlag)
    {
    if (vtkTimerLog::TimerLog == NULL)
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
    
    vtkTimerLog::TimerLog[0].WallTime = 0.0;
    vtkTimerLog::TimerLog[0].CpuTicks = 0;
    strncpy(vtkTimerLog::TimerLog[0].Event, event, strsize);
    vtkTimerLog::TimerLog[0].Event[strsize] = '\0';
    vtkTimerLog::NextEntry = 1;
    return;
    }
  
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

  vtkTimerLog::TimerLog[vtkTimerLog::NextEntry].WallTime = (float)time_diff;
  vtkTimerLog::TimerLog[vtkTimerLog::NextEntry].CpuTicks = ticks_diff;
  strncpy(vtkTimerLog::TimerLog[vtkTimerLog::NextEntry].Event, event, strsize);
  vtkTimerLog::TimerLog[vtkTimerLog::NextEntry].Event[strsize] = '\0';

  vtkTimerLog::NextEntry++;
  if (vtkTimerLog::NextEntry == vtkTimerLog::MaxEntries)
    {
    vtkTimerLog::NextEntry = 0;
    vtkTimerLog::WrapFlag = 1;
    }
}


// Write the timing table out to a file.  Calculate some helpful
// statistics (deltas and  percentages) in the process.
void vtkTimerLog::DumpLog(char *filename)
{
#ifndef _WIN32_WCE
  ofstream os(filename);
  int i;
  
  os << " Entry   Wall Time (sec)  Delta   CPU Time (sec)  Delta  %CPU   Event\n";
  os << "----------------------------------------------------------------------\n";
  
  if ( vtkTimerLog::WrapFlag )
    {
    vtkTimerLog::DumpEntry(os, 0,
                    vtkTimerLog::TimerLog[vtkTimerLog::NextEntry].WallTime, 0,
                    vtkTimerLog::TimerLog[vtkTimerLog::NextEntry].CpuTicks, 0,
                    vtkTimerLog::TimerLog[vtkTimerLog::NextEntry].Event);
    for (i=vtkTimerLog::NextEntry+1; i<vtkTimerLog::MaxEntries; i++)
      {
      vtkTimerLog::DumpEntry(os,
                i-vtkTimerLog::NextEntry, vtkTimerLog::TimerLog[i].WallTime,
                vtkTimerLog::TimerLog[i].WallTime
                 - vtkTimerLog::TimerLog[i-1].WallTime,
                vtkTimerLog::TimerLog[i].CpuTicks,
                vtkTimerLog::TimerLog[i].CpuTicks
                 - vtkTimerLog::TimerLog[i-1].CpuTicks,
                vtkTimerLog::TimerLog[i].Event);
      }
    vtkTimerLog::DumpEntry(os, vtkTimerLog::MaxEntries-vtkTimerLog::NextEntry,
                    vtkTimerLog::TimerLog[0].WallTime,
                    vtkTimerLog::TimerLog[0].WallTime
                    -vtkTimerLog::TimerLog[vtkTimerLog::MaxEntries-1].WallTime,
                    vtkTimerLog::TimerLog[0].CpuTicks,
                    vtkTimerLog::TimerLog[0].CpuTicks
                    -vtkTimerLog::TimerLog[vtkTimerLog::MaxEntries-1].CpuTicks,
                    vtkTimerLog::TimerLog[0].Event);
    for (i=1; i<vtkTimerLog::NextEntry; i++)
      {
      vtkTimerLog::DumpEntry(os, vtkTimerLog::MaxEntries-vtkTimerLog::NextEntry+i,
                      vtkTimerLog::TimerLog[i].WallTime,
                      vtkTimerLog::TimerLog[i].WallTime
                      - vtkTimerLog::TimerLog[i-1].WallTime,
                      vtkTimerLog::TimerLog[i].CpuTicks,
                      vtkTimerLog::TimerLog[i].CpuTicks
                      - vtkTimerLog::TimerLog[i-1].CpuTicks,
                      vtkTimerLog::TimerLog[i].Event);
      }
    }
  else
    {
    vtkTimerLog::DumpEntry(os, 0, vtkTimerLog::TimerLog[0].WallTime, 0,
                    vtkTimerLog::TimerLog[0].CpuTicks, 0,
                    vtkTimerLog::TimerLog[0].Event);
    for (i=1; i<vtkTimerLog::NextEntry; i++)
      {
      vtkTimerLog::DumpEntry(os, i, vtkTimerLog::TimerLog[i].WallTime,
                      vtkTimerLog::TimerLog[i].WallTime
                      - vtkTimerLog::TimerLog[i-1].WallTime,
                      vtkTimerLog::TimerLog[i].CpuTicks,
                      vtkTimerLog::TimerLog[i].CpuTicks
                      - vtkTimerLog::TimerLog[i-1].CpuTicks,
                      vtkTimerLog::TimerLog[i].Event);
      }
    }
  
  os.close();
#endif
}


// Print method for vtkTimerLog.
void vtkTimerLog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  int i;

  os << indent << "MaxEntries: " << vtkTimerLog::MaxEntries << "\n";
  os << indent << "NextEntry: " << vtkTimerLog::NextEntry << "\n";
  os << indent << "WrapFlag: " << vtkTimerLog::WrapFlag << "\n";
  os << indent << "TicksPerSecond: " << vtkTimerLog::TicksPerSecond << "\n";
  os << "\n";

  os << indent << "Entry \tWall Time\tCpuTicks\tEvent\n";
  os << indent << "----------------------------------------------\n";

  if ( vtkTimerLog::WrapFlag )
    {
    for (i=vtkTimerLog::NextEntry; i<vtkTimerLog::MaxEntries; i++)
      {
      os << indent << i << "\t\t" << TimerLog[i].WallTime << "\t\t" << 
        TimerLog[i].CpuTicks << "\t\t" << TimerLog[i].Event << "\n";
      }
    }
  
  for (i=0; i<vtkTimerLog::NextEntry; i++)
    {
    os << indent << i << "\t\t" << TimerLog[i].WallTime << "\t\t" << 
      TimerLog[i].CpuTicks << "\t\t" << TimerLog[i].Event << "\n";
    }
  
  os << "\n" << indent << "StartTime: " << this->StartTime << "\n";
  os << indent << "WrapFlag: " << vtkTimerLog::WrapFlag << "\n";
}


// Methods to support simple timer functionality, separate from
// timer table logging.

// Returns the elapsed number of seconds since January 1, 1970. This
// is also called Universal Coordinated Time.
double vtkTimerLog::GetCurrentTime()
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

  return (currentTimeInSeconds);
}

double vtkTimerLog::GetCPUTime()
{
  double   currentCPUTime = 1.0;
#ifndef _WIN32_WCE
  currentCPUTime = (double)clock() / (double)CLOCKS_PER_SEC;
#endif
  return currentCPUTime;
}

// Set the StartTime to the current time. Used with GetElapsedTime().
void vtkTimerLog::StartTimer()
{
  this->StartTime = vtkTimerLog::GetCurrentTime();
}

// Sets EndTime to the current time. Used with GetElapsedTime().
void vtkTimerLog::StopTimer()
{
  this->EndTime = vtkTimerLog::GetCurrentTime();
}

// Returns the difference between StartTime and EndTime as 
// a floating point value indicating the elapsed time in seconds.
double vtkTimerLog::GetElapsedTime()
{
  return (this->EndTime - this->StartTime);
}

void vtkTimerLog::DumpEntry(ostream& os, int index, float ttime, 
                            float deltatime,
                            int tick, int deltatick, char *event)
{
  os << index << "   "
     << ttime << "  "
     << deltatime << "   "
     << (float)tick/vtkTimerLog::TicksPerSecond << "  "
     << (float)deltatick/vtkTimerLog::TicksPerSecond << "  ";
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

void vtkTimerLog::SetMaxEntries(int a)
{
  vtkTimerLog::MaxEntries = a;
}

int vtkTimerLog::GetMaxEntries()
{
  return vtkTimerLog::MaxEntries;
}

