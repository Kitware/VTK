/*=========================================================================

  Program:   Visualization Toolkit
  Module:    FTensors.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "FTensors.hh"

vtkTensor *vtkFloatTensors::GetTensor(int i) 
{
  static vtkTensor t;
  t.SetDimension(this->Dimension);

  t.T = this->T.GetPtr(this->Dimension*this->Dimension*i);
  return &t;
}

vtkTensors *vtkFloatTensors::MakeObject(int sze, int d, int ext)
{
  return new vtkFloatTensors(sze,d,ext);
}

// Description:
// Deep copy of texture coordinates.
vtkFloatTensors& vtkFloatTensors::operator=(const vtkFloatTensors& ft)
{
  this->T = ft.T;
  this->Dimension = ft.Dimension;
  
  return *this;
}

