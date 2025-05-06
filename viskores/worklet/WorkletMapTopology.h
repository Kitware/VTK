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
#ifndef viskores_worklet_WorkletMapTopology_h
#define viskores_worklet_WorkletMapTopology_h

#include <viskores/worklet/internal/WorkletBase.h>

#include <viskores/TopologyElementTag.h>

#include <viskores/cont/arg/ControlSignatureTagBase.h>
#include <viskores/cont/arg/TransportTagArrayInOut.h>
#include <viskores/cont/arg/TransportTagArrayOut.h>
#include <viskores/cont/arg/TransportTagCellSetIn.h>
#include <viskores/cont/arg/TransportTagTopologyFieldIn.h>
#include <viskores/cont/arg/TypeCheckTagArrayIn.h>
#include <viskores/cont/arg/TypeCheckTagArrayInOut.h>
#include <viskores/cont/arg/TypeCheckTagArrayOut.h>
#include <viskores/cont/arg/TypeCheckTagCellSet.h>

#include <viskores/exec/arg/CellShape.h>
#include <viskores/exec/arg/FetchTagArrayDirectIn.h>
#include <viskores/exec/arg/FetchTagArrayDirectInOut.h>
#include <viskores/exec/arg/FetchTagArrayDirectOut.h>
#include <viskores/exec/arg/FetchTagArrayTopologyMapIn.h>
#include <viskores/exec/arg/FetchTagCellSetIn.h>
#include <viskores/exec/arg/IncidentElementCount.h>
#include <viskores/exec/arg/IncidentElementIndices.h>
#include <viskores/exec/arg/ThreadIndicesTopologyMap.h>

#include <viskores/worklet/DispatcherMapTopology.h>

namespace viskores
{
namespace worklet
{

template <typename WorkletType>
class DispatcherMapTopology;

namespace detail
{

struct WorkletMapTopologyBase : viskores::worklet::internal::WorkletBase
{
  template <typename Worklet>
  using Dispatcher = viskores::worklet::DispatcherMapTopology<Worklet>;
};

} // namespace detail

/// @brief Base class for worklets that map topology elements onto each other.
///
/// The template parameters for this class must be members of the
/// TopologyElementTag group. The VisitTopology indicates the elements of a
/// cellset that will be visited, and the IncidentTopology will be mapped onto
/// the VisitTopology.
///
/// For instance,
/// `WorkletMapTopology<TopologyElementTagPoint, TopologyElementCell>` will
/// execute one instance per point, and provides convenience methods for
/// gathering information about the cells incident to the current point.
///
template <typename VisitTopology, typename IncidentTopology>
class WorkletMapTopology : public detail::WorkletMapTopologyBase
{
public:
  using VisitTopologyType = VisitTopology;
  using IncidentTopologyType = IncidentTopology;

  /// \brief A control signature tag for input fields from the \em visited
  /// topology.
  ///
  struct FieldInVisit : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayIn;
    using TransportTag = viskores::cont::arg::TransportTagTopologyFieldIn<VisitTopologyType>;
    using FetchTag = viskores::exec::arg::FetchTagArrayDirectIn;
  };

  /// \brief A control signature tag for input fields from the \em incident
  /// topology.
  ///
  struct FieldInIncident : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayIn;
    using TransportTag = viskores::cont::arg::TransportTagTopologyFieldIn<IncidentTopologyType>;
    using FetchTag = viskores::exec::arg::FetchTagArrayTopologyMapIn;
  };

  /// \brief A control signature tag for output fields.
  ///
  struct FieldOut : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayOut;
    using TransportTag = viskores::cont::arg::TransportTagArrayOut;
    using FetchTag = viskores::exec::arg::FetchTagArrayDirectOut;
  };

  /// \brief A control signature tag for input-output (in-place) fields from
  /// the visited topology.
  ///
  struct FieldInOut : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayInOut;
    using TransportTag = viskores::cont::arg::TransportTagArrayInOut;
    using FetchTag = viskores::exec::arg::FetchTagArrayDirectInOut;
  };

  /// @brief A control signature tag for input connectivity.
  ///
  /// The associated parameter of the invoke should be a subclass of `viskores::cont::CellSet`.
  ///
  /// There should be exactly one `CellSetIn` argument in the `ControlSignature`,
  /// and the `InputDomain` must point to it.
  struct CellSetIn : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagCellSet;
    using TransportTag =
      viskores::cont::arg::TransportTagCellSetIn<VisitTopologyType, IncidentTopologyType>;
    using FetchTag = viskores::exec::arg::FetchTagCellSetIn;
  };

  /// \brief An execution signature tag for getting the cell shape. This only
  /// makes sense when visiting cell topologies.
  ///
  struct CellShape : viskores::exec::arg::CellShape
  {
  };

  /// \brief An execution signature tag to get the number of \em incident
  /// elements.
  ///
  /// In a topology map, there are \em visited and \em incident topology
  /// elements specified. The scheduling occurs on the \em visited elements,
  /// and for each \em visited element there is some number of incident \em
  /// mapped elements that are accessible. This \c ExecutionSignature tag
  /// provides the number of these \em mapped elements that are accessible.
  ///
  struct IncidentElementCount : viskores::exec::arg::IncidentElementCount
  {
  };

  /// \brief An execution signature tag to get the indices of from elements.
  ///
  /// In a topology map, there are \em visited and \em incident topology
  /// elements specified. The scheduling occurs on the \em visited elements,
  /// and for each \em visited element there is some number of incident \em
  /// mapped elements that are accessible. This \c ExecutionSignature tag
  /// provides the indices of the \em mapped elements that are incident to the
  /// current \em visited element.
  ///
  struct IncidentElementIndices : viskores::exec::arg::IncidentElementIndices
  {
  };

  /// Topology map worklets use topology map indices.
  ///
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType>
  VISKORES_EXEC
    viskores::exec::arg::ThreadIndicesTopologyMap<InputDomainType,
                                                  viskores::exec::arg::CustomScatterOrMaskTag>
    GetThreadIndices(viskores::Id threadIndex,
                     const OutToInArrayType& outToIn,
                     const VisitArrayType& visit,
                     const ThreadToOutArrayType& threadToOut,
                     const InputDomainType& connectivity) const
  {
    const viskores::Id outIndex = threadToOut.Get(threadIndex);
    return viskores::exec::arg::
      ThreadIndicesTopologyMap<InputDomainType, viskores::exec::arg::CustomScatterOrMaskTag>(
        threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex, connectivity);
  }

  /// In the remaining methods and `constexpr` we determine at compilation time
  /// which method definition will be actually used for GetThreadIndices.
  ///
  /// We want to avoid further function calls when we use WorkletMapTopology in which
  /// ScatterType is set as ScatterIdentity and MaskType as MaskNone.
  /// Otherwise, we call the default method defined at the bottom of this class.
private:
  static constexpr bool IsScatterIdentity =
    std::is_same<ScatterType, viskores::worklet::ScatterIdentity>::value;
  static constexpr bool IsMaskNone = std::is_same<MaskType, viskores::worklet::MaskNone>::value;

  template <bool Cond, typename ReturnType>
  using EnableFnWhen = typename std::enable_if<Cond, ReturnType>::type;

public:
  /// Optimized for ScatterIdentity and MaskNone
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType,
            bool S = IsScatterIdentity,
            bool M = IsMaskNone>
  VISKORES_EXEC EnableFnWhen<
    S && M,
    viskores::exec::arg::ThreadIndicesTopologyMap<InputDomainType,
                                                  viskores::exec::arg::DefaultScatterAndMaskTag>>
  GetThreadIndices(viskores::Id threadIndex1D,
                   const viskores::Id3& threadIndex3D,
                   const OutToInArrayType& viskoresNotUsed(outToIn),
                   const VisitArrayType& viskoresNotUsed(visit),
                   const ThreadToOutArrayType& viskoresNotUsed(threadToOut),
                   const InputDomainType& connectivity) const
  {
    return viskores::exec::arg::
      ThreadIndicesTopologyMap<InputDomainType, viskores::exec::arg::DefaultScatterAndMaskTag>(
        threadIndex3D, threadIndex1D, connectivity);
  }

  /// Default version
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType,
            bool S = IsScatterIdentity,
            bool M = IsMaskNone>
  VISKORES_EXEC EnableFnWhen<
    !(S && M),
    viskores::exec::arg::ThreadIndicesTopologyMap<InputDomainType,
                                                  viskores::exec::arg::CustomScatterOrMaskTag>>
  GetThreadIndices(viskores::Id threadIndex1D,
                   const viskores::Id3& threadIndex3D,
                   const OutToInArrayType& outToIn,
                   const VisitArrayType& visit,
                   const ThreadToOutArrayType& threadToOut,
                   const InputDomainType& connectivity) const
  {
    const viskores::Id outIndex = threadToOut.Get(threadIndex1D);
    return viskores::exec::arg::
      ThreadIndicesTopologyMap<InputDomainType, viskores::exec::arg::CustomScatterOrMaskTag>(
        threadIndex3D,
        threadIndex1D,
        outToIn.Get(outIndex),
        visit.Get(outIndex),
        outIndex,
        connectivity);
  }
};

/// Base class for worklets that map from Points to Cells.
///
class WorkletVisitCellsWithPoints
  : public WorkletMapTopology<viskores::TopologyElementTagCell, viskores::TopologyElementTagPoint>
{
public:
#ifndef VISKORES_DOXYGEN_ONLY
  using FieldInPoint = FieldInIncident;

  using FieldInCell = FieldInVisit;

  using FieldOutCell = FieldOut;

  using FieldInOutCell = FieldInOut;

  using PointCount = IncidentElementCount;

  using PointIndices = IncidentElementIndices;
#else  // VISKORES_DOXYGEN_ONLY
  // These redeclarations of superclass features are for documentation purposes only.

  /// @defgroup WorkletVisitCellsWithPointsControlSigTags `ControlSignature` tags
  /// Tags that can be used in the `ControlSignature` of a `WorkletVisitCellsWithPoints`.
  /// @{

  /// @copydoc viskores::worklet::WorkletMapTopology::CellSetIn
  struct CellSetIn
    : viskores::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::CellSetIn
  {
  };

  /// @brief A control signature tag for input fields on the cells of the topology.
  ///
  /// The associated parameter of the invoke should be a `viskores::cont::ArrayHandle` that has
  /// the same number of values as the cells of the provided `CellSet`.
  /// The worklet gets a single value that is the field at that cell.
  struct FieldInCell : FieldInVisit
  {
  };

  /// @brief A control signature tag for input fields on the points of the topology.
  ///
  /// The associated parameter of the invoke should be a `viskores::cont::ArrayHandle` that has
  /// the same number of values as the points of the provided `CellSet`.
  /// The worklet gets a Vec-like object containing the field values on all incident points.
  struct FieldInPoint : FieldInIncident
  {
  };

  /// @brief A control signature tag for input fields from the visited topology.
  ///
  /// For `WorkletVisitCellsWithPoints`, this is the same as `FieldInCell`.
  struct FieldInVisit
    : viskores::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::FieldInVisit
  {
  };

  /// @brief A control signature tag for input fields from the incident topology.
  ///
  /// For `WorkletVisitCellsWithPoints`, this is the same as `FieldInPoint`.
  struct FieldInIncident
    : viskores::worklet::WorkletMapTopology<VisitTopologyType,
                                            IncidentTopologyType>::FieldInIncident
  {
  };

  /// @brief A control signature tag for output fields.
  ///
  /// A `WorkletVisitCellsWithPoints` always has the output on the cells of the topology.
  /// The associated parameter of the invoke should be a `viskores::cont::ArrayHandle`, and it will
  /// be resized to the number of cells in the provided `CellSet`.
  struct FieldOutCell : FieldOut
  {
  };

  /// @copydoc FieldOutCell
  struct FieldOut
    : viskores::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::FieldOut
  {
  };

  /// @brief A control signature tag for input-output (in-place) fields.
  ///
  /// A `WorkletVisitCellsWithPoints` always has the output on the cells of the topology.
  /// The associated parameter of the invoke should be a `viskores::cont::ArrayHandle`, and it must
  /// have the same number of values as the number of cells of the topology.
  struct FieldInOutCell : FieldInOut
  {
  };

  /// @copydoc FieldInOutCell
  struct FieldInOut
    : viskores::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::FieldInOut
  {
  };

  /// @copydoc viskores::worklet::internal::WorkletBase::WholeArrayIn
  struct WholeArrayIn : viskores::worklet::internal::WorkletBase::WholeArrayIn
  {
  };

  /// @copydoc viskores::worklet::internal::WorkletBase::WholeArrayOut
  struct WholeArrayOut : viskores::worklet::internal::WorkletBase::WholeArrayOut
  {
  };

  /// @copydoc viskores::worklet::internal::WorkletBase::WholeArrayInOut
  struct WholeArrayInOut : viskores::worklet::internal::WorkletBase::WholeArrayInOut
  {
  };

  /// @copydoc viskores::worklet::internal::WorkletBase::AtomicArrayInOut
  struct AtomicArrayInOut : viskores::worklet::internal::WorkletBase::AtomicArrayInOut
  {
  };

  /// @copydoc viskores::worklet::internal::WorkletBase::WholeCellSetIn
  template <typename VisitTopology = Cell, typename IncidentTopology = Point>
  struct WholeCellSetIn
    : viskores::worklet::internal::WorkletBase::WholeCellSetIn<VisitTopology, IncidentTopology>
  {
  };

  /// @copydoc viskores::worklet::internal::WorkletBase::ExecObject
  struct ExecObject : viskores::worklet::internal::WorkletBase::ExecObject
  {
  };

  /// @}

  /// @defgroup WorkletVisitCellsWithPointsExecutionSigTags `ExecutionSignature` tags
  /// Tags that can be used in the `ExecutionSignature` of a `WorkletVisitCellsWithPoints`.
  /// @{

  /// @copydoc viskores::placeholders::Arg
  struct _1 : viskores::worklet::internal::WorkletBase::_1
  {
  };

  /// @brief An execution signature tag to get the shape of the visited cell.
  ///
  /// This tag causes a `viskores::UInt8` to be passed to the worklet containing containing an
  /// id for the shape of the cell being visited.
  struct CellShape
    : viskores::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::CellShape
  {
  };

  /// @brief An execution signature tag to get the number of incident points.
  ///
  /// Each cell in a `viskores::cont::CellSet` can be incident on a number of points. This
  /// tag causes a `viskores::IdComponent` to be passed to the worklet containing the number
  /// of incident points.
  struct PointCount
    : viskores::worklet::WorkletMapTopology<VisitTopologyType,
                                            IncidentTopologyType>::IncidentElementCount
  {
  };

  /// @brief An execution signature tag to get the indices of the incident points.
  ///
  /// The indices will be provided in a Vec-like object containing `viskores::Id` indices for the
  /// cells in the data set.
  struct PointIndices
    : viskores::worklet::WorkletMapTopology<VisitTopologyType,
                                            IncidentTopologyType>::IncidentElementIndices
  {
  };

  /// @copydoc viskores::exec::arg::WorkIndex
  struct WorkIndex : viskores::worklet::internal::WorkletBase::WorkIndex
  {
  };

  /// @copydoc viskores::exec::arg::VisitIndex
  struct VisitIndex : viskores::worklet::internal::WorkletBase::VisitIndex
  {
  };

  /// @copydoc viskores::exec::arg::InputIndex
  struct InputIndex : viskores::worklet::internal::WorkletBase::InputIndex
  {
  };

  /// @copydoc viskores::exec::arg::OutputIndex
  struct OutputIndex : viskores::worklet::internal::WorkletBase::OutputIndex
  {
  };

  /// @copydoc viskores::exec::arg::ThreadIndices
  struct ThreadIndices : viskores::worklet::internal::WorkletBase::ThreadIndices
  {
  };

  /// @copydoc viskores::worklet::internal::WorkletBase::Device
  struct Device : viskores::worklet::internal::WorkletBase::Device
  {
  };

/// @}
#endif // VISKORES_DOXYGEN_ONLY
};

/// Base class for worklets that map from Cells to Points.
///
class WorkletVisitPointsWithCells
  : public WorkletMapTopology<viskores::TopologyElementTagPoint, viskores::TopologyElementTagCell>
{
public:
#ifndef VISKORES_DOXYGEN_ONLY
  using FieldInCell = FieldInIncident;

  using FieldInPoint = FieldInVisit;

  using FieldOutPoint = FieldOut;

  using FieldInOutPoint = FieldInOut;

  using CellCount = IncidentElementCount;

  using CellIndices = IncidentElementIndices;
#else  // VISKORES_DOXYGEN_ONLY
  // These redeclarations of superclass features are for documentation purposes only.

  /// @defgroup WorkletVisitPointsWithCellsControlSigTags `ControlSignature` tags
  /// Tags that can be used in the `ControlSignature` of a `WorkletVisitPointsWithCells`.
  /// @{

  /// @copydoc viskores::worklet::WorkletMapTopology::CellSetIn
  struct CellSetIn
    : viskores::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::CellSetIn
  {
  };

  /// @brief A control signature tag for input fields on the points of the topology.
  ///
  /// The associated parameter of the invoke should be a `viskores::cont::ArrayHandle` that has
  /// the same number of values as the points of the provided `CellSet`.
  /// The worklet gets a single value that is the field at that point.
  struct FieldInPoint : FieldInVisit
  {
  };

  /// @brief A control signature tag for input fields on the cells of the topology.
  ///
  /// The associated parameter of the invoke should be a `viskores::cont::ArrayHandle` that has
  /// the same number of values as the cells of the provided `CellSet`.
  /// The worklet gets a Vec-like object containing the field values on all incident cells.
  struct FieldInCell : FieldInIncident
  {
  };

  /// @brief A control signature tag for input fields from the visited topology.
  ///
  /// For `WorkletVisitPointsWithCells`, this is the same as `FieldInPoint`.
  struct FieldInVisit
    : viskores::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::FieldInVisit
  {
  };

  /// @brief A control signature tag for input fields from the incident topology.
  ///
  /// For `WorkletVisitPointsWithCells`, this is the same as `FieldInCell`.
  struct FieldInIncident
    : viskores::worklet::WorkletMapTopology<VisitTopologyType,
                                            IncidentTopologyType>::FieldInIncident
  {
  };

  /// @brief A control signature tag for output fields.
  ///
  /// A `WorkletVisitPointsWithCells` always has the output on the points of the topology.
  /// The associated parameter of the invoke should be a `viskores::cont::ArrayHandle`, and it will
  /// be resized to the number of points in the provided `CellSet`.
  struct FieldOutPoint : FieldOut
  {
  };

  /// @copydoc FieldOutPoint
  struct FieldOut
    : viskores::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::FieldOut
  {
  };

  /// @brief A control signature tag for input-output (in-place) fields.
  ///
  /// A `WorkletVisitPointsWithCells` always has the output on the points of the topology.
  /// The associated parameter of the invoke should be a `viskores::cont::ArrayHandle`, and it must
  /// have the same number of values as the number of points of the topology.
  struct FieldInOutPoint : FieldInOut
  {
  };

  /// @copydoc FieldInOutPoint
  struct FieldInOut
    : viskores::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::FieldInOut
  {
  };

  /// @copydoc viskores::worklet::internal::WorkletBase::WholeArrayIn
  struct WholeArrayIn : viskores::worklet::internal::WorkletBase::WholeArrayIn
  {
  };

  /// @copydoc viskores::worklet::internal::WorkletBase::WholeArrayOut
  struct WholeArrayOut : viskores::worklet::internal::WorkletBase::WholeArrayOut
  {
  };

  /// @copydoc viskores::worklet::internal::WorkletBase::WholeArrayInOut
  struct WholeArrayInOut : viskores::worklet::internal::WorkletBase::WholeArrayInOut
  {
  };

  /// @copydoc viskores::worklet::internal::WorkletBase::AtomicArrayInOut
  struct AtomicArrayInOut : viskores::worklet::internal::WorkletBase::AtomicArrayInOut
  {
  };

  /// @copydoc viskores::worklet::internal::WorkletBase::WholeCellSetIn
  template <typename VisitTopology = Cell, typename IncidentTopology = Point>
  struct WholeCellSetIn
    : viskores::worklet::internal::WorkletBase::WholeCellSetIn<VisitTopology, IncidentTopology>
  {
  };

  /// @copydoc viskores::worklet::internal::WorkletBase::ExecObject
  struct ExecObject : viskores::worklet::internal::WorkletBase::ExecObject
  {
  };

  /// @}

  /// @defgroup WorkletVisitPointsWithCellsExecutionSigTags `ExecutionSignature` tags
  /// Tags that can be used in the `ExecutionSignature` of a `WorkletVisitPointsWithCells`.
  /// @{

  /// @copydoc viskores::placeholders::Arg
  struct _1 : viskores::worklet::internal::WorkletBase::_1
  {
  };

  /// @brief An execution signature tag to get the number of incident cells.
  ///
  /// Each point in a `viskores::cont::CellSet` can be incident on a number of cells. This
  /// tag causes a `viskores::IdComponent` to be passed to the worklet containing the number
  /// of incident cells.
  struct CellCount
    : viskores::worklet::WorkletMapTopology<VisitTopologyType,
                                            IncidentTopologyType>::IncidentElementCount
  {
  };

  /// @brief An execution signature tag to get the indices of the incident cells.
  ///
  /// The indices will be provided in a Vec-like object containing `viskores::Id` indices for the
  /// points in the data set.
  struct CellIndices
    : viskores::worklet::WorkletMapTopology<VisitTopologyType,
                                            IncidentTopologyType>::IncidentElementIndices
  {
  };

  /// @copydoc viskores::exec::arg::WorkIndex
  struct WorkIndex : viskores::worklet::internal::WorkletBase::WorkIndex
  {
  };

  /// @copydoc viskores::exec::arg::VisitIndex
  struct VisitIndex : viskores::worklet::internal::WorkletBase::VisitIndex
  {
  };

  /// @copydoc viskores::exec::arg::InputIndex
  struct InputIndex : viskores::worklet::internal::WorkletBase::InputIndex
  {
  };

  /// @copydoc viskores::exec::arg::OutputIndex
  struct OutputIndex : viskores::worklet::internal::WorkletBase::OutputIndex
  {
  };

  /// @copydoc viskores::exec::arg::ThreadIndices
  struct ThreadIndices : viskores::worklet::internal::WorkletBase::ThreadIndices
  {
  };

  /// @copydoc viskores::worklet::internal::WorkletBase::Device
  struct Device : viskores::worklet::internal::WorkletBase::Device
  {
  };

  /// @}
#endif // VISKORES_DOXYGEN_ONLY
};

}
} // namespace viskores::worklet

#endif //viskores_worklet_WorkletMapTopology_h
