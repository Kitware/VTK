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
#ifndef viskores_worklet_ImageDifference_h
#define viskores_worklet_ImageDifference_h

#include <viskores/VecTraits.h>
#include <viskores/VectorAnalysis.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DeviceAdapterAlgorithm.h>

#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletPointNeighborhood.h>

namespace viskores
{
namespace worklet
{

class ImageDifferenceNeighborhood : public viskores::worklet::WorkletPointNeighborhood
{
public:
  using ControlSignature = void(CellSetIn, FieldInNeighborhood, FieldIn, FieldOut, FieldOut);
  using ExecutionSignature = void(_2, _3, Boundary, _4, _5);
  using InputDomain = _1;

  ImageDifferenceNeighborhood(const viskores::IdComponent& radius,
                              const viskores::FloatDefault& threshold)
    : ShiftRadius(radius)
    , Threshold(threshold)
  {
  }

  template <typename InputFieldPortalType>
  VISKORES_EXEC void operator()(
    const viskores::exec::FieldNeighborhood<InputFieldPortalType>& primaryNeighborhood,
    const typename InputFieldPortalType::ValueType& secondary,
    const viskores::exec::BoundaryState& boundary,
    typename InputFieldPortalType::ValueType& diff,
    viskores::FloatDefault& diffThreshold) const
  {
    using T = typename InputFieldPortalType::ValueType;

    auto minIndices = boundary.MinNeighborIndices(this->ShiftRadius);
    auto maxIndices = boundary.MaxNeighborIndices(this->ShiftRadius);

    T minPixelDiff{};
    viskores::FloatDefault minPixelDiffThreshold = 10000.0f;
    for (viskores::IdComponent i = minIndices[0]; i <= maxIndices[0]; i++)
    {
      for (viskores::IdComponent j = minIndices[1]; j <= maxIndices[1]; j++)
      {
        for (viskores::IdComponent k = minIndices[2]; k <= maxIndices[2]; k++)
        {
          diff = viskores::Abs(primaryNeighborhood.Get(i, j, k) - secondary);
          diffThreshold = static_cast<viskores::FloatDefault>(viskores::Magnitude(diff));
          if (diffThreshold < this->Threshold)
          {
            return;
          }
          if (diffThreshold < minPixelDiffThreshold)
          {
            minPixelDiffThreshold = diffThreshold;
            minPixelDiff = diff;
          }
        }
      }
    }
    diff = minPixelDiff;
    diffThreshold = minPixelDiffThreshold;
  }

private:
  viskores::IdComponent ShiftRadius;
  viskores::FloatDefault Threshold;
};

class ImageDifference : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldIn, FieldOut, FieldOut);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  ImageDifference() = default;

  template <typename T, viskores::IdComponent Size>
  VISKORES_EXEC void operator()(const viskores::Vec<T, Size>& primary,
                                const viskores::Vec<T, Size>& secondary,
                                viskores::Vec<T, Size>& diff,
                                viskores::FloatDefault& diffThreshold) const
  {
    diff = viskores::Abs(primary - secondary);
    diffThreshold = static_cast<viskores::FloatDefault>(viskores::Magnitude(diff));
  }
};


} // viskores::worklet
} // viskores

#endif // viskores_worklet_ImageDifference_h
