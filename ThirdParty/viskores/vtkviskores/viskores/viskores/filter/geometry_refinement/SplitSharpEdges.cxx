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

#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/UnknownCellSet.h>
#include <viskores/filter/MapFieldPermutation.h>
#include <viskores/filter/geometry_refinement/SplitSharpEdges.h>
#include <viskores/filter/geometry_refinement/worklet/SplitSharpEdges.h>

namespace viskores
{
namespace filter
{
namespace
{
VISKORES_CONT bool DoMapField(viskores::cont::DataSet& result,
                              const viskores::cont::Field& field,
                              const viskores::worklet::SplitSharpEdges& worklet)
{
  if (field.IsPointField())
  {
    return viskores::filter::MapFieldPermutation(field, worklet.GetNewPointsIdArray(), result);
  }
  else if (field.IsCellField() || field.IsWholeDataSetField())
  {
    result.AddField(field); // pass through
    return true;
  }
  else
  {
    return false;
  }
}
} // anonymous namespace

namespace geometry_refinement
{
//-----------------------------------------------------------------------------
VISKORES_CONT viskores::cont::DataSet SplitSharpEdges::DoExecute(
  const viskores::cont::DataSet& input)
{
  const auto& field = this->GetFieldFromDataSet(input);
  const viskores::cont::UnknownCellSet& inCellSet = input.GetCellSet();
  const auto& oldCoords = input.GetCoordinateSystem().GetDataAsMultiplexer();

  viskores::cont::ArrayHandle<viskores::Vec3f> newCoords;
  viskores::cont::CellSetExplicit<> newCellset;
  viskores::worklet::SplitSharpEdges worklet;
  this->CastAndCallVecField<3>(
    field,
    [&](const auto& concrete)
    { worklet.Run(inCellSet, this->FeatureAngle, concrete, oldCoords, newCoords, newCellset); });

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  auto output = this->CreateResult(input, newCellset, mapper);
  output.AddCoordinateSystem(
    viskores::cont::CoordinateSystem(input.GetCoordinateSystem().GetName(), newCoords));
  return output;
}

} // namespace geometry_refinement
} // namespace filter
} // namespace viskores
