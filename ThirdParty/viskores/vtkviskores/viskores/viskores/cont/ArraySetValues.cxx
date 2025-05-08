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

#include <viskores/cont/ArraySetValues.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <viskores/List.h>
#include <viskores/TypeList.h>

void viskores::cont::internal::ArraySetValuesImpl(const viskores::cont::UnknownArrayHandle& ids,
                                                  const viskores::cont::UnknownArrayHandle& values,
                                                  const viskores::cont::UnknownArrayHandle& data,
                                                  std::false_type)
{
  auto idArray = ids.ExtractComponent<viskores::Id>(0, viskores::CopyFlag::On);
  VISKORES_ASSERT(ids.GetNumberOfValues() == values.GetNumberOfValues());

  bool copied = false;
  viskores::ListForEach(
    [&](auto base)
    {
      using T = decltype(base);
      if (!copied && data.IsBaseComponentType<T>())
      {
        viskores::IdComponent numComponents = data.GetNumberOfComponentsFlat();
        VISKORES_ASSERT(values.GetNumberOfComponentsFlat() == numComponents);

        for (viskores::IdComponent componentIdx = 0; componentIdx < numComponents; ++componentIdx)
        {
          auto valuesArray = values.ExtractComponent<T>(componentIdx, viskores::CopyFlag::On);
          auto dataArray = data.ExtractComponent<T>(componentIdx, viskores::CopyFlag::Off);
          auto permutedArray = viskores::cont::make_ArrayHandlePermutation(idArray, dataArray);

          bool copiedComponent = false;
          copiedComponent = viskores::cont::TryExecute(
            [&](auto device)
            {
              if (dataArray.IsOnDevice(device))
              {
                viskores::cont::DeviceAdapterAlgorithm<decltype(device)>::Copy(valuesArray,
                                                                               permutedArray);
                return true;
              }
              return false;
            });

          if (!copiedComponent)
          { // Fallback to control-side copy
            const viskores::Id numVals = ids.GetNumberOfValues();
            auto inPortal = valuesArray.ReadPortal();
            auto outPortal = permutedArray.WritePortal();
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
    throw viskores::cont::ErrorBadType("Unable to set values in array of type " +
                                       data.GetArrayTypeName());
  }
}
