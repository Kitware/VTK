/*=========================================================================

  Program:   Visualization Library
  Module:    Pixmap.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Pixmap.hh"

vlScalars *vlPixmap::MakeObject(int sze, int ext)
{
  return new vlPixmap(sze,ext);
}

// Description:
// Deep copy of scalars.
vlPixmap& vlPixmap::operator=(const vlPixmap& fs)
{
  this->S = fs.S;
  return *this;
}

// Description:
// Return a rgba color at array location i.
unsigned char *vlPixmap::GetColor(int i) 
{
  static unsigned char rgba[4];
  unsigned char *_rgb;
  _rgb = this->S.GetPtr(3*i);
  rgba[0] = _rgb[0];
  rgba[1] = _rgb[1];
  rgba[2] = _rgb[2];
  rgba[3] = 255;

  return rgba;
}

// Description:
// Copy rgba components into user provided array rgb[4] for specified
// point id. 
void vlPixmap::GetColor(int id, unsigned char rgba[4])
{
  unsigned char *_rgb;

  _rgb = this->S.GetPtr(3*id);
  rgba[0] = _rgb[0];
  rgba[1] = _rgb[1];
  rgba[2] = _rgb[2];
  rgba[3] = 255;
}
