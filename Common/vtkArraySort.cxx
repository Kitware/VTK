/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArraySort.cxx
  
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

#include "vtkArraySort.h"

vtkArraySort::vtkArraySort()
{
}

vtkArraySort::vtkArraySort(vtkIdType i) :
  Storage(1)
{
  this->Storage[0] = i;
}

vtkArraySort::vtkArraySort(vtkIdType i, vtkIdType j) :
  Storage(2)
{
  this->Storage[0] = i;
  this->Storage[1] = j;
}

vtkArraySort::vtkArraySort(vtkIdType i, vtkIdType j, vtkIdType k) :
  Storage(3)
{
  this->Storage[0] = i;
  this->Storage[1] = j;
  this->Storage[2] = k;
}

vtkIdType vtkArraySort::GetDimensions() const
{
  return this->Storage.size();
}

void vtkArraySort::SetDimensions(vtkIdType dimensions)
{
  this->Storage.assign(dimensions, 0);
}

vtkIdType& vtkArraySort::operator[](vtkIdType i)
{
  return this->Storage[i];
}

const vtkIdType& vtkArraySort::operator[](vtkIdType i) const
{
  return this->Storage[i];
}

bool vtkArraySort::operator==(const vtkArraySort& rhs) const
{
  return this->Storage == rhs.Storage;
}

bool vtkArraySort::operator!=(const vtkArraySort& rhs) const
{
  return !(*this == rhs);
}

ostream& operator<<(ostream& stream, const vtkArraySort& rhs)
{
  for(vtkIdType i = 0; i != rhs.GetDimensions(); ++i)
    {
    if(i)
      stream << ",";
    stream << rhs[i];
    }
  
  return stream;
}

