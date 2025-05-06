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
#ifndef viskores_worklet_MaskNone_h
#define viskores_worklet_MaskNone_h

#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/worklet/internal/MaskBase.h>

namespace viskores
{
namespace worklet
{

/// \brief Default mask object that does not suppress anything.
///
/// \c MaskNone is a worklet mask object that does not suppress any items in the output
/// domain. This is the default mask object so that the worklet is run for every possible
/// output element.
///
struct MaskNone : public internal::MaskBase
{
  template <typename RangeType>
  VISKORES_CONT RangeType GetThreadRange(RangeType outputRange) const
  {
    return outputRange;
  }

  using ThreadToOutputMapType = viskores::cont::ArrayHandleIndex;

  VISKORES_CONT ThreadToOutputMapType GetThreadToOutputMap(viskores::Id outputRange) const
  {
    return viskores::cont::ArrayHandleIndex(outputRange);
  }

  VISKORES_CONT ThreadToOutputMapType GetThreadToOutputMap(const viskores::Id3& outputRange) const
  {
    return this->GetThreadToOutputMap(outputRange[0] * outputRange[1] * outputRange[2]);
  }
};
}
} // namespace viskores::worklet

#endif //viskores_worklet_MaskNone_h
