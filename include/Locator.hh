/*=========================================================================

  Program:   Visualization Library
  Module:    Locator.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Object used to locate close points
//
#ifndef __vlLocator_h
#define __vlLocator_h

#include "Object.hh"
#include "Points.hh"
#include "IdList.hh"

class vlLocator : public vlObject
{
public:
  vlLocator();
  ~vlLocator();
  char *GetClassName() {return "vlLocator";};
  void Initialize();
  void FreeHashTable();

  vlSetObjectMacro(Points,vlPoints);
  vlGetObjectMacro(Points,vlPoints);

  vlSetVector3Macro(Divisions,int);
  vlGetVectorMacro(Divisions,int);

  vlSetMacro(Automatic,int);
  vlGetMacro(Automatic,int);
  vlBooleanMacro(Automatic,int);

  vlSetClampMacro(NumberOfPointsInCell,int,1,LARGE_INTEGER);
  vlGetMacro(NumberOfPointsInCell,int);

  vlSetClampMacro(Tolerance,float,0.0,LARGE_FLOAT);
  vlGetMacro(Tolerance,float);

  // return closest point id to coordinate specified
  int FindClosestPoint(float x[3]);
  // return index of merged points
  void MergePoints(int *index); // dimensioned Points->NumberOfPoints long

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
  
};

#endif


