// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkClosestPointStrategy
 * @brief   implement a specific vtkPointSet::FindCell() strategy based
 *          on closest point
 *
 * vtkClosestPointStrategy is implements a FindCell() strategy based on
 * locating the closest point in a dataset, and then searching the attached
 * cells. While relatively fast, it does not always return the correct result
 * (it may not find a cell, since the closest cell may not be connected to the
 * closest point). vtkCellLocatorStrategy or vtkClosestNPointsStrategy will
 * produce better results at the cost of speed.
 *
 * @sa
 * vtkFindCellStrategy vtkPointSet vtkCellLocatorStrategy
 * vtkClosestNPointsStrategy
 */

#ifndef vtkClosestPointStrategy_h
#define vtkClosestPointStrategy_h

#include "vtkCell.h"                  //inline SelectCell
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkFindCellStrategy.h"
#include "vtkGenericCell.h" //inline SelectCell
#include "vtkPointSet.h"    //inline SelectCell

#include <vector> // For tracking visited cells

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;
class vtkAbstractPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkClosestPointStrategy : public vtkFindCellStrategy
{
public:
  /**
   * Construct a vtkFindCellStrategy subclass.
   */
  static vtkClosestPointStrategy* New();

  ///@{
  /**
   * Standard methods for type information and printing.
   */
  vtkTypeMacro(vtkClosestPointStrategy, vtkFindCellStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Provide the necessary initialization method (see superclass for more
   * information). This method sets up the point locator, vtkPointSet relationship.
   * It will use the vtkPointSet's default locator if not defined by
   * SetPointLocator() below.
   */
  int Initialize(vtkPointSet* ps) override;

  /**
   * Implement the specific strategy. This method should only be called
   * after the Initialize() method has been invoked.
   */
  vtkIdType FindCell(double x[3], vtkCell* cell, vtkGenericCell* gencell, vtkIdType cellId,
    double tol2, int& subId, double pcoords[3], double* weights) override;

  /**
   * Implement the specific strategy. This method should only be called
   * after the Initialize() method has been invoked.
   */
  vtkIdType FindClosestPointWithinRadius(double x[3], double radius, double closestPoint[3],
    vtkGenericCell* cell, vtkIdType& cellId, int& subId, double& dist2, int& inside) override;

  /**
   * Implement the specific strategy.
   */
  bool InsideCellBounds(double x[3], vtkIdType cellId) override;

  ///@{
  /**
   * Set / get an instance of vtkAbstractPointLocator which is used to
   * implement the strategy for FindCell(). Note if a locator is not
   * specified, then the default locator instantiated by the vtkPointSet
   * provided in the Initialize() method is used.
   */
  virtual void SetPointLocator(vtkAbstractPointLocator*);
  vtkGetObjectMacro(PointLocator, vtkAbstractPointLocator);
  ///@}

  /**
   * Subclasses use this method to select the current cell.
   */
  vtkCell* SelectCell(vtkPointSet* self, vtkIdType cellId, vtkCell* cell, vtkGenericCell* gencell);

  /**
   * Copy essential parameters between instances of this class. This
   * generally is used to copy from instance prototype to another, or to copy
   * strategies between thread instances.  Sub-classes can contribute to
   * the parameter copying process via chaining.
   */
  void CopyParameters(vtkFindCellStrategy* from) override;

protected:
  vtkClosestPointStrategy();
  ~vtkClosestPointStrategy() override;

  std::vector<unsigned char> VisitedCells; // boolean array to track visited cells
  vtkNew<vtkIdList> VisitedCellIds;        // list of visited cell ids to reset boolean array
  vtkNew<vtkIdList> PointIds;
  vtkNew<vtkIdList> Neighbors;
  vtkNew<vtkIdList> CellIds;
  vtkNew<vtkIdList> NearPointIds;
  std::vector<double> Weights;

  vtkAbstractPointLocator* PointLocator;

private:
  vtkClosestPointStrategy(const vtkClosestPointStrategy&) = delete;
  void operator=(const vtkClosestPointStrategy&) = delete;
};

// Handle cases where starting cell is provided or not
inline vtkCell* vtkClosestPointStrategy::SelectCell(
  vtkPointSet* self, vtkIdType cellId, vtkCell* cell, vtkGenericCell* gencell)
{
  if (!cell)
  {
    if (gencell)
    {
      self->GetCell(cellId, gencell);
      cell = gencell;
    }
    else
    {
      cell = self->GetCell(cellId);
    }
  }
  return cell;
}

VTK_ABI_NAMESPACE_END
#endif
