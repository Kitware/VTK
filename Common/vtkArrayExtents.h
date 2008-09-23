/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayExtents.h
  
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

#ifndef __vtkArrayExtents_h
#define __vtkArrayExtents_h

#include "vtkSystemIncludes.h"
#include <vtksys/stl/vector>

// .NAME vtkArrayExtents - Stores the dimensions and size of an N-way array.

// .SECTION Description
// Describes the dimensions and size along each dimension of an N-way collection
// of values. Convenience constructors are provided for working with 1, 2, and
// 3-way data.  For higher dimensions, use the static Uniform() factory method,
// or use SetDimensions() and operator[] to assign values.

// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

class VTK_COMMON_EXPORT vtkArrayExtents
{
public:
  // Description:
  // Create zero-dimensional extents.
  vtkArrayExtents();
  
  // Description:
  // Create one-dimensional extents. 
  explicit vtkArrayExtents(vtkIdType i);
  
  // Description:
  // Create two-dimensional extents.
  vtkArrayExtents(vtkIdType i, vtkIdType j);
  
  // Description:
  // Create three-dimensional extents.
  vtkArrayExtents(vtkIdType i, vtkIdType j, vtkIdType k);
  
  // Description:
  // Create n-dimensional extents with size m along each dimension.
  static const vtkArrayExtents Uniform(vtkIdType n, vtkIdType m);

  // Description:
  // Return the current number of dimensions.
  const vtkIdType GetDimensions() const;
  
  // Description:
  // Return the number of values that *could* be stored using the
  // current extents.
  const vtkIdType GetSize() const;

  // Description:
  // Set the current number of dimensions.  Note that this method
  // resets the extent along each dimension to zero, so you must assign
  // each dimension's extent explicitly after calling it.
  void SetDimensions(vtkIdType dimensions);
  
  // Description:
  // Accesses the extent of the i-th dimension.
  vtkIdType& operator[](vtkIdType i);
  
  // Description:
  // Accesses the extent of the i-th dimension.
  const vtkIdType& operator[](vtkIdType i) const;
  
  // Description:
  // Equality comparison
  const bool operator==(const vtkArrayExtents& rhs) const;
  
  // Description:
  // Inequality comparison
  const bool operator!=(const vtkArrayExtents& rhs) const;

  VTK_COMMON_EXPORT friend ostream& operator<<(ostream& stream, const vtkArrayExtents& rhs);
  
private:
  vtkstd::vector<vtkIdType> Storage;
};

#endif

