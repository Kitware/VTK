/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BScalars.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include "BScalars.hh"

vtkScalars *vtkBitScalars::MakeObject(int sze, int ext)
{
  return new vtkBitScalars(sze,ext);
}

// Description:
// Deep copy of scalars.
vtkBitScalars& vtkBitScalars::operator=(const vtkBitScalars& cs)
{
  this->S = cs.S;
  return *this;
}

void vtkBitScalars::GetScalars(vtkIdList& ptId, vtkFloatScalars& fs)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    fs.InsertScalar(i,(float)this->S.GetValue(ptId.GetId(i)));
    }
}
