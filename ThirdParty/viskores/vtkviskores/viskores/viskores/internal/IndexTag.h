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
#ifndef viskores_internal_StaticIndex_h
#define viskores_internal_StaticIndex_h

#include <viskores/Types.h>

namespace viskores
{
namespace internal
{

/// \brief A convenience tag to represent static indices.
///
/// Some classes like \c FunctionInterface have a list of items that have
/// numeric indices that must be resolved at compile time. Typically these are
/// referenced with an integer template argument. However, such template
/// arguments have to be explicitly defined in the template. They cannot be
/// resolved through function or method arguments. In such cases, it is
/// convenient to use this tag to encapsulate the index.
///
template <viskores::IdComponent Index>
struct IndexTag
{
  static constexpr viskores::IdComponent INDEX = Index;
};
}
} // namespace viskores::internal

#endif //viskores_internal_StaticIndex_h
