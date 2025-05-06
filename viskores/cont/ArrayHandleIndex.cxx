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

#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/internal/ArrayRangeComputeUtils.h>

namespace viskores
{
namespace cont
{
namespace internal
{

VISKORES_CONT viskores::cont::ArrayHandle<viskores::Range>
ArrayRangeComputeImpl<viskores::cont::StorageTagIndex>::operator()(
  const viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagIndex>& input,
  const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
  bool viskoresNotUsed(computeFiniteRange), // assume array produces only finite values
  viskores::cont::DeviceAdapterId device) const
{
  viskores::Range range{};

  if (input.GetNumberOfValues() > 0)
  {
    viskores::Id2 firstAndLast{ 0, input.GetNumberOfValues() - 1 };
    if (maskArray.GetNumberOfValues() > 0)
    {
      firstAndLast = viskores::cont::internal::GetFirstAndLastUnmaskedIndices(maskArray, device);
    }
    if (firstAndLast[0] < firstAndLast[1])
    {
      range = viskores::Range(firstAndLast[0], firstAndLast[1]);
    }
  }

  viskores::cont::ArrayHandle<viskores::Range> result;
  result.Allocate(1);
  result.WritePortal().Set(0, range);
  return result;
}

}
}
} // viskores::cont::internal
