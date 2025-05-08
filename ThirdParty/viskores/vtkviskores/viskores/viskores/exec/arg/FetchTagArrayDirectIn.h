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
#ifndef viskores_exec_arg_FetchTagArrayDirectIn_h
#define viskores_exec_arg_FetchTagArrayDirectIn_h

#include <viskores/exec/arg/AspectTagDefault.h>
#include <viskores/exec/arg/Fetch.h>

namespace viskores
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for getting array values with direct indexing.
///
/// \c FetchTagArrayDirectIn is a tag used with the \c Fetch class to retrieve
/// values from an array portal. The fetch uses direct indexing, so the thread
/// index given to \c Load is used as the index into the array.
///
struct FetchTagArrayDirectIn
{
};


VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename T, typename U>
inline VISKORES_EXEC T load(const U& u, viskores::Id v)
{
  return u.Get(v);
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename T, typename U>
inline VISKORES_EXEC T load(const U* u, viskores::Id v)
{
  return u->Get(v);
}

template <typename ExecObjectType>
struct Fetch<viskores::exec::arg::FetchTagArrayDirectIn,
             viskores::exec::arg::AspectTagDefault,
             ExecObjectType>
{
  //need to remove pointer type from ThreadIdicesType
  using ET = typename std::remove_const<typename std::remove_pointer<ExecObjectType>::type>::type;
  using PortalType =
    typename std::conditional<std::is_pointer<ExecObjectType>::value, const ET*, const ET&>::type;

  using ValueType = typename ET::ValueType;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VISKORES_EXEC ValueType Load(const ThreadIndicesType& indices, PortalType arrayPortal) const
  {
    return load<ValueType>(arrayPortal, indices.GetInputIndex());
  }

  template <typename ThreadIndicesType>
  VISKORES_EXEC void Store(const ThreadIndicesType&, PortalType, const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};
}
}
} // namespace viskores::exec::arg

#endif //viskores_exec_arg_FetchTagArrayDirectIn_h
