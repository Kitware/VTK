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

#include <viskores/rendering/Actor.h>

#include <viskores/Assert.h>
#include <viskores/cont/BoundsCompute.h>
#include <viskores/cont/FieldRangeCompute.h>
#include <viskores/cont/TryExecute.h>
#include <viskores/cont/UnknownCellSet.h>

#include <utility>

namespace viskores
{
namespace rendering
{

struct Actor::InternalsType
{
  viskores::cont::PartitionedDataSet Data;
  std::string CoordinateName;
  std::string FieldName;
  viskores::cont::Field::Association FieldAssociation;

  viskores::cont::ColorTable ColorTable;

  viskores::Range ScalarRange;
  viskores::Bounds SpatialBounds;

  VISKORES_CONT
  InternalsType(const viskores::cont::PartitionedDataSet partitionedDataSet,
                const std::string coordinateName,
                const std::string fieldName,
                const viskores::rendering::Color& color)
    : Data(partitionedDataSet)
    , CoordinateName(coordinateName)
    , FieldName(fieldName)
    , ColorTable(viskores::Range{ 0, 1 }, color.Components, color.Components)
  {
  }

  VISKORES_CONT
  InternalsType(
    const viskores::cont::PartitionedDataSet partitionedDataSet,
    const std::string coordinateName,
    const std::string fieldName,
    const viskores::cont::ColorTable& colorTable = viskores::cont::ColorTable::Preset::Default)
    : Data(partitionedDataSet)
    , CoordinateName(coordinateName)
    , FieldName(fieldName)
    , ColorTable(colorTable)
  {
  }
};

Actor::Actor(const viskores::cont::DataSet dataSet,
             const std::string coordinateName,
             const std::string fieldName)
{
  viskores::cont::PartitionedDataSet partitionedDataSet(dataSet);
  this->Internals = std::make_unique<InternalsType>(partitionedDataSet, coordinateName, fieldName);
  this->Init();
}

Actor::Actor(const viskores::cont::DataSet dataSet,
             const std::string coordinateName,
             const std::string fieldName,
             const viskores::rendering::Color& color)
{
  viskores::cont::PartitionedDataSet partitionedDataSet(dataSet);
  this->Internals =
    std::make_unique<InternalsType>(partitionedDataSet, coordinateName, fieldName, color);
  this->Init();
}

Actor::Actor(const viskores::cont::DataSet dataSet,
             const std::string coordinateName,
             const std::string fieldName,
             const viskores::cont::ColorTable& colorTable)
{
  viskores::cont::PartitionedDataSet partitionedDataSet(dataSet);
  this->Internals =
    std::make_unique<InternalsType>(partitionedDataSet, coordinateName, fieldName, colorTable);
  this->Init();
}

Actor::Actor(const viskores::cont::PartitionedDataSet dataSet,
             const std::string coordinateName,
             const std::string fieldName)
  : Internals(new InternalsType(dataSet, coordinateName, fieldName))
{
  this->Init();
}

Actor::Actor(const viskores::cont::PartitionedDataSet dataSet,
             const std::string coordinateName,
             const std::string fieldName,
             const viskores::rendering::Color& color)
  : Internals(new InternalsType(dataSet, coordinateName, fieldName, color))
{
  this->Init();
}

Actor::Actor(const viskores::cont::PartitionedDataSet dataSet,
             const std::string coordinateName,
             const std::string fieldName,
             const viskores::cont::ColorTable& colorTable)
  : Internals(new InternalsType(dataSet, coordinateName, fieldName, colorTable))
{
  this->Init();
}

Actor::Actor(const viskores::cont::UnknownCellSet& cells,
             const viskores::cont::CoordinateSystem& coordinates,
             const viskores::cont::Field& scalarField)
{
  viskores::cont::DataSet dataSet;
  dataSet.SetCellSet(cells);
  dataSet.AddCoordinateSystem(coordinates);
  dataSet.AddField(scalarField);
  this->Internals =
    std::make_unique<InternalsType>(dataSet, coordinates.GetName(), scalarField.GetName());
  this->Init();
}

Actor::Actor(const viskores::cont::UnknownCellSet& cells,
             const viskores::cont::CoordinateSystem& coordinates,
             const viskores::cont::Field& scalarField,
             const viskores::rendering::Color& color)
{
  viskores::cont::DataSet dataSet;
  dataSet.SetCellSet(cells);
  dataSet.AddCoordinateSystem(coordinates);
  dataSet.AddField(scalarField);
  this->Internals =
    std::make_unique<InternalsType>(dataSet, coordinates.GetName(), scalarField.GetName(), color);
  this->Init();
}

Actor::Actor(const viskores::cont::UnknownCellSet& cells,
             const viskores::cont::CoordinateSystem& coordinates,
             const viskores::cont::Field& scalarField,
             const viskores::cont::ColorTable& colorTable)
{
  viskores::cont::DataSet dataSet;
  dataSet.SetCellSet(cells);
  dataSet.AddCoordinateSystem(coordinates);
  dataSet.AddField(scalarField);
  this->Internals = std::make_unique<InternalsType>(
    dataSet, coordinates.GetName(), scalarField.GetName(), colorTable);
  this->Init();
}

Actor::Actor(const Actor& rhs)
  : Internals(nullptr)
{
  // rhs might have been moved, its Internal would be nullptr
  if (rhs.Internals)
    Internals = std::make_unique<InternalsType>(*rhs.Internals);
}

Actor& Actor::operator=(const Actor& rhs)
{
  // both *this and rhs might have been moved.
  if (!rhs.Internals)
  {
    Internals.reset();
  }
  else if (!Internals)
  {
    Internals = std::make_unique<InternalsType>(*rhs.Internals);
  }
  else
  {
    *Internals = *rhs.Internals;
  }

  return *this;
}

Actor::Actor(viskores::rendering::Actor&&) noexcept = default;
Actor& Actor::operator=(Actor&&) noexcept = default;
Actor::~Actor() = default;

void Actor::Init()
{
  this->Internals->SpatialBounds = viskores::cont::BoundsCompute(this->Internals->Data);
  this->Internals->ScalarRange =
    viskores::cont::FieldRangeCompute(this->Internals->Data, this->Internals->FieldName)
      .ReadPortal()
      .Get(0);
}

void Actor::Render(viskores::rendering::Mapper& mapper,
                   viskores::rendering::Canvas& canvas,
                   const viskores::rendering::Camera& camera) const
{
  mapper.SetCanvas(&canvas);
  mapper.SetActiveColorTable(this->Internals->ColorTable);
  mapper.RenderCellsPartitioned(this->Internals->Data,
                                this->Internals->FieldName,
                                this->Internals->ColorTable,
                                camera,
                                this->Internals->ScalarRange);
}


const viskores::cont::UnknownCellSet& Actor::GetCells() const
{
  return this->Internals->Data.GetPartition(0).GetCellSet();
}

viskores::cont::CoordinateSystem Actor::GetCoordinates() const
{
  return this->Internals->Data.GetPartition(0).GetCoordinateSystem(this->Internals->CoordinateName);
}

const viskores::cont::Field& Actor::GetScalarField() const
{
  return this->Internals->Data.GetPartition(0).GetField(this->Internals->FieldName);
}

const viskores::cont::ColorTable& Actor::GetColorTable() const
{
  return this->Internals->ColorTable;
}

const viskores::Range& Actor::GetScalarRange() const
{
  return this->Internals->ScalarRange;
}

const viskores::Bounds& Actor::GetSpatialBounds() const
{
  return this->Internals->SpatialBounds;
}

void Actor::SetScalarRange(const viskores::Range& scalarRange)
{
  this->Internals->ScalarRange = scalarRange;
}
}
} // namespace viskores::rendering
