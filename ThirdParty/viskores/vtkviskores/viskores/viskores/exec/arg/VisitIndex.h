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
#ifndef viskores_exec_arg_VisitIndex_h
#define viskores_exec_arg_VisitIndex_h

#include <viskores/exec/arg/ExecutionSignatureTagBase.h>
#include <viskores/exec/arg/Fetch.h>

namespace viskores
{
namespace exec
{
namespace arg
{

/// @brief Aspect tag to use for getting the work index.
///
/// The `AspectTagVisitIndex` aspect tag causes the `Fetch` class to ignore
/// whatever data is in the associated execution object and return the visit
/// index.
///
struct AspectTagVisitIndex
{
};

/// @brief The `ExecutionSignature` tag to use to get the visit index
///
/// This tag produces a `viskores::IdComponent` that uniquely identifies when multiple
/// worklet invocations operate on the same input item, which can happen when
/// defining a worklet with scatter.
///
/// When a worklet is dispatched, there is a scatter operation defined that
/// optionally allows each input to go to multiple output entries. When one
/// input is assigned to multiple outputs, there needs to be a mechanism to
/// uniquely identify which output is which. The visit index is a value between
/// 0 and the number of outputs a particular input goes to. This tag in the
/// `ExecutionSignature` passes the visit index for this work.
///
struct VisitIndex : viskores::exec::arg::ExecutionSignatureTagBase
{
  // The index does not really matter because the fetch is going to ignore it.
  // However, it still has to point to a valid parameter in the
  // ControlSignature because the templating is going to grab a fetch tag
  // whether we use it or not. 1 should be guaranteed to be valid since you
  // need at least one argument for the input domain.
  static constexpr viskores::IdComponent INDEX = 1;
  using AspectTag = viskores::exec::arg::AspectTagVisitIndex;
};

template <typename FetchTag, typename ExecObjectType>
struct Fetch<FetchTag, viskores::exec::arg::AspectTagVisitIndex, ExecObjectType>
{
  using ValueType = viskores::IdComponent;

  template <typename ThreadIndicesType>
  VISKORES_EXEC viskores::IdComponent Load(const ThreadIndicesType& indices,
                                           const ExecObjectType&) const
  {
    return indices.GetVisitIndex();
  }

  template <typename ThreadIndicesType>
  VISKORES_EXEC void Store(const ThreadIndicesType&, const ExecObjectType&, const ValueType&) const
  {
    // Store is a no-op.
  }
};
}
}
} // namespace viskores::exec::arg

#endif //viskores_exec_arg_VisitIndex_h
