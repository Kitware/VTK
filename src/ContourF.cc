/*=========================================================================

  Program:   Visualization Library
  Module:    ContourF.cc
  Language:  C++
  Date:      02/07/94
  Version:   1.4

This file is part of the Visualization Library. No part of this file or its 
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
vlContourFilter::vlContourFilter()
{
  for (int i=0; i<MAX_CONTOURS; i++) this->Values[i] = 0.0;
  this->NumberOfContours = 1;
  this->Range[0] = 0.0;
  this->Range[1] = 1.0;
}

vlContourFilter::~vlContourFilter()
{
}

// Description:
// Set a particular contour value at contour number i.
void vlContourFilter::SetValue(int i, float value)
{
  i = (i >= MAX_CONTOURS ? MAX_CONTOURS-1 : (i < 0 ? 0 : i) );
  if ( this->Values[i] != value )
    {
    this->Modified();
    this->Values[i] = value;
    if ( i > NumberOfContours ) this->NumberOfContours = i;
    if ( value < this->Range[0] ) this->Range[0] = value;
    if ( value > this->Range[1] ) this->Range[1] = value;
    }
}

// Description:
// Generate numContours equally spaced contour values between specified
// range.
void vlContourFilter::GenerateValues(int numContours, float range[2])
{
  float val, incr;
  int i;

  numContours = (numContours >= MAX_CONTOURS ? MAX_CONTOURS-1 : 
                 (numContours < 0 ? 0 : numContours) );

  if ( numContours < 1 ) numContours = 1;

  incr = (range[1] - range[0]) / numContours;
  for (i=0, val=range[0]; i < numContours; i++, val+=incr)
    {
    this->SetValue(i,val); // don't modify object unless absolutely nec.
    }
}


//
// General contouring filter.  Handles arbitrary input.
//
void vlContourFilter::Execute()
{
  int cellId, i;
  vlIdList *cellPts;
  vlScalars *inScalars;
  vlFloatScalars cellScalars(MAX_CELL_SIZE);
  vlCell *cell;
  float *range, value;
  vlFloatScalars *newScalars;
  vlCellArray *newVerts, *newLines, *newPolys;
  vlFloatPoints *newPts;

  vlDebugMacro(<< "Executing contour filter");
//
// Initialize and check input
//
  this->Initialize();

  if ( ! (inScalars = this->Input->GetPointData()->GetScalars()) )
    {
    vlErrorMacro(<<"No scalar data to contour");
    return;
    }
  range = inScalars->GetRange();
//
// Create objects to hold output of contour operation
//
  newPts = new vlFloatPoints(1000,10000);
  newVerts = new vlCellArray(1000,10000);
  newLines = new vlCellArray(1000,10000);
  newPolys = new vlCellArray(1000,10000);
  newScalars = new vlFloatScalars(3000,30000);
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

  vlDebugMacro(<<"Created: " 
               << newPts->GetNumberOfPoints() << " points, " 
               << newVerts->GetNumberOfCells() << " verts, " 
               << newLines->GetNumberOfCells() << " lines, " 
               << newPolys->GetNumberOfCells() << " triangles");
//
// Update ourselves.  Because we don't know up front how many verts, lines,
// polys we've created, take care to reclaim memory. 
//
  this->SetPoints(newPts);
  this->PointData.SetScalars(newScalars);

  if (newVerts->GetNumberOfCells()) 
    this->SetVerts(newVerts);
  else
    delete newVerts;

  if (newLines->GetNumberOfCells()) 
    this->SetLines(newLines);
  else
    delete newLines;

  if (newPolys->GetNumberOfCells()) 
    this->SetPolys(newPolys);
  else
    delete newPolys;

  this->Squeeze();
}

void vlContourFilter::PrintSelf(ostream& os, vlIndent indent)
{
  int i;

  vlDataSetToPolyFilter::PrintSelf(os,indent);

  os << indent << "Number Of Contours : " << this->NumberOfContours << "\n";
  os << indent << "Contour Values: \n";
  for ( i=0; i<this->NumberOfContours; i++)
    {
    os << indent << "  Value " << i << ": " << this->Values[i] << "\n";
    }
}

