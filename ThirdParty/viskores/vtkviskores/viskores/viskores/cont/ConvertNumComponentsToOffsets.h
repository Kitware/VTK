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
#ifndef viskores_cont_ConvertNumComponentsToOffsets_h
#define viskores_cont_ConvertNumComponentsToOffsets_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <viskores/cont/viskores_cont_export.h>

namespace viskores
{
namespace cont
{


/// `ConvertNumComponentsToOffsets` takes an array of Vec sizes (i.e. the number of components in
/// each `Vec`) and returns an array of offsets to a packed array of such `Vec`s. The resulting
/// array can be used with `ArrayHandleGroupVecVariable`.
///
/// @param[in] numComponentsArray the input array that specifies the number of components in each group
/// Vec.
///
/// @param[out] offsetsArray (optional) the output \c ArrayHandle, which must have a value type of \c
/// viskores::Id. If the output \c ArrayHandle is not given, it is returned.
///
/// @param[in] componentsArraySize (optional) a reference to a \c viskores::Id and is filled with the
/// expected size of the component values array.
///
/// @param[in] device (optional) specifies the device on which to run the conversion.
///
/// Note that this function is pre-compiled for some set of `ArrayHandle` types. If you get a
/// warning about an inefficient conversion (or the operation fails outright), you might need to
/// use `viskores::cont::internal::ConvertNumComponentsToOffsetsTemplate`.
///
VISKORES_CONT_EXPORT void ConvertNumComponentsToOffsets(
  const viskores::cont::UnknownArrayHandle& numComponentsArray,
  viskores::cont::ArrayHandle<viskores::Id>& offsetsArray,
  viskores::Id& componentsArraySize,
  viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny{});

VISKORES_CONT_EXPORT void ConvertNumComponentsToOffsets(
  const viskores::cont::UnknownArrayHandle& numComponentsArray,
  viskores::cont::ArrayHandle<viskores::Id>& offsetsArray,
  viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny{});

VISKORES_CONT_EXPORT viskores::cont::ArrayHandle<viskores::Id> ConvertNumComponentsToOffsets(
  const viskores::cont::UnknownArrayHandle& numComponentsArray,
  viskores::Id& componentsArraySize,
  viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny{});

VISKORES_CONT_EXPORT viskores::cont::ArrayHandle<viskores::Id> ConvertNumComponentsToOffsets(
  const viskores::cont::UnknownArrayHandle& numComponentsArray,
  viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny{});

} // namespace viskores::cont
} // namespace viskores

#endif // viskores_cont_ConvertNumComponentsToOffsets_h
