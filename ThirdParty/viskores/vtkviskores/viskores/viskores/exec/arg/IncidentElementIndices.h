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
#ifndef viskores_exec_arg_IncidentElementIndices_h
#define viskores_exec_arg_IncidentElementIndices_h

#include <viskores/exec/arg/ExecutionSignatureTagBase.h>
#include <viskores/exec/arg/Fetch.h>
#include <viskores/exec/arg/FetchExtrude.h>

namespace viskores
{
namespace exec
{
namespace arg
{

/// \brief The \c ExecutionSignature tag to get the indices of visited elements.
///
/// In a topology map, there are \em visited and \em incident topology elements
/// specified. The scheduling occurs on the \em visited elements, and for each
/// \em visited element there is some number of incident \em incident elements
/// that are accessible. This \c ExecutionSignature tag provides the indices of
/// the \em incident elements that are incident to the current \em visited
/// element.
///
struct IncidentElementIndices : viskores::exec::arg::ExecutionSignatureTagBase
{
  static constexpr viskores::IdComponent INDEX = 1;
  using AspectTag = viskores::exec::arg::AspectTagIncidentElementIndices;
};
}
}
} // namespace viskores::exec::arg

#endif //viskores_exec_arg_IncidentElementIndices_h
