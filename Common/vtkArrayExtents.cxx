/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayExtents.cxx
  
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

#include "vtkArrayExtents.h"

#include <vtksys/stl/functional>
#include <vtksys/stl/numeric>

vtkArrayExtents::vtkArrayExtents()
{
}

vtkArrayExtents::vtkArrayExtents(vtkIdType i) :
  Storage(1)
{
  this->Storage[0] = i;
}

vtkArrayExtents::vtkArrayExtents(vtkIdType i, vtkIdType j) :
  Storage(2)
{
  this->Storage[0] = i;
  this->Storage[1] = j;
}

vtkArrayExtents::vtkArrayExtents(vtkIdType i, vtkIdType j, vtkIdType k) :
  Storage(3)
{
  this->Storage[0] = i;
  this->Storage[1] = j;
  this->Storage[2] = k;
}

const vtkArrayExtents vtkArrayExtents::Uniform(vtkIdType n, vtkIdType m)
{
  vtkArrayExtents result;
  result.Storage.assign(n, m);
  return result;
}

void vtkArrayExtents::Append(vtkIdType extent)
{
  this->Storage.push_back(extent);
}

vtkIdType vtkArrayExtents::GetDimensions() const
{
  return this->Storage.size();
}

vtkIdType vtkArrayExtents::GetSize() const
{
  if(this->Storage.empty())
    return 0;

  return vtkstd::accumulate(this->Storage.begin(), this->Storage.end(), 1, vtkstd::multiplies<vtkIdType>());
}

void vtkArrayExtents::SetDimensions(vtkIdType dimensions)
{
  this->Storage.assign(dimensions, 0);
}

vtkIdType& vtkArrayExtents::operator[](vtkIdType i)
{
  return this->Storage[i];
}

const vtkIdType& vtkArrayExtents::operator[](vtkIdType i) const
{
  return this->Storage[i];
}

bool vtkArrayExtents::operator==(const vtkArrayExtents& rhs) const
{
  return this->Storage == rhs.Storage;
}

bool vtkArrayExtents::operator!=(const vtkArrayExtents& rhs) const
{
  return !(*this == rhs);
}

ostream& operator<<(ostream& stream, const vtkArrayExtents& rhs)
{
  for(unsigned int i = 0; i != rhs.Storage.size(); ++i)
    {
    if(i)
      stream << "x";
    stream << rhs.Storage[i];
    }

  return stream;
}

