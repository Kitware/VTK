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
#ifndef viskores_exec_arg_ThreadIndicesExtrude_h
#define viskores_exec_arg_ThreadIndicesExtrude_h

#include <viskores/exec/ConnectivityExtrude.h>
#include <viskores/exec/arg/ThreadIndicesTopologyMap.h>

namespace viskores
{
namespace exec
{
namespace arg
{

// Specialization for extrude types.
template <typename ScatterAndMaskMode>
class ThreadIndicesTopologyMap<viskores::exec::ConnectivityExtrude, ScatterAndMaskMode>
{

  using ConnectivityType = viskores::exec::ConnectivityExtrude;

public:
  using CellShapeTag = typename ConnectivityType::CellShapeTag;
  using IndicesIncidentType = typename ConnectivityType::IndicesType;
  using LogicalIndexType = typename ConnectivityType::SchedulingRangeType;
  using Connectivity = viskores::exec::ConnectivityExtrude;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC ThreadIndicesTopologyMap(viskores::Id threadIndex,
                                         viskores::Id inputIndex,
                                         viskores::IdComponent visitIndex,
                                         viskores::Id outputIndex,
                                         const ConnectivityType& connectivity)
  {
    const LogicalIndexType logicalIndex = connectivity.FlatToLogicalToIndex(inputIndex);

    this->ThreadIndex = threadIndex;
    this->InputIndex = inputIndex;
    this->OutputIndex = outputIndex;
    this->VisitIndex = visitIndex;
    this->LogicalIndex = logicalIndex;
    this->IndicesIncident = connectivity.GetIndices(logicalIndex);
    //this->CellShape = connectivity.GetCellShape(index);
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  ThreadIndicesTopologyMap(const viskores::Id3& threadIndex3D,
                           viskores::Id threadIndex1D,
                           const ConnectivityType& connectivity)
  {
    // This constructor handles multidimensional indices on one-to-one input-to-output
    auto logicalIndex = detail::Deflate(threadIndex3D, LogicalIndexType());

    this->ThreadIndex = threadIndex1D;
    this->InputIndex = threadIndex1D;
    this->OutputIndex = threadIndex1D;
    this->VisitIndex = 0;
    this->LogicalIndex = logicalIndex;
    this->IndicesIncident = connectivity.GetIndices(logicalIndex);
    //this->CellShape = connectivity.GetCellShape(index);
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  ThreadIndicesTopologyMap(const viskores::Id3& threadIndex3D,
                           viskores::Id threadIndex1D,
                           viskores::Id inputIndex,
                           viskores::IdComponent visitIndex,
                           viskores::Id outputIndex,
                           const ConnectivityType& connectivity)
  {
    // This constructor handles multidimensional indices on many-to-many input-to-output
    auto logicalIndex = detail::Deflate(threadIndex3D, LogicalIndexType());

    this->ThreadIndex = threadIndex1D;
    this->InputIndex = inputIndex;
    this->OutputIndex = outputIndex;
    this->VisitIndex = visitIndex;
    this->LogicalIndex = logicalIndex;
    this->IndicesIncident = connectivity.GetIndices(logicalIndex);
    //this->CellShape = connectivity.GetCellShape(index);
  }

  /// \brief The index of the thread or work invocation.
  ///
  /// This index refers to which instance of the worklet is being invoked. Every invocation of the
  /// worklet has a unique thread index. This is also called the work index depending on the
  /// context.
  ///
  VISKORES_EXEC
  viskores::Id GetThreadIndex() const { return this->ThreadIndex; }

  /// \brief The logical index into the input domain.
  ///
  /// This is similar to \c GetIndex3D except the Vec size matches the actual
  /// dimensions of the data.
  ///
  VISKORES_EXEC
  LogicalIndexType GetIndexLogical() const { return this->LogicalIndex; }

  /// \brief The index into the input domain.
  ///
  /// This index refers to the input element (array value, cell, etc.) that
  /// this thread is being invoked for. This is the typical index used during
  /// fetches.
  ///
  VISKORES_EXEC
  viskores::Id GetInputIndex() const { return this->InputIndex; }

  /// \brief The 3D index into the input domain.
  ///
  /// Overloads the implementation in the base class to return the 3D index
  /// for the input.
  ///
  VISKORES_EXEC
  viskores::Id3 GetInputIndex3D() const { return detail::InflateTo3D(this->GetIndexLogical()); }

  /// \brief The index into the output domain.
  ///
  /// This index refers to the output element (array value, cell, etc.) that
  /// this thread is creating. This is the typical index used during
  /// Fetch::Store.
  ///
  VISKORES_EXEC
  viskores::Id GetOutputIndex() const { return this->OutputIndex; }

  /// \brief The visit index.
  ///
  /// When multiple output indices have the same input index, they are
  /// distinguished using the visit index.
  ///
  VISKORES_EXEC
  viskores::IdComponent GetVisitIndex() const { return this->VisitIndex; }

  /// \brief The input indices of the "from" elements.
  ///
  /// A topology map has "from" and "to" elements (for example from points to
  /// cells). For each worklet invocation, there is exactly one "to" element,
  /// but can be several "from" element. This method returns a Vec-like object
  /// containing the indices to the "from" elements.
  ///
  VISKORES_EXEC
  const IndicesIncidentType& GetIndicesIncident() const { return this->IndicesIncident; }

  /// \brief The input indices of the "from" elements in pointer form.
  ///
  /// Returns the same object as GetIndicesFrom except that it returns a
  /// pointer to the internally held object rather than a reference or copy.
  /// Since the from indices can be a sizeable Vec (8 entries is common), it is
  /// best not to have a bunch a copies. Thus, you can pass around a pointer
  /// instead. However, care should be taken to make sure that this object does
  /// not go out of scope, at which time the returned pointer becomes invalid.
  ///
  VISKORES_EXEC
  const IndicesIncidentType* GetIndicesIncidentPointer() const { return &this->IndicesIncident; }

  /// \brief The shape of the input cell.
  ///
  /// In topology maps that map from points to something, the indices make up
  /// the structure of a cell. Although the shape tag is not technically and
  /// index, it defines the meaning of the indices, so we put it here. (That
  /// and this class is the only convenient place to store it.)
  ///
  VISKORES_EXEC
  CellShapeTag GetCellShape() const { return viskores::CellShapeTagWedge{}; }

private:
  viskores::Id ThreadIndex;
  viskores::Id InputIndex;
  viskores::IdComponent VisitIndex;
  viskores::Id OutputIndex;
  LogicalIndexType LogicalIndex;
  IndicesIncidentType IndicesIncident;
};

// Specialization for extrude types.
template <typename ScatterAndMaskMode>
class ThreadIndicesTopologyMap<viskores::exec::ReverseConnectivityExtrude, ScatterAndMaskMode>
{
  using ConnectivityType = viskores::exec::ReverseConnectivityExtrude;

public:
  using CellShapeTag = typename ConnectivityType::CellShapeTag;
  using IndicesIncidentType = typename ConnectivityType::IndicesType;
  using LogicalIndexType = typename ConnectivityType::SchedulingRangeType;
  using Connectivity = ConnectivityType;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  ThreadIndicesTopologyMap(viskores::Id& threadIndex,
                           viskores::Id inputIndex,
                           viskores::IdComponent visitIndex,
                           viskores::Id outputIndex,
                           const ConnectivityType& connectivity)
  {
    const LogicalIndexType logicalIndex = connectivity.FlatToLogicalToIndex(inputIndex);

    this->ThreadIndex = threadIndex;
    this->InputIndex = inputIndex;
    this->OutputIndex = outputIndex;
    this->VisitIndex = visitIndex;
    this->LogicalIndex = logicalIndex;
    this->IndicesIncident = connectivity.GetIndices(logicalIndex);
  }

  VISKORES_EXEC
  ThreadIndicesTopologyMap(const viskores::Id3& threadIndex3D,
                           viskores::Id threadIndex1D,
                           const ConnectivityType& connectivity)
  {

    const LogicalIndexType logicalIndex = detail::Deflate(threadIndex3D, LogicalIndexType());

    this->ThreadIndex = threadIndex1D;
    this->InputIndex = threadIndex1D;
    this->OutputIndex = threadIndex1D;
    this->VisitIndex = 0;
    this->LogicalIndex = logicalIndex;
    this->IndicesIncident = connectivity.GetIndices(logicalIndex);
  }

  VISKORES_EXEC
  ThreadIndicesTopologyMap(const viskores::Id3& threadIndex3D,
                           viskores::Id threadIndex1D,
                           viskores::Id inputIndex,
                           viskores::IdComponent visitIndex,
                           viskores::Id outputIndex,
                           const ConnectivityType& connectivity)
  {

    const LogicalIndexType logicalIndex = detail::Deflate(threadIndex3D, LogicalIndexType());

    this->ThreadIndex = threadIndex1D;
    this->InputIndex = inputIndex;
    this->OutputIndex = outputIndex;
    this->VisitIndex = visitIndex;
    this->LogicalIndex = logicalIndex;
    this->IndicesIncident = connectivity.GetIndices(logicalIndex);
  }

  /// \brief The index of the thread or work invocation.
  ///
  /// This index refers to which instance of the worklet is being invoked. Every invocation of the
  /// worklet has a unique thread index. This is also called the work index depending on the
  /// context.
  ///
  VISKORES_EXEC
  viskores::Id GetThreadIndex() const { return this->ThreadIndex; }

  /// \brief The logical index into the input domain.
  ///
  /// This is similar to \c GetIndex3D except the Vec size matches the actual
  /// dimensions of the data.
  ///
  VISKORES_EXEC
  LogicalIndexType GetIndexLogical() const { return this->LogicalIndex; }

  /// \brief The index into the input domain.
  ///
  /// This index refers to the input element (array value, cell, etc.) that
  /// this thread is being invoked for. This is the typical index used during
  /// fetches.
  ///
  VISKORES_EXEC
  viskores::Id GetInputIndex() const { return this->InputIndex; }

  /// \brief The 3D index into the input domain.
  ///
  /// Overloads the implementation in the base class to return the 3D index
  /// for the input.
  ///
  VISKORES_EXEC
  viskores::Id3 GetInputIndex3D() const { return detail::InflateTo3D(this->GetIndexLogical()); }

  /// \brief The index into the output domain.
  ///
  /// This index refers to the output element (array value, cell, etc.) that
  /// this thread is creating. This is the typical index used during
  /// Fetch::Store.
  ///
  VISKORES_EXEC
  viskores::Id GetOutputIndex() const { return this->OutputIndex; }

  /// \brief The visit index.
  ///
  /// When multiple output indices have the same input index, they are
  /// distinguished using the visit index.
  ///
  VISKORES_EXEC
  viskores::IdComponent GetVisitIndex() const { return this->VisitIndex; }

  /// \brief The input indices of the "from" elements.
  ///
  /// A topology map has "from" and "to" elements (for example from points to
  /// cells). For each worklet invocation, there is exactly one "to" element,
  /// but can be several "from" element. This method returns a Vec-like object
  /// containing the indices to the "from" elements.
  ///
  VISKORES_EXEC
  const IndicesIncidentType& GetIndicesIncident() const { return this->IndicesIncident; }

  /// \brief The input indices of the "from" elements in pointer form.
  ///
  /// Returns the same object as GetIndicesFrom except that it returns a
  /// pointer to the internally held object rather than a reference or copy.
  /// Since the from indices can be a sizeable Vec (8 entries is common), it is
  /// best not to have a bunch a copies. Thus, you can pass around a pointer
  /// instead. However, care should be taken to make sure that this object does
  /// not go out of scope, at which time the returned pointer becomes invalid.
  ///
  VISKORES_EXEC
  const IndicesIncidentType* GetIndicesIncidentPointer() const { return &this->IndicesIncident; }

  /// \brief The shape of the input cell.
  ///
  /// In topology maps that map from points to something, the indices make up
  /// the structure of a cell. Although the shape tag is not technically and
  /// index, it defines the meaning of the indices, so we put it here. (That
  /// and this class is the only convenient place to store it.)
  ///
  VISKORES_EXEC
  CellShapeTag GetCellShape() const { return viskores::CellShapeTagVertex{}; }

private:
  viskores::Id ThreadIndex;
  viskores::Id InputIndex;
  viskores::IdComponent VisitIndex;
  viskores::Id OutputIndex;
  LogicalIndexType LogicalIndex;
  IndicesIncidentType IndicesIncident;
};

} //namespace arg
}
} // namespace viskores::exec

#include <viskores/exec/arg/FetchExtrude.h>

#endif
