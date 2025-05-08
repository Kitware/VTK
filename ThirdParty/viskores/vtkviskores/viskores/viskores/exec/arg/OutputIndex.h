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
#ifndef viskores_exec_arg_OutputIndex_h
#define viskores_exec_arg_OutputIndex_h

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
/// The `AspectTagOutputIndex` aspect tag causes the `Fetch` class to ignore
/// whatever data is in the associated execution object and return the index
/// of the output element.
///
struct AspectTagOutputIndex
{
};

/// @brief The `ExecutionSignature` tag to use to get the output index
///
/// This tag produces a `viskores::Id` that identifies the index of the output element.
/// (This is generally the same as `WorkIndex`.)
///
/// When a worklet is dispatched, it broken into pieces defined by the output
/// domain and scheduled on independent threads. This tag in the
/// `ExecutionSignature` passes the index of the output element that the work
/// thread is currently working on. When a worklet has a scatter associated
/// with it, the output and output indices can be different.
///
struct OutputIndex : viskores::exec::arg::ExecutionSignatureTagBase
{
  // The index does not really matter because the fetch is going to ignore it.
  // However, it still has to point to a valid parameter in the
  // ControlSignature because the templating is going to grab a fetch tag
  // whether we use it or not. 1 should be guaranteed to be valid since you
  // need at least one argument for the output domain.
  static constexpr viskores::IdComponent INDEX = 1;
  using AspectTag = viskores::exec::arg::AspectTagOutputIndex;
};

template <typename FetchTag, typename ExecObjectType>
struct Fetch<FetchTag, viskores::exec::arg::AspectTagOutputIndex, ExecObjectType>
{
  using ValueType = viskores::Id;

  template <typename ThreadIndicesType>
  VISKORES_EXEC viskores::Id Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    return indices.GetOutputIndex();
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

#endif //viskores_exec_arg_OutputIndex_h
