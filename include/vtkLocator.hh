/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLocator.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkLocator - spatial search object to quickly locate points
// .SECTION Description
// vtkLocator is a spatial search object to quickly locate points in 3-D.
// vtkLocator works by dividing a specified region of space into a regular
// array of "rectangular" buckets, and then keeping a list of points that 
// lie in each bucket. Typical operation involves giving a position in 3-D 
// and finding the closest point.
// .SECTION Caveats
// Many other types of spatial locators have been developed such as 
// octrees and k-d trees. These are often more efficient for the 
// operations described here.

#ifndef __vtkLocator_h
#define __vtkLocator_h

#include "vtkObject.hh"
#include "vtkPoints.hh"
#include "vtkIdList.hh"

class vtkLocator : public vtkObject
{
public:
  vtkLocator();
  virtual ~vtkLocator();
  char *GetClassName() {return "vtkLocator";};
  void Initialize();
  virtual void FreeSearchStructure();

  // Description:
  // Set list of points to insert into locator.
  vtkSetRefCountedObjectMacro(Points,vtkPoints);
  vtkGetObjectMacro(Points,vtkPoints);

  // Description:
  // Set the number of divisions in x-y-z directions.
  vtkSetVector3Macro(Divisions,int);
  vtkGetVectorMacro(Divisions,int,3);

  // Description:
  // Boolean controls whether automatic subdivision size is computed
  // from average number of points in bucket.
  vtkSetMacro(Automatic,int);
  vtkGetMacro(Automatic,int);
  vtkBooleanMacro(Automatic,int);

  // Description:
  // Specify the average number of points in each bucket.
  vtkSetClampMacro(NumberOfPointsInBucket,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfPointsInBucket,int);

  // Description:
  // Specify absolute tolerance (in world coordinates) for performing
  // merge operations.
  vtkSetClampMacro(Tolerance,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Tolerance,float);

  virtual int FindClosestPoint(float x[3]);
  virtual int *MergePoints();
  virtual int InitPointInsertion(vtkPoints *newPts, float bounds[6]);
  virtual int InsertPoint(float x[3]);

protected:
  // place points in appropriate buckets
  void SubDivide();
  void GetBucketNeighbors(int ijk[3], int ndivs[3], int level);

  vtkPoints *Points;
  int Divisions[3]; // Number of sub-divisions in x-y-z directions
  int Automatic; // boolean controls automatic subdivision (or uses user spec.)
  int NumberOfPointsInBucket; //Used with previous boolean to control subdivide
  float Tolerance; // for performing merging
  float Bounds[6]; // bounds of points
  vtkIdList **HashTable; // lists of point ids in buckets
  int NumberOfBuckets; // total size of hash table
  float H[3]; // width of each bucket in x-y-z directions
  vtkTimeStamp SubDivideTime;  

  float InsertionTol2;
  int InsertionPointId;
};

#endif


