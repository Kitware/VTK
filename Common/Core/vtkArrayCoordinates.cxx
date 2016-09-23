/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayCoordinates.cxx

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

vtkArrayCoordinates::vtkArrayCoordinates()
{
}

vtkArrayCoordinates::vtkArrayCoordinates(CoordinateT i) :
  Storage(1)
{
  this->Storage[0] = i;
}

vtkArrayCoordinates::vtkArrayCoordinates(CoordinateT i, CoordinateT j) :
  Storage(2)
{
  this->Storage[0] = i;
  this->Storage[1] = j;
}

vtkArrayCoordinates::vtkArrayCoordinates(CoordinateT i, CoordinateT j, CoordinateT k) :
  Storage(3)
{
  this->Storage[0] = i;
  this->Storage[1] = j;
  this->Storage[2] = k;
}

vtkArrayCoordinates::DimensionT vtkArrayCoordinates::GetDimensions() const
{
  return this->Storage.size();
}

void vtkArrayCoordinates::SetDimensions(DimensionT dimensions)
{
  this->Storage.assign(dimensions, 0);
}

vtkArrayCoordinates::CoordinateT& vtkArrayCoordinates::operator[](DimensionT i)
{
  return this->Storage[i];
}

const vtkArrayCoordinates::CoordinateT& vtkArrayCoordinates::operator[](DimensionT i) const
{
  return this->Storage[i];
}

vtkArrayCoordinates::CoordinateT vtkArrayCoordinates::GetCoordinate(DimensionT i) const
{
  return this->Storage[i];
}

void vtkArrayCoordinates::SetCoordinate(DimensionT i, const CoordinateT& coordinate)
{
  this->Storage[i] = coordinate;
}

bool vtkArrayCoordinates::operator==(const vtkArrayCoordinates& rhs) const
{
  return this->Storage == rhs.Storage;
}

bool vtkArrayCoordinates::operator!=(const vtkArrayCoordinates& rhs) const
{
  return !(*this == rhs);
}

ostream& operator<<(ostream& stream, const vtkArrayCoordinates& rhs)
{
  for(vtkArrayCoordinates::DimensionT i = 0; i != rhs.GetDimensions(); ++i)
  {
    if(i)
      stream << ",";
    stream << rhs[i];
  }

  return stream;
}

