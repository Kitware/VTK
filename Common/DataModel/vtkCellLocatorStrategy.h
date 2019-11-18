/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellLocatorStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class vtkAbstractCellLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkCellLocatorStrategy : public vtkFindCellStrategy
{
public:
  /**
   * Construct a vtkFindCellStrategy subclass.
   */
  static vtkCellLocatorStrategy* New();

  //@{
  /**
   * Standard methods for type information and printing.
   */
  vtkTypeMacro(vtkCellLocatorStrategy, vtkFindCellStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

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

  //@{
  /**
   * Set / get an instance of vtkAbstractCellLocator which is used to
   * implement the strategy for FindCell(). The locator is required to
   * already be built and non-NULL.
   */
  virtual void SetCellLocator(vtkAbstractCellLocator*);
  vtkGetObjectMacro(CellLocator, vtkAbstractCellLocator);
  //@}

protected:
  vtkCellLocatorStrategy();
  ~vtkCellLocatorStrategy() override;

  vtkAbstractCellLocator* CellLocator;
  bool OwnsLocator; // was the locator specified? or taken from associated point set

private:
  vtkCellLocatorStrategy(const vtkCellLocatorStrategy&) = delete;
  void operator=(const vtkCellLocatorStrategy&) = delete;
};

#endif
