/*=========================================================================

  Program:   Visualization Library
  Module:    SpherSrc.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Methods for Sphere generator
//
#include <math.h>
#include "SpherSrc.hh"
#include "FPoints.hh"
#include "FNormals.hh"
#include "vlMath.hh"

vlSphereSource::vlSphereSource(int res)
{
  res = res < 4 ? 4 : res;
  this->Radius = 0.5;
  this->ThetaResolution = res;
  this->PhiResolution = res;
}

void vlSphereSource::Execute()
{
  int i, j;
  int numPts;
  int numPolys;
  vlFloatPoints *newPoints; 
  vlFloatNormals *newNormals;
  vlCellArray *newPolys;
  float x[3], deltaPhi, deltaTheta, phi, theta, radius, norm;
  int pts[3], base;
  vlMath math;
//
// Set things up; allocate memory
//
  this->Initialize();

  numPts = (this->PhiResolution - 1) * this->ThetaResolution + 2;
  // creating triangles
  numPolys = (this->PhiResolution - 1) * 2 * this->ThetaResolution;

  newPoints = new vlFloatPoints(numPts);
  newNormals = new vlFloatNormals(numPts);
  newPolys = new vlCellArray;
  newPolys->Allocate(newPolys->EstimateSize(numPolys,3));
//
// Create sphere
//
  // Create north pole
  x[0] = x[1] = 0.0; x[2] = this->Radius;
  newPoints->InsertPoint(0,x);
  x[2] = 1.0;
  newNormals->InsertNormal(0,x);

  // Create south pole
  x[0] = x[1] = 0.0; x[2] = -(this->Radius);
  newPoints->InsertPoint(1,x);
  x[2] = -1.0;
  newNormals->InsertNormal(1,x);

  // Create intermediate points
  deltaPhi = math.Pi() / this->PhiResolution;
  deltaTheta = 2.0 * math.Pi() / this->ThetaResolution;
  for (i=0; i < this->ThetaResolution; i++)
    {
    theta = i * deltaTheta;
    for (j=1; j < this->PhiResolution; j++)
      {
      phi = j * deltaPhi;
      radius = this->Radius * sin((double)phi);
      x[0] = radius * cos((double)theta);
      x[1] = radius * sin((double)theta);
      x[2] = this->Radius * cos((double)phi);
      newPoints->InsertNextPoint(x);

      if ( (norm = math.Norm(x)) == 0.0 ) norm = 1.0;
      x[0] /= norm; x[1] /= norm; x[2] /= norm; 
      newNormals->InsertNextNormal(x);
      }
    }
//
// Generate mesh connectivity
//
  base = (this->PhiResolution - 1) * this->ThetaResolution;
  for (i=0; i < this->ThetaResolution; i++)
    {
    // around north pole
    pts[0] = (this->PhiResolution-1)*i + 2;
    pts[1] = (((this->PhiResolution-1)*(i+1)) % base) + 2;
    pts[2] = 0;
    newPolys->InsertNextCell(3,pts);

    // around south pole
    pts[0] = pts[0] + this->PhiResolution - 2;
    pts[2] = pts[1] + this->PhiResolution - 2;
    pts[1] = 1;
    newPolys->InsertNextCell(3,pts);
    }

  // bands inbetween poles
  for (i=0; i < this->ThetaResolution; i++)
    {
    for (j=0; j < (this->PhiResolution-2); j++)
      {
      pts[0] = 2 + (this->PhiResolution-1)*i + j;
      pts[1] = pts[0] + 1;
      pts[2] = (((this->PhiResolution-1)*(i+1)+j) % base) + 3;
      newPolys->InsertNextCell(3,pts);

      pts[1] = pts[2];
      pts[2] = pts[1] - 1;
      newPolys->InsertNextCell(3,pts);
      }
    }
//
// Update ourselves
//
  this->SetPoints(newPoints);
  this->PointData.SetNormals(newNormals);
  this->SetPolys(newPolys);
}

void vlSphereSource::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlSphereSource::GetClassName()))
    {
    vlPolySource::PrintSelf(os,indent);

    os << indent << "Theta Resolution: " << this->ThetaResolution << "\n";
    os << indent << "Phi Resolution: " << this->PhiResolution << "\n";
    os << indent << "Radius: " << this->Radius << "\n";
    }
}
