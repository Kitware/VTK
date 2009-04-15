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

// .NAME vtkArraySort - Controls sorting of sparse array coordinsates.
//
// .SECTION Description
// vtkArraySort stores an ordered set of dimensions along which the values
// stored in a sparse array should be sorted.
//
// Convenience constructors are provided for specifying one, two, and three
// dimensions.  To sort along more than three dimensions, use the default
// constructor, SetDimensions(), and operator[] to assign each dimension
// to be sorted.
//
// .SECTION See Also
// vtkSparseArray
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkArraySort_h
#define __vtkArraySort_h

#include "vtkSystemIncludes.h"
#include <vtksys/stl/vector>

class VTK_COMMON_EXPORT vtkArraySort
{
public:
  // Description:
  // Create an empty set of dimensions.  Use SetDimensions() and operator[]
  // to populate them.
  vtkArraySort();
  
  // Description:
  // Sorts an array along one dimension.
  explicit vtkArraySort(vtkIdType i);
  
  // Description:
  // Sorts an array along two dimensions.
  vtkArraySort(vtkIdType i, vtkIdType j);
  
  // Description:
  // Sorts an array along three dimensions.
  vtkArraySort(vtkIdType i, vtkIdType j, vtkIdType k);

  // Description:
  // Return the number of dimensions for sorting.
  vtkIdType GetDimensions() const;

  // Description:
  // Set the number of dimensions to be sorted.  Note that this method resets
  // every to zero, so you must set every dimension explicitly using operator[]
  // after calling SetDimensions().
  void SetDimensions(vtkIdType dimensions);
  
  // Description:
  // Returns the i-th dimension to be sorted.
  vtkIdType& operator[](vtkIdType i);
  
  // Description:
  // Returns the i-th dimension to be sorted.
  const vtkIdType& operator[](vtkIdType i) const;  

  
  // Description:
  // Equality comparison
  bool operator==(const vtkArraySort& rhs) const;
  
  // Description:
  // Inequality comparison
  bool operator!=(const vtkArraySort& rhs) const;
  friend ostream& operator<<(
    ostream& stream, const vtkArraySort& rhs);
  
private:
  vtkstd::vector<vtkIdType> Storage;
};

#endif

