/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDiskSource.cxx
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
#include "vtkDiskSource.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkDiskSource, "1.29");
vtkStandardNewMacro(vtkDiskSource);

vtkDiskSource::vtkDiskSource()
{
  this->InnerRadius = 0.25;
  this->OuterRadius = 0.5;
  this->RadialResolution = 1;
  this->CircumferentialResolution = 6;
}

void vtkDiskSource::Execute()
{
  vtkIdType numPolys, numPts;
  float x[3];
  int i, j;
  vtkIdType pts[4];
  float theta, deltaRadius;
  float cosTheta, sinTheta;
  vtkPoints *newPoints; 
  vtkCellArray *newPolys;
  vtkPolyData *output = this->GetOutput();
  
  //
  // Set things up; allocate memory
  //

  numPts = (this->RadialResolution + 1) * 
           (this->CircumferentialResolution + 1);
  numPolys = this->RadialResolution * this->CircumferentialResolution;
  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(numPolys,4));
//
// Create disk
//
  theta = 2.0 * vtkMath::Pi() / ((float)this->CircumferentialResolution);
  deltaRadius = (this->OuterRadius - this->InnerRadius) / 
                       ((float)this->RadialResolution);

  for (i=0; i<=this->CircumferentialResolution; i++) 
    {
    cosTheta = cos((double)i*theta);
    sinTheta = sin((double)i*theta);
    for (j=0; j <= this->RadialResolution; j++)
      {
      x[0] = (this->InnerRadius + j*deltaRadius) * cosTheta;
      x[1] = (this->InnerRadius + j*deltaRadius) * sinTheta;
      x[2] = 0.0;
      newPoints->InsertNextPoint(x);
      }
    }
//
//  Create connectivity
//
    for (i=0; i < this->CircumferentialResolution; i++) 
      {
      for (j=0; j < this->RadialResolution; j++) 
        {
        pts[0] = i*(this->RadialResolution+1) + j;
        pts[1] = pts[0] + 1;
        pts[2] = pts[1] + this->RadialResolution + 1;
        pts[3] = pts[2] - 1;
        newPolys->InsertNextCell(4,pts);
        }
      }
//
// Update ourselves and release memory
//
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();
}

void vtkDiskSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "InnerRadius: " << this->InnerRadius << "\n";
  os << indent << "OuterRadius: " << this->OuterRadius << "\n";
  os << indent << "RadialResolution: " << this->RadialResolution << "\n";
  os << indent << "CircumferentialResolution: " << this->CircumferentialResolution << "\n";
}
