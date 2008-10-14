/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDenseArray.h
  
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

// .NAME vtkDenseArray - vtkArray implementation that stores an N-way
// array using contiguous storage.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkDenseArray_h
#define __vtkDenseArray_h

#include "vtkArrayCoordinates.h"
#include "vtkObjectFactory.h"
#include "vtkTypedArray.h"
#include "vtkTypeTemplate.h"

template<typename T>
class vtkDenseArray :
  public vtkTypeTemplate<vtkDenseArray<T>, vtkTypedArray<T> >
{
public:
  using vtkTypedArray<T>::GetValue;
  using vtkTypedArray<T>::SetValue;

  static vtkDenseArray<T>* New();
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

  // vtkDenseArray API

  // Description:
  // Fills every element in the array with the given value.
  void Fill(const T& value);

  // Description:
  // Returns a value by-reference, which is useful for performance and code-clarity.
  T& operator[](const vtkArrayCoordinates& coordinates);

  // Description:
  // Returns a read-only reference to the underlying storage.  Values are stored
  // contiguously with fortran ordering.
  const T* GetStorage() const;
  
  // Description:
  // Returns a mutable reference to the underlying storage.  Values are stored
  // contiguously with fortran ordering.  Use at your own risk!
  T* GetStorage();

protected:
  vtkDenseArray();
  ~vtkDenseArray();

private:
  vtkDenseArray(const vtkDenseArray&); // Not implemented
  void operator=(const vtkDenseArray&); // Not implemented

  void InternalResize(const vtkArrayExtents& extents);
  void InternalSetDimensionLabel(vtkIdType i, const vtkStdString& label);
  vtkStdString InternalGetDimensionLabel(vtkIdType i);
  vtkIdType MapCoordinates(const vtkArrayCoordinates& coordinates);

  typedef vtkDenseArray<T> ThisT;

  // Description:
  // Stores the current array extents (its size along each dimension)
  vtkArrayExtents Extents;
  
  // Description:
  // Stores labels for each array dimension
  vtkstd::vector<vtkStdString> DimensionLabels;
  
  // Description:
  // Stores the current array values using a contiguous range of memory
  // with constant-time value lookup.
  vtkstd::vector<T> Storage;
  
  // Description:
  // Stores the strides along each array dimension (used for fast lookups).
  vtkstd::vector<vtkIdType> Strides;
};

#include "vtkDenseArray.txx"

#endif

