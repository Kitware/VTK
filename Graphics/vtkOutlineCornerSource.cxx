/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutlineCornerSource.cxx
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
#include "vtkOutlineCornerSource.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkOutlineCornerSource, "1.5");
vtkStandardNewMacro(vtkOutlineCornerSource);

//----------------------------------------------------------------------------
vtkOutlineCornerSource::vtkOutlineCornerSource()
    : vtkOutlineSource()
{
  this->CornerFactor = 0.2;
}

//----------------------------------------------------------------------------
void vtkOutlineCornerSource::Execute()
{
  float *bounds;
  float inner_bounds[6];

  int i, j, k;

  vtkDebugMacro(<< "Generating outline");

  // Initialize

  float delta;

  bounds = this->Bounds;
  for (i = 0; i < 3; i++)
  {
      delta = (bounds[2*i + 1] - bounds[2*i]) * this->CornerFactor;
      inner_bounds[2*i] = bounds[2*i] + delta;
      inner_bounds[2*i + 1] = bounds[2*i + 1] - delta;
  }

  // Allocate storage and create outline

  vtkPoints *newPts;
  vtkCellArray *newLines;
  vtkPolyData *output = this->GetOutput();
  
  newPts = vtkPoints::New();
  newPts->Allocate(32);
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(24,2));

  float x[3];
  vtkIdType pts[2];

  int pid = 0;

  // 32 points and 24 lines

  for (i = 0; i <= 1; i++)
  {
      for (j = 2; j <= 3; j++)
      {
          for (k = 4; k <= 5; k++)
          {
              pts[0] = pid;
              x[0] = bounds[i]; x[1] = bounds[j]; x[2] = bounds[k];
              newPts->InsertPoint(pid++, x);

              pts[1] = pid;
              x[0] = inner_bounds[i]; x[1] = bounds[j]; x[2] = bounds[k];
              newPts->InsertPoint(pid++, x);
              newLines->InsertNextCell(2,pts);

              pts[1] = pid;
              x[0] = bounds[i]; x[1] = inner_bounds[j]; x[2] = bounds[k];
              newPts->InsertPoint(pid++, x);
              newLines->InsertNextCell(2,pts);

              pts[1] = pid;
              x[0] = bounds[i]; x[1] = bounds[j]; x[2] = inner_bounds[k];
              newPts->InsertPoint(pid++, x);
              newLines->InsertNextCell(2,pts);
          }
      }
  }

  // Update selves and release memory

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();
}


//----------------------------------------------------------------------------
void vtkOutlineCornerSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CornerFactor: " << this->CornerFactor << "\n";
}
