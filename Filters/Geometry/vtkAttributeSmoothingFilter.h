// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAttributeSmoothingFilter
 * @brief   smooth mesh point attribute data using distance weighted Laplacian kernel
 *
 * vtkAttributeSmoothingFilter is a filter that smooths point attribute data
 * using a Laplacian smoothing approach. The effect is to "relax" or "smooth"
 * the attributes, reducing high frequency information. Note that this filter
 * operates on all dataset types.
 *
 * A central concept of this filter is the point smoothing stencil. A
 * smoothing stencil for a point p(i) is the list of points p(j) which
 * connect to p(i) via an edge. To smooth the attributes of point p(i),
 * p(i)'s attribute data a(i) are iteratively averaged using the distance
 * weighted average of the attributes of a(j) (the weights w[j] sum to
 * 1). This averaging process is repeated until the maximum number of
 * iterations is reached.
 *
 * The relaxation factor R is also important as the smoothing process
 * proceeds in an iterative fashion. The a(i+1) attributes are determined
 * from the a(i) attributes as follows:
 * ```
 * a(i+1) = (1-R)*a(i) + R*sum(w(j)*a(j))
 * ```
 *
 * Convergence occurs faster for larger relaxation factors. Typically a small
 * number of iterations is required for large relaxation factors, and in
 * cases where only points adjacent to the boundary are being smoothed, a
 * single iteration with R=1 may be adequate (i.e., just a distance weighted
 * average is computed).
 *
 * To control what regions in the dataset have their attributes smoothed, it
 * is possible to specify which points (and their attributes) are allowed to
 * be smoothed (and therefore also those that are constrained). Typically
 * point data attributes may be constrained on the boundary, or only point
 * attributes directly connected (i.e., adjacent) to the boundary may be
 * allowed to change (this supports smooth transition of attributes from the
 * boundary into the interior of the mesh). Note that the meaning of a
 * boundary point (versus interior point) changes depending on the input
 * dataset type. For vtkPolyData, boundary *edges* are used to identify
 * boundary points; for all other dataset types, points used by a boundary
 * *face* are considered boundary points. It is also possible to explicitly
 * specify which points are smoothed, and those that are constrained, by
 * specifying a smooth mask associated with each input point.
 *
 * To control which point data attributes are to be smoothed, specify in
 * ExcludedArrays which arrays should not be smoothed--these data arrays are
 * simply passed through to the output of the filter.
 *
 * @warning
 * Certain data attributes cannot be correctly interpolated using this
 * filter.  For example, surface normals are expected to be |n|=1; after
 * attribute smoothing this constraint is likely to be violated. Other
 * vectors and tensors may suffer from similar issues. In such a situation,
 * specify ExcludedArrays which will not be smoothed (and simply passed
 * through to the output of the filter).
 *
 * @warning
 * Currently the distance weighting function is based on averaging, 1/r, or
 * 1/(r**2) weights (user selectable), where r is the distance between the
 * point to be smoothed and an edge connected neighbor (defined by the
 * smoothing stencil). The weights are normalized so that sum(w(i))==1. When
 * smoothing based on averaging, the weights are simply 1/n, where n is the
 * number of connected points in the stencil.
 *
 * @warning
 * The smoothing process reduces high frequency information in the data
 * attributes. With excessive smoothing (large numbers of iterations, and/or
 * a large relaxation factor) important details may be lost, and the
 * attributes will move towards an "average" value.
 *
 * @warning
 * While this filter will process any dataset type, if the input data is a 3D
 * image volume, it's likely much faster to use an image-based algorithm to
 * perform data smoothing.
 *
 * @warning
 * To determine boundary points in vtkPolyData, edges used by only one cell
 * are considered boundary (and hence the associated points defining the
 * edge). To determine boundary points for all other dataset types, a
 * vtkMarkBoundaryFilter is used to extract the boundary faces - this can be
 * time consuming for large data.
 *
 * @sa
 * vtkConstrainedSmoothingFilter vtkWindowedSincPolyDataFilter
 * vtkSmoothPolyDataFilter vtkExtractEdges vtkMarkBoundaryFilter
 */

#ifndef vtkAttributeSmoothingFilter_h
#define vtkAttributeSmoothingFilter_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkSmartPointer.h"          // For point smoothing mask
#include "vtkUnsignedCharArray.h"     // For point smoothing mask
#include <vector>                     //For std::vector<> - ExcludedArrays

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGEOMETRY_EXPORT vtkAttributeSmoothingFilter : public vtkDataSetAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, to obtain type information, and
   * print the state of a class instance.
   */
  static vtkAttributeSmoothingFilter* New();
  vtkTypeMacro(vtkAttributeSmoothingFilter, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@{

  ///@{
  /**
   * Specify the maximum number of iterations for smoothing.  The default
   * value is 5.
   */
  vtkSetClampMacro(NumberOfIterations, int, 0, VTK_INT_MAX);
  vtkGetMacro(NumberOfIterations, int);
  ///@}

  ///@{
  /**
   * Specify the relaxation factor for smoothing. As in all iterative
   * methods, the stability of the process is sensitive to this parameter. In
   * general, small relaxation factors and large numbers of iterations are
   * more stable than larger relaxation factors and smaller numbers of
   * iterations. The default value is 0.10.
   */
  vtkSetClampMacro(RelaxationFactor, double, 0.0, 1.0);
  vtkGetMacro(RelaxationFactor, double);
  ///@}

  enum SmoothingStrategyType
  {
    ALL_POINTS = 0,
    ALL_BUT_BOUNDARY = 1,
    ADJACENT_TO_BOUNDARY = 2,
    SMOOTHING_MASK = 3
  };

  ///@{
  /**
   * Indicate how to constrain smoothing of the attribute data. By default,
   * all point data attributes are smoothed (ALL_POINTS). If ALL_BUT_BOUNDARY
   * is selected, then all point attribute data except those on the boundary
   * of the mesh are smoothed.  If ADJACENT_TO_BOUNDARY is selected, then
   * only point data connected to a boundary point are smoothed, but boundary
   * and interior points are not. (ALL_BUT_BOUNDARY and ADJACENT_TO_BOUNDARY
   * are useful for transitioning from fixed boundary conditions to interior
   * data.) If desired, it is possible to explicitly specify a smoothing mask
   * controlling which points are smoothed and not smoothed. The default
   * constraint strategy is ALL_POINTS.
   */
  vtkSetClampMacro(SmoothingStrategy, int, ALL_POINTS, SMOOTHING_MASK);
  vtkGetMacro(SmoothingStrategy, int);
  void SetSmoothingStrategyToAllPoints() { this->SetSmoothingStrategy(ALL_POINTS); }
  void SetSmoothingStrategyToAllButBoundary() { this->SetSmoothingStrategy(ALL_BUT_BOUNDARY); }
  void SetSmoothingStrategyToAdjacentToBoundary()
  {
    this->SetSmoothingStrategy(ADJACENT_TO_BOUNDARY);
  }
  void SetSmoothingStrategyToSmoothingMask() { this->SetSmoothingStrategy(SMOOTHING_MASK); }
  ///@}

  ///@{
  /**
   * Specify the smoothing mask to use (which takes effect only when a
   * SMOOTHING_MASK smoothing strategy is specified). The smoothing mask is a
   * vtkUnsignedCharArray with a value ==1 at all points whose attributes are
   * to be smoothed.  The size of the data array must match the number of
   * input points. If there is a mismatch between the size of the smoothing
   * mask, and the number of input points, then an ALL_POINTS smoothing
   * strategy is used.
   */
  vtkSetSmartPointerMacro(SmoothingMask, vtkUnsignedCharArray);
  vtkGetSmartPointerMacro(SmoothingMask, vtkUnsignedCharArray);
  ///@}

  enum InterpolationWeightsType
  {
    AVERAGE = 0,
    DISTANCE = 1,
    DISTANCE2 = 2
  };

  ///@{
  /**
   * Indicate how to compute weights, using 1) a simple average of all
   * connected points in the stencil; 2) a distance-weighted (i.e., 1/r)
   * approach; or 3) distance**2-weighted (i.e., 1/(r**2) interpolation
   * weights). The default constraint strategy is distance**2-weighted (i.e.,
   * DISTANCE2).
   */
  vtkSetClampMacro(WeightsType, int, AVERAGE, DISTANCE2);
  vtkGetMacro(WeightsType, int);
  void SetWeightsTypeToAverage() { this->SetWeightsType(AVERAGE); }
  void SetWeightsTypeToDistance() { this->SetWeightsType(DISTANCE); }
  void SetWeightsTypeToDistance2() { this->SetWeightsType(DISTANCE2); }
  ///@}

  ///@{
  /**
   * Adds an array to the list of arrays which are to be excluded from the
   * interpolation process. Any specified arrays are simply passed through
   * to the filter output.
   */
  void AddExcludedArray(const std::string& excludedArray)
  {
    this->ExcludedArrays.push_back(excludedArray);
    this->Modified();
  }
  ///@}

  ///@{
  /**
   * Clears the contents of excluded array list.
   */
  void ClearExcludedArrays()
  {
    this->ExcludedArrays.clear();
    this->Modified();
  }
  ///@}

  /**
   * Return the number of excluded arrays.
   */
  int GetNumberOfExcludedArrays() { return static_cast<int>(this->ExcludedArrays.size()); }

  ///@{
  /**
   * Return the name of the ith excluded array.
   */
  const char* GetExcludedArray(int i)
  {
    if (i < 0 || i >= static_cast<int>(this->ExcludedArrays.size()))
    {
      return nullptr;
    }
    return this->ExcludedArrays[i].c_str();
  }
  ///@}

protected:
  vtkAttributeSmoothingFilter();
  ~vtkAttributeSmoothingFilter() override = default;

  int NumberOfIterations;
  double RelaxationFactor;
  int SmoothingStrategy;
  vtkSmartPointer<vtkUnsignedCharArray> SmoothingMask;
  int WeightsType;

  std::vector<std::string> ExcludedArrays;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkAttributeSmoothingFilter(const vtkAttributeSmoothingFilter&) = delete;
  void operator=(const vtkAttributeSmoothingFilter&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif
