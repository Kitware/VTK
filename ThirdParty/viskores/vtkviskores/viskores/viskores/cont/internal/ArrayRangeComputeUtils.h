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
#ifndef viskores_cont_internal_ArrayRangeComputeUtils_h
#define viskores_cont_internal_ArrayRangeComputeUtils_h

#include <viskores/VecTraits.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/viskores_cont_export.h>

namespace viskores
{
namespace cont
{
namespace internal
{

VISKORES_CONT_EXPORT viskores::Id2 GetFirstAndLastUnmaskedIndices(
  const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
  viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny{});

}
}
} // viskores::cont::internal

#endif // viskores_cont_internal_ArrayRangeComputeUtils_h
