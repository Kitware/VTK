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
// .NAME vtkRandomSequence - Sequence of random numbers.
// .SECTION Description
// vtkRandomSequence defines the interface of any sequence of random numbers.
//
// At this level of abstraction, there is no assumption about the distribution
// of the numbers or about the quality of the sequence of numbers to be
// statistically independent. There is no assumption about the range of values.
//
// To the question about why a random "sequence" class instead of a random
// "generator" class or to a random "number" class?,
// see the OOSC book:
// "Object-Oriented Software Construction", 2nd Edition, by Bertrand Meyer.
// chapter 23, "Principles of class design", "Pseudo-random number generators:
// a design exercise", page 754--755.
#ifndef vtkRandomSequence_h
#define vtkRandomSequence_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

class VTKCOMMONCORE_EXPORT vtkRandomSequence : public vtkObject
{
public:
  vtkTypeMacro(vtkRandomSequence,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Current value
  virtual double GetValue()=0;

  // Description:
  // Move to the next number in the random sequence.
  virtual void Next()=0;

protected:
  vtkRandomSequence();
  virtual ~vtkRandomSequence();
private:
  vtkRandomSequence(const vtkRandomSequence&);  // Not implemented.
  void operator=(const vtkRandomSequence&);  // Not implemented.
};

#endif // #ifndef vtkRandomSequence_h
