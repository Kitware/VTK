/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayRange.h
  
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

// .NAME vtkArrayRange - Stores a half-open range of array coordinates.
//
// .SECTION Description
// vtkArrayRange stores a half-open range of array coordinates along a single
// dimension of a vtkArraySlice object.
//
// .SECTION See Also
// vtkArray, vtkArrayRange
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkArrayRange_h
#define __vtkArrayRange_h

#include "vtkSystemIncludes.h"

class VTK_COMMON_EXPORT vtkArrayRange
{
public:
  // Description:
  // Creates an empty range.
  vtkArrayRange();
  
  // Description:
  // Creates a half-open range [begin, end).  Note that begin must be <= end,
  // if not, creates the empty range [begin, begin).
  vtkArrayRange(vtkIdType begin, vtkIdType end);

  // Description:
  // Returns the beginning of the range
  vtkIdType GetBegin() const;
  
  // Description:
  // Returns one-past-the-end of the range
  vtkIdType GetEnd() const;

  // Description:
  // Returns the size of the range (the distance End - Begin).
  vtkIdType GetSize() const;

  // Description:
  // Returns true iff the given range is a non-overlapping subset of this range.
  bool Contains(const vtkArrayRange& range) const;

  // Description:
  // Returns true iff the given coordinate falls within this range.
  bool Contains(const vtkIdType coordinate) const;

  // Description:
  // Equality comparisons.
  VTK_COMMON_EXPORT friend bool operator==(const vtkArrayRange& lhs, const vtkArrayRange& rhs);
  VTK_COMMON_EXPORT friend bool operator!=(const vtkArrayRange& lhs, const vtkArrayRange& rhs);

  // Description:
  // Serialization.
  VTK_COMMON_EXPORT friend ostream& operator<<(ostream& stream, const vtkArrayRange& rhs);

private:
  // Description:
  // Stores the beginning of the range.
  vtkIdType Begin;
  
  // Description:
  // Stores one-past-the-end of the range.
  vtkIdType End;
};

#endif

