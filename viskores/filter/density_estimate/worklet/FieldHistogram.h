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

#ifndef viskores_worklet_FieldHistogram_h
#define viskores_worklet_FieldHistogram_h

#include <viskores/Math.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayGetValues.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/Field.h>

namespace
{
// GCC creates false positive warnings for signed/unsigned char* operations.
// This occurs because the values are implicitly casted up to int's for the
// operation, and than  casted back down to char's when return.
// This causes a false positive warning, even when the values is within
// the value types range
#if defined(VISKORES_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc
template <typename T>
T compute_delta(T fieldMinValue, T fieldMaxValue, viskores::Id num)
{
  using VecType = viskores::VecTraits<T>;
  const T fieldRange = fieldMaxValue - fieldMinValue;
  return fieldRange / static_cast<typename VecType::ComponentType>(num);
}
#if defined(VISKORES_GCC)
#pragma GCC diagnostic pop
#endif // gcc
}

namespace viskores
{
namespace worklet
{

//simple functor that prints basic statistics
class FieldHistogram
{
public:
  // For each value set the bin it should be in
  template <typename FieldType>
  class SetHistogramBin : public viskores::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn value, FieldOut binIndex);
    using ExecutionSignature = void(_1, _2);
    using InputDomain = _1;

    viskores::Id numberOfBins;
    FieldType minValue;
    FieldType delta;

    VISKORES_CONT
    SetHistogramBin(viskores::Id numberOfBins0, FieldType minValue0, FieldType delta0)
      : numberOfBins(numberOfBins0)
      , minValue(minValue0)
      , delta(delta0)
    {
    }

    VISKORES_EXEC
    void operator()(const FieldType& value, viskores::Id& binIndex) const
    {
      binIndex = static_cast<viskores::Id>((value - minValue) / delta);
      if (binIndex < 0)
        binIndex = 0;
      else if (binIndex >= numberOfBins)
        binIndex = numberOfBins - 1;
    }
  };

  // Calculate the adjacent difference between values in ArrayHandle
  class AdjacentDifference : public viskores::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn inputIndex, WholeArrayIn counts, FieldOut outputCount);
    using ExecutionSignature = void(_1, _2, _3);
    using InputDomain = _1;

    template <typename WholeArrayType>
    VISKORES_EXEC void operator()(const viskores::Id& index,
                                  const WholeArrayType& counts,
                                  viskores::Id& difference) const
    {
      if (index == 0)
        difference = counts.Get(index);
      else
        difference = counts.Get(index) - counts.Get(index - 1);
    }
  };

  // Execute the histogram binning filter given data and number of bins
  // Returns:
  // min value of the bins
  // delta/range of each bin
  // number of values in each bin
  template <typename FieldType, typename Storage>
  void Run(viskores::cont::ArrayHandle<FieldType, Storage> fieldArray,
           viskores::Id numberOfBins,
           viskores::Range& rangeOfValues,
           FieldType& binDelta,
           viskores::cont::ArrayHandle<viskores::Id>& binArray)
  {
    const viskores::Vec<FieldType, 2> initValue{ viskores::cont::ArrayGetValue(0, fieldArray) };

    viskores::Vec<FieldType, 2> result =
      viskores::cont::Algorithm::Reduce(fieldArray, initValue, viskores::MinAndMax<FieldType>());

    this->Run(fieldArray, numberOfBins, result[0], result[1], binDelta, binArray);

    //update the users data
    rangeOfValues = viskores::Range(result[0], result[1]);
  }

  // Execute the histogram binning filter given data and number of bins, min,
  // max values.
  // Returns:
  // number of values in each bin
  template <typename FieldType, typename Storage>
  void Run(viskores::cont::ArrayHandle<FieldType, Storage> fieldArray,
           viskores::Id numberOfBins,
           FieldType fieldMinValue,
           FieldType fieldMaxValue,
           FieldType& binDelta,
           viskores::cont::ArrayHandle<viskores::Id>& binArray)
  {
    const viskores::Id numberOfValues = fieldArray.GetNumberOfValues();

    const FieldType fieldDelta = compute_delta(fieldMinValue, fieldMaxValue, numberOfBins);

    // Worklet fills in the bin belonging to each value
    viskores::cont::ArrayHandle<viskores::Id> binIndex;
    binIndex.Allocate(numberOfValues);

    // Worklet to set the bin number for each data value
    SetHistogramBin<FieldType> binWorklet(numberOfBins, fieldMinValue, fieldDelta);
    viskores::worklet::DispatcherMapField<SetHistogramBin<FieldType>> setHistogramBinDispatcher(
      binWorklet);
    setHistogramBinDispatcher.Invoke(fieldArray, binIndex);

    // Sort the resulting bin array for counting
    viskores::cont::Algorithm::Sort(binIndex);

    // Get the upper bound of each bin number
    viskores::cont::ArrayHandle<viskores::Id> totalCount;
    viskores::cont::ArrayHandleCounting<viskores::Id> binCounter(0, 1, numberOfBins);
    viskores::cont::Algorithm::UpperBounds(binIndex, binCounter, totalCount);

    // Difference between adjacent items is the bin count
    viskores::worklet::DispatcherMapField<AdjacentDifference> dispatcher;
    dispatcher.Invoke(binCounter, totalCount, binArray);

    //update the users data
    binDelta = fieldDelta;
  }
};
}
} // namespace viskores::worklet

#endif // viskores_worklet_FieldHistogram_h
