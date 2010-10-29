/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayRange.cxx
  
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

#include "vtkArrayRange.h"

#include <vtkstd/algorithm> // for vtkstd::max()

vtkArrayRange::vtkArrayRange() :
  Begin(0),
  End(0)
{
}

vtkArrayRange::vtkArrayRange(CoordinateT begin, CoordinateT end) :
  Begin(begin),
  End(vtkstd::max(begin, end))
{
}

vtkArrayRange::CoordinateT vtkArrayRange::GetBegin() const
{
  return this->Begin;
}

vtkArrayRange::CoordinateT vtkArrayRange::GetEnd() const
{
  return this->End;
}

vtkArrayRange::CoordinateT vtkArrayRange::GetSize() const
{
  return this->End - this->Begin;
}

bool vtkArrayRange::Contains(const vtkArrayRange& range) const
{
  return this->Begin <= range.Begin && range.End <= this->End;
}

bool vtkArrayRange::Contains(const CoordinateT coordinate) const
{
  return this->Begin <= coordinate && coordinate < this->End;
}

bool operator==(const vtkArrayRange& lhs, const vtkArrayRange& rhs)
{
  return lhs.Begin == rhs.Begin && lhs.End == rhs.End;
}

bool operator!=(const vtkArrayRange& lhs, const vtkArrayRange& rhs)
{
  return !(lhs == rhs);
}

ostream& operator<<(ostream& stream, const vtkArrayRange& rhs)
{
  stream << "[" << rhs.Begin << ", " << rhs.End << ")";
  return stream;
}

