/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ContourF.cc
  Language:  C++
  Date:      02/07/94
  Version:   1.4

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "ContourF.hh"
#include "FScalars.hh"
#include "Cell.hh"
#include "MergePts.hh"

// Description:
// Construct object with initial range (0,1) and single contour value
// of 0.0.
vtkContourFilter::vtkContourFilter()
{
  for (int i=0; i<MAX_CONTOURS; i++) this->Values[i] = 0.0;
  this->NumberOfContours = 1;
  this->Range[0] = 0.0;
  this->Range[1] = 1.0;
}

vtkContourFilter::~vtkContourFilter()
{
}

// Description:
// Set a particular contour value at contour number i. The index i ranges 
// between 0<=i<NumberOfContours.
void vtkContourFilter::SetValue(int i, float value)
{
  i = (i >= MAX_CONTOURS ? MAX_CONTOURS-1 : (i < 0 ? 0 : i) );
  if ( this->Values[i] != value )
    {
    this->Modified();
    this->Values[i] = value;
    if ( i >= this->NumberOfContours ) this->NumberOfContours = i + 1;
    if ( value < this->Range[0] ) this->Range[0] = value;
    if ( value > this->Range[1] ) this->Range[1] = value;
    }
}

void vtkContourFilter::GenerateValues(int numContours, float range1, 
				     float range2)
{
  float rng[2];

  rng[0] = range1;
  rng[1] = range2;
  this->GenerateValues(numContours,rng);
}

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
void vtkContourFilter::GenerateValues(int numContours, float range[2])
{
  float val, incr;
  int i;

  numContours = (numContours > MAX_CONTOURS ? MAX_CONTOURS : 
                 (numContours > 1 ? numContours : 2) );

  incr = (range[1] - range[0]) / (numContours-1);
  for (i=0, val=range[0]; i < numContours; i++, val+=incr)
    {
    this->SetValue(i,val);
    }
}


//
// General contouring filter.  Handles arbitrary input.
//
void vtkContourFilter::Execute()
{
  int cellId, i;
  vtkIdList *cellPts;
  vtkScalars *inScalars;
  vtkFloatScalars cellScalars(MAX_CELL_SIZE);
  vtkCell *cell;
  float *range, value;
  vtkFloatScalars *newScalars;
  vtkCellArray *newVerts, *newLines, *newPolys;
  vtkFloatPoints *newPts;
  cellScalars.ReferenceCountingOff();

  vtkDebugMacro(<< "Executing contour filter");
//
// Initialize and check input
//
  this->Initialize();

  if ( ! (inScalars = this->Input->GetPointData()->GetScalars()) )
    {
    vtkErrorMacro(<<"No scalar data to contour");
    return;
    }
  range = inScalars->GetRange();
//
// Create objects to hold output of contour operation
//
  newPts = new vtkFloatPoints(1000,10000);
  newVerts = new vtkCellArray(1000,10000);
  newLines = new vtkCellArray(1000,10000);
  newPolys = new vtkCellArray(1000,10000);
  newScalars = new vtkFloatScalars(3000,30000);
//
// Loop over all contour values.  Then for each contour value, 
// loop over all cells.
//
  for (i=0; i<this->NumberOfContours; i++)
    {
    value = this->Values[i];
    for (cellId=0; cellId<Input->GetNumberOfCells(); cellId++)
      {
      cell = Input->GetCell(cellId);
      cellPts = cell->GetPointIds();
      inScalars->GetScalars(*cellPts,cellScalars);

      cell->Contour(value, &cellScalars, newPts, newVerts, newLines, newPolys, newScalars);

      } // for all cells
    } // for all contour values

  vtkDebugMacro(<<"Created: " 
               << newPts->GetNumberOfPoints() << " points, " 
               << newVerts->GetNumberOfCells() << " verts, " 
               << newLines->GetNumberOfCells() << " lines, " 
               << newPolys->GetNumberOfCells() << " triangles");
//
// Update ourselves.  Because we don't know up front how many verts, lines,
// polys we've created, take care to reclaim memory. 
//
  this->SetPoints(newPts);
  newPts->Delete();

  this->PointData.SetScalars(newScalars);
  newScalars->Delete();

  if (newVerts->GetNumberOfCells()) this->SetVerts(newVerts);
  newVerts->Delete();

  if (newLines->GetNumberOfCells()) this->SetLines(newLines);
  newLines->Delete();

  if (newPolys->GetNumberOfCells()) this->SetPolys(newPolys);
  newPolys->Delete();

  this->Squeeze();
}

void vtkContourFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  int i;

  vtkDataSetToPolyFilter::PrintSelf(os,indent);

  os << indent << "Number Of Contours : " << this->NumberOfContours << "\n";
  os << indent << "Contour Values: \n";
  for ( i=0; i<this->NumberOfContours; i++)
    {
    os << indent << "  Value " << i << ": " << this->Values[i] << "\n";
    }
}

