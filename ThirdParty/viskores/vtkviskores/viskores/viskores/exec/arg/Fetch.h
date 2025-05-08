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
#ifndef viskores_exec_arg_Fetch_h
#define viskores_exec_arg_Fetch_h

#include <viskores/Types.h>

namespace viskores
{
namespace exec
{
namespace arg
{

/// \brief Class for loading and storing values in thread instance.
///
/// The \c Fetch class is used within a thread in the execution environment
/// to load a value from an execution object specific for the given thread
/// instance and to store a resulting value back in the object. (Either load
/// or store can be a no-op.)
///
/// \c Fetch is a templated class with four arguments. The first argument is a
/// tag declaring the type of fetch, which is usually tied to a particular type
/// of execution object. The second argument is an aspect tag that declares
/// what type of data to pull/push. Together, these two tags determine the
/// mechanism for the fetch. The third argument is the type of thread indices
/// used (one of the classes that starts with \c ThreadIndices in the
/// viskores::exec::arg namespace), which defines the type of thread-local indices
/// available during the fetch. The fourth argument is the type of execution
/// object associated where the fetch (nominally) gets its data from. This
/// execution object is the data provided by the transport.
///
/// There is no generic implementation of \c Fetch. There are partial
/// specializations of \c Fetch for each mechanism (fetch-aspect tag
/// combination) supported. If you get a compiler error about an incomplete
/// type for \c Fetch, it means you used an invalid \c FetchTag - \c AspectTag
/// combination. Most likely this means that a parameter in an
/// ExecutionSignature with a particular aspect is pointing to the wrong
/// argument or an invalid argument in the ControlSignature.
///
template <typename FetchTag, typename AspectTag, typename ExecObjectType>
struct Fetch
#ifdef VISKORES_DOXYGEN_ONLY
{
  /// \brief The type of value to load and store.
  ///
  /// All \c Fetch specializations are expected to declare a type named \c
  /// ValueType that is the type of object returned from \c Load and passed to
  /// \c Store.
  ///
  using ValueType = typename ExecObjectType::ValueType;

  /// \brief Load data for a work instance.
  ///
  /// All \c Fetch specializations are expected to have a constant method named
  /// \c Load that takes a \c ThreadIndices object containing thread-local
  /// indices and an execution object and returns the value appropriate for the
  /// work instance. If there is no actual data to load (for example for a
  /// write-only fetch), this method can be a no-op and return any value.
  ///
  template <typename ThreadIndicesType>
  VISKORES_EXEC ValueType Load(const ThreadIndicesType& indices,
                               const ExecObjectType& execObject) const;

  /// \brief Store data from a work instance.
  ///
  /// All \c Fetch specializations are expected to have a constant method named
  /// \c Store that takes a \c ThreadIndices object containing thread-local
  /// indices, an execution object, and a value computed by the worklet call
  /// and stores that value into the execution object associated with this
  /// fetch. If the store is not applicable (for example for a read-only
  /// fetch), this method can be a no-op.
  ///
  template <typename ThreadIndicesType>
  VISKORES_EXEC void Store(const ThreadIndicesType& indices,
                           const ExecObjectType& execObject,
                           const ValueType& value) const;
};
#else  // VISKORES_DOXYGEN_ONLY
  ;
#endif // VISKORES_DOXYGEN_ONLY
}
}
} // namespace viskores::exec::arg

#endif //viskores_exec_arg_Fetch_h
