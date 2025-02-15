// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStaticCellLocator
 * @brief   perform fast cell location operations
 *
 * vtkStaticCellLocator is a type of vtkAbstractCellLocator that accelerates
 * certain operations when performing spatial operations on cells. These
 * operations include finding a point that contains a cell, and intersecting
 * cells with a line.
 *
 * vtkStaticCellLocator is an accelerated version of vtkCellLocator. It is
 * threaded (via vtkSMPTools), and supports one-time static construction
 * (i.e., incremental cell insertion is not supported).
 *
 * @warning
 * vtkStaticCellLocator utilizes the following parent class parameters:
 * - Automatic                   (default true)
 * - NumberOfCellsPerNode        (default 10)
 * - UseExistingSearchStructure  (default false)
 *
 * vtkStaticCellLocator does NOT utilize the following parameters:
 * - CacheCellBounds             (always cached)
 * - Tolerance
 * - Level
 * - MaxLevel
 * - RetainCellLists
 *
 * @warning
 * This class is templated. It may run slower than serial execution if the code
 * is not optimized during compilation. Build in Release or ReleaseWithDebugInfo.
 *
 * @sa
 * vtkAbstractCellLocator vtkCellLocator vtkCellTreeLocator vtkModifiedBSPTree vtkOBBTree
 */

#ifndef vtkStaticCellLocator_h
#define vtkStaticCellLocator_h

#include "vtkAbstractCellLocator.h"
#include "vtkCommonDataModelModule.h" // For export macro

// Forward declarations for PIMPL
VTK_ABI_NAMESPACE_BEGIN
struct vtkCellBinner;
struct vtkCellProcessor;

class VTKCOMMONDATAMODEL_EXPORT vtkStaticCellLocator : public vtkAbstractCellLocator
{
  friend struct vtkCellBinner;
  friend struct vtkCellProcessor;

public:
  ///@{
  /**
   * Standard methods to instantiate, print and obtain type-related information.
   */
  static vtkStaticCellLocator* New();
  vtkTypeMacro(vtkStaticCellLocator, vtkAbstractCellLocator);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Set the number of divisions in x-y-z directions. If the Automatic data
   * member is enabled, the Divisions are set according to the
   * NumberOfCellsPerNode and MaxNumberOfBuckets data members. The number
   * of divisions must be >= 1 in each direction.
   */
  vtkSetVector3Macro(Divisions, int);
  vtkGetVectorMacro(Divisions, int, 3);
  ///@}

  ///@{
  /**
   * Set the maximum number of buckets in the locator. By default the value
   * is set to VTK_INT_MAX. Note that there are significant performance
   * implications at work here. If the number of buckets is set very large
   * (meaning > VTK_INT_MAX) then internal sorting may be performed using
   * 64-bit integers (which is much slower than using a 32-bit int). Of
   * course, memory requirements may dramatically increase as well.  It is
   * recommended that the default value be used; but for extremely large data
   * it may be desired to create a locator with an exceptionally large number
   * of buckets. Note also that during initialization of the locator if the
   * MaxNumberOfBuckets threshold is exceeded, the Divisions are scaled down
   * in such a way as not to exceed the MaxNumberOfBuckets proportionally to
   * the size of the bounding box in the x-y-z directions.
   */
  vtkSetClampMacro(MaxNumberOfBuckets, vtkIdType, 1000, VTK_ID_MAX);
  vtkGetMacro(MaxNumberOfBuckets, vtkIdType);
  ///@}

  /**
   * Inform the user as to whether large ids are being used. This flag only
   * has meaning after the locator has been built. Large ids are used when the
   * number of binned points, or the number of bins, is >= the maximum number
   * of buckets (specified by the user). Note that LargeIds are only available
   * on 64-bit architectures.
   */
  bool GetLargeIds() { return this->LargeIds; }

  // Reuse any superclass signatures that we don't override.
  using vtkAbstractCellLocator::FindCell;
  using vtkAbstractCellLocator::FindClosestPoint;
  using vtkAbstractCellLocator::FindClosestPointWithinRadius;
  using vtkAbstractCellLocator::IntersectWithLine;

  /**
   * Return intersection point (if any) AND the cell which was intersected by
   * the finite line. The cell is returned as a cell id and as a generic cell.
   *
   * For other IntersectWithLine signatures, see vtkAbstractCellLocator.
   */
  int IntersectWithLine(const double a0[3], const double a1[3], double tol, double& t, double x[3],
    double pcoords[3], int& subId, vtkIdType& cellId, vtkGenericCell* cell) override;

  /**
   * Take the passed line segment and intersect it with the data set.
   * The return value of the function is 0 if no intersections were found.
   * For each intersection with the bounds of a cell or with a cell (if a cell is provided),
   * the points and cellIds have the relevant information added sorted by t.
   * If points or cellIds are nullptr pointers, then no information is generated for that list.
   *
   * For other IntersectWithLine signatures, see vtkAbstractCellLocator.
   */
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, vtkPoints* points,
    vtkIdList* cellIds, vtkGenericCell* cell) override;

  /**
   * Return the closest point and the cell which is closest to the point x.
   * The closest point is somewhere on a cell, it need not be one of the
   * vertices of the cell.
   *
   * Reimplemented from vtkAbstractCellLocator to showcase that it's a supported function.
   *
   * For other FindClosestPoint signatures, see vtkAbstractCellLocator.
   */
  void FindClosestPoint(const double x[3], double closestPoint[3], vtkGenericCell* cell,
    vtkIdType& cellId, int& subId, double& dist2) override
  {
    this->Superclass::FindClosestPoint(x, closestPoint, cell, cellId, subId, dist2);
  }

  /**
   * Return the closest point within a specified radius and the cell which is
   * closest to the point x. The closest point is somewhere on a cell, it
   * need not be one of the vertices of the cell. This method returns 1 if a
   * point is found within the specified radius. If there are no cells within
   * the specified radius, the method returns 0 and the values of
   * closestPoint, cellId, subId, and dist2 are undefined. If a closest point
   * is found, inside returns the return value of the EvaluatePosition call to
   * the closest cell; inside(=1) or outside(=0).
   */
  vtkIdType FindClosestPointWithinRadius(double x[3], double radius, double closestPoint[3],
    vtkGenericCell* cell, vtkIdType& cellId, int& subId, double& dist2, int& inside) override;

  /**
   * Return a list of unique cell ids inside of a given bounding box. The
   * user must provide the vtkIdList to populate.
   */
  void FindCellsWithinBounds(double* bbox, vtkIdList* cells) override;

  /**
   * Take the passed line segment and intersect it with the data set.
   * For each intersection with the bounds of a cell, the cellIds
   * have the relevant information added. If cellIds is nullptr
   * pointer, then no information is generated for that list.
   *
   * Reimplemented from vtkAbstractCellLocator to showcase that it's a supported function.
   */
  void FindCellsAlongLine(
    const double p1[3], const double p2[3], double tolerance, vtkIdList* cellsIds) override
  {
    this->Superclass::FindCellsAlongLine(p1, p2, tolerance, cellsIds);
  }

  /**
   * Take the passed line segment and intersect it with the data set.
   * For each intersection with the bounds of a cell, the cellIds
   * have the relevant information added sort by t. If cellIds is nullptr
   * pointer, then no information is generated for that list.
   */
  void FindCellsAlongPlane(
    const double o[3], const double n[3], double tolerance, vtkIdList* cells) override;

  /**
   * Find the cell containing a given point. returns -1 if no cell found
   * the cell parameters are copied into the supplied variables, a cell must
   * be provided to store the information.
   *
   * For other FindCell signatures, see vtkAbstractCellLocator.
   */
  vtkIdType FindCell(double x[3], double vtkNotUsed(tol2), vtkGenericCell* GenCell, int& subId,
    double pcoords[3], double* weights) override;

  /**
   * Quickly test if a point is inside the bounds of a particular cell.
   * This function should be used ONLY after the locator is built.
   */
  bool InsideCellBounds(double x[3], vtkIdType cellId) override;

  ///@{
  /**
   * Satisfy vtkLocator abstract interface.
   */
  void GenerateRepresentation(int level, vtkPolyData* pd) override;
  void FreeSearchStructure() override;
  void BuildLocator() override;
  void ForceBuildLocator() override;
  ///@}

  /**
   * Shallow copy of a vtkStaticCellLocator.
   *
   * Before you shallow copy, make sure to call SetDataSet()
   */
  void ShallowCopy(vtkAbstractCellLocator* locator) override;

protected:
  vtkStaticCellLocator();
  ~vtkStaticCellLocator() override;

  void BuildLocatorInternal() override;

  double Bounds[6]; // Bounding box of the whole dataset
  int Divisions[3]; // Number of sub-divisions in x-y-z directions
  double H[3];      // Width of each bin in x-y-z directions

  vtkIdType MaxNumberOfBuckets; // Maximum number of buckets in locator
  bool LargeIds;                // indicate whether integer ids are small or large

  // Support PIMPLd implementation
  vtkCellBinner* Binner;       // Does the binning
  vtkCellProcessor* Processor; // Invokes methods (templated subclasses)

private:
  vtkStaticCellLocator(const vtkStaticCellLocator&) = delete;
  void operator=(const vtkStaticCellLocator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
