/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Cutter.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Cutter.hh"

// Description:
// Construct with user-specified implicit function.
vtkCutter::vtkCutter(vtkImplicitFunction *cf)
{
  this->CutFunction = cf;
}

vtkCutter::~vtkCutter()
{
}

// Description:
// Overload standard modified time function. If cut functions is modified,
// then we are modified as well.
unsigned long vtkCutter::GetMTime()
{
  unsigned long mTime=this->vtkDataSetToPolyFilter::GetMTime();
  unsigned long cutFuncMTime;

  if ( this->CutFunction != NULL )
    {
    cutFuncMTime = this->CutFunction->GetMTime();
    mTime = ( cutFuncMTime > mTime ? cutFuncMTime : mTime );
    }

  return mTime;
}

//
// Cut through data generating surface.
//
void vtkCutter::Execute()
{
  int cellId, i;
  vtkFloatPoints *cellPts;
  vtkFloatScalars cellScalars(MAX_CELL_SIZE);
  vtkCell *cell;
  vtkFloatScalars *newScalars;
  vtkCellArray *newVerts, *newLines, *newPolys;
  vtkFloatPoints *newPoints;
  float value, *x, s;

  vtkDebugMacro(<< "Executing cutter");
//
// Initialize self; create output objects
//
  this->Initialize();

  if ( !this->CutFunction )
    {
    vtkErrorMacro(<<"No cut function specified");
    return;
    }
//
// Create objects to hold output of contour operation
//
  newPoints = new vtkFloatPoints(1000,10000);
  newVerts = new vtkCellArray(1000,1000);
  newLines = new vtkCellArray(1000,10000);
  newPolys = new vtkCellArray(1000,10000);
  newScalars = new vtkFloatScalars(3000,30000);
//
// Loop over all cells creating scalar function determined by evaluating cell
// points using cut function.
//
  value = 0.0;
  for (cellId=0; cellId<Input->GetNumberOfCells(); cellId++)
    {
    cell = Input->GetCell(cellId);
    cellPts = cell->GetPoints();
    for (i=0; i<cellPts->GetNumberOfPoints(); i++)
      {
      x = cellPts->GetPoint(i);
      s = this->CutFunction->EvaluateFunction(x);
      cellScalars.SetScalar(i,s);
      }

    cell->Contour(value, &cellScalars, newPoints, newVerts, newLines, newPolys, newScalars);

    } // for all cells
//
// Update ourselves.  Because we don't know upfront how many verts, lines,
// polys we've created, take care to reclaim memory. 
//
  this->SetPoints(newPoints);
  newPoints->Delete();

  if (newVerts->GetNumberOfCells()) this->SetVerts(newVerts);
  newVerts->Delete();

  if (newLines->GetNumberOfCells()) this->SetLines(newLines);
  newLines->Delete();

  if (newPolys->GetNumberOfCells()) this->SetPolys(newPolys);
  newPolys->Delete();

  this->PointData.SetScalars(newScalars);
  newScalars->Delete();

  this->Squeeze();
}

void vtkCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyFilter::PrintSelf(os,indent);

  os << indent << "Cut Function: " << this->CutFunction << "\n";
}
