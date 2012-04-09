/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoxMuellerRandomSequence.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// .NAME vtkBoxMuellerRandomSequence - Gaussian sequence of pseudo random numbers implemented with the Box-Mueller transform
// .SECTION Description
// vtkGaussianRandomSequence is a sequence of pseudo random numbers
// distributed according to the Gaussian/normal distribution (mean=0 and
// standard deviation=1).
//
// It based is calculation from a uniformly distributed pseudo random sequence.
// The initial sequence is a vtkMinimalStandardRandomSequence.

#ifndef __vtkBoxMuellerRandomSequence_h
#define __vtkBoxMuellerRandomSequence_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkGaussianRandomSequence.h"

class VTKCOMMONCORE_EXPORT vtkBoxMuellerRandomSequence: public vtkGaussianRandomSequence
{
public:
  vtkTypeMacro(vtkBoxMuellerRandomSequence,vtkGaussianRandomSequence);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkBoxMuellerRandomSequence* New();

  // Description:
  // Current value.
  virtual double GetValue();

  // Description:
  // Move to the next number in the random sequence.
  virtual void Next();

  // Description:
  // Return the uniformly distributed sequence of random numbers.
  vtkRandomSequence *GetUniformSequence();

  // Description:
  // Set the uniformly distributed sequence of random numbers.
  // Default is a .
  void SetUniformSequence(vtkRandomSequence *uniformSequence);

protected:
  vtkBoxMuellerRandomSequence();
  virtual ~vtkBoxMuellerRandomSequence();

  vtkRandomSequence *UniformSequence;
  double Value;
private:
  vtkBoxMuellerRandomSequence(const vtkBoxMuellerRandomSequence&);  // Not implemented.
  void operator=(const vtkBoxMuellerRandomSequence&);  // Not implemented.
};

#endif // #ifndef __vtkBoxMuellerRandomSequence_h
