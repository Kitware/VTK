/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArraySlice.cxx
  
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

#include "vtkArrayCoordinates.h"
#include "vtkArraySlice.h"

vtkArraySlice::vtkArraySlice()
{
}

vtkArraySlice::vtkArraySlice(const vtkArrayRange& i) :
  Storage(1)
{
  this->Storage[0] = i;
}

vtkArraySlice::vtkArraySlice(const vtkArrayRange& i, const vtkArrayRange& j) :
  Storage(2)
{
  this->Storage[0] = i;
  this->Storage[1] = j;
}

vtkArraySlice::vtkArraySlice(const vtkArrayRange& i, const vtkArrayRange& j, const vtkArrayRange& k) :
  Storage(3)
{
  this->Storage[0] = i;
  this->Storage[1] = j;
  this->Storage[2] = k;
}

vtkIdType vtkArraySlice::GetDimensions() const
{
  return this->Storage.size();
}

const vtkArrayExtents vtkArraySlice::GetExtents() const
{
  vtkArrayExtents result;
  result.SetDimensions(this->GetDimensions());

  for(int i = 0; i != this->GetDimensions(); ++i)
    result[i] = this->Storage[i].GetExtent();

  return result;
}

void vtkArraySlice::GetCoordinatesN(vtkIdType n, vtkArrayCoordinates& coordinates) const
{
  coordinates.SetDimensions(this->GetDimensions());

  vtkIdType divisor = 1;
  for(vtkIdType i = 0; i < this->GetDimensions(); ++i)
    {
    coordinates[i] = ((n / divisor) % this->Storage[i].GetExtent()) + this->Storage[i].GetBegin();
    divisor *= this->Storage[i].GetExtent();
    }
}

bool vtkArraySlice::Contains(const vtkArrayCoordinates& coordinates) const
{
  if(coordinates.GetDimensions() != this->GetDimensions())
    return false;

  for(vtkIdType i = 0; i < this->GetDimensions(); ++i)
    {
    if(!this->Storage[i].Contains(coordinates[i]))
      return false;
    }

  return true;
}

void vtkArraySlice::SetDimensions(vtkIdType dimensions)
{
  this->Storage.assign(dimensions, vtkArrayRange());
}

vtkArrayRange& vtkArraySlice::operator[](vtkIdType i)
{
  return this->Storage[i];
}

const vtkArrayRange& vtkArraySlice::operator[](vtkIdType i) const
{
  return this->Storage[i];
}

ostream& operator<<(ostream& stream, const vtkArraySlice& rhs)
{
  for(vtkIdType i = 0; i != static_cast<vtkIdType>(rhs.Storage.size()); ++i)
    {
    if(i)
      stream << " ";
    stream << rhs[i];
    }
    
  return stream;
}

