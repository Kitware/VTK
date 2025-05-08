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

#ifndef viskores_worklet_ComputeNDHistogram_h
#define viskores_worklet_ComputeNDHistogram_h

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayGetValues.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace histogram
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

// For each value set the bin it should be in
template <typename FieldType>
class SetHistogramBin : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn value, FieldIn binIndexIn, FieldOut binIndexOut);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  viskores::Id numberOfBins;
  viskores::Float64 minValue;
  viskores::Float64 delta;

  VISKORES_CONT
  SetHistogramBin(viskores::Id numberOfBins0, viskores::Float64 minValue0, viskores::Float64 delta0)
    : numberOfBins(numberOfBins0)
    , minValue(minValue0)
    , delta(delta0)
  {
  }

  VISKORES_EXEC
  void operator()(const FieldType& value,
                  const viskores::Id& binIndexIn,
                  viskores::Id& binIndexOut) const
  {
    const viskores::Float64 fvalue = static_cast<viskores::Float64>(value);
    viskores::Id localBinIdx = static_cast<viskores::Id>((fvalue - minValue) / delta);
    if (localBinIdx < 0)
      localBinIdx = 0;
    else if (localBinIdx >= numberOfBins)
      localBinIdx = numberOfBins - 1;

    binIndexOut = binIndexIn * numberOfBins + localBinIdx;
  }
};

// Convert N-dims bin index into 1D index
class ConvertHistBinToND : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn bin1DIndexIn,
                                FieldOut bin1DIndexOut,
                                FieldOut oneVariableIndexOut);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  viskores::Id numberOfBins;

  VISKORES_CONT
  explicit ConvertHistBinToND(viskores::Id numberOfBins0)
    : numberOfBins(numberOfBins0)
  {
  }

  VISKORES_EXEC
  void operator()(const viskores::Id& bin1DIndexIn,
                  viskores::Id& bin1DIndexOut,
                  viskores::Id& oneVariableIndexOut) const
  {
    oneVariableIndexOut = bin1DIndexIn % numberOfBins;
    bin1DIndexOut = (bin1DIndexIn - oneVariableIndexOut) / numberOfBins;
  }
};
}
}
} // namespace viskores::worklet

#endif // viskores_worklet_ComputeNDHistogram_h
