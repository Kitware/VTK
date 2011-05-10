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

vtkStandardNewMacro(vtkFastNumericConversion);


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
}
