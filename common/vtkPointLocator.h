/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointLocator.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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
// method, you suppy it with a dataset, and it operates on the points in 
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
#include "vtkIdList.h"

class VTK_EXPORT vtkPointLocator : public vtkLocator
{
public:
  vtkPointLocator();
  static vtkPointLocator *New() {return new vtkPointLocator;};
  char *GetClassName() {return "vtkPointLocator";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of divisions in x-y-z directions.
  vtkSetVector3Macro(Divisions,int);
  vtkGetVectorMacro(Divisions,int,3);

  // Description:
  // Specify the average number of points in each bucket.
  vtkSetClampMacro(NumberOfPointsPerBucket,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfPointsPerBucket,int);

  // these operate with specified dataset
  virtual int FindClosestPoint(float x[3]);
  virtual int *MergePoints();

  // these all operate on array of points from InitPointInsertion()
  virtual int InitPointInsertion(vtkPoints *newPts, float bounds[6]);
  virtual void InsertPoint(int ptId, float x[3]);
  virtual int InsertNextPoint(float x[3]);
  virtual int IsInsertedPoint(float x[3]);
  virtual int FindClosestInsertedPoint(float x[3]);
  
  // satisfy vtkLocator abstract interface
  void Initialize();
  void FreeSearchStructure();
  void BuildLocator();
  void GenerateRepresentation(int level, vtkPolyData *pd);

protected:
  // place points in appropriate buckets
  void GetBucketNeighbors(int ijk[3], int ndivs[3], int level);
  void GetOverlappingBuckets(float x[3], int ijk[3], float dist);
  void GenerateFace(int face, int i, int j, int k, 
                    vtkFloatPoints *pts, vtkCellArray *polys);

  vtkPoints *Points; // Used for merging points
  int Divisions[3]; // Number of sub-divisions in x-y-z directions
  int NumberOfPointsPerBucket; //Used with previous boolean to control subdivide
  float Bounds[6]; // bounds of points
  vtkIdList **HashTable; // lists of point ids in buckets
  int NumberOfBuckets; // total size of hash table
  float H[3]; // width of each bucket in x-y-z directions

  float InsertionTol2;
  int InsertionPointId;
};

#endif


