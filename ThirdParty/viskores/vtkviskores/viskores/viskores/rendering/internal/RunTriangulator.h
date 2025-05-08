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
#ifndef viskores_rendering_internal_RunTriangulator_h
#define viskores_rendering_internal_RunTriangulator_h

#include <viskores/rendering/viskores_rendering_export.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/RuntimeDeviceTracker.h>
#include <viskores/cont/UnknownCellSet.h>

namespace viskores
{
namespace rendering
{
namespace internal
{

/// This is a wrapper around the Triangulator worklet so that the
/// implementation of the triangulator only gets compiled once. This function
/// really is a stop-gap. Eventually, the Triangulator should be moved to
/// filters, and filters should be compiled in a library (for the same reason).
///
VISKORES_RENDERING_EXPORT
void RunTriangulator(const viskores::cont::UnknownCellSet& cellSet,
                     viskores::cont::ArrayHandle<viskores::Id4>& indices,
                     viskores::Id& numberOfTriangles,
                     const viskores::cont::Field& ghostField = viskores::cont::Field());
}
}
} // namespace viskores::rendering::internal

#endif //viskores_rendering_internal_RunTriangulator_h
