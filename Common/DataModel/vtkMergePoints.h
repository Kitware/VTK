// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMergePoints
 * @brief   merge exactly coincident points
 *
 * vtkMergePoints is a locator object to quickly locate points in 3D.
 * The primary difference between vtkMergePoints and its superclass
 * vtkPointLocator is that vtkMergePoints merges precisely coincident points
 * and is therefore much faster.
 * @sa
 * vtkCleanPolyData
 */

#ifndef vtkMergePoints_h
#define vtkMergePoints_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkPointLocator.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkMergePoints : public vtkPointLocator
{
public:
  static vtkMergePoints* New();
  vtkTypeMacro(vtkMergePoints, vtkPointLocator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Determine whether point given by x[3] has been inserted into points list.
   * Return id of previously inserted point if this is true, otherwise return
   * -1.
   */
  vtkIdType IsInsertedPoint(const double x[3]) override;
  vtkIdType IsInsertedPoint(double x, double y, double z) override
  {
    return this->vtkPointLocator::IsInsertedPoint(x, y, z);
  }
  ///@}

  /**
   * Determine whether point given by x[3] has been inserted into points list.
   * Return 0 if point was already in the list, otherwise return 1. If the
   * point was not in the list, it will be ADDED.  In either case, the id of
   * the point (newly inserted or not) is returned in the ptId argument.
   * Note this combines the functionality of IsInsertedPoint() followed
   * by a call to InsertNextPoint().
   */
  int InsertUniquePoint(const double x[3], vtkIdType& ptId) override;

protected:
  vtkMergePoints() = default;
  ~vtkMergePoints() override = default;

private:
  vtkMergePoints(const vtkMergePoints&) = delete;
  void operator=(const vtkMergePoints&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
