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

#include <viskores/worklet/MaskSelect.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleView.h>

namespace
{

struct ReverseOutputToThreadMap : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn outputToThreadMap,
                                FieldIn maskArray,
                                WholeArrayOut threadToOutputMap);
  using ExecutionSignature = void(_1, InputIndex, _2, _3);

  template <typename MaskType, typename ThreadToOutputPortal>
  VISKORES_EXEC void operator()(viskores::Id threadIndex,
                                viskores::Id outputIndex,
                                MaskType mask,
                                ThreadToOutputPortal threadToOutput) const
  {
    if (mask)
    {
      threadToOutput.Set(threadIndex, outputIndex);
    }
  }
};

VISKORES_CONT static viskores::worklet::MaskSelect::ThreadToOutputMapType
BuildThreadToOutputMapWithFind(viskores::Id numThreads,
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

template <typename MaskArrayType>
VISKORES_CONT static viskores::worklet::MaskSelect::ThreadToOutputMapType
BuildThreadToOutputMapWithCopy(viskores::Id numThreads,
                               const viskores::cont::ArrayHandle<viskores::Id>& outputToThreadMap,
                               const MaskArrayType& maskArray,
                               viskores::cont::DeviceAdapterId device)
{
  viskores::worklet::MaskSelect::ThreadToOutputMapType threadToOutputMap;
  threadToOutputMap.Allocate(numThreads);

  viskores::worklet::DispatcherMapField<ReverseOutputToThreadMap> dispatcher;
  dispatcher.SetDevice(device);
  dispatcher.Invoke(outputToThreadMap, maskArray, threadToOutputMap);

  return threadToOutputMap;
}

VISKORES_CONT static viskores::worklet::MaskSelect::ThreadToOutputMapType
BuildThreadToOutputMapAllOn(viskores::Id numThreads, viskores::cont::DeviceAdapterId device)
{
  viskores::worklet::MaskSelect::ThreadToOutputMapType threadToOutputMap;
  threadToOutputMap.Allocate(numThreads);
  viskores::cont::Algorithm::Copy(
    device,
    viskores::cont::make_ArrayHandleCounting<viskores::Id>(0, 1, numThreads),
    threadToOutputMap);
  return threadToOutputMap;
}

struct MaskBuilder
{
  template <typename ArrayHandleType>
  void operator()(const ArrayHandleType& maskArray,
                  viskores::worklet::MaskSelect::ThreadToOutputMapType& threadToOutputMap,
                  viskores::cont::DeviceAdapterId device)
  {
    viskores::cont::ArrayHandle<viskores::Id> outputToThreadMap;
    viskores::Id numThreads = viskores::cont::Algorithm::ScanExclusive(
      device, viskores::cont::make_ArrayHandleCast<viskores::Id>(maskArray), outputToThreadMap);
    VISKORES_ASSERT(numThreads <= maskArray.GetNumberOfValues());

    // We have implemented two different ways to compute the thread to output map. The first way is
    // to use a binary search on each thread index into the output map. The second way is to
    // schedule on each output and copy the the index to the thread map. The first way is faster
    // for output sizes that are small relative to the input and also tends to be well load
    // balanced. The second way is faster for larger outputs.
    //
    // The former is obviously faster for one thread and the latter is obviously faster when all
    // outputs have a thread. We have to guess for values in the middle. I'm using if the square of
    // the number of threads is less than the number of outputs because it is easy to compute.
    if (numThreads == maskArray.GetNumberOfValues())
    { //fast path when everything is on
      threadToOutputMap = BuildThreadToOutputMapAllOn(numThreads, device);
    }
    else if ((numThreads * numThreads) < maskArray.GetNumberOfValues())
    {
      threadToOutputMap = BuildThreadToOutputMapWithFind(numThreads, outputToThreadMap, device);
    }
    else
    {
      threadToOutputMap =
        BuildThreadToOutputMapWithCopy(numThreads, outputToThreadMap, maskArray, device);
    }
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
