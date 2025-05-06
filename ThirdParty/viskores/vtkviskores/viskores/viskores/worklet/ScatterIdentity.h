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
#ifndef viskores_worklet_ScatterIdentity_h
#define viskores_worklet_ScatterIdentity_h

#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/worklet/internal/ScatterBase.h>

namespace viskores
{
namespace worklet
{

/// \brief A scatter that maps input directly to output.
///
/// The \c Scatter classes are responsible for defining how much output is
/// generated based on some sized input. \c ScatterIdentity establishes a 1 to
/// 1 mapping from input to output (and vice versa). That is, every input
/// element generates one output element associated with it. This is the
/// default for basic maps.
///
struct ScatterIdentity : internal::ScatterBase
{
  using OutputToInputMapType = viskores::cont::ArrayHandleIndex;
  VISKORES_CONT
  OutputToInputMapType GetOutputToInputMap(viskores::Id inputRange) const
  {
    return OutputToInputMapType(inputRange);
  }
  VISKORES_CONT
  OutputToInputMapType GetOutputToInputMap(viskores::Id3 inputRange) const
  {
    return this->GetOutputToInputMap(inputRange[0] * inputRange[1] * inputRange[2]);
  }

  using VisitArrayType = viskores::cont::ArrayHandleConstant<viskores::IdComponent>;
  VISKORES_CONT
  VisitArrayType GetVisitArray(viskores::Id inputRange) const
  {
    return VisitArrayType(0, inputRange);
  }
  VISKORES_CONT
  VisitArrayType GetVisitArray(viskores::Id3 inputRange) const
  {
    return this->GetVisitArray(inputRange[0] * inputRange[1] * inputRange[2]);
  }

  template <typename RangeType>
  VISKORES_CONT RangeType GetOutputRange(RangeType inputRange) const
  {
    return inputRange;
  }
};
}
} // namespace viskores::worklet

#endif //viskores_worklet_ScatterIdentity_h
