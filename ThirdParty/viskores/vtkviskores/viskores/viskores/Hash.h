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
#ifndef viskores_Hash_h
#define viskores_Hash_h

#include <viskores/TypeTraits.h>
#include <viskores/Types.h>
#include <viskores/VecTraits.h>

namespace viskores
{

using HashType = viskores::UInt32;

namespace detail
{

static constexpr viskores::HashType FNV1A_OFFSET = 2166136261;
static constexpr viskores::HashType FNV1A_PRIME = 16777619;

/// \brief Performs an FNV-1a hash on 32-bit integers returning a 32-bit hash
///
template <typename InVecType>
VISKORES_EXEC_CONT inline viskores::HashType HashFNV1a32(const InVecType& inVec)
{
  using Traits = viskores::VecTraits<InVecType>;
  const viskores::IdComponent numComponents = Traits::GetNumberOfComponents(inVec);

  viskores::HashType hash = FNV1A_OFFSET;
  for (viskores::IdComponent index = 0; index < numComponents; index++)
  {
    viskores::HashType dataBits =
      static_cast<viskores::HashType>(Traits::GetComponent(inVec, index));
    hash = (hash * FNV1A_PRIME) ^ dataBits;
  }

  return hash;
}

/// \brief Performs an FNV-1a hash on 64-bit integers returning a 32-bit hash
///
template <typename InVecType>
VISKORES_EXEC_CONT inline viskores::HashType HashFNV1a64(const InVecType& inVec)
{
  using Traits = viskores::VecTraits<InVecType>;
  const viskores::IdComponent numComponents = Traits::GetNumberOfComponents(inVec);

  viskores::HashType hash = FNV1A_OFFSET;
  for (viskores::IdComponent index = 0; index < numComponents; index++)
  {
    viskores::UInt64 allDataBits =
      static_cast<viskores::UInt64>(Traits::GetComponent(inVec, index));
    viskores::HashType upperDataBits =
      static_cast<viskores::HashType>((allDataBits & 0xFFFFFFFF00000000L) >> 32);
    hash = (hash * FNV1A_PRIME) ^ upperDataBits;
    viskores::HashType lowerDataBits =
      static_cast<viskores::HashType>(allDataBits & 0x00000000FFFFFFFFL);
    hash = (hash * FNV1A_PRIME) ^ lowerDataBits;
  }

  return hash;
}

// If you get a compile error saying that there is no implementation of the class HashChooser,
// then you have tried to make a hash from an invalid type (like a float).
template <typename NumericTag, std::size_t DataSize>
struct HashChooser;

template <>
struct HashChooser<viskores::TypeTraitsIntegerTag, 4>
{
  template <typename InVecType>
  VISKORES_EXEC_CONT static viskores::HashType Hash(const InVecType& inVec)
  {
    return viskores::detail::HashFNV1a32(inVec);
  }
};

template <>
struct HashChooser<viskores::TypeTraitsIntegerTag, 8>
{
  template <typename InVecType>
  VISKORES_EXEC_CONT static viskores::HashType Hash(const InVecType& inVec)
  {
    return viskores::detail::HashFNV1a64(inVec);
  }
};

} // namespace detail

/// \brief Returns a 32-bit hash on a group of integer-type values.
///
/// The input to the hash is expected to be a viskores::Vec or a Vec-like object. The values can be
/// either 32-bit integers or 64-bit integers (signed or unsigned). Regardless, the resulting hash
/// is an unsigned 32-bit integer.
///
/// The hash is designed to minimize the probability of collisions, but collisions are always
/// possible. Thus, when using these hashes there should be a contingency for dealing with
/// collisions.
///
template <typename InVecType>
VISKORES_EXEC_CONT inline viskores::HashType Hash(const InVecType& inVec)
{
  using VecTraits = viskores::VecTraits<InVecType>;
  using ComponentType = typename VecTraits::ComponentType;
  using ComponentTraits = viskores::TypeTraits<ComponentType>;
  using Chooser = detail::HashChooser<typename ComponentTraits::NumericTag, sizeof(ComponentType)>;
  return Chooser::Hash(inVec);
}

} // namespace viskores

#endif //viskores_Hash_h
