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
// Deep copy of scalars.
vlAGraymap& vlAGraymap::operator=(const vlAGraymap& fs)
{
  this->S = fs.S;
  return *this;
}
