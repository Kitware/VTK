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

#include <viskores/cont/ArrayGetValues.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <viskores/List.h>
#include <viskores/TypeList.h>

void viskores::cont::internal::ArrayGetValuesImpl(const viskores::cont::UnknownArrayHandle& ids,
                                                  const viskores::cont::UnknownArrayHandle& data,
                                                  const viskores::cont::UnknownArrayHandle& output,
                                                  std::false_type)
{
  auto idArray = ids.ExtractComponent<viskores::Id>(0, viskores::CopyFlag::On);
  output.Allocate(ids.GetNumberOfValues());

  bool copied = false;
  viskores::ListForEach(
    [&](auto base)
    {
      using T = decltype(base);
      if (!copied && data.IsBaseComponentType<T>())
      {
        viskores::IdComponent numComponents = data.GetNumberOfComponentsFlat();
        VISKORES_ASSERT(output.GetNumberOfComponentsFlat() == numComponents);
        for (viskores::IdComponent componentIdx = 0; componentIdx < numComponents; ++componentIdx)
        {
          auto dataArray = data.ExtractComponent<T>(componentIdx, viskores::CopyFlag::On);
          auto outputArray = output.ExtractComponent<T>(componentIdx, viskores::CopyFlag::Off);
          auto permutedArray = viskores::cont::make_ArrayHandlePermutation(idArray, dataArray);

          bool copiedComponent = false;
          if (!dataArray.IsOnHost())
          {
            copiedComponent = viskores::cont::TryExecute(
              [&](auto device)
              {
                if (dataArray.IsOnDevice(device))
                {
                  viskores::cont::DeviceAdapterAlgorithm<decltype(device)>::Copy(permutedArray,
                                                                                 outputArray);
                  return true;
                }
                return false;
              });
          }

          if (!copiedComponent)
          { // Fallback to a control-side copy if the device copy fails or if the device
            // is undefined or if the data were already on the host. In this case, the
            // best we can do is grab the portals and copy one at a time on the host with
            // a for loop.
            const viskores::Id numVals = ids.GetNumberOfValues();
            auto inPortal = permutedArray.ReadPortal();
            auto outPortal = outputArray.WritePortal();
            for (viskores::Id i = 0; i < numVals; ++i)
            {
              outPortal.Set(i, inPortal.Get(i));
            }
          }
        }

        copied = true;
      }
    },
    viskores::TypeListBaseC{});

  if (!copied)
  {
    throw viskores::cont::ErrorBadType("Unable to get values from array of type " +
                                       data.GetArrayTypeName());
  }
}
