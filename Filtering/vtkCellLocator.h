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
// .NAME vtkCellLocator - octree-based spatial search object to quickly locate cells
// .SECTION Description
// vtkCellLocator is a spatial search object to quickly locate cells in 3D.
// vtkCellLocator uses a uniform-level octree subdivision, where each octant
// (an octant is also referred to as a bucket) carries an indication of
// whether it is empty or not, and each leaf octant carries a list of the
// cells inside of it. (An octant is not empty if it has one or more cells
// inside of it.)  Typical operations are intersection with a line to return
// candidate cells, or intersection with another vtkCellLocator to return
// candidate cells.

// .SECTION Caveats
// Many other types of spatial locators have been developed, such as 
// variable depth octrees and kd-trees. These are often more efficient 
// for the operations described here. vtkCellLocator has been designed
// for subclassing; so these locators can be derived if necessary.

// .SECTION See Also
// vtkLocator vtkPointLocator vtkOBBTree

#ifndef __vtkCellLocator_h
#define __vtkCellLocator_h

#include "vtkLocator.h"

class vtkCellArray;
class vtkGenericCell;
class vtkIdList;
class vtkNeighborCells;
class vtkPoints;

class VTK_FILTERING_EXPORT vtkCellLocator : public vtkLocator
{
public:
  vtkTypeRevisionMacro(vtkCellLocator,vtkLocator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with automatic computation of divisions, averaging
  // 25 cells per bucket.
  static vtkCellLocator *New();

  // Description:
  // Specify the average number of cells in each octant.
  vtkSetClampMacro(NumberOfCellsPerBucket,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfCellsPerBucket,int);

  // Description:
  // Boolean controls whether the bounds of each cell are computed only
  // once and then saved.  Should be 10 to 20% faster if repeatedly 
  // calling any of the FindClosestPoint routines and the extra memory
  // won't cause disk caching (24 extra bytes per cell are required to
  // save the bounds).
  vtkSetMacro(CacheCellBounds,int);
  vtkGetMacro(CacheCellBounds,int);
  vtkBooleanMacro(CacheCellBounds,int);

  // Description:
  // Return intersection point (if any) of finite line with cells contained
  // in cell locator.
  virtual int IntersectWithLine(double a0[3], double a1[3], double tol,
                                double& t, double x[3], double pcoords[3],
                                int &subId);

  // Description:
  // Return intersection point (if any) AND the cell which was intersected by
  // the finite line.
  virtual int IntersectWithLine(double a0[3], double a1[3], double tol,
                                double& t, double x[3], double pcoords[3],
                                int &subId, vtkIdType &cellId);

  // Description:
  // Return intersection point (if any) AND the cell which was intersected by
  // the finite line. The cell is returned as a cell id and as a generic cell.
  virtual int IntersectWithLine(double a0[3], double a1[3], double tol,
                                double& t, double x[3], double pcoords[3],
                                int &subId, vtkIdType &cellId,
                                vtkGenericCell *cell);

  // Description:
  // Return the closest point and the cell which is closest to the point x.
  // The closest point is somewhere on a cell, it need not be one of the
  // vertices of the cell.
  void FindClosestPoint(double x[3], double closestPoint[3], vtkIdType &cellId,
                        int &subId, double& dist2);
  
  // Description:
  // Return the closest point and the cell which is closest to the point x.
  // The closest point is somewhere on a cell, it need not be one of the
  // vertices of the cell.  This version takes in a vtkGenericCell
  // to avoid allocating and deallocating the cell.  This is much faster than
  // the version which does not take a *cell, especially when this function is
  // called many times in a row such as by a for loop, where the allocation and
  // deallocation can be done only once outside the for loop.  If a cell is
  // found, "cell" contains the points and ptIds for the cell "cellId" upon
  // exit.
  void FindClosestPoint(double x[3], double closestPoint[3],
                        vtkGenericCell *cell, vtkIdType &cellId, int &subId,
                        double& dist2);
  
  // Description:
  // Return the closest point within a specified radius and the cell which is
  // closest to the point x. The closest point is somewhere on a cell, it
  // need not be one of the vertices of the cell. This method returns 1 if
  // a point is found within the specified radius. If there are no cells within
  // the specified radius, the method returns 0 and the values of closestPoint,
  // cellId, subId, and dist2 are undefined.
  int FindClosestPointWithinRadius(double x[3], double radius,
                                   double closestPoint[3], vtkIdType &cellId,
                                   int &subId, double& dist2);
 
  // Description:
  // Return the closest point within a specified radius and the cell which is
  // closest to the point x. The closest point is somewhere on a cell, it
  // need not be one of the vertices of the cell. This method returns 1 if a
  // point is found within the specified radius. If there are no cells within
  // the specified radius, the method returns 0 and the values of
  // closestPoint, cellId, subId, and dist2 are undefined. This version takes
  // in a vtkGenericCell to avoid allocating and deallocating the cell.  This
  // is much faster than the version which does not take a *cell, especially
  // when this function is called many times in a row such as by a for loop,
  // where the allocation and deallocation can be done only once outside the
  // for loop.  If a closest point is found, "cell" contains the points and
  // ptIds for the cell "cellId" upon exit.
  int FindClosestPointWithinRadius(double x[3], double radius,
                                   double closestPoint[3],
                                   vtkGenericCell *cell, vtkIdType &cellId,
                                   int &subId, double& dist2);

  // Description:
  // Return the closest point within a specified radius and the cell which is
  // closest to the point x. The closest point is somewhere on a cell, it
  // need not be one of the vertices of the cell. This method returns 1 if a
  // point is found within the specified radius. If there are no cells within
  // the specified radius, the method returns 0 and the values of
  // closestPoint, cellId, subId, and dist2 are undefined. This version takes
  // in a vtkGenericCell to avoid allocating and deallocating the cell.  This
  // is much faster than the version which does not take a *cell, especially
  // when this function is called many times in a row such as by a for loop,
  // where the allocation and dealloction can be done only once outside the
  // for loop.  If a closest point is found, "cell" contains the points and
  // ptIds for the cell "cellId" upon exit.  If a closest point is found,
  // inside returns the return value of the EvaluatePosition call to the
  // closest cell; inside(=1) or outside(=0).
  int FindClosestPointWithinRadius(double x[3], double radius,
                                   double closestPoint[3],
                                   vtkGenericCell *cell, vtkIdType &cellId,
                                   int &subId, double& dist2, int &inside);
  
  // Description:
  // Get the cells in a particular bucket.
  virtual vtkIdList *GetCells(int bucket);

  // Description:
  // Return number of buckets available. Insure that the locator has been 
  // built before attempting to access buckets (octants).
  virtual int GetNumberOfBuckets(void);

  // Description:
  // Satisfy vtkLocator abstract interface.
  void FreeSearchStructure();
  void BuildLocator();
  void GenerateRepresentation(int level, vtkPolyData *pd);
  
protected:
  vtkCellLocator();
  ~vtkCellLocator();

  void GetBucketNeighbors(int ijk[3], int ndivs, int level);
  void GetOverlappingBuckets(double x[3], int ijk[3], double dist, 
                             int prevMinLevel[3], int prevMaxLevel[3]);

  void ClearCellHasBeenVisited();
  void ClearCellHasBeenVisited(int id);

  double Distance2ToBucket(double x[3], int nei[3]);
  double Distance2ToBounds(double x[3], double bounds[6]);
  
  int NumberOfCellsPerBucket; // cells per octant
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
  int CacheCellBounds;
//BTX - begin tcl exclude
  double (*CellBounds)[6];
//ETX - end tcl exclude

  void ComputeOctantBounds(int i, int j, int k);
  double OctantBounds[6]; //the bounds of the current octant
  int IsInOctantBounds(double x[3])
    {
    if ( this->OctantBounds[0] <= x[0] && x[0] <= this->OctantBounds[1] &&
         this->OctantBounds[2] <= x[1] && x[1] <= this->OctantBounds[3] &&
         this->OctantBounds[4] <= x[2] && x[2] <= this->OctantBounds[5] )
      {
      return 1;
      }
    else
      {
      return 0;
      }
    }

private:
  vtkCellLocator(const vtkCellLocator&);  // Not implemented.
  void operator=(const vtkCellLocator&);  // Not implemented.
};

#endif


