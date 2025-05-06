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
#include <viskores/filter/entity_extraction/Mask.h>
#include <viskores/filter/entity_extraction/worklet/Mask.h>

namespace
{
VISKORES_CONT bool DoMapField(viskores::cont::DataSet& result,
                              const viskores::cont::Field& field,
                              const viskores::worklet::Mask& worklet)
{
  if (field.IsPointField() || field.IsWholeDataSetField())
  {
    result.AddField(field); // pass through
    return true;
  }
  else if (field.IsCellField())
  {
    return viskores::filter::MapFieldPermutation(field, worklet.GetValidCellIds(), result);
  }
  else
  {
    return false;
  }
}
} // end anon namespace

namespace viskores
{
namespace filter
{
namespace entity_extraction
{
//-----------------------------------------------------------------------------
VISKORES_CONT viskores::cont::DataSet Mask::DoExecute(const viskores::cont::DataSet& input)
{
  const viskores::cont::UnknownCellSet& cells = input.GetCellSet();
  viskores::cont::UnknownCellSet cellOut;
  viskores::worklet::Mask worklet;

  cells.CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST>(
    [&](const auto& concrete) { cellOut = worklet.Run(concrete, this->Stride); });

  // create the output dataset
  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  return this->CreateResult(input, cellOut, mapper);
}
} // namespace entity_extraction
} // namespace filter
} // namespace viskores
