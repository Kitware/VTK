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
// Return a unsigned char rgba color value for a particular point id.
unsigned char *vlAGraymap::GetColor(int id)
{
  static unsigned char rgba[4];
  rgba[0] = rgba[1] = rgba[2] = this->S[2*id];
  rgba[3] = this->S[2*id+1];
  
  return rgba;
}

// Description:
// Copy rgba color value components into user provided array for specified
// point id.
void vlAGraymap::GetColor(int id, unsigned char rgba[3])
{
  rgba[0] = rgba[1] = rgba[2] = this->S[2*id];
  rgba[3] = this->S[2*id+1];
}

// Description:
// Deep copy of scalars.
vlAGraymap& vlAGraymap::operator=(const vlAGraymap& fs)
{
  this->S = fs.S;
  return *this;
}
