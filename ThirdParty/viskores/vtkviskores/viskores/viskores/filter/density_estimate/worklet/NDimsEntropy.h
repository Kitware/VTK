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

#ifndef viskores_worklet_NDimsEntropy_h
#define viskores_worklet_NDimsEntropy_h

#include <viskores/Math.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/filter/density_estimate/worklet/NDimsHistogram.h>
#include <viskores/filter/density_estimate/worklet/histogram/ComputeNDEntropy.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/Field.h>

namespace viskores
{
namespace worklet
{

class NDimsEntropy
{
public:
  void SetNumOfDataPoints(viskores::Id _numDataPoints)
  {
    NumDataPoints = _numDataPoints;
    NdHistogram.SetNumOfDataPoints(_numDataPoints);
  }

  // Add a field and the bin for this field
  // Return: rangeOfRange is min max value of this array
  //         binDelta is delta of a bin
  template <typename HandleType>
  void AddField(const HandleType& fieldArray, viskores::Id numberOfBins)
  {
    viskores::Range range;
    viskores::Float64 delta;

    NdHistogram.AddField(fieldArray, numberOfBins, range, delta);
  }

  // Execute the entropy computation filter given
  // fields and number of bins of each fields
  // Returns:
  // Entropy (log2) of the field of the data
  viskores::Float64 Run()
  {
    std::vector<viskores::cont::ArrayHandle<viskores::Id>> binIds;
    viskores::cont::ArrayHandle<viskores::Id> freqs;
    NdHistogram.Run(binIds, freqs);

    ///// calculate sum of frequency of the histogram /////
    viskores::Id initFreqSumValue = 0;
    viskores::Id freqSum =
      viskores::cont::Algorithm::Reduce(freqs, initFreqSumValue, viskores::Sum());

    ///// calculate information content of each bin using self-define worklet /////
    viskores::cont::ArrayHandle<viskores::Float64> informationContent;
    viskores::worklet::histogram::SetBinInformationContent binWorklet(
      static_cast<viskores::Float64>(freqSum));
    viskores::worklet::DispatcherMapField<viskores::worklet::histogram::SetBinInformationContent>
      setBinInformationContentDispatcher(binWorklet);
    setBinInformationContentDispatcher.Invoke(freqs, informationContent);

    ///// calculate entropy by summing up information conetent of all bins /////
    viskores::Float64 initEntropyValue = 0;
    viskores::Float64 entropy =
      viskores::cont::Algorithm::Reduce(informationContent, initEntropyValue, viskores::Sum());

    return entropy;
  }

private:
  viskores::worklet::NDimsHistogram NdHistogram;
  viskores::Id NumDataPoints{};
};
}
} // namespace viskores::worklet

#endif // viskores_worklet_NDimsEntropy_h
