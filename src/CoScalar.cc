/*=========================================================================

  Program:   Visualization Library
  Module:    CoScalar.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "CoScalar.hh"
#include "Lut.hh"

// Description:
// Use intensity to derive single scalar value from color
float vlColorScalars::GetScalar(int i)
{
  unsigned char *rgba;

  rgba  = this->GetColor(i);
  return (float) ((rgba[0] > rgba[1] ? (rgba[0] > rgba[2] ? rgba[0] : rgba[2]) : (rgba[1] > rgba[2] ? rgba[1] : rgba[2])) / 255.0);
}

// Description:
// Map through lookup table to set the color
void vlColorScalars::SetScalar(int i, float s)
{
  if ( this->LookupTable == NULL ) this->CreateDefaultLookupTable();

  this->SetColor(i,this->LookupTable->MapValue(s));
}

// Description:
// Map through lookup table  to set the color
void vlColorScalars::InsertScalar(int i, float s)
{
  if ( this->LookupTable == NULL ) this->CreateDefaultLookupTable();

  this->InsertColor(i,this->LookupTable->MapValue(s));
}

// Description:
// Map through lookup table to set the color
int vlColorScalars::InsertNextScalar(float s)
{
  if ( this->LookupTable == NULL ) this->CreateDefaultLookupTable();

  this->InsertNextColor(this->LookupTable->MapValue(s));
}

// Description:
// Given list of point id's, return colors for each point.
void vlColorScalars::GetColors(vlIdList& ptId, vlAPixmap& p)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    p.InsertColor(i,this->GetColor(ptId.GetId(i)));
    }
}

