// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkJumpAndWalkCellLocator
 * @brief   point locator adaptor to perform cell Location using the jump and walk approach
 *
 * vtkJumpAndWalkCellLocator is a point locator adaptor that can accept any point locator, e.g.
 * vtkStaticPointLocator, vtkPointLocator, which in order to find a cell, initially it locates the
 * the closest point in a dataset and "jumps" to it, and then searches/walks to cells attached to
 * this point.
 *
 * By default, it will attempt to jump only the closest point, and if it fails to find a cell,
 * it will stop. If requested, it can find the N closest points that could be used to jump and
 * walk to cells attached to those points.
 *
 * Using more points will produce less false positives, i.e. the cell was not found. at the cost of
 * speed.
 *
 * While relatively fast, it does not always return the correct result (it may not find a cell,
 * since the closest cell may not be connected to the closest point(s)). vtkCellLocator,
 * vtkStaticCellLocator, or vtkCellTreeLocator will produce better results at the cost of speed.
 *
 * vtkJumpAndWalkCellLocator utilizes the following parent class parameters:
 * - CacheCellBounds             (default true)
 * - UseExistingSearchStructure  (default false)
 *
 * vtkCellTreeLocator does NOT utilize the following parameters:
 * - NumberOfCellsPerNode
 * - Automatic
 * - Level
 * - MaxLevel
 * - RetainCellLists
 *
 * @warning If a point locator adaptor is not provided, then a vtkStaticPointLocator is build.
 * Also, if the dataset is a point set and its Cell Links have not been built, FindCell and
 * FindClosestPointWithinRadius are not thread-safe.
 *
 * @sa
 * vtkAbstractCellLocator vtkCellLocator vtkStaticCellLocator vtkCellTreeLocator
 */

#ifndef vtkJumpAndWalkCellLocator_h
#define vtkJumpAndWalkCellLocator_h

#include "vtkAbstractCellLocator.h"
#include "vtkAbstractPointLocator.h"  // For vtkAbstractPointLocator
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkSMPThreadLocal.h"        // For vtkSMPThreadLocal
#include "vtkSmartPointer.h"          // For vtkSmartPointer

#include <unordered_set> // For unordered_set

VTK_ABI_NAMESPACE_BEGIN

class vtkAbstractPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkJumpAndWalkCellLocator : public vtkAbstractCellLocator
{
public:
  ///@{
  /**
   * Standard methods to instantiate, print and obtain type-related information.
   */
  vtkTypeMacro(vtkJumpAndWalkCellLocator, vtkAbstractCellLocator);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkJumpAndWalkCellLocator* New();
  ///@}

  // Reuse any superclass signatures that we don't override.
  using vtkAbstractCellLocator::FindCell;
  using vtkAbstractCellLocator::FindClosestPointWithinRadius;

  ///@{
  /**
   * Set/Get the point locator to be used internally.
   *
   * Default is vtkStaticPointLocator.
   */
  vtkSetSmartPointerMacro(PointLocator, vtkAbstractPointLocator);
  vtkGetSmartPointerMacro(PointLocator, vtkAbstractPointLocator);
  ///@}

  ///@{
  /**
   * Set/Get the number of closest points to check.
   *
   * Default is 1, meaning only the closest point will be used to jump and walk to cells attached to
   * it.
   */
  vtkSetClampMacro(NumberOfClosestPoints, unsigned int, 1, 1000);
  vtkGetMacro(NumberOfClosestPoints, unsigned int);
  ///@}

  /**
   * Find the cell containing a given point. returns -1 if no cell found
   * the cell parameters are copied into the supplied variables, a cell must
   * be provided to store the information.
   *
   * For other FindCell signatures, see vtkAbstractCellLocator.
   *
   * @warning This method is not thread-safe if the dataset is a point set and its Cell Links have
   * not been built.
   */
  vtkIdType FindCell(double x[3], double tol2, vtkGenericCell* genCell, int& subId,
    double pcoords[3], double* weights) override;

  /**
   * Return the closest point within a specified radius and the cell which is
   * closest to the point x. The closest point is somewhere on a cell, it
   * need not be one of the vertices of the cell. This method returns 1 if a
   * point is found within the specified radius. If there are no cells within
   * the specified radius, the method returns 0 and the values of
   * closestPoint, cellId, subId, and dist2 are undefined. This version takes
   * in a vtkGenericCell to avoid allocating and deallocating the cell.  This
   * is much faster than the version which does not take a cell, especially
   * when this function is called many times in a row such as by a for loop,
   * where the allocation and dealloction can be done only once outside the
   * for loop.  If a closest point is found, "cell" contains the points and
   * ptIds for the cell "cellId" upon exit. If a closest point is found,
   * inside returns the return value of the EvaluatePosition call to the
   * closest cell; inside(=1) or outside(=0).
   *
   * @warning This method is not thread-safe if the dataset is a point set and its Cell Links have
   * not been built.
   *
   * For other FindClosestPointWithinRadius signatures, see vtkAbstractCellLocator.
   */
  vtkIdType FindClosestPointWithinRadius(double x[3], double radius, double closestPoint[3],
    vtkGenericCell* genCell, vtkIdType& cellId, int& subId, double& dist2, int& inside) override;

  ///@{
  /**
   * Satisfy vtkLocator abstract interface.
   */
  void FreeSearchStructure() override;
  void BuildLocator() override;
  void ForceBuildLocator() override;
  void GenerateRepresentation(int level, vtkPolyData* pd) override;
  ///@}

  /**
   * Shallow copy of a vtkJumpAndWalkCellLocator.
   *
   * Before you shallow copy, make sure to call SetDataSet()
   */
  void ShallowCopy(vtkAbstractCellLocator* locator) override;

protected:
  vtkJumpAndWalkCellLocator();
  ~vtkJumpAndWalkCellLocator() override;

  void BuildLocatorInternal() override;

  void ReportReferences(vtkGarbageCollector*) override;

private:
  vtkJumpAndWalkCellLocator(const vtkJumpAndWalkCellLocator&) = delete;
  void operator=(const vtkJumpAndWalkCellLocator&) = delete;

  vtkIdType FindCellWalk(vtkIdList* cellIds, double x[3], double tol2, vtkGenericCell* genCell,
    int& subId, double pcoords[3], double* weights, std::unordered_set<vtkIdType>& visitedCellIds,
    vtkIdList* ptIds, vtkIdList* neighbors);

  // Bounding box of the whole dataset
  double Bounds[6] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
    VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  unsigned int NumberOfClosestPoints = 1;

  vtkSmartPointer<vtkAbstractPointLocator> PointLocator = nullptr;

  struct ThreadLocalData;
  vtkSMPThreadLocal<ThreadLocalData> TLData;
};
VTK_ABI_NAMESPACE_END

#endif // vtkJumpAndWalkCellLocator_h
