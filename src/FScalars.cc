/*=========================================================================

  Program:   Visualization Toolkit
  Module:    FScalars.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "FScalars.hh"

vtkScalars *vtkFloatScalars::MakeObject(int sze, int ext)
{
  return new vtkFloatScalars(sze,ext);
}

// Description:
// Deep copy of scalars.
vtkFloatScalars& vtkFloatScalars::operator=(const vtkFloatScalars& fs)
{
  this->S = fs.S;
  return *this;
}

void vtkFloatScalars::GetScalars(vtkIdList& ptId, vtkFloatScalars& fs)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    fs.InsertScalar(i,this->S.GetValue(ptId.GetId(i)));
    }
}
