/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRandomSequence.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// .NAME vtkRandomSequence - Sequence of random numbers between 0.0 and 1.0.
// .SECTION Description
// vtkRandomSequence defines the interface of any sequence of random numbers
// between 0.0 and 1.0.
//
// At this level of abstraction, there is no assumption about the distribution
// of the numbers or about the quality of the sequence of numbers to be
// statistically independent.
//
// To the question about why a random "sequence" class instead of a random
// "generator" class or to a random "number" class?,
// see the OOSC book:
// "Object-Oriented Software Construction", 2nd Edition, by Bertrand Meyer.
// chapter 23, "Principles of class design", "Pseudo-random number generators:
// a design exercise", page 754--755.
#ifndef __vtkRandomSequence_h
#define __vtkRandomSequence_h

#include "vtkObject.h"

class VTK_COMMON_EXPORT vtkRandomSequence : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkRandomSequence,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Current value
  // \post unit_range: result>=0.0 && result<=1.0
  virtual double GetValue()=0;
  
  // Description:
  // Move to the next number in the random sequence.
  virtual void Next()=0;
  
  // Description:
  // Convenient method to return a value in a specific range from the
  // [0,1] range. There is an initial implementation that can be overridden
  // by a subclass.
  // There is no pre-condition on the range:
  // - it can be in increasing order: rangeMin<rangeMax
  // - it can be empty: rangeMin=rangeMax
  // - it can be in decreasing order: rangeMin>rangeMax
  // \post result_in_range:
  // (rangeMin<=rangeMax && result>=rangeMin && result<=rangeMax)
  // || (rangeMax<=rangeMin && result>=rangeMax && result<=rangeMin)
  virtual double GetRangeValue(double rangeMin,
                               double rangeMax);
protected:
  vtkRandomSequence();
  virtual ~vtkRandomSequence();
private:
  vtkRandomSequence(const vtkRandomSequence&);  // Not implemented.
  void operator=(const vtkRandomSequence&);  // Not implemented.
};

#endif // #ifndef __vtkRandomSequence_h
