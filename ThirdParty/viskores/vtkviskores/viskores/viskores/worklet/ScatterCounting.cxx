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

#include <viskores/worklet/ScatterCounting.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleConcatenate.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandleView.h>
#include <viskores/cont/ErrorBadValue.h>

#include <viskores/exec/FunctorBase.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

#include <sstream>

namespace
{

VISKORES_CONT
inline viskores::cont::ArrayHandleConcatenate<
  viskores::cont::ArrayHandleConstant<viskores::Id>,
  viskores::cont::ArrayHandleView<viskores::cont::ArrayHandle<viskores::Id>>>
ShiftArrayHandleByOne(const viskores::cont::ArrayHandle<viskores::Id>& array)
{
  return viskores::cont::make_ArrayHandleConcatenate(
    viskores::cont::make_ArrayHandleConstant<viskores::Id>(0, 1),
    viskores::cont::make_ArrayHandleView(array, 0, array.GetNumberOfValues() - 1));
}

struct ReverseInputToOutputMapWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn outputStartIndices,
                                FieldIn outputEndIndices,
                                WholeArrayOut outputToInputMap,
                                WholeArrayOut visit);
  using ExecutionSignature = void(_1, _2, _3, _4, InputIndex);
  using InputDomain = _2;

  template <typename OutputMapType, typename VisitType>
  VISKORES_EXEC void operator()(viskores::Id outputStartIndex,
                                viskores::Id outputEndIndex,
                                const OutputMapType& outputToInputMap,
                                const VisitType& visit,
                                viskores::Id inputIndex) const
  {
    viskores::IdComponent visitIndex = 0;
    for (viskores::Id outputIndex = outputStartIndex; outputIndex < outputEndIndex; outputIndex++)
    {
      outputToInputMap.Set(outputIndex, inputIndex);
      visit.Set(outputIndex, visitIndex);
      visitIndex++;
    }
  }

  VISKORES_CONT
  static void Run(const viskores::cont::ArrayHandle<viskores::Id>& inputToOutputMap,
                  const viskores::cont::ArrayHandle<viskores::Id>& outputToInputMap,
                  const viskores::cont::ArrayHandle<viskores::IdComponent>& visit,
                  viskores::cont::DeviceAdapterId device)
  {
    viskores::worklet::DispatcherMapField<ReverseInputToOutputMapWorklet> dispatcher;
    dispatcher.SetDevice(device);
    dispatcher.Invoke(
      ShiftArrayHandleByOne(inputToOutputMap), inputToOutputMap, outputToInputMap, visit);
  }
};

struct SubtractToVisitIndexWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn startsOfGroup, WholeArrayOut visit);
  using ExecutionSignature = void(InputIndex, _1, _2);
  using InputDomain = _1;

  template <typename VisitType>
  VISKORES_EXEC void operator()(viskores::Id inputIndex,
                                viskores::Id startOfGroup,
                                const VisitType& visit) const
  {
    viskores::IdComponent visitIndex =
      static_cast<viskores::IdComponent>(inputIndex - startOfGroup);
    visit.Set(inputIndex, visitIndex);
  }
};

} // anonymous namespace

namespace viskores
{
namespace worklet
{
namespace detail
{

struct ScatterCountingBuilder
{
  template <typename CountArrayType>
  VISKORES_CONT static void BuildArrays(viskores::worklet::ScatterCounting* self,
                                        const CountArrayType& countArray,
                                        viskores::cont::DeviceAdapterId device,
                                        bool saveInputToOutputMap)
  {
    VISKORES_IS_ARRAY_HANDLE(CountArrayType);

    self->InputRange = countArray.GetNumberOfValues();

    // The input to output map is actually built off by one. The first entry
    // is actually for the second value. The last entry is the total number of
    // output. This off-by-one is so that an upper bound find will work when
    // building the output to input map. Later we will either correct the
    // map or delete it.
    viskores::cont::ArrayHandle<viskores::Id> inputToOutputMapOffByOne;
    viskores::Id outputSize = viskores::cont::Algorithm::ScanInclusive(
      device,
      viskores::cont::make_ArrayHandleCast(countArray, viskores::Id()),
      inputToOutputMapOffByOne);

    // We have implemented two different ways to compute the output to input
    // map. The first way is to use a binary search on each output index into
    // the input map. The second way is to schedule on each input and
    // iteratively fill all the output indices for that input. The first way is
    // faster for output sizes that are small relative to the input (typical in
    // Marching Cubes, for example) and also tends to be well load balanced.
    // The second way is faster for larger outputs (typical in triangulation,
    // for example). We will use the first method for small output sizes and
    // the second for large output sizes. Toying with this might be a good
    // place for optimization.
    if (outputSize < self->InputRange)
    {
      BuildOutputToInputMapWithFind(self, outputSize, device, inputToOutputMapOffByOne);
    }
    else
    {
      BuildOutputToInputMapWithIterate(self, outputSize, device, inputToOutputMapOffByOne);
    }

    if (saveInputToOutputMap)
    {
      // Since we are saving it, correct the input to output map.
      viskores::cont::Algorithm::Copy(
        device, ShiftArrayHandleByOne(inputToOutputMapOffByOne), self->InputToOutputMap);
    }
  }

  VISKORES_CONT static void BuildOutputToInputMapWithFind(
    viskores::worklet::ScatterCounting* self,
    viskores::Id outputSize,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::ArrayHandle<viskores::Id> inputToOutputMapOffByOne)
  {
    viskores::cont::ArrayHandleIndex outputIndices(outputSize);
    viskores::cont::Algorithm::UpperBounds(
      device, inputToOutputMapOffByOne, outputIndices, self->OutputToInputMap);

    viskores::cont::ArrayHandle<viskores::Id> startsOfGroups;

    // This find gives the index of the start of a group.
    viskores::cont::Algorithm::LowerBounds(
      device, self->OutputToInputMap, self->OutputToInputMap, startsOfGroups);

    self->VisitArray.Allocate(outputSize);
    viskores::worklet::DispatcherMapField<SubtractToVisitIndexWorklet> dispatcher;
    dispatcher.SetDevice(device);
    dispatcher.Invoke(startsOfGroups, self->VisitArray);
  }

  VISKORES_CONT static void BuildOutputToInputMapWithIterate(
    viskores::worklet::ScatterCounting* self,
    viskores::Id outputSize,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::ArrayHandle<viskores::Id> inputToOutputMapOffByOne)
  {
    self->OutputToInputMap.Allocate(outputSize);
    self->VisitArray.Allocate(outputSize);

    ReverseInputToOutputMapWorklet::Run(
      inputToOutputMapOffByOne, self->OutputToInputMap, self->VisitArray, device);
  }

  template <typename ArrayType>
  void operator()(const ArrayType& countArray,
                  viskores::cont::DeviceAdapterId device,
                  bool saveInputToOutputMap,
                  viskores::worklet::ScatterCounting* self)
  {
    BuildArrays(self, countArray, device, saveInputToOutputMap);
  }
};
}
}
} // namespace viskores::worklet::detail

void viskores::worklet::ScatterCounting::BuildArrays(
  const viskores::cont::UnknownArrayHandle& countArray,
  viskores::cont::DeviceAdapterId device,
  bool saveInputToOutputMap)
{
  VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "ScatterCounting::BuildArrays");

  countArray.CastAndCallForTypes<CountTypes, viskores::List<viskores::cont::StorageTagBasic>>(
    viskores::worklet::detail::ScatterCountingBuilder(), device, saveInputToOutputMap, this);
}
