// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDecimatePolylineFilter
 * @brief   reduce the number of lines in a polyline
 *
 * vtkDecimatePolylineFilter is a filter to reduce the number of lines in a
 * polyline. The algorithm functions by evaluating an error metric for each
 * vertex (i.e., the distance of the vertex to a line defined from the two
 * vertices on either side of the vertex). This error metric is computed
 * through strategies, there are multiple implementations available and the
 * default one is `vtkDecimatePolylineDistanceStrategy`.
 * Then, these vertices are placed into a priority queue,
 * and those with smaller errors are deleted first.
 * The decimation continues until the target reduction is reached. While the
 * filter will not delete end points, it will decimate closed loops down to a
 * single line, thereby changing topology.
 *
 * Note that a maximum error value (which expression depends on the strategy used)
 * can also be specified. This may limit the amount of decimation so the target
 * reduction may not be met. When using the `vtkDecimatePolylineDistanceStrategy`,
 * setting the maximum error value to a very small number, will eliminate colinear points.
 *
 * @warning
 * This algorithm is a very simple implementation that overlooks some
 * potential complexities. For example, if a vertex is multiply connected,
 * meaning that it is used by multiple distinct polylines, then the extra
 * topological constraints are ignored. This can produce less than optimal
 * results.
 *
 * @sa
 * vtkDecimate vtkDecimateProp vtkQuadricClustering vtkQuadricDecimation
 */

#ifndef vtkDecimatePolylineFilter_h
#define vtkDecimatePolylineFilter_h

#include "vtkDecimatePolylineDistanceStrategy.h" // Default decimation strategy
#include "vtkFiltersCoreModule.h"                // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkSmartPointer.h" // Needed for SP ivars

VTK_ABI_NAMESPACE_BEGIN
class vtkPriorityQueue;

class VTKFILTERSCORE_EXPORT vtkDecimatePolylineFilter : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for type information and printing.
   */
  vtkTypeMacro(vtkDecimatePolylineFilter, vtkPolyDataAlgorithm);
  static vtkDecimatePolylineFilter* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify the desired reduction in the total number of polygons (e.g., if
   * TargetReduction is set to 0.9, this filter will try to reduce the data set
   * to 10% of its original size).
   * Defaults to 0.9 .
   */
  vtkSetClampMacro(TargetReduction, double, 0.0, 1.0);
  vtkGetMacro(TargetReduction, double);
  ///@}

  ///@{
  /**
   * Set the largest decimation error that is allowed during the decimation
   * process. This may limit the maximum reduction that may be achieved. The
   * maximum error is dependent on the decimation strategy used, by default it is
   * specified as a fraction of the maximum length of the input data bounding box.
   * Defaults to VTK_DOUBLE_MAX.
   */
  vtkSetClampMacro(MaximumError, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(MaximumError, double);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   * Defaults to DEFAULT_PRECISION.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

  ///@{
  /**
   * Set/get the decimation strategy. See the class that inherits `vtkDecimatePolylineStrategy`
   * to look at the implemented strategies.
   * Defaults to vtkDecimatePolylineDistanceStrategy.
   */
  vtkSetMacro(DecimationStrategy, vtkDecimatePolylineStrategy*);
  vtkGetObjectMacro(DecimationStrategy, vtkDecimatePolylineStrategy);
  ///@}

  /*
   * Inherits from vtkObject GetMTime() but also checks for the DecimationStrategy
   * member MTime.
   * @return The last time the state of the DecimatePolylineFilter got modified.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkDecimatePolylineFilter() = default;
  ~vtkDecimatePolylineFilter() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  class Polyline;
  double ComputeError(vtkPolyData* input, Polyline* polyline, vtkIdType id);

  vtkNew<vtkPriorityQueue> PriorityQueue;
  double TargetReduction = 0.90;
  double MaximumError = VTK_DOUBLE_MAX;
  int OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;

private:
  vtkDecimatePolylineFilter(const vtkDecimatePolylineFilter&) = delete;
  void operator=(const vtkDecimatePolylineFilter&) = delete;

  vtkSmartPointer<vtkDecimatePolylineStrategy> DecimationStrategy =
    vtkSmartPointer<vtkDecimatePolylineDistanceStrategy>::New();
};

VTK_ABI_NAMESPACE_END
#endif
