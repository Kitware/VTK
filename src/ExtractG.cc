/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ExtractG.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "ExtractG.hh"
#include "vtkMath.hh"

// Description:
// Construct with ExtractInside turned on.
vtkExtractGeometry::vtkExtractGeometry(vtkImplicitFunction *f)
{
  this->ImplicitFunction = f;
  this->ExtractInside = 1;
}

// Description:
// Overload standard modified time function. If cut functions is modified,
// then we are modified as well.
unsigned long vtkExtractGeometry::GetMTime()
{
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long impFuncMTime;

  if ( this->ImplicitFunction != NULL )
    {
    impFuncMTime = this->ImplicitFunction->GetMTime();
    mTime = ( impFuncMTime > mTime ? impFuncMTime : mTime );
    }

  return mTime;
}

void vtkExtractGeometry::Execute()
{
  int ptId, numPts, numCells, i, cellId;
  vtkIdList *cellPts;
  vtkCell *cell;
  vtkMath math;
  int numCellPts, newId, *pointMap;
  vtkPointData *pd;
  float *x;
  float multiplier;
  vtkFloatPoints *newPts;
  vtkIdList newCellPts(MAX_CELL_SIZE);

  vtkDebugMacro(<< "Extracting geometry");
  this->Initialize();

  if ( ! this->ImplicitFunction )
    {
    vtkErrorMacro(<<"No implicit function specified");
    return;
    }

  if ( this->ExtractInside ) multiplier = 1.0;
  else multiplier = -1.0;
//
// Loop over all points determining whether they are inside sphere. Copy if
// they are.
//
  numPts = this->Input->GetNumberOfPoints();
  numCells = this->Input->GetNumberOfCells();
  pointMap = new int[numPts]; // maps old point ids into new
  for (i=0; i < numPts; i++) pointMap[i] = -1;

  this->Allocate(numCells/4); //allocate storage for geometry/topology
  newPts = new vtkFloatPoints(numPts/4,numPts);
  pd = this->Input->GetPointData();
  this->PointData.CopyAllocate(pd);
  
  for ( ptId=0; ptId < numPts; ptId++ )
    {
    x = this->Input->GetPoint(ptId);
    if ( (this->ImplicitFunction->FunctionValue(x)*multiplier) < 0.0 )
      {
      newId = newPts->InsertNextPoint(x);
      pointMap[ptId] = newId;
      this->PointData.CopyData(pd,ptId,newId);
      }
    }
//
// Now loop over all cells to see whether they are inside sphere. Copy if
// they are.
//
  for (cellId=0; cellId < this->Input->GetNumberOfCells(); cellId++)
    {
    cell = this->Input->GetCell(cellId);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();

    for ( i=0; i < numCellPts; i++)
      {
      ptId = cellPts->GetId(i);
      if ( pointMap[ptId] < 0 ) break;
      newCellPts.SetId(i,pointMap[ptId]);
      }

    if ( i >= numCellPts )
      {
      this->InsertNextCell(cell->GetCellType(),newCellPts);
      }
    }
//
// Update ourselves and release memory
//
  delete [] pointMap;

  this->SetPoints(newPts);
  newPts->Delete();

  this->Squeeze();
}

void vtkExtractGeometry::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToUnstructuredGridFilter::PrintSelf(os,indent);

  os << indent << "Implicit Function: " << (void *)this->ImplicitFunction << "\n";
  os << indent << "Extract Inside: " << (this->ExtractInside ? "On\n" : "Off\n");
}
