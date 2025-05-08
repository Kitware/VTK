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
#ifndef viskores_TypeTraits_h
#define viskores_TypeTraits_h

#include <viskores/Types.h>

namespace viskores
{

/// Tag used to identify types that aren't Real, Integer, Scalar or Vector.
///
struct TypeTraitsUnknownTag
{
};

/// Tag used to identify types that store real (floating-point) numbers. A
/// TypeTraits class will typedef this class to NumericTag if it stores real
/// numbers (or vectors of real numbers).
///
struct TypeTraitsRealTag
{
};

/// Tag used to identify types that store integer numbers. A TypeTraits class
/// will typedef this class to NumericTag if it stores integer numbers (or
/// vectors of integers).
///
struct TypeTraitsIntegerTag
{
};

/// Tag used to identify 0 dimensional types (scalars). Scalars can also be
/// treated like vectors when used with VecTraits. A TypeTraits class will
/// typedef this class to DimensionalityTag.
///
struct TypeTraitsScalarTag
{
};

/// Tag used to identify 1 dimensional types (vectors). A TypeTraits class will
/// typedef this class to DimensionalityTag.
///
struct TypeTraitsVectorTag
{
};

/// The TypeTraits class provides helpful compile-time information about the
/// basic types used in Viskores (and a few others for convenience). The majority
/// of TypeTraits contents are typedefs to tags that can be used to easily
/// override behavior of called functions.
///
template <typename T>
class TypeTraits
{
public:
  /// \brief A tag to determine whether the type is integer or real.
  ///
  /// This tag is either TypeTraitsRealTag or TypeTraitsIntegerTag.
  using NumericTag = viskores::TypeTraitsUnknownTag;

  /// \brief A tag to determine whether the type has multiple components.
  ///
  /// This tag is either TypeTraitsScalarTag or TypeTraitsVectorTag. Scalars can
  /// also be treated as vectors with VecTraits.
  using DimensionalityTag = viskores::TypeTraitsUnknownTag;

  /// @brief A static function that returns 0 (or the closest equivalent to it)
  /// for the given type.
  VISKORES_EXEC_CONT static T ZeroInitialization() { return T(); }
};

// Const types should have the same traits as their non-const counterparts.
//
template <typename T>
struct TypeTraits<const T> : TypeTraits<T>
{
};

#define VISKORES_BASIC_REAL_TYPE(T)                  \
  template <>                                        \
  struct TypeTraits<T>                               \
  {                                                  \
    using NumericTag = TypeTraitsRealTag;            \
    using DimensionalityTag = TypeTraitsScalarTag;   \
    VISKORES_EXEC_CONT static T ZeroInitialization() \
    {                                                \
      return T();                                    \
    }                                                \
  };

#define VISKORES_BASIC_INTEGER_TYPE(T)               \
  template <>                                        \
  struct TypeTraits<T>                               \
  {                                                  \
    using NumericTag = TypeTraitsIntegerTag;         \
    using DimensionalityTag = TypeTraitsScalarTag;   \
    VISKORES_EXEC_CONT static T ZeroInitialization() \
    {                                                \
      using ReturnType = T;                          \
      return ReturnType();                           \
    }                                                \
  };

/// Traits for basic C++ types.
///

VISKORES_BASIC_REAL_TYPE(float)
VISKORES_BASIC_REAL_TYPE(double)

VISKORES_BASIC_INTEGER_TYPE(bool)
VISKORES_BASIC_INTEGER_TYPE(char)
VISKORES_BASIC_INTEGER_TYPE(signed char)
VISKORES_BASIC_INTEGER_TYPE(unsigned char)
VISKORES_BASIC_INTEGER_TYPE(short)
VISKORES_BASIC_INTEGER_TYPE(unsigned short)
VISKORES_BASIC_INTEGER_TYPE(int)
VISKORES_BASIC_INTEGER_TYPE(unsigned int)
VISKORES_BASIC_INTEGER_TYPE(long)
VISKORES_BASIC_INTEGER_TYPE(unsigned long)
VISKORES_BASIC_INTEGER_TYPE(long long)
VISKORES_BASIC_INTEGER_TYPE(unsigned long long)

#undef VISKORES_BASIC_REAL_TYPE
#undef VISKORES_BASIC_INTEGER_TYPE

/// Traits for Vec types.
///
template <typename T, viskores::IdComponent Size>
struct TypeTraits<viskores::Vec<T, Size>>
{
  using NumericTag = typename viskores::TypeTraits<T>::NumericTag;
  using DimensionalityTag = viskores::TypeTraitsVectorTag;

  VISKORES_EXEC_CONT
  static viskores::Vec<T, Size> ZeroInitialization()
  {
    return viskores::Vec<T, Size>(viskores::TypeTraits<T>::ZeroInitialization());
  }
};

/// Traits for VecCConst types.
///
template <typename T>
struct TypeTraits<viskores::VecCConst<T>>
{
  using NumericTag = typename viskores::TypeTraits<T>::NumericTag;
  using DimensionalityTag = TypeTraitsVectorTag;

  VISKORES_EXEC_CONT
  static viskores::VecCConst<T> ZeroInitialization() { return viskores::VecCConst<T>(); }
};

/// Traits for VecC types.
///
template <typename T>
struct TypeTraits<viskores::VecC<T>>
{
  using NumericTag = typename viskores::TypeTraits<T>::NumericTag;
  using DimensionalityTag = TypeTraitsVectorTag;

  VISKORES_EXEC_CONT
  static viskores::VecC<T> ZeroInitialization() { return viskores::VecC<T>(); }
};

/// \brief Traits for Pair types.
///
template <typename T, typename U>
struct TypeTraits<viskores::Pair<T, U>>
{
  using NumericTag = viskores::TypeTraitsUnknownTag;
  using DimensionalityTag = viskores::TypeTraitsScalarTag;

  VISKORES_EXEC_CONT
  static viskores::Pair<T, U> ZeroInitialization()
  {
    return viskores::Pair<T, U>(TypeTraits<T>::ZeroInitialization(),
                                TypeTraits<U>::ZeroInitialization());
  }
};

} // namespace viskores

#endif //viskores_TypeTraits_h
