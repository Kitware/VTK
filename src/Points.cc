/*=========================================================================

  Program:   Visualization Library
  Module:    Points.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Points.hh"

vlPoints::vlPoints()
{
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = 0.0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1.0;
}

void vlPoints::GetPoint(int id, float x[3])
{
  float *xp = this->GetPoint(id);
  for (int i=0; i<3; i++) x[i] = xp[i];
}

// Description:
// Given a list of pt ids, return an array of point coordinates.
void vlPoints::GetPoints(vlIdList& ptId, vlFloatPoints& fp)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    fp.InsertPoint(i,this->GetPoint(ptId.GetId(i)));
    }
}

// Description:
// Determine (xmin,xmax, ymin,ymax, zmin,zmax) bounds of points.
void vlPoints::ComputeBounds()
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
float *vlPoints::GetBounds()
{
  this->ComputeBounds();
  return this->Bounds;
}

// Description:
// Return the bounds of the points.
void vlPoints::GetBounds(float bounds[6])
{
  this->ComputeBounds();
  for (int i=0; i<6; i++) bounds[i] = this->Bounds[i];
}

void vlPoints::PrintSelf(ostream& os, vlIndent indent)
{
  float *bounds;

  vlRefCount::PrintSelf(os,indent);

  os << indent << "Number Of Points: " << this->GetNumberOfPoints() << "\n";
  bounds = this->GetBounds();
  os << indent << "Bounds: \n";
  os << indent << "  Xmin,Xmax: (" << bounds[0] << ", " << bounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << bounds[2] << ", " << bounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << bounds[4] << ", " << bounds[5] << ")\n";
}

