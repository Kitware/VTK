/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TMap2Box.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "TMap2Box.hh"
#include "vtkMath.hh"
#include "FTCoords.hh"

// Description:
// Construct with r-s-t range=(0,1) and automatic box generation turned on.
vtkTextureMapToBox::vtkTextureMapToBox()
{
  this->Box[0] = this->Box[2] = this->Box[4] = 0.0;
  this->Box[1] = this->Box[3] = this->Box[5] = 1.0;

  this->RRange[0] = 0.0;
  this->RRange[1] = 1.0;

  this->SRange[0] = 0.0;
  this->SRange[1] = 1.0;

  this->TRange[0] = 0.0;
  this->TRange[1] = 1.0;

  this->AutomaticBoxGeneration = 1;
}


void vtkTextureMapToBox::Execute()
{
  float tc[3];
  int numPts;
  vtkFloatTCoords *newTCoords;
  int i, j;
  float *box, *p;
  float min[3], max[3];

  vtkDebugMacro(<<"Generating 3D texture coordinates!");
  this->Initialize();
//
//  Allocate texture data
//
  if ( (numPts=this->Input->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro(<<"No points to texture!");
    return;
    }

  newTCoords = new vtkFloatTCoords(numPts,3);

  if ( this->AutomaticBoxGeneration ) 
    box = this->Input->GetBounds();
  else
    box = this->Box;
//
// Loop over all points generating coordinates
//
  min[0] = RRange[0]; min[1] = SRange[0]; min[2] = TRange[0]; 
  max[0] = RRange[1]; max[1] = SRange[1]; max[2] = TRange[1]; 

  for (i=0; i<numPts; i++) 
    {
    p = this->GetPoint(i);
    for (j=0; j<3; j++) 
      {
      tc[j] = min[j] + (max[j]-min[j]) * (p[j] - box[2*j]) / (box[2*j+1] - box[2*j]);
      if ( tc[j] < min[j] ) tc[j] = min[j];
      if ( tc[j] > max[j] ) tc[j] = max[j];
      }
    newTCoords->SetTCoord(i,tc);
    }
//
// Update ourselves
//
  this->PointData.CopyTCoordsOff();
  this->PointData.PassData(this->Input->GetPointData());

  this->PointData.SetTCoords(newTCoords);
}

// Description:
// Specify the bounding box to map into.
void vtkTextureMapToBox::SetBox(float xmin, float xmax, float ymin, float ymax,
                                float zmin, float zmax)
{
  if ( xmin != this->Box[0] || xmax != this->Box[1] ||
  ymin != this->Box[2] || ymax != this->Box[3] ||
  zmin != this->Box[4] || zmax != this->Box[5] )
    {
    this->Modified();

    this->Box[0] = xmin; this->Box[1] = xmax; 
    this->Box[2] = xmin; this->Box[3] = xmax; 
    this->Box[4] = xmin; this->Box[5] = xmax; 

    for (int i=0; i<3; i++)
      if ( this->Box[2*i] > this->Box[2*i+1] )
         this->Box[2*i] = this->Box[2*i+1];
    }
}

void vtkTextureMapToBox::SetBox(float *bounds)
{
  this->SetBox(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4],
               bounds[5]);
}

void vtkTextureMapToBox::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "R Range: (" << this->RRange[0] << ", "
                               << this->RRange[1] << ")\n";
  os << indent << "S Range: (" << this->SRange[0] << ", "
                               << this->SRange[1] << ")\n";
  os << indent << "T Range: (" << this->TRange[0] << ", "
                               << this->TRange[1] << ")\n";
  os << indent << "Automatic Box Generation: " << 
                  (this->AutomaticBoxGeneration ? "On\n" : "Off\n");
}

