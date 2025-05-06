//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_internal_ArrayPortalBasic_h
#define viskores_internal_ArrayPortalBasic_h

#include <viskores/Assert.h>
#include <viskores/Types.h>

#ifdef VISKORES_CUDA
// CUDA devices have special instructions for faster data loading
#include <viskores/exec/cuda/internal/ArrayPortalBasicCuda.h>
#endif // VISKORES_CUDA

namespace viskores
{
namespace internal
{

namespace detail
{

// These templated methods can be overloaded for special access to data.

template <typename T>
VISKORES_EXEC_CONT static inline T ArrayPortalBasicReadGet(const T* const data)
{
  return *data;
}

template <typename T>
VISKORES_EXEC_CONT static inline T ArrayPortalBasicWriteGet(const T* const data)
{
  return *data;
}

template <typename T>
VISKORES_EXEC_CONT static inline void ArrayPortalBasicWriteSet(T* data, const T& value)
{
  *data = value;
}

} // namespace detail

template <typename T>
class ArrayPortalBasicRead
{
  const T* Array = nullptr;
  viskores::Id NumberOfValues = 0;

public:
  using ValueType = T;

  VISKORES_EXEC_CONT viskores::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VISKORES_EXEC_CONT ValueType Get(viskores::Id index) const
  {
    VISKORES_ASSERT(index >= 0);
    VISKORES_ASSERT(index < this->NumberOfValues);

    return detail::ArrayPortalBasicReadGet(this->Array + index);
  }

  VISKORES_EXEC_CONT const ValueType* GetIteratorBegin() const { return this->Array; }
  VISKORES_EXEC_CONT const ValueType* GetIteratorEnd() const
  {
    return this->Array + this->NumberOfValues;
  }

  VISKORES_EXEC_CONT const ValueType* GetArray() const { return this->Array; }

  ArrayPortalBasicRead() = default;
  ArrayPortalBasicRead(ArrayPortalBasicRead&&) = default;
  ArrayPortalBasicRead(const ArrayPortalBasicRead&) = default;
  ArrayPortalBasicRead& operator=(ArrayPortalBasicRead&&) = default;
  ArrayPortalBasicRead& operator=(const ArrayPortalBasicRead&) = default;

  VISKORES_EXEC_CONT ArrayPortalBasicRead(const T* array, viskores::Id numberOfValues)
    : Array(array)
    , NumberOfValues(numberOfValues)
  {
  }
};

template <typename T>
class ArrayPortalBasicWrite
{
  T* Array = nullptr;
  viskores::Id NumberOfValues = 0;

public:
  using ValueType = T;

  VISKORES_EXEC_CONT viskores::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VISKORES_EXEC_CONT ValueType Get(viskores::Id index) const
  {
    VISKORES_ASSERT(index >= 0);
    VISKORES_ASSERT(index < this->NumberOfValues);

    return detail::ArrayPortalBasicWriteGet(this->Array + index);
  }

  VISKORES_EXEC_CONT void Set(viskores::Id index, const ValueType& value) const
  {
    VISKORES_ASSERT(index >= 0);
    VISKORES_ASSERT(index < this->NumberOfValues);

    detail::ArrayPortalBasicWriteSet(this->Array + index, value);
  }

  VISKORES_EXEC_CONT ValueType* GetIteratorBegin() const { return this->Array; }
  VISKORES_EXEC_CONT ValueType* GetIteratorEnd() const
  {
    return this->Array + this->NumberOfValues;
  }

  VISKORES_EXEC_CONT ValueType* GetArray() const { return this->Array; }

  ArrayPortalBasicWrite() = default;
  ArrayPortalBasicWrite(ArrayPortalBasicWrite&&) = default;
  ArrayPortalBasicWrite(const ArrayPortalBasicWrite&) = default;
  ArrayPortalBasicWrite& operator=(ArrayPortalBasicWrite&&) = default;
  ArrayPortalBasicWrite& operator=(const ArrayPortalBasicWrite&) = default;

  VISKORES_EXEC_CONT ArrayPortalBasicWrite(T* array, viskores::Id numberOfValues)
    : Array(array)
    , NumberOfValues(numberOfValues)
  {
  }
};
}
} // namespace viskores::internal

#endif //viskores_internal_ArrayPortalBasic_h
