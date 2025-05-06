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

#include <viskores/cont/UncertainCellSet.h>
#include <viskores/filter/MapFieldPermutation.h>
#include <viskores/filter/geometry_refinement/Shrink.h>
#include <viskores/filter/geometry_refinement/worklet/Shrink.h>

namespace
{
VISKORES_CONT bool DoMapField(viskores::cont::DataSet& result,
                              const viskores::cont::Field& inputField,
                              const viskores::cont::ArrayHandle<viskores::Id>& outputToInputCellMap)
{
  if (inputField.IsCellField() || inputField.IsWholeDataSetField())
  {
    result.AddField(inputField); // pass through
    return true;
  }
  else if (inputField.IsPointField())
  {
    return viskores::filter::MapFieldPermutation(inputField, outputToInputCellMap, result);
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
VISKORES_CONT viskores::cont::DataSet Shrink::DoExecute(const viskores::cont::DataSet& input)
{
  const viskores::cont::UnknownCellSet& inCellSet = input.GetCellSet();
  const auto& oldCoords = input.GetCoordinateSystem().GetDataAsMultiplexer();

  viskores::cont::ArrayHandle<viskores::Vec3f> newCoords;
  viskores::cont::ArrayHandle<viskores::Id> oldPointsMapping;
  viskores::cont::CellSetExplicit<> newCellset;
  viskores::worklet::Shrink worklet;

  worklet.Run(inCellSet, this->ShrinkFactor, oldCoords, newCoords, oldPointsMapping, newCellset);

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, oldPointsMapping); };

  viskores::cont::CoordinateSystem activeCoordSystem = input.GetCoordinateSystem();
  activeCoordSystem = viskores::cont::CoordinateSystem(activeCoordSystem.GetName(), newCoords);

  return this->CreateResultCoordinateSystem(input, newCellset, activeCoordSystem, mapper);
}
} // namespace geometry_refinement
} // namespace filter
} // namespace viskores
