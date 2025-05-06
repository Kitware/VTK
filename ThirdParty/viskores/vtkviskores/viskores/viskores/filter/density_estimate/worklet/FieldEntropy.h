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

#ifndef viskores_worklet_FieldEntropy_h
#define viskores_worklet_FieldEntropy_h

#include <viskores/Math.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/filter/density_estimate/worklet/FieldHistogram.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/Field.h>

namespace viskores
{
namespace worklet
{

//simple functor that returns basic statistics
class FieldEntropy
{
public:
  // For each bin, calculate its information content (log2)
  class SetBinInformationContent : public viskores::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn freq, FieldOut informationContent);
    using ExecutionSignature = void(_1, _2);

    viskores::Float64 FreqSum;

    VISKORES_CONT
    explicit SetBinInformationContent(viskores::Float64 _freqSum)
      : FreqSum(_freqSum)
    {
    }

    template <typename FreqType>
    VISKORES_EXEC void operator()(const FreqType& freq, viskores::Float64& informationContent) const
    {
      viskores::Float64 p = ((viskores::Float64)freq) / FreqSum;
      if (p > 0)
        informationContent = -1 * p * viskores::Log2(p);
      else
        informationContent = 0;
    }
  };


  // Execute the entropy computation filter given data(a field) and number of bins
  // Returns:
  // Entropy (log2) of the field of the data
  template <typename FieldType, typename Storage>
  viskores::Float64 Run(viskores::cont::ArrayHandle<FieldType, Storage> fieldArray,
                        viskores::Id numberOfBins)
  {
    ///// calculate histogram using FieldHistogram worklet /////
    viskores::Range range;
    FieldType delta;
    viskores::cont::ArrayHandle<viskores::Id> binArray;
    viskores::worklet::FieldHistogram histogram;
    histogram.Run(fieldArray, numberOfBins, range, delta, binArray);

    ///// calculate sum of frequency of the histogram /////
    viskores::Id initFreqSumValue = 0;
    viskores::Id freqSum =
      viskores::cont::Algorithm::Reduce(binArray, initFreqSumValue, viskores::Sum());

    ///// calculate information content of each bin using self-define worklet /////
    viskores::cont::ArrayHandle<viskores::Float64> informationContent;
    SetBinInformationContent binWorklet(static_cast<viskores::Float64>(freqSum));
    viskores::worklet::DispatcherMapField<SetBinInformationContent>
      setBinInformationContentDispatcher(binWorklet);
    setBinInformationContentDispatcher.Invoke(binArray, informationContent);

    ///// calculate entropy by summing up information conetent of all bins /////
    viskores::Float64 initEntropyValue = 0;
    viskores::Float64 entropy =
      viskores::cont::Algorithm::Reduce(informationContent, initEntropyValue, viskores::Sum());

    return entropy;
  }
};
}
} // namespace viskores::worklet

#endif // viskores_worklet_FieldEntropy_h
