// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellTreeLocator
 * @brief   This class implements the data structures, construction
 * algorithms for fast cell location.
 *
 * Cell Tree is a bounding interval hierarchy based data structure, where child boxes
 * do not form an exact split of the parent boxes along a dimension.  Therefore two axis-
 * aligned bounding planes (left max and right min) are stored for each node along a
 * dimension. This class implements the data structure (Cell Tree Node) and its build
 * and traversal algorithms described in the paper.
 * Some methods in building and traversing the cell tree in this class were derived
 * from avtCellLocatorBIH class in the VisIT Visualization Tool.
 *
 * vtkCellTreeLocator utilizes the following parent class parameters:
 * - NumberOfCellsPerNode        (default 8)
 * - CacheCellBounds             (default true)
 * - UseExistingSearchStructure  (default false)
 *
 * vtkCellTreeLocator does NOT utilize the following parameters:
 * - Automatic
 * - Level
 * - MaxLevel
 * - Tolerance
 * - RetainCellLists
 *
 * @warning
 * This class is templated. It may run slower than serial execution if the code
 * is not optimized during compilation. Build in Release or ReleaseWithDebugInfo.
 *
 * From the article: "Fast, Memory-Efficient Cell location in Unstructured Grids for Visualization"
 * by Christoph Garth and Kenneth I. Joy in VisWeek, 2011.
 *
 * @sa
 * vtkAbstractCellLocator vtkCellLocator vtkStaticCellLocator vtkModifiedBSPTree vtkOBBTree
 */

#ifndef vtkCellTreeLocator_h
#define vtkCellTreeLocator_h

#include "vtkAbstractCellLocator.h"
#include "vtkCommonDataModelModule.h" // For export macro

namespace detail
{
VTK_ABI_NAMESPACE_BEGIN
// Forward declarations for PIMPL
struct vtkCellTree;
template <typename T>
struct CellTree;
template <typename T>
struct CellTreeBuilder;
VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkCellTreeLocator : public vtkAbstractCellLocator
{
  template <typename>
  friend struct detail::CellTree;
  template <typename>
  friend struct detail::CellTreeBuilder;

public:
  ///@{
  /**
   * Standard methods to print and obtain type-related information.
   */
  vtkTypeMacro(vtkCellTreeLocator, vtkAbstractCellLocator);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Constructor sets the maximum number of cells in a leaf to 8 and number of buckets to 6.
   * Buckets are used in building the cell tree as described in the paper.
   */
  static vtkCellTreeLocator* New();

  ///@{
  /**
   * Set/Get the number of buckets.
   *
   * Default is 6.
   */
  vtkSetMacro(NumberOfBuckets, int);
  vtkGetMacro(NumberOfBuckets, int);
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
  using vtkAbstractCellLocator::IntersectWithLine;

  /**
   * Return intersection point (if any) AND the cell which was intersected by
   * the finite line. The cell is returned as a cell id and as a generic cell.
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

  /**
   * Find the cell containing a given point. returns -1 if no cell found
   * the cell parameters are copied into the supplied variables, a cell must
   * be provided to store the information.
   */
  vtkIdType FindCell(double pos[3], double vtkNotUsed(tol2), vtkGenericCell* cell, int& subId,
    double pcoords[3], double* weights) override;

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
   * Shallow copy of a vtkCellTreeLocator.
   *
   * Before you shallow copy, make sure to call SetDataSet()
   */
  void ShallowCopy(vtkAbstractCellLocator* locator) override;

protected:
  vtkCellTreeLocator();
  ~vtkCellTreeLocator() override;

  void BuildLocatorInternal() override;

  int NumberOfBuckets;
  bool LargeIds = false;

  detail::vtkCellTree* Tree;

private:
  vtkCellTreeLocator(const vtkCellTreeLocator&) = delete;
  void operator=(const vtkCellTreeLocator&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif
