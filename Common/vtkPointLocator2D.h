/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointLocator2D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
#include "vtkPoints.h"
#include "vtkIdList.h"

class vtkNeighborPoints2D;

class VTK_COMMON_EXPORT vtkPointLocator2D : public vtkLocator
{
public:
  // Description:
  // Construct with automatic computation of divisions, averaging
  // 25 points per bucket.
  static vtkPointLocator2D *New();

  vtkTypeMacro(vtkPointLocator2D,vtkLocator);
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
  vtkSetObjectMacro(Points,vtkPoints);
  vtkGetObjectMacro(Points,vtkPoints);
  
protected:
  vtkPointLocator2D();
  ~vtkPointLocator2D();
  vtkPointLocator2D(const vtkPointLocator2D&);
  void operator=(const vtkPointLocator2D&);

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
};

#endif


