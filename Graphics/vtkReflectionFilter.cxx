/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReflectionFilter.cxx
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
#include "vtkReflectionFilter.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkReflectionFilter, "1.3");
vtkStandardNewMacro(vtkReflectionFilter);

//---------------------------------------------------------------------------
vtkReflectionFilter::vtkReflectionFilter()
{
  this->Plane = VTK_USE_X_MIN;
}

//---------------------------------------------------------------------------
vtkReflectionFilter::~vtkReflectionFilter()
{
}

//---------------------------------------------------------------------------
void vtkReflectionFilter::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  
  output->CopyStructure(input);
  output->BuildCells();
  
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  vtkPoints *points = input->GetPoints();
  vtkPoints *outPoints = output->GetPoints();
  
  float bounds[6];
  input->GetBounds(bounds);
  
  vtkIdType i;
  float point[3];
  vtkGenericCell *cell = vtkGenericCell::New();

  float constant;
  
  switch (this->Plane)
    {
    case VTK_USE_X_MIN:
      constant = 2*bounds[0];
      for (i = 0; i < numPts; i++)
        {
        points->GetPoint(i, point);
        outPoints->InsertNextPoint(-point[0] + constant, point[1], point[2]);
        }
      break;
    case VTK_USE_X_MAX:
      constant = 2*bounds[1];
      for (i = 0; i < numPts; i++)
        {
        points->GetPoint(i, point);
        outPoints->InsertNextPoint(-point[0] + constant, point[1], point[2]);
        }
      break;
    case VTK_USE_Y_MIN:
      constant = 2*bounds[2];
      for (i = 0; i < numPts; i++)
        {
        points->GetPoint(i, point);
        outPoints->InsertNextPoint(point[0], -point[1] + constant, point[2]);
        }
      break;
    case VTK_USE_Y_MAX:
      constant = 2*bounds[3];
      for (i = 0; i < numPts; i++)
        {
        points->GetPoint(i, point);
        outPoints->InsertNextPoint(point[0], -point[1] + constant, point[2]);
        }
      break;
    case VTK_USE_Z_MIN:
      constant = 2*bounds[4];
      for (i = 0; i < numPts; i++)
        {
        points->GetPoint(i, point);
        outPoints->InsertNextPoint(point[0], point[1], -point[2] + constant);
        }
      break;
    case VTK_USE_Z_MAX:
      constant = 2*bounds[5];
      for (i = 0; i < numPts; i++)
        {
        points->GetPoint(i, point);
        outPoints->InsertNextPoint(point[0], point[1], -point[2] + constant);
        }
      break;
    }

  int numCellPts, j, cellType;
  vtkIdType *newCellPts;
  vtkIdList *cellPts;
  
  for (i = 0; i < numCells; i++)
    {
    input->GetCell(i, cell);
    numCellPts = cell->GetNumberOfPoints();
    cellType = cell->GetCellType();
    newCellPts = new vtkIdType[numCellPts];
    cellPts = cell->GetPointIds();
    for (j = 0; j < numCellPts; j++)
      {
      newCellPts[j] = cellPts->GetId(j) + numPts;
      }
    output->InsertNextCell(cellType, numCellPts, newCellPts);
    delete [] newCellPts;
    }
  
  cell->Delete();
}

//---------------------------------------------------------------------------
void vtkReflectionFilter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Plane: " << this->Plane << endl;
}
