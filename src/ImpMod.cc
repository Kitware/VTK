/*=========================================================================

  Program:   Visualization Library
  Module:    ImpMod.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "ImpMod.hh"
#include "FScalars.hh"

vlImplicitModeller::vlImplicitModeller()
{
  this->MaximumDistance = 0.1;

  this->ModelBounds[0] = 0.0;
  this->ModelBounds[1] = 0.0;
  this->ModelBounds[2] = 0.0;
  this->ModelBounds[3] = 0.0;
  this->ModelBounds[4] = 0.0;
  this->ModelBounds[5] = 0.0;

  this->Dimension[0] = 50;
  this->Dimension[1] = 50;
  this->Dimension[2] = 50;
}

void vlImplicitModeller::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlImplicitModeller::GetClassName()))
    {
    vlDataSetToStructuredPointsFilter::PrintSelf(os,indent);

    os << indent << "Maximum Distance: " << this->MaximumDistance << "\n";
    os << indent << "ModelBounds: \n";
    os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ")\n";
    os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ")\n";
    os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] << ", " << this->ModelBounds[5] << ")\n";
    }
}

void vlImplicitModeller::SetModelBounds(float *bounds)
{
  vlImplicitModeller::SetModelBounds(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
}

void vlImplicitModeller::SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
{
  if (this->ModelBounds[0] != xmin || this->ModelBounds[1] != xmax ||
  this->ModelBounds[2] != ymin || this->ModelBounds[3] != ymax ||
  this->ModelBounds[4] != zmin || this->ModelBounds[5] != zmax )
    {
    float length;

    this->Modified();
    this->ModelBounds[0] = xmin;
    this->ModelBounds[1] = xmax;
    this->ModelBounds[2] = ymin;
    this->ModelBounds[3] = ymax;
    this->ModelBounds[4] = zmin;
    this->ModelBounds[5] = zmax;

    this->Origin[0] = xmin;
    this->Origin[1] = ymin;
    this->Origin[2] = zmin;

    if ( (length = xmin - xmax) == 0.0 ) length = 1.0;
    this->AspectRatio[0] = 1.0;
    this->AspectRatio[1] = (ymax - ymin) / length;
    this->AspectRatio[2] = (zmax - zmin) / length;
    }
}

void vlImplicitModeller::Execute()
{
  int cellNum, i, j, k;
  float *bounds, adjBounds[6];
  vlCell *cell;
  float maxDistance, pcoords[3];
  vlFloatScalars *newScalars;
  int numPts, idx;
  int subId;
  int min[3], max[3];
  float x[3], prevDistance, distance;
  int jkFactor;
//
// Initialize self; create output objects
//
  this->Initialize();

  if ( this->Dimension[0] <= 1 || this->Dimension[1] <= 1 ||
  this->Dimension[2] <= 1 )
    {
    vlErrorMacro(<<"Bad dimensions, requires volume cells");
    return;
    }

  numPts = this->Dimension[0] * this->Dimension[1] * this->Dimension[2];
  newScalars = new vlFloatScalars(numPts);

  maxDistance = this->ComputeModelBounds();
//
// Traverse all cells; computing distance function on volume points.
//
  for (cellNum=0; cellNum < this->Input->NumberOfCells(); cellNum++)
    {
    cell = this->Input->GetCell(cellNum);
    bounds = cell->GetBounds();
    for (i=0; i<3; i++)
      {
      adjBounds[2*i] = bounds[2*i] - maxDistance;
      adjBounds[2*i+1] = bounds[2*i+1] + maxDistance;
      }

    // compute dimensional bounds in data set
    for (i=0; i<3; i++)
      {
      min[i] = (adjBounds[2*i] - this->Origin[i]) / this->AspectRatio[i];
      max[i] = (adjBounds[2*i+1] - this->Origin[i]) / this->AspectRatio[i];
      if (min[i] < 0) min[i] = 0;
      if (max[i] >= this->Dimension[i]) max[i] = this->Dimension[i] - 1;
      }

    jkFactor = this->Dimension[0]*this->Dimension[1];
    for (k = min[2]; k <= max[k]; k++) 
      {
      x[2] = this->AspectRatio[2] * k + this->Origin[2];
      for (j = min[1]; j <= max[1]; j++)
        {
        x[1] = this->AspectRatio[1] * j + this->Origin[1];
        for (i = min[0]; i <= max[0]; i++) 
          {
          x[0] = this->AspectRatio[0] * i + this->Origin[0];
          idx = jkFactor*k + this->Dimension[0]*j + i;
          prevDistance = newScalars->GetScalar(idx);
          distance = cell->EvaluatePosition(x,subId,pcoords);
          if (distance < prevDistance)
            newScalars->SetScalar(idx,distance);
          }
        }
      }
    }
//
// Update self
//
  this->PointData.SetScalars(newScalars);

}

float vlImplicitModeller::ComputeModelBounds()
{
  return 1.0;
}
