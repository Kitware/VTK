/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SScalars.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "SScalars.hh"

vtkScalars *vtkShortScalars::MakeObject(int sze, int ext)
{
  return new vtkShortScalars(sze,ext);
}

// Description:
// Deep copy of scalars.
vtkShortScalars& vtkShortScalars::operator=(const vtkShortScalars& ss)
{
  this->S = ss.S;
  return *this;
}

void vtkShortScalars::GetScalars(vtkIdList& ptId, vtkFloatScalars& fs)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    fs.InsertScalar(i,(float)this->S.GetValue(ptId.GetId(i)));
    }
}
