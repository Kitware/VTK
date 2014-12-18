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

// .NAME vtkArrayExtentsList - Stores a collection of vtkArrayExtents objects.
//
// .SECTION Description
// vtkArrayExtentsList provides storage for a collection of vtkArrayExtents
// instances.  Constructors are provided for creating collections
// containing one, two, three, or four slices.  To work with larger
// numbers of slices, use the default constructor, the SetCount() method,
// and operator[].
//
// vtkArrayExtentsList is most commonly used with the vtkInterpolate()
// function, which is used to computed weighted sums of vtkArray slices.
//
// .SECTION See Also
// vtkArray, vtkExtents
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National
// Laboratories.

#ifndef vtkArrayExtentsList_h
#define vtkArrayExtentsList_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkArrayExtents.h"
#include <vtksys/stl/vector> // STL Header

class VTKCOMMONCORE_EXPORT vtkArrayExtentsList
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
  // Sets the number of extents stored in this collection.  Note: all
  // extents will be empty after calling SetCount(), use operator[]
  // to assign extents to each item in the collection.
  void SetCount(vtkIdType count);


  // Description:
  // Accesses the i-th slice.
  vtkArrayExtents& operator[](vtkIdType i);

  // Description:
  // Accesses the i-th slice.
  const vtkArrayExtents& operator[](vtkIdType i) const;

private:
  //BTX
  std::vector<vtkArrayExtents> Storage;
  //ETX
};

#endif

// VTK-HeaderTest-Exclude: vtkArrayExtentsList.h
