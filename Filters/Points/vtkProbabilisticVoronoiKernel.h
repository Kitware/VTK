// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkProbabilisticVoronoiKernel
 * @brief   interpolate from the weighted closest point
 *
 *
 * vtkProbabilisticVoronoiKernel is an interpolation kernel that interpolates
 * from the closest weighted point from a neighborhood of points. The weights
 * refer to the probabilistic weighting that can be provided to the
 * ComputeWeights() method.
 *
 * Note that the local neighborhood is taken from the kernel footprint
 * specified in the superclass vtkGeneralizedKernel.
 *
 * @warning
 * If probability weightings are not defined, then the kernel provides the
 * same results as vtkVoronoiKernel, except a less efficiently.
 *
 * @sa
 * vtkInterpolationKernel vtkGeneralizedKernel vtkVoronoiKernel
 */

#ifndef vtkProbabilisticVoronoiKernel_h
#define vtkProbabilisticVoronoiKernel_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkGeneralizedKernel.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;
class vtkDoubleArray;

class VTKFILTERSPOINTS_EXPORT vtkProbabilisticVoronoiKernel : public vtkGeneralizedKernel
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and printing.
   */
  static vtkProbabilisticVoronoiKernel* New();
  vtkTypeMacro(vtkProbabilisticVoronoiKernel, vtkGeneralizedKernel);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  // Reuse any superclass signatures that we don't override.
  using vtkGeneralizedKernel::ComputeWeights;

  /**
   * Given a point x, a list of basis points pIds, and a probability
   * weighting function prob, compute interpolation weights associated with
   * these basis points.  Note that basis points list pIds, the probability
   * weighting prob, and the weights array are provided by the caller of the
   * method, and may be dynamically resized as necessary. The method returns
   * the number of weights (pIds may be resized in some cases). Typically
   * this method is called after ComputeBasis(), although advanced users can
   * invoke ComputeWeights() and provide the interpolation basis points pIds
   * directly. The probably weighting prob are numbers 0<=prob<=1 which are
   * multiplied against the interpolation weights before normalization. They
   * are estimates of local confidence of weights. The prob may be nullptr in
   * which all probabilities are considered =1.
   */
  vtkIdType ComputeWeights(
    double x[3], vtkIdList* pIds, vtkDoubleArray* prob, vtkDoubleArray* weights) override;

protected:
  vtkProbabilisticVoronoiKernel();
  ~vtkProbabilisticVoronoiKernel() override;

private:
  vtkProbabilisticVoronoiKernel(const vtkProbabilisticVoronoiKernel&) = delete;
  void operator=(const vtkProbabilisticVoronoiKernel&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
