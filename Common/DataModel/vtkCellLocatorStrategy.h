// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellLocatorStrategy
 * @brief   implement a specific vtkPointSet::FindCell() strategy based
 *          on using a cell locator
 *
 * vtkCellLocatorStrategy is implements a FindCell() strategy based on
 * using the FindCell() method in a cell locator. This is often the
 * slowest strategy, but the most robust.
 *
 * @sa
 * vtkFindCellStrategy vtkPointSet
 */

#ifndef vtkCellLocatorStrategy_h
#define vtkCellLocatorStrategy_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkFindCellStrategy.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractCellLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkCellLocatorStrategy : public vtkFindCellStrategy
{
public:
  /**
   * Construct a vtkFindCellStrategy subclass.
   */
  static vtkCellLocatorStrategy* New();

  ///@{
  /**
   * Standard methods for type information and printing.
   */
  vtkTypeMacro(vtkCellLocatorStrategy, vtkFindCellStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Provide necessary initialization method (see superclass for more
   * information).
   */
  int Initialize(vtkPointSet* ps) override;

  /**
   * Implement the specific strategy.
   */
  vtkIdType FindCell(double x[3], vtkCell* cell, vtkGenericCell* gencell, vtkIdType cellId,
    double tol2, int& subId, double pcoords[3], double* weights) override;

  /**
   * Implement the specific strategy.
   */
  vtkIdType FindClosestPointWithinRadius(double x[3], double radius, double closestPoint[3],
    vtkGenericCell* cell, vtkIdType& cellId, int& subId, double& dist2, int& inside) override;

  /**
   * Implement the specific strategy.
   */
  bool InsideCellBounds(double x[3], vtkIdType cellId) override;

  ///@{
  /**
   * Set / get an instance of vtkAbstractCellLocator which is used to
   * implement the strategy for FindCell(). The locator is required to
   * already be built and non-NULL.
   */
  virtual void SetCellLocator(vtkAbstractCellLocator*);
  vtkGetObjectMacro(CellLocator, vtkAbstractCellLocator);
  ///@}

  /**
   * Copy essential parameters between instances of this class. This
   * generally is used to copy from instance prototype to another, or to copy
   * strategies between thread instances.  Sub-classes can contribute to
   * the parameter copying process via chaining.
   */
  void CopyParameters(vtkFindCellStrategy* from) override;

protected:
  vtkCellLocatorStrategy();
  ~vtkCellLocatorStrategy() override;

  vtkAbstractCellLocator* CellLocator;

private:
  vtkCellLocatorStrategy(const vtkCellLocatorStrategy&) = delete;
  void operator=(const vtkCellLocatorStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
