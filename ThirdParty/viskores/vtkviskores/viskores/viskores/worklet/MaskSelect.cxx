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

#include <viskores/worklet/MaskSelectTemplate.h>

VISKORES_CONT viskores::worklet::MaskSelect::ThreadToOutputMapType
viskores::worklet::internal::BuildThreadToOutputMapWithFind(
  viskores::Id numThreads,
  viskores::cont::ArrayHandle<viskores::Id> outputToThreadMap,
  viskores::cont::DeviceAdapterId device)
{
  viskores::worklet::MaskSelect::ThreadToOutputMapType threadToOutputMap;

  viskores::Id outputSize = outputToThreadMap.GetNumberOfValues();

  viskores::cont::ArrayHandleIndex threadIndices(numThreads);
  viskores::cont::Algorithm::UpperBounds(
    device,
    viskores::cont::make_ArrayHandleView(outputToThreadMap, 1, outputSize - 1),
    threadIndices,
    threadToOutputMap);

  return threadToOutputMap;
}

VISKORES_CONT viskores::worklet::MaskSelect::ThreadToOutputMapType
viskores::worklet::internal::BuildThreadToOutputMapAllOn(viskores::Id numThreads,
                                                         viskores::cont::DeviceAdapterId device)
{
  viskores::worklet::MaskSelect::ThreadToOutputMapType threadToOutputMap;
  threadToOutputMap.Allocate(numThreads);
  viskores::cont::Algorithm::Copy(
    device,
    viskores::cont::make_ArrayHandleCounting<viskores::Id>(0, 1, numThreads),
    threadToOutputMap);
  return threadToOutputMap;
}

namespace
{

struct MaskBuilder
{
  template <typename ArrayHandleType>
  void operator()(const ArrayHandleType& maskArray,
                  viskores::worklet::MaskSelect::ThreadToOutputMapType& threadToOutputMap,
                  viskores::cont::DeviceAdapterId device)
  {
    // We could call MaskSelectBuild directly, but this ensures that the MaskSelectTemplate
    // constructor is working correctly.
    viskores::worklet::MaskSelectTemplate maskSelect{ maskArray, device };
    threadToOutputMap = maskSelect.GetThreadToOutputMap(viskores::Id{});
  }
};

} // anonymous namespace

viskores::worklet::MaskSelect::ThreadToOutputMapType viskores::worklet::MaskSelect::Build(
  const viskores::cont::UnknownArrayHandle& maskArray,
  viskores::cont::DeviceAdapterId device)
{
  VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "MaskSelect::Build");

  viskores::worklet::MaskSelect::ThreadToOutputMapType threadToOutputMap;
  maskArray.CastAndCallForTypes<MaskTypes, viskores::List<viskores::cont::StorageTagBasic>>(
    MaskBuilder(), threadToOutputMap, device);
  return threadToOutputMap;
}
