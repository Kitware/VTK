/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointLocator.h
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
// .NAME vtkPointLocator - quickly locate points in 3-space
// .SECTION Description
// vtkPointLocator is a spatial search object to quickly locate points in 3D.
// vtkPointLocator works by dividing a specified region of space into a regular
// array of "rectangular" buckets, and then keeping a list of points that 
// lie in each bucket. Typical operation involves giving a position in 3D 
// and finding the closest point.
//
// vtkPointLocator has two distinct methods of interaction. In the first
// method, you supply it with a dataset, and it operates on the points in 
// the dataset. In the second method, you supply it with an array of points,
// and the object operates on the array.

// .SECTION Caveats
// Many other types of spatial locators have been developed such as 
// octrees and kd-trees. These are often more efficient for the 
// operations described here.

// .SECTION See Also
// vtkCellPicker vtkPointPicker

#ifndef __vtkPointLocator_h
#define __vtkPointLocator_h

#include "vtkLocator.h"
#include "vtkPoints.h"

class vtkIdList;
class vtkNeighborPoints;

class VTK_EXPORT vtkPointLocator : public vtkLocator
{
public:
  // Description:
  // Construct with automatic computation of divisions, averaging
  // 25 points per bucket.
  static vtkPointLocator *New();

  vtkTypeMacro(vtkPointLocator,vtkLocator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of divisions in x-y-z directions.
  vtkSetVector3Macro(Divisions,int);
  vtkGetVectorMacro(Divisions,int,3);

  // Description:
  // Specify the average number of points in each bucket.
  vtkSetClampMacro(NumberOfPointsPerBucket,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfPointsPerBucket,int);

  // Description:
  // Given a position x, return the id of the point closest to it. Alternative
  // method requires separate x-y-z values.
  // These methods are thread safe if BuildLocator() is directly or
  // indirectly called from a single thread first.
  virtual vtkIdType FindClosestPoint(const float x[3]);
  vtkIdType FindClosestPoint(float x, float y, float z);

  // Description:
  // Given a position x and a radius r, return the id of the point 
  // closest to the point in that radius.
  // These methods are thread safe if BuildLocator() is directly or
  // indirectly called from a single thread first.
  vtkIdType FindClosestPointWithinRadius(float radius, const float x[3],
                                         float& dist2);
  vtkIdType FindClosestPointWithinRadius(float radius, const float x[3], 
                                         float inputDataLength, float& dist2);

  // Description:
  // Initialize the point insertion process. The newPts is an object
  // representing point coordinates into which incremental insertion methods
  // place their data. Bounds are the box that the points lie in.
  // Not thread safe.
  virtual int InitPointInsertion(vtkPoints *newPts, const float bounds[6]);

  // Description:
  // Initialize the point insertion process. The newPts is an object
  // representing point coordinates into which incremental insertion methods
  // place their data. Bounds are the box that the points lie in.
  // Not thread safe.
  virtual int InitPointInsertion(vtkPoints *newPts, const float bounds[6], 
				 int estSize);

  // Description:
  // Incrementally insert a point into search structure with a particular
  // index value. You should use the method IsInsertedPoint() to see whether 
  // this point has already been inserted (that is, if you desire to prevent
  // duplicate points). Before using this method you must make sure that 
  // newPts have been supplied, the bounds has been set properly, and that 
  // divs are properly set. (See InitPointInsertion().)
  // Not thread safe.
  virtual void InsertPoint(vtkIdType ptId, const float x[3]);

  // Description:
  // Incrementally insert a point into search structure. The method returns
  // the insertion location (i.e., point id). You should use the method 
  // IsInsertedPoint() to see whether this point has already been
  // inserted (that is, if you desire to prevent duplicate points).
  // Before using this method you must make sure that newPts have been
  // supplied, the bounds has been set properly, and that divs are 
  // properly set. (See InitPointInsertion().)
  // Not thread safe.
  virtual vtkIdType InsertNextPoint(const float x[3]);

  // Description:
  // Determine whether point given by x[3] has been inserted into points list.
  // Return id of previously inserted point if this is true, otherwise return
  // -1. This method is thread safe.
  vtkIdType IsInsertedPoint(float x, float  y, float z)
    {
    float xyz[3];
    xyz[0] = x; xyz[1] = y; xyz[2] = z;
    return this->IsInsertedPoint (xyz);
    };
  virtual vtkIdType IsInsertedPoint(const float x[3]);

  // Description:
  // Determine whether point given by x[3] has been inserted into points list.
  // Return 0 if point was already in the list, otherwise return 1. If the
  // point was not in the list, it will be ADDED.  In either case, the id of
  // the point (newly inserted or not) is returned in the ptId argument.
  // Note this combines the functionality of IsInsertedPoint() followed
  // by a call to InsertNextPoint().
  // This method is not thread safe.
  virtual int InsertUniquePoint(const float x[3], vtkIdType &ptId);

  // Description:
  // Given a position x, return the id of the point closest to it. This method
  // is used when performing incremental point insertion. Note that -1 
  // indicates that no point was found.
  // This method is thread safe if  BuildLocator() is directly or
  // indirectly called from a single thread first.
  virtual vtkIdType FindClosestInsertedPoint(const float x[3]);

  // Description:
  // Find the closest N points to a position. This returns the closest
  // N points to a position. A faster method could be created that returned
  // N close points to a position, but necessarily the exact N closest.
  // The returned points are sorted from closest to farthest.
  // These methods are thread safe if BuildLocator() is directly or
  // indirectly called from a single thread first.
  virtual void FindClosestNPoints(int N, const float x[3], vtkIdList *result);
  virtual void FindClosestNPoints(int N, float x, float y, float z,
				  vtkIdList *result);

  // Description:
  // Find the closest points to a position such that each octant of
  // space around the position contains at least N points. Loosely 
  // limit the search to a maximum number of points evaluated, M. 
  // These methods are thread safe if BuildLocator() is directly or
  // indirectly called from a single thread first.
  virtual void FindDistributedPoints(int N, const float x[3], 
				     vtkIdList *result, int M);
  virtual void FindDistributedPoints(int N, float x, float y, 
				     float z, vtkIdList *result, int M);

  // Description:
  // Find all points within a specified radius R of position x.
  // The result is not sorted in any specific manner.
  // These methods are thread safe if BuildLocator() is directly or
  // indirectly called from a single thread first.
  virtual void FindPointsWithinRadius(float R, const float x[3],
				      vtkIdList *result);
  virtual void FindPointsWithinRadius(float R, float x, float y, float z, 
				      vtkIdList *result);
  
  // Description:
  // Given a position x, return the list of points in the bucket that
  // contains the point. It is possible that NULL is returned. The user
  // provides an ijk array that is the indices into the locator.
  // This method is thread safe.
  virtual vtkIdList *GetPointsInBucket(const float x[3], int ijk[3]);

  // Description:
  // See vtkLocator interface documentation.
  // These methods are not thread safe.
  void Initialize();
  void FreeSearchStructure();
  void BuildLocator();
  void GenerateRepresentation(int level, vtkPolyData *pd);

protected:
  vtkPointLocator();
  ~vtkPointLocator();
  vtkPointLocator(const vtkPointLocator&) {};
  void operator=(const vtkPointLocator&) {};

  // place points in appropriate buckets
  void GetBucketNeighbors(vtkNeighborPoints* buckets,
			  const int ijk[3], const int ndivs[3], int level);
  void GetOverlappingBuckets(vtkNeighborPoints* buckets, 
			     const float x[3], const int ijk[3], float dist,
			     int level);
  void GetOverlappingBuckets(vtkNeighborPoints* buckets,
			     const float x[3], float dist,
			     int prevMinLevel[3],
			     int prevMaxLevel[3]);
  void GenerateFace(int face, int i, int j, int k, 
                    vtkPoints *pts, vtkCellArray *polys);
  float Distance2ToBucket(const float x[3], const int nei[3]);
  float Distance2ToBounds(const float x[3], const float bounds[6]);

  vtkPoints *Points; // Used for merging points
  int Divisions[3]; // Number of sub-divisions in x-y-z directions
  int NumberOfPointsPerBucket; //Used with previous boolean to control subdivide
  float Bounds[6]; // bounds of points
  vtkIdList **HashTable; // lists of point ids in buckets
  vtkIdType NumberOfBuckets; // total size of hash table
  float H[3]; // width of each bucket in x-y-z directions

  float InsertionTol2;
  vtkIdType InsertionPointId;

  float InsertionLevel; 
};

#endif


