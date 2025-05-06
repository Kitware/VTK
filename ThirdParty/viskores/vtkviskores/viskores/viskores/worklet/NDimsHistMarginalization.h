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

#ifndef viskores_worklet_NDimsHistMarginalization_h
#define viskores_worklet_NDimsHistMarginalization_h

#include <viskores/Math.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/DataSet.h>
#include <viskores/filter/density_estimate/worklet/histogram/ComputeNDHistogram.h>
#include <viskores/filter/density_estimate/worklet/histogram/MarginalizeNDHistogram.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/Field.h>

namespace viskores
{
namespace worklet
{


class NDimsHistMarginalization
{
public:
  // Execute the histogram (conditional) marginalization,
  //   given the multi-variable histogram(binId, freqIn)
  //   , marginalVariable and marginal condition
  // Input arguments:
  //   binId, freqsIn: input ND-histogram in the fashion of sparse representation
  //                   (definition of binId and frqIn please refer to NDimsHistogram.h),
  //                   (binId.size() is the number of variables)
  //   numberOfBins: number of bins of each variable (length of numberOfBins must be the same as binId.size() )
  //   marginalVariables: length is the same as number of variables.
  //                      1 indicates marginal variable, otherwise 0.
  //   conditionFunc: The Condition function for non-marginal variable.
  //                  This func takes two arguments (viskores::Id var, viskores::Id binId) and return bool
  //                  var is index of variable and binId is bin index in the variable var
  //                  return true indicates considering this bin into final marginal histogram
  //                  more details can refer to example in UnitTestNDimsHistMarginalization.cxx
  //   marginalBinId, marginalFreqs: return marginalized histogram in the fashion of sparse representation
  //                                 the definition is the same as (binId and freqsIn)
  template <typename BinaryCompare>
  void Run(const std::vector<viskores::cont::ArrayHandle<viskores::Id>>& binId,
           viskores::cont::ArrayHandle<viskores::Id>& freqsIn,
           viskores::cont::ArrayHandle<viskores::Id>& numberOfBins,
           viskores::cont::ArrayHandle<bool>& marginalVariables,
           BinaryCompare conditionFunc,
           std::vector<viskores::cont::ArrayHandle<viskores::Id>>& marginalBinId,
           viskores::cont::ArrayHandle<viskores::Id>& marginalFreqs)
  {
    //total variables
    viskores::Id numOfVariable = static_cast<viskores::Id>(binId.size());

    const viskores::Id numberOfValues = freqsIn.GetNumberOfValues();
    viskores::cont::ArrayHandleConstant<viskores::Id> constant0Array(0, numberOfValues);
    viskores::cont::ArrayHandle<viskores::Id> bin1DIndex;
    viskores::cont::ArrayCopy(constant0Array, bin1DIndex);

    viskores::cont::ArrayHandle<viskores::Id> freqs;
    viskores::cont::ArrayCopy(freqsIn, freqs);
    viskores::Id numMarginalVariables = 0; //count num of marginal variables
    const auto marginalPortal = marginalVariables.ReadPortal();
    const auto numBinsPortal = numberOfBins.ReadPortal();
    for (viskores::Id i = 0; i < numOfVariable; i++)
    {
      if (marginalPortal.Get(i) == true)
      {
        // Worklet to calculate 1D index for marginal variables
        numMarginalVariables++;
        const viskores::Id nFieldBins = numBinsPortal.Get(i);
        viskores::worklet::histogram::To1DIndex binWorklet(nFieldBins);
        viskores::worklet::DispatcherMapField<viskores::worklet::histogram::To1DIndex>
          to1DIndexDispatcher(binWorklet);
        size_t vecIndex = static_cast<size_t>(i);
        to1DIndexDispatcher.Invoke(binId[vecIndex], bin1DIndex, bin1DIndex);
      }
      else
      { //non-marginal variable
        // Worklet to set the frequency of entities which does not meet the condition
        // to 0 on non-marginal variables
        viskores::worklet::histogram::ConditionalFreq<BinaryCompare> conditionalFreqWorklet{
          conditionFunc
        };
        conditionalFreqWorklet.setVar(i);
        viskores::worklet::DispatcherMapField<
          viskores::worklet::histogram::ConditionalFreq<BinaryCompare>>
          cfDispatcher(conditionalFreqWorklet);
        size_t vecIndex = static_cast<size_t>(i);
        cfDispatcher.Invoke(binId[vecIndex], freqs, freqs);
      }
    }


    // Sort the freq array for counting by key(1DIndex)
    viskores::cont::Algorithm::SortByKey(bin1DIndex, freqs);

    // Add frequency within same 1d index bin (this get a nonSparse representation)
    viskores::cont::ArrayHandle<viskores::Id> nonSparseMarginalFreqs;
    viskores::cont::Algorithm::ReduceByKey(
      bin1DIndex, freqs, bin1DIndex, nonSparseMarginalFreqs, viskores::Add());

    // Convert to sparse representation(remove all zero freqncy entities)
    viskores::cont::ArrayHandle<viskores::Id> sparseMarginal1DBinId;
    viskores::cont::Algorithm::CopyIf(bin1DIndex, nonSparseMarginalFreqs, sparseMarginal1DBinId);
    viskores::cont::Algorithm::CopyIf(
      nonSparseMarginalFreqs, nonSparseMarginalFreqs, marginalFreqs);

    //convert back to multi variate binId
    marginalBinId.resize(static_cast<size_t>(numMarginalVariables));
    viskores::Id marginalVarIdx = numMarginalVariables - 1;
    for (viskores::Id i = numOfVariable - 1; i >= 0; i--)
    {
      if (marginalPortal.Get(i) == true)
      {
        const viskores::Id nFieldBins = numBinsPortal.Get(i);
        viskores::worklet::histogram::ConvertHistBinToND binWorklet(nFieldBins);
        viskores::worklet::DispatcherMapField<viskores::worklet::histogram::ConvertHistBinToND>
          convertHistBinToNDDispatcher(binWorklet);
        size_t vecIndex = static_cast<size_t>(marginalVarIdx);
        convertHistBinToNDDispatcher.Invoke(
          sparseMarginal1DBinId, sparseMarginal1DBinId, marginalBinId[vecIndex]);
        marginalVarIdx--;
      }
    }
  } //Run()

  // Execute the histogram marginalization WITHOUT CONDITION,
  // Please refer to the other Run() functions for the definition of input arguments.
  void Run(const std::vector<viskores::cont::ArrayHandle<viskores::Id>>& binId,
           viskores::cont::ArrayHandle<viskores::Id>& freqsIn,
           viskores::cont::ArrayHandle<viskores::Id>& numberOfBins,
           viskores::cont::ArrayHandle<bool>& marginalVariables,
           std::vector<viskores::cont::ArrayHandle<viskores::Id>>& marginalBinId,
           viskores::cont::ArrayHandle<viskores::Id>& marginalFreqs)
  {
    //total variables
    viskores::Id numOfVariable = static_cast<viskores::Id>(binId.size());

    const viskores::Id numberOfValues = freqsIn.GetNumberOfValues();
    viskores::cont::ArrayHandleConstant<viskores::Id> constant0Array(0, numberOfValues);
    viskores::cont::ArrayHandle<viskores::Id> bin1DIndex;
    viskores::cont::ArrayCopy(constant0Array, bin1DIndex);

    viskores::cont::ArrayHandle<viskores::Id> freqs;
    viskores::cont::ArrayCopy(freqsIn, freqs);
    viskores::Id numMarginalVariables = 0; //count num of marginal variables
    const auto marginalPortal = marginalVariables.ReadPortal();
    const auto numBinsPortal = numberOfBins.ReadPortal();
    for (viskores::Id i = 0; i < numOfVariable; i++)
    {
      if (marginalPortal.Get(i) == true)
      {
        // Worklet to calculate 1D index for marginal variables
        numMarginalVariables++;
        const viskores::Id nFieldBins = numBinsPortal.Get(i);
        viskores::worklet::histogram::To1DIndex binWorklet(nFieldBins);
        viskores::worklet::DispatcherMapField<viskores::worklet::histogram::To1DIndex>
          to1DIndexDispatcher(binWorklet);
        size_t vecIndex = static_cast<size_t>(i);
        to1DIndexDispatcher.Invoke(binId[vecIndex], bin1DIndex, bin1DIndex);
      }
    }

    // Sort the freq array for counting by key (1DIndex)
    viskores::cont::Algorithm::SortByKey(bin1DIndex, freqs);

    // Add frequency within same 1d index bin
    viskores::cont::Algorithm::ReduceByKey(
      bin1DIndex, freqs, bin1DIndex, marginalFreqs, viskores::Add());

    //convert back to multi variate binId
    marginalBinId.resize(static_cast<size_t>(numMarginalVariables));
    viskores::Id marginalVarIdx = numMarginalVariables - 1;
    for (viskores::Id i = numOfVariable - 1; i >= 0; i--)
    {
      if (marginalPortal.Get(i) == true)
      {
        const viskores::Id nFieldBins = numBinsPortal.Get(i);
        viskores::worklet::histogram::ConvertHistBinToND binWorklet(nFieldBins);
        viskores::worklet::DispatcherMapField<viskores::worklet::histogram::ConvertHistBinToND>
          convertHistBinToNDDispatcher(binWorklet);
        size_t vecIndex = static_cast<size_t>(marginalVarIdx);
        convertHistBinToNDDispatcher.Invoke(bin1DIndex, bin1DIndex, marginalBinId[vecIndex]);
        marginalVarIdx--;
      }
    }
  } //Run()
};
}
} // namespace viskores::worklet

#endif // viskores_worklet_NDimsHistMarginalization_h
