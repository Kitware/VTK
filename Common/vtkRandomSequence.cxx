/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRandomSequence.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkRandomSequence.h"

#include <cassert>

vtkCxxRevisionMacro(vtkRandomSequence, "1.1");

// ----------------------------------------------------------------------------
vtkRandomSequence::vtkRandomSequence()
{
}

// ----------------------------------------------------------------------------
vtkRandomSequence::~vtkRandomSequence()
{
}

// ----------------------------------------------------------------------------
double vtkRandomSequence::GetRangeValue(double rangeMin,
                                        double rangeMax)
{
  double result;
  if(rangeMin==rangeMax)
    {
    result=rangeMin;
    }
  else
    {
    result=rangeMin+this->GetValue()*(rangeMax-rangeMin);
    }
  assert("post: valid_result" &&
         ((rangeMin<=rangeMax && result>=rangeMin && result<=rangeMax)
          || (rangeMax<=rangeMin && result>=rangeMax && result<=rangeMin)));
  return result;
}

// ----------------------------------------------------------------------------
void vtkRandomSequence::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
