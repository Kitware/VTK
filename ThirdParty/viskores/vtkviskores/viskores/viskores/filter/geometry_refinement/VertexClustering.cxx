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
#include <viskores/filter/geometry_refinement/VertexClustering.h>
#include <viskores/filter/geometry_refinement/worklet/VertexClustering.h>

namespace
{
VISKORES_CONT bool DoMapField(viskores::cont::DataSet& result,
                              const viskores::cont::Field& field,
                              const viskores::worklet::VertexClustering& worklet)
{
  if (field.IsPointField())
  {
    return viskores::filter::MapFieldPermutation(field, worklet.GetPointIdMap(), result);
  }
  else if (field.IsCellField())
  {
    return viskores::filter::MapFieldPermutation(field, worklet.GetCellIdMap(), result);
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
VISKORES_CONT viskores::cont::DataSet VertexClustering::DoExecute(
  const viskores::cont::DataSet& input)
{
  //need to compute bounds first
  viskores::Bounds bounds = input.GetCoordinateSystem().GetBounds();

  auto inCellSet =
    input.GetCellSet().ResetCellSetList<VISKORES_DEFAULT_CELL_SET_LIST_UNSTRUCTURED>();
  viskores::cont::UnknownCellSet outCellSet;
  viskores::cont::UnknownArrayHandle outCoords;
  viskores::worklet::VertexClustering worklet;
  worklet.Run(inCellSet,
              input.GetCoordinateSystem(),
              bounds,
              this->GetNumberOfDivisions(),
              outCellSet,
              outCoords);

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  return this->CreateResultCoordinateSystem(
    input, outCellSet, input.GetCoordinateSystem().GetName(), outCoords, mapper);
}
} // namespace geometry_refinement
} // namespace filter
} // namespace viskores
