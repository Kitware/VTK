/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Locator.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

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

#include "Object.hh"
#include "Points.hh"
#include "IdList.hh"

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
  vtkSetClampMacro(NumberOfPointsInBucket,int,1,LARGE_INTEGER);
  vtkGetMacro(NumberOfPointsInBucket,int);

  // Description:
  // Specify absolute tolerance (in world coordinates) for performing
  // merge operations.
  vtkSetClampMacro(Tolerance,float,0.0,LARGE_FLOAT);
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


