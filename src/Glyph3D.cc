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
  this->Source = 0;
  this->Scaling = 1;
  this->ScaleMode = SCALE_BY_SCALAR;
  this->ScaleFactor = 1.0;
  this->Range[0] = 0.0;
  this->Range[0] = 1.0;
}

vlGlyph3D::~vlGlyph3D()
{
  if (this->Source)
    {
    this->Source->UnRegister((void *)this);
    }
}

void vlGlyph3D::PrintSelf(ostream& os, vlIndent indent)
{

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

