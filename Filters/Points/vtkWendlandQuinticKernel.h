// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;
class vtkDoubleArray;

class VTKFILTERSPOINTS_EXPORT vtkWendlandQuinticKernel : public vtkSPHKernel
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and printing.
   */
  static vtkWendlandQuinticKernel* New();
  vtkTypeMacro(vtkWendlandQuinticKernel, vtkSPHKernel);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Produce the computational parameters for the kernel. Invoke this method
   * after setting initial values like SpatialStep.
   */
  void Initialize(vtkAbstractPointLocator* loc, vtkDataSet* ds, vtkPointData* pd) override;

  ///@{
  /**
   * Compute weighting factor given a normalized distance from a sample point.
   * Note that the formulation is slightly different to avoid an extra operation
   * (which has the effect of affecting the NormFactor by 1/16).
   */
  double ComputeFunctionWeight(const double d) override
  {
    if (d >= 2.0)
    {
      return 0.0;
    }
    else
    {
      double tmp = 1.0 - 0.5 * d;
      return (tmp * tmp * tmp * tmp) * (1.0 + 2.0 * d);
    }
  }
  ///@}

  ///@{
  /**
   * Compute weighting factor for derivative quantities given a normalized
   * distance from a sample point.
   */
  double ComputeDerivWeight(const double d) override
  {
    if (d >= 2.0)
    {
      return 0.0;
    }
    else
    {
      double tmp = 1.0 - 0.5 * d;
      return -2.0 * (tmp * tmp * tmp) * (1.0 + 2.0 * d) + 2.0 * (tmp * tmp * tmp * tmp);
    }
  }
  ///@}

protected:
  vtkWendlandQuinticKernel();
  ~vtkWendlandQuinticKernel() override;

private:
  vtkWendlandQuinticKernel(const vtkWendlandQuinticKernel&) = delete;
  void operator=(const vtkWendlandQuinticKernel&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
