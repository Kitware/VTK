/*=========================================================================

  Program:   Visualization Library
  Module:    DS2DSF.hh
  Language:  C++
  Date:      2/17/94
  Version:   1.8

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// DataSetToDataSetFilter takes any dataset as input and copies it through, 
// changing the point attributes along the way.
//
#ifndef __vlDataSetToDataSetFilter_h
#define __vlDataSetToDataSetFilter_h

#include "DataSetF.hh"
#include "DataSet.hh"

class vlDataSetToDataSetFilter : public vlDataSet, public vlDataSetFilter
{
public:
  vlDataSetToDataSetFilter();
  ~vlDataSetToDataSetFilter();
  char *GetClassName() {return "vlDataSetToDataSetFilter";};
  char *GetDataType() {return this->DataSet->GetDataType();};
  void PrintSelf(ostream& os, vlIndent indent);

  // dataset interface
  vlDataSet *MakeObject() {return this->DataSet->MakeObject();};
  int GetNumberOfCells() {return this->DataSet->GetNumberOfCells();}
  int GetNumberOfPoints() {return this->DataSet->GetNumberOfPoints();}
  float *GetPoint(int i) {return this->DataSet->GetPoint(i);}
  vlCell *GetCell(int cellId) {return this->DataSet->GetCell(cellId);}
  int GetCellType(int cellId) {return this->DataSet->GetCellType(cellId);}
  void Initialize();
  void GetCellPoints(int cellId, vlIdList& ptIds)
    {this->DataSet->GetCellPoints(cellId, ptIds);};
  void GetPointCells(int ptId, vlIdList& cellIds)
    {this->DataSet->GetPointCells(ptId, cellIds);};
  int FindCell(float x[3], vlCell *cell, float tol2, int& subId, float pc[3]) 
    {return this->DataSet->FindCell(x,cell,tol2,subId,pc);};

  void ComputeBounds() {this->DataSet->ComputeBounds();};
  void Update();

protected:
  vlDataSet *DataSet;

};

#endif


