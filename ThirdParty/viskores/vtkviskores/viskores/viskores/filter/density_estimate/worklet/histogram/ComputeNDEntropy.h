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

#ifndef viskores_worklet_ComputeNDEntropy_h
#define viskores_worklet_ComputeNDEntropy_h

#include <viskores/worklet/DispatcherMapField.h>

namespace viskores
{
namespace worklet
{
namespace histogram
{
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
}
}
} // namespace viskores::worklet

#endif // viskores_worklet_ComputeNDEntropy_h
