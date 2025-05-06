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
#include <viskores/cont/ArrayHandleIndex.h>

#include <viskores/filter/MapFieldPermutation.h>
#include <viskores/filter/entity_extraction/ExtractStructured.h>
#include <viskores/filter/entity_extraction/worklet/ExtractStructured.h>

namespace
{
VISKORES_CONT void DoMapField(viskores::cont::DataSet& result,
                              const viskores::cont::Field& field,
                              const viskores::cont::ArrayHandle<viskores::Id>& CellFieldMap,
                              const viskores::cont::ArrayHandle<viskores::Id>& PointFieldMap,
                              const viskores::worklet::ExtractStructured& worklet)
{
  if (field.IsPointField())
  {
    viskores::cont::UnknownArrayHandle array = field.GetData();
    using UniformCoordinatesArrayHandle =
      viskores::worklet::ExtractStructured::UniformCoordinatesArrayHandle;
    using RectilinearCoordinatesArrayHandle =
      viskores::worklet::ExtractStructured::RectilinearCoordinatesArrayHandle;
    if (array.CanConvert<UniformCoordinatesArrayHandle>())
    {
      // Special case that is more efficient for uniform coordinate arrays.
      UniformCoordinatesArrayHandle newCoords =
        worklet.MapCoordinatesUniform(array.AsArrayHandle<UniformCoordinatesArrayHandle>());
      result.AddField(viskores::cont::Field(field.GetName(), field.GetAssociation(), newCoords));
    }
    else if (array.CanConvert<RectilinearCoordinatesArrayHandle>())
    {
      // Special case that is more efficient for uniform coordinate arrays.
      RectilinearCoordinatesArrayHandle newCoords =
        worklet.MapCoordinatesRectilinear(array.AsArrayHandle<RectilinearCoordinatesArrayHandle>());
      result.AddField(viskores::cont::Field(field.GetName(), field.GetAssociation(), newCoords));
    }
    else
    {
      viskores::filter::MapFieldPermutation(field, PointFieldMap, result);
    }
  }
  else if (field.IsCellField())
  {
    viskores::filter::MapFieldPermutation(field, CellFieldMap, result);
  }
  else if (field.IsWholeDataSetField())
  {
    result.AddField(field);
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
viskores::cont::DataSet ExtractStructured::DoExecute(const viskores::cont::DataSet& input)
{
  const viskores::cont::UnknownCellSet& cells = input.GetCellSet();

  viskores::worklet::ExtractStructured worklet;
  auto cellset = worklet.Run(cells.ResetCellSetList<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>(),
                             this->VOI,
                             this->SampleRate,
                             this->IncludeBoundary,
                             this->IncludeOffset);

  // Create map arrays for mapping fields. Could potentially save some time to first check to see
  // if these arrays would be used.
  auto CellFieldMap =
    worklet.ProcessCellField(viskores::cont::ArrayHandleIndex(input.GetNumberOfCells()));
  auto PointFieldMap =
    worklet.ProcessPointField(viskores::cont::ArrayHandleIndex(input.GetNumberOfPoints()));

  auto mapper = [&](auto& result, const auto& f)
  { DoMapField(result, f, CellFieldMap, PointFieldMap, worklet); };
  return this->CreateResult(input, cellset, mapper);
}

} // namespace entity_extraction
} // namespace filter
} // namespace viskores
