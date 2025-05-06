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

#ifndef viskores_exec_ConnectivityStructured_h
#define viskores_exec_ConnectivityStructured_h

#include <viskores/Deprecated.h>
#include <viskores/TopologyElementTag.h>
#include <viskores/Types.h>
#include <viskores/internal/ConnectivityStructuredInternals.h>

namespace viskores
{
namespace exec
{

/// @brief A class holding information about topology connections.
///
/// An object of `ConnectivityStructured` is provided to a worklet when the
/// `ControlSignature` argument is `WholeCellSetIn` and the `viskores::cont::CellSet`
/// provided is a `viskores::cont::CellSetStructured`.
template <typename VisitTopology, typename IncidentTopology, viskores::IdComponent Dimension>
class ConnectivityStructured
{
  VISKORES_IS_TOPOLOGY_ELEMENT_TAG(VisitTopology);
  VISKORES_IS_TOPOLOGY_ELEMENT_TAG(IncidentTopology);

  using InternalsType = viskores::internal::ConnectivityStructuredInternals<Dimension>;

  using Helper = viskores::internal::
    ConnectivityStructuredIndexHelper<VisitTopology, IncidentTopology, Dimension>;

public:
  using SchedulingRangeType = typename InternalsType::SchedulingRangeType;

  ConnectivityStructured() = default;

  VISKORES_EXEC_CONT
  ConnectivityStructured(const InternalsType& src)
    : Internals(src)
  {
  }

  ConnectivityStructured(const ConnectivityStructured& src) = default;

  VISKORES_EXEC_CONT
  ConnectivityStructured(
    const ConnectivityStructured<IncidentTopology, VisitTopology, Dimension>& src)
    : Internals(src.Internals)
  {
  }


  ConnectivityStructured& operator=(const ConnectivityStructured& src) = default;
  ConnectivityStructured& operator=(ConnectivityStructured&& src) = default;


  /// @brief Provides the number of elements in the topology.
  ///
  /// This number of elements is associated with the "visit" type of topology element,
  /// which is the first template argument to `WholeCellSetIn`. The number of elements
  /// defines the valid indices for the other methods of this class.
  VISKORES_EXEC
  viskores::Id GetNumberOfElements() const { return Helper::GetNumberOfElements(this->Internals); }

  /// @brief The tag representing the cell shape of the visited elements.
  ///
  /// If the "visit" element is cells, then the returned tag is `viskores::CellShapeTagHexahedron`
  /// for a 3D structured grid, `viskores::CellShapeTagQuad` for a 2D structured grid, or
  /// `viskores::CellShapeLine` for a 1D structured grid.
  using CellShapeTag = typename Helper::CellShapeTag;

  /// @brief Returns a tag for the cell shape associated with the element at the given index.
  ///
  /// If the "visit" element is cells, then the returned tag is `viskores::CellShapeTagHexahedron`
  /// for a 3D structured grid, `viskores::CellShapeTagQuad` for a 2D structured grid, or
  /// `viskores::CellShapeLine` for a 1D structured grid.
  VISKORES_EXEC
  CellShapeTag GetCellShape(viskores::Id) const { return CellShapeTag(); }

  /// Given the index of a visited element, returns the number of incident elements
  /// touching it.
  template <typename IndexType>
  VISKORES_EXEC viskores::IdComponent GetNumberOfIndices(const IndexType& index) const
  {
    return Helper::GetNumberOfIndices(this->Internals, index);
  }

  /// @brief Type of variable that lists of incident indices will be put into.
  using IndicesType = typename Helper::IndicesType;

  /// Provides the indices of all elements incident to the visit element of the provided
  /// index.
  template <typename IndexType>
  VISKORES_EXEC IndicesType GetIndices(const IndexType& index) const
  {
    return Helper::GetIndices(this->Internals, index);
  }

  /// Convenience method that converts a flat, 1D index to the visited elements to a `viskores::Vec`
  /// containing the logical indices in the grid.
  VISKORES_EXEC_CONT SchedulingRangeType FlatToLogicalVisitIndex(viskores::Id flatVisitIndex) const
  {
    return Helper::FlatToLogicalVisitIndex(this->Internals, flatVisitIndex);
  }

  /// Convenience method that converts a flat, 1D index to the incident elements to a `viskores::Vec`
  /// containing the logical indices in the grid.
  VISKORES_EXEC_CONT SchedulingRangeType
  FlatToLogicalIncidentIndex(viskores::Id flatIncidentIndex) const
  {
    return Helper::FlatToLogicalIncidentIndex(this->Internals, flatIncidentIndex);
  }

  /// Convenience method that converts logical indices in a `viskores::Vec` of a visited element
  /// to a flat, 1D index.
  VISKORES_EXEC_CONT viskores::Id LogicalToFlatVisitIndex(
    const SchedulingRangeType& logicalVisitIndex) const
  {
    return Helper::LogicalToFlatVisitIndex(this->Internals, logicalVisitIndex);
  }

  /// Convenience method that converts logical indices in a `viskores::Vec` of an incident element
  /// to a flat, 1D index.
  VISKORES_EXEC_CONT viskores::Id LogicalToFlatIncidentIndex(
    const SchedulingRangeType& logicalIncidentIndex) const
  {
    return Helper::LogicalToFlatIncidentIndex(this->Internals, logicalIncidentIndex);
  }

  VISKORES_EXEC_CONT
  VISKORES_DEPRECATED(2.1, "Use FlatToLogicalIncidentIndex.")
  SchedulingRangeType FlatToLogicalFromIndex(viskores::Id flatFromIndex) const
  {
    return this->FlatToLogicalIncidentIndex(flatFromIndex);
  }

  VISKORES_EXEC_CONT
  VISKORES_DEPRECATED(2.1, "Use LogicalToFlatIncidentIndex.")
  viskores::Id LogicalToFlatFromIndex(const SchedulingRangeType& logicalFromIndex) const
  {
    return this->LogicalToFlatIncidentIndex(logicalFromIndex);
  }

  VISKORES_EXEC_CONT
  VISKORES_DEPRECATED(2.1, "Use FlatToLogicalVisitIndex.")
  SchedulingRangeType FlatToLogicalToIndex(viskores::Id flatToIndex) const
  {
    return this->FlatToLogicalVisitIndex(flatToIndex);
  }

  VISKORES_EXEC_CONT
  VISKORES_DEPRECATED(2.1, "Use LogicalToFlatVisitIndex.")
  viskores::Id LogicalToFlatToIndex(const SchedulingRangeType& logicalToIndex) const
  {
    return this->LogicalToFlatVisitIndex(logicalToIndex);
  }

  /// Return the dimensions of the points in the cell set.
  VISKORES_EXEC_CONT
  viskores::Vec<viskores::Id, Dimension> GetPointDimensions() const
  {
    return this->Internals.GetPointDimensions();
  }

  /// Return the dimensions of the points in the cell set.
  VISKORES_EXEC_CONT
  viskores::Vec<viskores::Id, Dimension> GetCellDimensions() const
  {
    return this->Internals.GetCellDimensions();
  }

  VISKORES_EXEC_CONT
  SchedulingRangeType GetGlobalPointIndexStart() const
  {
    return this->Internals.GetGlobalPointIndexStart();
  }

  friend class ConnectivityStructured<IncidentTopology, VisitTopology, Dimension>;

private:
  InternalsType Internals;
};
}
} // namespace viskores::exec

#endif //viskores_exec_ConnectivityStructured_h
