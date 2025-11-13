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
/// The `Scatter` classes are responsible for defining how much output is
/// generated based on some sized input. `ScatterIdentity` establishes a 1 to
/// 1 mapping from input to output (and vice versa). That is, every input
/// element generates one output element associated with it. This is the
/// default for basic maps.
///
struct ScatterIdentity : internal::ScatterBase
{
  /// @brief The type of array handle used to map output indices to input indices.
  ///
  /// For the case of `ScatterIdentity`, this is a
  /// `viskores::cont::ArrayHandleIndex` to do a direct 1-to-1 maping.
  using OutputToInputMapType = viskores::cont::ArrayHandleIndex;

  /// @brief Provides the array that maps output indices to input indices.
  /// @param inputRange The size of the input domain.
  /// @return A `viskores::cont::ArrayHandleIndex` of the same size as the `inputRange`.
  VISKORES_CONT
  OutputToInputMapType GetOutputToInputMap(viskores::Id inputRange) const
  {
    return OutputToInputMapType(inputRange);
  }

  /// @brief Provides the array that maps output indices to input indices.
  /// @param inputRange The size of the input domain in 3 dimensions.
  /// @return A `viskores::cont::ArrayHandleIndex` of the same size as the `inputRange`.
  VISKORES_CONT
  OutputToInputMapType GetOutputToInputMap(viskores::Id3 inputRange) const
  {
    return this->GetOutputToInputMap(inputRange[0] * inputRange[1] * inputRange[2]);
  }

  /// @brief The type of array handle used for the visit index for each output.
  ///

  /// For the case of `ScatterIdentity`, this is a
  /// `viskores::cont::ArrayHandleConstant` to do a direct 1-to-1 maping (so
  /// every visit index is 0).

  using VisitArrayType = viskores::cont::ArrayHandleConstant<viskores::IdComponent>;

  /// @brief Provides the array that gives the visit index for each output.
  /// @param inputRange The size of the input domain.
  /// @return A `viskores::cont::ArrayHandleConstant` of the same size as the
  /// `inputRange` with value of 0.
  VISKORES_CONT
  VisitArrayType GetVisitArray(viskores::Id inputRange) const
  {
    return VisitArrayType(0, inputRange);
  }
  /// @copydoc GetVisitArray
  VISKORES_CONT
  VisitArrayType GetVisitArray(viskores::Id3 inputRange) const
  {
    return this->GetVisitArray(inputRange[0] * inputRange[1] * inputRange[2]);
  }

  /// @brief Provides the number of output values for a given input domain size.
  /// @param inputRange The size of the input domain.
  /// @return The same value as `inputRange`. For a `ScatterIdentity`, the number of outputs
  ///   are the same as the number of inputs.
  template <typename RangeType>
  VISKORES_CONT RangeType GetOutputRange(RangeType inputRange) const
  {
    return inputRange;
  }
};
}
} // namespace viskores::worklet

#endif //viskores_worklet_ScatterIdentity_h
