/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWendlandQuinticKernel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWendlandQuinticKernel
 * @brief   a quintic SPH interpolation kernel
 *
 *
 * vtkWendlandQuinticKernel is an smooth particle hydrodynamics interpolation kernel as
 * described by D.J. Price. This is a quintic formulation.
 *
 * @warning
 * FOr more information see D.J. Price, Smoothed particle hydrodynamics and
 * magnetohydrodynamics, J. Comput. Phys. 231:759-794, 2012. Especially
 * equation 49.
 *
 * @par Acknowledgments:
 * The following work has been generously supported by Altair Engineering
 * and FluiDyna GmbH. Please contact Steve Cosgrove or Milos Stanic for
 * more information.
 *
 * @sa
 * vtkSPHKernel vtkSPHInterpolator
*/

#ifndef vtkWendlandQuinticKernel_h
#define vtkWendlandQuinticKernel_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkSPHKernel.h"
#include <algorithm> // For std::min()

class vtkIdList;
class vtkDoubleArray;


class VTKFILTERSPOINTS_EXPORT vtkWendlandQuinticKernel : public vtkSPHKernel
{
public:
  //@{
  /**
   * Standard methods for instantiation, obtaining type information, and printing.
   */
  static vtkWendlandQuinticKernel *New();
  vtkTypeMacro(vtkWendlandQuinticKernel,vtkSPHKernel);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * Produce the computational parameters for the kernel. Invoke this method
   * after setting initial values like SpatialStep.
   */
  void Initialize(vtkAbstractPointLocator *loc, vtkDataSet *ds,
                          vtkPointData *pd) VTK_OVERRIDE;

  //@{
  /**
   * Compute weighting factor given a normalized distance from a sample point.
   * Note that the formulation is slightly different to avoid an extra operation
   * (which has the effect of affecting the NormFactor by 1/16).
   */
  double ComputeFunctionWeight(const double d) VTK_OVERRIDE
  {
    if ( d >= 2.0 )
    {
      return 0.0;
    }
    else
    {
      double tmp = 1.0 - 0.5*d;
      return (tmp*tmp*tmp*tmp) * (1.0 + 2.0*d);
    }
  }
  //@}

  //@{
  /**
   * Compute weighting factor for derivative quantities given a normalized
   * distance from a sample point.
   */
  double ComputeDerivWeight(const double d) VTK_OVERRIDE
  {
    if ( d >= 2.0 )
    {
      return 0.0;
    }
    else
    {
      double tmp = 1.0 - 0.5*d;
      return -2.0*(tmp*tmp*tmp) * (1.0 + 2.0*d) +
        2.0*(tmp*tmp*tmp*tmp);
    }
  }
  //@}

protected:
  vtkWendlandQuinticKernel();
  ~vtkWendlandQuinticKernel() VTK_OVERRIDE;

private:
  vtkWendlandQuinticKernel(const vtkWendlandQuinticKernel&) VTK_DELETE_FUNCTION;
  void operator=(const vtkWendlandQuinticKernel&) VTK_DELETE_FUNCTION;
};

#endif
