/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UCScalar.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "UCScalar.hh"

vtkScalars *vtkUnsignedCharScalars::MakeObject(int sze, int ext)
{
  return new vtkUnsignedCharScalars(sze,ext);
}

// Description:
// Deep copy of scalars.
vtkUnsignedCharScalars& vtkUnsignedCharScalars::operator=(const vtkUnsignedCharScalars& cs)
{
  this->S = cs.S;
  return *this;
}

void vtkUnsignedCharScalars::GetScalars(vtkIdList& ptId, vtkFloatScalars& fs)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    fs.InsertScalar(i,(float)this->S.GetValue(ptId.GetId(i)));
    }
}
