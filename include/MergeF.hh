/*=========================================================================

  Program:   Visualization Library
  Module:    MergeF.hh
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
// Extracts cells where scalar value of every point in cell satisfies 
// threshold criterion. 
//
#ifndef __vlMergeFilter_h
#define __vlMergeFilter_h

#include "DS2UGrid.hh"

class vlMergeFilter : public vlDataSet, public vlFilter
{
public:
  vlMergeFilter();
  ~vlMergeFilter();
  char *GetClassName() {return "vlMergeFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  // dataset interface
  vlDataSet *MakeObject() {return this->Geometry->MakeObject();};
  int GetNumberOfCells() {return this->Geometry->GetNumberOfCells();}
  int GetNumberOfPoints() {return this->Geometry->GetNumberOfPoints();}
  float *GetPoint(int i) {return this->Geometry->GetPoint(i);}
  vlCell *GetCell(int cellId) {return this->Geometry->GetCell(cellId);}
  int GetCellType(int cellId) {return this->Geometry->GetCellType(cellId);}
  void Initialize();
  void GetCellPoints(int cellId, vlIdList *ptIds)
    {this->Geometry->GetCellPoints(cellId, ptIds);};
  void GetPointCells(int ptId, vlIdList *cellIds)
    {this->Geometry->GetPointCells(ptId, cellIds);};
  int FindCell(float x[3], vlCell *cell, float tol2, int& subId, float pc[3]) 
    {return this->Geometry->FindCell(x,cell,tol2,subId,pc);};

  void ComputeBounds() {this->Geometry->ComputeBounds();};
  vlMapper *MakeMapper();

  // Filter interface
  void Update();

  // Components to merge
  vlSetObjectMacro(Geometry,vlDataSet);
  vlGetObjectMacro(Geometry,vlDataSet);
  
  vlSetObjectMacro(Scalars,vlDataSet);
  vlGetObjectMacro(Scalars,vlDataSet);
  
  vlSetObjectMacro(Vectors,vlDataSet);
  vlGetObjectMacro(Vectors,vlDataSet);
  
  vlSetObjectMacro(Normals,vlDataSet);
  vlGetObjectMacro(Normals,vlDataSet);
  
  vlSetObjectMacro(TCoords,vlDataSet);
  vlGetObjectMacro(TCoords,vlDataSet);

protected:
  // Usual data generation method
  void Execute();

  vlDataSet *Geometry; // output geometry
  vlDataSet *Scalars;  // scalars to merge
  vlDataSet *Vectors;  // vectors
  vlDataSet *Normals;  // normals
  vlDataSet *TCoords;  // texture coords
};

#endif


