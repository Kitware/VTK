/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Bitmap.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Bitmap.hh"

vtkScalars *vtkBitmap::MakeObject(int sze, int ext)
{
  return new vtkBitmap(sze,ext);
}

// Description:
// Deep copy of scalars.
vtkBitmap& vtkBitmap::operator=(const vtkBitmap& fs)
{
  this->S = fs.S;
  return *this;
}

// Description:
// Return a rgba color for a particular point id.
unsigned char *vtkBitmap::GetColor(int id)
{
  static unsigned char rgba[4];
  rgba[0] = rgba[1] = rgba[2] = this->S.GetValue(id);
  rgba[3] = 255;
  return rgba;
}

// Description:
// Get rgba color value for id indicated.
void vtkBitmap::GetColor(int id, unsigned char rgba[4])
{
  rgba[0] = rgba[1] = rgba[2] = this->S.GetValue(id);
  rgba[3] = 255;
}

// Description:
// Insert rgba color value into object. No range checking performed (fast!).
void vtkBitmap::SetColor(int id, unsigned char rgba[4])
{
  unsigned char b=rgba[0]|rgba[1]|rgba[2];
  this->S.SetValue(id,b);
}

// Description:
// Insert rgba color value into object. Range checking performed and memory
// allocated as necessary.
void vtkBitmap::InsertColor(int id, unsigned char rgba[4])
{
  unsigned char b=rgba[0]|rgba[1]|rgba[2];
  this->S.InsertValue(id,b);
}

// Description:
// Insert rgba color value into next available slot. Returns point id of slot.
int vtkBitmap::InsertNextColor(unsigned char rgba[4])
{
  unsigned char b=rgba[0]|rgba[1]|rgba[2];
  int id = this->S.InsertNextValue(b);
  return id;
}

