
// .NAME vtkTimerLog - Maintains timing table for performance analysis
// .SECTION Description
// vtkTimerLog contains walltime and cputime measurements associated
// with a given event.  These results can be later analyzed when
// "dumping out" the table.

#ifndef __vtkTimerLog_h
#define __vtkTimerLog_h

#include <stdio.h>
#include <fstream.h>

#ifdef _WIN32
#include <sys/types.h>
#include <sys/timeb.h>
#else
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/times.h>
#endif

// var argss
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <strings.h>

#include "vtkObject.h"


#define TIMER_LOG_EVENT_LEN		40

typedef struct
{
    float WallTime;
    int CpuTicks;
    char Event[TIMER_LOG_EVENT_LEN];
} TIMER_LOG_ENTRY;

#define DEFAULT_NUM_TIMER_LOG_ELEMS	100


class vtkTimerLog : public vtkObject 
{
public:
  vtkTimerLog();
  ~vtkTimerLog();
  char *GetClassName() {return "vtkTimerLog";};
  void DumpLog(char *filename);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetMacro( MaxEntries, int );
  vtkGetMacro( MaxEntries, int );

//BTX
  void FormatAndMarkEvent(char *EventString, ...);
//ETX
  void MarkEvent(char *EventString);

  void ResetLog();

protected:

  int			MaxEntries;
  int			NextEntry;
  int			WrapFlag;
  TIMER_LOG_ENTRY	*TimerLog;
  int			TicksPerSecond;

#ifdef _WIN32
  timeb			FirstWallTime;
  timeb			CurrentWallTime;
#else
  timeval		FirstWallTime;
  timeval		CurrentWallTime;
  tms			FirstCpuTicks;
  tms			CurrentCpuTicks;
#endif

//BTX
  void AllocateLog();

  inline void DumpEntry(ostream& os, int index, float time, float deltatime,
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
//ETX

};

#endif


