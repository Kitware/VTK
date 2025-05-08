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

#ifndef viskores_worklet_MarginalizeNDHistogram_h
#define viskores_worklet_MarginalizeNDHistogram_h

#include <viskores/worklet/DispatcherMapField.h>

namespace viskores
{
namespace worklet
{
namespace histogram
{
// Set freq of the entity, which does not meet the condition, to 0
template <class BinaryCompare>
class ConditionalFreq : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  explicit ConditionalFreq(BinaryCompare _bop)
    : bop(_bop)
  {
  }

  VISKORES_CONT
  void setVar(viskores::Id _var) { var = _var; }

  BinaryCompare bop;
  viskores::Id var{};

  using ControlSignature = void(FieldIn, FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3);

  VISKORES_EXEC
  void operator()(const viskores::Id& binIdIn,
                  const viskores::Id& freqIn,
                  viskores::Id& freqOut) const
  {
    if (bop(var, binIdIn))
      freqOut = freqIn;
    else
      freqOut = 0;
  }
};

// Convert multiple indices to 1D index
class To1DIndex : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn bin, FieldIn binIndexIn, FieldOut binIndexOut);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  viskores::Id numberOfBins;

  VISKORES_CONT
  explicit To1DIndex(viskores::Id numberOfBins0)
    : numberOfBins(numberOfBins0)
  {
  }

  VISKORES_EXEC
  void operator()(const viskores::Id& bin,
                  const viskores::Id& binIndexIn,
                  viskores::Id& binIndexOut) const
  {
    binIndexOut = binIndexIn * numberOfBins + bin;
  }
};
}
}
} // namespace viskores::worklet

#endif // viskores_worklet_MarginalizeNDHistogram_h
