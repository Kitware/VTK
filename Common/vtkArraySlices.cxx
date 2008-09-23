/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArraySlices.cxx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkArraySlices.h"

vtkArraySlices::vtkArraySlices()
{
}

vtkArraySlices::vtkArraySlices(const vtkArraySlice& i) :
  Storage(1)
{
  this->Storage[0] = i;
}
  
vtkArraySlices::vtkArraySlices(const vtkArraySlice& i, const vtkArraySlice& j) :
  Storage(2)
{
  this->Storage[0] = i;
  this->Storage[1] = j;
}
  
vtkArraySlices::vtkArraySlices(const vtkArraySlice& i, const vtkArraySlice& j, const vtkArraySlice& k) :
  Storage(3)
{
  this->Storage[0] = i;
  this->Storage[1] = j;
  this->Storage[2] = k;
}

vtkArraySlices::vtkArraySlices(const vtkArraySlice& i, const vtkArraySlice& j, const vtkArraySlice& k, const vtkArraySlice& l) :
  Storage(4)
{
  this->Storage[0] = i;
  this->Storage[1] = j;
  this->Storage[2] = k;
  this->Storage[3] = l;
}

const vtkIdType vtkArraySlices::GetCount() const
{
  return this->Storage.size();
}

void vtkArraySlices::SetCount(vtkIdType count)
{
  this->Storage.assign(count, vtkArraySlice());
}

vtkArraySlice& vtkArraySlices::operator[](vtkIdType i)
{
  return this->Storage[i];
}

const vtkArraySlice& vtkArraySlices::operator[](vtkIdType i) const
{
  return this->Storage[i];
}

