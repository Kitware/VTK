/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Tensors.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Tensors.hh"

vtkTensors::vtkTensors(int dim)
{
  this->Dimension = dim;
}

void vtkTensors::GetTensor(int id, vtkTensor &ft)
{
  vtkTensor *t = this->GetTensor(id);
  ft = *t;
}

void vtkTensors::InsertTensor(int id, float t11, float t12, float t13, 
                              float t21, float t22, float t23, 
                              float t31, float t32, float t33)
{
  vtkTensor t;
  t.SetComponent(0,0,t11);
  t.SetComponent(0,1,t12);
  t.SetComponent(0,2,t13);
  t.SetComponent(1,0,t21);
  t.SetComponent(1,1,t22);
  t.SetComponent(1,2,t23);
  t.SetComponent(2,0,t31);
  t.SetComponent(2,1,t32);
  t.SetComponent(2,2,t33);

  this->InsertTensor(id,t);
}

int vtkTensors::InsertNextTensor(float t11, float t12, float t13, 
                                 float t21, float t22, float t23, 
                                 float t31, float t32, float t33)
{
  vtkTensor t;
  t.SetComponent(0,0,t11);
  t.SetComponent(0,1,t12);
  t.SetComponent(0,2,t13);
  t.SetComponent(1,0,t21);
  t.SetComponent(1,1,t22);
  t.SetComponent(1,2,t23);
  t.SetComponent(2,0,t31);
  t.SetComponent(2,1,t32);
  t.SetComponent(2,2,t33);

  return this->InsertNextTensor(t);
}

// Description:
// Given a list of pt ids, return an array of tensors.
void vtkTensors::GetTensors(vtkIdList& ptId, vtkFloatTensors& ft)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    ft.InsertTensor(i,this->GetTensor(ptId.GetId(i)));
    }
}

void vtkTensors::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkRefCount::PrintSelf(os,indent);

  os << indent << "Number Of Tensors: " << this->GetNumberOfTensors() << "\n";
}
