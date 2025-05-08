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

#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetExtrude.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/Logging.h>

#include <algorithm>

namespace
{

VISKORES_CONT void CheckFieldSize(const viskores::cont::UnknownCellSet& cellSet,
                                  const viskores::cont::Field& field)
{
  if (!cellSet.IsValid())
  {
    return;
  }
  switch (field.GetAssociation())
  {
    case viskores::cont::Field::Association::Points:
      if (cellSet.GetNumberOfPoints() != field.GetData().GetNumberOfValues())
      {
        VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                       "The size of field `"
                         << field.GetName() << "` (" << field.GetData().GetNumberOfValues()
                         << " values) does not match the size of the data set structure ("
                         << cellSet.GetNumberOfPoints() << " points).");
      }
      break;
    case viskores::cont::Field::Association::Cells:
      if (cellSet.GetNumberOfCells() != field.GetData().GetNumberOfValues())
      {
        VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                       "The size of field `"
                         << field.GetName() << "` (" << field.GetData().GetNumberOfValues()
                         << " values) does not match the size of the data set structure ("
                         << cellSet.GetNumberOfCells() << " cells).");
      }
      break;
    default:
      // Ignore as the association does not match any topological element.
      break;
  }
}

VISKORES_CONT void CheckFieldSizes(const viskores::cont::UnknownCellSet& cellSet,
                                   const viskores::cont::internal::FieldCollection& fields)
{
  viskores::IdComponent numFields = fields.GetNumberOfFields();
  for (viskores::IdComponent fieldIndex = 0; fieldIndex < numFields; ++fieldIndex)
  {
    CheckFieldSize(cellSet, fields.GetField(fieldIndex));
  }
}

} // anonymous namespace

namespace viskores
{
namespace cont
{

VISKORES_CONT std::string& GlobalGhostCellFieldName() noexcept
{
  static std::string GhostCellName("vtkGhostCells");
  return GhostCellName;
}

VISKORES_CONT const std::string& GetGlobalGhostCellFieldName() noexcept
{
  return GlobalGhostCellFieldName();
}

VISKORES_CONT void SetGlobalGhostCellFieldName(const std::string& name) noexcept
{
  GlobalGhostCellFieldName() = name;
}

void DataSet::Clear()
{
  this->CoordSystemNames.clear();
  this->Fields.Clear();
  this->CellSet = this->CellSet.NewInstance();
}

void DataSet::AddField(const Field& field)
{
  CheckFieldSize(this->CellSet, field);
  this->Fields.AddField(field);
}

void DataSet::AddField(const std::string& name,
                       viskores::cont::Field::Association association,
                       const viskores::cont::UnknownArrayHandle& data)
{
  this->AddField({ name, association, data });
}


viskores::Id DataSet::GetNumberOfCells() const
{
  return this->CellSet.GetNumberOfCells();
}

viskores::Id DataSet::GetNumberOfPoints() const
{
  if (this->CellSet.IsValid())
  {
    return this->CellSet.GetNumberOfPoints();
  }

  // If there is no cell set, then try to use a coordinate system to get the number
  // of points.
  if (this->GetNumberOfCoordinateSystems() > 0)
  {
    return this->GetCoordinateSystem().GetNumberOfPoints();
  }

  // If there is no coordinate system either, we can try to guess the number of
  // points by finding a point field.
  for (viskores::IdComponent fieldIdx = 0; fieldIdx < this->Fields.GetNumberOfFields(); ++fieldIdx)
  {
    const viskores::cont::Field& field = this->Fields.GetField(fieldIdx);
    if (field.GetAssociation() == viskores::cont::Field::Association::Points)
    {
      return field.GetData().GetNumberOfValues();
    }
  }

  // There are no point fields either.
  return 0;
}

const std::string& DataSet::GetGhostCellFieldName() const
{
  if (this->GhostCellName)
  {
    return *this->GhostCellName;
  }
  else
  {
    return GetGlobalGhostCellFieldName();
  }
}

bool DataSet::HasGhostCellField() const
{
  return this->HasCellField(this->GetGhostCellFieldName());
}

viskores::cont::Field DataSet::GetGhostCellField() const
{
  if (this->HasGhostCellField())
  {
    return this->GetCellField(this->GetGhostCellFieldName());
  }
  else
  {
    return make_FieldCell(
      this->GetGhostCellFieldName(),
      viskores::cont::ArrayHandleConstant<viskores::UInt8>(0, this->GetNumberOfCells()));
  }
}

viskores::IdComponent DataSet::AddCoordinateSystem(const viskores::cont::CoordinateSystem& cs)
{
  this->AddField(cs);
  return this->AddCoordinateSystem(cs.GetName());
}

viskores::IdComponent DataSet::AddCoordinateSystem(const std::string& name,
                                                   const viskores::cont::UnknownArrayHandle& data)
{
  return this->AddCoordinateSystem({ name, data });
}

viskores::IdComponent DataSet::AddCoordinateSystem(const std::string& pointFieldName)
{
  // Check to see if we already have this coordinate system.
  viskores::IdComponent index = this->GetCoordinateSystemIndex(pointFieldName);
  if (index >= 0)
  {
    return index;
  }

  // Check to make sure this is a valid point field.
  if (!this->HasPointField(pointFieldName))
  {
    throw viskores::cont::ErrorBadValue("Cannot set point field named `" + pointFieldName +
                                        "` as a coordinate system because it does not exist.");
  }

  // Add the field to the list of coordinates.
  this->CoordSystemNames.push_back(pointFieldName);
  return static_cast<viskores::IdComponent>(this->CoordSystemNames.size() - 1);
}

void DataSet::SetCellSetImpl(const viskores::cont::UnknownCellSet& cellSet)
{
  CheckFieldSizes(cellSet, this->Fields);
  this->CellSet = cellSet;
}

void DataSet::SetGhostCellFieldName(const std::string& name)
{
  this->GhostCellName.reset(new std::string(name));
}

void DataSet::SetGhostCellField(const std::string& name)
{
  if (this->HasCellField(name))
  {
    this->SetGhostCellFieldName(name);
  }
  else
  {
    throw viskores::cont::ErrorBadValue("No such cell field " + name);
  }
}

void DataSet::SetGhostCellField(const viskores::cont::Field& field)
{
  if (field.GetAssociation() == viskores::cont::Field::Association::Cells)
  {
    this->SetGhostCellField(field.GetName(), field.GetData());
  }
  else
  {
    throw viskores::cont::ErrorBadValue("A ghost cell field must be a cell field.");
  }
}

void DataSet::SetGhostCellField(const std::string& fieldName,
                                const viskores::cont::UnknownArrayHandle& field)
{
  this->AddCellField(fieldName, field);
  this->SetGhostCellField(fieldName);
}

void DataSet::SetGhostCellField(const viskores::cont::UnknownArrayHandle& field)
{
  this->SetGhostCellField(GetGlobalGhostCellFieldName(), field);
}

void DataSet::CopyStructure(const viskores::cont::DataSet& source)
{
  // Copy the cells.
  this->CellSet = source.CellSet;

  // Copy the coordinate systems.
  this->CoordSystemNames.clear();
  viskores::IdComponent numCoords = source.GetNumberOfCoordinateSystems();
  for (viskores::IdComponent coordIndex = 0; coordIndex < numCoords; ++coordIndex)
  {
    this->AddCoordinateSystem(source.GetCoordinateSystem(coordIndex));
  }

  // Copy the ghost cells.
  // Note that we copy the GhostCellName separately from the field it points to
  // to preserve (or remove) the case where the ghost cell name follows the
  // global ghost cell name.
  this->GhostCellName = source.GhostCellName;
  if (source.HasGhostCellField())
  {
    this->AddField(source.GetGhostCellField());
  }

  CheckFieldSizes(this->CellSet, this->Fields);
}

viskores::cont::CoordinateSystem DataSet::GetCoordinateSystem(viskores::Id index) const
{
  VISKORES_ASSERT((index >= 0) && (index < this->GetNumberOfCoordinateSystems()));
  return this->GetPointField(this->CoordSystemNames[static_cast<std::size_t>(index)]);
}

viskores::IdComponent DataSet::GetCoordinateSystemIndex(const std::string& name) const
{
  auto nameIter = std::find(this->CoordSystemNames.begin(), this->CoordSystemNames.end(), name);
  if (nameIter != this->CoordSystemNames.end())
  {
    return static_cast<viskores::IdComponent>(
      std::distance(this->CoordSystemNames.begin(), nameIter));
  }
  else
  {
    return -1;
  }
}

const std::string& DataSet::GetCoordinateSystemName(viskores::Id index) const
{
  VISKORES_ASSERT((index >= 0) && (index < this->GetNumberOfCoordinateSystems()));
  return this->CoordSystemNames[static_cast<std::size_t>(index)];
}

viskores::cont::CoordinateSystem DataSet::GetCoordinateSystem(const std::string& name) const
{
  viskores::Id index = this->GetCoordinateSystemIndex(name);
  if (index < 0)
  {
    std::string error_message("No coordinate system with the name " + name +
                              " valid names are: \n");
    for (const auto& csn : this->CoordSystemNames)
    {
      error_message += csn + "\n";
    }
    throw viskores::cont::ErrorBadValue(error_message);
  }
  return this->GetCoordinateSystem(index);
}

void DataSet::PrintSummary(std::ostream& out) const
{
  out << "DataSet:\n";
  out << "  CoordSystems[" << this->CoordSystemNames.size() << "]\n";
  out << "   ";
  for (const auto& csn : this->CoordSystemNames)
  {
    out << " " << csn;
  }
  out << "\n";

  out << "  CellSet \n";
  this->GetCellSet().PrintSummary(out);

  out << "  Fields[" << this->GetNumberOfFields() << "]\n";
  for (viskores::Id index = 0; index < this->GetNumberOfFields(); index++)
  {
    this->GetField(index).PrintSummary(out);
  }

  out.flush();
}

void DataSet::ConvertToExpected()
{
  for (viskores::IdComponent coordIndex = 0; coordIndex < this->GetNumberOfCoordinateSystems();
       ++coordIndex)
  {
    this->GetCoordinateSystem(coordIndex).ConvertToExpected();
  }

  for (viskores::IdComponent fieldIndex = 0; fieldIndex < this->GetNumberOfFields(); ++fieldIndex)
  {
    this->GetField(fieldIndex).ConvertToExpected();
  }
}

} // namespace cont
} // namespace viskores


namespace mangled_diy_namespace
{

using SerializedCellSetTypes =
  viskores::ListAppend<VISKORES_DEFAULT_CELL_SET_LIST,
                       viskores::List<viskores::cont::CellSetStructured<1>,
                                      viskores::cont::CellSetStructured<2>,
                                      viskores::cont::CellSetStructured<3>,
                                      viskores::cont::CellSetExplicit<>,
                                      viskores::cont::CellSetSingleType<>,
                                      viskores::cont::CellSetExtrude>>;
using DefaultDataSetWithCellTypes = viskores::cont::DataSetWithCellSetTypes<SerializedCellSetTypes>;

void Serialization<viskores::cont::DataSet>::save(BinaryBuffer& bb,
                                                  const viskores::cont::DataSet& obj)
{
  viskoresdiy::save(bb, DefaultDataSetWithCellTypes{ obj });
}

void Serialization<viskores::cont::DataSet>::load(BinaryBuffer& bb, viskores::cont::DataSet& obj)
{
  DefaultDataSetWithCellTypes data;
  viskoresdiy::load(bb, data);
  obj = data.DataSet;
}

} // namespace mangled_diy_namespace
