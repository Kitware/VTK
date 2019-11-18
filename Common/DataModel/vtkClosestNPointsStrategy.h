/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClosestNPointsStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class VTKCOMMONDATAMODEL_EXPORT vtkClosestNPointsStrategy : public vtkClosestPointStrategy
{
public:
  /**
   * Construct a vtkFindCellStrategy subclass.
   */
  static vtkClosestNPointsStrategy* New();

  //@{
  /**
   * Standard methods for type information and printing.
   */
  vtkTypeMacro(vtkClosestNPointsStrategy, vtkClosestPointStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  // Re-use any superclass signatures that we don't override.
  using vtkClosestPointStrategy::Initialize;

  /**
   * Implement the specific strategy.
   */
  vtkIdType FindCell(double x[3], vtkCell* cell, vtkGenericCell* gencell, vtkIdType cellId,
    double tol2, int& subId, double pcoords[3], double* weights) override;

  //@{
  /**
   * Set / get the value for the N closest points.
   */
  vtkSetClampMacro(ClosestNPoints, int, 1, 100);
  vtkGetMacro(ClosestNPoints, int);
  //@}

protected:
  vtkClosestNPointsStrategy();
  ~vtkClosestNPointsStrategy() override;

  int ClosestNPoints;

private:
  vtkClosestNPointsStrategy(const vtkClosestNPointsStrategy&) = delete;
  void operator=(const vtkClosestNPointsStrategy&) = delete;
};

#endif
