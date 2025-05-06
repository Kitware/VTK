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
#ifndef viskores_exec_arg_OnBoundary_h
#define viskores_exec_arg_OnBoundary_h

#include <viskores/exec/arg/ExecutionSignatureTagBase.h>
#include <viskores/exec/arg/Fetch.h>
#include <viskores/exec/arg/ThreadIndicesPointNeighborhood.h>

namespace viskores
{
namespace exec
{
namespace arg
{

/// \brief Aspect tag to use for getting if a point is a boundary point.
///
/// The \c AspectTagBoundary aspect tag causes the \c Fetch class to obtain
/// if the point is on a boundary.
///
struct AspectTagBoundary
{
};


/// \brief The \c ExecutionSignature tag to get if executing on a boundary element
///
struct Boundary : viskores::exec::arg::ExecutionSignatureTagBase
{
  static constexpr viskores::IdComponent INDEX = 1;
  using AspectTag = viskores::exec::arg::AspectTagBoundary;
};

template <typename FetchTag, typename ExecObjectType>
struct Fetch<FetchTag, viskores::exec::arg::AspectTagBoundary, ExecObjectType>
{

  using ValueType = viskores::exec::BoundaryState;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VISKORES_EXEC ValueType Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    return indices.GetBoundaryState();
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

#endif //viskores_exec_arg_OnBoundary_h
