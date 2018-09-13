/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellLocator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
 * Many other types of spatial locators have been developed, such as
 * variable depth octrees and kd-trees. These are often more efficient
 * for the operations described here. vtkCellLocator has been designed
 * for subclassing; so these locators can be derived if necessary.
 *
 * @sa
 * vtkLocator vtkPointLocator vtkOBBTree
*/

#ifndef vtkCellLocator_h
#define vtkCellLocator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkAbstractCellLocator.h"

class vtkNeighborCells;

class VTKCOMMONDATAMODEL_EXPORT vtkCellLocator : public vtkAbstractCellLocator
{
public:
  vtkTypeMacro(vtkCellLocator,vtkAbstractCellLocator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct with automatic computation of divisions, averaging
   * 25 cells per bucket.
   */
  static vtkCellLocator *New();

  /**
   * Specify the average number of cells in each octant.
   */
  void SetNumberOfCellsPerBucket(int N)
  { this->SetNumberOfCellsPerNode(N); }
  int GetNumberOfCellsPerBucket()
  { return this->NumberOfCellsPerNode; }

  // Re-use any superclass signatures that we don't override.
  using vtkAbstractCellLocator::IntersectWithLine;
  using vtkAbstractCellLocator::FindCell;
  using vtkAbstractCellLocator::FindClosestPoint;
  using vtkAbstractCellLocator::FindClosestPointWithinRadius;

  /**
   * Return intersection point (if any) AND the cell which was intersected by
   * the finite line. The cell is returned as a cell id and as a generic
   * cell.  For other IntersectWithLine signatures, see
   * vtkAbstractCellLocator.  Note this is currently not thread-safe.
   */
  int IntersectWithLine(const double a0[3], const double a1[3], double tol,
                        double& t, double x[3], double pcoords[3],
                        int &subId, vtkIdType &cellId,
                        vtkGenericCell *cell) override;

  /**
   * Return the closest point and the cell which is closest to the point x.
   * The closest point is somewhere on a cell, it need not be one of the
   * vertices of the cell.  This version takes in a vtkGenericCell
   * to avoid allocating and deallocating the cell.  This is much faster than
   * the version which does not take a *cell, especially when this function is
   * called many times in a row such as by a for loop, where the allocation and
   * deallocation can be done only once outside the for loop.  If a cell is
   * found, "cell" contains the points and ptIds for the cell "cellId" upon
   * exit.
   */
  void FindClosestPoint(
    const double x[3], double closestPoint[3],
    vtkGenericCell *cell, vtkIdType &cellId,
    int &subId, double& dist2) override;

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
   * closest cell; inside(=1) or outside(=0). For other
   * FindClosestPointWithinRadius signatures, see vtkAbstractCellLocator.
   */
  vtkIdType FindClosestPointWithinRadius(
    double x[3], double radius, double closestPoint[3],
    vtkGenericCell *cell, vtkIdType &cellId,
    int &subId, double& dist2, int &inside) override;

  /**
   * Get the cells in a particular bucket.
   */
  virtual vtkIdList *GetCells(int bucket);

  /**
   * Return number of buckets available. Insure that the locator has been
   * built before attempting to access buckets (octants).
   */
  virtual int GetNumberOfBuckets(void);

  /**
   * Find the cell containing a given point. returns -1 if no cell found
   * the cell parameters are copied into the supplied variables, a cell must
   * be provided to store the information.
   */
  vtkIdType FindCell(
    double x[3], double tol2, vtkGenericCell *GenCell,
    double pcoords[3], double *weights) override;

  /**
   * Return a list of unique cell ids inside of a given bounding box. The
   * user must provide the vtkIdList to populate. This method returns data
   * only after the locator has been built.
   */
  void FindCellsWithinBounds(double *bbox, vtkIdList *cells) override;

  /**
   * Given a finite line defined by the two points (p1,p2), return the list
   * of unique cell ids in the buckets containing the line. It is possible
   * that an empty cell list is returned. The user must provide the vtkIdList
   * to populate. This method returns data only after the locator has been
   * built.
   */
  void FindCellsAlongLine(const double p1[3], const double p2[3],
                          double tolerance, vtkIdList *cells) override;

  //@{
  /**
   * Satisfy vtkLocator abstract interface.
   */
  void FreeSearchStructure() override;
  void BuildLocator() override;
  virtual void BuildLocatorIfNeeded();
  virtual void ForceBuildLocator();
  virtual void BuildLocatorInternal();
  void GenerateRepresentation(int level, vtkPolyData *pd) override;
  //@}

protected:
  vtkCellLocator();
  ~vtkCellLocator() override;

  void GetBucketNeighbors(int ijk[3], int ndivs, int level);
  void GetOverlappingBuckets(const double x[3], int ijk[3], double dist,
                             int prevMinLevel[3], int prevMaxLevel[3]);

  void ClearCellHasBeenVisited();
  void ClearCellHasBeenVisited(vtkIdType id);

  double Distance2ToBucket(const double x[3], int nei[3]);
  double Distance2ToBounds(const double x[3], double bounds[6]);

  int NumberOfOctants; // number of octants in tree
  double Bounds[6]; // bounding box root octant
  int NumberOfParents; // number of parent octants
  double H[3]; // width of leaf octant in x-y-z directions
  int NumberOfDivisions; // number of "leaf" octant sub-divisions
  vtkIdList **Tree; // octree

  void MarkParents(void*, int, int, int, int, int);
  void GetChildren(int idx, int level, int children[8]);
  int GenerateIndex(int offset, int numDivs, int i, int j, int k,
                    vtkIdType &idx);
  void GenerateFace(int face, int numDivs, int i, int j, int k,
                    vtkPoints *pts, vtkCellArray *polys);

  vtkNeighborCells *Buckets;
  unsigned char *CellHasBeenVisited;
  unsigned char QueryNumber;

  void ComputeOctantBounds(int i, int j, int k);
  double OctantBounds[6]; //the bounds of the current octant
  int IsInOctantBounds(const double x[3], double tol = 0.0)
  {
    if ( this->OctantBounds[0]-tol <= x[0] && x[0] <= this->OctantBounds[1]+tol &&
         this->OctantBounds[2]-tol <= x[1] && x[1] <= this->OctantBounds[3]+tol &&
         this->OctantBounds[4]-tol <= x[2] && x[2] <= this->OctantBounds[5]+tol )
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }

private:
  vtkCellLocator(const vtkCellLocator&) = delete;
  void operator=(const vtkCellLocator&) = delete;
};

#endif
