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
#ifndef viskores_exec_arg_FetchTagExecObject_h
#define viskores_exec_arg_FetchTagExecObject_h

#include <viskores/exec/arg/AspectTagDefault.h>
#include <viskores/exec/arg/Fetch.h>

#include <type_traits>

namespace viskores
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for execution objects.
///
/// \c FetchTagExecObject is a tag used with the \c Fetch class to retrieve
/// execution objects. For safety, execution objects are read-only. \c
/// FetchTagExecObject is almost always used in conjunction with \c
/// TransportTagExecObject and vice versa.
///
struct FetchTagExecObject
{
};

template <typename ExecObjectType>
struct Fetch<viskores::exec::arg::FetchTagExecObject,
             viskores::exec::arg::AspectTagDefault,
             ExecObjectType>
{
  using ValueType = ExecObjectType;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VISKORES_EXEC ValueType Load(const ThreadIndicesType& viskoresNotUsed(indices),
                               const ExecObjectType& execObject) const
  {
    return execObject;
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

#endif //viskores_exec_arg_FetchTagExecObject_h
