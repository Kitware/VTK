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
/// `MaskNone` is a worklet mask object that does not suppress any items in the output
/// domain. This is the default mask object so that the worklet is run for every possible
/// output element.
///
struct MaskNone : public internal::MaskBase
{
  /// @brief Provides the number of threads for a given output domain size.
  /// @param outputRange The size of the full output domain.
  /// @return The total number of output values, which for `MaskNone` is the
  ///   same as the `outputRange`.
  template <typename RangeType>
  VISKORES_CONT RangeType GetThreadRange(RangeType outputRange) const
  {
    return outputRange;
  }

  /// @brief The type of array handle used to map thread indices to output indices.
  ///
  /// For the case of `MaskNone`, this is an index array.
  using ThreadToOutputMapType = viskores::cont::ArrayHandleIndex;

  /// @brief Provides the array that maps thread indices to output indices.
  /// @param outputRange The size of the full output domain.
  /// @return A basic array of indices that identifies which output each thread
  ///   writes to.
  VISKORES_CONT ThreadToOutputMapType GetThreadToOutputMap(viskores::Id outputRange) const
  {
    return viskores::cont::ArrayHandleIndex(outputRange);
  }

  /// @copydoc ThreadToOutputMapType
  VISKORES_CONT ThreadToOutputMapType GetThreadToOutputMap(const viskores::Id3& outputRange) const
  {
    return this->GetThreadToOutputMap(outputRange[0] * outputRange[1] * outputRange[2]);
  }
};
}
} // namespace viskores::worklet

#endif //viskores_worklet_MaskNone_h
