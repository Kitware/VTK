// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME
// .SECTION Description
// this program tests the TimerLog

#include "vtkDebugLeaks.h"
#include "vtkTimerLog.h"

#include <sstream>

// this is needed for the unlink call
#if defined(__CYGWIN__)
#include <sys/unistd.h>
#elif defined(_WIN32)
#include <io.h>
#endif

#include "vtkWindows.h" // for Sleep

void otherTimerLogTest(ostream& strm)
{
  // actual test
  float a = 1.0;
  int i, j;
  strm << "Test vtkTimerLog Start" << endl;
  vtkTimerLog* timer1 = vtkTimerLog::New();

  vtkTimerLog::SetMaxEntries(8);
  timer1->StartTimer();
  for (j = 0; j < 4; j++)
  {
    vtkTimerLog::FormatAndMarkEvent("%s%d", "start", j);
    for (i = 0; i < 10000000; i++)
    {
      a *= a;
    }
#ifndef _WIN32
    sleep(1);
#else
    Sleep(1000);
#endif
    vtkTimerLog::InsertTimedEvent("Timed Event", .00001, 0);
    vtkTimerLog::FormatAndMarkEvent("%s%d", "end", j);
  }
  timer1->StopTimer();
  strm << *timer1;
  strm << "GetElapsedTime: " << timer1->GetElapsedTime() << endl;
  strm << "GetCPUTime: " << vtkTimerLog::GetCPUTime() << endl;
  vtkTimerLog::DumpLog("timing");
  vtkTimerLog::DumpLogWithIndents(&cerr, 0);
  vtkTimerLog::ResetLog();
  vtkTimerLog::CleanupLog();
  unlink("timing");

  cerr << "============== timer separator ================\n";

  vtkTimerLog::ResetLog();
  vtkTimerLog::SetMaxEntries(5);

  for (j = 0; j < 4; j++)
  {
    vtkTimerLog::MarkStartEvent("Other");
    for (i = 0; i < 10000000; i++)
    {
      a *= a;
    }
#ifndef _WIN32
    sleep(1);
#else
    Sleep(1000);
#endif
    vtkTimerLog::InsertTimedEvent("Other Timed Event", .00001, 0);
    vtkTimerLog::MarkEndEvent("Other");
  }
  timer1->StopTimer();
  strm << *timer1;
  strm << "GetElapsedTime: " << timer1->GetElapsedTime() << endl;
  strm << "GetCPUTime: " << vtkTimerLog::GetCPUTime() << endl;
  vtkTimerLog::DumpLog("timing2");
  vtkTimerLog::DumpLogWithIndents(&cerr, 0);
  timer1->PrintSelf(cerr, vtkIndent());
  vtkTimerLog::ResetLog();
  vtkTimerLog::CleanupLog();
  unlink("timing2");

  vtkTimerLog::SetMaxEntries(50);

  timer1->Delete();
  strm << "Test vtkTimerLog End" << endl;
}

void timerLogScopeTest()
{
  {
    vtkTimerLogScope timer("Test");
    {
      vtkTimerLogScope timer2("Test2");
#ifndef _WIN32
      sleep(1);
#else
      Sleep(1000);
#endif
    }
#ifndef _WIN32
    sleep(1);
#else
    Sleep(1000);
#endif
  }
  vtkTimerLog::DumpLogWithIndents(&cerr, 0);
}

int otherTimerLog(int, char*[])
{
  std::ostringstream vtkmsg_with_warning_C4701;
  otherTimerLogTest(vtkmsg_with_warning_C4701);
  timerLogScopeTest();
  return 0;
}
