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

#ifndef viskores_internal_ConnectivityStructuredInternals_h
#define viskores_internal_ConnectivityStructuredInternals_h

#include <viskores/CellShape.h>
#include <viskores/StaticAssert.h>
#include <viskores/TopologyElementTag.h>
#include <viskores/Types.h>
#include <viskores/VecVariable.h>

#include <viskores/internal/Assume.h>

namespace viskores
{
namespace internal
{

template <viskores::IdComponent>
class ConnectivityStructuredInternals;

//1 D specialization.
template <>
class ConnectivityStructuredInternals<1>
{
public:
  using SchedulingRangeType = viskores::Id;

  VISKORES_EXEC_CONT
  void SetPointDimensions(viskores::Id dimensions) { this->PointDimensions = dimensions; }

  VISKORES_EXEC_CONT
  void SetGlobalPointDimensions(viskores::Id dimensions)
  {
    this->GlobalPointDimensions = dimensions;
  }

  VISKORES_EXEC_CONT
  void SetGlobalPointIndexStart(viskores::Id start) { this->GlobalPointIndexStart = start; }

  VISKORES_EXEC_CONT
  viskores::Id GetPointDimensions() const { return this->PointDimensions; }

  VISKORES_EXEC_CONT
  viskores::Id GetGlobalPointDimensions() const { return this->GlobalPointDimensions; }

  VISKORES_EXEC_CONT
  viskores::Id GetCellDimensions() const { return this->PointDimensions - 1; }

  VISKORES_EXEC_CONT
  viskores::Id GetGlobalCellDimensions() const { return this->GlobalPointDimensions - 1; }

  VISKORES_EXEC_CONT
  SchedulingRangeType GetSchedulingRange(viskores::TopologyElementTagCell) const
  {
    return this->GetNumberOfCells();
  }

  VISKORES_EXEC_CONT
  SchedulingRangeType GetSchedulingRange(viskores::TopologyElementTagPoint) const
  {
    return this->GetNumberOfPoints();
  }

  VISKORES_EXEC_CONT
  SchedulingRangeType GetGlobalPointIndexStart() const { return this->GlobalPointIndexStart; }

  static constexpr viskores::IdComponent NUM_POINTS_IN_CELL = 2;
  static constexpr viskores::IdComponent MAX_CELL_TO_POINT = 2;

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfPoints() const { return this->PointDimensions; }
  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfCells() const { return this->PointDimensions - 1; }
  VISKORES_EXEC_CONT
  viskores::IdComponent GetNumberOfPointsInCell() const { return NUM_POINTS_IN_CELL; }
  VISKORES_EXEC_CONT
  viskores::IdComponent GetCellShape() const { return viskores::CELL_SHAPE_LINE; }

  using CellShapeTag = viskores::CellShapeTagLine;

  VISKORES_EXEC_CONT
  viskores::Vec<viskores::Id, NUM_POINTS_IN_CELL> GetPointsOfCell(viskores::Id index) const
  {
    VISKORES_ASSUME(index >= 0);

    viskores::Vec<viskores::Id, NUM_POINTS_IN_CELL> pointIds;
    pointIds[0] = index;
    pointIds[1] = pointIds[0] + 1;
    return pointIds;
  }

  VISKORES_EXEC_CONT
  viskores::IdComponent GetNumberOfCellsOnPoint(viskores::Id pointIndex) const
  {
    VISKORES_ASSUME(pointIndex >= 0);

    if ((pointIndex > 0) && (pointIndex < this->PointDimensions - 1))
    {
      return 2;
    }
    else
    {
      return 1;
    }
  }

  VISKORES_EXEC_CONT
  viskores::VecVariable<viskores::Id, MAX_CELL_TO_POINT> GetCellsOfPoint(viskores::Id index) const
  {
    VISKORES_ASSUME(index >= 0);
    VISKORES_ASSUME(this->PointDimensions > 1);

    viskores::VecVariable<viskores::Id, MAX_CELL_TO_POINT> cellIds;

    if (index > 0)
    {
      cellIds.Append(index - 1);
    }
    if (index < this->PointDimensions - 1)
    {
      cellIds.Append(index);
    }

    return cellIds;
  }


  VISKORES_EXEC_CONT
  viskores::Id FlatToLogicalPointIndex(viskores::Id flatPointIndex) const { return flatPointIndex; }

  VISKORES_EXEC_CONT
  viskores::Id LogicalToFlatPointIndex(viskores::Id logicalPointIndex) const
  {
    return logicalPointIndex;
  }

  VISKORES_EXEC_CONT
  viskores::Id FlatToLogicalCellIndex(viskores::Id flatCellIndex) const { return flatCellIndex; }

  VISKORES_EXEC_CONT
  viskores::Id LogicalToFlatCellIndex(viskores::Id logicalCellIndex) const
  {
    return logicalCellIndex;
  }

  VISKORES_CONT
  void PrintSummary(std::ostream& out) const
  {
    out << "   UniformConnectivity<1> ";
    out << "PointDimensions[" << this->PointDimensions << "] ";
    out << "GlobalPointDimensions[" << this->GlobalPointDimensions << "] ";
    out << "GlobalPointIndexStart[" << this->GlobalPointIndexStart << "] ";
    out << "\n";
  }

private:
  viskores::Id PointDimensions = 0;
  viskores::Id GlobalPointDimensions = 0;
  viskores::Id GlobalPointIndexStart = 0;
};

//2 D specialization.
template <>
class ConnectivityStructuredInternals<2>
{
public:
  using SchedulingRangeType = viskores::Id2;

  VISKORES_EXEC_CONT
  void SetPointDimensions(viskores::Id2 dims) { this->PointDimensions = dims; }

  VISKORES_EXEC_CONT
  void SetGlobalPointDimensions(viskores::Id2 dims) { this->GlobalPointDimensions = dims; }

  VISKORES_EXEC_CONT
  void SetGlobalPointIndexStart(viskores::Id2 start) { this->GlobalPointIndexStart = start; }

  VISKORES_EXEC_CONT
  const viskores::Id2& GetPointDimensions() const { return this->PointDimensions; }

  VISKORES_EXEC_CONT
  const viskores::Id2& GetGlobalPointDimensions() const { return this->GlobalPointDimensions; }

  VISKORES_EXEC_CONT
  viskores::Id2 GetCellDimensions() const { return this->PointDimensions - viskores::Id2(1); }

  VISKORES_EXEC_CONT
  viskores::Id2 GetGlobalCellDimensions() const
  {
    return this->GlobalPointDimensions - viskores::Id2(1);
  }

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfPoints() const
  {
    return viskores::ReduceProduct(this->GetPointDimensions());
  }

  //returns an id2 to signal what kind of scheduling to use
  VISKORES_EXEC_CONT
  viskores::Id2 GetSchedulingRange(viskores::TopologyElementTagCell) const
  {
    return this->GetCellDimensions();
  }
  VISKORES_EXEC_CONT
  viskores::Id2 GetSchedulingRange(viskores::TopologyElementTagPoint) const
  {
    return this->GetPointDimensions();
  }

  VISKORES_EXEC_CONT
  const viskores::Id2& GetGlobalPointIndexStart() const { return this->GlobalPointIndexStart; }

  static constexpr viskores::IdComponent NUM_POINTS_IN_CELL = 4;
  static constexpr viskores::IdComponent MAX_CELL_TO_POINT = 4;

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfCells() const
  {
    return viskores::ReduceProduct(this->GetCellDimensions());
  }
  VISKORES_EXEC_CONT
  viskores::IdComponent GetNumberOfPointsInCell() const { return NUM_POINTS_IN_CELL; }
  VISKORES_EXEC_CONT
  viskores::IdComponent GetCellShape() const { return viskores::CELL_SHAPE_QUAD; }

  using CellShapeTag = viskores::CellShapeTagQuad;

  VISKORES_EXEC_CONT
  viskores::Vec<viskores::Id, NUM_POINTS_IN_CELL> GetPointsOfCell(
    const SchedulingRangeType& logicalCellIndex) const
  {
    viskores::Vec<viskores::Id, NUM_POINTS_IN_CELL> pointIds;
    pointIds[0] = this->LogicalToFlatPointIndex(logicalCellIndex);
    pointIds[1] = pointIds[0] + 1;
    pointIds[2] = pointIds[1] + this->PointDimensions[0];
    pointIds[3] = pointIds[2] - 1;
    return pointIds;
  }

  VISKORES_EXEC_CONT
  viskores::Vec<viskores::Id, NUM_POINTS_IN_CELL> GetPointsOfCell(viskores::Id cellIndex) const
  {
    return this->GetPointsOfCell(this->FlatToLogicalCellIndex(cellIndex));
  }

  VISKORES_EXEC_CONT viskores::IdComponent GetNumberOfCellsOnPoint(
    const SchedulingRangeType& ij) const
  {
    viskores::IdComponent numCells = 1;

    for (viskores::IdComponent dim = 0; dim < 2; dim++)
    {
      if ((ij[dim] > 0) && (ij[dim] < this->PointDimensions[dim] - 1))
      {
        numCells *= 2;
      }
    }

    return numCells;
  }

  VISKORES_EXEC_CONT
  viskores::IdComponent GetNumberOfCellsOnPoint(viskores::Id pointIndex) const
  {
    return this->GetNumberOfCellsOnPoint(this->FlatToLogicalPointIndex(pointIndex));
  }

  VISKORES_EXEC_CONT
  viskores::VecVariable<viskores::Id, MAX_CELL_TO_POINT> GetCellsOfPoint(
    const SchedulingRangeType& ij) const
  {
    viskores::VecVariable<viskores::Id, MAX_CELL_TO_POINT> cellIds;

    if ((ij[0] > 0) && (ij[1] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ij - viskores::Id2(1, 1)));
    }
    if ((ij[0] < this->PointDimensions[0] - 1) && (ij[1] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ij - viskores::Id2(0, 1)));
    }
    if ((ij[0] > 0) && (ij[1] < this->PointDimensions[1] - 1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ij - viskores::Id2(1, 0)));
    }
    if ((ij[0] < this->PointDimensions[0] - 1) && (ij[1] < this->PointDimensions[1] - 1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ij));
    }

    return cellIds;
  }

  VISKORES_EXEC_CONT
  viskores::VecVariable<viskores::Id, MAX_CELL_TO_POINT> GetCellsOfPoint(
    viskores::Id pointIndex) const
  {
    return this->GetCellsOfPoint(this->FlatToLogicalPointIndex(pointIndex));
  }

  VISKORES_EXEC_CONT
  viskores::Id2 FlatToLogicalPointIndex(viskores::Id flatPointIndex) const
  {
    viskores::Id2 logicalPointIndex;
    logicalPointIndex[0] = flatPointIndex % this->PointDimensions[0];
    logicalPointIndex[1] = flatPointIndex / this->PointDimensions[0];
    return logicalPointIndex;
  }

  VISKORES_EXEC_CONT
  viskores::Id LogicalToFlatPointIndex(const viskores::Id2& logicalPointIndex) const
  {
    return logicalPointIndex[0] + this->PointDimensions[0] * logicalPointIndex[1];
  }

  VISKORES_EXEC_CONT
  viskores::Id2 FlatToLogicalCellIndex(viskores::Id flatCellIndex) const
  {
    viskores::Id2 cellDimensions = this->GetCellDimensions();
    viskores::Id2 logicalCellIndex;
    logicalCellIndex[0] = flatCellIndex % cellDimensions[0];
    logicalCellIndex[1] = flatCellIndex / cellDimensions[0];
    return logicalCellIndex;
  }

  VISKORES_EXEC_CONT
  viskores::Id LogicalToFlatCellIndex(const viskores::Id2& logicalCellIndex) const
  {
    viskores::Id2 cellDimensions = this->GetCellDimensions();
    return logicalCellIndex[0] + cellDimensions[0] * logicalCellIndex[1];
  }

  VISKORES_CONT
  void PrintSummary(std::ostream& out) const
  {
    out << "   UniformConnectivity<2> ";
    out << "PointDimensions[" << this->PointDimensions[0] << " " << this->PointDimensions[1]
        << "] ";
    out << "GlobalPointDimensions[" << this->GlobalPointDimensions[0] << " "
        << this->GlobalPointDimensions[1] << "] ";
    out << "GlobalPointIndexStart[" << this->GlobalPointIndexStart[0] << " "
        << this->GlobalPointIndexStart[1] << "] ";
    out << std::endl;
  }

private:
  viskores::Id2 PointDimensions = { 0, 0 };
  viskores::Id2 GlobalPointDimensions = { 0, 0 };
  viskores::Id2 GlobalPointIndexStart = { 0, 0 };
};

//3 D specialization.
template <>
class ConnectivityStructuredInternals<3>
{
public:
  using SchedulingRangeType = viskores::Id3;

  VISKORES_EXEC_CONT
  void SetPointDimensions(viskores::Id3 dims)
  {
    this->PointDimensions = dims;
    this->CellDimensions = dims - viskores::Id3(1);
    this->CellDim01 = (dims[0] - 1) * (dims[1] - 1);
  }

  VISKORES_EXEC_CONT
  void SetGlobalPointDimensions(viskores::Id3 dims)
  {
    this->GlobalPointDimensions = dims;
    this->GlobalCellDimensions = dims - viskores::Id3(1);
  }

  VISKORES_EXEC_CONT
  void SetGlobalPointIndexStart(viskores::Id3 start) { this->GlobalPointIndexStart = start; }

  VISKORES_EXEC_CONT
  const viskores::Id3& GetPointDimensions() const { return this->PointDimensions; }

  VISKORES_EXEC_CONT
  const viskores::Id3& GetGlobalPointDimensions() const { return this->GlobalPointDimensions; }

  VISKORES_EXEC_CONT
  const viskores::Id3& GetCellDimensions() const { return this->CellDimensions; }

  VISKORES_EXEC_CONT
  const viskores::Id3& GetGlobalCellDimensions() const { return this->GlobalCellDimensions; }

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfPoints() const { return viskores::ReduceProduct(this->PointDimensions); }

  //returns an id3 to signal what kind of scheduling to use
  VISKORES_EXEC_CONT
  const viskores::Id3& GetSchedulingRange(viskores::TopologyElementTagCell) const
  {
    return this->GetCellDimensions();
  }
  VISKORES_EXEC_CONT
  const viskores::Id3& GetSchedulingRange(viskores::TopologyElementTagPoint) const
  {
    return this->GetPointDimensions();
  }

  VISKORES_EXEC_CONT
  const viskores::Id3& GetGlobalPointIndexStart() const { return this->GlobalPointIndexStart; }

  static constexpr viskores::IdComponent NUM_POINTS_IN_CELL = 8;
  static constexpr viskores::IdComponent MAX_CELL_TO_POINT = 8;

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfCells() const
  {
    return viskores::ReduceProduct(this->GetCellDimensions());
  }
  VISKORES_EXEC_CONT
  viskores::IdComponent GetNumberOfPointsInCell() const { return NUM_POINTS_IN_CELL; }
  VISKORES_EXEC_CONT
  viskores::IdComponent GetCellShape() const { return viskores::CELL_SHAPE_HEXAHEDRON; }

  using CellShapeTag = viskores::CellShapeTagHexahedron;

  VISKORES_EXEC_CONT
  viskores::Vec<viskores::Id, NUM_POINTS_IN_CELL> GetPointsOfCell(
    const SchedulingRangeType& ijk) const
  {
    viskores::Vec<viskores::Id, NUM_POINTS_IN_CELL> pointIds;
    pointIds[0] = (ijk[2] * this->PointDimensions[1] + ijk[1]) * this->PointDimensions[0] + ijk[0];
    pointIds[1] = pointIds[0] + 1;
    pointIds[2] = pointIds[1] + this->PointDimensions[0];
    pointIds[3] = pointIds[2] - 1;
    pointIds[4] = pointIds[0] + this->PointDimensions[0] * this->PointDimensions[1];
    pointIds[5] = pointIds[4] + 1;
    pointIds[6] = pointIds[5] + this->PointDimensions[0];
    pointIds[7] = pointIds[6] - 1;

    return pointIds;
  }

  VISKORES_EXEC_CONT
  viskores::Vec<viskores::Id, NUM_POINTS_IN_CELL> GetPointsOfCell(viskores::Id cellIndex) const
  {
    return this->GetPointsOfCell(this->FlatToLogicalCellIndex(cellIndex));
  }

  VISKORES_EXEC_CONT viskores::IdComponent GetNumberOfCellsOnPoint(
    const SchedulingRangeType& ijk) const
  {
    viskores::IdComponent numCells = 1;

    for (viskores::IdComponent dim = 0; dim < 3; dim++)
    {
      if ((ijk[dim] > 0) && (ijk[dim] < this->PointDimensions[dim] - 1))
      {
        numCells *= 2;
      }
    }

    return numCells;
  }

  VISKORES_EXEC_CONT
  viskores::IdComponent GetNumberOfCellsOnPoint(viskores::Id pointIndex) const
  {
    return this->GetNumberOfCellsOnPoint(this->FlatToLogicalPointIndex(pointIndex));
  }

  VISKORES_EXEC_CONT
  viskores::VecVariable<viskores::Id, MAX_CELL_TO_POINT> GetCellsOfPoint(
    const SchedulingRangeType& ijk) const
  {
    viskores::VecVariable<viskores::Id, MAX_CELL_TO_POINT> cellIds;

    if ((ijk[0] > 0) && (ijk[1] > 0) && (ijk[2] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - viskores::Id3(1, 1, 1)));
    }
    if ((ijk[0] < this->PointDimensions[0] - 1) && (ijk[1] > 0) && (ijk[2] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - viskores::Id3(0, 1, 1)));
    }
    if ((ijk[0] > 0) && (ijk[1] < this->PointDimensions[1] - 1) && (ijk[2] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - viskores::Id3(1, 0, 1)));
    }
    if ((ijk[0] < this->PointDimensions[0] - 1) && (ijk[1] < this->PointDimensions[1] - 1) &&
        (ijk[2] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - viskores::Id3(0, 0, 1)));
    }

    if ((ijk[0] > 0) && (ijk[1] > 0) && (ijk[2] < this->PointDimensions[2] - 1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - viskores::Id3(1, 1, 0)));
    }
    if ((ijk[0] < this->PointDimensions[0] - 1) && (ijk[1] > 0) &&
        (ijk[2] < this->PointDimensions[2] - 1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - viskores::Id3(0, 1, 0)));
    }
    if ((ijk[0] > 0) && (ijk[1] < this->PointDimensions[1] - 1) &&
        (ijk[2] < this->PointDimensions[2] - 1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - viskores::Id3(1, 0, 0)));
    }
    if ((ijk[0] < this->PointDimensions[0] - 1) && (ijk[1] < this->PointDimensions[1] - 1) &&
        (ijk[2] < this->PointDimensions[2] - 1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk));
    }

    return cellIds;
  }

  VISKORES_EXEC_CONT
  viskores::VecVariable<viskores::Id, MAX_CELL_TO_POINT> GetCellsOfPoint(
    viskores::Id pointIndex) const
  {
    return this->GetCellsOfPoint(this->FlatToLogicalPointIndex(pointIndex));
  }

  VISKORES_CONT
  void PrintSummary(std::ostream& out) const
  {
    out << "   UniformConnectivity<3> ";
    out << "PointDimensions[" << this->PointDimensions[0] << " " << this->PointDimensions[1] << " "
        << this->PointDimensions[2] << "] ";
    out << "GlobalPointDimensions[" << this->GlobalPointDimensions[0] << " "
        << this->GlobalPointDimensions[1] << " " << this->GlobalPointDimensions[2] << "] ";
    out << "GlobalPointIndexStart[" << this->GlobalPointIndexStart[0] << " "
        << this->GlobalPointIndexStart[1] << " " << this->GlobalPointIndexStart[2] << "] ";
    out << std::endl;
  }

  VISKORES_EXEC_CONT
  viskores::Id3 FlatToLogicalPointIndex(viskores::Id flatPointIndex) const
  {
    const viskores::Id pointDims01 = this->PointDimensions[0] * this->PointDimensions[1];
    const viskores::Id indexij = flatPointIndex % pointDims01;

    return viskores::Id3(indexij % this->PointDimensions[0],
                         indexij / this->PointDimensions[0],
                         flatPointIndex / pointDims01);
  }

  VISKORES_EXEC_CONT
  viskores::Id LogicalToFlatPointIndex(const viskores::Id3& logicalPointIndex) const
  {
    return logicalPointIndex[0] +
      this->PointDimensions[0] *
      (logicalPointIndex[1] + this->PointDimensions[1] * logicalPointIndex[2]);
  }

  VISKORES_EXEC_CONT
  viskores::Id3 FlatToLogicalCellIndex(viskores::Id flatCellIndex) const
  {
    const viskores::Id indexij = flatCellIndex % this->CellDim01;
    return viskores::Id3(indexij % this->CellDimensions[0],
                         indexij / this->CellDimensions[0],
                         flatCellIndex / this->CellDim01);
  }

  VISKORES_EXEC_CONT
  viskores::Id LogicalToFlatCellIndex(const viskores::Id3& logicalCellIndex) const
  {
    return logicalCellIndex[0] +
      this->CellDimensions[0] *
      (logicalCellIndex[1] + this->CellDimensions[1] * logicalCellIndex[2]);
  }

private:
  viskores::Id3 PointDimensions = { 0, 0, 0 };
  viskores::Id3 GlobalPointDimensions = { 0, 0, 0 };
  viskores::Id3 GlobalCellDimensions = { 0, 0, 0 };
  viskores::Id3 GlobalPointIndexStart = { 0, 0, 0 };
  viskores::Id3 CellDimensions = { 0, 0, 0 };
  viskores::Id CellDim01 = 0;
};

// We may want to generalize this class depending on how ConnectivityExplicit
// eventually handles retrieving cell to point connectivity.

template <typename VisitTopology, typename IncidentTopology, viskores::IdComponent Dimension>
struct ConnectivityStructuredIndexHelper
{
  // We want an unconditional failure if this unspecialized class ever gets
  // instantiated, because it means someone missed a topology mapping type.
  // We need to create a test which depends on the templated types so
  // it doesn't get picked up without a concrete instantiation.
  VISKORES_STATIC_ASSERT_MSG(sizeof(VisitTopology) == static_cast<size_t>(-1),
                             "Missing Specialization for Topologies");
};

template <viskores::IdComponent Dimension>
struct ConnectivityStructuredIndexHelper<viskores::TopologyElementTagCell,
                                         viskores::TopologyElementTagPoint,
                                         Dimension>
{
  using ConnectivityType = viskores::internal::ConnectivityStructuredInternals<Dimension>;
  using LogicalIndexType = typename ConnectivityType::SchedulingRangeType;

  using CellShapeTag = typename ConnectivityType::CellShapeTag;

  using IndicesType = viskores::Vec<viskores::Id, ConnectivityType::NUM_POINTS_IN_CELL>;

  VISKORES_EXEC_CONT static viskores::Id GetNumberOfElements(const ConnectivityType& connectivity)
  {
    return connectivity.GetNumberOfCells();
  }

  template <typename IndexType>
  VISKORES_EXEC_CONT static viskores::IdComponent GetNumberOfIndices(
    const ConnectivityType& viskoresNotUsed(connectivity),
    const IndexType& viskoresNotUsed(cellIndex))
  {
    return ConnectivityType::NUM_POINTS_IN_CELL;
  }

  template <typename IndexType>
  VISKORES_EXEC_CONT static IndicesType GetIndices(const ConnectivityType& connectivity,
                                                   const IndexType& cellIndex)
  {
    return connectivity.GetPointsOfCell(cellIndex);
  }

  VISKORES_EXEC_CONT
  static LogicalIndexType FlatToLogicalIncidentIndex(const ConnectivityType& connectivity,
                                                     viskores::Id flatFromIndex)
  {
    return connectivity.FlatToLogicalPointIndex(flatFromIndex);
  }

  VISKORES_EXEC_CONT
  static viskores::Id LogicalToFlatIncidentIndex(const ConnectivityType& connectivity,
                                                 const LogicalIndexType& logicalFromIndex)
  {
    return connectivity.LogicalToFlatPointIndex(logicalFromIndex);
  }

  VISKORES_EXEC_CONT
  static LogicalIndexType FlatToLogicalVisitIndex(const ConnectivityType& connectivity,
                                                  viskores::Id flatToIndex)
  {
    return connectivity.FlatToLogicalCellIndex(flatToIndex);
  }

  VISKORES_EXEC_CONT
  static viskores::Id LogicalToFlatVisitIndex(const ConnectivityType& connectivity,
                                              const LogicalIndexType& logicalToIndex)
  {
    return connectivity.LogicalToFlatCellIndex(logicalToIndex);
  }
};

template <viskores::IdComponent Dimension>
struct ConnectivityStructuredIndexHelper<viskores::TopologyElementTagPoint,
                                         viskores::TopologyElementTagCell,
                                         Dimension>
{
  using ConnectivityType = viskores::internal::ConnectivityStructuredInternals<Dimension>;
  using LogicalIndexType = typename ConnectivityType::SchedulingRangeType;

  using CellShapeTag = viskores::CellShapeTagVertex;

  using IndicesType = viskores::VecVariable<viskores::Id, ConnectivityType::MAX_CELL_TO_POINT>;

  VISKORES_EXEC_CONT static viskores::Id GetNumberOfElements(const ConnectivityType& connectivity)
  {
    return connectivity.GetNumberOfPoints();
  }

  template <typename IndexType>
  VISKORES_EXEC_CONT static viskores::IdComponent GetNumberOfIndices(
    const ConnectivityType& connectivity,
    const IndexType& pointIndex)
  {
    return connectivity.GetNumberOfCellsOnPoint(pointIndex);
  }

  template <typename IndexType>
  VISKORES_EXEC_CONT static IndicesType GetIndices(const ConnectivityType& connectivity,
                                                   const IndexType& pointIndex)
  {
    return connectivity.GetCellsOfPoint(pointIndex);
  }

  VISKORES_EXEC_CONT
  static LogicalIndexType FlatToLogicalIncidentIndex(const ConnectivityType& connectivity,
                                                     viskores::Id flatFromIndex)
  {
    return connectivity.FlatToLogicalCellIndex(flatFromIndex);
  }

  VISKORES_EXEC_CONT
  static viskores::Id LogicalToFlatIncidentIndex(const ConnectivityType& connectivity,
                                                 const LogicalIndexType& logicalFromIndex)
  {
    return connectivity.LogicalToFlatCellIndex(logicalFromIndex);
  }

  VISKORES_EXEC_CONT
  static LogicalIndexType FlatToLogicalVisitIndex(const ConnectivityType& connectivity,
                                                  viskores::Id flatToIndex)
  {
    return connectivity.FlatToLogicalPointIndex(flatToIndex);
  }

  VISKORES_EXEC_CONT
  static viskores::Id LogicalToFlatVisitIndex(const ConnectivityType& connectivity,
                                              const LogicalIndexType& logicalToIndex)
  {
    return connectivity.LogicalToFlatPointIndex(logicalToIndex);
  }
};
}
} // namespace viskores::internal

#endif //viskores_internal_ConnectivityStructuredInternals_h
