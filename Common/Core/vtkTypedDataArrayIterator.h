// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkTypedDataArrayIterator
 * @brief   STL-style random access iterator for
 * vtkTypedDataArrays.
 *
 *
 * vtkTypedDataArrayIterator provides an STL-style iterator that can be used to
 * interact with instances of vtkTypedDataArray. It is intended to provide an
 * alternative to using vtkDataArray::GetVoidPointer() that only uses
 * vtkTypedDataArray API functions to retrieve values. It is especially helpful
 * for safely iterating through subclasses of vtkMappedDataArray, which may
 * not use the same memory layout as a typical vtkDataArray.
 *
 * NOTE: This class has been superseded by the newer vtkGenericDataArray and
 * vtkArrayDispatch mechanism.
 */

#ifndef vtkTypedDataArrayIterator_h
#define vtkTypedDataArrayIterator_h

#include <iterator> // For iterator traits

#include "vtkDeprecation.h"    // For VTK_DEPRECATED_IN_9_5_0
#include "vtkTypedDataArray.h" // For vtkTypedDataArray

VTK_ABI_NAMESPACE_BEGIN
template <class Scalar>
class VTK_DEPRECATED_IN_9_5_0(
  "This iterator is deprecated because vtkTypedDataArray is deprecated.") vtkTypedDataArrayIterator
{
public:
  typedef std::random_access_iterator_tag iterator_category;
  typedef Scalar value_type;
  typedef std::ptrdiff_t difference_type;
  typedef Scalar& reference;
  typedef Scalar* pointer;

  vtkTypedDataArrayIterator()
    : Data(nullptr)
    , Index(0)
  {
  }

  explicit vtkTypedDataArrayIterator(vtkTypedDataArray<Scalar>* arr, const vtkIdType index = 0)
    : Data(arr)
    , Index(index)
  {
  }

  vtkTypedDataArrayIterator(const vtkTypedDataArrayIterator& o)
    : Data(o.Data)
    , Index(o.Index)
  {
  }

  vtkTypedDataArrayIterator& operator=(vtkTypedDataArrayIterator<Scalar> o)
  {
    std::swap(this->Data, o.Data);
    std::swap(this->Index, o.Index);
    return *this;
  }

  bool operator==(const vtkTypedDataArrayIterator<Scalar>& o) const
  {
    return this->Data == o.Data && this->Index == o.Index;
  }

  bool operator!=(const vtkTypedDataArrayIterator<Scalar>& o) const
  {
    return this->Data == o.Data && this->Index != o.Index;
  }

  bool operator>(const vtkTypedDataArrayIterator<Scalar>& o) const
  {
    return this->Data == o.Data && this->Index > o.Index;
  }

  bool operator>=(const vtkTypedDataArrayIterator<Scalar>& o) const
  {
    return this->Data == o.Data && this->Index >= o.Index;
  }

  bool operator<(const vtkTypedDataArrayIterator<Scalar>& o) const
  {
    return this->Data == o.Data && this->Index < o.Index;
  }

  bool operator<=(const vtkTypedDataArrayIterator<Scalar>& o) const
  {
    return this->Data == o.Data && this->Index <= o.Index;
  }

  Scalar& operator*() { return this->Data->GetValueReference(this->Index); }

  Scalar* operator->() const { return &this->Data->GetValueReference(this->Index); }

  Scalar& operator[](const difference_type& n)
  {
    return this->Data->GetValueReference(this->Index + n);
  }

  vtkTypedDataArrayIterator& operator++()
  {
    ++this->Index;
    return *this;
  }

  vtkTypedDataArrayIterator& operator--()
  {
    --this->Index;
    return *this;
  }

  vtkTypedDataArrayIterator operator++(int)
  {
    return vtkTypedDataArrayIterator(this->Data, this->Index++);
  }

  vtkTypedDataArrayIterator operator--(int)
  {
    return vtkTypedDataArrayIterator(this->Data, this->Index--);
  }

  vtkTypedDataArrayIterator operator+(const difference_type& n) const
  {
    return vtkTypedDataArrayIterator(this->Data, this->Index + n);
  }

  vtkTypedDataArrayIterator operator-(const difference_type& n) const
  {
    return vtkTypedDataArrayIterator(this->Data, this->Index - n);
  }

  difference_type operator-(const vtkTypedDataArrayIterator& other) const
  {
    return this->Index - other.Index;
  }

  vtkTypedDataArrayIterator& operator+=(const difference_type& n)
  {
    this->Index += n;
    return *this;
  }

  vtkTypedDataArrayIterator& operator-=(const difference_type& n)
  {
    this->Index -= n;
    return *this;
  }

private:
  vtkTypedDataArray<Scalar>* Data;
  vtkIdType Index;
};

VTK_ABI_NAMESPACE_END
#endif // vtkTypedDataArrayIterator_h

// VTK-HeaderTest-Exclude: vtkTypedDataArrayIterator.h
