/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PointSrc.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "PointSrc.hh"
#include "vtkMath.hh"

vtkPointSource::vtkPointSource(int numPts)
{
  this->NumberOfPoints = (numPts > 0 ? numPts : 10);

  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;

  this->Radius = 0.5;
}

void vtkPointSource::Execute()
{
  int i;
  float radius, theta, phi, x[3], rho;
  vtkFloatPoints *newPoints;
  vtkCellArray *newVerts;
  vtkMath math;

  vtkDebugMacro(<< "Executing Brownian filter");
  this->Initialize();

  newPoints = new vtkFloatPoints(this->NumberOfPoints);
  newVerts = new vtkCellArray;
  newVerts->Allocate(newVerts->EstimateSize(1,this->NumberOfPoints));

  newVerts->InsertNextCell(this->NumberOfPoints);
  for (i=0; i<this->NumberOfPoints; i++)
    {
    phi = math.Pi() * math.Random();
    rho = this->Radius * math.Random();
    radius = rho * sin((double)phi);
    theta = 2.0*math.Pi() * math.Random();
    x[0] = this->Center[0] + radius * cos((double)theta);
    x[1] = this->Center[1] + radius * sin((double)theta);
    x[2] = this->Center[2] + rho * cos((double)phi);
    newVerts->InsertCellPoint(newPoints->InsertNextPoint(x));
    }
//
// Update ourselves and release memory
//
  this->SetPoints(newPoints);
  newPoints->Delete();

  this->SetVerts(newVerts);
  newVerts->Delete();
}

void vtkPointSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolySource::PrintSelf(os,indent);

  os << indent << "Number Of Points: " << this->NumberOfPoints << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Center: (" << this->Center[0] << ", "
                              << this->Center[1] << ", "
                              << this->Center[2] << ")\n";

}
