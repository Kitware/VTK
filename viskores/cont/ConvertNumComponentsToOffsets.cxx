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

#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/ErrorBadType.h>

#include <viskores/cont/internal/ConvertNumComponentsToOffsetsTemplate.h>

#include <viskores/List.h>

namespace
{

struct CallNumToOffsets
{
  template <typename BaseType>
  VISKORES_CONT void operator()(BaseType,
                                const viskores::cont::UnknownArrayHandle& numComponentsArray,
                                viskores::cont::ArrayHandle<viskores::Id>& offsetsArray,
                                viskores::cont::DeviceAdapterId device,
                                bool& converted)
  {
    if (!numComponentsArray.IsBaseComponentType<BaseType>())
    {
      // Not the right type.
      return;
    }

    viskores::cont::internal::ConvertNumComponentsToOffsetsTemplate(
      numComponentsArray.ExtractComponent<BaseType>(0, viskores::CopyFlag::Off), // TODO: Allow copy
      offsetsArray,
      device);
    converted = true;
  }
};

} // anonymous namespace

namespace viskores
{
namespace cont
{

void ConvertNumComponentsToOffsets(const viskores::cont::UnknownArrayHandle& numComponentsArray,
                                   viskores::cont::ArrayHandle<viskores::Id>& offsetsArray,
                                   viskores::Id& componentsArraySize,
                                   viskores::cont::DeviceAdapterId device)
{
  viskores::cont::ConvertNumComponentsToOffsets(numComponentsArray, offsetsArray, device);

  componentsArraySize =
    viskores::cont::ArrayGetValue(offsetsArray.GetNumberOfValues() - 1, offsetsArray);
}

void ConvertNumComponentsToOffsets(const viskores::cont::UnknownArrayHandle& numComponentsArray,
                                   viskores::cont::ArrayHandle<viskores::Id>& offsetsArray,
                                   viskores::cont::DeviceAdapterId device)
{
  if (numComponentsArray.GetNumberOfComponentsFlat() > 1)
  {
    throw viskores::cont::ErrorBadType(
      "ConvertNumComponentsToOffsets only works with arrays of integers, not Vecs.");
  }

  using SupportedTypes = viskores::List<viskores::Int32, viskores::Int64>;
  bool converted = false;
  viskores::ListForEach(
    CallNumToOffsets{}, SupportedTypes{}, numComponentsArray, offsetsArray, device, converted);
  if (!converted)
  {
    internal::ThrowCastAndCallException(numComponentsArray, typeid(SupportedTypes));
  }
}

viskores::cont::ArrayHandle<viskores::Id> ConvertNumComponentsToOffsets(
  const viskores::cont::UnknownArrayHandle& numComponentsArray,
  viskores::Id& componentsArraySize,
  viskores::cont::DeviceAdapterId device)
{
  viskores::cont::ArrayHandle<viskores::Id> offsetsArray;
  viskores::cont::ConvertNumComponentsToOffsets(
    numComponentsArray, offsetsArray, componentsArraySize, device);
  return offsetsArray;
}

viskores::cont::ArrayHandle<viskores::Id> ConvertNumComponentsToOffsets(
  const viskores::cont::UnknownArrayHandle& numComponentsArray,
  viskores::cont::DeviceAdapterId device)
{
  viskores::cont::ArrayHandle<viskores::Id> offsetsArray;
  viskores::cont::ConvertNumComponentsToOffsets(numComponentsArray, offsetsArray, device);
  return offsetsArray;
}

} // namespace viskores::cont
} // namespace viskores
