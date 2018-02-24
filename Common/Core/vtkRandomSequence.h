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
/**
 * @class   vtkRandomSequence
 * @brief   Generate a sequence of random numbers.
 *
 * vtkRandomSequence defines the interface of any sequence of random numbers.
 *
 * At this level of abstraction, there is no assumption about the distribution
 * of the numbers or about the quality of the sequence of numbers to be
 * statistically independent. There is no assumption about the range of values.
 *
 * To the question about why a random "sequence" class instead of a random
 * "generator" class or to a random "number" class?, see the OOSC book:
 * "Object-Oriented Software Construction", 2nd Edition, by Bertrand Meyer.
 * chapter 23, "Principles of class design", "Pseudo-random number
 * generators: a design exercise", page 754--755.
*/

#ifndef vtkRandomSequence_h
#define vtkRandomSequence_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

class VTKCOMMONCORE_EXPORT vtkRandomSequence : public vtkObject
{
public:
  //@{
  /**
   * Standard methods for type information and printing.
   */
  vtkTypeMacro(vtkRandomSequence,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Initialize the sequence with a seed.
   */
  virtual void Initialize(vtkTypeUInt32 seed) = 0;

  /**
   * Return the current value.
   */
  virtual double GetValue() = 0;

  /**
   * Move to the next number in the random sequence.
   */
  virtual void Next() = 0;

protected:
  vtkRandomSequence();
  ~vtkRandomSequence() override;

private:
  vtkRandomSequence(const vtkRandomSequence&) = delete;
  void operator=(const vtkRandomSequence&) = delete;
};

#endif // #ifndef vtkRandomSequence_h
