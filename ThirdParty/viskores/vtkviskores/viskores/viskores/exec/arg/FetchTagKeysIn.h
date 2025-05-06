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
#ifndef viskores_exec_arg_FetchTagKeysIn_h
#define viskores_exec_arg_FetchTagKeysIn_h

#include <viskores/exec/arg/AspectTagDefault.h>
#include <viskores/exec/arg/Fetch.h>
#include <viskores/exec/arg/ThreadIndicesReduceByKey.h>

#include <viskores/exec/internal/ReduceByKeyLookup.h>

namespace viskores
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for getting key values in a reduce by key.
///
/// \c FetchTagKeysIn is a tag used with the \c Fetch class to retrieve keys
/// from the input domain of a reduce by keys worklet.
///
struct FetchTagKeysIn
{
};

template <typename KeyPortalType, typename IdPortalType, typename IdComponentPortalType>
struct Fetch<
  viskores::exec::arg::FetchTagKeysIn,
  viskores::exec::arg::AspectTagDefault,
  viskores::exec::internal::ReduceByKeyLookup<KeyPortalType, IdPortalType, IdComponentPortalType>>
{
  using ExecObjectType =
    viskores::exec::internal::ReduceByKeyLookup<KeyPortalType, IdPortalType, IdComponentPortalType>;

  using ValueType = typename ExecObjectType::KeyType;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VISKORES_EXEC ValueType Load(const ThreadIndicesType& indices, const ExecObjectType& keys) const
  {
    return keys.UniqueKeys.Get(indices.GetInputIndex());
  }

  template <typename ThreadIndicesType>
  VISKORES_EXEC void Store(const ThreadIndicesType&, const ExecObjectType&, const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};
}
}
} // namespace viskores::exec::arg

#endif //viskores_exec_arg_FetchTagKeysIn_h
