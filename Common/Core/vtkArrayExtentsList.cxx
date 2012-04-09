/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayExtentsList.cxx
  
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

#include "vtkArrayExtentsList.h"

vtkArrayExtentsList::vtkArrayExtentsList()
{
}

vtkArrayExtentsList::vtkArrayExtentsList(const vtkArrayExtents& i) :
  Storage(1)
{
  this->Storage[0] = i;
}
  
vtkArrayExtentsList::vtkArrayExtentsList(const vtkArrayExtents& i, const vtkArrayExtents& j) :
  Storage(2)
{
  this->Storage[0] = i;
  this->Storage[1] = j;
}
  
vtkArrayExtentsList::vtkArrayExtentsList(const vtkArrayExtents& i, const vtkArrayExtents& j, const vtkArrayExtents& k) :
  Storage(3)
{
  this->Storage[0] = i;
  this->Storage[1] = j;
  this->Storage[2] = k;
}

vtkArrayExtentsList::vtkArrayExtentsList(const vtkArrayExtents& i, const vtkArrayExtents& j, const vtkArrayExtents& k, const vtkArrayExtents& l) :
  Storage(4)
{
  this->Storage[0] = i;
  this->Storage[1] = j;
  this->Storage[2] = k;
  this->Storage[3] = l;
}

vtkIdType vtkArrayExtentsList::GetCount() const
{
  return this->Storage.size();
}

void vtkArrayExtentsList::SetCount(vtkIdType count)
{
  this->Storage.assign(count, vtkArrayExtents());
}

vtkArrayExtents& vtkArrayExtentsList::operator[](vtkIdType i)
{
  return this->Storage[i];
}

const vtkArrayExtents& vtkArrayExtentsList::operator[](vtkIdType i) const
{
  return this->Storage[i];
}

