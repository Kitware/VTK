/*=========================================================================

  Program:   Visualization Library
  Module:    MaskPoly.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "MaskPoly.hh"

//
// Down sample polygonal data.  Don't down sample points, usually not worth it.
//
void vlMaskPolyData::Execute()
{
  int numVerts, numLines, numPolys, numStrips;
  vlCellArray *inVerts,*inLines,*inPolys,*inStrips;
  int numNewVerts, numNewLines, numNewPolys, numNewStrips;
  vlCellArray *newVerts, *newLines, *newPolys, *newStrips;
  int id, interval;
  vlPointData *pd;
  int npts, *pts;
//
// Check input / pass data through
//
  this->Initialize();

  inVerts = this->Input->GetVerts();
  numVerts = inVerts->GetNumberOfCells();
  numNewVerts = numVerts / this->OnRatio;

  inLines = this->Input->GetLines();
  numLines = inLines->GetNumberOfCells();
  numNewLines = numLines / this->OnRatio;

  inPolys = this->Input->GetPolys();
  numPolys = inPolys->GetNumberOfCells();
  numNewPolys = numPolys / this->OnRatio;

  inStrips = this->Input->GetStrips();
  numStrips = inStrips->GetNumberOfCells();
  numNewStrips = numStrips / this->OnRatio;

  if ( numNewVerts < 1 && numNewLines < 1 &&
  numNewPolys < 1 && numNewStrips < 1 )
    {
    vlErrorMacro (<<"No PolyData to mask!");
    return;
    }
//
// Allocate space
//
  newVerts = new vlCellArray(numNewVerts);

  newLines = new vlCellArray;
  newLines->Allocate(newLines->EstimateSize(numNewLines,2));

  newPolys = new vlCellArray;
  newPolys->Allocate(newPolys->EstimateSize(numNewPolys,4));

  newStrips = new vlCellArray;
  newStrips->Allocate(newStrips->EstimateSize(numNewStrips,6));
//
// Traverse topological lists and traverse
//
  interval = this->Offset + this->OnRatio;
  for (id=0, inVerts->InitTraversal(); inVerts->GetNextCell(npts,pts); id++)
    {
    if ( ! (id % interval) )
      {
      newVerts->InsertNextCell(npts,pts);
      }
    }

  for (id=0, inLines->InitTraversal(); inLines->GetNextCell(npts,pts); id++)
    {
    if ( ! (id % interval) )
      {
      newLines->InsertNextCell(npts,pts);
      }
    }

  for (id=0, inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); id++)
    {
    if ( ! (id % interval) )
      {
      newPolys->InsertNextCell(npts,pts);
      }
    }

  for (id=0, inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); id++)
    {
    if ( ! (id % interval) )
      {
      newStrips->InsertNextCell(npts,pts);
      }
    }

//
// Update ourselves
//
  // pass through points and point data
  this->SetPoints(this->Input->GetPoints());
  pd = this->Input->GetPointData();
  this->PointData = *pd;

  newVerts->Squeeze();
  this->SetVerts(newVerts);

  newLines->Squeeze();
  this->SetLines(newLines);

  newPolys->Squeeze();
  this->SetPolys(newPolys);

  newStrips->Squeeze();
  this->SetStrips(newStrips);
}

void vlMaskPolyData::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlMaskPolyData::GetClassName()))
    {
    vlPolyToPolyFilter::PrintSelf(os,indent);

    os << indent << "On Ratio: " << this->OnRatio << "\n";
    os << indent << "Offset: " << this->Offset << "\n";
    }
}
