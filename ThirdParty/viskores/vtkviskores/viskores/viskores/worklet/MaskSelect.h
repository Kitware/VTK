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
/// `MaskSelect` is a worklet mask object that is used to select elements in the output of a
/// worklet to suppress the invocation. That is, the worklet will only be invoked for elements in
/// the output that are not masked out by the given array.
///
/// `MaskSelect` is initialized with a mask array. This array should contain a 0 for any entry
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
  /// @brief The type of array handle used to map thread indices to output indices.
  ///
  /// For the case of `MaskSelect`, this is a basic array handle.
  using ThreadToOutputMapType = viskores::cont::ArrayHandle<viskores::Id>;

  /// Construct a `MaskSelect` object using an array that masks an output
  /// value with `0` and enables an output value with `1`.
  MaskSelect(const viskores::cont::UnknownArrayHandle& maskArray,
             viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny())
  {
    this->ThreadToOutputMap = this->Build(maskArray, device);
  }

  /// @brief Provides the number of threads for a given output domain size.
  /// @param outputRange The size of the full output domain (including masked
  ///   entries), which must be the same size as the select array provided in
  ///   the constructor.
  /// @return The total number of threads.
  template <typename RangeType>
  viskores::Id GetThreadRange(RangeType outputRange) const
  {
    (void)outputRange;
    return this->ThreadToOutputMap.GetNumberOfValues();
  }

  /// @brief Provides the array that maps thread indices to output indices.
  /// @param outputRange The size of the full output domain (including masked
  ///   entries), which must be the same size as the select array provided in
  ///   the constructor.
  /// @return A basic array of indices that identifies which output each thread
  ///   writes to.
  template <typename RangeType>
  ThreadToOutputMapType GetThreadToOutputMap(RangeType outputRange) const
  {
    (void)outputRange;
    return this->ThreadToOutputMap;
  }

protected:
  // Allows to differentiate between MaskSelect and ThreadToOutputMap arrays in constructors.
  struct ThreadToOutputMapWrapper
  {
    ThreadToOutputMapType ThreadToOutputMap;
  };

  MaskSelect(const ThreadToOutputMapWrapper& threadToOutputMap)
    : ThreadToOutputMap(threadToOutputMap.ThreadToOutputMap)
  {
  }

private:
  ThreadToOutputMapType ThreadToOutputMap;

  VISKORES_CONT ThreadToOutputMapType Build(const viskores::cont::UnknownArrayHandle& maskArray,
                                            viskores::cont::DeviceAdapterId device);
};
}
} // namespace viskores::worklet

#endif //viskores_worklet_MaskSelect_h
