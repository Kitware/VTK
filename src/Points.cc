/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Points.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Points.hh"

vtkPoints::vtkPoints()
{
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = 0.0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1.0;
}

// Description:
// Insert point into position indicated.
void vtkPoints::InsertPoint(int id, float x, float y, float z)
{
  float X[3];

  X[0] = x;
  X[1] = y;
  X[2] = z;
  this->InsertPoint(id,X);
}

// Description:
// Insert point into position indicated.
int vtkPoints::InsertNextPoint(float x, float y, float z)
{
  float X[3];

  X[0] = x;
  X[1] = y;
  X[2] = z;
  return this->InsertNextPoint(X);
}

void vtkPoints::GetPoint(int id, float x[3])
{
  float *xp = this->GetPoint(id);
  for (int i=0; i<3; i++) x[i] = xp[i];
}

// Description:
// Given a list of pt ids, return an array of point coordinates.
void vtkPoints::GetPoints(vtkIdList& ptId, vtkFloatPoints& fp)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    fp.InsertPoint(i,this->GetPoint(ptId.GetId(i)));
    }
}

// Description:
// Determine (xmin,xmax, ymin,ymax, zmin,zmax) bounds of points.
void vtkPoints::ComputeBounds()
{
  int i, j;
  float *x;

  if ( this->GetMTime() > this->ComputeTime )
    {
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] =  LARGE_FLOAT;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -LARGE_FLOAT;
    for (i=0; i<this->GetNumberOfPoints(); i++)
      {
      x = this->GetPoint(i);
      for (j=0; j<3; j++)
        {
        if ( x[j] < this->Bounds[2*j] ) this->Bounds[2*j] = x[j];
        if ( x[j] > this->Bounds[2*j+1] ) this->Bounds[2*j+1] = x[j];
        }
      }

    this->ComputeTime.Modified();
    }
}

// Description:
// Return the bounds of the points.
float *vtkPoints::GetBounds()
{
  this->ComputeBounds();
  return this->Bounds;
}

// Description:
// Return the bounds of the points.
void vtkPoints::GetBounds(float bounds[6])
{
  this->ComputeBounds();
  for (int i=0; i<6; i++) bounds[i] = this->Bounds[i];
}

void vtkPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  float *bounds;

  vtkRefCount::PrintSelf(os,indent);

  os << indent << "Number Of Points: " << this->GetNumberOfPoints() << "\n";
  bounds = this->GetBounds();
  os << indent << "Bounds: \n";
  os << indent << "  Xmin,Xmax: (" << bounds[0] << ", " << bounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << bounds[2] << ", " << bounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << bounds[4] << ", " << bounds[5] << ")\n";
}

