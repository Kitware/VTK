/*=========================================================================

  Program:   Visualization Toolkit
  Module:    IScalars.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "IScalars.hh"

vtkScalars *vtkIntScalars::MakeObject(int sze, int ext)
{
  return new vtkIntScalars(sze,ext);
}

// Description:
// Deep copy of scalars.
vtkIntScalars& vtkIntScalars::operator=(const vtkIntScalars& is)
{
  this->S = is.S;
  return *this;
}

void vtkIntScalars::GetScalars(vtkIdList& ptId, vtkFloatScalars& fs)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    fs.InsertScalar(i,(float)this->S.GetValue(ptId.GetId(i)));
    }
}
