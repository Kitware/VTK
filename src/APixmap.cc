/*=========================================================================

  Program:   Visualization Library
  Module:    APixmap.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "APixmap.hh"

vlScalars *vlAPixmap::MakeObject(int sze, int ext)
{
  return new vlAPixmap(sze,ext);
}

// Description:
// Deep copy of scalars.
vlAPixmap& vlAPixmap::operator=(const vlAPixmap& fs)
{
  this->S = fs.S;
  return *this;
}

// Description:
// Return a unsigned char rgba for a particular point id.
// (Note: this interface varies from superclass vlColorScalars.
// A pointer to four values are returned: rgba.)
unsigned char *vlAPixmap::GetColor(int id)
{
  return this->S.GetPtr(4*id);
}

// Description:
// Copy rgba components into user provided array rgba[4] for specified
// point id. (Note: this interface varies from superclass vlColorScalars.
// Four values are returned: rgba.)
void vlAPixmap::GetColor(int id, unsigned char rgba[4])
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
// (Note: this interface varies from superclass vlColorScalars.
// Four values are inserted: rgba.)
void vlAPixmap::SetColor(int id, unsigned char rgba[4])
{
  id *= 4;
  for(int j=0; j<4; j++) this->S[id+j] = rgba[j];
}

// Description:
// Insert color into object. Range checking performed and memory
// allocated as necessary.
// (Note: this interface varies from superclass vlColorScalars.
// Four values are inserted: rgba.)
void vlAPixmap::InsertColor(int id, unsigned char rgba[4])
{
  id *= 4;
  for(int j=0; j<4; j++) this->S.InsertValue(id+j,rgba[j]);
}

// Description:
// Insert color into next available slot. Returns point id of slot.
// (Note: this interface varies from superclass vlColorScalars.
// Four values are inserted: rgba.)
int vlAPixmap::InsertNextColor(unsigned char rgba[4])
{
  int id = this->S.InsertNextValue(rgba[0]);
  for(int j=1; j<4; j++) this->S.InsertNextValue(rgba[j]);
  return id/4;
}

