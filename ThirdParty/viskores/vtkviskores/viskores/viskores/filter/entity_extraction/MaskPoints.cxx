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
#include <viskores/filter/clean_grid/CleanGrid.h>
#include <viskores/filter/entity_extraction/MaskPoints.h>
#include <viskores/filter/entity_extraction/worklet/MaskPoints.h>

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
VISKORES_CONT viskores::cont::DataSet MaskPoints::DoExecute(const viskores::cont::DataSet& input)
{
  // extract the input cell set
  const viskores::cont::UnknownCellSet& cells = input.GetCellSet();

  // run the worklet on the cell set and input field
  viskores::cont::CellSetSingleType<> outCellSet;
  viskores::worklet::MaskPoints worklet;

  outCellSet = worklet.Run(cells, this->Stride);

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
