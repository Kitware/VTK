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

#include <viskores/interop/anari/ANARIActor.h>

namespace viskores
{
namespace interop
{
namespace anari
{

const char* AnariMaterialInputString(viskores::IdComponent p)
{
  switch (p)
  {
    case 0:
    default:
      return "attribute0";
    case 1:
      return "attribute1";
    case 2:
      return "attribute2";
    case 3:
      return "attribute3";
  }

  return "attribute0";
}

ANARIActor::ANARIActor(const viskores::cont::UnknownCellSet& cells,
                       const viskores::cont::CoordinateSystem& coordinates,
                       const viskores::cont::Field& field0,
                       const viskores::cont::Field& field1,
                       const viskores::cont::Field& field2,
                       const viskores::cont::Field& field3)
{
  this->Data->Cells = cells;
  this->Data->Coordinates = coordinates;
  this->Data->Fields[0] = field0;
  this->Data->Fields[1] = field1;
  this->Data->Fields[2] = field2;
  this->Data->Fields[3] = field3;
}

ANARIActor::ANARIActor(const viskores::cont::UnknownCellSet& cells,
                       const viskores::cont::CoordinateSystem& coordinates,
                       const FieldSet& f)
  : ANARIActor(cells, coordinates, f[0], f[1], f[2], f[3])
{
}

ANARIActor::ANARIActor(const viskores::cont::DataSet& dataset,
                       const std::string& field0,
                       const std::string& field1,
                       const std::string& field2,
                       const std::string& field3)
{
  this->Data->Cells = dataset.GetCellSet();
  if (dataset.GetNumberOfCoordinateSystems() > 0)
    this->Data->Coordinates = dataset.GetCoordinateSystem();
  this->Data->Fields[0] = field0.empty() ? viskores::cont::Field{} : dataset.GetField(field0);
  this->Data->Fields[1] = field1.empty() ? viskores::cont::Field{} : dataset.GetField(field1);
  this->Data->Fields[2] = field2.empty() ? viskores::cont::Field{} : dataset.GetField(field2);
  this->Data->Fields[3] = field3.empty() ? viskores::cont::Field{} : dataset.GetField(field3);
}

const viskores::cont::UnknownCellSet& ANARIActor::GetCellSet() const
{
  return this->Data->Cells;
}

const viskores::cont::CoordinateSystem& ANARIActor::GetCoordinateSystem() const
{
  return this->Data->Coordinates;
}

const viskores::cont::Field& ANARIActor::GetField(viskores::IdComponent idx) const
{
  return this->Data->Fields[idx < 0 ? GetPrimaryFieldIndex() : idx];
}

FieldSet ANARIActor::GetFieldSet() const
{
  return this->Data->Fields;
}

void ANARIActor::SetPrimaryFieldIndex(viskores::IdComponent idx)
{
  this->Data->PrimaryField = idx;
}

viskores::IdComponent ANARIActor::GetPrimaryFieldIndex() const
{
  return this->Data->PrimaryField;
}

viskores::cont::DataSet ANARIActor::MakeDataSet(bool includeFields) const
{
  viskores::cont::DataSet dataset;
  dataset.SetCellSet(GetCellSet());
  dataset.AddCoordinateSystem(GetCoordinateSystem());
  if (!includeFields)
    return dataset;

  auto addField = [&](const viskores::cont::Field& field)
  {
    if (field.GetNumberOfValues() > 0)
      dataset.AddField(field);
  };

  addField(this->Data->Fields[0]);
  addField(this->Data->Fields[1]);
  addField(this->Data->Fields[2]);
  addField(this->Data->Fields[3]);

  return dataset;
}

} // namespace anari
} // namespace interop
} // namespace viskores
