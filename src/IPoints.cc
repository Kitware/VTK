/*=========================================================================

  Program:   Visualization Toolkit
  Module:    IPoints.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "IPoints.hh"

vtkPoints *vtkIntPoints::MakeObject(int sze, int ext)
{
  return new vtkIntPoints(sze,ext);
}

// Description:
// Deep copy of points.
vtkIntPoints& vtkIntPoints::operator=(const vtkIntPoints& fp)
{
  this->P = fp.P;
  return *this;
}

float *vtkIntPoints::GetPoint(int i)
{
  static float x[3];
  int *iptr = this->P.GetPtr(3*i);
  x[0] = (float)iptr[0]; x[1] = (float)iptr[1]; x[2] = (float)iptr[2];
  return x;
};

void vtkIntPoints::GetPoints(vtkIdList& ptId, vtkFloatPoints& fp)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    fp.InsertPoint(i,this->GetPoint(ptId.GetId(i)));
    }
}
