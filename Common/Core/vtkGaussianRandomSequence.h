/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGaussianRandomSequence.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
/**
 * @class   vtkGaussianRandomSequence
 * @brief   Gaussian sequence of pseudo random numbers
 *
 * vtkGaussianRandomSequence is a sequence of pseudo random numbers
 * distributed according to the Gaussian/normal distribution (mean=0 and
 * standard deviation=1)
 *
 * This is just an interface.
*/

#ifndef vtkGaussianRandomSequence_h
#define vtkGaussianRandomSequence_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkRandomSequence.h"

class VTKCOMMONCORE_EXPORT vtkGaussianRandomSequence : public vtkRandomSequence
{
public:
  vtkTypeMacro(vtkGaussianRandomSequence,vtkRandomSequence);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Convenient method to return a value given the mean and standard deviation
   * of the Gaussian distribution from the the Gaussian distribution of mean=0
   * and standard deviation=1.0. There is an initial implementation that can
   * be overridden by a subclass.
   */
  virtual double GetScaledValue(double mean,
                                double standardDeviation);

protected:
  vtkGaussianRandomSequence();
  ~vtkGaussianRandomSequence() VTK_OVERRIDE;
private:
  vtkGaussianRandomSequence(const vtkGaussianRandomSequence&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGaussianRandomSequence&) VTK_DELETE_FUNCTION;
};

#endif // #ifndef vtkGaussianRandomSequence_h
