/*=========================================================================

  Program:   Visualization Library
  Module:    FNormals.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "FNormals.hh"

vlNormals *vlFloatNormals::MakeObject(int sze, int ext)
{
  return new vlFloatNormals(sze,ext);
}

// Description:
// Deep copy of normals.
vlFloatNormals& vlFloatNormals::operator=(const vlFloatNormals& fn)
{
  this->N = fn.N;
  return *this;
}

