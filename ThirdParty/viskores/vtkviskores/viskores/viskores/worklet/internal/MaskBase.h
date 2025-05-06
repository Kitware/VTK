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
#ifndef viskores_worklet_internal_MaskBase_h
#define viskores_worklet_internal_MaskBase_h

#include <viskores/internal/DecayHelpers.h>
#include <viskores/internal/ExportMacros.h>

namespace viskores
{
namespace worklet
{
namespace internal
{

/// Base class for all mask classes.
///
/// This allows Viskores to determine when a parameter
/// is a mask type instead of a worklet parameter.
///
struct VISKORES_ALWAYS_EXPORT MaskBase
{
};

template <typename T>
using is_mask = std::is_base_of<MaskBase, viskores::internal::remove_cvref<T>>;
}
}
}
#endif
