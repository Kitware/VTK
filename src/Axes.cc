/*=========================================================================

  Program:   Visualization Library
  Module:    Axes.cc
  Language:  C++
  Date:      5/16/94
  Version:   1.1

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Axes.hh"
#include "FScalars.hh"
#include "FNormals.hh"

vlAxes::vlAxes()
{
  this->Origin[0] = 0.0;  
  this->Origin[1] = 0.0;  
  this->Origin[2] = 0.0;

  this->ScaleFactor = 1.0;
}

void vlAxes::Execute()
{
  int i;
  int numPts=6, numLines=3;
  vlFloatPoints *newPts;
  vlCellArray *newLines;
  vlFloatScalars *newScalars;
  vlFloatNormals *newNormals;
  float x[3], n[3];
  int ptIds[2];

  this->Initialize();

  newPts = new vlFloatPoints(numPts);
  newLines = new vlCellArray();
  newLines->Allocate(newLines->EstimateSize(numLines,2));
  newScalars = new vlFloatScalars(numPts);
  newNormals = new vlFloatNormals(numPts);
//
// Create axes
//
  n[0] = 0.0; n[1] = 1.0; n[2] = 0.0; 
  ptIds[0] = newPts->InsertNextPoint(this->Origin);
  newScalars->InsertNextScalar(0.0);
  newNormals->InsertNextNormal(n);

  x[0] = this->Origin[0] + this->ScaleFactor;
  x[1] = this->Origin[1];
  x[2] = this->Origin[2];
  ptIds[1] = newPts->InsertNextPoint(x);
  newLines->InsertNextCell(2,ptIds);
  newScalars->InsertNextScalar(0.0);
  newNormals->InsertNextNormal(n);


  n[0] = 0.0; n[1] = 0.0; n[2] = 1.0; 
  ptIds[0] = newPts->InsertNextPoint(this->Origin);
  newScalars->InsertNextScalar(0.25);
  newNormals->InsertNextNormal(n);

  x[0] = this->Origin[0];
  x[1] = this->Origin[1] + this->ScaleFactor;
  x[2] = this->Origin[2];
  ptIds[1] = newPts->InsertNextPoint(x);
  newScalars->InsertNextScalar(0.25);
  newNormals->InsertNextNormal(n);
  newLines->InsertNextCell(2,ptIds);


  n[0] = 1.0; n[1] = 0.0; n[2] = 0.0; 
  ptIds[0] = newPts->InsertNextPoint(this->Origin);
  newScalars->InsertNextScalar(0.5);
  newNormals->InsertNextNormal(n);

  x[0] = this->Origin[0];
  x[1] = this->Origin[1];
  x[2] = this->Origin[2] + this->ScaleFactor;
  ptIds[1] = newPts->InsertNextPoint(x);
  newScalars->InsertNextScalar(0.5);
  newNormals->InsertNextNormal(n);
  newLines->InsertNextCell(2,ptIds);

//
// Update self
// 
  this->SetPoints(newPts);
  this->PointData.SetScalars(newScalars);
  this->PointData.SetNormals(newNormals);
  this->SetLines(newLines);
}

void vlAxes::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlAxes::GetClassName()))
    {
    vlPolySource::PrintSelf(os,indent);

    os << indent << "Origin: (" << this->Origin[0] << ", "
                 << this->Origin[1] << ", "
                 << this->Origin[2] << ")\n";

    os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
    }
}
