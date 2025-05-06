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

#ifndef viskores_exec_ConnectivityPermuted_h
#define viskores_exec_ConnectivityPermuted_h

#include <viskores/CellShape.h>
#include <viskores/TopologyElementTag.h>
#include <viskores/Types.h>
#include <viskores/VecFromPortal.h>

namespace viskores
{
namespace exec
{

template <typename PermutationPortal, typename OriginalConnectivity>
class ConnectivityPermutedVisitCellsWithPoints
{
public:
  using SchedulingRangeType = typename OriginalConnectivity::SchedulingRangeType;

  ConnectivityPermutedVisitCellsWithPoints() = default;

  VISKORES_EXEC_CONT
  ConnectivityPermutedVisitCellsWithPoints(const PermutationPortal& portal,
                                           const OriginalConnectivity& src)
    : Portal(portal)
    , Connectivity(src)
  {
  }

  ConnectivityPermutedVisitCellsWithPoints(const ConnectivityPermutedVisitCellsWithPoints& src) =
    default;

  ConnectivityPermutedVisitCellsWithPoints& operator=(
    const ConnectivityPermutedVisitCellsWithPoints& src) = default;
  ConnectivityPermutedVisitCellsWithPoints& operator=(
    ConnectivityPermutedVisitCellsWithPoints&& src) = default;

  VISKORES_EXEC
  viskores::Id GetNumberOfElements() const { return this->Portal.GetNumberOfValues(); }

  using CellShapeTag = typename OriginalConnectivity::CellShapeTag;

  VISKORES_EXEC
  CellShapeTag GetCellShape(viskores::Id index) const
  {
    viskores::Id pIndex = this->Portal.Get(index);
    return this->Connectivity.GetCellShape(pIndex);
  }

  VISKORES_EXEC
  viskores::IdComponent GetNumberOfIndices(viskores::Id index) const
  {
    return this->Connectivity.GetNumberOfIndices(this->Portal.Get(index));
  }

  using IndicesType = typename OriginalConnectivity::IndicesType;

  template <typename IndexType>
  VISKORES_EXEC IndicesType GetIndices(const IndexType& index) const
  {
    return this->Connectivity.GetIndices(this->Portal.Get(index));
  }

  PermutationPortal Portal;
  OriginalConnectivity Connectivity;
};

template <typename ConnectivityPortalType, typename OffsetPortalType>
class ConnectivityPermutedVisitPointsWithCells
{
public:
  using SchedulingRangeType = viskores::Id;
  using IndicesType = viskores::VecFromPortal<ConnectivityPortalType>;
  using CellShapeTag = viskores::CellShapeTagVertex;

  ConnectivityPermutedVisitPointsWithCells() = default;

  ConnectivityPermutedVisitPointsWithCells(const ConnectivityPortalType& connectivity,
                                           const OffsetPortalType& offsets)
    : Connectivity(connectivity)
    , Offsets(offsets)
  {
  }

  VISKORES_EXEC
  SchedulingRangeType GetNumberOfElements() const { return this->Offsets.GetNumberOfValues() - 1; }

  VISKORES_EXEC CellShapeTag GetCellShape(viskores::Id) const { return CellShapeTag(); }

  VISKORES_EXEC
  viskores::IdComponent GetNumberOfIndices(viskores::Id index) const
  {
    const viskores::Id offBegin = this->Offsets.Get(index);
    const viskores::Id offEnd = this->Offsets.Get(index + 1);
    return static_cast<viskores::IdComponent>(offEnd - offBegin);
  }

  VISKORES_EXEC IndicesType GetIndices(viskores::Id index) const
  {
    const viskores::Id offBegin = this->Offsets.Get(index);
    const viskores::Id offEnd = this->Offsets.Get(index + 1);
    return IndicesType(
      this->Connectivity, static_cast<viskores::IdComponent>(offEnd - offBegin), offBegin);
  }

private:
  ConnectivityPortalType Connectivity;
  OffsetPortalType Offsets;
};
}
} // namespace viskores::exec

#endif //viskores_exec_ConnectivityPermuted_h
