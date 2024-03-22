// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHashCombiner
 * @brief   Combine 4- and 8-byte integers into a single hash value.
 *
 * This templated class accepts one 4- or 8-byte integer
 * and combines it with an existing hash. It is useful for
 * combining hashes of strings with integer values such as
 * connectivity entries for shape primitives.
 *
 * This class was adapted from (i.e., uses the integer constants from)
 * boost::hash_combine but adds the templating on input value sizes.
 *
 * \sa vtkCellGridSidesQuery for an example of its use.
 */

#ifndef vtkHashCombiner_h
#define vtkHashCombiner_h

#include "vtkCommonCoreModule.h" // For export macro

#include <cstdint> // For uint32_t.

VTK_ABI_NAMESPACE_BEGIN

class VTKCOMMONCORE_EXPORT VTK_WRAPEXCLUDE vtkHashCombiner
{
public:
  /// Combine an integer \a k with the 64-bit hash \a h (which is modified on exit).
  template <typename T>
  void operator()(T& h, typename std::enable_if<sizeof(T) == 8, std::size_t>::type k)
  {
    constexpr T m = 0xc6a4a7935bd1e995ull;
    constexpr int r = 47;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;

    // Completely arbitrary number, to prevent 0's
    // from hashing to 0.
    h += 0xe6546b64;
  }

  /// Combine an integer \a k with the 32-bit hash \a h (which is modified on exit).
  template <typename T>
  void operator()(T& h, typename std::enable_if<sizeof(T) == 4, std::size_t>::type k)
  {
    constexpr std::uint32_t c1 = 0xcc9e2d51;
    constexpr std::uint32_t c2 = 0x1b873593;
    constexpr std::uint32_t r1 = 15;
    constexpr std::uint32_t r2 = 13;

    k *= c1;
    k = (k << r1) | (k >> (32 - r1));
    k *= c2;

    h ^= k;
    h = (h << r2) | (h >> (32 - r2));
    h = h * 5 + 0xe6546b64;
  }
};

VTK_ABI_NAMESPACE_END
#endif // vtkHashCombiner_h
