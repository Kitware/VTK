/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DiskSrc.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Methods for Disk generator
//
#include <math.h>
#include "DiskSrc.hh"
#include "vtkMath.hh"

vtkDiskSource::vtkDiskSource()
{
  this->InnerRadius = 0.25;
  this->OuterRadius = 0.5;
  this->RadialResolution = 1;
  this->CircumferentialResolution = 6;
}

void vtkDiskSource::Execute()
{
  int numPolys, numPts;
  float x[3];
  int i, j;
  int pts[4];
  float theta, deltaRadius;
  float cosTheta, sinTheta;
  vtkFloatPoints *newPoints; 
  vtkCellArray *newPolys;
  vtkMath math;
//
// Set things up; allocate memory
//
  this->Initialize();

  numPts = (this->RadialResolution + 1) * 
           (this->CircumferentialResolution + 1);
  numPolys = this->RadialResolution * this->CircumferentialResolution;
  newPoints = new vtkFloatPoints(numPts);
  newPolys = new vtkCellArray;
  newPolys->Allocate(newPolys->EstimateSize(numPolys,4));
//
// Create disk
//
  theta = 2.0 * math.Pi() / ((float)this->CircumferentialResolution);
  deltaRadius = (this->OuterRadius - InnerRadius) / 
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
// Update ourselves
//
  this->SetPoints(newPoints);
  this->SetPolys(newPolys);
}

void vtkDiskSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolySource::PrintSelf(os,indent);

  os << indent << "InnerRadius: " << this->InnerRadius << "\n";
  os << indent << "OuterRadius: " << this->OuterRadius << "\n";
  os << indent << "RadialResolution: " << this->RadialResolution << "\n";
  os << indent << "CircumferentialResolution: " << this->CircumferentialResolution << "\n";
}
