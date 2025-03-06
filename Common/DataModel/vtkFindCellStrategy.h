// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkFindCellStrategy
 * @brief   helper class to manage the vtkPointSet::FindCell() method
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
 * either a locator associated with the input dataset, or possibly an
 * alternative locator defined by the strategy (subclasses of
 * vtkFindCellStrategy do this). The second important method, FindCell()
 * mimics vtkDataSet::FindCell() and can be used in place of it.
 *
 * Note that vtkFindCellStrategy is in general not thread-safe as the
 * strategies contain state used to accelerate the search process. Hence
 * if multiple threads are attempting to invoke FindCell(), each thread
 * needs to have its own instance of the vtkFindCellStrategy.
 *
 * @sa
 * vtkPointSet vtkPolyData vtkStructuredGrid vtkUnstructuredGrid
 * vtkAbstractInterpolatedVelocityField vtkClosetPointStrategy
 * vtkCellLocatorStrategy vtkClosestNPointsStrategy
 */

#ifndef vtkFindCellStrategy_h
#define vtkFindCellStrategy_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCell;
class vtkGenericCell;
class vtkPointSet;

class VTKCOMMONDATAMODEL_EXPORT vtkFindCellStrategy : public vtkObject
{
public:
  ///@{
  /**
   * Standard methods for type information and printing.
   */
  vtkTypeMacro(vtkFindCellStrategy, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * All subclasses of this class must provide an initialize method.  This
   * method performs handshaking and setup between the vtkPointSet dataset
   * and associated locator(s). A return value==0 means the initialization
   * process failed. The initialization is set up in such a way as to prevent
   * multiple locators from being built.
   */
  virtual int Initialize(vtkPointSet* ps);

  /**
   * Virtual method for finding a cell. Subclasses must satisfy this API.
   * This method is of the same signature as vtkDataSet::FindCell(). This
   * method is not thread safe: separate instances of vtkFindCellStrategy
   * should be created for each thread invoking FindCell(). This is done for
   * performance reasons to reduce the number of objects created/destroyed on
   * each FindCell() invocation.
   */
  virtual vtkIdType FindCell(double x[3], vtkCell* cell, vtkGenericCell* gencell, vtkIdType cellId,
    double tol2, int& subId, double pcoords[3], double* weights) = 0;

  /**
   * Return the closest point within a specified radius and the cell which is
   * closest to the point x. The closest point is somewhere on a cell, it
   * need not be one of the vertices of the cell. This method returns 1 if a
   * point is found within the specified radius. If there are no cells within
   * the specified radius, the method returns 0 and the values of
   * closestPoint, cellId, subId, and dist2 are undefined. This version takes
   * in a vtkGenericCell to avoid allocating and deallocating the cell.  This
   * is much faster than the version which does not take a *cell, especially
   * when this function is called many times in a row such as by a for loop,
   * where the allocation and dealloction can be done only once outside the
   * for loop.  If a closest point is found, "cell" contains the points and
   * ptIds for the cell "cellId" upon exit.  If a closest point is found,
   * inside returns the return value of the EvaluatePosition call to the
   * closest cell; inside(=1) or outside(=0).
   */
  virtual vtkIdType FindClosestPointWithinRadius(double x[3], double radius, double closestPoint[3],
    vtkGenericCell* cell, vtkIdType& cellId, int& subId, double& dist2, int& inside) = 0;

  /**
   * Quickly test if a point is inside the bounds of a particular cell.
   */
  virtual bool InsideCellBounds(double x[3], vtkIdType cellId) = 0;

  /**
   * Copy essential parameters between instances of this class. This
   * generally is used to copy from instance prototype to another, or to copy
   * strategies between thread instances.  Sub-classes can contribute to
   * the parameter copying process via chaining.
   *
   * Note: CopyParameters should ALWAYS be called BEFORE Initialize.
   */
  virtual void CopyParameters(vtkFindCellStrategy* from);

protected:
  vtkFindCellStrategy();
  ~vtkFindCellStrategy() override;

  // You may ask why this OwnsLocator rigamarole. The reason is that the reference counting garbage
  // collector gets confused when the  (cell/point) locator, point set, and strategy are all mixed
  // together; resulting in memory leaks etc, So this defines if the locator specified or taken from
  // another strategy instance or the dataset.
  bool OwnsLocator;
  // IsACopy is needed to ensure the point-set's locator is up-to-date
  // otherwise thread-safety issue can arise.
  bool IsACopy;
  vtkPointSet* PointSet; // vtkPointSet which this strategy is associated with
  double Bounds[6];      // bounding box of vtkPointSet

  vtkTimeStamp InitializeTime; // time at which strategy was initialized

private:
  vtkFindCellStrategy(const vtkFindCellStrategy&) = delete;
  void operator=(const vtkFindCellStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
