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
#ifndef viskores_exec_arg_ValueCount_h
#define viskores_exec_arg_ValueCount_h

#include <viskores/exec/arg/ExecutionSignatureTagBase.h>
#include <viskores/exec/arg/Fetch.h>
#include <viskores/exec/arg/ThreadIndicesReduceByKey.h>

namespace viskores
{
namespace exec
{
namespace arg
{

/// \brief Aspect tag to use for getting the value count.
///
/// The \c AspectTagValueCount aspect tag causes the \c Fetch class to obtain
/// the number of values that map to the key.
///
struct AspectTagValueCount
{
};

/// \brief The \c ExecutionSignature tag to get the number of values.
///
/// A \c WorkletReduceByKey operates by collecting all values associated with
/// identical keys and then giving the worklet a Vec-like object containing all
/// values with a matching key. This \c ExecutionSignature tag provides the
/// number of values associated with the key and given in the Vec-like objects.
///
struct ValueCount : viskores::exec::arg::ExecutionSignatureTagBase
{
  static constexpr viskores::IdComponent INDEX = 1;
  using AspectTag = viskores::exec::arg::AspectTagValueCount;
};

template <typename FetchTag, typename ExecObjectType>
struct Fetch<FetchTag, viskores::exec::arg::AspectTagValueCount, ExecObjectType>
{

  using ValueType = viskores::IdComponent;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VISKORES_EXEC ValueType Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    return indices.GetNumberOfValues();
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

#endif //viskores_exec_arg_ValueCount_h
