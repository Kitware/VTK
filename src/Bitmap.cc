/*=========================================================================

  Program:   Visualization Library
  Module:    Bitmap.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Bitmap.hh"

vlScalars *vlBitmap::MakeObject(int sze, int ext)
{
  return new vlBitmap(sze,ext);
}

// Description:
// Deep copy of scalars.
vlBitmap& vlBitmap::operator=(const vlBitmap& fs)
{
  this->S = fs.S;
  return *this;
}

// Description:
// Return a color for a particular point id.
// (Note: gray value converted into full rgb triplet.)
unsigned char *vlBitmap::GetColor(int id)
{
  static unsigned char rgb[3];
  rgb[0] = rgb[1] = rgb[2] = this->S.GetValue(id);
  return rgb;
}

// Description:
// Copy gray components into user provided array for specified
// point id.
// (Note: gray value converted into full rgb triplet.)
void vlBitmap::GetColor(int id, unsigned char rgb[3])
{
  rgb[0] = rgb[1] = rgb[2] = this->S.GetValue(id);
}

// Description:
// Insert gray value into object. No range checking performed (fast!).
// (Note: interface varies from superclass vlColorScalars. Only first
// component of rgb[3] (here b[1]) is inserted.)
void vlBitmap::SetColor(int id, unsigned char b[1])
{
  this->S.SetValue(id,b[0]);
}

// Description:
// Insert gray value into object. Range checking performed and memory
// allocated as necessary.
// (Note: interface varies from superclass vlColorScalars. Only first
// component of rgb[3] (here b[1]) is inserted.)
void vlBitmap::InsertColor(int id, unsigned char b[1])
{
  this->S.InsertValue(id,b[0]);
}

// Description:
// Insert gray value into next available slot. Returns point id of slot.
// (Note: interface varies from superclass vlColorScalars. Only first
// component of rgb[3] (here b[1]) is inserted.)
int vlBitmap::InsertNextColor(unsigned char b[1])
{
  int id = this->S.InsertNextValue(b[0]);
  return id;
}

