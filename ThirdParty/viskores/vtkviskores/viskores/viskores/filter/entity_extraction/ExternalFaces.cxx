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
#include <viskores/filter/clean_grid/CleanGrid.h>
#include <viskores/filter/entity_extraction/ExternalFaces.h>
#include <viskores/filter/entity_extraction/worklet/ExternalFaces.h>

namespace viskores
{
namespace filter
{
namespace entity_extraction
{
//-----------------------------------------------------------------------------
ExternalFaces::ExternalFaces()
  : Worklet(std::make_unique<viskores::worklet::ExternalFaces>())
{
  this->SetPassPolyData(true);
}

ExternalFaces::~ExternalFaces() = default;

//-----------------------------------------------------------------------------
void ExternalFaces::SetPassPolyData(bool value)
{
  this->PassPolyData = value;
  this->Worklet->SetPassPolyData(value);
}

//-----------------------------------------------------------------------------
viskores::cont::DataSet ExternalFaces::GenerateOutput(const viskores::cont::DataSet& input,
                                                      viskores::cont::UnknownCellSet& outCellSet)
{
  //3. Check the fields of the dataset to see what kinds of fields are present, so
  //   we can free the cell mapping array if it won't be needed.
  const viskores::Id numFields = input.GetNumberOfFields();
  bool hasCellFields = false;
  for (viskores::Id fieldIdx = 0; fieldIdx < numFields && !hasCellFields; ++fieldIdx)
  {
    const auto& f = input.GetField(fieldIdx);
    hasCellFields = f.IsCellField();
  }

  if (!hasCellFields)
  {
    this->Worklet->ReleaseCellMapArrays();
  }

  //4. create the output dataset
  auto mapper = [&](auto& result, const auto& f)
  {
    // New Design: We are still using the old MapFieldOntoOutput to demonstrate the transition
    this->MapFieldOntoOutput(result, f);
  };
  return this->CreateResult(input, outCellSet, mapper);
}

//-----------------------------------------------------------------------------
viskores::cont::DataSet ExternalFaces::DoExecute(const viskores::cont::DataSet& input)
{
  //1. extract the cell set
  const viskores::cont::UnknownCellSet& cells = input.GetCellSet();

  //2. using the policy convert the dynamic cell set, and run the
  // external faces worklet
  viskores::cont::UnknownCellSet outCellSet;

  if (cells.CanConvert<viskores::cont::CellSetStructured<3>>())
  {
    outCellSet = this->Worklet->Run(cells.AsCellSet<viskores::cont::CellSetStructured<3>>());
  }
  else
  {
    outCellSet =
      this->Worklet->Run(cells.ResetCellSetList<VISKORES_DEFAULT_CELL_SET_LIST_UNSTRUCTURED>());
  }

  // New Filter Design: we generate new output and map the fields first.
  auto output = this->GenerateOutput(input, outCellSet);

  // New Filter Design: then we remove entities if requested.
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

bool ExternalFaces::MapFieldOntoOutput(viskores::cont::DataSet& result,
                                       const viskores::cont::Field& field)
{
  if (field.IsPointField())
  {
    result.AddField(field);
    return true;
  }
  else if (field.IsCellField())
  {
    return viskores::filter::MapFieldPermutation(field, this->Worklet->GetCellIdMap(), result);
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
} // namespace entity_extraction
} // namespace filter
} // namespace viskores
