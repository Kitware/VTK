// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkHyperTreeGridGeometricLocator
 * @brief class that implements accelerated searches through HyperTree Grids (HTGs) using geometric
 * information
 *
 * The goal of this class is to implement a geometric locator search through the HTG structure. Its
 * main feature should be to expose a generic interface to finding the HTG cells that contain a
 * given geometric object. The search through the HTG is implemented using a
 * vtkHyperTreeGridNonOrientedGeometricCursor. The arborescent structure of the HTG should be
 * sufficient to accelerate the search and achieve good performance in general.
 *
 * All methods in this class should be thread safe since it is meant to be used in a multi-threaded
 * environment out of the box (except SetHTG which should be called outside any multi-threaded
 * setting).
 *
 * @sa
 * vtkHyperTreeGridLocator, vtkHyperTreeGrid, vtkHyperTree, vtkHyperTreeGridOrientedCursor,
 * vtkHyperTreeGridNonOrientedCursor
 */

#ifndef vtkHyperTreeGridGeometricLocator_h
#define vtkHyperTreeGridGeometricLocator_h

#include "vtkCommonDataModelModule.h" //For export macro
#include "vtkHyperTreeGridLocator.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkGenericCell;
class vtkPoints;
class vtkIdList;
class vtkHyperTreeGridNonOrientedGeometryCursor;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGridGeometricLocator : public vtkHyperTreeGridLocator
{
public:
  vtkTypeMacro(vtkHyperTreeGridGeometricLocator, vtkHyperTreeGridLocator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkHyperTreeGridGeometricLocator* New();

  /**
   * Set the vtkHyperTreeGrid to use for locating
   */
  void SetHTG(vtkHyperTreeGrid* candHTG) override;

  /**
   * Basic search for cell holding a given point
   * @param point coordinated of sought point
   * @return the global index of the cell holding the point (-1 if cell not found or masked)
   */
  vtkIdType Search(const double point[3]) override;

  /**
   * Basic search for cell holding a given point that also return a cursor
   * @param point coordinated of sought point
   * @param[out] cursor the cursor at the cell holding the point
   * @return the global index of the cell holding the point (-1 if cell not found or masked)
   */
  vtkIdType Search(const double point[3], vtkHyperTreeGridNonOrientedGeometryCursor* cursor);

  /**
   * Find the cell where a given point lies
   * @param[in] point an array holding the coordinates of the point to search for
   * @param[in] tol tolerance level
   * @param[out] cell pointer to a cell configured with information from return value cell index
   * @param[out] subId index of the sub cell if composite cell
   * @param[out] pcoords parametric coordinates of the point in the cell
   * @param[out] weights interpolation weights of the sought point in the cell
   * @return the global index of the cell holding the point (-1 if no cell is found or is masked)
   */
  vtkIdType FindCell(const double point[3], double tol, vtkGenericCell* cell, int& subId,
    double pcoords[3], double* weights) override;

  /**
   * Find first intersection of the line defined by (p0, p1) with the HTG
   * @param[in] p0 first point of the line
   * @param[in] p1 second point of the line
   * @param[in] tol tolerance level
   * @param[out] t pseudo-time along line path at intersection
   * @param[out] x intersection point
   * @param[out] pcoords parametric coordinatesof intersection
   * @param[out] subId index of the sub cell if composite cell
   * @param[out] cellId the global index of the intersected cell
   * @param[out] cell pointer to a vtkCell object corresponding to cellId
   * @return an integer with 0 if no intersection could be found
   */
  int IntersectWithLine(const double p0[3], const double p1[3], double tol, double& t, double x[3],
    double pcoords[3], int& subId, vtkIdType& cellId, vtkGenericCell* cell) override;

  /**
   * Find all intersections of the line defined by (p0, p1) with the HTG
   * @param[in] p0 first point of the line
   * @param[in] p1 second point of the line
   * @param[in] tol tolerance level
   * @param[out] points array of points on the line intersecting the HTG
   * @param[out] cellIds array of cellIds holding the different points of the points array
   * @param[out] cell pointer to a vtkCell object corresponding to the last cellId found
   * @return an integer with 0 if no intersection could be found
   */
  int IntersectWithLine(const double p0[3], const double p1[3], double tol, vtkPoints* points,
    vtkIdList* cellIds, vtkGenericCell* cell) override;

protected:
  vtkHyperTreeGridGeometricLocator() = default;
  ~vtkHyperTreeGridGeometricLocator() override = default;

  /**
   * The recursive part of the point search
   */
  vtkIdType RecursiveSearch(vtkHyperTreeGridNonOrientedGeometryCursor* cursor, const double pt[3]);

  /**
   * Recursive part of single line intersection search
   */
  vtkIdType RecurseSingleIntersectWithLine(const double p0[3], const double p1[3], double tol,
    vtkHyperTreeGridNonOrientedGeometryCursor* cursor, vtkGenericCell* cell, double& t, int& subId,
    double x[3], double pcoords[3]) const;

  /**
   * Recursive part of all line intersections search
   */
  void RecurseAllIntersectsWithLine(const double p0[3], const double p1[3], double tol,
    vtkHyperTreeGridNonOrientedGeometryCursor* cursor, std::vector<double>* ts, vtkPoints* points,
    vtkIdList* cellIds, vtkGenericCell* cell) const;

private:
  vtkHyperTreeGridGeometricLocator(const vtkHyperTreeGridGeometricLocator&) = delete;
  void operator=(const vtkHyperTreeGridGeometricLocator&) = delete;

  struct RecurseTreesFunctor;

  /**
   * Helper method for determining whether a cursor is a leaf or if all its children are masked
   * (does not deal with current cursor masked)
   * @param cursor the cursor to check
   */
  bool CheckLeafOrChildrenMasked(vtkHyperTreeGridNonOrientedGeometryCursor* cursor) const;

  /**
   * Helper method for determining the index of a child where the point is held
   */
  vtkIdType FindChildIndex(unsigned int dim, unsigned int bf, const double normalizedPt[3]) const;

  /**
   * Helper method for constructing a cell from a cursor
   * @param cursor the cursor pointing to the HTG cell one wishes to construct
   * @param[out] cell an already allocated cell to fill with the values from cursor
   * @return false for failure and true for success
   */
  bool ConstructCell(vtkHyperTreeGridNonOrientedGeometryCursor* cursor, vtkGenericCell* cell) const;

  /**
   * Helper method for constructing a cell from origin and size vectors
   * @param origin origin point coordinates of the point
   * @param size sizes in each dimension of the cell
   * @param[out] cell an already allocated cell to fill with the values from cursor
   * @return false for failure and true for success
   */
  bool ConstructCell(const double* origin, const double* size, vtkGenericCell* cell) const;

  /**
   * Helper method for getting origin and size vector for entire HTG
   * @param[out] origin pointer to origin data
   * @param[out] sizes pointer to size data
   */
  void GetZeroLevelOriginAndSize(double* origin, double* sizes) const;

  /**
   * Helper method for sorting indexes based on the other vector
   */
  std::vector<int> GetSortingMap(const std::vector<double>& other) const;

  /**
   * An array holding the 1D bin divisions given the branching factor
   */
  std::vector<double> Bins1D;

}; // vtkHyperTreeGridGeometricLocator

VTK_ABI_NAMESPACE_END

#endif // vtkHyperTreeGridGeometricLocator_h
