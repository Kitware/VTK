/*=========================================================================

  Program:   Visualization Library
  Module:    Locator.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlLocator - spatial search object to quickly locate points
// .SECTION Description
// vlLocator is a spatial search object to quickly locate points in 3-D.
// vlLocator works by dividing a specified region of space into a regular
// array of cells, and then keeping a list of points that exist in each 
// cell. Typical operation involves giving a position in 3-D and finding
// the closest point.
// .SECTION Caveats
// Many other types of spatial locators have been developed such as 
// octrees and k-d trees. These are often more efficient for the 
// operations described here.

#ifndef __vlLocator_h
#define __vlLocator_h

#include "Object.hh"
#include "Points.hh"
#include "IdList.hh"

class vlLocator : public vlObject
{
public:
  vlLocator();
  virtual ~vlLocator();
  char *GetClassName() {return "vlLocator";};
  void Initialize();
  virtual void FreeSearchStructure();

  // Description:
  // Set list of points to insert into locator.
  vlSetRefCountedObjectMacro(Points,vlPoints);
  vlGetObjectMacro(Points,vlPoints);

  // Description:
  // Set the number of divisions in x-y-z directions.
  vlSetVector3Macro(Divisions,int);
  vlGetVectorMacro(Divisions,int,3);

  // Description:
  // Boolean controls whether automatic subdivision size is computed
  // from average number of points in cell.
  vlSetMacro(Automatic,int);
  vlGetMacro(Automatic,int);
  vlBooleanMacro(Automatic,int);

  // Description:
  // Specify the average number of points in each "cell".
  vlSetClampMacro(NumberOfPointsInCell,int,1,LARGE_INTEGER);
  vlGetMacro(NumberOfPointsInCell,int);

  // Description:
  // Specify absolute tolerance (in world coordinates) for performing
  // merge operations.
  vlSetClampMacro(Tolerance,float,0.0,LARGE_FLOAT);
  vlGetMacro(Tolerance,float);

  virtual int FindClosestPoint(float x[3]);
  virtual int *MergePoints();
  virtual int InitPointInsertion(vlPoints *newPts, float bounds[6]);
  virtual int InsertPoint(float x[3]);

protected:
  // place points in appropriate cells
  void SubDivide();
  void GetCellNeighbors(int ijk[3], int ndivs[3], int level);

  vlPoints *Points;
  int Divisions[3]; // Number of sub-divisions in x-y-z directions
  int Automatic; // boolean controls automatic subdivision (or uses user spec.)
  int NumberOfPointsInCell; // Used with previous boolean to control subdivide
  float Tolerance; // for performing merging
  float Bounds[6]; // bounds of points
  vlIdList **HashTable; // lists of point ids in cells
  int NumberOfCells; // total size of hash table
  float H[3]; // width of each cell in x-y-z directions
  vlTimeStamp SubDivideTime;  

  float InsertionTol2;
  int InsertionPointId;
};

#endif


