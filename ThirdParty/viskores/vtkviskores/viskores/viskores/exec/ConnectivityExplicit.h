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
#ifndef viskores_exec_ConnectivityExplicit_h
#define viskores_exec_ConnectivityExplicit_h

#include <viskores/CellShape.h>
#include <viskores/Types.h>

#include <viskores/VecFromPortal.h>

namespace viskores
{
namespace exec
{

/// @brief A class holding information about topology connections.
///
/// An object of `ConnectivityExplicit` is provided to a worklet when the
/// `ControlSignature` argument is `WholeCellSetIn` and the `viskores::cont::CellSet`
/// provided is a `viskores::cont::CellSetExplicit`.
template <typename ShapesPortalType, typename ConnectivityPortalType, typename OffsetsPortalType>
class ConnectivityExplicit
{
public:
  using SchedulingRangeType = viskores::Id;

  ConnectivityExplicit() {}

  ConnectivityExplicit(const ShapesPortalType& shapesPortal,
                       const ConnectivityPortalType& connPortal,
                       const OffsetsPortalType& offsetsPortal)
    : Shapes(shapesPortal)
    , Connectivity(connPortal)
    , Offsets(offsetsPortal)
  {
  }

  /// @brief Provides the number of elements in the topology.
  ///
  /// This number of elements is associated with the "visit" type of topology element,
  /// which is the first template argument to `WholeCellSetIn`. The number of elements
  /// defines the valid indices for the other methods of this class.
  VISKORES_EXEC
  SchedulingRangeType GetNumberOfElements() const { return this->Shapes.GetNumberOfValues(); }

  /// @brief The tag representing the cell shape of the visited elements.
  ///
  /// The tag type is allways `viskores::CellShapeTagGeneric` and its id is filled with the
  /// identifier for the appropriate shape.
  using CellShapeTag = viskores::CellShapeTagGeneric;

  /// @brief Returns a tagfor the cell shape associated with the element at the given index.
  ///
  /// The tag type is allways `viskores::CellShapeTagGeneric` and its id is filled with the
  /// identifier for the appropriate shape.
  VISKORES_EXEC
  CellShapeTag GetCellShape(viskores::Id index) const
  {
    return CellShapeTag(this->Shapes.Get(index));
  }

  /// Given the index of a visited element, returns the number of incident elements
  /// touching it.
  VISKORES_EXEC
  viskores::IdComponent GetNumberOfIndices(viskores::Id index) const
  {
    return static_cast<viskores::IdComponent>(this->Offsets.Get(index + 1) -
                                              this->Offsets.Get(index));
  }

  /// @brief Type of variable that lists of incident indices will be put into.
  using IndicesType = viskores::VecFromPortal<ConnectivityPortalType>;

  /// Provides the indices of all elements incident to the visit element of the provided
  /// index.
  /// Returns a Vec-like object containing the indices for the given index.
  /// The object returned is not an actual array, but rather an object that
  /// loads the indices lazily out of the connectivity array. This prevents
  /// us from having to know the number of indices at compile time.
  ///
  VISKORES_EXEC
  IndicesType GetIndices(viskores::Id index) const
  {
    const viskores::Id offset = this->Offsets.Get(index);
    const viskores::Id endOffset = this->Offsets.Get(index + 1);
    const auto length = static_cast<viskores::IdComponent>(endOffset - offset);

    return IndicesType(this->Connectivity, length, offset);
  }

private:
  ShapesPortalType Shapes;
  ConnectivityPortalType Connectivity;
  OffsetsPortalType Offsets;
};

} // namespace exec
} // namespace viskores

#endif //  viskores_exec_ConnectivityExplicit_h
