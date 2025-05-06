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
#ifndef viskores_worklet_internal_WorkletBase_h
#define viskores_worklet_internal_WorkletBase_h

#include <viskores/TopologyElementTag.h>

#include <viskores/exec/FunctorBase.h>
#include <viskores/exec/arg/BasicArg.h>
#include <viskores/exec/arg/FetchTagExecObject.h>
#include <viskores/exec/arg/FetchTagWholeCellSetIn.h>
#include <viskores/exec/arg/InputIndex.h>
#include <viskores/exec/arg/OutputIndex.h>
#include <viskores/exec/arg/ThreadIndices.h>
#include <viskores/exec/arg/ThreadIndicesBasic.h>
#include <viskores/exec/arg/ThreadIndicesBasic3D.h>
#include <viskores/exec/arg/VisitIndex.h>
#include <viskores/exec/arg/WorkIndex.h>

#include <viskores/cont/arg/ControlSignatureTagBase.h>
#include <viskores/cont/arg/TransportTagAtomicArray.h>
#include <viskores/cont/arg/TransportTagBitField.h>
#include <viskores/cont/arg/TransportTagCellSetIn.h>
#include <viskores/cont/arg/TransportTagExecObject.h>
#include <viskores/cont/arg/TransportTagWholeArrayIn.h>
#include <viskores/cont/arg/TransportTagWholeArrayInOut.h>
#include <viskores/cont/arg/TransportTagWholeArrayOut.h>
#include <viskores/cont/arg/TypeCheckTagArrayIn.h>
#include <viskores/cont/arg/TypeCheckTagArrayInOut.h>
#include <viskores/cont/arg/TypeCheckTagArrayOut.h>
#include <viskores/cont/arg/TypeCheckTagAtomicArray.h>
#include <viskores/cont/arg/TypeCheckTagBitField.h>
#include <viskores/cont/arg/TypeCheckTagCellSet.h>
#include <viskores/cont/arg/TypeCheckTagExecObject.h>

#include <viskores/cont/internal/Hints.h>

#include <viskores/worklet/MaskNone.h>
#include <viskores/worklet/ScatterIdentity.h>
#include <viskores/worklet/internal/Placeholders.h>

namespace viskores
{
namespace worklet
{
namespace internal
{

/// Base class for all worklet classes. Worklet classes are subclasses and a
/// operator() const is added to implement an algorithm in Viskores. Different
/// worklets have different calling semantics.
///
class VISKORES_ALWAYS_EXPORT WorkletBase : public viskores::exec::FunctorBase
{
public:
  using _1 = viskores::placeholders::Arg<1>;
  using _2 = viskores::placeholders::Arg<2>;
  using _3 = viskores::placeholders::Arg<3>;
  using _4 = viskores::placeholders::Arg<4>;
  using _5 = viskores::placeholders::Arg<5>;
  using _6 = viskores::placeholders::Arg<6>;
  using _7 = viskores::placeholders::Arg<7>;
  using _8 = viskores::placeholders::Arg<8>;
  using _9 = viskores::placeholders::Arg<9>;
  using _10 = viskores::placeholders::Arg<10>;
  using _11 = viskores::placeholders::Arg<11>;
  using _12 = viskores::placeholders::Arg<12>;
  using _13 = viskores::placeholders::Arg<13>;
  using _14 = viskores::placeholders::Arg<14>;
  using _15 = viskores::placeholders::Arg<15>;
  using _16 = viskores::placeholders::Arg<16>;
  using _17 = viskores::placeholders::Arg<17>;
  using _18 = viskores::placeholders::Arg<18>;
  using _19 = viskores::placeholders::Arg<19>;
  using _20 = viskores::placeholders::Arg<20>;

  /// @copydoc viskores::exec::arg::WorkIndex
  using WorkIndex = viskores::exec::arg::WorkIndex;

  /// @copydoc viskores::exec::arg::InputIndex
  using InputIndex = viskores::exec::arg::InputIndex;

  /// @copydoc viskores::exec::arg::OutputIndex
  using OutputIndex = viskores::exec::arg::OutputIndex;

  /// @copydoc viskores::exec::arg::ThreadIndices
  using ThreadIndices = viskores::exec::arg::ThreadIndices;

  /// @copydoc viskores::exec::arg::VisitIndex
  using VisitIndex = viskores::exec::arg::VisitIndex;

  /// @brief `ExecutionSignature` tag for getting the device adapter tag.
  ///
  /// This tag passes a device adapter tag object. This allows the worklet function
  /// to template on or overload itself based on the type of device that it is
  /// being executed on.
  struct Device : viskores::exec::arg::ExecutionSignatureTagBase
  {
    // INDEX 0 (which is an invalid parameter index) is reserved to mean the device adapter tag.
    static constexpr viskores::IdComponent INDEX = 0;
    using AspectTag = viskores::exec::arg::AspectTagDefault;
  };

  /// @brief `ControlSignature` tag for execution object inputs.
  ///
  /// This tag represents an execution object that is passed directly from the
  /// control environment to the worklet. A `ExecObject` argument expects a subclass
  /// of `viskores::exec::ExecutionObjectBase`. Subclasses of `viskores::exec::ExecutionObjectBase`
  /// behave like a factory for objects that work on particular devices. They
  /// do this by implementing a `PrepareForExecution()` method that takes a device
  /// adapter tag and returns an object that works on that device. That device-specific
  /// object is passed directly to the worklet.
  struct ExecObject : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagExecObject;
    using TransportTag = viskores::cont::arg::TransportTagExecObject;
    using FetchTag = viskores::exec::arg::FetchTagExecObject;
  };

  /// Default input domain is the first argument. Worklet subclasses can
  /// override this by redefining this type.
  using InputDomain = _1;

  /// All worklets must define their scatter operation. The scatter defines
  /// what output each input contributes to. The default scatter is the
  /// identity scatter (1-to-1 input to output).
  using ScatterType = viskores::worklet::ScatterIdentity;

  /// All worklets must define their mask operation. The mask defines which
  /// outputs are generated. The default mask is the none mask, which generates
  /// everything in the output domain.
  using MaskType = viskores::worklet::MaskNone;

  /// Worklets can provide hints to the scheduler by defining a `Hints` type that
  /// resolves to a `viskores::cont::internal::HintList`. The default hint list is empty
  /// so that scheduling uses all defaults.
  using Hints = viskores::cont::internal::HintList<>;

  /// @brief `ControlSignature` tag for whole input arrays.
  ///
  /// The `WholeArrayIn` control signature tag specifies a `viskores::cont::ArrayHandle`
  /// passed to the invoke of the worklet. An array portal capable of reading
  /// from any place in the array is given to the worklet.
  ///
  struct WholeArrayIn : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayIn;
    using TransportTag = viskores::cont::arg::TransportTagWholeArrayIn;
    using FetchTag = viskores::exec::arg::FetchTagExecObject;
  };

  /// @brief `ControlSignature` tag for whole output arrays.
  ///
  /// The `WholeArrayOut` control signature tag specifies an `viskores::cont::ArrayHandle`
  /// passed to the invoke of the worklet. An array portal capable of writing
  /// to any place in the array is given to the worklet. Developers should take
  /// care when using writable whole arrays as introducing race conditions is possible.
  ///
  struct WholeArrayOut : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayOut;
    using TransportTag = viskores::cont::arg::TransportTagWholeArrayOut;
    using FetchTag = viskores::exec::arg::FetchTagExecObject;
  };

  /// @brief `ControlSignature` tag for whole input/output arrays.
  ///
  /// The `WholeArrayOut` control signature tag specifies a `viskores::cont::ArrayHandle`
  /// passed to the invoke of the worklet.  An array portal capable of reading
  /// from or writing to any place in the array is given to the worklet. Developers
  /// should take care when using writable whole arrays as introducing race
  /// conditions is possible.
  ///
  struct WholeArrayInOut : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayInOut;
    using TransportTag = viskores::cont::arg::TransportTagWholeArrayInOut;
    using FetchTag = viskores::exec::arg::FetchTagExecObject;
  };

  /// @brief `ControlSignature` tag for whole input/output arrays.
  ///
  /// The `AtomicArrayInOut` control signature tag specifies `viskores::cont::ArrayHandle`
  /// passed to the invoke of the worklet. A `viskores::exec::AtomicArray` object capable
  /// of performing atomic operations to the entries in the array is given to the
  /// worklet. Atomic arrays can help avoid race conditions but can slow down the
  /// running of a parallel algorithm.
  ///
  struct AtomicArrayInOut : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagAtomicArray;
    using TransportTag = viskores::cont::arg::TransportTagAtomicArray;
    using FetchTag = viskores::exec::arg::FetchTagExecObject;
  };

  /// \c ControlSignature tags for whole BitFields.
  ///
  /// When a BitField is passed in to a worklet expecting this ControlSignature
  /// type, the appropriate BitPortal is generated and given to the worklet's
  /// execution.
  ///
  /// Be aware that this data structure is especially prone to race conditions,
  /// so be sure to use the appropriate atomic methods when necessary.
  /// @{
  ///
  struct BitFieldIn : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagBitField;
    using TransportTag = viskores::cont::arg::TransportTagBitFieldIn;
    using FetchTag = viskores::exec::arg::FetchTagExecObject;
  };
  struct BitFieldOut : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagBitField;
    using TransportTag = viskores::cont::arg::TransportTagBitFieldOut;
    using FetchTag = viskores::exec::arg::FetchTagExecObject;
  };
  struct BitFieldInOut : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagBitField;
    using TransportTag = viskores::cont::arg::TransportTagBitFieldInOut;
    using FetchTag = viskores::exec::arg::FetchTagExecObject;
  };
  /// @}

  using Point = viskores::TopologyElementTagPoint;
  using Cell = viskores::TopologyElementTagCell;
  using Edge = viskores::TopologyElementTagEdge;
  using Face = viskores::TopologyElementTagFace;

  /// @brief `ControlSignature` tag for whole input topology.
  ///
  /// The `WholeCellSetIn` control signature tag specifies a `viskores::cont::CellSet`
  /// passed to the invoke of the worklet. A connectivity object capable of finding
  /// elements of one type that are incident on elements of a different type. This
  /// can be used to global lookup for arbitrary topology information
  template <typename VisitTopology = Cell, typename IncidentTopology = Point>
  struct WholeCellSetIn : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagCellSet;
    using TransportTag =
      viskores::cont::arg::TransportTagCellSetIn<VisitTopology, IncidentTopology>;
    using FetchTag = viskores::exec::arg::FetchTagWholeCellSetIn;
  };

  /// \brief Creates a \c ThreadIndices object.
  ///
  /// Worklet types can add additional indices by returning different object
  /// types.
  ///
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType>
  VISKORES_EXEC viskores::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const viskores::Id& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType&) const
  {
    viskores::Id outIndex = threadToOut.Get(threadIndex);
    return viskores::exec::arg::ThreadIndicesBasic(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex);
  }

  /// \brief Creates a \c ThreadIndices object.
  ///
  /// Worklet types can add additional indices by returning different object
  /// types.
  ///
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType>
  VISKORES_EXEC viskores::exec::arg::ThreadIndicesBasic3D GetThreadIndices(
    viskores::Id threadIndex1D,
    const viskores::Id3& threadIndex3D,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType&) const
  {
    viskores::Id outIndex = threadToOut.Get(threadIndex1D);
    return viskores::exec::arg::ThreadIndicesBasic3D(
      threadIndex3D, threadIndex1D, outToIn.Get(outIndex), visit.Get(outIndex), outIndex);
  }
};
}
}
} // namespace viskores::worklet::internal

#endif //viskores_worklet_internal_WorkletBase_h
