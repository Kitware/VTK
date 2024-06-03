// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDecimatePolylineAngleStrategy
 * @brief   Decimation strategy using the angle between 3 consecutive points as a metric.
 *
 * This class inherits from `vtkDecimatePolylineStrategy` and its decimation strategy
 * uses the angle between 3 consecutive points as a metric of error.
 *
 * @warning
 * The ComputeError method doesn't check the validity of its parameters for performance purposes.
 * This is up to the caller to ensure the provided data are valid.
 *
 * @sa
 * vtkDecimatePolylineFilter, vtkDecimatePolylineStrategy
 */

#ifndef vtkDecimatePolylineAngleStrategy_h
#define vtkDecimatePolylineAngleStrategy_h

#include "vtkDecimatePolylineStrategy.h"
#include "vtkFiltersCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSCORE_EXPORT vtkDecimatePolylineAngleStrategy : public vtkDecimatePolylineStrategy
{
public:
  static vtkDecimatePolylineAngleStrategy* New();

  vtkTypeMacro(vtkDecimatePolylineAngleStrategy, vtkDecimatePolylineStrategy);

  /**
   * Method for computing the decimation error. This implementation uses the
   * angle between the origin and the points as metric of error.
   * The caller should ensure that the Ids provided correspond to existing and allocated points in
   * the dataset.
   * @param dataset the dataset containing the points to evaluate.
   * @param originId the Id of the origin point.
   * @param p1Id the Id of the previous point to the origin.
   * @param p2Id the Id of the next point to the origin.
   * @return the cosinus of the angle between origin-p1 and origin-p2.
   */
  double ComputeError(
    vtkPointSet* dataset, vtkIdType originId, vtkIdType p1Id, vtkIdType p2Id) override;

protected:
  vtkDecimatePolylineAngleStrategy() = default;
  ~vtkDecimatePolylineAngleStrategy() override = default;

private:
  vtkDecimatePolylineAngleStrategy(const vtkDecimatePolylineAngleStrategy&) = delete;
  void operator=(const vtkDecimatePolylineAngleStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
