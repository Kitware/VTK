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
  vtkTensor& t = this->GetTensor(id);
  ft = t;
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
