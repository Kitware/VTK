/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArraySlices.h
  
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

// .NAME vtkArraySlices - Stores a collection of vtkArraySlice objects.
//
// .SECTION Description
// vtkArraySlices provides storage for a collection of vtkArraySlice instances.
// Constructors are provided for creating collections containing one, two, three,
// or four slices.  To work with larger numbers of slices, use the default
// constructor, the SetCount() method, and operator[].
//
// vtkArraySlices is most commonly used with the vtkInterpolate() function, which
// is used to computed weighted sums of vtkArray slices.
//
// .SECTION See Also
// vtkArray, vtkSlice
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkArraySlices_h
#define __vtkArraySlices_h

#include "vtkArraySlice.h"
#include <vtksys/stl/vector>

class VTK_COMMON_EXPORT vtkArraySlices
{
public:
  // Description:
  // Creates an empty collection of slices.
  vtkArraySlices();
  
  // Description:
  // Creates a collection containing one slice.
  vtkArraySlices(const vtkArraySlice& i);
  
  // Description:
  // Creates a collection containing two slices.
  vtkArraySlices(const vtkArraySlice& i, const vtkArraySlice& j);
  
  // Description:
  // Creates a collection containing three slices.
  vtkArraySlices(const vtkArraySlice& i, const vtkArraySlice& j, const vtkArraySlice& k);
  
  // Description:
  // Creates a collection containing four slices.
  vtkArraySlices(const vtkArraySlice& i, const vtkArraySlice& j, const vtkArraySlice& k, const vtkArraySlice& l);

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
  vtkArraySlice& operator[](vtkIdType i);
  
  // Description:
  // Accesses the i-th slice.
  const vtkArraySlice& operator[](vtkIdType i) const;

private:
  vtkstd::vector<vtkArraySlice> Storage;
};

#endif

