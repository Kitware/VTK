/*=========================================================================

  Program:   Visualization Toolkit
  Module:    MaskPoly.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "MaskPoly.hh"

vtkMaskPolyData::vtkMaskPolyData()
{
  this->OnRatio = 11;
  this->Offset = 0;
}

//
// Down sample polygonal data.  Don't down sample points (that is, use the
// original points, since usually not worth it.
//
void vtkMaskPolyData::Execute()
{
  int numVerts, numLines, numPolys, numStrips;
  vtkCellArray *inVerts,*inLines,*inPolys,*inStrips;
  int numNewVerts, numNewLines, numNewPolys, numNewStrips;
  vtkCellArray *newVerts=NULL, *newLines=NULL;
  vtkCellArray *newPolys=NULL, *newStrips=NULL;
  int id, interval;
  vtkPointData *pd;
  int npts, *pts;
  vtkPolyData *input=(vtkPolyData *)this->Input;
//
// Check input / pass data through
//
  this->Initialize();

  inVerts = input->GetVerts();
  numVerts = inVerts->GetNumberOfCells();
  numNewVerts = numVerts / this->OnRatio;

  inLines = input->GetLines();
  numLines = inLines->GetNumberOfCells();
  numNewLines = numLines / this->OnRatio;

  inPolys = input->GetPolys();
  numPolys = inPolys->GetNumberOfCells();
  numNewPolys = numPolys / this->OnRatio;

  inStrips = input->GetStrips();
  numStrips = inStrips->GetNumberOfCells();
  numNewStrips = numStrips / this->OnRatio;

  if ( numNewVerts < 1 && numNewLines < 1 &&
  numNewPolys < 1 && numNewStrips < 1 )
    {
    vtkErrorMacro (<<"No PolyData to mask!");
    return;
    }
//
// Allocate space
//
  if ( numNewVerts )
    newVerts = new vtkCellArray(numNewVerts);

  if ( numNewLines )
    {
    newLines = new vtkCellArray;
    newLines->Allocate(newLines->EstimateSize(numNewLines,2));
    }

  if ( numNewPolys )
    {
    newPolys = new vtkCellArray;
    newPolys->Allocate(newPolys->EstimateSize(numNewPolys,4));
    }

  if ( numNewStrips )
    {
    newStrips = new vtkCellArray;
    newStrips->Allocate(newStrips->EstimateSize(numNewStrips,6));
    }
//
// Traverse topological lists and traverse
//
  interval = this->Offset + this->OnRatio;
  if ( newVerts )
    {
    for (id=0, inVerts->InitTraversal(); inVerts->GetNextCell(npts,pts); id++)
      {
      if ( ! (id % interval) )
        {
        newVerts->InsertNextCell(npts,pts);
        }
      }
    }

  if ( newLines )
    {
    for (id=0, inLines->InitTraversal(); inLines->GetNextCell(npts,pts); id++)
      {
      if ( ! (id % interval) )
        {
        newLines->InsertNextCell(npts,pts);
        }
      }
    }

  if ( newPolys )
    {
    for (id=0, inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); id++)
      {
      if ( ! (id % interval) )
        {
        newPolys->InsertNextCell(npts,pts);
        }
      }
    }

  if ( newStrips )
    {
    for (id=0, inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); id++)
      {
      if ( ! (id % interval) )
        {
        newStrips->InsertNextCell(npts,pts);
        }
      }
    }
//
// Update ourselves and release memory
//
  // pass through points and point data
  this->SetPoints(input->GetPoints());
  pd = input->GetPointData();
  this->PointData = *pd;

  if (newVerts)
    {
    this->SetVerts(newVerts);
    newVerts->Delete();
    }

  if (newLines)
    {
    this->SetLines(newLines);
    newLines->Delete();
    } 

  if (newPolys)
    {
    this->SetPolys(newPolys);
    newPolys->Delete();
    }

  if (newStrips)
    {
    this->SetStrips(newStrips);
    newStrips->Delete();
    }

  this->Squeeze();
}

void vtkMaskPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "On Ratio: " << this->OnRatio << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
}
