/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PtS2PtSF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkPointSetToPointSetFilter - abstract filter class 
// .SECTION Description
// vtkPointSetToPointSetFilter is an abstract filter class whose subclasses
// take as input a point set and generates a point set on output.
// At a minimum the concrete subclasses of vtkPointSetToPointSetFilter
// modify their point coordinates. They never modify their topological 
// form, however.

#ifndef __vtkPointSetToPointSetFilter_h
#define __vtkPointSetToPointSetFilter_h

#include "PtSetF.hh"
#include "PointSet.hh"

class vtkPointSetToPointSetFilter : public vtkPointSet, public vtkPointSetFilter
{
public:
  vtkPointSetToPointSetFilter();
  ~vtkPointSetToPointSetFilter();
  char *GetDataType() {return this->PointSet->GetDataType();};
  char *GetClassName() {return "vtkPointSetToPointSetFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // dataset interface
  vtkDataSet *MakeObject();
  int GetNumberOfCells() {return this->PointSet->GetNumberOfCells();};
  vtkCell *GetCell(int cellId) {return this->PointSet->GetCell(cellId);};
  int GetCellType(int cellId) {return this->PointSet->GetCellType(cellId);};
  void GetCellPoints(int cellId, vtkIdList& ptIds)
    {this->PointSet->GetCellPoints(cellId, ptIds);};
  void GetPointCells(int ptId, vtkIdList& cellIds)
    {this->PointSet->GetPointCells(ptId, cellIds);};
  void Initialize();

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
  vtkDataSet *PointSet;

  //Filter interface
  int GetDataReleased();
  void SetDataReleased(int flag);
};

#endif


