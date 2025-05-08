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
#ifndef viskores_count_ArrayHandleRandomStandardNormal_h
#define viskores_count_ArrayHandleRandomStandardNormal_h

#include <viskores/Math.h>
#include <viskores/cont/ArrayHandleRandomUniformReal.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/ArrayHandleZip.h>

namespace viskores
{
namespace cont
{
namespace detail
{
struct BoxMuller
{
  VISKORES_EXEC_CONT viskores::Float32 operator()(
    const viskores::Pair<viskores::Float32, viskores::Float32>& uv) const
  {
    // We take two U(0, 1) and return one N(0, 1)
    return viskores::Sqrt(-2.0f * viskores::Log(uv.first)) *
      viskores::Cos(2.0f * viskores::TwoPif() * uv.second);
  }

  VISKORES_EXEC_CONT viskores::Float64 operator()(
    const viskores::Pair<viskores::Float64, viskores::Float64>& uv) const
  {
    // We take two U(0, 1) and return one N(0, 1)
    return viskores::Sqrt(-2.0 * viskores::Log(uv.first)) *
      viskores::Cos(2 * viskores::TwoPi() * uv.second);
  }
};
} //detail

/// @brief An `ArrayHandle` that provides a source of random numbers with a standard normal distribution.
///
/// `ArrayHandleRandomStandardNormal` takes a user supplied seed and hashes it to provide
/// a sequence of numbers drawn from a random standard normal distribution. The probability
/// density function of the numbers is @f$\frac{e^{-x^2/2}}{\sqrt{2\pi}}@f$. The range of possible
/// values is technically infinite, but the probability of large positive or negative numbers
/// becomes vanishingly small.
///
/// This array uses the Box-Muller transform to pick random numbers in the stanard normal
/// distribution.
///
/// Note: In contrast to traditional random number generator,
/// `ArrayHandleRandomStandardNormal` does not have "state", i.e. multiple calls
/// the Get() method with the same index will always return the same hash value.
/// To get a new set of random bits, create a new `ArrayHandleRandomUniformBits`
/// with a different seed.
template <typename Real = viskores::Float64>
class VISKORES_ALWAYS_EXPORT ArrayHandleRandomStandardNormal
  : public viskores::cont::ArrayHandleTransform<
      viskores::cont::ArrayHandleZip<viskores::cont::ArrayHandleRandomUniformReal<Real>,
                                     viskores::cont::ArrayHandleRandomUniformReal<Real>>,
      detail::BoxMuller>
{
public:
  using SeedType = viskores::Vec<viskores::UInt32, 1>;
  using UniformReal = viskores::cont::ArrayHandleRandomUniformReal<Real>;

  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleRandomStandardNormal,
    (ArrayHandleRandomStandardNormal<Real>),
    (viskores::cont::ArrayHandleTransform<
      viskores::cont::ArrayHandleZip<viskores::cont::ArrayHandleRandomUniformReal<Real>,
                                     viskores::cont::ArrayHandleRandomUniformReal<Real>>,
      detail::BoxMuller>));

  /// Construct an `ArrayHandleRandomStandardNormal`.
  ///
  /// @param length Specifies the length of the generated array.
  /// @param seed Provides a seed to use for the pseudorandom numbers. To prevent confusion
  /// between the seed and the length, the type of the seed is a `viskores::Vec` of size 1. To
  /// specify the seed, declare it in braces. For example, to construct a random array of
  /// size 50 with seed 123, use `ArrayHandleRandomStandardNormal(50, { 123 })`.
  explicit ArrayHandleRandomStandardNormal(viskores::Id length,
                                           SeedType seed = { std::random_device{}() })
    : Superclass(viskores::cont::make_ArrayHandleZip(UniformReal{ length, seed },
                                                     UniformReal{ length, { ~seed[0] } }),
                 detail::BoxMuller{})
  {
  }
};
}
}
#endif // viskores_count_ArrayHandleRandomStandardNormal_h
