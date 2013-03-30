/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractCellLocator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAbstractCellLocator - an abstract base class for locators which find cells
// .SECTION Description
// vtkAbstractCellLocator is a spatial search object to quickly locate cells in 3D.
// vtkAbstractCellLocator supplies a basic interface which concrete subclasses
// should implement.
//
// .SECTION Warning
// When deriving a class from vtkAbstractCellLocator, one should include the
// 'hidden' member functions by the following construct in the derived class
// \verbatim
// //BTX
//  using vtkAbstractCellLocator::IntersectWithLine;
//  using vtkAbstractCellLocator::FindClosestPoint;
//  using vtkAbstractCellLocator::FindClosestPointWithinRadius;
// //ETX
// \endverbatim

//
// .SECTION See Also
// vtkLocator vtkPointLocator vtkOBBTree vtkCellLocator

#ifndef __vtkAbstractCellLocator_h
#define __vtkAbstractCellLocator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkLocator.h"

class vtkCellArray;
class vtkGenericCell;
class vtkIdList;
class vtkPoints;

class VTKCOMMONDATAMODEL_EXPORT vtkAbstractCellLocator : public vtkLocator
{
public:
  vtkTypeMacro(vtkAbstractCellLocator,vtkLocator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the preferred/maximum number of cells in each node/bucket.
  // Default 32. Locators generally operate by subdividing space into
  // smaller regions until the number of cells in each region (or node)
  // reaches the desired level.
  vtkSetClampMacro(NumberOfCellsPerNode,int,1,VTK_INT_MAX);
  vtkGetMacro(NumberOfCellsPerNode,int);

  // Description:
  // Boolean controls whether the bounds of each cell are computed only
  // once and then saved.  Should be 10 to 20% faster if repeatedly
  // calling any of the Intersect/Find routines and the extra memory
  // won't cause disk caching (24 extra bytes per cell are required to
  // save the bounds).
  vtkSetMacro(CacheCellBounds,int);
  vtkGetMacro(CacheCellBounds,int);
  vtkBooleanMacro(CacheCellBounds,int);

  // Description:
  // Boolean controls whether to maintain list of cells in each node.
  // not applicable to all implementations, but if the locator is being used
  // as a geometry simplification technique, there is no need to keep them.
  vtkSetMacro(RetainCellLists,int);
  vtkGetMacro(RetainCellLists,int);
  vtkBooleanMacro(RetainCellLists,int);

  // Description:
  // Most Locators build their search structures during BuildLocator
  // but some may delay construction until it is actually needed.
  // If LazyEvaluation is supported, this turns on/off the feature.
  // if not supported, it is ignored.
  vtkSetMacro(LazyEvaluation,int);
  vtkGetMacro(LazyEvaluation,int);
  vtkBooleanMacro(LazyEvaluation,int);

  // Description:
  // Some locators support querying a new dataset without rebuilding
  // the search structure (typically this may occur when a dataset
  // changes due to a time update, but is actually the same topology)
  // Turning on this flag enables some locators to skip the rebuilding
  // phase
  vtkSetMacro(UseExistingSearchStructure,int);
  vtkGetMacro(UseExistingSearchStructure,int);
  vtkBooleanMacro(UseExistingSearchStructure,int);

  // Description:
  // Return intersection point (if any) of finite line with cells contained
  // in cell locator.
  virtual int IntersectWithLine(
    double p1[3], double p2[3], double tol, double& t, double x[3],
    double pcoords[3], int &subId);

  // Description:
  // Return intersection point (if any) AND the cell which was intersected by
  // the finite line.
  virtual int IntersectWithLine(
    double p1[3], double p2[3], double tol, double& t, double x[3],
    double pcoords[3], int &subId, vtkIdType &cellId);

  // Description:
  // Return intersection point (if any) AND the cell which was intersected by
  // the finite line. The cell is returned as a cell id and as a generic cell.
  virtual int IntersectWithLine(
    double p1[3], double p2[3], double tol, double& t, double x[3],
    double pcoords[3], int &subId, vtkIdType &cellId, vtkGenericCell *cell);

  // Description:
  // Take the passed line segment and intersect it with the data set.
  // This method assumes that the data set is a vtkPolyData that describes
  // a closed surface, and the intersection points that are returned in
  // 'points' alternate between entrance points and exit points.
  // The return value of the function is 0 if no intersections were found,
  // -1 if point 'a0' lies inside the closed surface, or +1 if point 'a0'
  // lies outside the closed surface.
  // Either 'points' or 'cellIds' can be set to NULL if you don't want
  // to receive that information. This method is currently only implemented
  // in vtkOBBTree
  virtual int IntersectWithLine(
    const double p1[3], const double p2[3],
    vtkPoints *points, vtkIdList *cellIds);

  // Description:
  // Return the closest point and the cell which is closest to the point x.
  // The closest point is somewhere on a cell, it need not be one of the
  // vertices of the cell.
  virtual void FindClosestPoint(
    double x[3], double closestPoint[3],
    vtkIdType &cellId, int &subId, double& dist2);

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
  virtual void FindClosestPoint(
    double x[3], double closestPoint[3],
    vtkGenericCell *cell, vtkIdType &cellId,
    int &subId, double& dist2);

  // Description:
  // Return the closest point within a specified radius and the cell which is
  // closest to the point x. The closest point is somewhere on a cell, it
  // need not be one of the vertices of the cell. This method returns 1 if
  // a point is found within the specified radius. If there are no cells within
  // the specified radius, the method returns 0 and the values of closestPoint,
  // cellId, subId, and dist2 are undefined.
  virtual vtkIdType FindClosestPointWithinRadius(
    double x[3], double radius,
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
  virtual vtkIdType FindClosestPointWithinRadius(
    double x[3], double radius,
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
  virtual vtkIdType FindClosestPointWithinRadius(
    double x[3], double radius,
    double closestPoint[3],
    vtkGenericCell *cell, vtkIdType &cellId,
    int &subId, double& dist2, int &inside);

  // Description:
  // Return a list of unique cell ids inside of a given bounding box. The
  // user must provide the vtkIdList to populate. This method returns data
  // only after the locator has been built.
  virtual void FindCellsWithinBounds(double *bbox, vtkIdList *cells);

  // Description:
  // Given a finite line defined by the two points (p1,p2), return the list
  // of unique cell ids in the buckets containing the line. It is possible
  // that an empty cell list is returned. The user must provide the vtkIdList
  // to populate. This method returns data only after the locator has been
  // built.
  virtual void FindCellsAlongLine(
    double p1[3], double p2[3], double tolerance, vtkIdList *cells);

  // Description:
  // Returns the Id of the cell containing the point,
  // returns -1 if no cell found. This interface uses a tolerance of zero
  virtual vtkIdType FindCell(double x[3]);

  // Description:
  // Find the cell containing a given point. returns -1 if no cell found
  // the cell parameters are copied into the supplied variables, a cell must
  // be provided to store the information.
  virtual vtkIdType FindCell(
    double x[3], double tol2, vtkGenericCell *GenCell,
    double pcoords[3], double *weights);

  // Description:
  // Quickly test if a point is inside the bounds of a particular cell.
  // Some locators cache cell bounds and this function can make use
  // of fast access to the data.
  virtual bool InsideCellBounds(double x[3], vtkIdType cell_ID);

protected:
   vtkAbstractCellLocator();
  ~vtkAbstractCellLocator();

  // Description:
  // This command is used internally by the locator to copy
  // all cell Bounds into the internal CellBounds array. Subsequent
  // calls to InsideCellBounds(...) can make use of the data
  // A valid dataset must be present for this to work. Returns true
  // if bounds wre copied, false otherwise.
  virtual bool StoreCellBounds();
  virtual void FreeCellBounds();

  int NumberOfCellsPerNode;
  int RetainCellLists;
  int CacheCellBounds;
  int LazyEvaluation;
  int UseExistingSearchStructure;
  vtkGenericCell *GenericCell;
//BTX - begin tcl exclude
  double (*CellBounds)[6];
//ETX - end tcl exclude

private:
  vtkAbstractCellLocator(const vtkAbstractCellLocator&);  // Not implemented.
  void operator=(const vtkAbstractCellLocator&);  // Not implemented.
};

#endif


