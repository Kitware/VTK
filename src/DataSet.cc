/*=========================================================================

  Program:   Visualization Library
  Module:    DataSet.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// DataSet methods
//
#include <math.h>
#include "DataSet.hh"

vlDataSet::vlDataSet ()
{
  this->Bounds[0] = 0.0;
  this->Bounds[1] = 1.0;
  this->Bounds[2] = 0.0;
  this->Bounds[3] = 1.0;
  this->Bounds[4] = 0.0;
  this->Bounds[5] = 1.0;

  this->Mapper = 0;
}

void vlDataSet::Initialize()
{
  this->PointData.Initialize();
  this->Modified();
};

void vlDataSet::ComputeBounds()
{
  int i, j;
  float *x;

  if ( this->GetMtime() > this->ComputeTime )
    {
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] =  LARGE_FLOAT;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -LARGE_FLOAT;
    for (i=0; i<this->NumPoints(); i++)
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

float *vlDataSet::GetBounds()
{
  this->ComputeBounds();
  return this->Bounds;
}
  
float *vlDataSet::GetCenter()
{
  static float center[3];

  this->ComputeBounds();
  for (int i=0; i<3; i++) 
    center[i] = (this->Bounds[2*i+1] + this->Bounds[2*i]) / 2.0;
  return center;
}

float vlDataSet::GetLength()
{
  double diff, l=0.0;
  int i;

  this->ComputeBounds();
  for (i=0; i<3; i++)
    {
    diff = this->Bounds[2*i+1] - this->Bounds[2*i];
    l += diff * diff;
    }
 
  return (float)sqrt(l);
}

unsigned long int vlDataSet::GetMtime()
{
  if ( this->PointData.GetMtime() > this->Mtime ) return this->PointData.GetMtime();
  else return this->Mtime;
}

void vlDataSet::PrintSelf(ostream& os, vlIndent indent)
{
  float *bounds;

  vlObject::PrintSelf(os,indent);

  os << indent << "Point Data:\n";
  this->PointData.PrintSelf(os,indent.GetNextIndent());
  bounds = this->GetBounds();
  os << indent << "Bounds: \n";
  os << indent << "  Xmin,Xmax: (" << bounds[0] << ", " << bounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << bounds[2] << ", " << bounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << bounds[4] << ", " << bounds[5] << ")\n";
  os << indent << "Compute time: " << this->ComputeTime.GetMtime() << "\n";

}
