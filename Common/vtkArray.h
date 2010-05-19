/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArray.h
  
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

// .NAME vtkArray - Abstract interface for N-dimensional arrays.
//
// .SECTION Description
// vtkArray is the root of a hierarchy of arrays that can be used to store data with
// any number of dimensions.  It provides an abstract interface for retrieving and
// setting array attributes that are independent of the type of values stored in the
// array - such as the number of dimensions, extents along each dimension, and number
// of values stored in the array.
//
// To get and set array values, the vtkTypedArray template class derives from vtkArray
// and provides type-specific methods for retrieval and update.
//
// Two concrete derivatives of vtkTypedArray are provided at the moment: vtkDenseArray
// and vtkSparseArray, which provide dense and sparse storage for arbitrary-dimension
// data, respectively.  Toolkit users can create their own concrete derivatives that
// implement alternative storage strategies, such as compressed-sparse-row, etc.  You
// could also create an array that provided read-only access to 'virtual' data, such
// as an array that returned a Fibonacci sequence, etc.
//
// .SECTION See Also
// vtkTypedArray, vtkDenseArray, vtkSparseArray
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkArray_h
#define __vtkArray_h

#include "vtkArrayCoordinates.h"
#include "vtkArrayExtents.h"
#include "vtkObject.h"
#include "vtkStdString.h"
#include "vtkVariant.h"

class VTK_COMMON_EXPORT vtkArray : public vtkObject
{
public:
  vtkTypeMacro(vtkArray, vtkObject);
  void PrintSelf(ostream &os, vtkIndent indent);

//BTX
  enum
  {
    /// Used with CreateArray() to create dense arrays
    DENSE = 0,
    /// Used with CreateArray() to create sparse arrays
    SPARSE = 1
  };
//ETX

  // Description:
  // Creates a new array where StorageType is one of vtkArray::DENSE or vtkArray::SPARSE, and
  // ValueType is one of VTK_CHAR, VTK_UNSIGNED_CHAR, VTK_SHORT, VTK_UNSIGNED_SHORT,
  // VTK_INT, VTK_UNSIGNED_INT, VTK_LONG, VTK_UNSIGNED_LONG, VTK_DOUBLE, VTK_ID_TYPE,
  // or VTK_STRING.  The caller is responsible for the lifetime of the returned object.
  static vtkArray* CreateArray(int StorageType, int ValueType);

  // Description:
  // Returns true iff the underlying array storage is "dense", i.e. that GetSize() and
  // GetNonNullSize() will always return the same value.  If not, the array is "sparse".
  virtual bool IsDense() = 0;

  // Description:
  // Resizes the array to the given extents (number of dimensions and size of each dimension).
  // Note that concrete implementations of vtkArray may place constraints on the the extents
  // that they will store, so you cannot assume that GetExtents() will always return the same
  // value passed to Resize().
  //
  // The contents of the array are undefined after calling Resize() - you should
  // initialize its contents accordingly.  In particular, dimension-labels will be
  // undefined, dense array values will be undefined, and sparse arrays will be
  // empty.
  void Resize(const vtkIdType i);
  void Resize(const vtkIdType i, const vtkIdType j);
  void Resize(const vtkIdType i, const vtkIdType j, const vtkIdType k);
//BTX
  void Resize(const vtkArrayRange& i);
  void Resize(const vtkArrayRange& i, const vtkArrayRange& j);
  void Resize(const vtkArrayRange& i, const vtkArrayRange& j, const vtkArrayRange& k);
  void Resize(const vtkArrayExtents& extents);
//ETX

//BTX
  // Description:
  // Returns the extent (valid coordinate range) along the given dimension.
  const vtkArrayRange GetExtent(vtkIdType dimension);
  // Description:
  // Returns the extents (the number of dimensions and size along each dimension) of the array.
  virtual const vtkArrayExtents& GetExtents() = 0;
//ETX

  // Description:
  // Returns the number of dimensions stored in the array.  Note that this is the same as
  // calling GetExtents().GetDimensions().
  vtkIdType GetDimensions();
  
  // Description:
  // Returns the number of values stored in the array.  Note that this is the same as calling
  // GetExtents().GetSize(), and represents the maximum number of values that could ever
  // be stored using the current extents.  This is equal to the number of values stored in a
  // dense array, but may be larger than the number of values stored in a sparse array.
  vtkIdType GetSize();
  
  // Description:
  // Returns the number of non-null values stored in the array.  Note that this
  // value will equal GetSize() for dense arrays, and will be less-than-or-equal
  // to GetSize() for sparse arrays.
  virtual vtkIdType GetNonNullSize() = 0;

  // Description:
  // Sets the array name.
  void SetName(const vtkStdString& name);
  // Description:
  // Returns the array name.
  vtkStdString GetName();

  // Description:
  // Sets the label for the i-th array dimension.
  void SetDimensionLabel(vtkIdType i, const vtkStdString& label);
  
  // Description:
  // Returns the label for the i-th array dimension.
  vtkStdString GetDimensionLabel(vtkIdType i);

  //BTX
  // Description:
  // Returns the coordinates of the n-th value in the array, where n is in the
  // range [0, GetNonNullSize()).  Note that the order in which coordinates are visited
  // is undefined, but is guaranteed to match the order in which values are visited using
  // vtkTypedArray::GetValueN() and vtkTypedArray::SetValueN().
  virtual void GetCoordinatesN(const vtkIdType n, vtkArrayCoordinates& coordinates) = 0;

  // Description:
  // Returns the value stored in the array at the given coordinates.
  // Note that the number of dimensions in the supplied coordinates must
  // match the number of dimensions in the array.
  inline vtkVariant GetVariantValue(vtkIdType i);
  inline vtkVariant GetVariantValue(vtkIdType i, vtkIdType j);
  inline vtkVariant GetVariantValue(vtkIdType i, vtkIdType j, vtkIdType k);
  virtual vtkVariant GetVariantValue(const vtkArrayCoordinates& coordinates) = 0;
  
  // Description:
  // Returns the n-th value stored in the array, where n is in the
  // range [0, GetNonNullSize()).  This is useful for efficiently
  // visiting every value in the array.  Note that the order in which
  // values are visited is undefined, but is guaranteed to match the
  // order used by vtkArray::GetCoordinatesN().
  virtual vtkVariant GetVariantValueN(const vtkIdType n) = 0;
  
  // Description:
  // Overwrites the value stored in the array at the given coordinates.
  // Note that the number of dimensions in the supplied coordinates must
  // match the number of dimensions in the array.
  inline void SetVariantValue(vtkIdType i, const vtkVariant& value);
  inline void SetVariantValue(vtkIdType i, vtkIdType j, const vtkVariant& value);
  inline void SetVariantValue(vtkIdType i, vtkIdType j, vtkIdType k, const vtkVariant& value);
  virtual void SetVariantValue(const vtkArrayCoordinates& coordinates, const vtkVariant& value) = 0;
  
  // Description:
  // Overwrites the n-th value stored in the array, where n is in the
  // range [0, GetNonNullSize()).  This is useful for efficiently
  // visiting every value in the array.  Note that the order in which
  // values are visited is undefined, but is guaranteed to match the
  // order used by vtkArray::GetCoordinatesN().
  virtual void SetVariantValueN(const vtkIdType n, const vtkVariant& value) = 0;

  // Description:
  // Overwrites a value with a value retrieved from another array.  Both
  // arrays must store the same data types.
  virtual void CopyValue(vtkArray* source, const vtkArrayCoordinates& source_coordinates, const vtkArrayCoordinates& target_coordinates) = 0;
  virtual void CopyValue(vtkArray* source, const vtkIdType source_index, const vtkArrayCoordinates& target_coordinates) = 0;
  virtual void CopyValue(vtkArray* source, const vtkArrayCoordinates& source_coordinates, const vtkIdType target_index) = 0;
  //ETX

  // Description:
  // Returns a new array that is a deep copy of this array.
  virtual vtkArray* DeepCopy() = 0;

protected:
  vtkArray();
  ~vtkArray();

private:
  vtkArray(const vtkArray&); // Not implemented
  void operator=(const vtkArray&); // Not implemented

  // Description:
  // Stores the array name.
  vtkStdString Name;

  // Description:
  // Implemented in concrete derivatives to update their storage
  // when the array is resized.
  virtual void InternalResize(const vtkArrayExtents&) = 0;
  
  // Description:
  // Implemented in concrete derivatives to set dimension labels.
  virtual void InternalSetDimensionLabel(vtkIdType i, const vtkStdString& label) = 0;
  
  // Description:
  // Implemented in concrete derivatives to get dimension labels.
  virtual vtkStdString InternalGetDimensionLabel(vtkIdType i) = 0;
};

vtkVariant vtkArray::GetVariantValue(vtkIdType i)
{
  return this->GetVariantValue(vtkArrayCoordinates(i));
}

vtkVariant vtkArray::GetVariantValue(vtkIdType i, vtkIdType j)
{
  return this->GetVariantValue(vtkArrayCoordinates(i, j));
}

vtkVariant vtkArray::GetVariantValue(vtkIdType i, vtkIdType j, vtkIdType k)
{
  return this->GetVariantValue(vtkArrayCoordinates(i, j, k));
}

void vtkArray::SetVariantValue(vtkIdType i, const vtkVariant& value)
{
  this->SetVariantValue(vtkArrayCoordinates(i), value);
}

void vtkArray::SetVariantValue(vtkIdType i, vtkIdType j, const vtkVariant& value)
{
  this->SetVariantValue(vtkArrayCoordinates(i, j), value);
}

void vtkArray::SetVariantValue(vtkIdType i, vtkIdType j, vtkIdType k, const vtkVariant& value)
{
  this->SetVariantValue(vtkArrayCoordinates(i, j, k), value);
}

#endif

