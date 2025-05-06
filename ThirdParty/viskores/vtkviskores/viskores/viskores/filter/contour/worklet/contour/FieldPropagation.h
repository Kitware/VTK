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

#ifndef viskores_worklet_contour_FieldPropagation_h
#define viskores_worklet_contour_FieldPropagation_h

#include <viskores/VectorAnalysis.h>
#include <viskores/worklet/WorkletMapField.h>
namespace viskores
{
namespace worklet
{
namespace contour
{

// ---------------------------------------------------------------------------
class MapPointField : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn interpolation_ids,
                                FieldIn interpolation_weights,
                                WholeArrayIn inputField,
                                FieldOut output);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  VISKORES_CONT
  MapPointField() {}

  template <typename WeightType, typename InFieldPortalType, typename OutFieldType>
  VISKORES_EXEC void operator()(const viskores::Id2& low_high,
                                const WeightType& weight,
                                const InFieldPortalType& inPortal,
                                OutFieldType& result) const
  {
    //fetch the low / high values from inPortal
    OutFieldType lowValue = inPortal.Get(low_high[0]);
    OutFieldType highValue = inPortal.Get(low_high[1]);

    // Interpolate per-vected because some vec-like objects do not allow intermediate variables
    using VTraits = viskores::VecTraits<OutFieldType>;
    VISKORES_ASSERT(VTraits::GetNumberOfComponents(lowValue) ==
                    VTraits::GetNumberOfComponents(result));
    VISKORES_ASSERT(VTraits::GetNumberOfComponents(highValue) ==
                    VTraits::GetNumberOfComponents(result));
    for (viskores::IdComponent cIndex = 0; cIndex < VTraits::GetNumberOfComponents(result);
         ++cIndex)
    {
      VTraits::SetComponent(result,
                            cIndex,
                            viskores::Lerp(VTraits::GetComponent(lowValue, cIndex),
                                           VTraits::GetComponent(highValue, cIndex),
                                           weight));
    }
  }
};
}
}
}


#endif
