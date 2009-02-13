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

// .NAME vtkArrayExtents - Stores the number of dimensions and size of an
// N-way array.
//
// .SECTION Description
// vtkArrayExtents describes the number of dimensions and size along each
// dimension of an N-way collection of values.  It is used to retrieve and
// update the extents of a vtkArray object.
//
// Convenience constructors are provided for creating extents along one, two,
// and three dimensions.  For higher dimensions, you can:
//
// Use the static Uniform() factory method to create extents that have the same
// size along an arbitrary number of dimensions.
//
// Use the default constructor and the Append() method to "grow" your extents
// to the correct number of dimensions.
// 
// Use the default constructuor, SetDimensions() and operator[] to assign a
// size along each dimension.
//
// .SECTION See Also
// vtkArray, vtkArrayCoordinates
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkArrayExtents_h
#define __vtkArrayExtents_h

#include "vtkSystemIncludes.h"
#include <vtksys/stl/vector>

class vtkArrayCoordinates;

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
  // Grow the number of dimensions by one, specifying the extent
  // of the new dimension.
  void Append(vtkIdType extent);

  // Description:
  // Return the current number of dimensions.
  vtkIdType GetDimensions() const;
  
  // Description:
  // Return the number of values that *could* be stored using the
  // current extents.  This is equal to the product of the extents
  // along each dimension.
  vtkIdType GetSize() const;

  // Description:
  // Set the current number of dimensions.  Note that this method
  // resets the extent along each dimension to zero, so you must assign
  // each dimension's extent explicitly using operator[] after calling
  // SetDimensions().
  void SetDimensions(vtkIdType dimensions);
  
  // Description:
  // Accesses the extent of the i-th dimension.
  vtkIdType& operator[](vtkIdType i);
  
  // Description:
  // Accesses the extent of the i-th dimension.
  const vtkIdType& operator[](vtkIdType i) const;
  
  // Description:
  // Equality comparison
  bool operator==(const vtkArrayExtents& rhs) const;
  
  // Description:
  // Inequality comparison
  bool operator!=(const vtkArrayExtents& rhs) const;

  // Description:
  // Returns true if the given array coordinates are completely contained
  // by the current extents (i.e. that 0 <= coordinate and coordinate < extent
  // along every dimension).  Returns false if the array coordinates are outside
  // the current extents, or contain a different number of dimensions.
  bool Contains(const vtkArrayCoordinates& coordinates) const;

  VTK_COMMON_EXPORT friend ostream& operator<<(
    ostream& stream, const vtkArrayExtents& rhs);
  
private:
  vtkstd::vector<vtkIdType> Storage;
};

#endif

