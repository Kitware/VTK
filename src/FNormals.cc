/*=========================================================================

  Program:   Visualization Toolkit
  Module:    FNormals.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "FNormals.hh"

vtkNormals *vtkFloatNormals::MakeObject(int sze, int ext)
{
  return new vtkFloatNormals(sze,ext);
}

// Description:
// Deep copy of normals.
vtkFloatNormals& vtkFloatNormals::operator=(const vtkFloatNormals& fn)
{
  this->N = fn.N;
  return *this;
}

