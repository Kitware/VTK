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
#ifndef viskores_exec_arg_ThreadIndices_h
#define viskores_exec_arg_ThreadIndices_h

#include <viskores/exec/arg/ExecutionSignatureTagBase.h>
#include <viskores/exec/arg/Fetch.h>

namespace viskores
{
namespace exec
{
namespace arg
{

/// @brief Aspect tag to use for getting the thread indices.
///
/// The `AspectTagThreadIndices` aspect tag causes the `Fetch` class to
/// ignore whatever data is in the associated execution object and return the
/// thread indices.
///
struct AspectTagThreadIndices
{
};

/// @brief The `ExecutionSignature` tag to use to get the thread indices
///
/// This tag produces an internal object that manages indices and other metadata
/// of the current thread. Thread indices objects vary by worklet type, but most
/// users can get the information they need through other signature tags.
///
/// When a worklet is dispatched, it broken into pieces defined by the input
/// domain and scheduled on independent threads. During this process multiple
/// indices associated with the input and output can be generated. This tag in
/// the `ExecutionSignature` passes the index for this work.
///
struct ThreadIndices : viskores::exec::arg::ExecutionSignatureTagBase
{
  // The index does not really matter because the fetch is going to ignore it.
  // However, it still has to point to a valid parameter in the
  // ControlSignature because the templating is going to grab a fetch tag
  // whether we use it or not. 1 should be guaranteed to be valid since you
  // need at least one argument for the input domain.
  static constexpr viskores::IdComponent INDEX = 1;
  using AspectTag = viskores::exec::arg::AspectTagThreadIndices;
};

template <typename FetchTag, typename ExecObjectType>
struct Fetch<FetchTag, viskores::exec::arg::AspectTagThreadIndices, ExecObjectType>
{

  template <typename ThreadIndicesType>
  VISKORES_EXEC const ThreadIndicesType& Load(const ThreadIndicesType& indices,
                                              const ExecObjectType&) const
  {
    return indices;
  }

  template <typename ThreadIndicesType>
  VISKORES_EXEC void Store(const ThreadIndicesType&,
                           const ExecObjectType&,
                           const ThreadIndicesType&) const
  {
    // Store is a no-op.
  }
};
}
}
} // namespace viskores::exec::arg

#endif //viskores_exec_arg_ThreadIndices_h
