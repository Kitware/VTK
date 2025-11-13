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
#ifndef viskores_worklet_ScatterPermutation_h
#define viskores_worklet_ScatterPermutation_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/worklet/internal/ScatterBase.h>

namespace viskores
{
namespace worklet
{

/// \brief A scatter that maps input to output based on a permutation array.
///
/// The `Scatter` classes are responsible for defining how much output is
/// generated based on some sized input. `ScatterPermutation` is similar to
/// `ScatterCounting` but can have lesser memory usage for some cases.
/// The constructor takes an array of ids, where each entry maps the
/// corresponding output to an input. The ids can be in any order and there
/// can be duplicates. Note that even with duplicates the `VistIndex` is always 0.
///
template <typename PermutationStorage = VISKORES_DEFAULT_STORAGE_TAG>
struct ScatterPermutation : public internal::ScatterBase
{
  /// @brief The type of array handle used to map output indices to input indices.
  ///
  /// For the case of `ScatterPermutation`, this is an array handle. It is a basic
  /// array handle by default, but can be modified by the template parameter of
  /// the `ScatterPermutation` class.
  using OutputToInputMapType = viskores::cont::ArrayHandle<viskores::Id, PermutationStorage>;

  /// @brief The type of array handle used for the visit index for each output.
  ///
  /// For the case of `ScatterPermutation`, this is a `viskores::cont::ArrayHandleConstant`
  /// where are values are 0. All outputs are assumed to point to a single input.
  /// This is not enforced, but if two outputs point to the same input, they cannot
  /// be differentiated by the visit index.
  using VisitArrayType = viskores::cont::ArrayHandleConstant<viskores::IdComponent>;

  /// Constructs a `ScatterPermutation` given an array of indices that point from output
  /// to input. The provided array handle is sized to the number of output values and maps
  /// output indices to input indices. For example, if index $i$ of the permutation array
  /// contains $j$, then the worklet invocation for output $i$ will get the $j^{th}$
  /// input values. The reordering does not have to be 1 to 1. Any input not referenced by
  /// the permutation array will be dropped, and any input referenced by the permutation
  /// array multiple times will be duplicated. However, unlike `ScatterCounting`, `VisitIndex`
  /// is always 0 even if an input value happens to be duplicated.
  ScatterPermutation(const OutputToInputMapType& permutation)
    : Permutation(permutation)
  {
  }

  /// @brief Provides the number of output values for a given input domain size.
  /// @param inputRange The size of the input domain, which must be the same size as
  ///   the permutation array provided in the constructor.
  /// @return The total number of output values.
  VISKORES_CONT
  template <typename RangeType>
  viskores::Id GetOutputRange(RangeType inputRange) const
  {
    (void)inputRange;
    return this->Permutation.GetNumberOfValues();
  }

  /// @brief Provides the array that maps output indices to input indices.
  /// @param inputRange The size of the input domain, which must be the same size as
  ///   the permutation array provided in the constructor.
  /// @return The provided permutation array, which is the same as the output to
  ///   input map.
  template <typename RangeType>
  VISKORES_CONT OutputToInputMapType GetOutputToInputMap(RangeType inputRange) const
  {
    (void)inputRange;
    return this->Permutation;
  }

  /// @brief Provides the array that maps output indices to input indices.
  /// @return The provided permutation array, which is the same as the output to
  ///   input map.
  VISKORES_CONT OutputToInputMapType GetOutputToInputMap() const { return this->Permutation; }

  /// @brief Provides the array that gives the visit index for each output.
  /// @param inputRange The size of the input domain.
  /// @return A `viskores::cont::ArrayHandleConstant` of the same size as the
  /// `inputRange` with value of 0.
  VISKORES_CONT
  VisitArrayType GetVisitArray(viskores::Id inputRange) const
  {
    return VisitArrayType(0, this->GetOutputRange(inputRange));
  }

  /// @copydoc GetVisitArray
  VISKORES_CONT
  VisitArrayType GetVisitArray(viskores::Id3 inputRange) const
  {
    return this->GetVisitArray(inputRange[0] * inputRange[1] * inputRange[2]);
  }

private:
  OutputToInputMapType Permutation;
};
}
} // viskores::worklet

#endif // viskores_worklet_ScatterPermutation_h
