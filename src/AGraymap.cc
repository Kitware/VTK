/*=========================================================================

  Program:   Visualization Library
  Module:    AGraymap.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "AGraymap.hh"

vlScalars *vlAGraymap::MakeObject(int sze, int ext)
{
  return new vlAGraymap(sze,ext);
}

// Description:
// Return a unsigned char gray for a particular point id.
// (Note: gray value converted into full rgb triplet.)
unsigned char *vlAGraymap::GetColor(int id)
{
  static unsigned char rgb[3];
  rgb[0] = rgb[1] = rgb[2] = this->S[2*id];
  return rgb;
}

// Description:
// Copy gray components into user provided array for specified
// point id.
// (Note: gray value converted into full rgb triplet.)
void vlAGraymap::GetColor(int id, unsigned char rgb[3])
{
  rgb[0] = rgb[1] = rgb[2] = this->S[2*id];
}

// Description:
// Deep copy of scalars.
vlAGraymap& vlAGraymap::operator=(const vlAGraymap& fs)
{
  this->S = fs.S;
  return *this;
}
