/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFastNumericConversion.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkFastNumericConversion.
// .SECTION Description
// Tests performance of the vtkFastNumericConversion methods.

#include "vtkFastNumericConversion.h"
#include "vtkTimerLog.h"


int TestFastNumericConversion(int,char *[])
{
  double bare_time;
  double cast_time;
  double convert_time;
  double quickfloor_time;
  double safefloor_time;
  double round_time;

  vtkFastNumericConversion* fnc = vtkFastNumericConversion::New();
  const int inner_count = 10000;
  const int outer_count = 10000;
  double *dval = new double[inner_count];
  int *ival = new int[inner_count];
  int *frac = new int[inner_count];


  int i,o;
  vtkTimerLog *timer = vtkTimerLog::New();

  for (i=0; i<inner_count; i++)
    {
    dval[i] = i;
    ival[i] = 0;
    }

  timer->StartTimer();
  for (o=0; o<outer_count; o++)
    {
    for (i=0; i<inner_count; i++)
      {
      // Pure bit copy
      ival[i] = *reinterpret_cast<int *>(&dval[i]);
      }
    }
  timer->StopTimer();
  bare_time = timer->GetElapsedTime();


  // Compute cast time
  timer->StartTimer();
  for (o=0; o<outer_count; o++)
    {
    for (i=0; i<inner_count; i++)
      {
      ival[i] = static_cast<int>(dval[i]);
      }
    }
  timer->StopTimer();
  cast_time = timer->GetElapsedTime();


  // Compute convert time
  timer->StartTimer();
  for (o=0; o<outer_count; o++)
    {
    for (i=0; i<inner_count; i++)
      {
      ival[i] =fnc->ConvertFixedPoint(dval[i], frac[i]);
      }
    }
  timer->StopTimer();
  convert_time = timer->GetElapsedTime();

  // Compute quickfloor time
  timer->StartTimer();
  for (o=0; o<outer_count; o++)
    {
    for (i=0; i<inner_count; i++)
      {
      ival[i] = vtkFastNumericConversion::QuickFloor(dval[i]);
      }
    }
  timer->StopTimer();
  quickfloor_time = timer->GetElapsedTime();

  // Compute safefloor time
  timer->StartTimer();
  for (o=0; o<outer_count; o++)
    {
    for (i=0; i<inner_count; i++)
      {
      ival[i] = vtkFastNumericConversion::SafeFloor(dval[i]);
      }
    }
  timer->StopTimer();
  safefloor_time = timer->GetElapsedTime();

  // Compute round time
  timer->StartTimer();
  for (o=0; o<outer_count; o++)
    {
    for (i=0; i<inner_count; i++)
      {
      ival[i] = vtkFastNumericConversion::Round(dval[i]);
      }
    }
  timer->StopTimer();
  round_time = timer->GetElapsedTime();

  delete [] dval;
  delete [] ival;
  delete [] frac;

  timer->Delete();
  fnc->Delete();

  cout << "Bare time from last PerformanceTest() call: " << bare_time << endl;
  cout << "Cast time from last PerformanceTest() call: " << cast_time << endl;
  cout << "ConvertFixedPoint time from last PerformanceTest() call: " << convert_time << endl;
  cout << "QuickFloor time from last PerformanceTest() call: " << quickfloor_time << endl;
  cout << "SafeFloor time from last PerformanceTest() call: " << safefloor_time << endl;
  cout << "Round time from last PerformanceTest() call: " << round_time << endl;

  if (bare_time != 0.0)
    {
    // Don't do this if we haven't run the tests yet.
    if (quickfloor_time-bare_time > 0.0)
      {
      cout << "Speedup ratio from cast to quickfloor is: " <<
        (cast_time-bare_time)/(quickfloor_time-bare_time) << endl;
      }
    else
      {
      cout << "quickfloor_time <= bare_time, cannot calculate speedup ratio" << endl;
      }

    if (safefloor_time-bare_time > 0.0)
      {
      cout << "Speedup ratio from cast to safefloor is: " <<
        (cast_time-bare_time)/(safefloor_time-bare_time) << endl;
      }
    else
      {
      cout <<"safefloor_time <= bare_time, cannot calculate speedup ratio" << endl;
      }

    if (round_time-bare_time > 0.0)
      {
      cout << "Speedup ratio from cast to round is: " <<
        (cast_time-bare_time)/(round_time-bare_time) << endl;
      }
    else
      {
      cout << "round_time <= bare_time, cannot calculate speedup ratio" << endl;
      }
    }

  return 0;
}
