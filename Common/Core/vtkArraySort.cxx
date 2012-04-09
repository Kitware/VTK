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

vtkArraySort::vtkArraySort(DimensionT i) :
  Storage(1)
{
  this->Storage[0] = i;
}

vtkArraySort::vtkArraySort(DimensionT i, DimensionT j) :
  Storage(2)
{
  this->Storage[0] = i;
  this->Storage[1] = j;
}

vtkArraySort::vtkArraySort(DimensionT i, DimensionT j, DimensionT k) :
  Storage(3)
{
  this->Storage[0] = i;
  this->Storage[1] = j;
  this->Storage[2] = k;
}

vtkArraySort::DimensionT vtkArraySort::GetDimensions() const
{
  return this->Storage.size();
}

void vtkArraySort::SetDimensions(DimensionT dimensions)
{
  this->Storage.assign(dimensions, 0);
}

vtkArraySort::DimensionT& vtkArraySort::operator[](DimensionT i)
{
  return this->Storage[i];
}

const vtkArraySort::DimensionT& vtkArraySort::operator[](DimensionT i) const
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
  for(vtkArraySort::DimensionT i = 0; i != rhs.GetDimensions(); ++i)
    {
    if(i)
      stream << ",";
    stream << rhs[i];
    }

  return stream;
}
