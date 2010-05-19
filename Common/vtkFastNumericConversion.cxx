/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFastNumericConversion.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFastNumericConversion - Enables fast conversion of floating point to fixed point
// .SECTION Description
// vtkFastNumericConversion uses a portable (assuming IEEE format) method for converting single and
// double precision floating point values to a fixed point representation. This allows fast
// integer flooring on platforms, such as Intel X86, in which CPU floating point flooring
// algorithms are very slow. It is based on the techniques described in Chris Hecker's article,
// "Let's Get to the (Floating) Point", in Game Developer Magazine, Feb/Mar 1996, and the
// techniques described in Michael Herf's website, http://www.stereopsis.com/FPU.html. 
// The Hecker article can be found at http://www.d6.com/users/checker/pdfs/gdmfp.pdf.
// Unfortunately, each of these techniques is incomplete, and doesn't floor properly,
// in a way that depends on how many bits are reserved for fixed point fractional use, due to
// failing to properly account for the default round-towards-even rounding mode of the X86. Thus,
// my implementation incorporates some rounding correction that undoes the rounding that the 
// FPU performs during denormalization of the floating point value. Note that
// the rounding affect I'm talking about here is not the effect on the fistp instruction,
// but rather the effect that occurs during the denormalization of a value that occurs when
// adding it to a much larger value. The bits must be shifted to the right, and when a "1" bit
// falls off the edge, the rounding mode determines what happens next, in order
// to avoid completely "losing" the 1-bit. Furthermore, my implementation works on Linux, where the
// default precision mode is 64-bit extended precision.

// This class is contributed to VTK by Chris Volpe of Applied Research Associates, Inc.
// (My employer requires me to say that -- CRV)


#include "vtkFastNumericConversion.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"

vtkStandardNewMacro(vtkFastNumericConversion);

int vtkFastNumericConversion::TestQuickFloor(double val)
  {
  return vtkFastNumericConversion::QuickFloor(val);
  }

int vtkFastNumericConversion::TestSafeFloor(double val)
  {
  return vtkFastNumericConversion::SafeFloor(val);
  }

int vtkFastNumericConversion::TestRound(double val)
  {
  return vtkFastNumericConversion::Round(val);
  }

int vtkFastNumericConversion::TestConvertFixedPointIntPart(double val)
  {
  int frac;
  return this->ConvertFixedPoint(val, frac);
  }

int vtkFastNumericConversion::TestConvertFixedPointFracPart(double val)
  {
  int frac;
  this->ConvertFixedPoint(val, frac);
  return frac;
  }

void vtkFastNumericConversion::InternalRebuild()
  {
  int i; 
  this->fixRound=.5;
  for (i=this->internalReservedFracBits; i; i--)
    {
    this->fixRound *= .5;
    }
  this->fracMask = (1<<this->internalReservedFracBits)-1;
  this->fpDenormalizer = (static_cast<unsigned long>(1) << (52-30-this->internalReservedFracBits)) * 
    this->two30() * this->BorrowBit();
  this->epTempDenormalizer = this->fpDenormalizer * (static_cast<unsigned long>(1) << (63-52));
  }


void vtkFastNumericConversion::PrintSelf(ostream &os, vtkIndent indent)
  {
  os << indent << "ReservedFracBits: " << this->internalReservedFracBits << endl;
  os << indent << "Bare time from last PerformanceTest() call: " << this->bare_time << endl;
  os << indent << "Cast time from last PerformanceTest() call: " << this->cast_time << endl;
  os << indent << "ConvertFixedPoint time from last PerformanceTest() call: " << this->convert_time << endl;
  os << indent << "QuickFloor time from last PerformanceTest() call: " << this->quickfloor_time << endl;
  os << indent << "SafeFloor time from last PerformanceTest() call: " << this->safefloor_time << endl;
  os << indent << "Round time from last PerformanceTest() call: " << this->round_time << endl;
  if (this->bare_time != 0.0)
    {
    // Don't do this if we haven't run the tests yet.
    if (this->quickfloor_time-this->bare_time > 0.0)
      {
      os << indent << "Speedup ratio from cast to quickfloor is: " << 
        (this->cast_time-this->bare_time)/(this->quickfloor_time-this->bare_time) << endl;
      }
    else
      {
      os << indent << "quickfloor_time <= bare_time, cannot calculate speedup ratio" << endl;
      }

    if (this->safefloor_time-this->bare_time > 0.0)
      {
      os << indent << "Speedup ratio from cast to safefloor is: " << 
        (this->cast_time-this->bare_time)/(this->safefloor_time-this->bare_time) << endl;
      }
    else
      {
      os << indent << "safefloor_time <= bare_time, cannot calculate speedup ratio" << endl;
      }

    if (this->round_time-this->bare_time > 0.0)
      {
      os << indent << "Speedup ratio from cast to round is: " << 
        (this->cast_time-this->bare_time)/(this->round_time-this->bare_time) << endl;
      }
    else
      {
      os << indent << "round_time <= bare_time, cannot calculate speedup ratio" << endl;
      }
    }
  }


void vtkFastNumericConversion::PerformanceTests(void)
  {
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
  this->bare_time = timer->GetElapsedTime();
  
 
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
  this->cast_time = timer->GetElapsedTime();


  // Compute convert time
  timer->StartTimer();
  for (o=0; o<outer_count; o++)
    {
    for (i=0; i<inner_count; i++)
      {
      ival[i] = this->ConvertFixedPoint(dval[i], frac[i]);
      }
    }
  timer->StopTimer();
  this->convert_time = timer->GetElapsedTime();

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
  this->quickfloor_time = timer->GetElapsedTime();

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
  this->safefloor_time = timer->GetElapsedTime();

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
  this->round_time = timer->GetElapsedTime();



  delete [] dval;
  delete [] ival;
  delete [] frac;

  timer->Delete();
  }
