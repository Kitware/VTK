/*=========================================================================

  Program:   Visualization Toolkit
  Module:    otherTimerLog.cxx
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

// .NAME 
// .SECTION Description
// this program tests the TimerLog

#include "vtkTimerLog.h"
#include "vtkDebugLeaks.h"

// this is needed for the unlink call
#if defined(__CYGWIN__)
#include <sys/unistd.h>
#endif

void Test(ostream& strm)
{
  // actual test
  float a = 1.0;
  int i, j;
  strm << "Test vtkTimerLog Start" << endl;
  vtkTimerLog *timer1 = vtkTimerLog::New();

  timer1->SetMaxEntries (3);
  timer1->StartTimer();
  for (j = 0; j < 4; j++)
    {
    timer1->FormatAndMarkEvent("%s%d", "start", j);
    for (i = 0; i < 10000000; i++)
      {
      a *= a;
      }
#ifndef WIN32
    sleep (1);
#else
    Sleep(1000);
#endif
    timer1->FormatAndMarkEvent("%s%d", "end", j);
    }
  timer1->StopTimer();
  strm << *timer1;
  strm << "GetElapsedTime: " << timer1->GetElapsedTime() << endl;
  strm << "GetCPUTime: " << timer1->GetCPUTime() << endl;
  timer1->DumpLog( "timing" );
  timer1->ResetLog ();
  unlink("timing");
  timer1->Delete();
  strm << "Test vtkTimerLog End" << endl;
}


int main()
{
  vtkDebugLeaks::PromptUserOff();

  ostrstream vtkmsg; 
  Test(vtkmdg);

  return 0;
} 
