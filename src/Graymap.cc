/*=========================================================================

  Program:   Visualization Library
  Module:    Graymap.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Graymap.hh"

vlScalars *vlGraymap::MakeObject(int sze, int ext)
{
  return new vlGraymap(sze,ext);
}

// Description:
// Deep copy of scalars.
vlGraymap& vlGraymap::operator=(const vlGraymap& fs)
{
  this->S = fs.S;
  return *this;
}

// Description:
// Return a unsigned char gray for a particular point id.
// (Note: gray value converted into full rgb triplet.)
unsigned char *vlGraymap::GetColor(int id)
{
  static unsigned char rgb[3];
  rgb[0] = rgb[1] = rgb[2] = this->S[id];
  return rgb;
}

// Description:
// Copy gray components into user provided array for specified
// point id.
// (Note: gray value converted into full rgb triplet.)
void vlGraymap::GetColor(int id, unsigned char rgb[3])
{
  rgb[0] = rgb[1] = rgb[2] = this->S[id];
}

// Description:
// Insert gray value into object. No range checking performed (fast!).
// (Note: interface varies from superclass vlColorScalars. Only first
// component of rgb[3] (here g[1]) is inserted.)
void vlGraymap::SetColor(int id, unsigned char g[1])
{
  this->S[id] = g[0];
}

// Description:
// Insert gray value into object. Range checking performed and memory
// allocated as necessary.
// (Note: interface varies from superclass vlColorScalars. Only first
// component of rgb[3] (here g[1]) is inserted.)
void vlGraymap::InsertColor(int id, unsigned char g[1])
{
  this->S.InsertValue(id,g[0]);
}

// Description:
// Insert gray value into next available slot. Returns point id of slot.
// (Note: interface varies from superclass vlColorScalars. Only first
// component of rgb[3] (here g[1]) is inserted.)
int vlGraymap::InsertNextColor(unsigned char g[1])
{
  int id = this->S.InsertNextValue(g[0]);
  return id;
}

