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
  void PrintSelf(ostream& os, vlIndent indent);

  // dataset interface
  vlDataSet *MakeObject() {return this->DataSet->MakeObject();};
  int GetNumberOfCells() {return this->DataSet->GetNumberOfCells();}
  int GetNumberOfPoints() {return this->DataSet->GetNumberOfPoints();}
  float *GetPoint(int i) {return this->DataSet->GetPoint(i);}
  vlCell *GetCell(int cellId) {return this->DataSet->GetCell(cellId);}
  void Initialize();

  void ComputeBounds() {this->DataSet->ComputeBounds();};
  vlMapper *MakeMapper();
  void Update();

protected:
  vlDataSet *DataSet;

};

#endif


