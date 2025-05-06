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
#ifndef viskores_exec_ConnectivityExtrude_h
#define viskores_exec_ConnectivityExtrude_h

#include <viskores/internal/IndicesExtrude.h>

#include <viskores/CellShape.h>
#include <viskores/cont/ArrayHandle.h>

#include <viskores/exec/arg/ThreadIndicesTopologyMap.h>


namespace viskores
{
namespace exec
{

class VISKORES_ALWAYS_EXPORT ConnectivityExtrude
{
private:
  using Int32HandleType = viskores::cont::ArrayHandle<viskores::Int32>;
  using Int32PortalType = typename Int32HandleType::ReadPortalType;

public:
  using ConnectivityPortalType = Int32PortalType;
  using NextNodePortalType = Int32PortalType;

  using SchedulingRangeType = viskores::Id2;

  using CellShapeTag = viskores::CellShapeTagWedge;

  using IndicesType = IndicesExtrude;

  ConnectivityExtrude() = default;

  ConnectivityExtrude(const ConnectivityPortalType& conn,
                      const NextNodePortalType& nextnode,
                      viskores::Int32 cellsPerPlane,
                      viskores::Int32 pointsPerPlane,
                      viskores::Int32 numPlanes,
                      bool periodic);

  VISKORES_EXEC
  viskores::Id GetNumberOfElements() const { return this->NumberOfCells; }

  VISKORES_EXEC
  CellShapeTag GetCellShape(viskores::Id) const { return viskores::CellShapeTagWedge(); }

  VISKORES_EXEC
  IndicesType GetIndices(viskores::Id index) const
  {
    return this->GetIndices(this->FlatToLogicalToIndex(index));
  }

  VISKORES_EXEC
  IndicesType GetIndices(const viskores::Id2& index) const;
  template <typename IndexType>
  VISKORES_EXEC viskores::IdComponent GetNumberOfIndices(
    const IndexType& viskoresNotUsed(index)) const
  {
    return 6;
  }

  VISKORES_EXEC
  viskores::Id LogicalToFlatToIndex(const viskores::Id2& index) const
  {
    return index[0] + (index[1] * this->NumberOfCellsPerPlane);
  };

  VISKORES_EXEC
  viskores::Id2 FlatToLogicalToIndex(viskores::Id index) const
  {
    const viskores::Id cellId = index % this->NumberOfCellsPerPlane;
    const viskores::Id plane = index / this->NumberOfCellsPerPlane;
    return viskores::Id2(cellId, plane);
  }

private:
  ConnectivityPortalType Connectivity;
  NextNodePortalType NextNode;
  viskores::Int32 NumberOfCellsPerPlane;
  viskores::Int32 NumberOfPointsPerPlane;
  viskores::Int32 NumberOfPlanes;
  viskores::Id NumberOfCells;
};


class ReverseConnectivityExtrude
{
private:
  using Int32HandleType = viskores::cont::ArrayHandle<viskores::Int32>;
  using Int32PortalType = typename Int32HandleType::ReadPortalType;

public:
  using ConnectivityPortalType = Int32PortalType;
  using OffsetsPortalType = Int32PortalType;
  using CountsPortalType = Int32PortalType;
  using PrevNodePortalType = Int32PortalType;

  using SchedulingRangeType = viskores::Id2;

  using CellShapeTag = viskores::CellShapeTagVertex;

  using IndicesType = ReverseIndicesExtrude<ConnectivityPortalType>;

  ReverseConnectivityExtrude() = default;

  VISKORES_EXEC
  ReverseConnectivityExtrude(const ConnectivityPortalType& conn,
                             const OffsetsPortalType& offsets,
                             const CountsPortalType& counts,
                             const PrevNodePortalType& prevNode,
                             viskores::Int32 cellsPerPlane,
                             viskores::Int32 pointsPerPlane,
                             viskores::Int32 numPlanes);

  VISKORES_EXEC
  viskores::Id GetNumberOfElements() const
  {
    return this->NumberOfPointsPerPlane * this->NumberOfPlanes;
  }

  VISKORES_EXEC
  CellShapeTag GetCellShape(viskores::Id) const { return viskores::CellShapeTagVertex(); }

  /// Returns a Vec-like object containing the indices for the given index.
  /// The object returned is not an actual array, but rather an object that
  /// loads the indices lazily out of the connectivity array. This prevents
  /// us from having to know the number of indices at compile time.
  ///
  VISKORES_EXEC
  IndicesType GetIndices(viskores::Id index) const
  {
    return this->GetIndices(this->FlatToLogicalToIndex(index));
  }

  VISKORES_EXEC
  IndicesType GetIndices(const viskores::Id2& index) const;

  template <typename IndexType>
  VISKORES_EXEC viskores::IdComponent GetNumberOfIndices(
    const IndexType& viskoresNotUsed(index)) const
  {
    return 1;
  }

  VISKORES_EXEC
  viskores::Id LogicalToFlatToIndex(const viskores::Id2& index) const
  {
    return index[0] + (index[1] * this->NumberOfPointsPerPlane);
  };

  VISKORES_EXEC
  viskores::Id2 FlatToLogicalToIndex(viskores::Id index) const
  {
    const viskores::Id vertId = index % this->NumberOfPointsPerPlane;
    const viskores::Id plane = index / this->NumberOfPointsPerPlane;
    return viskores::Id2(vertId, plane);
  }

  ConnectivityPortalType Connectivity;
  OffsetsPortalType Offsets;
  CountsPortalType Counts;
  PrevNodePortalType PrevNode;
  viskores::Int32 NumberOfCellsPerPlane;
  viskores::Int32 NumberOfPointsPerPlane;
  viskores::Int32 NumberOfPlanes;
};


inline ConnectivityExtrude::ConnectivityExtrude(const ConnectivityPortalType& conn,
                                                const ConnectivityPortalType& nextNode,
                                                viskores::Int32 cellsPerPlane,
                                                viskores::Int32 pointsPerPlane,
                                                viskores::Int32 numPlanes,
                                                bool periodic)
  : Connectivity(conn)
  , NextNode(nextNode)
  , NumberOfCellsPerPlane(cellsPerPlane)
  , NumberOfPointsPerPlane(pointsPerPlane)
  , NumberOfPlanes(numPlanes)
{
  this->NumberOfCells = periodic ? (static_cast<viskores::Id>(cellsPerPlane) * numPlanes)
                                 : (static_cast<viskores::Id>(cellsPerPlane) * (numPlanes - 1));
}

VISKORES_EXEC inline ConnectivityExtrude::IndicesType ConnectivityExtrude::GetIndices(
  const viskores::Id2& index) const
{
  viskores::Id tr = index[0];
  viskores::Id p0 = index[1];
  viskores::Id p1 = (p0 < (this->NumberOfPlanes - 1)) ? (p0 + 1) : 0;

  viskores::Vec3i_32 pointIds1, pointIds2;
  for (int i = 0; i < 3; ++i)
  {
    pointIds1[i] = this->Connectivity.Get((tr * 3) + i);
    pointIds2[i] = this->NextNode.Get(pointIds1[i]);
  }

  return IndicesType(pointIds1,
                     static_cast<viskores::Int32>(p0),
                     pointIds2,
                     static_cast<viskores::Int32>(p1),
                     this->NumberOfPointsPerPlane);
}


VISKORES_EXEC inline ReverseConnectivityExtrude::ReverseConnectivityExtrude(
  const ConnectivityPortalType& conn,
  const OffsetsPortalType& offsets,
  const CountsPortalType& counts,
  const PrevNodePortalType& prevNode,
  viskores::Int32 cellsPerPlane,
  viskores::Int32 pointsPerPlane,
  viskores::Int32 numPlanes)
  : Connectivity(conn)
  , Offsets(offsets)
  , Counts(counts)
  , PrevNode(prevNode)
  , NumberOfCellsPerPlane(cellsPerPlane)
  , NumberOfPointsPerPlane(pointsPerPlane)
  , NumberOfPlanes(numPlanes)
{
}

VISKORES_EXEC inline ReverseConnectivityExtrude::IndicesType ReverseConnectivityExtrude::GetIndices(
  const viskores::Id2& index) const
{
  auto ptCur = index[0];
  auto ptPre = this->PrevNode.Get(ptCur);
  auto plCur = index[1];
  auto plPre = (plCur == 0) ? (this->NumberOfPlanes - 1) : (plCur - 1);

  return IndicesType(this->Connectivity,
                     this->Offsets.Get(ptPre),
                     this->Counts.Get(ptPre),
                     this->Offsets.Get(ptCur),
                     this->Counts.Get(ptCur),
                     static_cast<viskores::IdComponent>(plPre),
                     static_cast<viskores::IdComponent>(plCur),
                     this->NumberOfCellsPerPlane);
}
}
} // namespace viskores::exec
#endif
