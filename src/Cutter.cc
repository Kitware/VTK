/*=========================================================================

  Program:   Visualization Library
  Module:    Cutter.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Cutter.hh"

// Description:
// Construct with user-specified implicit function.
vlCutter::vlCutter(vlImplicitFunction *cf)
{
  this->CutFunction = cf;
}

vlCutter::~vlCutter()
{
}

// Description:
// Overload standard modified time function. If cut functions is modified,
// then we are modified as well.
unsigned long vlCutter::GetMTime()
{
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long cutFuncMTime;

  if ( this->CutFunction != NULL )
    {
    cutFuncMTime = this->CutFunction->GetMTime();
    mTime = ( cutFuncMTime > mTime ? cutFuncMTime : mTime );
    }

  return mTime;
}

//
// Cut through data generating surface.  Point values can be determined using
// ProbeFilter.
//
void vlCutter::Execute()
{
  int cellId, i;
  vlFloatPoints *cellPts;
  vlFloatScalars cellScalars(MAX_CELL_SIZE);
  vlCell *cell;
  vlFloatScalars *newScalars;
  vlCellArray *newVerts, *newLines, *newPolys;
  vlFloatPoints *newPoints;
  float value, *x, s;

  vlDebugMacro(<< "Executing cutter");
//
// Initialize self; create output objects
//
  this->Initialize();

  if ( !this->CutFunction )
    {
    vlErrorMacro(<<"No cut function specified");
    return;
    }
//
// Create objects to hold output of contour operation
//
  newPoints = new vlFloatPoints(1000,1000);
  newVerts = new vlCellArray(1000,1000);
  newLines = new vlCellArray(1000,10000);
  newPolys = new vlCellArray(1000,10000);
  newScalars = new vlFloatScalars(3000,30000);
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
      s = this->CutFunction->Evaluate(x[0], x[1], x[2]);
      cellScalars.SetScalar(i,s);
      }

    cell->Contour(value, &cellScalars, newPoints, newVerts, newLines, newPolys, newScalars);

    } // for all cells
//
// Update ourselves.  Because we don't know upfront how many verts, lines,
// polys we've created, take care to reclaim memory. 
//
  newPoints->Squeeze();
  this->SetPoints(newPoints);

  if (newVerts->GetNumberOfCells()) 
    {
    newVerts->Squeeze();    
    this->SetVerts(newVerts);
    }
  else
    {
    delete newVerts;
    }

  if (newLines->GetNumberOfCells()) 
    {
    newLines->Squeeze();    
    this->SetLines(newLines);
    }
  else
    {
    delete newLines;
    }

  if (newPolys->GetNumberOfCells()) 
    {
    newPolys->Squeeze();    
    this->SetPolys(newPolys);
    }
  else
    {
    delete newPolys;
    }

  this->PointData.SetScalars(newScalars);
  

}
