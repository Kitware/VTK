/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTimerLog.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#endif

// initialze the class variables
int vtkTimerLog::MaxEntries = 100;
int vtkTimerLog::NextEntry = 0;
int vtkTimerLog::WrapFlag = 0;
vtkTimerLogEntry *vtkTimerLog::TimerLog = NULL;
#ifdef _WIN32
int vtkTimerLog::TicksPerSecond = 60;
#else
int vtkTimerLog::TicksPerSecond = CLK_TCK;
#endif

#ifdef _WIN32
timeb vtkTimerLog::FirstWallTime;
timeb vtkTimerLog::CurrentWallTime;
#else
timeval vtkTimerLog::FirstWallTime;
timeval vtkTimerLog::CurrentWallTime;
tms     vtkTimerLog::FirstCpuTicks;
tms     vtkTimerLog::CurrentCpuTicks;
#endif

// Description:
// Allocate timing table with MaxEntries elements.
void vtkTimerLog::AllocateLog()
{
  if (vtkTimerLog::TimerLog != NULL) delete [] vtkTimerLog::TimerLog;
  vtkTimerLog::TimerLog = new vtkTimerLogEntry[vtkTimerLog::MaxEntries];
}


// Description:
// Clear the timing table.  walltime and cputime will also be set
// to zero when the first new event is recorded.
void vtkTimerLog::ResetLog()
{
  vtkTimerLog::WrapFlag = 0;
  vtkTimerLog::NextEntry = 0;
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

  vtkTimerLog::MarkEvent(event);
}


// Description:
// Record a timing event and capture walltime and cputicks.
void vtkTimerLog::MarkEvent(char *event)
{
  int strsize;
  double time_diff;
  int ticks_diff;

  strsize = (strlen(event)) > VTK_LOG_EVENT_LENGTH - 1 ? VTK_LOG_EVENT_LENGTH-1 : strlen(event);

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
    ftime( &(vtkTimerLog::FirstWallTime) );
#else
    gettimeofday( &(vtkTimerLog::FirstWallTime), NULL );
    times(&FirstCpuTicks);
#endif
    
    TimerLog[0].WallTime = 0.0;
    TimerLog[0].CpuTicks = 0;
    strncpy(TimerLog[0].Event, event, strsize);
    TimerLog[0].Event[strsize] = '\0';
    NextEntry = 1;
    return;
    }
  
#ifdef _WIN32
  static double scale = 1.0/1000.0;
  ftime( &(vtkTimerLog::CurrentWallTime) );
  time_diff  =  vtkTimerLog::CurrentWallTime.time - vtkTimerLog::FirstWallTime.time;
  time_diff += 
    (vtkTimerLog::CurrentWallTime.millitm - vtkTimerLog::FirstWallTime.millitm) * scale;
  ticks_diff = 0;
#else
  static double scale = 1.0/1000000.0;
  gettimeofday( &(vtkTimerLog::CurrentWallTime), NULL );
  time_diff  =  vtkTimerLog::CurrentWallTime.tv_sec - vtkTimerLog::FirstWallTime.tv_sec;
  time_diff += 
    (vtkTimerLog::CurrentWallTime.tv_usec - vtkTimerLog::FirstWallTime.tv_usec) * scale;

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

  os << indent << "MaxEntries: " << vtkTimerLog::MaxEntries << "\n";
  os << indent << "NextEntry: " << vtkTimerLog::NextEntry << "\n";
  os << indent << "WrapFlag: " << vtkTimerLog::WrapFlag << "\n";
  os << indent << "TicksPerSecond: " << vtkTimerLog::TicksPerSecond << "\n";
  os << "\n";

  os << indent << "Entry \tWall Time\tCpuTicks\tEvent\n";
  os << indent << "----------------------------------------------\n";

  if ( WrapFlag )
    {
    for (i=NextEntry; i<MaxEntries; i++)
      {
      os << indent << i << "\t\t" << TimerLog[i].WallTime << "\t\t" << 
	TimerLog[i].CpuTicks << "\t\t" << TimerLog[i].Event << "\n";
      }
    }
  
  for (i=0; i<NextEntry; i++)
    {
    os << indent << i << "\t\t" << TimerLog[i].WallTime << "\t\t" << 
      TimerLog[i].CpuTicks << "\t\t" << TimerLog[i].Event << "\n";
    }
  
  os << "\n" << indent << "StartTime: " << this->StartTime << "\n";
  os << indent << "WrapFlag: " << vtkTimerLog::WrapFlag << "\n";
}


// Methods to support simple timer functionality, separate from
// timer table logging.

// Description:
// Returns the elapsed number of seconds since January 1, 1970. This
// is also called Universal Coordinated Time.
double vtkTimerLog::GetCurrentTime()
{
  double currentTimeInSeconds;

#ifdef _WIN32
  timeb CurrentTime;
  static double scale = 1.0/1000.0;
  ftime( &CurrentTime );
  currentTimeInSeconds = CurrentTime.time + scale * CurrentTime.millitm;
#else
  timeval CurrentTime;
  static double scale = 1.0/1000000.0;
  gettimeofday( &CurrentTime, NULL );
  currentTimeInSeconds = CurrentTime.tv_sec + scale * CurrentTime.tv_usec;
#endif

  return (currentTimeInSeconds);
}

// Description:
// Set the StartTime to the current time. Used with GetElapsedTime().
void vtkTimerLog::StartTimer()
{
  this->StartTime = vtkTimerLog::GetCurrentTime();
}

// Description:
// Sets EndTime to the current time. Used with GetElapsedTime().
void vtkTimerLog::StopTimer()
{
  this->EndTime = vtkTimerLog::GetCurrentTime();
}

// Description:
// Returns the difference between StartTime and EndTime as 
// a floating point value indicating the elapsed time in seconds.
double vtkTimerLog::GetElapsedTime()
{
  return (this->EndTime - this->StartTime);
}

void vtkTimerLog::DumpEntry(ostream& os, int index, float time, 
			    float deltatime,
			    int tick, int deltatick, char *event)
{
  os << index << "   "
     << time << "  "
     << deltatime << "   "
     << (float)tick/TicksPerSecond << "  "
     << (float)deltatick/TicksPerSecond << "  ";
  if (deltatime == 0.0)
    os << "0.0   ";
  else
    os << 100.0*deltatick/TicksPerSecond/deltatime << "   ";
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
