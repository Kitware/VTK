/*=========================================================================

  Program:   Visualization Library
  Module:    Glyph3D.cc
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
#include "Glyph3D.hh"

vlGlyph3D::vlGlyph3D()
{
  this->Source = NULL;
  this->Scaling = 1;
  this->ScaleMode = SCALE_BY_SCALAR;
  this->ScaleFactor = 1.0;
  this->Range[0] = 0.0;
  this->Range[1] = 1.0;
  this->Orient = 1;
  this->OrientMode = ORIENT_BY_VECTOR;
}

vlGlyph3D::~vlGlyph3D()
{
  if (this->Source)
    {
    this->Source->UnRegister(this);
    }
}

void vlGlyph3D::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlGlyph3D::GetClassName()))
    {
    vlDataSetToPolyFilter::PrintSelf(os,indent);

    os << indent << "Source: " << this->Source << "\n";
    os << indent << "Scaling: " << (this->Scaling ? "On\n" : "Off\n");
    os << indent << "Scale Mode: " << (this->ScaleMode == SCALE_BY_SCALAR ? "Scale by scalar\n" : "Scale by vector\n");
    os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
    os << indent << "Range: (" << this->Range[0] << ", " << this->Range[1] << ")\n";
    os << indent << "Orient: " << (this->Orient ? "On\n" : "Off\n");



    os << indent << "Orient Mode: " << (this->OrientMode == ORIENT_BY_VECTOR ? "Orient by vector\n" : "Orient by normal\n");
    }
}

void vlGlyph3D::Execute()
{
  vlPointData *pd;
  vlScalars *inScalars;
  vlVectors *inVectors;
  int numPts;
  int i;
  vlPoints *sourcePts;
  vlCellArray *sourceVerts;  
  vlCellArray *sourceLines;  
  vlCellArray *sourcePolys;  
  vlCellArray *sourceStrips;  
  vlFloatPoints *newPts;
  vlCellArray *newVerts;
  vlCellArray *newLines;
  vlCellArray *newPolys;
  vlCellArray *newStrips;
  float *x, s, *v;
//
// Initialize
//
  this->Initialize();

  pd = this->Input->GetPointData();
  inScalars = pd->GetScalars();
  inVectors = pd->GetVectors();

  numPts = this->Input->GetNumberOfPoints();

  if ( ! this->Source )
    {
    vlErrorMacro(<< "No data to copy\n");
    return;
    }
  else
    {
    this->Source->Update();
    }
//
// Allocate storage for output PolyData
//
  sourcePts = this->Source->GetPoints();
  sourceVerts = this->Source->GetVerts();
  sourceLines = this->Source->GetLines();
  sourcePolys = this->Source->GetPolys();
  sourceStrips = this->Source->GetStrips();

  newPts = new vlFloatPoints(numPts*sourcePts->GetNumberOfPoints());
  newVerts = new vlCellArray(numPts*sourceVerts->GetSize());
  newLines = new vlCellArray(numPts*sourceLines->GetSize());
  newPolys = new vlCellArray(numPts*sourcePolys->GetSize());
  newStrips = new vlCellArray(numPts*sourceStrips->GetSize());

//
// Traverse all points, copying data to point
//

  for (i=0; i<numPts; i++)
    {
    x = this->Input->GetPoint(i);
    if (inScalars) s = inScalars->GetScalar(i);
    if (inVectors) v = inVectors->GetVector(i);


    }
//
// Update ourselves
//
  this->SetPoints(newPts);

  this->SetVerts(newVerts);
  this->SetLines(newLines);
  this->SetPolys(newPolys);
  this->SetStrips(newStrips);
  
}

//
// Override update method because execution can branch two ways (Input 
// and Source)
//
void vlGlyph3D::Update()
{
  // make sure input is available
  if ( !this->Input )
    {
    vlErrorMacro(<< "No input!\n");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  this->Source->Update();
  this->Updating = 0;

  if (this->Input->GetMTime() > this->GetMTime() || this->GetMTime() > this->ExecuteTime )
    {
    if ( this->StartMethod ) (*this->StartMethod)();
    this->Execute();
    this->ExecuteTime.Modified();
    if ( this->EndMethod ) (*this->EndMethod)();
    }
}
