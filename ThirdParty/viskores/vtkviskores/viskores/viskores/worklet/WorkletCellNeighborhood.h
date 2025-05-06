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
#ifndef viskores_worklet_WorkletCellNeighborhood_h
#define viskores_worklet_WorkletCellNeighborhood_h

/// \brief Worklet for volume algorithms that require a neighborhood
///
/// WorkletCellNeighborhood executes on every point inside a volume providing
/// access to the 3D neighborhood values. The neighborhood is always cubic in
/// nature and is fixed at compile time.

#include <viskores/exec/arg/ThreadIndicesCellNeighborhood.h>
#include <viskores/worklet/DispatcherCellNeighborhood.h>
#include <viskores/worklet/WorkletNeighborhood.h>

namespace viskores
{
namespace worklet
{

template <typename WorkletType>
class DispatcherCellNeighborhood;

/// @brief Base class for worklets that map over the cells in a structured grid with neighborhood information.
///
/// The domain of a `WorkletCellNeighborhood` is a `viskores::cont::CellSetStructured`. It visits
/// all the cells in the mesh and provides access to the cell field values of the visited cell
/// and the field values of the nearby connected neighborhood of a prescribed size.
class WorkletCellNeighborhood : public WorkletNeighborhood
{
public:
  template <typename Worklet>
  using Dispatcher = viskores::worklet::DispatcherCellNeighborhood<Worklet>;

  /// @defgroup WorkletCellNeighborhoodControlSigTags `ControlSignature` tags
  /// Tags that can be used in the `ControlSignature` of a `WorkletPointNeighborhood`.
  /// @{
#ifdef VISKORES_DOXYGEN_ONLY
  // These redeclarations of superclass features are for documentation purposes only.

  /// @copydoc viskores::worklet::WorkletNeighborhood::CellSetIn
  struct CellSetIn : viskores::worklet::WorkletNeighborhood::CellSetIn
  {
  };

  /// @copydoc viskores::worklet::WorkletNeighborhood::FieldIn
  struct FieldIn : viskores::worklet::WorkletNeighborhood::FieldIn
  {
  };

  /// @copydoc viskores::worklet::WorkletNeighborhood::FieldInNeighborhood
  struct FieldInNeighborhood : viskores::worklet::WorkletNeighborhood::FieldInNeighborhood
  {
  };

  /// @copydoc viskores::worklet::WorkletNeighborhood::FieldOut
  struct FieldOut : viskores::worklet::WorkletNeighborhood::FieldOut
  {
  };

  /// @copydoc viskores::worklet::WorkletNeighborhood::FieldInOut
  struct FieldInOut : viskores::worklet::WorkletNeighborhood::FieldInOut
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
#endif // VISKORES_DOXYGEN_ONLY
  /// @}

  /// @defgroup WorkletCellNeighborhoodExecutionSigTags `ExecutionSignature` tags
  /// Tags that can be used in the `ExecutionSignature` of a `WorkletPointNeighborhood`.
  /// @{
#ifdef VISKORES_DOXYGEN_ONLY
  // These redeclarations of superclass features are for documentation purposes only.

  /// @copydoc viskores::placeholders::Arg
  struct _1 : viskores::worklet::internal::WorkletBase::_1
  {
  };

  /// @copydoc viskores::worklet::WorkletNeighborhood::Boundary
  struct Boundary : viskores::worklet::WorkletNeighborhood::Boundary
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
#endif // VISKORES_DOXYGEN_ONLY
  /// @}

  /// Point neighborhood worklets use the related thread indices class.
  ///
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            viskores::IdComponent Dimension>
  VISKORES_EXEC viskores::exec::arg::ThreadIndicesCellNeighborhood GetThreadIndices(
    viskores::Id threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const viskores::exec::ConnectivityStructured<viskores::TopologyElementTagPoint,
                                                 viskores::TopologyElementTagCell,
                                                 Dimension>& inputDomain //this should be explicit
  ) const
  {
    const viskores::Id outIndex = threadToOut.Get(threadIndex);
    return viskores::exec::arg::ThreadIndicesCellNeighborhood(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex, inputDomain);
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

public:
  template <bool Cond, typename ReturnType>
  using EnableFnWhen = typename std::enable_if<Cond, ReturnType>::type;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType,
            bool S = IsScatterIdentity,
            bool M = IsMaskNone>
  VISKORES_EXEC EnableFnWhen<S && M, viskores::exec::arg::ThreadIndicesCellNeighborhood>
  GetThreadIndices(viskores::Id threadIndex1D,
                   const viskores::Id3& threadIndex3D,
                   const OutToInArrayType& viskoresNotUsed(outToIn),
                   const VisitArrayType& viskoresNotUsed(visit),
                   const ThreadToOutArrayType& viskoresNotUsed(threadToOut),
                   const InputDomainType& connectivity) const
  {
    return viskores::exec::arg::ThreadIndicesCellNeighborhood(
      threadIndex3D, threadIndex1D, connectivity);
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType,
            bool S = IsScatterIdentity,
            bool M = IsMaskNone>
  VISKORES_EXEC EnableFnWhen<!(S && M), viskores::exec::arg::ThreadIndicesCellNeighborhood>
  GetThreadIndices(viskores::Id threadIndex1D,
                   const viskores::Id3& threadIndex3D,
                   const OutToInArrayType& outToIn,
                   const VisitArrayType& visit,
                   const ThreadToOutArrayType& threadToOut,
                   const InputDomainType& connectivity) const
  {
    const viskores::Id outIndex = threadToOut.Get(threadIndex1D);
    return viskores::exec::arg::ThreadIndicesCellNeighborhood(threadIndex3D,
                                                              threadIndex1D,
                                                              outToIn.Get(outIndex),
                                                              visit.Get(outIndex),
                                                              outIndex,
                                                              connectivity);
  }
};
}
}

#endif
