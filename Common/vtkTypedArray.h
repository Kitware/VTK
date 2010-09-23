/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTypedArray.h

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

// .NAME vtkTypedArray - Provides a type-specific interface to N-way arrays
//
// .SECTION Description
// vtkTypedArray provides an interface for retrieving and updating data in an
// arbitrary-dimension array.  It derives from vtkArray and is templated on the
// type of value stored in the array.
//
// Methods are provided for retrieving and updating array values based either
// on their array coordinates, or on a 1-dimensional integer index.  The latter
// approach can be used to iterate over the values in an array in arbitrary order,
// which is useful when writing filters that operate efficiently on sparse arrays
// and arrays that can have any number of dimensions.
//
// Special overloaded methods provide simple access for arrays with one, two, or
// three dimensions.
//
// .SECTION See Also
// vtkArray, vtkDenseArray, vtkSparseArray
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkTypedArray_h
#define __vtkTypedArray_h

#include "vtkArray.h"
#include "vtkTypeTemplate.h"

class vtkArrayCoordinates;

template<typename T>
class vtkTypedArray : public vtkTypeTemplate<vtkTypedArray<T>, vtkArray>
{
public:
#if defined(_MSC_VER) && _MSC_VER < 1400
  vtkVariant GetVariantValue(vtkIdType i) { return this->vtkArray::GetVariantValue(i); }
  vtkVariant GetVariantValue(vtkIdType i, vtkIdType j) { return this->vtkArray::GetVariantValue(i,j); }
  vtkVariant GetVariantValue(vtkIdType i, vtkIdType j, vtkIdType k) { return this->vtkArray::GetVariantValue(i,j,k); }
  void SetVariantValue(vtkIdType i, const vtkVariant& value) { this->vtkArray::SetVariantValue(i, value); }
  void SetVariantValue(vtkIdType i, vtkIdType j, const vtkVariant& value) { this->vtkArray::SetVariantValue(i,j, value); }
  void SetVariantValue(vtkIdType i, vtkIdType j, vtkIdType k, const vtkVariant& value) { this->vtkArray::SetVariantValue(i,j,k, value); }
#else
  using vtkTypeTemplate<vtkTypedArray<T>, vtkArray>::GetVariantValue;
  using vtkTypeTemplate<vtkTypedArray<T>, vtkArray>::SetVariantValue;
#endif

  void PrintSelf(ostream &os, vtkIndent indent);

  // vtkArray API
  virtual vtkVariant GetVariantValue(const vtkArrayCoordinates& coordinates);
  virtual vtkVariant GetVariantValueN(const vtkIdType n);
  virtual void SetVariantValue(const vtkArrayCoordinates& coordinates, const vtkVariant& value);
  virtual void SetVariantValueN(const vtkIdType n, const vtkVariant& value);
  virtual void CopyValue(vtkArray* source, const vtkArrayCoordinates& source_coordinates, const vtkArrayCoordinates& target_coordinates);
  virtual void CopyValue(vtkArray* source, const vtkIdType source_index, const vtkArrayCoordinates& target_coordinates);
  virtual void CopyValue(vtkArray* source, const vtkArrayCoordinates& source_coordinates, const vtkIdType target_index);

  // Description:
  // Returns the value stored in the array at the given coordinates.
  // Note that the number of dimensions in the supplied coordinates must
  // match the number of dimensions in the array.
  virtual const T& GetValue(vtkIdType i) = 0;
  virtual const T& GetValue(vtkIdType i, vtkIdType j) = 0;
  virtual const T& GetValue(vtkIdType i, vtkIdType j, vtkIdType k) = 0;
  virtual const T& GetValue(const vtkArrayCoordinates& coordinates) = 0;

  // Description:
  // Returns the n-th value stored in the array, where n is in the
  // range [0, GetNonNullSize()).  This is useful for efficiently
  // visiting every value in the array.  Note that the order in which
  // values are visited is undefined, but is guaranteed to match the
  // order used by vtkArray::GetCoordinatesN().
  virtual const T& GetValueN(const vtkIdType n) = 0;

  // Description:
  // Overwrites the value stored in the array at the given coordinates.
  // Note that the number of dimensions in the supplied coordinates must
  // match the number of dimensions in the array.
  virtual void SetValue(vtkIdType i, const T& value) = 0;
  virtual void SetValue(vtkIdType i, vtkIdType j, const T& value) = 0;
  virtual void SetValue(vtkIdType i, vtkIdType j, vtkIdType k, const T& value) = 0;
  virtual void SetValue(const vtkArrayCoordinates& coordinates, const T& value) = 0;

  // Description:
  // Overwrites the n-th value stored in the array, where n is in the
  // range [0, GetNonNullSize()).  This is useful for efficiently
  // visiting every value in the array.  Note that the order in which
  // values are visited is undefined, but is guaranteed to match the
  // order used by vtkArray::GetCoordinatesN().
  virtual void SetValueN(const vtkIdType n, const T& value) = 0;

protected:
  vtkTypedArray() {}
  ~vtkTypedArray() {}

private:
  vtkTypedArray(const vtkTypedArray&); // Not implemented
  void operator=(const vtkTypedArray&); // Not implemented
};

#include "vtkTypedArray.txx"

#endif

