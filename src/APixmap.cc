/*=========================================================================

  Program:   Visualization Toolkit
  Module:    APixmap.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "APixmap.hh"

vtkScalars *vtkAPixmap::MakeObject(int sze, int ext)
{
  return new vtkAPixmap(sze,ext);
}

// Description:
// Deep copy of scalars.
vtkAPixmap& vtkAPixmap::operator=(const vtkAPixmap& fs)
{
  this->S = fs.S;
  return *this;
}

// Description:
// Copy rgba components into user provided array rgba[4] for specified
// point id. (Note: this interface varies from superclass vtkColorScalars.
// Four values are returned: rgba.)
void vtkAPixmap::GetColor(int id, unsigned char rgba[4])
{
  unsigned char *_rgba;

  _rgba = this->S.GetPtr(4*id);
  rgba[0] = _rgba[0];
  rgba[1] = _rgba[1];
  rgba[2] = _rgba[2];
  rgba[3] = _rgba[3];
}

// Description:
// Insert color into object. No range checking performed (fast!).
// (Note: this interface varies from superclass vtkColorScalars.
// Four values are inserted: rgba.)
void vtkAPixmap::SetColor(int id, unsigned char rgba[4])
{
  id *= 4;
  for(int j=0; j<4; j++) this->S[id+j] = rgba[j];
}

// Description:
// Insert color into object. Range checking performed and memory
// allocated as necessary.
// (Note: this interface varies from superclass vtkColorScalars.
// Four values are inserted: rgba.)
void vtkAPixmap::InsertColor(int id, unsigned char rgba[4])
{
  id *= 4;
  for(int j=0; j<4; j++) this->S.InsertValue(id+j,rgba[j]);
}

// Description:
// Insert color into next available slot. Returns point id of slot.
// (Note: this interface varies from superclass vtkColorScalars.
// Four values are inserted: rgba.)
int vtkAPixmap::InsertNextColor(unsigned char rgba[4])
{
  int id = this->S.InsertNextValue(rgba[0]);
  for(int j=1; j<4; j++) this->S.InsertNextValue(rgba[j]);
  return id/4;
}

