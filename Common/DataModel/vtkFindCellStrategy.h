/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFindCellStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFindCellStrategy
 * @brief   helper class to manage the vtkPointSet::FindCell() METHOD
 *
 * vtkFindCellStrategy is a helper class to manage the use of locators for
 * locating cells containing a query point x[3], the so-called FindCell()
 * method. The use of vtkDataSet::FindCell() is a common operation in
 * applications such as streamline generation and probing. However, in some
 * dataset types FindCell() can be implemented very simply (e.g.,
 * vtkImageData) while in other datasets it is a complex operation requiring
 * supplemental objects like locators to perform efficiently. In particular,
 * vtkPointSet and its subclasses (like vtkUnstructuredGrid) require complex
 * strategies to efficiently implement the FindCell() operation. Subclasses
 * of the abstract vtkFindCellStrategy implement several of these strategies.
 *
 * The are two key methods to this class and subclasses. The Initialize()
 * method negotiates with an input dataset to define the locator to use:
 * either a locator associated with the inout dataset, or possibly an
 * alternative locator defined by the strategy (subclasses do this). The
 * second important method, FindCell() mimics vtkDataSet::FindCell() and
 * can be used in place of it.
 *
 * @sa
 * vtkPointSet vtkPolyData vtkStructuredGrid vtkUnstructuredGrid
 */

#ifndef vtkFindCellStrategy_h
#define vtkFindCellStrategy_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class vtkCell;
class vtkGenericCell;
class vtkPointSet;

class VTKCOMMONDATAMODEL_EXPORT vtkFindCellStrategy : public vtkObject
{
public:
  //@{
  /**
   * Standard methdos for type information and printing.
   */
  vtkTypeMacro(vtkFindCellStrategy, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * All subclasses of this class must provide an initialize method.  This
   * method performs handshaking and setup between the vtkPointSet dataset
   * and associated locator(s). A return value==0 means the initialization
   * process failed.
   */
  virtual int Initialize(vtkPointSet* ps);

  /**
   * Virtual method for finding a cell. Subclasses must satisfy this API.
   * This method is of the same signature as vtkDataSet::FindCell().
   */
  virtual vtkIdType FindCell(double x[3], vtkCell* cell, vtkGenericCell* gencell, vtkIdType cellId,
    double tol2, int& subId, double pcoords[3], double* weights) = 0;

protected:
  vtkFindCellStrategy();
  ~vtkFindCellStrategy() override;

  vtkPointSet* PointSet; // vtkPointSet which this strategy is associated with
  double Bounds[6];      // bounding box of vtkPointSet

  vtkTimeStamp InitializeTime; // time at which strategy was initialized

private:
  vtkFindCellStrategy(const vtkFindCellStrategy&) = delete;
  void operator=(const vtkFindCellStrategy&) = delete;
};

#endif
