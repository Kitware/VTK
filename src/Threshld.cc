/*=========================================================================

  Program:   Visualization Library
  Module:    Threshld.cc
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
#include "Threshld.hh"

vlThreshold::vlThreshold()
{
  this->LowerThreshold = 0.0;
  this->UpperThreshold = 1.0;

 // this->ThresholdFunction = this->Upper;
}

void vlThreshold::Execute()
{
  int cellId;
  vlIdList *cellPts, *pointMap;
  vlIdList newCellPts(MAX_CELL_SIZE);
  vlScalars *inScalars;
  vlFloatScalars cellScalars(MAX_CELL_SIZE);
  vlCell *cell;
  vlFloatPoints *newPoints;
  vlPointData *pd;
  int i, ptId, newId, numPts, numCellPts;
  float *x;

  vlDebugMacro(<< "Executing threshold filter");

  // check input
  this->Initialize();

  if ( ! (inScalars = this->Input->GetPointData()->GetScalars()) )
    {
    vlErrorMacro(<<"No scalar data to threshold");
    return;
    }
     
  numPts = this->Input->GetNumberOfPoints();

  this->Allocate(this->Input->GetNumberOfCells());
  newPoints = new vlFloatPoints(numPts);
  pd = this->Input->GetPointData();
  this->PointData.CopyAllocate(pd);

  pointMap = new vlIdList(numPts); // maps old point ids into new
  for (i=0; i < numPts; i++) pointMap->SetId(i,-1);

  // Check that the scalars of each cell satisfy the threshold criterion
  for (cellId=0; cellId < this->Input->GetNumberOfCells(); cellId++)
    {
    cell = this->Input->GetCell(cellId);
    cellPts = cell->GetPointIds();
    inScalars->GetScalars(*cellPts,cellScalars);
    numCellPts = cell->GetNumberOfPoints();

    for ( i=0; i < numCellPts; i++)
      {
      ptId = cellPts->GetId(i);
      if ( !(*(this->ThresholdFunction))(cellScalars.GetScalar(ptId)) ) break;
      }

    if ( i >= numCellPts ) // satisfied thresholding
      {
      for (i=0; i < numCellPts; i++)
        {
        ptId = cellPts->GetId(i);
        if ( (newId = pointMap->GetId(ptId)) < 0 )
          {
          x = this->Input->GetPoint(ptId);
          newId = newPoints->InsertNextPoint(x);
          pointMap->SetId(ptId,newId);
          this->PointData.CopyData(pd,ptId,newId);
          }
        newCellPts.SetId(i,newId);
        }
      this->InsertNextCell(cell->GetCellType(),newCellPts);
      } // satisfied thresholding
    } // for all cells

  vlDebugMacro(<< "Extracted " << this->GetNumberOfCells() 
               << " number of cells.");

  // now clean up / update ourselves
  delete pointMap;
  this->Squeeze();
  newPoints->Squeeze();

  this->SetPoints(newPoints);
}

