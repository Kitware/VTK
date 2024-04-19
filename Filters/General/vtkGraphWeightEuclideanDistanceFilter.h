// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGraphWeightEuclideanDistanceFilter
 * @brief   Weights the edges of a
 * graph based on the Euclidean distance between the points.
 *
 *
 * Weights the edges of a graph based on the Euclidean distance between the points.
 */

#ifndef vtkGraphWeightEuclideanDistanceFilter_h
#define vtkGraphWeightEuclideanDistanceFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkGraphWeightFilter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkGraph;

class VTKFILTERSGENERAL_EXPORT vtkGraphWeightEuclideanDistanceFilter : public vtkGraphWeightFilter
{
public:
  static vtkGraphWeightEuclideanDistanceFilter* New();
  vtkTypeMacro(vtkGraphWeightEuclideanDistanceFilter, vtkGraphWeightFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkGraphWeightEuclideanDistanceFilter() = default;
  ~vtkGraphWeightEuclideanDistanceFilter() override = default;

  /**
   * Compute the Euclidean distance between the Points defined for the
   * vertices of a specified 'edge'.
   */
  float ComputeWeight(vtkGraph* graph, const vtkEdgeType& edge) const override;

  /**
   * Ensure that 'graph' has Points defined.
   */
  bool CheckRequirements(vtkGraph* graph) const override;

private:
  vtkGraphWeightEuclideanDistanceFilter(const vtkGraphWeightEuclideanDistanceFilter&) = delete;
  void operator=(const vtkGraphWeightEuclideanDistanceFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
