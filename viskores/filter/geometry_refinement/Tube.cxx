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

#include <viskores/filter/MapFieldPermutation.h>
#include <viskores/filter/geometry_refinement/Tube.h>
#include <viskores/filter/geometry_refinement/worklet/Tube.h>

namespace
{
VISKORES_CONT bool DoMapField(viskores::cont::DataSet& result,
                              const viskores::cont::Field& field,
                              const viskores::worklet::Tube& worklet)
{
  if (field.IsPointField())
  {
    return viskores::filter::MapFieldPermutation(
      field, worklet.GetOutputPointSourceIndex(), result);
  }
  else if (field.IsCellField())
  {
    return viskores::filter::MapFieldPermutation(field, worklet.GetOutputCellSourceIndex(), result);
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

namespace viskores
{
namespace filter
{
namespace geometry_refinement
{
VISKORES_CONT viskores::cont::DataSet Tube::DoExecute(const viskores::cont::DataSet& input)
{
  viskores::worklet::Tube worklet;

  worklet.SetCapping(this->Capping);
  worklet.SetNumberOfSides(this->NumberOfSides);
  worklet.SetRadius(this->Radius);

  const auto& originalPoints = input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());
  viskores::cont::ArrayHandle<viskores::Vec3f> newPoints;
  viskores::cont::CellSetSingleType<> newCells;
  worklet.Run(originalPoints.GetDataAsMultiplexer(), input.GetCellSet(), newPoints, newCells);

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  // create the output dataset (without a CoordinateSystem).
  viskores::cont::DataSet output = this->CreateResult(input, newCells, mapper);

  output.AddCoordinateSystem(viskores::cont::CoordinateSystem(originalPoints.GetName(), newPoints));
  return output;
}
} // namespace geometry_refinement
} // namespace filter
} // namespace viskores::filter
