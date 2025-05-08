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
/// The \c Scatter classes are responsible for defining how much output is
/// generated based on some sized input. \c ScatterPermutation is similar to
/// \c ScatterCounting but can have lesser memory usage for some cases.
/// The constructor takes an array of ids, where each entry maps the
/// corresponding output to an input. The ids can be in any order and there
/// can be duplicates. Note that even with duplicates the VistIndex is always 0.
///
template <typename PermutationStorage = VISKORES_DEFAULT_STORAGE_TAG>
class ScatterPermutation : public internal::ScatterBase
{
private:
  using PermutationArrayHandle = viskores::cont::ArrayHandle<viskores::Id, PermutationStorage>;

public:
  using OutputToInputMapType = PermutationArrayHandle;
  using VisitArrayType = viskores::cont::ArrayHandleConstant<viskores::IdComponent>;

  ScatterPermutation(const PermutationArrayHandle& permutation)
    : Permutation(permutation)
  {
  }

  VISKORES_CONT
  template <typename RangeType>
  viskores::Id GetOutputRange(RangeType) const
  {
    return this->Permutation.GetNumberOfValues();
  }

  template <typename RangeType>
  VISKORES_CONT OutputToInputMapType GetOutputToInputMap(RangeType) const
  {
    return this->Permutation;
  }

  VISKORES_CONT OutputToInputMapType GetOutputToInputMap() const { return this->Permutation; }

  VISKORES_CONT
  VisitArrayType GetVisitArray(viskores::Id inputRange) const
  {
    return VisitArrayType(0, this->GetOutputRange(inputRange));
  }

  VISKORES_CONT
  VisitArrayType GetVisitArray(viskores::Id3 inputRange) const
  {
    return this->GetVisitArray(inputRange[0] * inputRange[1] * inputRange[2]);
  }

private:
  PermutationArrayHandle Permutation;
};
}
} // viskores::worklet

#endif // viskores_worklet_ScatterPermutation_h
