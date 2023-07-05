// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSPHQuarticKernel
 * @brief   a quartic SPH interpolation kernel
 *
 *
 * vtkSPHQuarticKernel is an smooth particle hydrodynamics interpolation kernel as
 * described by D.J. Price. This is a quartic formulation.
 *
 * @warning
 * For more information see D.J. Price, Smoothed particle hydrodynamics and
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

#ifndef vtkSPHQuarticKernel_h
#define vtkSPHQuarticKernel_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkSPHKernel.h"
#include <algorithm> // For std::min()

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;
class vtkDoubleArray;

class VTKFILTERSPOINTS_EXPORT vtkSPHQuarticKernel : public vtkSPHKernel
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and printing.
   */
  static vtkSPHQuarticKernel* New();
  vtkTypeMacro(vtkSPHQuarticKernel, vtkSPHKernel);
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
   */
  double ComputeFunctionWeight(const double d) override
  {
    double tmp1 = 2.5 - (std::min)(d, 2.5);
    double tmp2 = 1.5 - (std::min)(d, 1.5);
    double tmp3 = 0.5 - (std::min)(d, 0.5);
    return (tmp1 * tmp1 * tmp1 * tmp1 - 5.0 * tmp2 * tmp2 * tmp2 * tmp2 +
      10.0 * tmp3 * tmp3 * tmp3 * tmp3);
  }
  ///@}

  ///@{
  /**
   * Compute weighting factor for derivative quantities given a normalized
   * distance from a sample point.
   */
  double ComputeDerivWeight(const double d) override
  {
    double tmp1 = 2.5 - (std::min)(d, 2.5);
    double tmp2 = 1.5 - (std::min)(d, 1.5);
    double tmp3 = 0.5 - (std::min)(d, 0.5);
    return (-4.0 * tmp1 * tmp1 * tmp1 + 20.0 * tmp2 * tmp2 * tmp2 - 40.0 * tmp3 * tmp3 * tmp3);
  }
  ///@}

protected:
  vtkSPHQuarticKernel();
  ~vtkSPHQuarticKernel() override;

private:
  vtkSPHQuarticKernel(const vtkSPHQuarticKernel&) = delete;
  void operator=(const vtkSPHQuarticKernel&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
