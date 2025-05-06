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
#ifndef viskores_cont_ArrayHandleRandomUniformBits_h
#define viskores_cont_ArrayHandleRandomUniformBits_h

#include <random>
#include <viskores/cont/ArrayHandleImplicit.h>
#include <viskores/random/Philox.h>

namespace viskores
{
namespace cont
{

namespace detail
{
struct PhiloxFunctor
{
  using SeedType = viskores::Vec<viskores::UInt32, 1>;

  PhiloxFunctor() = default;

  explicit PhiloxFunctor(SeedType seed)
    : Seed(seed)
  {
  }

  VISKORES_EXEC_CONT
  viskores::UInt64 operator()(viskores::Id index) const
  {
    using philox_functor = viskores::random::PhiloxFunctor2x32x10;
    using counters_type = typename philox_functor::counters_type;

    auto idx = static_cast<viskores::UInt64>(index);
    counters_type counters{ static_cast<viskores::UInt32>(idx),
                            static_cast<viskores::UInt32>(idx >> 32) };
    counters_type result = philox_functor{}(counters, Seed);
    return static_cast<viskores::UInt64>(result[0]) |
      static_cast<viskores::UInt64>(result[1]) << 32;
  }

private:
  // This is logically a const, however, this make the Functor non-copyable which is required
  // by Viskores infrastructure (e.g. ArrayHandleTransform.)
  SeedType Seed;
}; // class PhiloxFunctor
} // namespace detail

/// @brief An `ArrayHandle` that provides a source of random bits
///
/// `ArrayHandleRandomUniformBits` is a specialization of `ArrayHandleImplicit`.
/// It takes a user supplied seed and hash it with the a given index value. The
/// hashed value is the value of the array at that position.
///
/// Currently, Philox2x32x10 as described in the
///   "Parallel Random Numbers: As Easy as 1, 2, 3," Proceedings of the
///   International Conference for High Performance Computing, Networking,
///   Storage and Analysis (SC11)
/// is used as the hash function.
///
/// Note: In contrast to traditional random number generator,
/// `ArrayHandleRandomUniformBits` does not have "state", i.e. multiple calls
/// the Get() method with the same index will always return the same hash value.
/// To get a new set of random bits, create a new `ArrayHandleRandomUniformBits`
/// with a different seed.
class VISKORES_ALWAYS_EXPORT ArrayHandleRandomUniformBits
  : public viskores::cont::ArrayHandleImplicit<detail::PhiloxFunctor>
{
public:
  using SeedType = viskores::Vec<viskores::UInt32, 1>;

  VISKORES_ARRAY_HANDLE_SUBCLASS_NT(ArrayHandleRandomUniformBits,
                                    (viskores::cont::ArrayHandleImplicit<detail::PhiloxFunctor>));

  /// Construct an `ArrayHandleRandomUniformBits`.
  ///
  /// @param length Specifies the length of the generated array.
  /// @param seed Provides a seed to use for the pseudorandom numbers. To prevent confusion
  /// between the seed and the length, the type of the seed is a `viskores::Vec` of size 1. To
  /// specify the seed, declare it in braces. For example, to construct a random array of
  /// size 50 with seed 123, use `ArrayHandleRandomUniformBits(50, { 123 })`.
  explicit ArrayHandleRandomUniformBits(viskores::Id length,
                                        SeedType seed = { std::random_device{}() })
    : Superclass(detail::PhiloxFunctor(seed), length)
  {
  }
}; // class ArrayHandleRandomUniformBits
}
} // namespace viskores::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION

namespace viskores
{
namespace cont
{
}
} // namespace viskores::cont
/// @endcond
#endif //viskores_cont_ArrayHandleRandomUniformBits_h
