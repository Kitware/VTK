/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProbabilisticVoronoiKernel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class vtkIdList;
class vtkDoubleArray;


class VTKFILTERSPOINTS_EXPORT vtkProbabilisticVoronoiKernel : public vtkGeneralizedKernel
{
public:
  //@{
  /**
   * Standard methods for instantiation, obtaining type information, and printing.
   */
  static vtkProbabilisticVoronoiKernel *New();
  vtkTypeMacro(vtkProbabilisticVoronoiKernel,vtkGeneralizedKernel);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  // Re-use any superclass signatures that we don't override.
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
   * are estimates of local confidence of weights. The prob may be NULL in
   * which all probabilities are considered =1.
   */
  vtkIdType ComputeWeights(double x[3], vtkIdList *pIds,
                                   vtkDoubleArray *prob, vtkDoubleArray *weights) VTK_OVERRIDE;

protected:
  vtkProbabilisticVoronoiKernel();
  ~vtkProbabilisticVoronoiKernel() VTK_OVERRIDE;

private:
  vtkProbabilisticVoronoiKernel(const vtkProbabilisticVoronoiKernel&) VTK_DELETE_FUNCTION;
  void operator=(const vtkProbabilisticVoronoiKernel&) VTK_DELETE_FUNCTION;
};

#endif
