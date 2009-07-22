/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayCoordinateIterator.h

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

// .NAME vtkArrayCoordinateIterator - Iterates over array coordinates.
//
// .SECTION Description
// vtkArrayCoordinateIterator iterates over every unique set of coordinates
// that are defined by a set of vtkArrayExtents.  The order in which
// coordinates are visited is undefined.
//
// Note that vtkArrayCoordinateIterator visits every permutation of
// coordinates defined by a set of array extents, *not* the array itself - if
// you are working with a sparse array, any of the coordinates that are visited
// by the iterator could be NULL within the array.
//
// vtkArrayCoordinateIterator is mainly useful if you are writing a source that
// generates arbitrary-dimension arrays from scratch.  Algorithms that process
// existing arrays can almost always be written more efficiently using
// vtkArray::GetCoordinatesN() and vtkTypedArray::GetValueN(), which only visit
// non-NULL values within an array.
//
// .SECTION See Also
// vtkArray, vtkArrayExtents, vtkArrayCoordinates
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkArrayCoordinateIterator_h
#define __vtkArrayCoordinateIterator_h

#include "vtkArrayExtents.h"
#include "vtkArrayCoordinates.h"
#include "vtkObject.h"

class VTK_COMMON_EXPORT vtkArrayCoordinateIterator : public vtkObject
{
public:
  static vtkArrayCoordinateIterator* New();
  vtkTypeRevisionMacro(vtkArrayCoordinateIterator, vtkObject);
  void PrintSelf(ostream &os, vtkIndent indent);

//BTX
  // Description:
  // Sets the array extents to iterate over, and resets the iterator to
  // the beginning of the range of unique coordinates.
  void SetExtents(const vtkArrayExtents&);
//ETX

  // Description:
  // Returns true iff the iterator has not reached the end
  // of the range of unique coordinates.
  bool HasNext();

  // Description:
  // Returns the next set of coordinates and advances the iterator.
//BTX
  vtkArrayCoordinates Next();
//ETX

protected:
  vtkArrayCoordinateIterator();
  ~vtkArrayCoordinateIterator();

private:
  vtkArrayCoordinateIterator(const vtkArrayCoordinateIterator&); // Not implemented
  void operator=(const vtkArrayCoordinateIterator&); // Not implemented

  vtkArrayExtents Extents;
  vtkArrayCoordinates Coordinates;
  vtkIdType Current;
  vtkIdType End;
};

#endif

