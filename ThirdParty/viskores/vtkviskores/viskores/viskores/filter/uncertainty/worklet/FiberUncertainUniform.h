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

#ifndef viskores_filter_uncertainty_FiberUncertainUniform_inner_h
#define viskores_filter_uncertainty_FiberUncertainUniform_inner_h
#include <iostream>
#include <utility>
#include <vector>
#include <viskores/filter/uncertainty/worklet/FiberUncertainUniform.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace detail
{
class MultiVariateMonteCarlo : public viskores::worklet::WorkletMapField
{
public:
  MultiVariateMonteCarlo(const viskores::Range& rangeAxis1,
                         const viskores::Range& rangeAxis2,
                         const viskores::Id numSamples)
    : RangeAxis1(rangeAxis1)
    , RangeAxis2(rangeAxis2)
    , NumSamples(numSamples){};

  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, FieldOut, WholeArrayIn);

  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  template <typename Min1,
            typename Max1,
            typename Min2,
            typename Max2,
            typename OutCellFieldType,
            typename RandomPortalType>

  VISKORES_EXEC void operator()(const Min1& ensembleMin1,
                                const Max1& ensembleMax1,
                                const Min2& ensembleMin2,
                                const Max2& ensembleMax2,
                                OutCellFieldType& probability,
                                const RandomPortalType& randomPortal) const

  {
    Min1 min1_user = static_cast<Min1>(this->RangeAxis1.Min);
    Min2 min2_user = static_cast<Min2>(this->RangeAxis2.Min);
    Max1 max1_user = static_cast<Max1>(this->RangeAxis1.Max);
    Max2 max2_user = static_cast<Max2>(this->RangeAxis2.Max);

    viskores::Id nonZeroCases = 0;
    viskores::FloatDefault mcProbability = 0.0;

    Min1 min1_intersection = viskores::Max(min1_user, ensembleMin1);
    Min2 min2_intersection = viskores::Max(min2_user, ensembleMin2);
    Max1 max1_intersection = viskores::Min(max1_user, ensembleMax1);
    Max2 max2_intersection = viskores::Min(max2_user, ensembleMax2);

    // Case where data has no uncertainty
    if ((ensembleMin1 == ensembleMax1) && (ensembleMin2 == ensembleMax2))
    {
      // Check if certain data point is within user-specified trait
      if ((ensembleMin1 <= max1_user) && (ensembleMin1 >= min1_user) &&
          (ensembleMin2 <= max2_user) && (ensembleMin2 >= min2_user))
      {
        nonZeroCases = this->NumSamples;
      }
    }
    // Case where data has uncertainty
    else if ((ensembleMin1 < ensembleMax1) && (ensembleMin2 == ensembleMax2))
    {
      if ((min1_intersection < max1_intersection) && (ensembleMin2 >= min2_user) &&
          (ensembleMin2 <= max2_user))
      {
        viskores::Float64 rangeX = ensembleMax1 - ensembleMin1;
        for (viskores::IdComponent i = 0; i < this->NumSamples; i++)
        {
          viskores::Float64 r1 = randomPortal.Get(i);
          viskores::Float64 n1 = ensembleMin1 + r1 * rangeX;

          if ((n1 > min1_user) && (n1 < max1_user))
          {
            nonZeroCases++;
          }
        }
      }
    }
    else if ((ensembleMin1 == ensembleMax1) && (ensembleMin2 < ensembleMax2))
    {
      if ((min2_intersection < max2_intersection) && (ensembleMin1 >= min1_user) &&
          (ensembleMin1 <= max1_user))
      {
        viskores::Float64 rangeY = ensembleMax2 - ensembleMin2;
        for (viskores::IdComponent i = 0; i < this->NumSamples; i++)
        {
          viskores::Float64 r2 = randomPortal.Get(i + this->NumSamples);
          viskores::Float64 n2 = ensembleMin2 + r2 * rangeY;

          if ((n2 > min2_user) && (n2 < max2_user))
          {
            nonZeroCases++;
          }
        }
      }
    }
    else
    {
      viskores::Float64 rangeX = ensembleMax1 - ensembleMin1;
      viskores::Float64 rangeY = ensembleMax2 - ensembleMin2;
      for (viskores::IdComponent i = 0; i < this->NumSamples; i++)
      {
        viskores::Float64 r1 = randomPortal.Get(i);
        viskores::Float64 r2 = randomPortal.Get(i + this->NumSamples);
        viskores::Float64 n1 = ensembleMin1 + r1 * rangeX;
        viskores::Float64 n2 = ensembleMin2 + r2 * rangeY;

        if ((n1 > min1_user) && (n1 < max1_user) && (n2 > min2_user) && (n2 < max2_user))
        {
          nonZeroCases++;
        }
      }
    }

    mcProbability = static_cast<viskores::FloatDefault>(nonZeroCases) /
      static_cast<viskores::FloatDefault>(this->NumSamples);
    probability = mcProbability;

    return;
  }

private:
  viskores::Range RangeAxis1;
  viskores::Range RangeAxis2;
  viskores::Id NumSamples = 1;
};

class MultiVariateClosedForm : public viskores::worklet::WorkletMapField
{
public:
  MultiVariateClosedForm(const viskores::Range& rangeAxis1, const viskores::Range& rangeAxis2)
    : RangeAxis1(rangeAxis1)
    , RangeAxis2(rangeAxis2)
  {
  }

  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);

  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename Min1, typename Max1, typename Min2, typename Max2, typename OutCellFieldType>
  VISKORES_EXEC void operator()(const Min1& ensembleMin1,
                                const Max1& ensembleMax1,
                                const Min2& ensembleMin2,
                                const Max2& ensembleMax2,
                                OutCellFieldType& probability) const
  {

    Min1 min1_user = static_cast<Min1>(this->RangeAxis1.Min);
    Min2 min2_user = static_cast<Min2>(this->RangeAxis2.Min);
    Max1 max1_user = static_cast<Max1>(this->RangeAxis1.Max);
    Max2 max2_user = static_cast<Max2>(this->RangeAxis2.Max);

    viskores::Float64 intersectionArea = 0.0;
    viskores::Float64 intersectionProbability = 0.0;
    viskores::Float64 intersectionHeight = 0.0;
    viskores::Float64 intersectionWidth = 0.0;

    Min1 min1_intersection = viskores::Max(min1_user, ensembleMin1);
    Min2 min2_intersection = viskores::Max(min2_user, ensembleMin2);
    Max1 max1_intersection = viskores::Min(max1_user, ensembleMax1);
    Max2 max2_intersection = viskores::Min(max2_user, ensembleMax2);

    if ((ensembleMin1 == ensembleMax1) && (ensembleMin2 == ensembleMax2))
    {
      if ((ensembleMin1 <= max1_user) && (ensembleMin1 >= min1_user) &&
          (ensembleMin2 <= max2_user) && (ensembleMin2 >= min2_user))
      {
        intersectionProbability = 1.0;
      }
      else
      {
        intersectionProbability = 0.0;
      }
    }
    else if ((ensembleMin1 < ensembleMax1) && (ensembleMin2 == ensembleMax2))
    {
      if ((min1_intersection < max1_intersection) && (ensembleMin2 >= min2_user) &&
          (ensembleMin2 <= max2_user))
      {
        intersectionProbability =
          (max1_intersection - min1_intersection) / (ensembleMax1 - ensembleMin1);
      }
      else
      {
        intersectionProbability = 0.0;
      }
    }
    else if ((ensembleMin1 == ensembleMax1) && (ensembleMin2 < ensembleMax2))
    {
      if ((min2_intersection < max2_intersection) && (ensembleMin1 >= min1_user) &&
          (ensembleMin1 <= max1_user))
      {
        intersectionProbability =
          (max2_intersection - min2_intersection) / (ensembleMax2 - ensembleMin2);
      }
      else
      {
        intersectionProbability = 0.0;
      }
    }
    else
    {
      intersectionHeight = max2_intersection - min2_intersection;
      intersectionWidth = max1_intersection - min1_intersection;

      viskores::FloatDefault DataArea =
        (ensembleMax1 - ensembleMin1) * (ensembleMax2 - ensembleMin2);

      if ((intersectionHeight > 0) && (intersectionWidth > 0) &&
          (min1_intersection < max1_intersection) && (min2_intersection < max2_intersection))
      {
        intersectionArea = intersectionHeight * intersectionWidth;
        intersectionProbability = intersectionArea / DataArea;
      }
    }
    probability = static_cast<OutCellFieldType>(intersectionProbability);
    return;
  }

private:
  viskores::Range RangeAxis1;
  viskores::Range RangeAxis2;
};

class MultiVariateMean : public viskores::worklet::WorkletMapField
{
public:
  MultiVariateMean(const viskores::Range& rangeAxis1, const viskores::Range& rangeAxis2)
    : RangeAxis1(rangeAxis1)
    , RangeAxis2(rangeAxis2)
  {
  }

  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);

  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename Min1, typename Max1, typename Min2, typename Max2, typename OutCellFieldType>

  VISKORES_EXEC void operator()(const Min1& ensembleMin1,
                                const Max1& ensembleMax1,
                                const Min2& ensembleMin2,
                                const Max2& ensembleMax2,
                                OutCellFieldType& probability) const
  {

    Min1 min1_user = static_cast<Min1>(this->RangeAxis1.Min);
    Min2 min2_user = static_cast<Min2>(this->RangeAxis2.Min);
    Max1 max1_user = static_cast<Max1>(this->RangeAxis1.Max);
    Max2 max2_user = static_cast<Max2>(this->RangeAxis2.Max);

    Min1 Xmean = (ensembleMin1 + ensembleMax1) / 2;
    Min2 Ymean = (ensembleMin2 + ensembleMax2) / 2;

    if ((Xmean <= max1_user) && (Xmean >= min1_user) && (Ymean <= max2_user) &&
        (Ymean >= min2_user))
    {
      probability = 1.0;
      return;
    }
    else
    {
      probability = 0.0;
      return;
    }
  }

private:
  viskores::Range RangeAxis1;
  viskores::Range RangeAxis2;
};

class MultiVariateTruth : public viskores::worklet::WorkletMapField
{
public:
  MultiVariateTruth(const viskores::Range& rangeAxis1, const viskores::Range& rangeAxis2)
    : RangeAxis1(rangeAxis1)
    , RangeAxis2(rangeAxis2)
  {
  }

  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);

  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename Min1, typename Max1, typename Min2, typename Max2, typename OutCellFieldType>

  VISKORES_EXEC void operator()(const Min1& ensembleMin1,
                                const Max1& ensembleMax1,
                                const Min2& ensembleMin2,
                                const Max2& ensembleMax2,
                                OutCellFieldType& probability) const
  {

    Min1 min1_user = static_cast<Min1>(this->RangeAxis1.Min);
    Min2 min2_user = static_cast<Min2>(this->RangeAxis2.Min);
    Max1 max1_user = static_cast<Max1>(this->RangeAxis1.Max);
    Max2 max2_user = static_cast<Max2>(this->RangeAxis2.Max);

    if ((ensembleMax1 <= max1_user) && (ensembleMin1 >= min1_user) && (ensembleMax2 <= max2_user) &&
        (ensembleMin2 >= min2_user))
    {
      probability = 1.0;
      return;
    }
    else
    {
      probability = 0.0;
      return;
    }
  }

private:
  viskores::Range RangeAxis1;
  viskores::Range RangeAxis2;
};

}
}
}

#endif
