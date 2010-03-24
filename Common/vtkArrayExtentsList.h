/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayExtentsList.h
  
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

// .NAME vtkArrayExtentsList - Stores a collection of vtkArraySlice objects.
//
// .SECTION Description
// vtkArrayExtentsList provides storage for a collection of vtkArraySlice instances.
// Constructors are provided for creating collections containing one, two, three,
// or four slices.  To work with larger numbers of slices, use the default
// constructor, the SetCount() method, and operator[].
//
// vtkArrayExtentsList is most commonly used with the vtkInterpolate() function, which
// is used to computed weighted sums of vtkArray slices.
//
// .SECTION See Also
// vtkArray, vtkSlice
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkArrayExtentsList_h
#define __vtkArrayExtentsList_h

#include "vtkArrayExtents.h"
#include <vtksys/stl/vector> // STL Header

class VTK_COMMON_EXPORT vtkArrayExtentsList
{
public:
  // Description:
  // Creates an empty collection of slices.
  vtkArrayExtentsList();
  
  // Description:
  // Creates a collection containing one slice.
  vtkArrayExtentsList(const vtkArrayExtents& i);
  
  // Description:
  // Creates a collection containing two slices.
  vtkArrayExtentsList(const vtkArrayExtents& i, const vtkArrayExtents& j);
  
  // Description:
  // Creates a collection containing three slices.
  vtkArrayExtentsList(const vtkArrayExtents& i, const vtkArrayExtents& j, const vtkArrayExtents& k);
  
  // Description:
  // Creates a collection containing four slices.
  vtkArrayExtentsList(const vtkArrayExtents& i, const vtkArrayExtents& j, const vtkArrayExtents& k, const vtkArrayExtents& l);

  // Description:
  // Returns the number of slices stored in this collection.
  vtkIdType GetCount() const;
  
  // Description:
  // Sets the number of slices stored in this collection.  Note: all
  // slices will be empty after calling SetSliceCount(), use operator[]
  // to assign each slice.
  void SetCount(vtkIdType count);

  // Description:
  // Accesses the i-th slice.
  vtkArrayExtents& operator[](vtkIdType i);
  
  // Description:
  // Accesses the i-th slice.
  const vtkArrayExtents& operator[](vtkIdType i) const;

private:
  vtkstd::vector<vtkArrayExtents> Storage;
};

#endif

