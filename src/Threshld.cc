/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Threshld.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Threshld.hh"

// Construct with lower threshold=0, upper threshold=1, and threshold 
// function=upper.
vtkThreshold::vtkThreshold()
{
  this->LowerThreshold = 0.0;
  this->UpperThreshold = 1.0;

  this->ThresholdFunction = &vtkThreshold::Upper;
}

// Description:
// Criterion is cells whose scalars are less than lower threshold.
void vtkThreshold::ThresholdByLower(float lower) 
{
  if ( this->LowerThreshold != lower )
    {
    this->LowerThreshold = lower; 
    this->ThresholdFunction = &vtkThreshold::Lower;
    this->Modified();
    }
}
                           
// Description:
// Criterion is cells whose scalars are less than upper threshold.
void vtkThreshold::ThresholdByUpper(float upper)
{
  if ( this->UpperThreshold != upper )
    {
    this->UpperThreshold = upper; 
    this->ThresholdFunction = &vtkThreshold::Upper;
    this->Modified();
    }
}
                           
// Description:
// Criterion is cells whose scalars are between lower and upper thresholds.
void vtkThreshold::ThresholdBetween(float lower, float upper)
{
  if ( this->LowerThreshold != lower || this->UpperThreshold != upper )
    {
    this->LowerThreshold = lower; 
    this->UpperThreshold = upper;
    this->ThresholdFunction = vtkThreshold::Between;
    this->Modified();
    }
}
  
void vtkThreshold::Execute()
{
  int cellId;
  vtkIdList *cellPts, *pointMap;
  vtkIdList newCellPts(MAX_CELL_SIZE);
  vtkScalars *inScalars;
  vtkFloatScalars cellScalars(MAX_CELL_SIZE);
  vtkCell *cell;
  vtkFloatPoints *newPoints;
  vtkPointData *pd;
  int i, ptId, newId, numPts, numCellPts;
  float *x;

  vtkDebugMacro(<< "Executing threshold filter");
  this->Initialize();

  if ( ! (inScalars = this->Input->GetPointData()->GetScalars()) )
    {
    vtkErrorMacro(<<"No scalar data to threshold");
    return;
    }
     
  numPts = this->Input->GetNumberOfPoints();

  this->Allocate(this->Input->GetNumberOfCells());
  newPoints = new vtkFloatPoints(numPts);
  pd = this->Input->GetPointData();
  this->PointData.CopyAllocate(pd);

  pointMap = new vtkIdList(numPts); // maps old point ids into new
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
      if ( ! ((this->*(this->ThresholdFunction))(cellScalars.GetScalar(ptId))) ) break;
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

  vtkDebugMacro(<< "Extracted " << this->GetNumberOfCells() 
               << " number of cells.");

  // now clean up / update ourselves
  pointMap->Delete();

  this->SetPoints(newPoints);
  newPoints->Delete();

  this->Squeeze();
}

void vtkThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToUnstructuredGridFilter::PrintSelf(os,indent);

  if ( this->ThresholdFunction == &vtkThreshold::Upper )
    os << indent << "Threshold By Upper\n";

  else if ( this->ThresholdFunction == &vtkThreshold::Lower )
    os << indent << "Threshold By Lower\n";

  else if ( this->ThresholdFunction == &vtkThreshold::Between )
    os << indent << "Threshold Between\n";

  os << indent << "Lower Threshold: " << this->LowerThreshold << "\n";;
  os << indent << "Upper Threshold: " << this->UpperThreshold << "\n";;
}
