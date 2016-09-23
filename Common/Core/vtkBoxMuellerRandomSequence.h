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
/**
 * @class   vtkBoxMuellerRandomSequence
 * @brief   Gaussian sequence of pseudo random numbers implemented with the Box-Mueller transform
 *
 * vtkGaussianRandomSequence is a sequence of pseudo random numbers
 * distributed according to the Gaussian/normal distribution (mean=0 and
 * standard deviation=1).
 *
 * It based is calculation from a uniformly distributed pseudo random sequence.
 * The initial sequence is a vtkMinimalStandardRandomSequence.
*/

#ifndef vtkBoxMuellerRandomSequence_h
#define vtkBoxMuellerRandomSequence_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkGaussianRandomSequence.h"

class VTKCOMMONCORE_EXPORT vtkBoxMuellerRandomSequence: public vtkGaussianRandomSequence
{
public:
  vtkTypeMacro(vtkBoxMuellerRandomSequence,vtkGaussianRandomSequence);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkBoxMuellerRandomSequence* New();

  /**
   * Current value.
   */
  double GetValue() VTK_OVERRIDE;

  /**
   * Move to the next number in the random sequence.
   */
  void Next() VTK_OVERRIDE;

  /**
   * Return the uniformly distributed sequence of random numbers.
   */
  vtkRandomSequence *GetUniformSequence();

  /**
   * Set the uniformly distributed sequence of random numbers.
   * Default is a .
   */
  void SetUniformSequence(vtkRandomSequence *uniformSequence);

protected:
  vtkBoxMuellerRandomSequence();
  ~vtkBoxMuellerRandomSequence() VTK_OVERRIDE;

  vtkRandomSequence *UniformSequence;
  double Value;
private:
  vtkBoxMuellerRandomSequence(const vtkBoxMuellerRandomSequence&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBoxMuellerRandomSequence&) VTK_DELETE_FUNCTION;
};

#endif // #ifndef vtkBoxMuellerRandomSequence_h
