/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridOutlineFilter.cxx
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
#include "vtkRectilinearGridOutlineFilter.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkRectilinearGridOutlineFilter, "1.3");
vtkStandardNewMacro(vtkRectilinearGridOutlineFilter);


void vtkRectilinearGridOutlineFilter::ExecuteInformation()
{
  if (this->GetInput() == NULL)
    {
    return;
    }

  // Although there may be overlap between piece outlines,
  // it is not worth requesting exact extents.
  this->GetInput()->RequestExactExtentOff();
}


void vtkRectilinearGridOutlineFilter::Execute()
{
  float         bounds[6];
  float         *range;
  float         x[3];
  vtkIdType     pts[2];
  vtkPoints*    newPts;
  vtkCellArray* newLines;
  vtkPolyData*  output = this->GetOutput();
  vtkRectilinearGrid* input = this->GetInput();

  if (this->GetInput() == NULL)
    {
    return;
    }
  
  vtkDataArray* xCoords  = input->GetXCoordinates();
  vtkDataArray* yCoords  = input->GetYCoordinates();
  vtkDataArray* zCoords  = input->GetZCoordinates();
  int*          ext      = input->GetExtent();;
  int*          wholeExt = input->GetWholeExtent();


  if (xCoords == NULL || yCoords == NULL || zCoords == NULL ||
      input->GetNumberOfCells() == 0)
    {
    return;
    }

  // We could probably use just the input bounds ...
  range = xCoords->GetRange();
  bounds[0] = range[0];
  bounds[1] = range[1];
  range = yCoords->GetRange();
  bounds[2] = range[0];
  bounds[3] = range[1];
  range = zCoords->GetRange();
  bounds[4] = range[0];
  bounds[5] = range[1];

  //
  // Allocate storage and create outline
  //
  newPts = vtkPoints::New();
  newPts->Allocate(24);
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(12,2));

  // xMin yMin
  if (ext[0] == wholeExt[0] && ext[2] == wholeExt[2])
    {
    x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }
  // xMin yMax
  if (ext[0] == wholeExt[0] && ext[3] == wholeExt[3])
    {
    x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }
  // xMin zMin
  if (ext[0] == wholeExt[0] && ext[4] == wholeExt[4])
    {
    x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[4];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }
  // xMin zMax
  if (ext[0] == wholeExt[0] && ext[5] == wholeExt[5])
    {
    x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[5];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }
  // xMax yMin
  if (ext[1] == wholeExt[1] && ext[2] == wholeExt[2])
    {
    x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }
  // xMax yMax
  if (ext[1] == wholeExt[1] && ext[3] == wholeExt[3])
    {
    x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }
  // xMax zMin
  if (ext[1] == wholeExt[1] && ext[4] == wholeExt[4])
    {
    x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[4];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }
  // xMax zMax
  if (ext[1] == wholeExt[1] && ext[5] == wholeExt[5])
    {
    x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[5];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }  
  // yMin zMin
  if (ext[2] == wholeExt[2] && ext[4] == wholeExt[4])
    {
    x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[4];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }  
  // yMin zMax
  if (ext[2] == wholeExt[2] && ext[5] == wholeExt[5])
    {
    x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[5];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }  
  // yMax zMin
  if (ext[3] == wholeExt[3] && ext[4] == wholeExt[4])
    {
    x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[4];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }  
  // yMax zMax
  if (ext[3] == wholeExt[3] && ext[5] == wholeExt[5])
    {
    x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[5];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    }  


  
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  output->Squeeze();
}


