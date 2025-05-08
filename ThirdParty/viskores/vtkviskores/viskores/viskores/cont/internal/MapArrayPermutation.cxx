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

#include <viskores/cont/internal/CastInvalidValue.h>
#include <viskores/cont/internal/MapArrayPermutation.h>

#include <viskores/cont/ErrorBadType.h>

#include <viskores/worklet/WorkletMapField.h>


namespace
{

template <typename T>
struct MapPermutationWorklet : viskores::worklet::WorkletMapField
{
  T InvalidValue;

  explicit MapPermutationWorklet(T invalidValue)
    : InvalidValue(invalidValue)
  {
  }

  using ControlSignature = void(FieldIn permutationIndex, WholeArrayIn input, FieldOut output);

  template <typename InputPortalType, typename OutputType>
  VISKORES_EXEC void operator()(viskores::Id permutationIndex,
                                InputPortalType inputPortal,
                                OutputType& output) const
  {
    if ((permutationIndex >= 0) && (permutationIndex < inputPortal.GetNumberOfValues()))
    {
      output = inputPortal.Get(permutationIndex);
    }
    else
    {
      output = this->InvalidValue;
    }
  }
};

struct DoMapFieldPermutation
{
  template <typename InputArrayType, typename PermutationArrayType>
  void operator()(const InputArrayType& input,
                  const PermutationArrayType& permutation,
                  viskores::cont::UnknownArrayHandle& output,
                  viskores::Float64 invalidValue) const
  {
    using BaseComponentType = typename InputArrayType::ValueType::ComponentType;

    MapPermutationWorklet<BaseComponentType> worklet(
      viskores::cont::internal::CastInvalidValue<BaseComponentType>(invalidValue));
    viskores::cont::Invoker{}(
      worklet,
      permutation,
      input,
      output.ExtractArrayFromComponents<BaseComponentType>(viskores::CopyFlag::Off));
  }
};

} // anonymous namespace

namespace viskores
{
namespace cont
{
namespace internal
{

viskores::cont::UnknownArrayHandle MapArrayPermutation(
  const viskores::cont::UnknownArrayHandle& inputArray,
  const viskores::cont::UnknownArrayHandle& permutation,
  viskores::Float64 invalidValue)
{
  if (!permutation.IsBaseComponentType<viskores::Id>())
  {
    throw viskores::cont::ErrorBadType("Permutation array input to MapArrayPermutation must have "
                                       "values of viskores::Id. Reported type is " +
                                       permutation.GetBaseComponentTypeName());
  }
  viskores::cont::UnknownArrayHandle outputArray = inputArray.NewInstanceBasic();
  outputArray.Allocate(permutation.GetNumberOfValues());
  inputArray.CastAndCallWithExtractedArray(DoMapFieldPermutation{},
                                           permutation.ExtractComponent<viskores::Id>(0),
                                           outputArray,
                                           invalidValue);
  return outputArray;
}

}
}
} // namespace viskores::cont::internal
