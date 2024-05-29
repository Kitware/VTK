// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDecimatePolylineDistanceStrategy
 * @brief   decimation strategy using distance between points as a metric.
 *
 * This class inherits from `vtkDecimatePolylineStrategy` and its decimation strategy
 * uses the distance between 3 consecutive points as a metric of error.
 *
 * @warning
 * The ComputeError method doesn't check the validity of its parameters for performance purposes.
 * This is up to the caller to ensure the provided data are valid.
 *
 * @sa
 * vtkDecimatePolylineFilter, vtkDecimatePolylineStrategy
 */

#ifndef vtkDecimatePolylineDistanceStrategy_h
#define vtkDecimatePolylineDistanceStrategy_h

#include "vtkDecimatePolylineStrategy.h"
#include "vtkFiltersCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSCORE_EXPORT vtkDecimatePolylineDistanceStrategy : public vtkDecimatePolylineStrategy
{
public:
  static vtkDecimatePolylineDistanceStrategy* New();

  vtkTypeMacro(vtkDecimatePolylineDistanceStrategy, vtkDecimatePolylineStrategy);

  /**
   * Method for computing the decimation error. This implementation uses the
   * distance between the origin and the line formed by the other 2 points points as metric of
   * error. The caller should ensure that the Ids provided correspond to existing and allocated
   * points in the dataset.
   * @param dataset the dataset containing the points to evaluate.
   * @param originId the Id of the origin point.
   * @param p1Id the Id of the previous point to the origin.
   * @param p2Id the Id of the next point to the origin.
   * @return the eulerian distance from the origin to the line formed by p1 and p2.
   */
  double ComputeError(
    vtkPointSet* dataset, vtkIdType originId, vtkIdType p1Id, vtkIdType p2Id) override;

protected:
  vtkDecimatePolylineDistanceStrategy() = default;
  ~vtkDecimatePolylineDistanceStrategy() override = default;

private:
  vtkDecimatePolylineDistanceStrategy(const vtkDecimatePolylineDistanceStrategy&) = delete;
  void operator=(const vtkDecimatePolylineDistanceStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
