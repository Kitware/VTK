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
#ifndef viskores_StaticAssert_h
#define viskores_StaticAssert_h


#include <type_traits>

#define VISKORES_STATIC_ASSERT(condition) \
  static_assert((condition), "Failed static assert: " #condition)
#define VISKORES_STATIC_ASSERT_MSG(condition, message) static_assert((condition), message)

namespace viskores
{

template <bool noError>
struct ReadTheSourceCodeHereForHelpOnThisError;

template <>
struct ReadTheSourceCodeHereForHelpOnThisError<true> : std::true_type
{
};

} // namespace viskores

#define VISKORES_READ_THE_SOURCE_CODE_FOR_HELP(noError) \
  VISKORES_STATIC_ASSERT(viskores::ReadTheSourceCodeHereForHelpOnThisError<noError>::value)

#endif //viskores_StaticAssert_h
