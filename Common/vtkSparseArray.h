/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSparseArray.h
  
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

// .NAME vtkSparseArray - vtkArray implementation that stores an N-way
// array using sparse storage.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkSparseArray_h
#define __vtkSparseArray_h

#include "vtkArrayCoordinates.h"
#include "vtkObjectFactory.h"
#include "vtkTypeTemplate.h"
#include "vtkTypedArray.h"

template<typename T>
class vtkSparseArray :
  public vtkTypeTemplate<vtkSparseArray<T>, vtkTypedArray<T> >
{
public:
  using vtkTypedArray<T>::GetValue;
  using vtkTypedArray<T>::SetValue;

  static vtkSparseArray<T>* New();
  void PrintSelf(ostream &os, vtkIndent indent);

  // vtkArray API 
  vtkArrayExtents GetExtents();
  vtkIdType GetNonNullSize();
  void GetCoordinatesN(const vtkIdType n, vtkArrayCoordinates& coordinates);
  vtkArray* DeepCopy();
    
  // vtkTypedArray API
  const T& GetValue(const vtkArrayCoordinates& coordinates);
  const T& GetValueN(const vtkIdType n);
  void SetValue(const vtkArrayCoordinates& coordinates, const T& value);
  void SetValueN(const vtkIdType n, const T& value);

  // vtkSparseArray API

  // Description:
  // Set the value that will be returned by GetValue() for NULL areas of the array.
  void SetNullValue(const T& value);

  // Description:
  // Returns the value that will be returned by GetValue() for NULL areas of the array.
  const T& GetNullValue();

  // Description:
  // Remove all non-null elements from the array, leaving the number of dimensions, the
  // extent of each dimension, and the label for each dimension unchanged.
  void Clear();

  // Description:
  // Return a read-only reference to the underlying coordinate storage.  Coordinates
  // are stored contiguously as a one-dimensional array with the coordinates for each value
  // stored adjacent to one-another.  The ordering of coordinates is arbitrary.
  const vtkIdType* GetCoordinateStorage() const;
  
  // Description:
  // Return a mutable reference to the underlying coordinate storage.  Coordinates
  // are stored contiguously as a one-dimensional array with the coordinates for each value
  // stored adjacent to one-another.  The ordering of coordinates is arbitrary.
  // Use at your own risk!
  vtkIdType* GetCoordinateStorage();
  
  // Description:
  // Return a read-only reference to the underlying value storage.  Values are stored
  // contiguously, but in arbitrary order.  Use GetCoordinateStorage() if you need to
  // get the corresponding coordinates for a value.
  const T* GetValueStorage() const;
  
  // Description:
  // Return a mutable reference to the underlying value storage.  Values are stored
  // contiguously, but in arbitrary order.  Use GetCoordinateStorage() if you need to
  // get the corresponding coordinates for a value.  Use at your own risk!
  T* GetValueStorage();
  
  // Description:
  // Update the array extents to match its contents, so that the extent along each dimension
  // matches the maximum index value along that dimension.
  void ResizeToContents();
  
  // Description:
  // Adds a new non-null element to the array.  Does not test to see if an element with
  // matching coordinates already exists.  Useful for providing fast initialization of the
  // array as long as the caller is prepared to guarantee that no duplicate coordinates are
  // ever used.
  inline void AddValue(vtkIdType i, const T& value);
  inline void AddValue(vtkIdType i, vtkIdType j, const T& value);
  inline void AddValue(vtkIdType i, vtkIdType j, vtkIdType k, const T& value);
  void AddValue(const vtkArrayCoordinates& coordinates, const T& value);

protected:
  vtkSparseArray();
  ~vtkSparseArray();

private:
  vtkSparseArray(const vtkSparseArray&); // Not implemented
  void operator=(const vtkSparseArray&); // Not implemented

  void InternalResize(const vtkArrayExtents& extents);
  void InternalSetDimensionLabel(vtkIdType i, const vtkStdString& label);
  vtkStdString InternalGetDimensionLabel(vtkIdType i);

  typedef vtkSparseArray<T> ThisT;

  // Description:
  // Stores the current array extents (size along each dimension)
  vtkArrayExtents Extents;
  
  // Description:
  // Stores a label for each array dimension
  vtkstd::vector<vtkStdString> DimensionLabels;
  
  // Description:
  // Stores the coordinates of each non-null element within the array
  // as a contiguous block of values organized into a row-major ("C")
  // 2D array.
  vtkstd::vector<vtkIdType> Coordinates;
  
  // Description:
  // Stores the value of each non-null element within the array
  vtkstd::vector<T> Values;

  // Description:
  // Stores the value that will be returned when accessing NULL areas
  // of the array.
  T NullValue;
};

#include "vtkSparseArray.txx"

#endif

