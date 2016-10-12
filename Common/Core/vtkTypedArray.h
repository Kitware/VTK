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

/**
 * @class   vtkTypedArray
 * @brief   Provides a type-specific interface to N-way arrays
 *
 *
 * vtkTypedArray provides an interface for retrieving and updating data in an
 * arbitrary-dimension array.  It derives from vtkArray and is templated on the
 * type of value stored in the array.
 *
 * Methods are provided for retrieving and updating array values based either
 * on their array coordinates, or on a 1-dimensional integer index.  The latter
 * approach can be used to iterate over the values in an array in arbitrary order,
 * which is useful when writing filters that operate efficiently on sparse arrays
 * and arrays that can have any number of dimensions.
 *
 * Special overloaded methods provide simple access for arrays with one, two, or
 * three dimensions.
 *
 * @sa
 * vtkArray, vtkDenseArray, vtkSparseArray
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
*/

#ifndef vtkTypedArray_h
#define vtkTypedArray_h

#include "vtkArray.h"

class vtkArrayCoordinates;

template<typename T>
class vtkTypedArray : public vtkArray
{
public:
  vtkTemplateTypeMacro(vtkTypedArray<T>, vtkArray)
  typedef typename vtkArray::CoordinateT CoordinateT;
  typedef typename vtkArray::SizeT SizeT;

  using vtkArray::GetVariantValue;
  using vtkArray::SetVariantValue;

  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  // vtkArray API
  vtkVariant GetVariantValue(const vtkArrayCoordinates& coordinates) VTK_OVERRIDE;
  vtkVariant GetVariantValueN(const SizeT n) VTK_OVERRIDE;
  void SetVariantValue(const vtkArrayCoordinates& coordinates, const vtkVariant& value) VTK_OVERRIDE;
  void SetVariantValueN(const SizeT n, const vtkVariant& value) VTK_OVERRIDE;
  void CopyValue(vtkArray* source, const vtkArrayCoordinates& source_coordinates, const vtkArrayCoordinates& target_coordinates) VTK_OVERRIDE;
  void CopyValue(vtkArray* source, const SizeT source_index, const vtkArrayCoordinates& target_coordinates) VTK_OVERRIDE;
  void CopyValue(vtkArray* source, const vtkArrayCoordinates& source_coordinates, const SizeT target_index) VTK_OVERRIDE;

  //@{
  /**
   * Returns the value stored in the array at the given coordinates.
   * Note that the number of dimensions in the supplied coordinates must
   * match the number of dimensions in the array.
   */
  virtual const T& GetValue(CoordinateT i) = 0;
  virtual const T& GetValue(CoordinateT i, CoordinateT j) = 0;
  virtual const T& GetValue(CoordinateT i, CoordinateT j, CoordinateT k) = 0;
  virtual const T& GetValue(const vtkArrayCoordinates& coordinates) = 0;
  //@}

  /**
   * Returns the n-th value stored in the array, where n is in the
   * range [0, GetNonNullSize()).  This is useful for efficiently
   * visiting every value in the array.  Note that the order in which
   * values are visited is undefined, but is guaranteed to match the
   * order used by vtkArray::GetCoordinatesN().
   */
  virtual const T& GetValueN(const SizeT n) = 0;

  //@{
  /**
   * Overwrites the value stored in the array at the given coordinates.
   * Note that the number of dimensions in the supplied coordinates must
   * match the number of dimensions in the array.
   */
  virtual void SetValue(CoordinateT i, const T& value) = 0;
  virtual void SetValue(CoordinateT i, CoordinateT j, const T& value) = 0;
  virtual void SetValue(CoordinateT i, CoordinateT j, CoordinateT k, const T& value) = 0;
  virtual void SetValue(const vtkArrayCoordinates& coordinates, const T& value) = 0;
  //@}

  /**
   * Overwrites the n-th value stored in the array, where n is in the
   * range [0, GetNonNullSize()).  This is useful for efficiently
   * visiting every value in the array.  Note that the order in which
   * values are visited is undefined, but is guaranteed to match the
   * order used by vtkArray::GetCoordinatesN().
   */
  virtual void SetValueN(const SizeT n, const T& value) = 0;

protected:
  vtkTypedArray() {}
  ~vtkTypedArray() VTK_OVERRIDE {}

private:
  vtkTypedArray(const vtkTypedArray&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTypedArray&) VTK_DELETE_FUNCTION;
};

#include "vtkTypedArray.txx"

#endif

// VTK-HeaderTest-Exclude: vtkTypedArray.h
