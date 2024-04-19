// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkHyperTreeGridLocator
 * @brief abstract base class for objects that implement accelerated searches through HyperTree
 * Grids (HTGs)
 *
 * The goal of this abstract class is to define an interface to helper objects that implement
 * optimized search methods through vtkHyperTreeGrids. This class is heavily inspired from the
 * vtkLocator interface but constructed to be compatible with vtkHyperTreeGrids (which are not
 * vtkDataSets at the time of this implementation). Ideally, implementations of this interface
 * leverage the specific structure of HyperTrees and HyperTreeGrids to deliver accelerated search
 * algorithms through their data.
 *
 * As a side comment: ideally we would inherit from vtkLocator which only supports vtkDataSets right
 * now. Hopefully in the future vtkHyperTreeGrid will become a vtkDataSet or vtkCompositeDataSet and
 * we could accomplish just that with minimal refactoring.
 *
 * @sa
 * vtkHyperTreeGrid, vtkHyperTree, vtkHyperTreeGridOrientedCursor, vtkHyperTreeGridNonOrientedCursor
 */

#ifndef vtkHyperTreeGridLocator_h
#define vtkHyperTreeGridLocator_h

#include "vtkCommonDataModelModule.h" //For export macro
#include "vtkObject.h"
#include "vtkWeakPointer.h" //For HTG member

VTK_ABI_NAMESPACE_BEGIN

class vtkHyperTreeGrid;
class vtkPoints;
class vtkIdList;
class vtkGenericCell;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGridLocator : public vtkObject
{
public:
  vtkTypeMacro(vtkHyperTreeGridLocator, vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Getter/Setter methods for setting the vtkHyperTreeGrid
   */
  virtual vtkHyperTreeGrid* GetHTG();
  virtual void SetHTG(vtkHyperTreeGrid*);

  /**
   * Initialize or reinitialize the locator (setting or re-setting clean objects in memory)
   * (Does nothing)
   */
  virtual void Initialize() {}

  /**
   * Update the locator's internal variables with respect to changes that could have happened
   * outside.
   */
  virtual void Update();

  /**
   * Basic search for cell holding a given point
   * @param point coordinated of sought point
   * @return the global index of the cell holding the point (-1 if cell not found or masked)
   */
  virtual vtkIdType Search(const double point[3]) = 0;

  /**
   * Pure virtual. Find the cell where a given point lies
   * @param[in] point an array holding the coordinates of the point to search for
   * @param[in] tol tolerance level
   * @param[out] cell pointer to a cell configured with information from return value cell index
   * @param[out] subId index of the sub cell if composite cell
   * @param[out] pcoords parametric coordinates of the point in the cell
   * @param[out] weights interpolation weights of the sought point in the cell
   * @return the global index of the cell holding the point (-1 if no cell is found or masked)
   */
  virtual vtkIdType FindCell(const double point[3], double tol, vtkGenericCell* cell, int& subId,
    double pcoords[3], double* weights) = 0;

  /**
   * Pure virtual. Find first intersection of the line defined by (p0, p1) with the HTG
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
  virtual int IntersectWithLine(const double p0[3], const double p1[3], double tol, double& t,
    double x[3], double pcoords[3], int& subId, vtkIdType& cellId, vtkGenericCell* cell) = 0;

  /**
   * Pure virtual. Find all intersections of the line defined by (p0, p1) with the HTG
   * @param[in] p0 first point of the line
   * @param[in] p1 second point of the line
   * @param[in] tol tolerance level
   * @param[out] points array of points on the line intersecting the HTG
   * @param[out] cellIds array of cellIds holding the different points of the points array
   * @param[out] cell pointer to a vtkCell object corresponding to the last cellId found
   * @return an integer with 0 if no intersection could be found
   */
  virtual int IntersectWithLine(const double p0[3], const double p1[3], double tol,
    vtkPoints* points, vtkIdList* cellIds, vtkGenericCell* cell) = 0;

  ///@{
  /**
   * Get/Set tolerance used when searching for cells in the HTG.
   * Default is 0.0
   */
  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);
  ///@}

protected:
  // Constructor/Destructor defaults
  vtkHyperTreeGridLocator() = default;
  ~vtkHyperTreeGridLocator() override = default;

  /**
   * Internal reference to the HyperTreeGrid one wants to search over
   */
  vtkWeakPointer<vtkHyperTreeGrid> HTG;

  double Tolerance = 0.0;

private:
  /**
   * Deletion of copy constructors
   */
  vtkHyperTreeGridLocator(const vtkHyperTreeGridLocator&) = delete;
  void operator=(const vtkHyperTreeGridLocator&) = delete;

}; // vtkHyperTreeGridLocator

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridLocator_h
