/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PlaneSrc.cc
  Language:  C++
  Date:      5/15/94
  Version:   1.12

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "PlaneSrc.hh"
#include "FPoints.hh"
#include "FNormals.hh"
#include "FTCoords.hh"

// Description:
// Set the number of x-y subdivisions in the plane.
void vtkPlaneSource::SetResolution(const int xR, const int yR)
{
  if ( xR != this->XRes || yR != this->YRes )
  {
    this->XRes = xR;
    this->YRes = yR;

    this->XRes = (this->XRes > 0 ? this->XRes : 1);
    this->YRes = (this->YRes > 0 ? this->YRes : 1);

    this->Modified();
  }
}

void vtkPlaneSource::Execute()
{
  float x[3], tc[2], n[3], xinc, yinc;
  int pts[MAX_CELL_SIZE];
  int i, j;
  int numPts;
  int numPolys;
  vtkFloatPoints *newPoints; 
  vtkFloatNormals *newNormals;
  vtkFloatTCoords *newTCoords;
  vtkCellArray *newPolys;
//
// Set things up; allocate memory
//
  Initialize();

  numPts = (this->XRes+1) * (this->YRes+1);
  numPolys = this->XRes * this->YRes;

  newPoints = new vtkFloatPoints(numPts);
  newNormals = new vtkFloatNormals(numPts);
  newTCoords = new vtkFloatTCoords(numPts,2);

  newPolys = new vtkCellArray;
  newPolys->Allocate(newPolys->EstimateSize(numPolys,4));
//
// Generate points and point data
//
  xinc = 1.0 / ((float)this->XRes);
  yinc = 1.0 / ((float)this->YRes);
  x[2] = 0.0; // z-value
  n[0] = 0.0; n[1] = 0.0; n[2] = 1.0;

  for (numPts=0, i=0; i<(this->YRes+1); i++)
    {
    x[1] = -0.5 + i*yinc;
    tc[1] = (float) i / this->YRes;
    for (j=0; j<(this->XRes+1); j++)
      {
      x[0] = -0.5 + j*xinc;
      tc[0] = (float) j / this->XRes;

      newPoints->InsertPoint(numPts,x);
      newTCoords->InsertTCoord(numPts,tc);
      newNormals->InsertNormal(numPts++,n);
      }
    }
//
// Generate polygons
//
  for (i=0; i<this->YRes; i++)
    {
    x[1] = tc[1] = i*yinc;
    for (j=0; j<this->XRes; j++)
      {
      pts[0] = j + i*(this->XRes+1);
      pts[1] = pts[0] + 1;
      pts[2] = pts[0] + this->XRes + 2;
      pts[3] = pts[0] + this->XRes + 1;
      newPolys->InsertNextCell(4,pts);
      }
    }
//
// Update ourselves and release memory
//
  this->SetPoints(newPoints);
  newPoints->Delete();

  this->PointData.SetNormals(newNormals);
  newNormals->Delete();

  this->PointData.SetTCoords(newTCoords);
  newTCoords->Delete();

  this->SetPolys(newPolys);
  newPolys->Delete();
}

void vtkPlaneSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolySource::PrintSelf(os,indent);

  os << indent << "Resolution: (" << this->XRes << " by " << this->YRes << ")\n";
}
