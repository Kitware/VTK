// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDecimatePolylineCustomFieldStrategy
 * @brief   Decimation strategy using a custom point data array to retrieve the data to be used a a
 * metric.
 *
 * This class inherits from `vtkDecimatePolylineStrategy` and its decimation strategy uses the
 * values stored in a defined point data array as a metric of error. This array is retrieved using
 * the the class property FieldName.
 *
 * @warning
 * The ComputeError method doesn't check the validity of its parameters for performance purposes.
 * This is up to the caller to ensure the provided data are valid.
 *
 * @sa
 * vtkDecimatePolylineFilter
 */

#ifndef vtkDecimatePolylineCustomFieldStrategy_h
#define vtkDecimatePolylineCustomFieldStrategy_h

#include "vtkDecimatePolylineStrategy.h"
#include "vtkFiltersCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSCORE_EXPORT vtkDecimatePolylineCustomFieldStrategy
  : public vtkDecimatePolylineStrategy
{
public:
  static vtkDecimatePolylineCustomFieldStrategy* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkDecimatePolylineCustomFieldStrategy, vtkDecimatePolylineStrategy);

  /**
   * Method for computing the decimation error. This implementation uses the
   * values stored in the custom field which are associated to the three
   * vertices passed in parameter.
   * The caller should ensure that the Ids provided correspond to existing and allocated points in
   * the dataset.
   * @param dataset the dataset containing the points to evaluate.
   * @param originId the Id of the origin point.
   * @param p1Id the Id of the previous point to the origin.
   * @param p2Id the Id of the next point to the origin.
   * @return the max of the absolute distance between the values stored in the PointData array for
   * each points given in parameter.
   */
  double ComputeError(
    vtkPointSet* dataset, vtkIdType originId, vtkIdType p1Id, vtkIdType p2Id) override;

  bool IsStateValid(vtkPointSet* dataset) const override;

  ///@{
  /**
   * The name of the field containing the decimation information to
   * evaluate.
   * Defaults to an empty string.
   */
  vtkSetMacro(FieldName, std::string);
  vtkGetMacro(FieldName, std::string);
  /// @}

protected:
  vtkDecimatePolylineCustomFieldStrategy() = default;
  ~vtkDecimatePolylineCustomFieldStrategy() override = default;

private:
  vtkDecimatePolylineCustomFieldStrategy(const vtkDecimatePolylineCustomFieldStrategy&) = delete;
  void operator=(const vtkDecimatePolylineCustomFieldStrategy&) = delete;

  std::string FieldName;
};

VTK_ABI_NAMESPACE_END
#endif
