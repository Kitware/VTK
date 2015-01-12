/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArraySort.h

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

// .NAME vtkArraySort - Controls sorting of sparse array coordinates.
//
// .SECTION Description
// vtkArraySort stores an ordered set of dimensions along which the
// values stored in a sparse array should be sorted.
//
// Convenience constructors are provided for specifying one, two, and
// three dimensions.  To sort along more than three dimensions, use the
// default constructor, SetDimensions(), and operator[] to assign each
// dimension to be sorted.
//
// .SECTION See Also
// vtkSparseArray
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National
// Laboratories.

#ifndef vtkArraySort_h
#define vtkArraySort_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSystemIncludes.h"
#include "vtkArrayCoordinates.h"
#include <vector>

class VTKCOMMONCORE_EXPORT vtkArraySort
{
public:
  typedef vtkArrayCoordinates::DimensionT DimensionT;

  // Description:
  // Create an empty set of dimensions.  Use SetDimensions() and
  // operator[] to populate them.
  vtkArraySort();

  // Description:
  // Sorts an array along one dimension.
  explicit vtkArraySort(DimensionT i);

  // Description:
  // Sorts an array along two dimensions.
  vtkArraySort(DimensionT i, DimensionT j);

  // Description:
  // Sorts an array along three dimensions.
  vtkArraySort(DimensionT i, DimensionT j, DimensionT k);

  // Description:
  // Return the number of dimensions for sorting.
  DimensionT GetDimensions() const;

  // Description:
  // Set the number of dimensions to be sorted.  Note that this method
  // resets every dimension to zero, so you must set every dimension
  // explicitly using operator[] after calling SetDimensions().
  void SetDimensions(DimensionT dimensions);

  // Description:
  // Returns the i-th dimension to be sorted.
  DimensionT& operator[](DimensionT i);

  // Description:
  // Returns the i-th dimension to be sorted.
  const DimensionT& operator[](DimensionT i) const;


  // Description:
  // Equality comparison
  bool operator==(const vtkArraySort& rhs) const;

  // Description:
  // Inequality comparison
  bool operator!=(const vtkArraySort& rhs) const;

  // Description:
  // Serialization
  VTKCOMMONCORE_EXPORT friend ostream& operator<<(
    ostream& stream, const vtkArraySort& rhs);

private:
  //BTX
  std::vector<DimensionT> Storage;
  //ETX
};

#endif

// VTK-HeaderTest-Exclude: vtkArraySort.h
