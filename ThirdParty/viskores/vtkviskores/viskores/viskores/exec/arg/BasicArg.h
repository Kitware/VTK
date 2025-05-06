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
#ifndef viskores_exec_arg_BasicArg_h
#define viskores_exec_arg_BasicArg_h

#include <viskores/Types.h>

#include <viskores/exec/arg/AspectTagDefault.h>
#include <viskores/exec/arg/ExecutionSignatureTagBase.h>

namespace viskores
{
namespace exec
{
namespace arg
{

/// \brief The underlying tag for basic \c ExecutionSignature arguments.
///
/// The basic \c ExecutionSignature arguments of _1, _2, etc. are all
/// subclasses of \c BasicArg. They all make available the components of
/// this class.
///
template <viskores::IdComponent ControlSignatureIndex>
struct BasicArg : viskores::exec::arg::ExecutionSignatureTagBase
{
  static constexpr viskores::IdComponent INDEX = ControlSignatureIndex;
  using AspectTag = viskores::exec::arg::AspectTagDefault;
};
}
}
} // namespace viskores::exec::arg

#endif //viskores_exec_arg_BasicArg_h
