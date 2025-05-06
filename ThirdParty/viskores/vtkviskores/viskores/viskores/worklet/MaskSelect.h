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
#ifndef viskores_worklet_MaskSelect_h
#define viskores_worklet_MaskSelect_h

#include <viskores/worklet/internal/MaskBase.h>
#include <viskores/worklet/viskores_worklet_export.h>

#include <viskores/cont/UnknownArrayHandle.h>

namespace viskores
{
namespace worklet
{

/// \brief Mask using arrays to select specific elements to suppress.
///
/// \c MaskSelect is a worklet mask object that is used to select elements in the output of a
/// worklet to suppress the invocation. That is, the worklet will only be invoked for elements in
/// the output that are not masked out by the given array.
///
/// \c MaskSelect is initialized with a mask array. This array should contain a 0 for any entry
/// that should be masked and a 1 for any output that should be generated. It is an error to have
/// any value that is not a 0 or 1. This method is slower than specifying an index array.
///
class VISKORES_WORKLET_EXPORT MaskSelect : public internal::MaskBase
{
  using MaskTypes = viskores::List<viskores::Int32,
                                   viskores::Int64,
                                   viskores::UInt32,
                                   viskores::UInt64,
                                   viskores::Int8,
                                   viskores::UInt8,
                                   char>;

public:
  using ThreadToOutputMapType = viskores::cont::ArrayHandle<viskores::Id>;

  MaskSelect(const viskores::cont::UnknownArrayHandle& maskArray,
             viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny())
  {
    this->ThreadToOutputMap = this->Build(maskArray, device);
  }

  template <typename RangeType>
  viskores::Id GetThreadRange(RangeType viskoresNotUsed(outputRange)) const
  {
    return this->ThreadToOutputMap.GetNumberOfValues();
  }

  template <typename RangeType>
  ThreadToOutputMapType GetThreadToOutputMap(RangeType viskoresNotUsed(outputRange)) const
  {
    return this->ThreadToOutputMap;
  }

private:
  ThreadToOutputMapType ThreadToOutputMap;

  VISKORES_CONT ThreadToOutputMapType Build(const viskores::cont::UnknownArrayHandle& maskArray,
                                            viskores::cont::DeviceAdapterId device);
};
}
} // namespace viskores::worklet

#endif //viskores_worklet_MaskSelect_h
