/*=========================================================================

  Program:   Visualization Library
  Module:    ExtractG.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "ExtractG.hh"
#include "vlMath.hh"

// Description:
// Construct with ExtractInside turned on.
vlExtractGeometry::vlExtractGeometry(vlImplicitFunction *f)
{
  this->ImplicitFunction = f;
  this->ExtractInside = 1;
}

// Description:
// Overload standard modified time function. If cut functions is modified,
// then we are modified as well.
unsigned long vlExtractGeometry::GetMTime()
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

void vlExtractGeometry::Execute()
{
  int ptId, numPts, i, cellId;
  vlIdList *cellPts, *pointMap;
  vlCell *cell;
  vlMath math;
  int numCellPts, newId;
  vlPointData *pd;
  float *x;
  float multiplier;
  vlFloatPoints *newPts;
  vlIdList newCellPts(MAX_CELL_SIZE);

  vlDebugMacro(<< "Extracting geometry");
  this->Initialize();

  if ( ! this->ImplicitFunction )
    {
    vlErrorMacro(<<"No implicit function specified");
    return;
    }

  if ( this->ExtractInside ) multiplier = 1.0;
  else multiplier = -1.0;
//
// Loop over all points determining whether they are inside sphere. Copy if
// they are.
//
  numPts = this->Input->GetNumberOfPoints();
  pointMap = new vlIdList(numPts); // maps old point ids into new
  for (i=0; i < numPts; i++) pointMap->SetId(i,-1);

  newPts = new vlFloatPoints(numPts/4,numPts);
  pd = this->Input->GetPointData();
  this->PointData.CopyAllocate(pd);
  
  for ( ptId=0; ptId < numPts; ptId++ )
    {
    x = this->Input->GetPoint(ptId);
    if ( (this->ImplicitFunction->Evaluate(x[0],x[1],x[2])*multiplier) > 0.0 )
      {
      newId = newPts->InsertNextPoint(x);
      pointMap->SetId(ptId,newId);
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
      if ( (newId=pointMap->GetId(ptId)) < 0 ) break;
      newCellPts.SetId(i,newId);
      }

    if ( i >= numCellPts )
      {
      this->InsertNextCell(cell->GetCellType(),newCellPts);
      }
    }
//
// Update ourselves
//
  delete pointMap;

  this->SetPoints(newPts);
  this->Squeeze();
}

void vlExtractGeometry::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataSetToUnstructuredGridFilter::PrintSelf(os,indent);

  os << indent << "Implicit Function: " << (void *)this->ImplicitFunction << "\n";
  os << indent << "Extract Inside: " << (this->ExtractInside ? "On\n" : "Off\n");
}
