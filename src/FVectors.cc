/*=========================================================================

  Program:   Visualization Toolkit
  Module:    FVectors.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "FVectors.hh"

vtkVectors *vtkFloatVectors::MakeObject(int sze, int ext)
{
  return new vtkFloatVectors(sze,ext);
}

// Description:
// Deep copy of vectors.
vtkFloatVectors& vtkFloatVectors::operator=(const vtkFloatVectors& fv)
{
  this->V = fv.V;
  return *this;
}

