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
#ifndef viskores_count_ArrayHandleRandomUniformReal_h
#define viskores_count_ArrayHandleRandomUniformReal_h

#include <viskores/cont/ArrayHandleRandomUniformBits.h>
#include <viskores/cont/ArrayHandleTransform.h>

namespace viskores
{
namespace cont
{

namespace detail
{
template <typename Real>
struct CanonicalFunctor;

template <>
struct CanonicalFunctor<viskores::Float64>
{
  /// \brief Viskores's equivalent of std::generate_canonical, turning a random bit source into
  /// random real number in the range of [0, 1).
  // We take 53 bits (number of bits in mantissa in a double) from the 64 bits random source
  // and divide it by (1 << 53).
  static constexpr viskores::Float64 DIVISOR =
    static_cast<viskores::Float64>(viskores::UInt64{ 1 } << 53);
  static constexpr viskores::UInt64 MASK = (viskores::UInt64{ 1 } << 53) - viskores::UInt64{ 1 };

  VISKORES_EXEC_CONT
  viskores::Float64 operator()(viskores::UInt64 bits) const { return (bits & MASK) / DIVISOR; }
};

template <>
struct CanonicalFunctor<viskores::Float32>
{
  // We take 24 bits (number of bits in mantissa in a double) from the 64 bits random source
  // and divide it by (1 << 24).
  static constexpr viskores::Float32 DIVISOR =
    static_cast<viskores::Float32>(viskores::UInt32{ 1 } << 24);
  static constexpr viskores::UInt32 MASK = (viskores::UInt32{ 1 } << 24) - viskores::UInt32{ 1 };

  VISKORES_EXEC_CONT
  viskores::Float32 operator()(viskores::UInt64 bits) const { return (bits & MASK) / DIVISOR; }
};
} // detail

/// @brief An `ArrayHandle` that provides a source of random numbers with uniform distribution.
///
/// `ArrayHandleRandomUniformReal` takes a user supplied seed and hashes it to provide
/// a sequence of numbers drawn from a random uniform distribution in the range [0, 1).
/// `ArrayHandleRandomUniformReal` is built on top of `ArrayHandleRandomUniformBits` so
/// shares its behavior with that array.
///
/// Note: In contrast to traditional random number generator,
/// `ArrayHandleRandomUniformReal` does not have "state", i.e. multiple calls
/// the Get() method with the same index will always return the same hash value.
/// To get a new set of random bits, create a new `ArrayHandleRandomUniformBits`
/// with a different seed.
template <typename Real = viskores::Float64>
class VISKORES_ALWAYS_EXPORT ArrayHandleRandomUniformReal
  : public viskores::cont::ArrayHandleTransform<viskores::cont::ArrayHandleRandomUniformBits,
                                                detail::CanonicalFunctor<Real>>
{
public:
  using SeedType = viskores::Vec<viskores::UInt32, 1>;

  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleRandomUniformReal,
    (ArrayHandleRandomUniformReal<Real>),
    (viskores::cont::ArrayHandleTransform<viskores::cont::ArrayHandleRandomUniformBits,
                                          detail::CanonicalFunctor<Real>>));

  /// Construct an `ArrayHandleRandomUniformReal`.
  ///
  /// @param length Specifies the length of the generated array.
  /// @param seed Provides a seed to use for the pseudorandom numbers. To prevent confusion
  /// between the seed and the length, the type of the seed is a `viskores::Vec` of size 1. To
  /// specify the seed, declare it in braces. For example, to construct a random array of
  /// size 50 with seed 123, use `ArrayHandleRandomUniformReal(50, { 123 })`.
  explicit ArrayHandleRandomUniformReal(viskores::Id length,
                                        SeedType seed = { std::random_device{}() })
    : Superclass(viskores::cont::ArrayHandleRandomUniformBits{ length, seed },
                 detail::CanonicalFunctor<Real>{})
  {
  }
};

} // cont
} // viskores
#endif //viskores_count_ArrayHandleRandomUniformReal_h
