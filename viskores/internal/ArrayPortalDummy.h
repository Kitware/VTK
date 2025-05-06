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
#ifndef viskores_internal_ArrayPortalDummy
#define viskores_internal_ArrayPortalDummy

#include <viskores/Assert.h>
#include <viskores/Types.h>

namespace viskores
{
namespace internal
{

/// A class that can be used in place of an `ArrayPortal` when the `ArrayPortal` is
/// not actually supported. It allows templates to be compiled, but will cause undefined
/// behavior if actually used.
template <typename T>
struct ArrayPortalDummy
{
  using ValueType = T;

  VISKORES_EXEC_CONT viskores::Id GetNumberOfValues() const { return 0; }

  VISKORES_EXEC_CONT ValueType Get(viskores::Id) const
  {
    VISKORES_ASSERT(false && "Tried to use a dummy portal.");
    return ValueType{};
  }
};

}
} // namespace viskores::internal

#endif //viskores_internal_ArrayPortalDummy
