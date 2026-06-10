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

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayIsMonotonic.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/viskores_cont_export.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace cont
{

namespace
{
struct MonotonicIncreasing : public viskores::worklet::WorkletMapField
{
  MonotonicIncreasing() = default;

  using ControlSignature = void(WholeArrayIn, FieldOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename ArrayType>
  VISKORES_EXEC void operator()(const viskores::Id& idx, const ArrayType& input, bool& result) const
  {
    if (idx == 0)
      result = true;
    else
      result = input.Get(idx) >= input.Get(idx - 1);
  }
};

struct MonotonicDecreasing : public viskores::worklet::WorkletMapField
{
  MonotonicDecreasing() = default;

  using ControlSignature = void(WholeArrayIn, FieldOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename ArrayType>
  VISKORES_EXEC void operator()(const viskores::Id& idx, const ArrayType& input, bool& result) const
  {
    if (idx == 0)
      result = true;
    else
      result = input.Get(idx) <= input.Get(idx - 1);
  }
};
} //anonymous namespace


VISKORES_CONT_EXPORT
bool ArrayIsMonotonicIncreasing(const viskores::cont::UnknownArrayHandle& input)
{
  if (input.GetNumberOfComponentsFlat() != 1)
  {
    throw viskores::cont::ErrorBadType(
      "ArrayIsMonotonicIncreasing only supported for scalar arrays.");
  }

  viskores::Id numValues = input.GetNumberOfValues();
  if (numValues < 2)
    return true;

  viskores::cont::ArrayHandle<bool> result;

  auto resolveType = [&](auto recombineVec)
  {
    VISKORES_ASSERT(recombineVec.GetNumberOfComponents() == 1);
    auto componentArray = recombineVec.GetComponentArray(0);
    viskores::cont::Invoker invoke;

    invoke(MonotonicIncreasing{}, componentArray, result);
  };
  input.CastAndCallWithExtractedArray(resolveType);

  return viskores::cont::Algorithm::Reduce(result, true, viskores::LogicalAnd());
}

VISKORES_CONT_EXPORT
bool ArrayIsMonotonicDecreasing(const viskores::cont::UnknownArrayHandle& input)
{
  if (input.GetNumberOfComponentsFlat() != 1)
  {
    throw viskores::cont::ErrorBadType(
      "ArrayIsMonotonicDecreasing only supported for scalar arrays.");
  }
  viskores::Id numValues = input.GetNumberOfValues();
  if (numValues < 2)
    return true;

  viskores::cont::ArrayHandle<bool> result;

  auto resolveType = [&](auto recombineVec)
  {
    VISKORES_ASSERT(recombineVec.GetNumberOfComponents() == 1);
    auto componentArray = recombineVec.GetComponentArray(0);
    viskores::cont::Invoker invoke;

    invoke(MonotonicDecreasing{}, componentArray, result);
  };
  input.CastAndCallWithExtractedArray(resolveType);

  return viskores::cont::Algorithm::Reduce(result, true, viskores::LogicalAnd());
}


}
} // namespace viskores::cont
