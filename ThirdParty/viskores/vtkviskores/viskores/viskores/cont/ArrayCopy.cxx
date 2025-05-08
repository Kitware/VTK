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

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayCopyDevice.h>

namespace viskores
{
namespace cont
{
namespace detail
{

void ArrayCopyConcreteSrc<viskores::cont::StorageTagCounting>::CopyCountingFloat(
  viskores::FloatDefault start,
  viskores::FloatDefault step,
  viskores::Id size,
  const viskores::cont::UnknownArrayHandle& result) const
{
  if (result.IsBaseComponentType<viskores::FloatDefault>())
  {
    auto outArray = result.ExtractComponent<viskores::FloatDefault>(0);
    viskores::cont::ArrayCopyDevice(viskores::cont::make_ArrayHandleCounting(start, step, size),
                                    outArray);
  }
  else
  {
    viskores::cont::ArrayHandle<viskores::FloatDefault> outArray;
    outArray.Allocate(size);
    CopyCountingFloat(start, step, size, outArray);
    result.DeepCopyFrom(outArray);
  }
}

viskores::cont::ArrayHandle<Id>
ArrayCopyConcreteSrc<viskores::cont::StorageTagCounting>::CopyCountingId(
  const viskores::cont::ArrayHandleCounting<viskores::Id>& source) const
{
  viskores::cont::ArrayHandle<Id> destination;
  viskores::cont::ArrayCopyDevice(source, destination);
  return destination;
}

}
}
} // namespace viskores::cont::detail
