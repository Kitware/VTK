// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkClosestNPointsStrategy
 * @brief   implement a specific vtkPointSet::FindCell() strategy based
 *          on the N closest points
 *
 * vtkClosestNPointsStrategy is implements a FindCell() strategy based on
 * locating the closest N points in a dataset, and then searching attached
 * cells. This class extends its superclass vtkClosestPointStrategy by looking
 * at the additional N points.
 *
 * @sa
 * vtkFindCellStrategy vtkPointSet
 */

#ifndef vtkClosestNPointsStrategy_h
#define vtkClosestNPointsStrategy_h

#include "vtkClosestPointStrategy.h"
#include "vtkCommonDataModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkClosestNPointsStrategy : public vtkClosestPointStrategy
{
public:
  /**
   * Construct a vtkFindCellStrategy subclass.
   */
  static vtkClosestNPointsStrategy* New();

  ///@{
  /**
   * Standard methods for type information and printing.
   */
  vtkTypeMacro(vtkClosestNPointsStrategy, vtkClosestPointStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  // Reuse any superclass signatures that we don't override.
  using vtkClosestPointStrategy::Initialize;

  /**
   * Implement the specific strategy.
   */
  vtkIdType FindCell(double x[3], vtkCell* cell, vtkGenericCell* gencell, vtkIdType cellId,
    double tol2, int& subId, double pcoords[3], double* weights) override;

  ///@{
  /**
   * Set / get the value for the N closest points.
   */
  vtkSetClampMacro(ClosestNPoints, int, 1, 100);
  vtkGetMacro(ClosestNPoints, int);
  ///@}

  /**
   * Copy essential parameters between instances of this class. This
   * generally is used to copy from instance prototype to another, or to copy
   * strategies between thread instances.  Sub-classes can contribute to
   * the parameter copying process via chaining.
   */
  void CopyParameters(vtkFindCellStrategy* from) override;

protected:
  vtkClosestNPointsStrategy();
  ~vtkClosestNPointsStrategy() override;

  int ClosestNPoints;

private:
  vtkClosestNPointsStrategy(const vtkClosestNPointsStrategy&) = delete;
  void operator=(const vtkClosestNPointsStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
