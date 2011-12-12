/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkVectorBase - templated base type for sized vector containers.
//
// .SECTION Description
// This class is a templated data type for storing and manipulating fixed
// size vectors.

#ifndef __vtkVectorBase_h
#define __vtkVectorBase_h

#include <cassert> // For inline assert for bounds checked methods.

template<typename T, int Size>
class vtkVectorBase
{
public:
  vtkVectorBase()
  {
  }

  // Description:
  // Initialize all of the vector's elements with the supplied scalar.
  explicit vtkVectorBase(const T& scalar)
  {
    for (int i = 0; i < Size; ++i)
      {
      this->Data[i] = scalar;
      }
  }

  // Description:
  // Initalize the vector's elements with the elements of the supplied array.
  // Note that the supplied pointer must contain at least as many elements as
  // the vector, or it will result in access to out of bounds memory.
  explicit vtkVectorBase(const T* init)
  {
    for (int i = 0; i < Size; ++i)
      {
      this->Data[i] = init[i];
      }
  }

  // Description:
  // Get the size of the vtkVector.
  int GetSize() const { return Size; }

  // Description:
  // Get a pointer to the underlying data of the vtkVector.
  T* GetData() { return this->Data; }
  const T* GetData() const { return this->Data; }

  // Description:
  // Get a reference to the underlying data element of the vtkVector. Can be
  // used in much the same way as vector[i] is used.
  T& operator[](int i) { return this->Data[i]; }
  const T& operator[](int i) const { return this->Data[i]; }

  // Description:
  // Get the value of the vector at the index speciifed. Does bounds checking,
  // used in much the same way as vector.at(i) is used.
  T operator()(int i) const
  {
    assert("pre: index_in_bounds" && i >= 0 && i < Size);
    return this->Data[i];
  }

  // Description:
  // Equality operator with a tolerance to allow fuzzy comparisons.
  bool Compare(const vtkVectorBase<T, Size>& other, const T& tol) const
  {
    if (Size != other.GetSize())
      {
      return false;
      }
    for (int i = 0; i < Size; ++i)
      {
      if (fabs(this->Data[i] - other.Data[i]) >= tol)
        {
        return false;
        }
      }
    return true;
  }

  // Description:
  // Cast the vector to the specified type, returning the result.
  template<typename TR>
  vtkVectorBase<TR, Size> Cast() const
  {
    vtkVectorBase<TR, Size> result;
    for (int i = 0; i < Size; ++i)
      {
      result[i] = static_cast<TR>(this->Data[i]);
      }
    return result;
  }

protected:
  // Description:
  // The only thing stored in memory!
  T Data[Size];
};

#endif // __vtkVectorBase_h
