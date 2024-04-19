// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSPHCubicKernel
 * @brief   a cubic SPH interpolation kernel
 *
 *
 * vtkSPHCubicKernel is an smooth particle hydrodynamics interpolation kernel as
 * described by D.J. Price. This is a cubic formulation.
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

#ifndef vtkSPHCubicKernel_h
#define vtkSPHCubicKernel_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkSPHKernel.h"
#include <algorithm> // For std::min()

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;
class vtkDoubleArray;

class VTKFILTERSPOINTS_EXPORT vtkSPHCubicKernel : public vtkSPHKernel
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and printing.
   */
  static vtkSPHCubicKernel* New();
  vtkTypeMacro(vtkSPHCubicKernel, vtkSPHKernel);
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
    double tmp1 = 2.0 - (std::min)(d, 2.0);
    double tmp2 = 1.0 - (std::min)(d, 1.0);
    return (0.25 * tmp1 * tmp1 * tmp1 - tmp2 * tmp2 * tmp2);
  }
  ///@}

  ///@{
  /**
   * Compute weighting factor for derivative quantities given a normalized
   * distance from a sample point.
   */
  double ComputeDerivWeight(const double d) override
  {
    double tmp1 = 2.0 - (std::min)(d, 2.0);
    double tmp2 = 1.0 - (std::min)(d, 1.0);
    return (-0.75 * tmp1 * tmp1 + 3.0 * tmp2 * tmp2);
  }
  ///@}

protected:
  vtkSPHCubicKernel();
  ~vtkSPHCubicKernel() override;

private:
  vtkSPHCubicKernel(const vtkSPHCubicKernel&) = delete;
  void operator=(const vtkSPHCubicKernel&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
