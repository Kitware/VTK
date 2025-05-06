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

#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/UnknownCellSet.h>
#include <viskores/filter/MapFieldPermutation.h>
#include <viskores/filter/contour/ClipWithImplicitFunction.h>
#include <viskores/filter/contour/worklet/Clip.h>

namespace viskores
{
namespace filter
{
namespace contour
{
namespace
{

bool DoMapField(viskores::cont::DataSet& result,
                const viskores::cont::Field& field,
                viskores::worklet::Clip& worklet)
{
  if (field.IsPointField())
  {
    viskores::cont::UnknownArrayHandle inputArray = field.GetData();
    viskores::cont::UnknownArrayHandle outputArray = inputArray.NewInstanceBasic();

    auto resolve = [&](const auto& concrete)
    {
      // use std::decay to remove const ref from the decltype of concrete.
      using BaseT = typename std::decay_t<decltype(concrete)>::ValueType::ComponentType;
      auto concreteOut = outputArray.ExtractArrayFromComponents<BaseT>();
      worklet.ProcessPointField(concrete, concreteOut);
    };

    inputArray.CastAndCallWithExtractedArray(resolve);
    result.AddPointField(field.GetName(), outputArray);
    return true;
  }
  else if (field.IsCellField())
  {
    // Use the precompiled field permutation function.
    viskores::cont::ArrayHandle<viskores::Id> permutation = worklet.GetCellMapOutputToInput();
    return viskores::filter::MapFieldPermutation(field, permutation, result);
  }
  else if (field.IsWholeDataSetField())
  {
    result.AddField(field);
    return true;
  }
  else
  {
    return false;
  }
}
} // anonymous namespace

//-----------------------------------------------------------------------------
viskores::cont::DataSet ClipWithImplicitFunction::DoExecute(const viskores::cont::DataSet& input)
{
  const viskores::cont::UnknownCellSet& inputCellSet = input.GetCellSet();
  const viskores::cont::CoordinateSystem& inputCoords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  viskores::worklet::Clip worklet;

  viskores::cont::CellSetExplicit<> outputCellSet = this->Invert
    ? worklet.Run<true>(inputCellSet, this->Function, this->Offset, inputCoords)
    : worklet.Run<false>(inputCellSet, this->Function, this->Offset, inputCoords);

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  return this->CreateResult(input, outputCellSet, mapper);
}
} // namespace contour
} // namespace filter
} // namespace viskores
