/*=========================================================================

  Program:   Visualization Toolkit
  Module:    FPoints.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "FPoints.hh"

vtkPoints *vtkFloatPoints::MakeObject(int sze, int ext)
{
  return new vtkFloatPoints(sze,ext);
}

// Description:
// Deep copy of points.
vtkFloatPoints& vtkFloatPoints::operator=(const vtkFloatPoints& fp)
{
  this->P = fp.P;
  return *this;
}

void vtkFloatPoints::GetPoints(vtkIdList& ptId, vtkFloatPoints& fp)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    fp.InsertPoint(i,this->P.GetPtr(3*ptId.GetId(i)));
    }
}
