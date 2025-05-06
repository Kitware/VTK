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

#include <viskores/cont/internal/ArrayRangeComputeUtils.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/ArrayHandleZip.h>

#include <viskores/BinaryOperators.h>

#include <limits>

namespace
{

struct UnmaskedIndicesTransform
{
  VISKORES_EXEC viskores::Id2 operator()(viskores::Pair<viskores::UInt8, viskores::Id> in) const
  {
    if (in.first == 0)
    {
      return { std::numeric_limits<viskores::Id>::max(), std::numeric_limits<viskores::Id>::min() };
    }
    return { in.second };
  }
};

} // namespace

viskores::Id2 viskores::cont::internal::GetFirstAndLastUnmaskedIndices(
  const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
  viskores::cont::DeviceAdapterId device)
{
  viskores::Id2 initialValue = { std::numeric_limits<viskores::Id>::max(),
                                 std::numeric_limits<viskores::Id>::min() };
  auto maskValsAndInds = viskores::cont::make_ArrayHandleZip(
    maskArray, viskores::cont::ArrayHandleIndex(maskArray.GetNumberOfValues()));
  auto unmaskedIndices =
    viskores::cont::make_ArrayHandleTransform(maskValsAndInds, UnmaskedIndicesTransform{});
  return viskores::cont::Algorithm::Reduce(
    device, unmaskedIndices, initialValue, viskores::MinAndMax<viskores::Id>());
}
