/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutlineSource.cxx
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
#include "vtkOutlineSource.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkOutlineSource, "1.28");
vtkStandardNewMacro(vtkOutlineSource);

//----------------------------------------------------------------------------
vtkOutlineSource::vtkOutlineSource()
{
  for (int i=0; i<3; i++) 
    {
    this->Bounds[2*i] = -1.0;
    this->Bounds[2*i+1] = 1.0;
    }
}

//----------------------------------------------------------------------------
void vtkOutlineSource::Execute()
{
  float *bounds;
  float x[3];
  vtkIdType pts[2];
  vtkPoints *newPts;
  vtkCellArray *newLines;
  vtkPolyData *output = this->GetOutput();
  
  vtkDebugMacro(<< "Generating outline");
  //
  // Initialize
  //
  bounds = this->Bounds;
  //
  // Allocate storage and create outline
  //
  newPts = vtkPoints::New();
  newPts->Allocate(8);
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(12,2));

  x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[4];
  newPts->InsertPoint(0,x);
  x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[4];
  newPts->InsertPoint(1,x);
  x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[4];
  newPts->InsertPoint(2,x);
  x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[4];
  newPts->InsertPoint(3,x);
  x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[5];
  newPts->InsertPoint(4,x);
  x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[5];
  newPts->InsertPoint(5,x);
  x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[5];
  newPts->InsertPoint(6,x);
  x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[5];
  newPts->InsertPoint(7,x);

  pts[0] = 0; pts[1] = 1;
  newLines->InsertNextCell(2,pts);
  pts[0] = 2; pts[1] = 3;
  newLines->InsertNextCell(2,pts);
  pts[0] = 4; pts[1] = 5;
  newLines->InsertNextCell(2,pts);
  pts[0] = 6; pts[1] = 7;
  newLines->InsertNextCell(2,pts);
  pts[0] = 0; pts[1] = 2;
  newLines->InsertNextCell(2,pts);
  pts[0] = 1; pts[1] = 3;
  newLines->InsertNextCell(2,pts);
  pts[0] = 4; pts[1] = 6;
  newLines->InsertNextCell(2,pts);
  pts[0] = 5; pts[1] = 7;
  newLines->InsertNextCell(2,pts);
  pts[0] = 0; pts[1] = 4;
  newLines->InsertNextCell(2,pts);
  pts[0] = 1; pts[1] = 5;
  newLines->InsertNextCell(2,pts);
  pts[0] = 2; pts[1] = 6;
  newLines->InsertNextCell(2,pts);
  pts[0] = 3; pts[1] = 7;
  newLines->InsertNextCell(2,pts);
  //
  // Update selves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();
}


//----------------------------------------------------------------------------
void vtkOutlineSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Bounds: (" << this->Bounds[0] << ", " 
     << this->Bounds[1] << ") (" << this->Bounds[2] << ") ("
     << this->Bounds[3] << ") (" << this->Bounds[4] << ") ("
     << this->Bounds[5] << ")\n";
}
