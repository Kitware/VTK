/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointLocator2D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPointLocator2D - quickly locate points in 2-space
// .SECTION Description
// vtkPointLocator2D is a spatial search object to quickly locate points in 2D.
// vtkPointLocator2D works by dividing a specified region of space into a regular
// array of "rectangular" buckets, and then keeping a list of points that 
// lie in each bucket. Typical operation involves giving a position in 2D 
// and finding the closest point.
//
// vtkPointLocator2D has two distinct methods of interaction. In the first
// method, you supply it with a dataset, and it operates on the points in 
// the dataset. In the second method, you supply it with an array of points,
// and the object operates on the array.
//
// This class is similar to vtkPointLocator except that it assumes the points
// are located in 2D (or at least that the z-coordinate is ignored).

// .SECTION Caveats
// Many other types of spatial locators have been developed such as 
// octrees and kd-trees. These are often more efficient for the 
// operations described here.

// .SECTION See Also
// vtkPointLocator vtkCellPicker vtkPointPicker

#ifndef __vtkPointLocator2D_h
#define __vtkPointLocator2D_h

#include "vtkLocator.h"

class vtkPoints;
class vtkIdList;

class vtkNeighborPoints2D;

class VTK_COMMON_EXPORT vtkPointLocator2D : public vtkLocator
{
public:
  // Description:
  // Construct with automatic computation of divisions, averaging
  // 25 points per bucket.
  static vtkPointLocator2D *New();

  vtkTypeRevisionMacro(vtkPointLocator2D,vtkLocator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of divisions in x-y directions.
  vtkSetVector2Macro(Divisions,int);
  vtkGetVectorMacro(Divisions,int,2);

  // Description:
  // Specify the average number of points in each bucket.
  vtkSetClampMacro(NumberOfPointsPerBucket,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfPointsPerBucket,int);

  // Description:
  // Given a position x, return the id of the point closest to it.
  virtual int FindClosestPoint(float x[2]);

  // Description:
  // Determine whether point given by x[2] has been inserted into points list.
  // Return id of previously inserted point if this is true, otherwise return
  // -1.
  virtual int IsInsertedPoint(float x[2]);

  // Description:
  // Find the closest N points to a position. This returns the closest
  // N points to a position. A faster method could be created that returned
  // N close points to a position, but necessarily the exact N closest.
  // The returned points are sorted from closest to farthest.
  virtual void FindClosestNPoints(int N, float x[2], vtkIdList *result);
  virtual void FindClosestNPoints(int N, float x, float y,
                                  vtkIdList *result);

  // Description:
  // Find the closest points to a position such that each quadrant of
  // space around the position contains at least N points. Loosely 
  // limit the search to a maximum number of points evaluated, M. 
  virtual void FindDistributedPoints(int N, float x[2], 
                                     vtkIdList *result, int M);
  virtual void FindDistributedPoints(int N, float x, float y, 
                                     vtkIdList *result, int M);

  // Description:
  // Find all points within a specified radius R of position x.
  // The result is not sorted in any specific manner.
  virtual void FindPointsWithinRadius(float R, float x[2], vtkIdList *result);
  virtual void FindPointsWithinRadius(float R, float x, float y,
                                      vtkIdList *result);
  // Description:
  // See vtkLocator interface documentation.
  void Initialize();
  void FreeSearchStructure();
  void BuildLocator();
  void GenerateRepresentation(int level, vtkPolyData *pd);

  // Description:
  // set the points to use when looking up a coordinate
  virtual void SetPoints(vtkPoints*);
  vtkGetObjectMacro(Points,vtkPoints);
  
protected:
  vtkPointLocator2D();
  ~vtkPointLocator2D();

  // place points in appropriate buckets
  void GetBucketNeighbors(int ijk[2], int ndivs[2], int level);
  void GetOverlappingBuckets(float x[2], int ijk[2], float dist, int level);
  void GenerateFace(int face, int i, int j, int k, 
                    vtkPoints *pts, vtkCellArray *polys);

  vtkPoints *Points; // Used for merging points
  int Divisions[2]; // Number of sub-divisions in x-y-z directions
  int NumberOfPointsPerBucket; //Used with previous boolean to control subdivide
  float Bounds[4]; // bounds of points
  vtkIdList **HashTable; // lists of point ids in buckets
  int NumberOfBuckets; // total size of hash table
  float H[2]; // width of each bucket in x-y-z directions
  vtkNeighborPoints2D *Buckets;
  float InsertionTol2;
private:
  vtkPointLocator2D(const vtkPointLocator2D&);  // Not implemented.
  void operator=(const vtkPointLocator2D&);  // Not implemented.
};

#endif


