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
#ifndef viskores_worklet_HistSampling_h
#define viskores_worklet_HistSampling_h

#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
class AcceptanceProbsWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldIn, FieldIn, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, _3, _4);
  template <typename TypeOutPortal>
  VISKORES_EXEC void operator()(const viskores::FloatDefault& targetSampleNum,
                                const viskores::Id& binIndex,
                                const viskores::Id& binCount,
                                TypeOutPortal arrayOutPortal) const
  {
    if (binCount < 1 || targetSampleNum < 0.000001)
    {
      arrayOutPortal.Set(binIndex, 0.0);
    }
    else
    {
      arrayOutPortal.Set(binIndex, targetSampleNum / static_cast<viskores::FloatDefault>(binCount));
    }
  }
};

class LookupWorklet : public viskores::worklet::WorkletMapField
{
private:
  viskores::Id m_num_bins;
  viskores::Float64 m_min;
  viskores::Float64 m_bin_delta;

public:
  LookupWorklet(viskores::Id num_bins, viskores::Float64 min_value, viskores::Float64 bin_delta)
    : m_num_bins(num_bins)
    , m_min(min_value)
    , m_bin_delta(bin_delta)
  {
  }

  using ControlSignature = void(FieldIn, FieldOut, WholeArrayIn, FieldIn);
  using ExecutionSignature = _2(_1, _3, _4);
  template <typename TablePortal, typename FieldType>
  VISKORES_EXEC viskores::FloatDefault operator()(const FieldType& field_value,
                                                  TablePortal table,
                                                  const viskores::FloatDefault& random) const
  {
    viskores::Id bin = static_cast<viskores::Id>((field_value - m_min) / m_bin_delta);
    if (bin < 0)
    {
      bin = 0;
    }
    if (bin >= m_num_bins)
    {
      bin = m_num_bins - 1;
    }
    return random < table.Get(bin);
  }
};
};
} // viskores::worklet


#endif // viskores_worklet_HistSampling_h
