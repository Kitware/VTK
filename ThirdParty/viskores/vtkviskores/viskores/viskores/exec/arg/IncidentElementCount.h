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
#ifndef viskores_exec_arg_IncidentElementCount_h
#define viskores_exec_arg_IncidentElementCount_h

#include <viskores/exec/arg/ExecutionSignatureTagBase.h>
#include <viskores/exec/arg/Fetch.h>

namespace viskores
{
namespace exec
{
namespace arg
{

/// \brief Aspect tag to use for getting the incident element count.
///
/// The \c AspectTagIncidentElementCount aspect tag causes the \c Fetch class to
/// obtain the number of indices that map to the current topology element.
///
struct AspectTagIncidentElementCount
{
};

/// \brief The \c ExecutionSignature tag to get the number of incident elements.
///
/// In a topology map, there are \em visited and \em incident topology elements
/// specified. The scheduling occurs on the \em visited elements, and for each
/// \em visited element there is some number of incident \em incident elements
/// that are accessible. This \c ExecutionSignature tag provides the number of
/// these \em incident elements that are accessible.
///
struct IncidentElementCount : viskores::exec::arg::ExecutionSignatureTagBase
{
  static constexpr viskores::IdComponent INDEX = 1;
  using AspectTag = viskores::exec::arg::AspectTagIncidentElementCount;
};

template <typename FetchTag, typename ExecObjectType>
struct Fetch<FetchTag, viskores::exec::arg::AspectTagIncidentElementCount, ExecObjectType>
{
  using ValueType = viskores::IdComponent;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VISKORES_EXEC ValueType Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    return indices.GetIndicesIncident().GetNumberOfComponents();
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

#endif //viskores_exec_arg_IncidentElementCount_h
