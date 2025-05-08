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
#ifndef viskores_worklet_WorkletReduceByKey_h
#define viskores_worklet_WorkletReduceByKey_h

#include <viskores/worklet/internal/WorkletBase.h>

#include <viskores/cont/arg/TransportTagArrayIn.h>
#include <viskores/cont/arg/TransportTagArrayInOut.h>
#include <viskores/cont/arg/TransportTagArrayOut.h>
#include <viskores/cont/arg/TransportTagKeyedValuesIn.h>
#include <viskores/cont/arg/TransportTagKeyedValuesInOut.h>
#include <viskores/cont/arg/TransportTagKeyedValuesOut.h>
#include <viskores/cont/arg/TransportTagKeysIn.h>
#include <viskores/cont/arg/TypeCheckTagArrayIn.h>
#include <viskores/cont/arg/TypeCheckTagArrayInOut.h>
#include <viskores/cont/arg/TypeCheckTagArrayOut.h>
#include <viskores/cont/arg/TypeCheckTagKeys.h>

#include <viskores/exec/internal/ReduceByKeyLookup.h>

#include <viskores/exec/arg/FetchTagArrayDirectIn.h>
#include <viskores/exec/arg/FetchTagArrayDirectInOut.h>
#include <viskores/exec/arg/FetchTagArrayDirectOut.h>
#include <viskores/exec/arg/FetchTagKeysIn.h>
#include <viskores/exec/arg/ThreadIndicesReduceByKey.h>
#include <viskores/exec/arg/ValueCount.h>
#include <viskores/worklet/DispatcherReduceByKey.h>

namespace viskores
{
namespace worklet
{

/// @brief Base class for worklets that group elements by keys.
///
/// The `InputDomain` of this worklet is a `viskores::worklet::Keys` object,
/// which holds an array of keys. All entries of this array with the same
/// key are collected together, and the operator of the worklet is called
/// once for each unique key.
///
/// Input arrays are (typically) the same size as the number of keys. When
/// these objects are passed to the operator of the worklet, all values of
/// the associated key are placed in a Vec-like object. Output arrays get
/// sized by the number of unique keys, and each call to the operator produces
/// one result for each output.
class WorkletReduceByKey : public viskores::worklet::internal::WorkletBase
{
public:
  template <typename Worklet>
  using Dispatcher = viskores::worklet::DispatcherReduceByKey<Worklet>;

  /// @defgroup WorkletReduceByKeyControlSigTags `ControlSignature` tags
  /// Tags that can be used in the `ControlSignature` of a `WorkletMapField`.
  /// @{

  /// @brief A control signature tag for input keys.
  ///
  /// A `WorkletReduceByKey` operates by collecting all identical keys and
  /// then executing the worklet on each unique key. This tag specifies a
  /// `viskores::worklet::Keys` object that defines and manages these keys.
  ///
  /// A `WorkletReduceByKey` should have exactly one `KeysIn` tag in its
  /// `ControlSignature`, and the `InputDomain` should point to it.
  ///
  struct KeysIn : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagKeys;
    using TransportTag = viskores::cont::arg::TransportTagKeysIn;
    using FetchTag = viskores::exec::arg::FetchTagKeysIn;
  };

  /// @brief A control signature tag for input values associated with the keys.
  ///
  /// A `WorkletReduceByKey` operates by collecting all values associated with
  /// identical keys and then giving the worklet a Vec-like object containing
  /// all values with a matching key. This tag specifies an `viskores::cont::ArrayHandle`
  /// object that holds the values. The number of values in this array must be equal
  /// to the size of the array used with the `KeysIn` argument.
  ///
  struct ValuesIn : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayIn;
    using TransportTag = viskores::cont::arg::TransportTagKeyedValuesIn;
    using FetchTag = viskores::exec::arg::FetchTagArrayDirectIn;
  };

  /// @brief A control signature tag for input/output values associated with the keys.
  ///
  /// A `WorkletReduceByKey` operates by collecting all values associated with
  /// identical keys and then giving the worklet a Vec-like object containing
  /// all values with a matching key. This tag specifies an `viskores::cont::ArrayHandle`
  /// object that holds the values. The number of values in this array must be equal
  /// to the size of the array used with the `KeysIn` argument.
  ///
  /// This tag might not work with scatter operations.
  ///
  struct ValuesInOut : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayInOut;
    using TransportTag = viskores::cont::arg::TransportTagKeyedValuesInOut;
    using FetchTag = viskores::exec::arg::FetchTagArrayDirectIn;
  };

  /// \brief A control signature tag for output values associated with the keys.
  ///
  /// This tag behaves the same as `ValuesInOut` except that the array is resized
  /// appropriately and no input values are passed to the worklet. As with
  /// `ValuesInOut`, values the worklet writes to its |Veclike| object get placed
  /// in the location of the original arrays.
  ///
  /// Use of `ValuesOut` is rare.
  ///
  /// This tag might not work with scatter operations.
  ///
  struct ValuesOut : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayOut;
    using TransportTag = viskores::cont::arg::TransportTagKeyedValuesOut;
    using FetchTag = viskores::exec::arg::FetchTagArrayDirectIn;
  };

  /// @brief A control signature tag for reduced output values.
  ///
  /// A `WorkletReduceByKey` operates by collecting all identical keys and
  /// calling one instance of the worklet for those identical keys. The worklet
  /// then produces a "reduced" value per key. This tag specifies a
  /// `viskores::cont::ArrayHandle` object that holds the values. The array is resized
  /// to be the number of unique keys, and each call of the operator sets
  /// a single value in the array
  ///
  struct ReducedValuesOut : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayOut;
    using TransportTag = viskores::cont::arg::TransportTagArrayOut;
    using FetchTag = viskores::exec::arg::FetchTagArrayDirectOut;
  };

  /// @brief A control signature tag for reduced input values.
  ///
  /// A`WorkletReduceByKey` operates by collecting all identical keys and
  /// calling one instance of the worklet for those identical keys. The worklet
  /// then produces a "reduced" value per key.
  ///
  /// This tag specifies a `viskores::cont::ArrayHandle` object that holds the values.
  /// It is an input array with entries for each reduced value. The number of values
  /// in the array must equal the number of _unique_ keys.
  ///
  /// A `ReducedValuesIn` argument is usually used to pass reduced values from one
  /// invoke of a reduce by key worklet to another invoke of a reduced by key worklet
  /// such as in an algorithm that requires iterative steps.
  ///
  struct ReducedValuesIn : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayIn;
    using TransportTag = viskores::cont::arg::TransportTagArrayIn;
    using FetchTag = viskores::exec::arg::FetchTagArrayDirectIn;
  };

  /// @brief A control signature tag for reduced output values.
  ///
  /// A `WorkletReduceByKey` operates by collecting all identical keys and
  /// calling one instance of the worklet for those identical keys. The worklet
  /// then produces a "reduced" value per key.
  ///
  /// This tag specifies a `viskores::cont::ArrayHandle` object that holds the values.
  /// It is an input/output array with entries for each reduced value. The number
  /// of values in the array must equal the number of _unique_ keys.
  ///
  /// This tag behaves the same as `ReducedValuesIn` except that the worklet may
  /// write values back into the array. Make sure that the associated parameter to
  /// the worklet operator is a reference so that the changed value gets written
  /// back to the array.
  ///
  struct ReducedValuesInOut : viskores::cont::arg::ControlSignatureTagBase
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

  /// @defgroup WorkletReduceByKeyExecutionSigTags `ExecutionSignature` tags
  /// Tags that can be used in the `ExecutionSignature` of a `WorkletMapField`.
  /// @{

#ifdef VISKORES_DOXYGEN_ONLY
  // These redeclarations of superclass features are for documentation purposes only.

  /// @copydoc viskores::placeholders::Arg
  struct _1 : viskores::worklet::internal::WorkletBase::_1
  {
  };
#endif

  /// @brief The `ExecutionSignature` tag to get the number of values.
  ///
  /// A `WorkletReduceByKey` operates by collecting all values associated with
  /// identical keys and then giving the worklet a Vec-like object containing all
  /// values with a matching key. This tag produces a `viskores::IdComponent` that is
  /// equal to the number of times the key associated with this call to the worklet
  /// occurs in the input. This is the same size as the Vec-like objects provided
  /// by `ValuesIn` arguments.
  ///
  struct ValueCount : viskores::exec::arg::ValueCount
  {
  };

#ifdef VISKORES_DOXYGEN_ONLY
  // These redeclarations of superclass features are for documentation purposes only.

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

  /// Reduce by key worklets use the related thread indices class.
  ///
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType>
  VISKORES_EXEC viskores::exec::arg::ThreadIndicesReduceByKey GetThreadIndices(
    viskores::Id threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType& inputDomain) const
  {
    const viskores::Id outIndex = threadToOut.Get(threadIndex);
    return viskores::exec::arg::ThreadIndicesReduceByKey(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex, inputDomain);
  }
};
}
} // namespace viskores::worklet

#endif //viskores_worklet_WorkletReduceByKey_h
