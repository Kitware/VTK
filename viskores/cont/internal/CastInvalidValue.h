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
#ifndef viskores_cont_internal_CastInvalidValue_h
#define viskores_cont_internal_CastInvalidValue_h

#include <viskores/Math.h>
#include <viskores/Types.h>
#include <viskores/VecTraits.h>

namespace viskores
{
namespace cont
{
namespace internal
{

/// \brief Convert an invalid value to something type-appropriate.
///
/// There are algorithms in Viskores that require a placeholder for invalid values in an array
/// or field. For example, when probing something, a probe location outside of the source data
/// has to be set to something.
///
/// Often we want to set this to something like NaN to make it clear that this is invalid.
/// However, integer types cannot represent these non-finite numbers.
///
/// For convenience, it is easiest to allow the user to specify the invalid value as a
/// viskores::Float64 and use this function to convert it to something type-appropriate.
///
template <typename T>
T CastInvalidValue(viskores::Float64 invalidValue)
{
  using ComponentType = typename viskores::VecTraits<T>::BaseComponentType;

  if (std::is_same<viskores::TypeTraitsIntegerTag,
                   typename viskores::TypeTraits<T>::NumericTag>::value)
  {
    // Casting to integer types
    if (viskores::IsFinite(invalidValue))
    {
      return T(static_cast<ComponentType>(invalidValue));
    }
    else if (viskores::IsInf(invalidValue) && (invalidValue > 0))
    {
      return T(std::numeric_limits<ComponentType>::max());
    }
    else
    {
      return T(std::numeric_limits<ComponentType>::min());
    }
  }
  else
  {
    // Not an integer type. Assume can be directly cast
    return T(static_cast<ComponentType>(invalidValue));
  }
}
}
}
} // namespace viskores::cont::internal

#endif //viskores_cont_internal_CastInvalidValue_h
