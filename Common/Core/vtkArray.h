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

/**
 * @class   vtkArray
 * @brief   Abstract interface for N-dimensional arrays.
 *
 *
 * vtkArray is the root of a hierarchy of arrays that can be used to
 * store data with any number of dimensions.  It provides an abstract
 * interface for retrieving and setting array attributes that are
 * independent of the type of values stored in the array - such as the
 * number of dimensions, extents along each dimension, and number of
 * values stored in the array.
 *
 * To get and set array values, the vtkTypedArray template class derives
 * from vtkArray and provides type-specific methods for retrieval and
 * update.
 *
 * Two concrete derivatives of vtkTypedArray are provided at the moment:
 * vtkDenseArray and vtkSparseArray, which provide dense and sparse
 * storage for arbitrary-dimension data, respectively.  Toolkit users
 * can create their own concrete derivatives that implement alternative
 * storage strategies, such as compressed-sparse-row, etc.  You could
 * also create an array that provided read-only access to 'virtual' data,
 * such as an array that returned a Fibonacci sequence, etc.
 *
 * @sa
 * vtkTypedArray, vtkDenseArray, vtkSparseArray
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at  Sandia National
 * Laboratories.
*/

#ifndef vtkArray_h
#define vtkArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkArrayCoordinates.h"
#include "vtkArrayExtents.h"
#include "vtkObject.h"
#include "vtkStdString.h"
#include "vtkVariant.h"

class VTKCOMMONCORE_EXPORT vtkArray : public vtkObject
{
public:
  vtkTypeMacro(vtkArray, vtkObject);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  typedef vtkArrayExtents::CoordinateT CoordinateT;
  typedef vtkArrayExtents::DimensionT DimensionT;
  typedef vtkArrayExtents::SizeT SizeT;

  enum
  {
    /// Used with CreateArray() to create dense arrays
    DENSE = 0,
    /// Used with CreateArray() to create sparse arrays
    SPARSE = 1
  };

  /**
   * Creates a new array where StorageType is one of vtkArray::DENSE
   * or vtkArray::SPARSE, and ValueType is one of VTK_CHAR,
   * VTK_UNSIGNED_CHAR, VTK_SHORT, VTK_UNSIGNED_SHORT,  VTK_INT,
   * VTK_UNSIGNED_INT, VTK_LONG, VTK_UNSIGNED_LONG, VTK_DOUBLE,
   * VTK_ID_TYPE, or VTK_STRING.  The caller is responsible for the
   * lifetime of the returned object.
   */
  VTK_NEWINSTANCE
  static vtkArray* CreateArray(int StorageType, int ValueType);

  /**
   * Returns true iff the underlying array storage is "dense", i.e. that
   * GetSize() and GetNonNullSize() will always return the same value.
   * If not, the array is "sparse".
   */
  virtual bool IsDense() = 0;

  //@{
  /**
   * Resizes the array to the given extents (number of dimensions and
   * size of each dimension).  Note that concrete implementations of
   * vtkArray may place constraints on the the extents that they will
   * store, so you cannot assume that GetExtents() will always return
   * the same value passed to Resize().

   * The contents of the array are undefined after calling Resize() - you
   * should initialize its contents accordingly.  In particular,
   * dimension-labels will be undefined, dense array values will be
   * undefined, and sparse arrays will be empty.
   */
  void Resize(const CoordinateT i);
  void Resize(const CoordinateT i, const CoordinateT j);
  void Resize(const CoordinateT i, const CoordinateT j, const CoordinateT k);
  void Resize(const vtkArrayRange& i);
  void Resize(const vtkArrayRange& i, const vtkArrayRange& j);
  void Resize(const vtkArrayRange& i, const vtkArrayRange& j, const vtkArrayRange& k);
  void Resize(const vtkArrayExtents& extents);
  //@}

  /**
   * Returns the extent (valid coordinate range) along the given
   * dimension.
   */
  const vtkArrayRange GetExtent(DimensionT dimension);
  /**
   * Returns the extents (the number of dimensions and size along each
   * dimension) of the array.
   */
  virtual const vtkArrayExtents& GetExtents() = 0;

  /**
   * Returns the number of dimensions stored in the array.  Note that
   * this is the same as calling GetExtents().GetDimensions().
   */
  DimensionT GetDimensions();

  /**
   * Returns the number of values stored in the array.  Note that this is
   * the same as calling GetExtents().GetSize(), and represents the
   * maximum number of values that could ever be stored using the current
   * extents.  This is equal to the number of values stored in a  dense
   * array, but may be larger than the number of values stored in a
   * sparse array.
   */
  SizeT GetSize();

  /**
   * Returns the number of non-null values stored in the array.  Note
   * that this value will equal GetSize() for dense arrays, and will be
   * less-than-or-equal to GetSize() for sparse arrays.
   */
  virtual SizeT GetNonNullSize() = 0;

  /**
   * Sets the array name.
   */
  void SetName(const vtkStdString& name);
  /**
   * Returns the array name.
   */
  vtkStdString GetName();

  /**
   * Sets the label for the i-th array dimension.
   */
  void SetDimensionLabel(DimensionT i, const vtkStdString& label);

  /**
   * Returns the label for the i-th array dimension.
   */
  vtkStdString GetDimensionLabel(DimensionT i);

  /**
   * Returns the coordinates of the n-th value in the array, where n is
   * in the range [0, GetNonNullSize()).  Note that the order in which
   * coordinates are visited is undefined, but is guaranteed to match the
   * order in which values are visited using vtkTypedArray::GetValueN()
   * and vtkTypedArray::SetValueN().
   */
  virtual void GetCoordinatesN(const SizeT n, vtkArrayCoordinates& coordinates) = 0;

  //@{
  /**
   * Returns the value stored in the array at the given coordinates.
   * Note that the number of dimensions in the supplied coordinates must
   * match the number of dimensions in the array.
   */
  inline vtkVariant GetVariantValue(CoordinateT i);
  inline vtkVariant GetVariantValue(CoordinateT i, CoordinateT j);
  inline vtkVariant GetVariantValue(CoordinateT i, CoordinateT j, CoordinateT k);
  virtual vtkVariant GetVariantValue(const vtkArrayCoordinates& coordinates) = 0;
  //@}

  /**
   * Returns the n-th value stored in the array, where n is in the
   * range [0, GetNonNullSize()).  This is useful for efficiently
   * visiting every value in the array.  Note that the order in which
   * values are visited is undefined, but is guaranteed to match the
   * order used by vtkArray::GetCoordinatesN().
   */
  virtual vtkVariant GetVariantValueN(const SizeT n) = 0;

  //@{
  /**
   * Overwrites the value stored in the array at the given coordinates.
   * Note that the number of dimensions in the supplied coordinates must
   * match the number of dimensions in the array.
   */
  inline void SetVariantValue(CoordinateT i, const vtkVariant& value);
  inline void SetVariantValue(CoordinateT i, CoordinateT j, const vtkVariant& value);
  inline void SetVariantValue(CoordinateT i, CoordinateT j, CoordinateT k, const vtkVariant& value);
  virtual void SetVariantValue(const vtkArrayCoordinates& coordinates, const vtkVariant& value) = 0;
  //@}

  /**
   * Overwrites the n-th value stored in the array, where n is in the
   * range [0, GetNonNullSize()).  This is useful for efficiently
   * visiting every value in the array.  Note that the order in which
   * values are visited is undefined, but is guaranteed to match the
   * order used by vtkArray::GetCoordinatesN().
   */
  virtual void SetVariantValueN(const SizeT n, const vtkVariant& value) = 0;

  //@{
  /**
   * Overwrites a value with a value retrieved from another array.  Both
   * arrays must store the same data types.
   */
  virtual void CopyValue(vtkArray* source, const vtkArrayCoordinates& source_coordinates, const vtkArrayCoordinates& target_coordinates) = 0;
  virtual void CopyValue(vtkArray* source, const SizeT source_index, const vtkArrayCoordinates& target_coordinates) = 0;
  virtual void CopyValue(vtkArray* source, const vtkArrayCoordinates& source_coordinates, const SizeT target_index) = 0;
  //@}

  /**
   * Returns a new array that is a deep copy of this array.
   */
  virtual vtkArray* DeepCopy() = 0;

protected:
  vtkArray();
  ~vtkArray() VTK_OVERRIDE;

private:
  vtkArray(const vtkArray&) VTK_DELETE_FUNCTION;
  void operator=(const vtkArray&) VTK_DELETE_FUNCTION;

  /**
   * Stores the array name.
   */
  vtkStdString Name;

  /**
   * Implemented in concrete derivatives to update their storage
   * when the array is resized.
   */
  virtual void InternalResize(const vtkArrayExtents&) = 0;

  /**
   * Implemented in concrete derivatives to set dimension labels.
   */
  virtual void InternalSetDimensionLabel(DimensionT i, const vtkStdString& label) = 0;

  //@{
  /**
   * Implemented in concrete derivatives to get dimension labels.
   */
  virtual vtkStdString InternalGetDimensionLabel(DimensionT i) = 0;
};
  //@}

vtkVariant vtkArray::GetVariantValue(CoordinateT i)
{
  return this->GetVariantValue(vtkArrayCoordinates(i));
}

vtkVariant vtkArray::GetVariantValue(CoordinateT i, CoordinateT j)
{
  return this->GetVariantValue(vtkArrayCoordinates(i, j));
}

vtkVariant vtkArray::GetVariantValue(CoordinateT i, CoordinateT j, CoordinateT k)
{
  return this->GetVariantValue(vtkArrayCoordinates(i, j, k));
}

void vtkArray::SetVariantValue(CoordinateT i, const vtkVariant& value)
{
  this->SetVariantValue(vtkArrayCoordinates(i), value);
}

void vtkArray::SetVariantValue(CoordinateT i, CoordinateT j, const vtkVariant& value)
{
  this->SetVariantValue(vtkArrayCoordinates(i, j), value);
}

void vtkArray::SetVariantValue(CoordinateT i, CoordinateT j, CoordinateT k, const vtkVariant& value)
{
  this->SetVariantValue(vtkArrayCoordinates(i, j, k), value);
}

#endif

// VTK-HeaderTest-Exclude: vtkArray.h
