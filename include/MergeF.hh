/*=========================================================================

  Program:   Visualization Toolkit
  Module:    MergeF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkMergeFilter - extract separate components of data from different datasets
// .SECTION Description
// vtkMergeFilter is a filter that extracts separate components of data from
// different datasets and merges them into a single dataset.

#ifndef __vtkMergeFilter_h
#define __vtkMergeFilter_h

#include "DataSet.hh"
#include "Filter.hh"

class vtkMergeFilter : public vtkDataSet, public vtkFilter
{
public:
  vtkMergeFilter();
  ~vtkMergeFilter();
  char *GetClassName() {return "vtkMergeFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // dataset interface
  char *GetDataType() {return this->Geometry->GetDataType();};
  vtkDataSet *MakeObject() {return this->Geometry->MakeObject();};
  int GetNumberOfCells() {return this->Geometry->GetNumberOfCells();};
  int GetNumberOfPoints() {return this->Geometry->GetNumberOfPoints();};
  float *GetPoint(int i) {return this->Geometry->GetPoint(i);};
  void GetPoint(int i, float x[3]) {this->Geometry->GetPoint(i,x);};
  vtkCell *GetCell(int cellId) {return this->Geometry->GetCell(cellId);};
  int GetCellType(int cellId) {return this->Geometry->GetCellType(cellId);};
  void Initialize();
  void GetCellPoints(int cellId, vtkIdList& ptIds)
    {this->Geometry->GetCellPoints(cellId, ptIds);};
  void GetPointCells(int ptId, vtkIdList& cellIds)
    {this->Geometry->GetPointCells(ptId, cellIds);};
  int FindCell(float x[3], vtkCell *cell, float tol2, int& subId, float pc[3], float weights[MAX_CELL_SIZE]) 
    {return this->Geometry->FindCell(x,cell,tol2,subId,pc,weights);};

  void ComputeBounds() {this->Geometry->ComputeBounds();};

  // Filter interface
  void Update();

  // Description:
  // Specify object from which to extract geometry information.
  vtkSetObjectMacro(Geometry,vtkDataSet);
  vtkGetObjectMacro(Geometry,vtkDataSet);
  
  // Description:
  // Specify object from which to extract scalar information.
  vtkSetObjectMacro(Scalars,vtkDataSet);
  vtkGetObjectMacro(Scalars,vtkDataSet);
  
  // Description:
  // Specify object from which to extract vector information.
  vtkSetObjectMacro(Vectors,vtkDataSet);
  vtkGetObjectMacro(Vectors,vtkDataSet);
  
  // Description:
  // Specify object from which to extract normal information.
  vtkSetObjectMacro(Normals,vtkDataSet);
  vtkGetObjectMacro(Normals,vtkDataSet);
  
  // Description:
  // Specify object from which to extract texture coordinates information.
  vtkSetObjectMacro(TCoords,vtkDataSet);
  vtkGetObjectMacro(TCoords,vtkDataSet);

  // Description:
  // Specify object from which to extract tensor data.
  vtkSetObjectMacro(Tensors,vtkDataSet);
  vtkGetObjectMacro(Tensors,vtkDataSet);

  // Description:
  // Specify object from which to extract user defined data.
  vtkSetObjectMacro(UserDefined,vtkDataSet);
  vtkGetObjectMacro(UserDefined,vtkDataSet);

protected:
  // Usual data generation method
  void Execute();

  vtkDataSet *Geometry; // output geometry
  vtkDataSet *Scalars;  // scalars to merge
  vtkDataSet *Vectors;  // vectors
  vtkDataSet *Normals;  // normals
  vtkDataSet *TCoords;  // texture coords
  vtkDataSet *Tensors;  // tensors
  vtkDataSet *UserDefined;  // user defined

  //Filter interface
  int GetDataReleased();
  void SetDataReleased(int flag);
};

#endif


