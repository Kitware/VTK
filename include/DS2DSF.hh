/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DS2DSF.hh
  Language:  C++
  Date:      2/17/94
  Version:   1.8

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkDataSetToDataSetFilter - abstract filter class
// .SECTION Description
// vtkDataSetToDataSetFilter is an abstract filter class. Subclasses of 
// vtkDataSetToDataSetFilter take a dataset as input and create a dataset 
// as output. The form of the input geometry is not changed in these 
// filters, only the point attributes (e,g,, scalars, vectors, etc.).

#ifndef __vtkDataSetToDataSetFilter_h
#define __vtkDataSetToDataSetFilter_h

#include "DataSetF.hh"
#include "DataSet.hh"

class vtkDataSetToDataSetFilter : public vtkDataSet, public vtkDataSetFilter
{
public:
  vtkDataSetToDataSetFilter();
  ~vtkDataSetToDataSetFilter();
  char *GetClassName() {return "vtkDataSetToDataSetFilter";};
  char *GetDataType() {return this->DataSet->GetDataType();};
  void PrintSelf(ostream& os, vtkIndent indent);

  // dataset interface
  vtkDataSet *MakeObject() {return this->DataSet->MakeObject();};
  int GetNumberOfCells() {return this->DataSet->GetNumberOfCells();}
  int GetNumberOfPoints() {return this->DataSet->GetNumberOfPoints();}
  float *GetPoint(int i) {return this->DataSet->GetPoint(i);}
  void GetPoint(int i, float p[3]) {this->DataSet->GetPoint(i,p);}
  vtkCell *GetCell(int cellId) {return this->DataSet->GetCell(cellId);}
  int GetCellType(int cellId) {return this->DataSet->GetCellType(cellId);}
  void Initialize();
  void GetCellPoints(int cellId, vtkIdList& ptIds) {this->DataSet->GetCellPoints(cellId, ptIds);};
  void GetPointCells(int ptId, vtkIdList& cellIds) {this->DataSet->GetPointCells(ptId, cellIds);};
  int FindCell(float x[3], vtkCell *cell, float tol2, int& subId, 
               float pc[3], float weights[MAX_CELL_SIZE])
    {return this->DataSet->FindCell(x,cell,tol2,subId,pc,weights);};
  void ComputeBounds();

  // Object interface
  void Modified();
  unsigned long int GetMTime();
  unsigned long int _GetMTime() {return this->GetMTime();};
  void DebugOn();
  void DebugOff();

  //DataSet interface
  void Update();

protected:
  vtkDataSet *DataSet;

  //Filter interface
  int GetDataReleased();
  void SetDataReleased(int flag);
};



#endif


