// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDecimatePolylineStrategy
 * @brief   abstract class to define a decimation strategy for the vtkDecimatePolylineFilter
 *
 * Parent class for the polyline decimation strategies. It defines a pure virtual method
 * `ComputeError` that requires an implementation for each strategy inheriting this class.
 * It also requires to be given a dataset on which it will gather the points to perform
 * the error computation.
 *
 * @sa
 * vtkDecimatePolylineFilter, vtkDecimatePolylineAngleStrategy, vtkDecimateCustomFieldStrategy,
 * vtkDecimateDistanceStrategy
 */

#ifndef vtkDecimatePolylineStrategy_h
#define vtkDecimatePolylineStrategy_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkPointSet;

class VTKFILTERSCORE_EXPORT vtkDecimatePolylineStrategy : public vtkObject
{
public:
  vtkTypeMacro(vtkDecimatePolylineStrategy, vtkObject);
  /**
   * Virtual method for computing the decimation error.
   * The caller should ensure that the Ids provided correspond to existing and allocated points in
   * the corresponding dataset.
   * @param dataset the dataset containing the points to evaluate.
   * @param originId the Id of the origin point.
   * @param p1Id the Id of the previous point to the origin.
   * @param p2Id the Id of the next point to the origin.
   * @return the decimation error between the 3 consecutive points.
   */
  virtual double ComputeError(
    vtkPointSet* dataset, vtkIdType originId, vtkIdType p1Id, vtkIdType p2Id) = 0;

  /**
   * @brief Returns whether this decimation strategy is in
   * a valid state and ready to compute errors.
   * Subclasses are expected to implement this method if they need to
   * have to ensure that some of their state is set correctly. Strategies in invalid
   * state will cause the filter to early return.
   * Returns `true` by default.
   */
  virtual bool IsStateValid(vtkPointSet* vtkNotUsed(dataset)) const { return true; }

protected:
  vtkDecimatePolylineStrategy() = default;
  ~vtkDecimatePolylineStrategy() override = default;

private:
  vtkDecimatePolylineStrategy(const vtkDecimatePolylineStrategy&) = delete;
  void operator=(const vtkDecimatePolylineStrategy&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif
