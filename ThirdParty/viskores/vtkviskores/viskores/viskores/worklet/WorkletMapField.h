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
#ifndef viskores_worklet_WorkletMapField_h
#define viskores_worklet_WorkletMapField_h

#include <viskores/worklet/internal/WorkletBase.h>

#include <viskores/cont/arg/ControlSignatureTagBase.h>
#include <viskores/cont/arg/TransportTagArrayIn.h>
#include <viskores/cont/arg/TransportTagArrayInOut.h>
#include <viskores/cont/arg/TransportTagArrayOut.h>
#include <viskores/cont/arg/TypeCheckTagArrayIn.h>
#include <viskores/cont/arg/TypeCheckTagArrayInOut.h>
#include <viskores/cont/arg/TypeCheckTagArrayOut.h>

#include <viskores/exec/arg/FetchTagArrayDirectIn.h>
#include <viskores/exec/arg/FetchTagArrayDirectInOut.h>
#include <viskores/exec/arg/FetchTagArrayDirectOut.h>

#include <viskores/worklet/DispatcherMapField.h>

namespace viskores
{
namespace worklet
{

/// @brief Base class for worklets that do a simple mapping of field arrays.
///
/// All inputs and outputs are on the same domain. That is, all the arrays are the
/// same size.
///
class WorkletMapField : public viskores::worklet::internal::WorkletBase
{
public:
  template <typename Worklet>
  using Dispatcher = viskores::worklet::DispatcherMapField<Worklet>;

  /// @defgroup WorkletMapFieldControlSigTags `ControlSignature` tags
  /// Tags that can be used in the `ControlSignature` of a `WorkletMapField`.
  /// @{

  /// @brief A control signature tag for input fields.
  ///
  /// A `FieldIn` argument expects a `viskores::cont::ArrayHandle` in the associated
  /// parameter of the invoke. Each invocation of the worklet gets a single value
  /// out of this array.
  ///
  /// This tag means that the field is read only.
  ///
  /// The worklet's `InputDomain` can be set to a `FieldIn` argument. In this case,
  /// the input domain will be the size of the array.
  ///
  struct FieldIn : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayIn;
    using TransportTag = viskores::cont::arg::TransportTagArrayIn;
    using FetchTag = viskores::exec::arg::FetchTagArrayDirectIn;
  };

  /// @brief A control signature tag for output fields.
  ///
  /// A `FieldOut` argument expects a `viskores::cont::ArrayHandle` in the associated
  /// parameter of the invoke. The array is resized before scheduling begins, and
  /// each invocation of the worklet sets a single value in the array.
  ///
  /// This tag means that the field is write only.
  ///
  /// Although uncommon, it is possible to set the worklet's `InputDomain` to a
  /// `FieldOut` argument. If this is the case, then the `viskores::cont::ArrayHandle`
  /// passed as the argument must be allocated before being passed to the invoke,
  /// and the input domain will be the size of the array.
  ///
  struct FieldOut : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayOut;
    using TransportTag = viskores::cont::arg::TransportTagArrayOut;
    using FetchTag = viskores::exec::arg::FetchTagArrayDirectOut;
  };

  /// @brief A control signature tag for input-output (in-place) fields.
  ///
  /// A `FieldInOut` argument expects a `viskores::cont::ArrayHandle` in the
  /// associated parameter of the invoke. Each invocation of the worklet gets a
  /// single value out of this array, which is replaced by the resulting value
  /// after the worklet completes.
  ///
  /// This tag means that the field is read and write.
  ///
  /// The worklet's `InputDomain` can be set to a `FieldInOut` argument. In
  /// this case, the input domain will be the size of the array.
  ///
  struct FieldInOut : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayInOut;
    using TransportTag = viskores::cont::arg::TransportTagArrayInOut;
    using FetchTag = viskores::exec::arg::FetchTagArrayDirectInOut;
  };

#ifdef VISKORES_DOXYGEN_ONLY
  // These redeclarations of superclass features are for documentation purposes only.

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
#endif

  /// @}

  /// @defgroup WorkletMapFieldExecutionSigTags `ExecutionSignature` tags
  /// Tags that can be used in the `ExecutionSignature` of a `WorkletMapField`.
  /// @{

#ifdef VISKORES_DOXYGEN_ONLY
  // These redeclarations of superclass features are for documentation purposes only.

  /// @copydoc viskores::placeholders::Arg
  struct _1 : viskores::worklet::internal::WorkletBase::_1
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
#endif

  /// @}
};

}
} // namespace viskores::worklet

#endif //viskores_worklet_WorkletMapField_h
