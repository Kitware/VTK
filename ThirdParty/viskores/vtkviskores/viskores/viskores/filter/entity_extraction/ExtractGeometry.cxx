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
#include <viskores/filter/entity_extraction/ExtractGeometry.h>
#include <viskores/filter/entity_extraction/worklet/ExtractGeometry.h>

namespace
{
bool DoMapField(viskores::cont::DataSet& result,
                const viskores::cont::Field& field,
                const viskores::worklet::ExtractGeometry& worklet)
{
  if (field.IsPointField())
  {
    result.AddField(field);
    return true;
  }
  else if (field.IsCellField())
  {
    viskores::cont::ArrayHandle<viskores::Id> permutation = worklet.GetValidCellIds();
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

namespace viskores
{
namespace filter
{
namespace entity_extraction
{
//-----------------------------------------------------------------------------
viskores::cont::DataSet ExtractGeometry::DoExecute(const viskores::cont::DataSet& input)
{
  // extract the input cell set and coordinates
  const viskores::cont::UnknownCellSet& cells = input.GetCellSet();
  const viskores::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  viskores::worklet::ExtractGeometry worklet;
  viskores::cont::UnknownCellSet outCells;

  cells.CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST>(
    [&](const auto& concrete)
    {
      outCells = worklet.Run(concrete,
                             coords,
                             this->Function,
                             this->ExtractInside,
                             this->ExtractBoundaryCells,
                             this->ExtractOnlyBoundaryCells);
    });

  // create the output dataset
  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  return this->CreateResult(input, outCells, mapper);
}

} // namespace entity_extraction
} // namespace filter
} // namespace viskores
