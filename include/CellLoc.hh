/*=========================================================================

  Program:   Visualization Library
  Module:    CellLoc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlCellLocator - spatial search object to quickly locate cells
// .SECTION Description
// vlCellLocator is a spatial search object to quickly locate cells in 3-D.
// vlCellLocator works by dividing a specified region of space into a regular
// array of "rectangular" buckets, and then keeping a list of cells whose 
// bounding box passes through each bucket. Typical operation are intersection
// with a line to return candidate cells, or intersection with another 
// vlCellLocator to return candidate cells.
// .SECTION Caveats
// Many other types of spatial locators have been developed such as 
// octrees and k-d trees. These are often more efficient for the 
// operations described here.

#ifndef __vlCellLocator_h
#define __vlCellLocator_h

#include "Object.hh"
#include "Points.hh"
#include "IdList.hh"
#include "DataSet.hh"

class vlCellLocator : public vlObject
{
public:
  vlCellLocator();
  virtual ~vlCellLocator();
  char *GetClassName() {return "vlCellLocator";};
  void Initialize();
  virtual void FreeSearchStructure();

  // Description:
  // Set list of cells to insert into locator.
  vlSetObjectMacro(DataSet,vlDataSet);
  vlGetObjectMacro(DataSet,vlDataSet);

  // Description:
  // Set the number of divisions in x-y-z directions.
  vlSetVector3Macro(Divisions,int);
  vlGetVectorMacro(Divisions,int,3);

  // Description:
  // Boolean controls whether automatic subdivision size is computed
  // from average number of cells in bucket.
  vlSetMacro(Automatic,int);
  vlGetMacro(Automatic,int);
  vlBooleanMacro(Automatic,int);

  // Description:
  // Specify the average number of cells in each bucket.
  vlSetClampMacro(NumberOfCellsInBucket,int,1,LARGE_INTEGER);
  vlGetMacro(NumberOfCellsInBucket,int);

  // Description:
  // Specify absolute tolerance (in world coordinates) for performing
  // intersection computations.
  vlSetClampMacro(Tolerance,float,0.0,LARGE_FLOAT);
  vlGetMacro(Tolerance,float);

  virtual int FindClosestCell(float x[3], float dist2, int& subId, float pcoords[3]);
  virtual int IntersectWithLine(float a0[3], float a1[3], vlIdList& cells);
  virtual int IntersectWithCellLocator(vlCellLocator& locator, vlIdList cells);

protected:
  // place points in appropriate cells
  void SubDivide();
  void GetCellNeighbors(int ijk[3], int ndivs[3], int level);

  vlDataSet *DataSet; // Dataset of cells to insert
  int Divisions[3]; // Number of sub-divisions in x-y-z directions
  int Automatic; // boolean controls automatic subdivision (or uses user spec.)
  int NumberOfCellsInBucket; // Used with previous boolean to control subdivide
  float Tolerance; // for performing intersection
  float Bounds[6]; // bounding box of cells
  vlIdList **HashTable; // lists of cell ids in buckets
  int NumberOfCells; // total size of hash table
  float H[3]; // width of each bucket in x-y-z directions
  vlTimeStamp SubDivideTime;  

  float InsertionTol2;
};

#endif


