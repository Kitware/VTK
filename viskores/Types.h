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
#ifndef viskores_Types_h
#define viskores_Types_h

#include <viskores/internal/Configure.h>
#include <viskores/internal/ExportMacros.h>

#include <viskores/Assert.h>
#include <viskores/StaticAssert.h>

#include <cstdint>
#include <iostream>
#include <type_traits>

/*!
 * \namespace viskores
 * \brief Viskores Toolkit.
 *
 * viskores is the namespace for the Viskores Toolkit. It contains other sub namespaces,
 * as well as basic data types and functions callable from all components in Viskores
 * toolkit.
 *
 * \namespace viskores::cont
 * \brief Viskores Control Environment.
 *
 * viskores::cont defines the publicly accessible API for the Viskores Control
 * Environment. Users of the Viskores Toolkit can use this namespace to access the
 * Control Environment.
 *
 * \namespace viskores::cont::arg
 * \brief Transportation controls for Control Environment Objects.
 *
 * viskores::cont::arg includes the classes that allows the viskores::worklet::Dispatchers
 * to request Control Environment Objects to be transferred to the Execution Environment.
 *
 * \namespace viskores::cont::cuda
 * \brief CUDA implementation for Control Environment.
 *
 * viskores::cont::cuda includes the code to implement the Viskores Control Environment
 * for the CUDA-based device adapter.
 *
 * \namespace viskores::cont::openmp
 * \brief OPenMP implementation for Control Environment.
 *
 * viskores::cont::openmp includes the code to implement the Viskores Control Environment
 * for the OpenMP-based device adapter.
 *
 * \namespace viskores::cont::serial
 * \brief Serial implementation for Control Environment.
 *
 * viskores::cont::serial includes the code to implement the Viskores Control Environment
 * for the serial device adapter.
 *
 * \namespace viskores::cont::tbb
 * \brief TBB implementation for Control Environment.
 *
 * viskores::cont::tbb includes the code to implement the Viskores Control Environment
 * for the TBB-based device adapter.
 *
 * \namespace viskores::exec
 * \brief Viskores Execution Environment.
 *
 * viskores::exec defines the publicly accessible API for the Viskores Execution
 * Environment. Worklets typically use classes/apis defined within this
 * namespace alone.
 *
 * \namespace viskores::exec::cuda
 * \brief CUDA implementation for Execution Environment.
 *
 * viskores::exec::cuda includes the code to implement the Viskores Execution Environment
 * for the CUDA-based device adapter.
 *
* \namespace viskores::exec::openmp
 * \brief CUDA implementation for Execution Environment.
 *
 * viskores::exec::openmp includes the code to implement the Viskores Execution Environment
 * for the OpenMP device adapter.
 *
 * \namespace viskores::exec::serial
 * \brief CUDA implementation for Execution Environment.
 *
 * viskores::exec::serial includes the code to implement the Viskores Execution Environment
 * for the serial device adapter.
 *
 * \namespace viskores::exec::tbb
 * \brief TBB implementation for Execution Environment.
 *
 * viskores::exec::tbb includes the code to implement the Viskores Execution Environment
 * for the TBB device adapter.
 *
 * \namespace viskores::filter
 * \brief Viskores Filters
 *
 * viskores::filter is the collection of predefined filters that take data as input
 * and write new data as output. Filters operate on viskores::cont::DataSet objects,
 * viskores::cont::Fields, and other runtime typeless objects.
 *
 * \namespace viskores::internal
 * \brief Viskores Internal Environment
 *
 * viskores::internal defines API which is internal and subject to frequent
 * change. This should not be used for projects using Viskores. Instead it servers
 * are a reference for the developers of Viskores.
 *
 * \namespace viskores::interop
 * \brief Viskores OpenGL Interoperability
 *
 * viskores::interop defines the publicly accessible API for interoperability between
 * viskores and OpenGL.
 *
 * \namespace viskores::io
 * \brief Viskores File input and output classes
 *
 * viskores::io defines API for basic reading of VTK files. Intended to be used for
 * examples and testing.
 *
 * \namespace viskores::rendering
 * \brief Viskores Rendering
 *
 * viskores::rendering defines API for
 *
 * \namespace viskores::source
 * \brief Viskores Input source such as Wavelet
 *
 * viskores::source is the collection of predefined sources that generate data.
 *
 * \namespace viskores::testing
 * \brief Internal testing classes
 *
 * \namespace viskores::worklet
 * \brief Viskores Worklets
 *
 * viskores::worklet defines API for the low level worklets that operate on an element of data,
 * and the dispatcher that execute them in the execution environment.
 *
 * Viskores provides numerous worklet implementations. These worklet implementations for the most
 * part provide the underlying implementations of the algorithms in viskores::filter.
 *
 */

namespace viskores
{
//*****************************************************************************
// Typedefs for basic types.
//*****************************************************************************

/// Base type to use for 32-bit floating-point numbers.
///
using Float32 = float;

/// Base type to use for 64-bit floating-point numbers.
///
using Float64 = double;

/// Base type to use for 8-bit signed integer numbers.
///
using Int8 = int8_t;

/// Base type to use for 8-bit unsigned integer numbers.
///
using UInt8 = uint8_t;

/// Base type to use for 16-bit signed integer numbers.
///
using Int16 = int16_t;

/// Base type to use for 16-bit unsigned integer numbers.
///
using UInt16 = uint16_t;

/// Base type to use for 32-bit signed integer numbers.
///
using Int32 = int32_t;

/// Base type to use for 32-bit unsigned integer numbers.
///
using UInt32 = uint32_t;

/// \brief Base type to use to index small lists.
///
/// This type represents a component ID (index of component in a vector). The number
/// of components, being a value fixed at compile time, is generally assumed
/// to be quite small. However, we are currently using a 32-bit width
/// integer because modern processors tend to access them more efficiently
/// than smaller widths.
using IdComponent = viskores::Int32;

/// The default word size used for atomic bitwise operations. Universally
/// supported on all devices.
using WordTypeDefault = viskores::UInt32;

//In this order so that we exactly match the logic that exists in VTK
#if (VISKORES_SIZE_LONG_LONG == 8) || defined(VISKORES_DOXYGEN_ONLY)
/// Base type to use for 64-bit signed integer numbers.
///
using Int64 = signed long long;
/// Base type to use for 64-bit signed integer numbers.
///
using UInt64 = unsigned long long;
#define VISKORES_UNUSED_INT_TYPE long
#elif VISKORES_SIZE_LONG == 8
/// Base type to use for 64-bit signed integer numbers.
///
using Int64 = signed long;
/// Base type to use for 64-bit unsigned integer numbers.
///
using UInt64 = unsigned long;
#define VISKORES_UNUSED_INT_TYPE long long
#else
#error Could not find a 64-bit integer.
#endif

/// \brief Base type to use to index arrays.
///
/// This type represents an ID (index into arrays). It should be used whenever
/// indexing data that could grow arbitrarily large.
///
#ifdef VISKORES_USE_64BIT_IDS
using Id = viskores::Int64;
#else
using Id = viskores::Int32;
#endif

/// The floating point type to use when no other precision is specified.
#ifdef VISKORES_USE_DOUBLE_PRECISION
using FloatDefault = viskores::Float64;
#else
using FloatDefault = viskores::Float32;
#endif


namespace internal
{

/// Placeholder class for when a type is not applicable.
struct NullType
{
};

} // namespace internal

// Disable conversion warnings for Add, Subtract, Multiply, Divide on GCC only.
// GCC creates false positive warnings for signed/unsigned char* operations.
// This occurs because the values are implicitly casted up to int's for the
// operation, and than  casted back down to char's when return.
// This causes a false positive warning, even when the values is within
// the value types range
#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc || clang
struct Add
{
  template <typename T, typename U>
  inline VISKORES_EXEC_CONT auto operator()(const T& a, const U& b) const -> decltype(a + b)
  {
    return a + b;
  }

  // If both arguments are short integers, explicitly cast the result back to the
  // type to avoid narrowing conversion warnings from operations that promote to
  // integers.
  template <typename T>
  inline VISKORES_EXEC_CONT
    typename std::enable_if<std::is_integral<T>::value && sizeof(T) < sizeof(int), T>::type
    operator()(T a, T b) const
  {
    return static_cast<T>(a + b);
  }
};

struct Subtract
{
  template <typename T, typename U>
  inline VISKORES_EXEC_CONT auto operator()(const T& a, const U& b) const -> decltype(a - b)
  {
    return a - b;
  }

  // If both arguments are short integers, explicitly cast the result back to the
  // type to avoid narrowing conversion warnings from operations that promote to
  // integers.
  template <typename T>
  inline VISKORES_EXEC_CONT
    typename std::enable_if<std::is_integral<T>::value && sizeof(T) < sizeof(int), T>::type
    operator()(T a, T b) const
  {
    return static_cast<T>(a - b);
  }
};

struct Multiply
{
  template <typename T, typename U>
  inline VISKORES_EXEC_CONT auto operator()(const T& a, const U& b) const -> decltype(a * b)
  {
    return a * b;
  }

  // If both arguments are short integers, explicitly cast the result back to the
  // type to avoid narrowing conversion warnings from operations that promote to
  // integers.
  template <typename T>
  inline VISKORES_EXEC_CONT
    typename std::enable_if<std::is_integral<T>::value && sizeof(T) < sizeof(int), T>::type
    operator()(T a, T b) const
  {
    return static_cast<T>(a * b);
  }
};

struct Divide
{
  template <typename T, typename U>
  inline VISKORES_EXEC_CONT auto operator()(const T& a, const U& b) const -> decltype(a / b)
  {
    return a / b;
  }

  // If both arguments are short integers, explicitly cast the result back to the
  // type to avoid narrowing conversion warnings from operations that promote to
  // integers.
  template <typename T>
  inline VISKORES_EXEC_CONT
    typename std::enable_if<std::is_integral<T>::value && sizeof(T) < sizeof(int), T>::type
    operator()(T a, T b) const
  {
    return static_cast<T>(a / b);
  }
};

struct Negate
{
  template <typename T>
  inline VISKORES_EXEC_CONT T operator()(const T& x) const
  {
    return T(-x);
  }
};

#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang

//-----------------------------------------------------------------------------

// Pre declaration
template <typename T, viskores::IdComponent Size>
class VISKORES_ALWAYS_EXPORT Vec;

template <typename T>
class VISKORES_ALWAYS_EXPORT VecC;

template <typename T>
class VISKORES_ALWAYS_EXPORT VecCConst;

namespace detail
{

/// Base implementation of all Vec and VecC classes.
///
// Disable conversion warnings for Add, Subtract, Multiply, Divide on GCC only.
// GCC creates false positive warnings for signed/unsigned char* operations.
// This occurs because the values are implicitly casted up to int's for the
// operation, and than  casted back down to char's when return.
// This causes a false positive warning, even when the values is within
// the value types range
//
// NVCC 7.5 and below does not recognize this pragma inside of class bodies,
// so put them before entering the class.
//
#if (defined(VISKORES_CUDA) && (__CUDACC_VER_MAJOR__ < 8))
#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif // gcc || clang
#endif // use cuda < 8
template <typename T, typename DerivedClass>
class VISKORES_ALWAYS_EXPORT VecBaseCommon
{
public:
  using ComponentType = T;

protected:
  constexpr VecBaseCommon() = default;

  VISKORES_EXEC_CONT
  constexpr const DerivedClass& Derived() const { return *static_cast<const DerivedClass*>(this); }

  VISKORES_EXEC_CONT
  constexpr DerivedClass& Derived() { return *static_cast<DerivedClass*>(this); }

private:
  // Only for internal use
  VISKORES_EXEC_CONT
  constexpr viskores::IdComponent NumComponents() const
  {
    return this->Derived().GetNumberOfComponents();
  }

  // Only for internal use
  VISKORES_EXEC_CONT
  constexpr const T& Component(viskores::IdComponent index) const { return this->Derived()[index]; }

  // Only for internal use
  VISKORES_EXEC_CONT
  constexpr T& Component(viskores::IdComponent index) { return this->Derived()[index]; }

public:
  template <viskores::IdComponent OtherSize>
  VISKORES_EXEC_CONT void CopyInto(viskores::Vec<ComponentType, OtherSize>& dest) const
  {
    for (viskores::IdComponent index = 0; (index < this->NumComponents()) && (index < OtherSize);
         index++)
    {
      dest[index] = this->Component(index);
    }
  }

  // Only works with Vec-like objects with operator[] and GetNumberOfComponents().
  template <typename OtherVecType>
  VISKORES_EXEC_CONT DerivedClass& operator=(const OtherVecType& src)
  {
    VISKORES_ASSERT(this->NumComponents() == src.GetNumberOfComponents());
    for (viskores::IdComponent i = 0; i < this->NumComponents(); ++i)
    {
      this->Component(i) = src[i];
    }
    return this->Derived();
  }

  VISKORES_EXEC_CONT
  bool operator==(const DerivedClass& other) const
  {
    bool equal = true;
    for (viskores::IdComponent i = 0; i < this->NumComponents() && equal; ++i)
    {
      equal = (this->Component(i) == other[i]);
    }
    return equal;
  }

  VISKORES_EXEC_CONT
  bool operator<(const DerivedClass& other) const
  {
    for (viskores::IdComponent i = 0; i < this->NumComponents(); ++i)
    {
      // ignore equals as that represents check next value
      if (this->Component(i) < other[i])
      {
        return true;
      }
      else if (other[i] < this->Component(i))
      {
        return false;
      }
    } // if all same we are not less

    return false;
  }

  VISKORES_EXEC_CONT
  bool operator!=(const DerivedClass& other) const { return !(this->operator==(other)); }

#if (!(defined(VISKORES_CUDA) && (__CUDACC_VER_MAJOR__ < 8)))
#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif // gcc || clang
#endif // not using cuda < 8

  template <viskores::IdComponent Size>
  inline VISKORES_EXEC_CONT viskores::Vec<ComponentType, Size> operator+(
    const viskores::Vec<ComponentType, Size>& other) const
  {
    VISKORES_ASSERT(Size == this->NumComponents());
    viskores::Vec<ComponentType, Size> result;
    for (viskores::IdComponent i = 0; i < Size; ++i)
    {
      result[i] = this->Component(i) + other[i];
    }
    return result;
  }

  template <typename OtherClass>
  inline VISKORES_EXEC_CONT DerivedClass& operator+=(const OtherClass& other)
  {
    VISKORES_ASSERT(this->NumComponents() == other.GetNumberOfComponents());
    for (viskores::IdComponent i = 0; i < this->NumComponents(); ++i)
    {
      this->Component(i) += other[i];
    }
    return this->Derived();
  }

  template <viskores::IdComponent Size>
  inline VISKORES_EXEC_CONT viskores::Vec<ComponentType, Size> operator-(
    const viskores::Vec<ComponentType, Size>& other) const
  {
    VISKORES_ASSERT(Size == this->NumComponents());
    viskores::Vec<ComponentType, Size> result;
    for (viskores::IdComponent i = 0; i < Size; ++i)
    {
      result[i] = this->Component(i) - other[i];
    }
    return result;
  }

  template <typename OtherClass>
  inline VISKORES_EXEC_CONT DerivedClass& operator-=(const OtherClass& other)
  {
    VISKORES_ASSERT(this->NumComponents() == other.GetNumberOfComponents());
    for (viskores::IdComponent i = 0; i < this->NumComponents(); ++i)
    {
      this->Component(i) -= other[i];
    }
    return this->Derived();
  }

  template <viskores::IdComponent Size>
  inline VISKORES_EXEC_CONT viskores::Vec<ComponentType, Size> operator*(
    const viskores::Vec<ComponentType, Size>& other) const
  {
    viskores::Vec<ComponentType, Size> result;
    for (viskores::IdComponent i = 0; i < Size; ++i)
    {
      result[i] = this->Component(i) * other[i];
    }
    return result;
  }

  template <typename OtherClass>
  inline VISKORES_EXEC_CONT DerivedClass& operator*=(const OtherClass& other)
  {
    VISKORES_ASSERT(this->NumComponents() == other.GetNumberOfComponents());
    for (viskores::IdComponent i = 0; i < this->NumComponents(); ++i)
    {
      this->Component(i) *= other[i];
    }
    return this->Derived();
  }

  template <viskores::IdComponent Size>
  inline VISKORES_EXEC_CONT viskores::Vec<ComponentType, Size> operator/(
    const viskores::Vec<ComponentType, Size>& other) const
  {
    viskores::Vec<ComponentType, Size> result;
    for (viskores::IdComponent i = 0; i < Size; ++i)
    {
      result[i] = this->Component(i) / other[i];
    }
    return result;
  }

  template <typename OtherClass>
  VISKORES_EXEC_CONT DerivedClass& operator/=(const OtherClass& other)
  {
    VISKORES_ASSERT(this->NumComponents() == other.GetNumberOfComponents());
    for (viskores::IdComponent i = 0; i < this->NumComponents(); ++i)
    {
      this->Component(i) /= other[i];
    }
    return this->Derived();
  }

#if (!(defined(VISKORES_CUDA) && (__CUDACC_VER_MAJOR__ < 8)))
#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang
#endif // not using cuda < 8

  VISKORES_EXEC_CONT
  constexpr ComponentType* GetPointer() { return &this->Component(0); }

  VISKORES_EXEC_CONT
  constexpr const ComponentType* GetPointer() const { return &this->Component(0); }
};


/// Base implementation of all Vec classes.
///
template <typename T, viskores::IdComponent Size, typename DerivedClass>
class VISKORES_ALWAYS_EXPORT VecBase : public viskores::detail::VecBaseCommon<T, DerivedClass>
{
public:
  using ComponentType = T;
  static constexpr viskores::IdComponent NUM_COMPONENTS = Size;

  VecBase() = default;

  // The enable_if predicate will disable this constructor for Size=1 so that
  // the variadic constructor constexpr VecBase(T, Ts&&...) is called instead.
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <viskores::IdComponent Size2 = Size, typename std::enable_if<Size2 != 1, int>::type = 0>
  VISKORES_EXEC_CONT explicit VecBase(const ComponentType& value)
  {
    for (viskores::IdComponent i = 0; i < Size; ++i)
    {
      this->Components[i] = value;
    }
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename... Ts>
  VISKORES_EXEC_CONT constexpr VecBase(ComponentType value0, Ts&&... values)
    : Components{ value0, values... }
  {
    VISKORES_STATIC_ASSERT(sizeof...(Ts) + 1 == Size);
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  VecBase(std::initializer_list<ComponentType> values)
  {
    ComponentType* dest = this->Components;
    auto src = values.begin();
    if (values.size() == 1)
    {
      for (viskores::IdComponent i = 0; i < Size; ++i)
      {
        this->Components[i] = *src;
        ++dest;
      }
    }
    else
    {
      VISKORES_ASSERT((values.size() == NUM_COMPONENTS) &&
                      "Vec object initialized wrong number of components.");
      for (; src != values.end(); ++src)
      {
        *dest = *src;
        ++dest;
      }
    }
  }

#if (!(defined(VISKORES_CUDA) && (__CUDACC_VER_MAJOR__ < 8)))
#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif // gcc || clang
#endif //not using cuda < 8
#if defined(VISKORES_MSVC)
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename OtherValueType, typename OtherDerivedType>
  VISKORES_EXEC_CONT explicit VecBase(const VecBase<OtherValueType, Size, OtherDerivedType>& src)
  {
    //DO NOT CHANGE THIS AND THE ABOVE PRAGMA'S UNLESS YOU FULLY UNDERSTAND THE
    //ISSUE https://gitlab.kitware.com/vtk/viskores/-/issues/221
    for (viskores::IdComponent i = 0; i < Size; ++i)
    {
      this->Components[i] = src[i];
    }
  }

public:
  inline VISKORES_EXEC_CONT constexpr viskores::IdComponent GetNumberOfComponents() const
  {
    return NUM_COMPONENTS;
  }

  inline VISKORES_EXEC_CONT constexpr const ComponentType& operator[](
    viskores::IdComponent idx) const
  {
    return this->Components[idx];
  }

  inline VISKORES_EXEC_CONT constexpr ComponentType& operator[](viskores::IdComponent idx)
  {
    VISKORES_ASSERT(idx >= 0);
    VISKORES_ASSERT(idx < NUM_COMPONENTS);
    return this->Components[idx];
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename OtherComponentType, typename OtherClass>
  inline VISKORES_EXEC_CONT DerivedClass
  operator+(const VecBaseCommon<OtherComponentType, OtherClass>& other) const
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    VISKORES_ASSERT(NUM_COMPONENTS == other_derived.GetNumberOfComponents());

    DerivedClass result;
    for (viskores::IdComponent i = 0; i < NUM_COMPONENTS; ++i)
    {
      result[i] = this->Components[i] + static_cast<ComponentType>(other_derived[i]);
    }
    return result;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename OtherComponentType, typename OtherClass>
  inline VISKORES_EXEC_CONT DerivedClass
  operator-(const VecBaseCommon<OtherComponentType, OtherClass>& other) const
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    VISKORES_ASSERT(NUM_COMPONENTS == other_derived.GetNumberOfComponents());

    DerivedClass result;
    for (viskores::IdComponent i = 0; i < NUM_COMPONENTS; ++i)
    {
      result[i] = this->Components[i] - static_cast<ComponentType>(other_derived[i]);
    }
    return result;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename OtherComponentType, typename OtherClass>
  inline VISKORES_EXEC_CONT DerivedClass
  operator*(const VecBaseCommon<OtherComponentType, OtherClass>& other) const
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    VISKORES_ASSERT(NUM_COMPONENTS == other_derived.GetNumberOfComponents());

    DerivedClass result;
    for (viskores::IdComponent i = 0; i < NUM_COMPONENTS; ++i)
    {
      result[i] = this->Components[i] * static_cast<ComponentType>(other_derived[i]);
    }
    return result;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename OtherComponentType, typename OtherClass>
  inline VISKORES_EXEC_CONT DerivedClass
  operator/(const VecBaseCommon<OtherComponentType, OtherClass>& other) const
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    VISKORES_ASSERT(NUM_COMPONENTS == other_derived.GetNumberOfComponents());

    DerivedClass result;
    for (viskores::IdComponent i = 0; i < NUM_COMPONENTS; ++i)
    {
      result[i] = this->Components[i] / static_cast<ComponentType>(other_derived[i]);
    }
    return result;
  }

#if (!(defined(VISKORES_CUDA) && (__CUDACC_VER_MAJOR__ < 8)))
#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang
#endif // not using cuda < 8
#if defined(VISKORES_MSVC)
#pragma warning(pop)
#endif

protected:
  ComponentType Components[NUM_COMPONENTS];
};

#if (defined(VISKORES_CUDA) && (__CUDACC_VER_MAJOR__ < 8))
#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang
#endif // use cuda < 8

/// Base of all VecC and VecCConst classes.
///
template <typename T, typename DerivedClass>
class VISKORES_ALWAYS_EXPORT VecCBase : public viskores::detail::VecBaseCommon<T, DerivedClass>
{
protected:
  VISKORES_EXEC_CONT
  constexpr VecCBase() {}
};

} // namespace detail

//-----------------------------------------------------------------------------

/// \brief A short fixed-length array.
///
/// The \c Vec templated class holds a short array of values of a size and
/// type specified by the template arguments.
///
/// The \c Vec class is most often used to represent vectors in the
/// mathematical sense as a quantity with a magnitude and direction. Vectors
/// are, of course, used extensively in computational geometry as well as
/// physical simulations. The \c Vec class can be (and is) repurposed for more
/// general usage of holding a fixed-length sequence of objects.
///
/// There is no real limit to the size of the sequence (other than the largest
/// number representable by viskores::IdComponent), but the \c Vec class is really
/// designed for small sequences (seldom more than 10).
///
template <typename T, viskores::IdComponent Size>
class VISKORES_ALWAYS_EXPORT Vec : public detail::VecBase<T, Size, Vec<T, Size>>
{
  using Superclass = detail::VecBase<T, Size, Vec<T, Size>>;

public:
#ifdef VISKORES_DOXYGEN_ONLY
  using ComponentType = T;
  static constexpr viskores::IdComponent NUM_COMPONENTS = Size;
#endif

  using Superclass::Superclass;
  constexpr Vec() = default;
#if defined(_MSC_VER) && _MSC_VER < 1910
  template <typename... Ts>
  constexpr Vec(T value, Ts&&... values)
    : Superclass(value, std::forward<Ts>(values)...)
  {
  }
#endif

  inline VISKORES_EXEC_CONT void CopyInto(Vec<T, Size>& dest) const { dest = *this; }
};

//-----------------------------------------------------------------------------
// Specializations for common small tuples. We implement them a bit specially.

// A vector of size 0 cannot use VecBase because it will try to create a
// zero length array which troubles compilers. Vecs of size 0 are a bit
// pointless but might occur in some generic functions or classes.
template <typename T>
class VISKORES_ALWAYS_EXPORT Vec<T, 0>
{
public:
  using ComponentType = T;
  static constexpr viskores::IdComponent NUM_COMPONENTS = 0;

  constexpr Vec() = default;
  VISKORES_EXEC_CONT explicit Vec(const ComponentType&) {}

  template <typename OtherType>
  VISKORES_EXEC_CONT Vec(const Vec<OtherType, NUM_COMPONENTS>&)
  {
  }

  VISKORES_EXEC_CONT
  Vec<ComponentType, NUM_COMPONENTS>& operator=(const Vec<ComponentType, NUM_COMPONENTS>&)
  {
    return *this;
  }

  inline VISKORES_EXEC_CONT constexpr viskores::IdComponent GetNumberOfComponents() const
  {
    return NUM_COMPONENTS;
  }

  VISKORES_EXEC_CONT
  constexpr ComponentType operator[](viskores::IdComponent viskoresNotUsed(idx)) const
  {
    return ComponentType();
  }

  VISKORES_EXEC_CONT
  bool operator==(const Vec<T, NUM_COMPONENTS>& viskoresNotUsed(other)) const { return true; }
  VISKORES_EXEC_CONT
  bool operator!=(const Vec<T, NUM_COMPONENTS>& viskoresNotUsed(other)) const { return false; }
};

// Vectors of size 1 should implicitly convert between the scalar and the
// vector. Otherwise, it should behave the same.
template <typename T>
class VISKORES_ALWAYS_EXPORT Vec<T, 1> : public detail::VecBase<T, 1, Vec<T, 1>>
{
  using Superclass = detail::VecBase<T, 1, Vec<T, 1>>;

public:
  Vec() = default;
  VISKORES_EXEC_CONT constexpr Vec(const T& value)
    : Superclass(value)
  {
  }

  template <typename OtherType>
  VISKORES_EXEC_CONT Vec(const Vec<OtherType, 1>& src)
    : Superclass(src)
  {
  }
};

//-----------------------------------------------------------------------------
// Specializations for common tuple sizes (with special names).

template <typename T>
class VISKORES_ALWAYS_EXPORT Vec<T, 2> : public detail::VecBase<T, 2, Vec<T, 2>>
{
  using Superclass = detail::VecBase<T, 2, Vec<T, 2>>;

public:
  constexpr Vec() = default;
  VISKORES_EXEC_CONT Vec(const T& value)
    : Superclass(value)
  {
  }

  template <typename OtherType>
  VISKORES_EXEC_CONT Vec(const Vec<OtherType, 2>& src)
    : Superclass(src)
  {
  }

  VISKORES_EXEC_CONT
  constexpr Vec(const T& x, const T& y)
    : Superclass(x, y)
  {
  }
};

/// \brief Id2 corresponds to a 2-dimensional index.
///
using Id2 = viskores::Vec<viskores::Id, 2>;

/// \brief IdComponent2 corresponds to an index to a local (small) 2-d array or equivalent.
///
using IdComponent2 = viskores::Vec<viskores::IdComponent, 2>;

/// \brief Vec2f corresponds to a 2-dimensional vector of floating point values.
///
/// Each floating point value is of the default precision (i.e. viskores::FloatDefault). It is
/// typedef for viskores::Vec<viskores::FloatDefault, 2>.
///
using Vec2f = viskores::Vec<viskores::FloatDefault, 2>;

/// \brief Vec2f_32 corresponds to a 2-dimensional vector of 32-bit floating point values.
///
/// It is typedef for viskores::Vec<viskores::Float32, 2>.
///
using Vec2f_32 = viskores::Vec<viskores::Float32, 2>;

/// \brief Vec2f_64 corresponds to a 2-dimensional vector of 64-bit floating point values.
///
/// It is typedef for viskores::Vec<viskores::Float64, 2>.
///
using Vec2f_64 = viskores::Vec<viskores::Float64, 2>;

/// \brief Vec2i corresponds to a 2-dimensional vector of integer values.
///
/// Each integer value is of the default precision (i.e. viskores::Id).
///
using Vec2i = viskores::Vec<viskores::Id, 2>;

/// \brief Vec2i_8 corresponds to a 2-dimensional vector of 8-bit integer values.
///
/// It is typedef for viskores::Vec<viskores::Int32, 2>.
///
using Vec2i_8 = viskores::Vec<viskores::Int8, 2>;

/// \brief Vec2i_16 corresponds to a 2-dimensional vector of 16-bit integer values.
///
/// It is typedef for viskores::Vec<viskores::Int32, 2>.
///
using Vec2i_16 = viskores::Vec<viskores::Int16, 2>;

/// \brief Vec2i_32 corresponds to a 2-dimensional vector of 32-bit integer values.
///
/// It is typedef for viskores::Vec<viskores::Int32, 2>.
///
using Vec2i_32 = viskores::Vec<viskores::Int32, 2>;

/// \brief Vec2i_64 corresponds to a 2-dimensional vector of 64-bit integer values.
///
/// It is typedef for viskores::Vec<viskores::Int64, 2>.
///
using Vec2i_64 = viskores::Vec<viskores::Int64, 2>;

/// \brief Vec2ui corresponds to a 2-dimensional vector of unsigned integer values.
///
/// Each integer value is of the default precision (following viskores::Id).
///
#ifdef VISKORES_USE_64BIT_IDS
using Vec2ui = viskores::Vec<viskores::UInt64, 2>;
#else
using Vec2ui = viskores::Vec<viskores::UInt32, 2>;
#endif

/// \brief Vec2ui_8 corresponds to a 2-dimensional vector of 8-bit unsigned integer values.
///
/// It is typedef for viskores::Vec<viskores::UInt32, 2>.
///
using Vec2ui_8 = viskores::Vec<viskores::UInt8, 2>;

/// \brief Vec2ui_16 corresponds to a 2-dimensional vector of 16-bit unsigned integer values.
///
/// It is typedef for viskores::Vec<viskores::UInt32, 2>.
///
using Vec2ui_16 = viskores::Vec<viskores::UInt16, 2>;

/// \brief Vec2ui_32 corresponds to a 2-dimensional vector of 32-bit unsigned integer values.
///
/// It is typedef for viskores::Vec<viskores::UInt32, 2>.
///
using Vec2ui_32 = viskores::Vec<viskores::UInt32, 2>;

/// \brief Vec2ui_64 corresponds to a 2-dimensional vector of 64-bit unsigned integer values.
///
/// It is typedef for viskores::Vec<viskores::UInt64, 2>.
///
using Vec2ui_64 = viskores::Vec<viskores::UInt64, 2>;

template <typename T>
class VISKORES_ALWAYS_EXPORT Vec<T, 3> : public detail::VecBase<T, 3, Vec<T, 3>>
{
  using Superclass = detail::VecBase<T, 3, Vec<T, 3>>;

public:
  constexpr Vec() = default;
  VISKORES_EXEC_CONT Vec(const T& value)
    : Superclass(value)
  {
  }

  template <typename OtherType>
  VISKORES_EXEC_CONT Vec(const Vec<OtherType, 3>& src)
    : Superclass(src)
  {
  }

  VISKORES_EXEC_CONT
  constexpr Vec(const T& x, const T& y, const T& z)
    : Superclass(x, y, z)
  {
  }
};

/// \brief Id3 corresponds to a 3-dimensional index for 3d arrays.
///
/// Note that the precision of each index may be less than viskores::Id.
///
using Id3 = viskores::Vec<viskores::Id, 3>;

/// \brief IdComponent2 corresponds to an index to a local (small) 3-d array or equivalent.
///
using IdComponent3 = viskores::Vec<viskores::IdComponent, 3>;

/// \brief Vec3f corresponds to a 3-dimensional vector of floating point values.
///
/// Each floating point value is of the default precision (i.e. viskores::FloatDefault). It is
/// typedef for viskores::Vec<viskores::FloatDefault, 3>.
///
using Vec3f = viskores::Vec<viskores::FloatDefault, 3>;

/// \brief Vec3f_32 corresponds to a 3-dimensional vector of 32-bit floating point values.
///
/// It is typedef for viskores::Vec<viskores::Float32, 3>.
///
using Vec3f_32 = viskores::Vec<viskores::Float32, 3>;

/// \brief Vec3f_64 corresponds to a 3-dimensional vector of 64-bit floating point values.
///
/// It is typedef for viskores::Vec<viskores::Float64, 3>.
///
using Vec3f_64 = viskores::Vec<viskores::Float64, 3>;

/// \brief Vec3i corresponds to a 3-dimensional vector of integer values.
///
/// Each integer value is of the default precision (i.e. viskores::Id).
///
using Vec3i = viskores::Vec<viskores::Id, 3>;

/// \brief Vec3i_8 corresponds to a 3-dimensional vector of 8-bit integer values.
///
/// It is typedef for viskores::Vec<viskores::Int32, 3>.
///
using Vec3i_8 = viskores::Vec<viskores::Int8, 3>;

/// \brief Vec3i_16 corresponds to a 3-dimensional vector of 16-bit integer values.
///
/// It is typedef for viskores::Vec<viskores::Int32, 3>.
///
using Vec3i_16 = viskores::Vec<viskores::Int16, 3>;

/// \brief Vec3i_32 corresponds to a 3-dimensional vector of 32-bit integer values.
///
/// It is typedef for viskores::Vec<viskores::Int32, 3>.
///
using Vec3i_32 = viskores::Vec<viskores::Int32, 3>;

/// \brief Vec3i_64 corresponds to a 3-dimensional vector of 64-bit integer values.
///
/// It is typedef for viskores::Vec<viskores::Int64, 3>.
///
using Vec3i_64 = viskores::Vec<viskores::Int64, 3>;

/// \brief Vec3ui corresponds to a 3-dimensional vector of unsigned integer values.
///
/// Each integer value is of the default precision (following viskores::Id).
///
#ifdef VISKORES_USE_64BIT_IDS
using Vec3ui = viskores::Vec<viskores::UInt64, 3>;
#else
using Vec3ui = viskores::Vec<viskores::UInt32, 3>;
#endif

/// \brief Vec3ui_8 corresponds to a 3-dimensional vector of 8-bit unsigned integer values.
///
/// It is typedef for viskores::Vec<viskores::UInt32, 3>.
///
using Vec3ui_8 = viskores::Vec<viskores::UInt8, 3>;

/// \brief Vec3ui_16 corresponds to a 3-dimensional vector of 16-bit unsigned integer values.
///
/// It is typedef for viskores::Vec<viskores::UInt32, 3>.
///
using Vec3ui_16 = viskores::Vec<viskores::UInt16, 3>;

/// \brief Vec3ui_32 corresponds to a 3-dimensional vector of 32-bit unsigned integer values.
///
/// It is typedef for viskores::Vec<viskores::UInt32, 3>.
///
using Vec3ui_32 = viskores::Vec<viskores::UInt32, 3>;

/// \brief Vec3ui_64 corresponds to a 3-dimensional vector of 64-bit unsigned integer values.
///
/// It is typedef for viskores::Vec<viskores::UInt64, 3>.
///
using Vec3ui_64 = viskores::Vec<viskores::UInt64, 3>;

template <typename T>
class VISKORES_ALWAYS_EXPORT Vec<T, 4> : public detail::VecBase<T, 4, Vec<T, 4>>
{
  using Superclass = detail::VecBase<T, 4, Vec<T, 4>>;

public:
  constexpr Vec() = default;
  VISKORES_EXEC_CONT Vec(const T& value)
    : Superclass(value)
  {
  }

  template <typename OtherType>
  VISKORES_EXEC_CONT Vec(const Vec<OtherType, 4>& src)
    : Superclass(src)
  {
  }

  VISKORES_EXEC_CONT
  constexpr Vec(const T& x, const T& y, const T& z, const T& w)
    : Superclass(x, y, z, w)
  {
  }
};

/// \brief Id4 corresponds to a 4-dimensional index.
///
using Id4 = viskores::Vec<viskores::Id, 4>;

/// \brief IdComponent4 corresponds to an index to a local (small) 4-d array or equivalent.
///
using IdComponent4 = viskores::Vec<viskores::IdComponent, 4>;

/// \brief Vec4f corresponds to a 4-dimensional vector of floating point values.
///
/// Each floating point value is of the default precision (i.e. viskores::FloatDefault). It is
/// typedef for viskores::Vec<viskores::FloatDefault, 4>.
///
using Vec4f = viskores::Vec<viskores::FloatDefault, 4>;

/// \brief Vec4f_32 corresponds to a 4-dimensional vector of 32-bit floating point values.
///
/// It is typedef for viskores::Vec<viskores::Float32, 4>.
///
using Vec4f_32 = viskores::Vec<viskores::Float32, 4>;

/// \brief Vec4f_64 corresponds to a 4-dimensional vector of 64-bit floating point values.
///
/// It is typedef for viskores::Vec<viskores::Float64, 4>.
///
using Vec4f_64 = viskores::Vec<viskores::Float64, 4>;

/// \brief Vec4i corresponds to a 4-dimensional vector of integer values.
///
/// Each integer value is of the default precision (i.e. viskores::Id).
///
using Vec4i = viskores::Vec<viskores::Id, 4>;

/// \brief Vec4i_8 corresponds to a 4-dimensional vector of 8-bit integer values.
///
/// It is typedef for viskores::Vec<viskores::Int32, 4>.
///
using Vec4i_8 = viskores::Vec<viskores::Int8, 4>;

/// \brief Vec4i_16 corresponds to a 4-dimensional vector of 16-bit integer values.
///
/// It is typedef for viskores::Vec<viskores::Int32, 4>.
///
using Vec4i_16 = viskores::Vec<viskores::Int16, 4>;

/// \brief Vec4i_32 corresponds to a 4-dimensional vector of 32-bit integer values.
///
/// It is typedef for viskores::Vec<viskores::Int32, 4>.
///
using Vec4i_32 = viskores::Vec<viskores::Int32, 4>;

/// \brief Vec4i_64 corresponds to a 4-dimensional vector of 64-bit integer values.
///
/// It is typedef for viskores::Vec<viskores::Int64, 4>.
///
using Vec4i_64 = viskores::Vec<viskores::Int64, 4>;

/// \brief Vec4ui corresponds to a 4-dimensional vector of unsigned integer values.
///
/// Each integer value is of the default precision (following viskores::Id).
///
#ifdef VISKORES_USE_64BIT_IDS
using Vec4ui = viskores::Vec<viskores::UInt64, 4>;
#else
using Vec4ui = viskores::Vec<viskores::UInt32, 4>;
#endif

/// \brief Vec4ui_8 corresponds to a 4-dimensional vector of 8-bit unsigned integer values.
///
/// It is typedef for viskores::Vec<viskores::UInt32, 4>.
///
using Vec4ui_8 = viskores::Vec<viskores::UInt8, 4>;

/// \brief Vec4ui_16 corresponds to a 4-dimensional vector of 16-bit unsigned integer values.
///
/// It is typedef for viskores::Vec<viskores::UInt32, 4>.
///
using Vec4ui_16 = viskores::Vec<viskores::UInt16, 4>;

/// \brief Vec4ui_32 corresponds to a 4-dimensional vector of 32-bit unsigned integer values.
///
/// It is typedef for viskores::Vec<viskores::UInt32, 4>.
///
using Vec4ui_32 = viskores::Vec<viskores::UInt32, 4>;

/// \brief Vec4ui_64 corresponds to a 4-dimensional vector of 64-bit unsigned integer values.
///
/// It is typedef for viskores::Vec<viskores::UInt64, 4>.
///
using Vec4ui_64 = viskores::Vec<viskores::UInt64, 4>;

/// Initializes and returns a Vec containing all the arguments. The arguments should all be the
/// same type or compile issues will occur.
///
template <typename T, typename... Ts>
VISKORES_EXEC_CONT constexpr viskores::Vec<T, viskores::IdComponent(sizeof...(Ts) + 1)> make_Vec(
  T value0,
  Ts&&... args)
{
  return viskores::Vec<T, viskores::IdComponent(sizeof...(Ts) + 1)>(value0, T(args)...);
}

/// \brief A Vec-like representation for short arrays.
///
/// The \c VecC class takes a short array of values and provides an interface
/// that mimics \c Vec. This provides a mechanism to treat C arrays like a \c
/// Vec. It is useful in situations where you want to use a \c Vec but the data
/// must come from elsewhere or in certain situations where the size cannot be
/// determined at compile time. In particular, \c Vec objects of different
/// sizes can potentially all be converted to a \c VecC of the same type.
///
/// Note that \c VecC holds a reference to an outside array given to it. If
/// that array gets destroyed (for example because the source goes out of
/// scope), the behavior becomes undefined.
///
/// You cannot use \c VecC with a const type in its template argument. For
/// example, you cannot declare <tt>VecC<const viskores::Id></tt>. If you want a
/// non-mutable \c VecC, the \c VecCConst class (e.g.
/// <tt>VecCConst<viskores::Id></tt>).
///
template <typename T>
class VISKORES_ALWAYS_EXPORT VecC : public detail::VecCBase<T, VecC<T>>
{
  using Superclass = detail::VecCBase<T, VecC<T>>;

  VISKORES_STATIC_ASSERT_MSG(std::is_const<T>::value == false,
                             "You cannot use VecC with a const type as its template argument. "
                             "Use either const VecC or VecCConst.");

public:
#ifdef VISKORES_DOXYGEN_ONLY
  using ComponentType = T;
#endif

  VISKORES_EXEC_CONT
  constexpr VecC()
    : Components(nullptr)
    , NumberOfComponents(0)
  {
  }

  VISKORES_EXEC_CONT
  constexpr VecC(T* array, viskores::IdComponent size)
    : Components(array)
    , NumberOfComponents(size)
  {
  }

  template <viskores::IdComponent Size>
  constexpr VISKORES_EXEC_CONT VecC(viskores::Vec<T, Size>& src)
    : Components(src.GetPointer())
    , NumberOfComponents(Size)
  {
  }

  VISKORES_EXEC_CONT
  explicit constexpr VecC(T& src)
    : Components(&src)
    , NumberOfComponents(1)
  {
  }

  VISKORES_EXEC_CONT
  constexpr VecC(const VecC<T>& src)
    : Components(src.Components)
    , NumberOfComponents(src.NumberOfComponents)
  {
  }

  inline VISKORES_EXEC_CONT constexpr const T& operator[](viskores::IdComponent index) const
  {
    VISKORES_ASSERT(index >= 0);
    VISKORES_ASSERT(index < this->NumberOfComponents);
    return this->Components[index];
  }

  inline VISKORES_EXEC_CONT constexpr T& operator[](viskores::IdComponent index)
  {
    VISKORES_ASSERT(index >= 0);
    VISKORES_ASSERT(index < this->NumberOfComponents);
    return this->Components[index];
  }

  inline VISKORES_EXEC_CONT constexpr viskores::IdComponent GetNumberOfComponents() const
  {
    return this->NumberOfComponents;
  }

  VISKORES_EXEC_CONT
  VecC<T>& operator=(const VecC<T>& src)
  {
    VISKORES_ASSERT(this->NumberOfComponents == src.GetNumberOfComponents());
    for (viskores::IdComponent index = 0; index < this->NumberOfComponents; index++)
    {
      (*this)[index] = src[index];
    }

    return *this;
  }

private:
  T* const Components;
  viskores::IdComponent NumberOfComponents;
};

/// \brief A const version of VecC
///
/// \c VecCConst is a non-mutable form of \c VecC. It can be used in place of
/// \c VecC when a constant array is available.
///
/// A \c VecC can be automatically converted to a \c VecCConst, but not vice
/// versa, so function arguments should use \c VecCConst when the data do not
/// need to be changed.
///
template <typename T>
class VISKORES_ALWAYS_EXPORT VecCConst : public detail::VecCBase<T, VecCConst<T>>
{
  using Superclass = detail::VecCBase<T, VecCConst<T>>;

  VISKORES_STATIC_ASSERT_MSG(std::is_const<T>::value == false,
                             "You cannot use VecCConst with a const type as its template argument. "
                             "Remove the const from the type.");

public:
#ifdef VISKORES_DOXYGEN_ONLY
  using ComponentType = T;
#endif

  VISKORES_EXEC_CONT
  constexpr VecCConst()
    : Components(nullptr)
    , NumberOfComponents(0)
  {
  }

  VISKORES_EXEC_CONT
  constexpr VecCConst(const T* array, viskores::IdComponent size)
    : Components(array)
    , NumberOfComponents(size)
  {
  }

  template <viskores::IdComponent Size>
  VISKORES_EXEC_CONT constexpr VecCConst(const viskores::Vec<T, Size>& src)
    : Components(src.GetPointer())
    , NumberOfComponents(Size)
  {
  }

  VISKORES_EXEC_CONT
  explicit constexpr VecCConst(const T& src)
    : Components(&src)
    , NumberOfComponents(1)
  {
  }

  VISKORES_EXEC_CONT
  constexpr VecCConst(const VecCConst<T>& src)
    : Components(src.Components)
    , NumberOfComponents(src.NumberOfComponents)
  {
  }

  VISKORES_EXEC_CONT
  constexpr VecCConst(const VecC<T>& src)
    : Components(src.Components)
    , NumberOfComponents(src.NumberOfComponents)
  {
  }

  inline VISKORES_EXEC_CONT constexpr const T& operator[](viskores::IdComponent index) const
  {
    VISKORES_ASSERT(index >= 0);
    VISKORES_ASSERT(index < this->NumberOfComponents);
    return this->Components[index];
  }

  inline VISKORES_EXEC_CONT constexpr viskores::IdComponent GetNumberOfComponents() const
  {
    return this->NumberOfComponents;
  }

private:
  const T* const Components;
  viskores::IdComponent NumberOfComponents;

  // You are not allowed to assign to a VecCConst, so these operators are not
  // implemented and are disallowed.
  void operator=(const VecCConst<T>&) = delete;
  void operator+=(const VecCConst<T>&) = delete;
  void operator-=(const VecCConst<T>&) = delete;
  void operator*=(const VecCConst<T>&) = delete;
  void operator/=(const VecCConst<T>&) = delete;
};

/// Creates a \c VecC from an input array.
///
template <typename T>
static inline VISKORES_EXEC_CONT constexpr viskores::VecC<T> make_VecC(T* array,
                                                                       viskores::IdComponent size)
{
  return viskores::VecC<T>(array, size);
}

/// Creates a \c VecCConst from a constant input array.
///
template <typename T>
static inline VISKORES_EXEC_CONT constexpr viskores::VecCConst<T> make_VecC(
  const T* array,
  viskores::IdComponent size)
{
  return viskores::VecCConst<T>(array, size);
}

namespace detail
{

template <typename T>
static inline VISKORES_EXEC_CONT auto vec_dot(const T& a, const T& b)
{
  auto result = a[0] * b[0];
  for (viskores::IdComponent i = 1; i < a.GetNumberOfComponents(); ++i)
  {
    result = result + a[i] * b[i];
  }
  return result;
}
template <typename T, viskores::IdComponent Size>
static inline VISKORES_EXEC_CONT auto vec_dot(const viskores::Vec<T, Size>& a,
                                              const viskores::Vec<T, Size>& b)
{
  auto result = a[0] * b[0];
  for (viskores::IdComponent i = 1; i < Size; ++i)
  {
    result = result + a[i] * b[i];
  }
  return result;
}

} // namespace detail

template <typename T>
static inline VISKORES_EXEC_CONT auto Dot(const T& a, const T& b)
{
  return detail::vec_dot(a, b);
}

template <typename T>
static inline VISKORES_EXEC_CONT auto Dot(const viskores::Vec<T, 2>& a,
                                          const viskores::Vec<T, 2>& b)
{
  return (a[0] * b[0]) + (a[1] * b[1]);
}
template <typename T>
static inline VISKORES_EXEC_CONT auto Dot(const viskores::Vec<T, 3>& a,
                                          const viskores::Vec<T, 3>& b)
{
  return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
}
template <typename T>
static inline VISKORES_EXEC_CONT auto Dot(const viskores::Vec<T, 4>& a,
                                          const viskores::Vec<T, 4>& b)
{
  return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]) + (a[3] * b[3]);
}
// Integer types of a width less than an integer get implicitly casted to
// an integer when doing a multiplication.
#define VISKORES_SCALAR_DOT(stype)                            \
  static inline VISKORES_EXEC_CONT auto dot(stype a, stype b) \
  {                                                           \
    return a * b;                                             \
  } /* LEGACY */                                              \
  static inline VISKORES_EXEC_CONT auto Dot(stype a, stype b) \
  {                                                           \
    return a * b;                                             \
  }
VISKORES_SCALAR_DOT(viskores::Int8)
VISKORES_SCALAR_DOT(viskores::UInt8)
VISKORES_SCALAR_DOT(viskores::Int16)
VISKORES_SCALAR_DOT(viskores::UInt16)
VISKORES_SCALAR_DOT(viskores::Int32)
VISKORES_SCALAR_DOT(viskores::UInt32)
VISKORES_SCALAR_DOT(viskores::Int64)
VISKORES_SCALAR_DOT(viskores::UInt64)
VISKORES_SCALAR_DOT(viskores::Float32)
VISKORES_SCALAR_DOT(viskores::Float64)

// v============ LEGACY =============v
template <typename T>
static inline VISKORES_EXEC_CONT auto dot(const T& a, const T& b) -> decltype(detail::vec_dot(a, b))
{
  return viskores::Dot(a, b);
}
template <typename T>
static inline VISKORES_EXEC_CONT auto dot(const viskores::Vec<T, 2>& a,
                                          const viskores::Vec<T, 2>& b)
{
  return viskores::Dot(a, b);
}
template <typename T>
static inline VISKORES_EXEC_CONT auto dot(const viskores::Vec<T, 3>& a,
                                          const viskores::Vec<T, 3>& b)
{
  return viskores::Dot(a, b);
}
template <typename T>
static inline VISKORES_EXEC_CONT auto dot(const viskores::Vec<T, 4>& a,
                                          const viskores::Vec<T, 4>& b)
{
  return viskores::Dot(a, b);
}
// ^============ LEGACY =============^

template <typename T, viskores::IdComponent Size>
inline VISKORES_EXEC_CONT T ReduceSum(const viskores::Vec<T, Size>& a)
{
  T result = a[0];
  for (viskores::IdComponent i = 1; i < Size; ++i)
  {
    result += a[i];
  }
  return result;
}

template <typename T>
inline VISKORES_EXEC_CONT T ReduceSum(const viskores::Vec<T, 2>& a)
{
  return a[0] + a[1];
}

template <typename T>
inline VISKORES_EXEC_CONT T ReduceSum(const viskores::Vec<T, 3>& a)
{
  return a[0] + a[1] + a[2];
}

template <typename T>
inline VISKORES_EXEC_CONT T ReduceSum(const viskores::Vec<T, 4>& a)
{
  return a[0] + a[1] + a[2] + a[3];
}

template <typename T, viskores::IdComponent Size>
inline VISKORES_EXEC_CONT T ReduceProduct(const viskores::Vec<T, Size>& a)
{
  T result = a[0];
  for (viskores::IdComponent i = 1; i < Size; ++i)
  {
    result *= a[i];
  }
  return result;
}

template <typename T>
inline VISKORES_EXEC_CONT T ReduceProduct(const viskores::Vec<T, 2>& a)
{
  return a[0] * a[1];
}

template <typename T>
inline VISKORES_EXEC_CONT T ReduceProduct(const viskores::Vec<T, 3>& a)
{
  return a[0] * a[1] * a[2];
}

template <typename T>
inline VISKORES_EXEC_CONT T ReduceProduct(const viskores::Vec<T, 4>& a)
{
  return a[0] * a[1] * a[2] * a[3];
}

// A pre-declaration of viskores::Pair so that classes templated on them can refer
// to it. The actual implementation is in viskores/Pair.h.
template <typename U, typename V>
struct Pair;

/// Helper function for printing out vectors during testing.
///
template <typename T, viskores::IdComponent Size>
inline VISKORES_CONT std::ostream& operator<<(std::ostream& stream,
                                              const viskores::Vec<T, Size>& vec)
{
  stream << "[";
  for (viskores::IdComponent component = 0; component < Size - 1; component++)
  {
    stream << vec[component] << ",";
  }
  return stream << vec[Size - 1] << "]";
}

/// Helper function for printing out pairs during testing.
///
template <typename T, typename U>
inline VISKORES_EXEC_CONT std::ostream& operator<<(std::ostream& stream,
                                                   const viskores::Pair<T, U>& vec)
{
  return stream << "[" << vec.first << "," << vec.second << "]";
}

} // End of namespace viskores

#include <viskores/internal/VecOperators.h>
// Declared inside of viskores namespace so that the operator work with ADL lookup
#endif //viskores_Types_h
