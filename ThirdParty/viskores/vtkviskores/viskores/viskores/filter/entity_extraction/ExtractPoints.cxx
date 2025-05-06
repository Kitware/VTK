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
#include <viskores/cont/UnknownCellSet.h>
#include <viskores/filter/clean_grid/CleanGrid.h>
#include <viskores/filter/entity_extraction/ExtractPoints.h>
#include <viskores/filter/entity_extraction/worklet/ExtractPoints.h>

namespace
{
bool DoMapField(viskores::cont::DataSet& result, const viskores::cont::Field& field)
{
  // point data is copied as is because it was not collapsed
  if (field.IsPointField())
  {
    result.AddField(field);
    return true;
  }
  else if (field.IsWholeDataSetField())
  {
    result.AddField(field);
    return true;
  }
  else
  {
    // cell data does not apply
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
VISKORES_CONT
viskores::cont::DataSet ExtractPoints::DoExecute(const viskores::cont::DataSet& input)
{
  // extract the input cell set and coordinates
  const viskores::cont::UnknownCellSet& cells = input.GetCellSet();
  const viskores::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  // run the worklet on the cell set
  viskores::cont::CellSetSingleType<> outCellSet;
  viskores::worklet::ExtractPoints worklet;

  // FIXME: is the other overload of .Run ever used?
  outCellSet = worklet.Run(cells, coords.GetData(), this->Function, this->ExtractInside);

  // create the output dataset
  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f); };
  viskores::cont::DataSet output = this->CreateResult(input, outCellSet, mapper);

  // compact the unused points in the output dataset
  if (this->CompactPoints)
  {
    viskores::filter::clean_grid::CleanGrid compactor;
    compactor.SetCompactPointFields(true);
    compactor.SetMergePoints(false);
    return compactor.Execute(output);
  }
  else
  {
    return output;
  }
}

} // namespace entity_extraction
} // namespace filter
} // namespace viskores
