// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellLocator
 * @brief   octree-based spatial search object to quickly locate cells
 *
 * vtkCellLocator is a spatial search object to quickly locate cells in 3D.
 * vtkCellLocator uses a uniform-level octree subdivision, where each octant
 * (an octant is also referred to as a bucket) carries an indication of
 * whether it is empty or not, and each leaf octant carries a list of the
 * cells inside of it. (An octant is not empty if it has one or more cells
 * inside of it.)  Typical operations are intersection with a line to return
 * candidate cells, or intersection with another vtkCellLocator to return
 * candidate cells.
 *
 * @warning
 * vtkCellLocator utilizes the following parent class parameters:
 * - Automatic                   (default true)
 * - Level                       (default 8)
 * - MaxLevel                    (default 8)
 * - NumberOfCellsPerNode        (default 25)
 * - CacheCellBounds             (default true)
 * - UseExistingSearchStructure  (default false)
 *
 * vtkCellLocator does NOT utilize the following parameters:
 * - Tolerance
 * - RetainCellLists
 *
 * @sa
 * vtkAbstractCellLocator vtkStaticCellLocator vtkCellTreeLocator vtkModifiedBSPTree vtkOBBTree
 */

#ifndef vtkCellLocator_h
#define vtkCellLocator_h

#include "vtkAbstractCellLocator.h"
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNew.h"                   // For vtkNew

VTK_ABI_NAMESPACE_BEGIN
class vtkIntArray;

class VTKCOMMONDATAMODEL_EXPORT vtkCellLocator : public vtkAbstractCellLocator
{
public:
  ///@{
  /**
   * Standard methods to print and obtain type-related information.
   */
  vtkTypeMacro(vtkCellLocator, vtkAbstractCellLocator);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Construct with automatic computation of divisions, averaging
   * 25 cells per bucket.
   */
  static vtkCellLocator* New();

  /**
   * Specify the average number of cells in each octant.
   */
  void SetNumberOfCellsPerBucket(int N) { this->SetNumberOfCellsPerNode(N); }
  int GetNumberOfCellsPerBucket() { return this->NumberOfCellsPerNode; }

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
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t, double x[3],
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
   *
   * For other FindClosestPointWithinRadius signatures, see vtkAbstractCellLocator.
   */
  vtkIdType FindClosestPointWithinRadius(double x[3], double radius, double closestPoint[3],
    vtkGenericCell* cell, vtkIdType& cellId, int& subId, double& dist2, int& inside) override;

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
   * Return a list of unique cell ids inside of a given bounding box. The
   * user must provide the vtkIdList to populate.
   */
  void FindCellsWithinBounds(double* bbox, vtkIdList* cells) override;

  /**
   * Take the passed line segment and intersect it with the data set.
   * For each intersection with the bounds of a cell, the cellIds
   * have the relevant information added sort by t. If cellIds is nullptr
   * pointer, then no information is generated for that list.
   *
   * Reimplemented from vtkAbstractCellLocator to showcase that it's a supported function.
   */
  void FindCellsAlongLine(
    const double p1[3], const double p2[3], double tolerance, vtkIdList* cellsIds) override
  {
    this->Superclass::FindCellsAlongLine(p1, p2, tolerance, cellsIds);
  }

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
   * Get the cells in a particular bucket.
   */
  virtual vtkIdList* GetCells(int bucket);

  /**
   * Return number of buckets available. Ensure that the locator has been
   * built before attempting to access buckets (octants).
   */
  virtual int GetNumberOfBuckets();

  /**
   * Shallow copy of a vtkCellLocator.
   *
   * Before you shallow copy, make sure to call SetDataSet()
   */
  void ShallowCopy(vtkAbstractCellLocator* locator) override;

protected:
  vtkCellLocator();
  ~vtkCellLocator() override;

  void BuildLocatorInternal() override;

  //------------------------------------------------------------------------------
  class vtkNeighborCells
  {
  public:
    vtkNeighborCells(int size);

    inline int GetNumberOfNeighbors();

    inline void Reset();

    inline int* GetPoint(int i);

    inline int InsertNextPoint(int* x);

  protected:
    vtkNew<vtkIntArray> Points;
  };

  void GetOverlappingBuckets(vtkNeighborCells& buckets, const double x[3], double dist,
    int prevMinLevel[3], int prevMaxLevel[3]);

  inline void GetBucketIndices(const double x[3], int ijk[3]);

  double Distance2ToBucket(const double x[3], int nei[3]);
  double Distance2ToBounds(const double x[3], double bounds[6]);

  int NumberOfOctants;   // number of octants in tree
  double Bounds[6];      // bounding box root octant
  double H[3];           // width of leaf octant in x-y-z directions
  int NumberOfDivisions; // number of "leaf" octant sub-divisions
  std::shared_ptr<std::vector<vtkSmartPointer<vtkIdList>>> TreeSharedPtr;
  vtkSmartPointer<vtkIdList>* Tree; // octree

  void MarkParents(const vtkSmartPointer<vtkIdList>&, int, int, int, int, int);
  int GenerateIndex(int offset, int numDivs, int i, int j, int k, vtkIdType& idx);
  void GenerateFace(
    int face, int numDivs, int i, int j, int k, vtkPoints* pts, vtkCellArray* polys);
  void ComputeOctantBounds(double octantBounds[6], int i, int j, int k);

private:
  vtkCellLocator(const vtkCellLocator&) = delete;
  void operator=(const vtkCellLocator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
