/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CoScalar.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "CoScalar.hh"
#include "Lut.hh"

// Description:
// Use intensity to derive single scalar value from color
float vtkColorScalars::GetScalar(int i)
{
  unsigned char *rgba;

  rgba  = this->GetColor(i);
  return (float) ((rgba[0] > rgba[1] ? (rgba[0] > rgba[2] ? rgba[0] : rgba[2]) : (rgba[1] > rgba[2] ? rgba[1] : rgba[2])) / 255.0);
}

// Description:
// Map through lookup table to set the color
void vtkColorScalars::SetScalar(int i, float s)
{
  if ( this->LookupTable == NULL ) this->CreateDefaultLookupTable();

  this->SetColor(i,this->LookupTable->MapValue(s));
}

// Description:
// Map through lookup table  to set the color
void vtkColorScalars::InsertScalar(int i, float s)
{
  if ( this->LookupTable == NULL ) this->CreateDefaultLookupTable();

  this->InsertColor(i,this->LookupTable->MapValue(s));
}

// Description:
// Map through lookup table to set the color
int vtkColorScalars::InsertNextScalar(float s)
{
  if ( this->LookupTable == NULL ) this->CreateDefaultLookupTable();

  return this->InsertNextColor(this->LookupTable->MapValue(s));
}

// Description:
// Given list of point id's, return colors for each point.
void vtkColorScalars::GetColors(vtkIdList& ptId, vtkAPixmap& p)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    p.InsertColor(i,this->GetColor(ptId.GetId(i)));
    }
}

// Description:
// Compute range of color rgba data (rmin,rmax, gmin,gmax, bmin,bmax, 
// amin,amax). Return pointer to array of length 8.
unsigned char *vtkColorScalars::GetComponentRange ()
{
  unsigned char *rgba;
  int i, j;
  static unsigned char range[8];

  range[0] = range[2] = range[4] = range[6] = 255;
  range[1] = range[3] = range[5] = range[7] = 0;

  for (i=0; i<this->GetNumberOfColors(); i++)
    {
    rgba = this->GetColor(i);
    for (j=0; j<4; j++)
      {
      if ( rgba[j] < range[2*j] ) range[2*j] = rgba[j];
      if ( rgba[j] > range[2*j+1] ) range[2*j+1] = rgba[j];
      }
    }
  return range;
}

// Description:
// Compute range of color rgba data (rmin,rmax, gmin,gmax, bmin,bmax, 
// amin,amax). Copy result into user provided array.
void vtkColorScalars::GetComponentRange(unsigned char range[8])
{
  unsigned char *r=this->GetComponentRange();
  for (int i=0; i < 8; i++) range[i] = r[i];
}

