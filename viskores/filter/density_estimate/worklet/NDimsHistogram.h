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

#ifndef viskores_worklet_NDimsHistogram_h
#define viskores_worklet_NDimsHistogram_h

#include <viskores/Math.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/filter/density_estimate/worklet/histogram/ComputeNDHistogram.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/Field.h>

namespace viskores
{
namespace worklet
{

class NDimsHistogram
{
public:
  void SetNumOfDataPoints(viskores::Id _numDataPoints)
  {
    NumDataPoints = _numDataPoints;

    // Initialize bin1DIndex array
    viskores::cont::ArrayHandleConstant<viskores::Id> constant0Array(0, NumDataPoints);
    viskores::cont::ArrayCopy(constant0Array, Bin1DIndex);
  }

  // Add a field and the bin number for this field along with specific range of the data
  // Return: binDelta is delta of a bin
  template <typename HandleType>
  void AddField(const HandleType& fieldArray,
                viskores::Id numberOfBins,
                viskores::Range& rangeOfValues,
                viskores::Float64& binDelta,
                bool rangeProvided = false)
  {
    NumberOfBins.push_back(numberOfBins);

    if (fieldArray.GetNumberOfValues() != NumDataPoints)
    {
      throw viskores::cont::ErrorBadValue("Array lengths does not match");
    }
    else
    {
      auto computeBins = [&](auto resolvedField)
      {
        // CastAndCallWithExtractedArray should give us an ArrayHandleRecombineVec
        using T = typename std::decay_t<decltype(resolvedField)>::ValueType::ComponentType;
        viskores::cont::ArrayHandleRecombineVec<T> recombineField{ resolvedField };
        if (recombineField.GetNumberOfComponents() != 1)
        {
          VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                         "NDHistogram expects scalar fields, but was given field with "
                           << recombineField.GetNumberOfComponents()
                           << " components. Extracting first component.");
        }
        viskores::cont::ArrayHandleStride<T> field =
          viskores::cont::ArrayExtractComponent(recombineField, 0);
        if (!rangeProvided)
        {
          const viskores::Vec<T, 2> initValue(viskores::cont::ArrayGetValue(0, field));
          viskores::Vec<T, 2> minMax =
            viskores::cont::Algorithm::Reduce(field, initValue, viskores::MinAndMax<T>());
          rangeOfValues.Min = static_cast<viskores::Float64>(minMax[0]);
          rangeOfValues.Max = static_cast<viskores::Float64>(minMax[1]);
        }
        binDelta = viskores::worklet::histogram::compute_delta(
          rangeOfValues.Min, rangeOfValues.Max, numberOfBins);

        viskores::worklet::histogram::SetHistogramBin<T> binWorklet(
          numberOfBins, rangeOfValues.Min, binDelta);
        viskores::cont::Invoker{}(binWorklet, field, this->Bin1DIndex, this->Bin1DIndex);
      };
      fieldArray.CastAndCallWithExtractedArray(computeBins);
    }
  }

  // Execute N-Dim histogram worklet to get N-Dims histogram from input fields
  // Input arguments:
  //   binId: returned bin id of NDims-histogram, binId has n arrays, if length of fieldName is n
  //   freqs: returned frequency(count) array
  //     Note: the ND-histogram is returned in the fashion of sparse representation.
  //           (no zero frequency in freqs array)
  //           the length of all arrays in binId and freqs array must be the same
  //           if the length of fieldNames is n (compute a n-dimensional hisotgram)
  //           freqs[i] is the frequency of the bin with bin Ids{ binId[0][i], binId[1][i], ... binId[n-1][i] }
  void Run(std::vector<viskores::cont::ArrayHandle<viskores::Id>>& binId,
           viskores::cont::ArrayHandle<viskores::Id>& freqs)
  {
    binId.resize(NumberOfBins.size());

    // Sort the resulting bin(1D) array for counting
    viskores::cont::Algorithm::Sort(Bin1DIndex);

    // Count frequency of each bin
    viskores::cont::ArrayHandleConstant<viskores::Id> constArray(1, NumDataPoints);
    viskores::cont::Algorithm::ReduceByKey(
      Bin1DIndex, constArray, Bin1DIndex, freqs, viskores::Add());

    //convert back to multi variate binId
    for (viskores::Id i = static_cast<viskores::Id>(NumberOfBins.size()) - 1; i >= 0; i--)
    {
      const viskores::Id nFieldBins = NumberOfBins[static_cast<size_t>(i)];
      viskores::worklet::histogram::ConvertHistBinToND binWorklet(nFieldBins);
      viskores::worklet::DispatcherMapField<viskores::worklet::histogram::ConvertHistBinToND>
        convertHistBinToNDDispatcher(binWorklet);
      size_t vectorId = static_cast<size_t>(i);
      convertHistBinToNDDispatcher.Invoke(Bin1DIndex, Bin1DIndex, binId[vectorId]);
    }
  }

private:
  std::vector<viskores::Id> NumberOfBins;
  viskores::cont::ArrayHandle<viskores::Id> Bin1DIndex;
  viskores::Id NumDataPoints;
};
}
} // namespace viskores::worklet

#endif // viskores_worklet_NDimsHistogram_h
