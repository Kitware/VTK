// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkArrayRange.h"

#include <algorithm> // for std::max()

VTK_ABI_NAMESPACE_BEGIN
vtkArrayRange::vtkArrayRange()
  : Begin(0)
  , End(0)
{
}

vtkArrayRange::vtkArrayRange(CoordinateT begin, CoordinateT end)
  : Begin(begin)
  , End(std::max(begin, end))
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

bool vtkArrayRange::Contains(CoordinateT coordinate) const
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
VTK_ABI_NAMESPACE_END
