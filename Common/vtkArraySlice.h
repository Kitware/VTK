/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArraySlice.h
  
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

// .NAME vtkArraySlice - Describes a subset of an N-way array.
//
// .SECTION Description
// vtkArraySlice describes a subset of a vtkArray as a set of half-open
// ranges along each dimension.
//
// Convenience constructors are provided for specifying one, two, and three
// dimension slices.  For higher dimensions, use the default constructor, the
// SetDimensions() method and operator[] to assign a range along each dimension
// of a slice.
//
// vtkArraySlice is most commonly used with the vtkInterpolate() function, which
// is used to compute weighted sums of vtkArray slices.
//
// .SECTION See Also
// vtkArray, vtkRange
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkArraySlice_h
#define __vtkArraySlice_h

#include "vtkArrayExtents.h"
#include "vtkArrayCoordinates.h"
#include "vtkArrayRange.h"

#include <vtksys/stl/vector>

class VTK_COMMON_EXPORT vtkArraySlice
{
public:
  // Description:
  // Create a zero-dimensional slice.
  vtkArraySlice();
  
  // Description:
  // Create a one-dimensional slice.
  vtkArraySlice(
    const vtkArrayRange& i);
  
  // Description:
  // Create a two-dimensional slice.
  vtkArraySlice(
    const vtkArrayRange& i,
    const vtkArrayRange& j);
  
  // Description:
  // Create a three-dimensional slice.
  vtkArraySlice(
    const vtkArrayRange& i,
    const vtkArrayRange& j,
    const vtkArrayRange& k);

  // Description:
  // Returns the number of dimensions in this slice.
  vtkIdType GetDimensions() const;
  
  // Description:
  // Returns the extents of this slice - i.e: the size of the range
  // along each dimension.
  const vtkArrayExtents GetExtents() const;
  
  // Description:
  // Returns coordinates that reference the n-th value in the slice, where
  // n is in the range [0, GetExtents().GetSize()).  Note that the order
  // in which coordinates are visited is undefined.
  const vtkArrayCoordinates GetCoordinatesN(vtkIdType n) const;

  // Description:
  // Sets the number of slice dimensions.  Use operator[] to set the range
  // along each dimension.  Note that the range along each slice dimension will
  // be empty after calling SetDimensions(), so you must explicitly set them all.
  void SetDimensions(vtkIdType dimensions);
  
  // Description:
  // Accesses the range of the i-th dimension.
  vtkArrayRange& operator[](vtkIdType i);
  
  // Description:
  // Accesses the rnage of the i-th dimension.
  const vtkArrayRange& operator[](vtkIdType i) const;
  
  friend ostream& operator<<(ostream& stream, const vtkArraySlice& rhs);

private:
  vtkstd::vector<vtkArrayRange> Storage;
};

#endif

