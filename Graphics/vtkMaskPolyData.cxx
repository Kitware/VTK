/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMaskPolyData.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMaskPolyData.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMaskPolyData, "1.38");
vtkStandardNewMacro(vtkMaskPolyData);

vtkMaskPolyData::vtkMaskPolyData()
{
  this->OnRatio = 11;
  this->Offset = 0;
}

// Down sample polygonal data.  Don't down sample points (that is, use the
// original points, since usually not worth it.
//
void vtkMaskPolyData::Execute()
{
  vtkIdType numVerts, numLines, numPolys, numStrips;
  vtkCellArray *inVerts,*inLines,*inPolys,*inStrips;
  vtkIdType numNewVerts, numNewLines, numNewPolys, numNewStrips;
  vtkCellArray *newVerts=NULL, *newLines=NULL;
  vtkCellArray *newPolys=NULL, *newStrips=NULL;
  vtkIdType id, interval;
  vtkPointData *pd;
  vtkIdType numCells;
  vtkIdType *pts = 0;
  vtkIdType npts = 0;
  vtkPolyData *input= this->GetInput();
  vtkPolyData *output = this->GetOutput();
  
  // Check input / pass data through
  //
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

  numCells = numVerts + numLines + numPolys + numStrips;

  if ( numCells < 1 )
    {
    vtkErrorMacro (<<"No PolyData to mask!");
    return;
    }

  // Allocate space
  //
  if ( numNewVerts )
    {
    newVerts = vtkCellArray::New();
    newVerts->Allocate(numNewVerts);
    }

  if ( numNewLines )
    {
    newLines = vtkCellArray::New();
    newLines->Allocate(newLines->EstimateSize(numNewLines,2));
    }

  if ( numNewPolys )
    {
    newPolys = vtkCellArray::New();
    newPolys->Allocate(newPolys->EstimateSize(numNewPolys,4));
    }

  if ( numNewStrips )
    {
    newStrips = vtkCellArray::New();
    newStrips->Allocate(newStrips->EstimateSize(numNewStrips,6));
    }

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
    this->UpdateProgress((float)numVerts/numCells);
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
    this->UpdateProgress((float)(numVerts+numLines)/numCells);
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
    this->UpdateProgress((float)(numVerts+numLines+numPolys)/numCells);
    }

  if ( newStrips )
    {
    for (id=0, inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts);
         id++)
      {
      if ( ! (id % interval) )
        {
        newStrips->InsertNextCell(npts,pts);
        }
      }
    }

  // Update ourselves and release memory
  //
  output->SetPoints(input->GetPoints());
  pd = input->GetPointData();
  output->GetPointData()->PassData(pd);

  if (newVerts)
    {
    output->SetVerts(newVerts);
    newVerts->Delete();
    }

  if (newLines)
    {
    output->SetLines(newLines);
    newLines->Delete();
    } 

  if (newPolys)
    {
    output->SetPolys(newPolys);
    newPolys->Delete();
    }

  if (newStrips)
    {
    output->SetStrips(newStrips);
    newStrips->Delete();
    }

  output->Squeeze();
}

void vtkMaskPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "On Ratio: " << this->OnRatio << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
}
