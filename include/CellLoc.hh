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
// .NAME vlCellLocator - octree-based spatial search object to quickly locate cells
// .SECTION Description
// vlCellLocator is a spatial search object to quickly locate cells in 3-D.
// vlCellLocator uses a uniform-level octree subdivision, where each octant
// carries an indication of whether it is empty or not, a list of the
// cells inside of it. (An octant is not empty if it has one or more cells
// inside of it). Typical operation are intersection with a line to return
// candidate cells, or intersection with another vlCellLocator to return 
// candidate cells.
// .SECTION Caveats
// Many other types of spatial locators have been developed such as 
// variable depth-octrees and k-d trees. These are often more efficient 
// for the operations described here.

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
  // Set the level of the octree (set automatically if Automatic is true).
  vlSetClampMacro(Level,int,1,this->MaxLevel);
  vlGetMacro(Level,int);

  // Description:
  // Set the maximum allowable level for the octree.
  vlSetClampMacro(MaxLevel,int,1,5);
  vlGetMacro(MaxLevel,int);

  // Description:
  // Boolean controls whether automatic subdivision size is computed
  // from average number of cells in octant.
  vlSetMacro(Automatic,int);
  vlGetMacro(Automatic,int);
  vlBooleanMacro(Automatic,int);

  // Description:
  // Specify the average number of cells in each octant.
  vlSetClampMacro(NumberOfCellsInOctant,int,1,LARGE_INTEGER);
  vlGetMacro(NumberOfCellsInOctant,int);

  // Description:
  // Specify absolute tolerance (in world coordinates) for performing
  // intersection computations.
  vlSetClampMacro(Tolerance,float,0.0,LARGE_FLOAT);
  vlGetMacro(Tolerance,float);

  virtual int FindClosestCell(float x[3], float dist2, int& subId, float pcoords[3]);
  virtual int IntersectWithLine(float a0[3], float a1[3], vlIdList& cells);
  vlIdList *GetOctantCells(int octantId);
  virtual int IntersectWithCellLocator(vlCellLocator& locator, vlIdList cells);

protected:
  // place points in appropriate cells
  void SubDivide();

  vlDataSet *DataSet; // Dataset of cells to insert
  int MaxLevel; // Maximum tree level
  int Level; // Tree level
  int NumberOfOctants; // number of octants in tree
  int Automatic; // boolean controls automatic subdivision (or uses user spec.)
  int NumberOfCellsInOctant; // Used with previous boolean to control subdivide
  float Tolerance; // for performing intersection
  float Bounds[6]; // bounding box root octant
  int NumberOfParents; // number of parent octants
  float H[3]; // width of root octant in x-y-z directions
  int NumberOfDivisions; // number of "leaf" octant sub-divisions
  vlIdList **Tree; // octree
  vlTimeStamp SubDivideTime;  

  void MarkParents(void*, int i, int j, int k);
  void GetChildren(int idx, int level, int children[8]);
};

#endif


