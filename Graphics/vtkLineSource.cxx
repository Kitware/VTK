/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLineSource.cxx
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
#include "vtkLineSource.h"
#include "vtkPoints.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

#include <math.h>
vtkCxxRevisionMacro(vtkLineSource, "1.40");
vtkStandardNewMacro(vtkLineSource);

vtkLineSource::vtkLineSource(int res)
{
  this->Point1[0] = -0.5;
  this->Point1[1] =  0.0;
  this->Point1[2] =  0.0;

  this->Point2[0] =  0.5;
  this->Point2[1] =  0.0;
  this->Point2[2] =  0.0;

  this->Resolution = (res < 1 ? 1 : res);
}

void vtkLineSource::Execute()
{
  int numLines=this->Resolution;
  int numPts=this->Resolution+1;
  float x[3], tc[3], v[3];
  int i, j;
  vtkPoints *newPoints; 
  vtkFloatArray *newTCoords; 
  vtkCellArray *newLines;
  vtkPolyData *output = this->GetOutput();
  
  vtkDebugMacro(<<"Creating line");

  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);
  newTCoords = vtkFloatArray::New();
  newTCoords->SetNumberOfComponents(2);
  newTCoords->Allocate(2*numPts);

  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(numLines,2));
//
// Generate points and texture coordinates
//
  for (i=0; i<3; i++)
    {
    v[i] = this->Point2[i] - this->Point1[i];
    }

  tc[1] = 0.0;
  tc[2] = 0.0;
  for (i=0; i<numPts; i++) 
    {
    tc[0] = ((float)i/this->Resolution);
    for (j=0; j<3; j++)
      {
      x[j] = this->Point1[j] + tc[0]*v[j];
      }
    newPoints->InsertPoint(i,x);
    newTCoords->InsertTuple(i,tc);
    }
//
//  Generate lines
//
  newLines->InsertNextCell(numPts);
  for (i=0; i < numPts; i++) 
    {
    newLines->InsertCellPoint (i);
    }
//
// Update ourselves and release memory
//
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();

  output->SetLines(newLines);
  newLines->Delete();
}

void vtkLineSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";

  os << indent << "Point 1: (" << this->Point1[0] << ", "
                               << this->Point1[1] << ", "
                               << this->Point1[2] << ")\n";

  os << indent << "Point 2: (" << this->Point2[0] << ", "
                               << this->Point2[1] << ", "
                               << this->Point2[2] << ")\n";


}
