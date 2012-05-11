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

// .NAME vtkArrayExtents - Stores the number of dimensions and valid
// coordinate ranges along each dimension for vtkArray.
//
// .SECTION Description
// vtkArrayExtents describes the number of dimensions and coordinate
// ranges along each dimension of an N-way collection of values.  It is
// used to retrieve and update the extents of a vtkArray object.
//
// Conceptually, vtkArrayExtents is a collection of vtkArrayRange
// objects, one per dimension, that store the half-open range of valid
// coordinates (the "extent") for that dimension.  Because each extent is
// stored as a range rather than a size, you can: create arrays that use
// one-based coordinates for consistency with mathematics and tools such
// as MATLAB; easily represent arbitrary subsets of an array; and easily
// store and manipulate distributed arrays using "global" coordinates.
//
// Convenience constructors are provided for creating extents along one,
// two, and three dimensions.  For higher dimensions, you can:
//
// Use the static Uniform() factory method to create extents that have
// the same size along an arbitrary number of dimensions.
//
// Use the default constructor and the Append() method to "grow" your
// extents to the correct number of dimensions.
//
// Use the default constructuor, SetDimensions() and operator[] to assign
// a size along each dimension.
//
// .SECTION See Also
// vtkArray, vtkArrayRange, vtkArrayCoordinates
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National
// Laboratories.

#ifndef __vtkArrayExtents_h
#define __vtkArrayExtents_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSystemIncludes.h"
#include "vtkArrayRange.h"
#include <vector> // STL Header

class VTKCOMMONCORE_EXPORT vtkArrayExtents
{
public:
  typedef vtkArrayCoordinates::DimensionT DimensionT;
  typedef vtkArrayCoordinates::CoordinateT CoordinateT;
  typedef vtkTypeUInt64 SizeT;

  // Description:
  // Create zero-dimensional extents.
  vtkArrayExtents();

  // Description:
  // Create zero-based one-dimensional extents.  This constructor is shorthand for
  // vtkArrayExtents(vtkArrayRange(0, i)).
  explicit vtkArrayExtents(const CoordinateT i);
  // Description:
  // Create one-dimensional extents.
  explicit vtkArrayExtents(const vtkArrayRange& i);

  // Description:
  // Create zero-based two-dimensional extents.  This constructor is shorthand for
  // vtkArrayExtents(vtkArrayRange(0, i), vtkArrayRange(0, j)).
  vtkArrayExtents(const CoordinateT i, const CoordinateT j);
  // Description:
  // Create two-dimensional extents.
  vtkArrayExtents(const vtkArrayRange& i, const vtkArrayRange& j);

  // Description:
  // Create zero-based three-dimensional extents.  This constructor is shorthand for
  // vtkArrayExtents(vtkArrayRange(0, i), vtkArrayRange(0, j),
  // vtkArrayRange(0, k)).
  vtkArrayExtents(const CoordinateT i, const CoordinateT j, const CoordinateT k);
  // Description:
  // Create three-dimensional extents.
  vtkArrayExtents(const vtkArrayRange& i, const vtkArrayRange& j, const vtkArrayRange& k);

  // Description:
  // Create n-dimensional extents with extent [0, m) along each dimension.
  // This is useful for creating e.g: a square matrix.
  static const vtkArrayExtents Uniform(DimensionT n, CoordinateT m);

  // Description:
  // Grow the number of dimensions by one, specifying the extent
  // of the new dimension.
  void Append(const vtkArrayRange& extent);

  // Description:
  // Return the current number of dimensions.
  DimensionT GetDimensions() const;

  // Description:
  // Return the number of values that *could* be stored using the
  // current extents.  This is equal to the product of the size of the
  // extent along each dimension.
  SizeT GetSize() const;

  // Description:
  // Set the current number of dimensions.  Note that this method
  // resets the extent along each dimension to an empty range, so you
  // must assign each dimension's extent explicitly using operator[]
  // after calling SetDimensions().
  void SetDimensions(DimensionT dimensions);

  // Description:
  // Accesses the extent of the i-th dimension.
  vtkArrayRange& operator[](DimensionT i);

  // Description:
  // Accesses the extent of the i-th dimension.
  const vtkArrayRange& operator[](DimensionT i) const;

  // Description:
  // Get the extent of the i-th dimension.
  vtkArrayRange GetExtent(DimensionT i) const;

  // Description:
  // Set the extent of the i-th dimension.
  void SetExtent(DimensionT i, const vtkArrayRange&);

  // Description:
  // Equality comparison
  bool operator==(const vtkArrayExtents& rhs) const;

  // Description:
  // Inequality comparison
  bool operator!=(const vtkArrayExtents& rhs) const;

  // Description:
  // Returns true iff every range in the current extents is zero-based.
  // This is useful as a precondition test for legacy filters/operations
  // that predate the switch to range-based extents and assume that all
  // extents are zero-based.  In general, new code should be written to
  // work with arbitrary range extents, so won't need to perform this
  // check.
  bool ZeroBased() const;

  // Description:
  // Returns true iff the given extents have the same number of
  // dimensions and size along each dimension.  Note that the ranges
  // along each dimension may have different values, so long as their
  // sizes match.
  bool SameShape(const vtkArrayExtents& rhs) const;

  // Description:
  // Returns coordinates that reference the n-th value in the extents,
  // where n is in the range [0, GetSize()).  The returned coordinates
  // will be ordered so that the left-most indices vary fastest.  This is
  // equivalent to column-major ordering for matrices, and corresponds to
  // the order in which consecutive array values would be stored in
  // languages such as Fortran, MATLAB, Octave, and R.
  void GetLeftToRightCoordinatesN(SizeT n, vtkArrayCoordinates& coordinates) const;

  // Description:
  // Returns coordinates that reference the n-th value in the extents,
  // where n is in the range [0, GetSize()).  The returned coordinates
  // will be ordered so that the right-most indices vary fastest.  This is
  // equivalent to row-major ordering for matrices, and corresponds to
  // the order in which consecutive array values would be stored in
  // languages including C and C++.
  void GetRightToLeftCoordinatesN(SizeT n, vtkArrayCoordinates& coordinates) const;

  // Description:
  // Returns true if the given extents are a non-overlapping subset of
  // the current extents.  Returns false if any of the given extents fall
  // outside the current extents, or there is a mismatch in the number of
  // dimensions.
  bool Contains(const vtkArrayExtents& extents) const;

  // Description:
  // Returns true if the given array coordinates are completely contained
  // by the current extents (i.e. extent begin <= coordinate and
  // coordinate < extent end along every dimension).  Returns false if
  // the array coordinates are outside the current extents, or contain a
  // different number of dimensions.
  bool Contains(const vtkArrayCoordinates& coordinates) const;

  VTKCOMMONCORE_EXPORT friend ostream& operator<<(
    ostream& stream, const vtkArrayExtents& rhs);

private:
  //BTX
  std::vector<vtkArrayRange> Storage;
  //ETX
};

#endif
// VTK-HeaderTest-Exclude: vtkArrayExtents.h
