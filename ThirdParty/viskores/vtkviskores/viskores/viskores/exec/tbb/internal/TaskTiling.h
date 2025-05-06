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
#ifndef viskores_exec_tbb_internal_TaskTiling_h
#define viskores_exec_tbb_internal_TaskTiling_h

#include <viskores/exec/serial/internal/TaskTiling.h>

namespace viskores
{
namespace exec
{
namespace tbb
{
namespace internal
{

using TaskTiling1D = viskores::exec::serial::internal::TaskTiling1D;
using TaskTiling3D = viskores::exec::serial::internal::TaskTiling3D;
}
}
}
} // namespace viskores::exec::tbb::internal

#endif //viskores_exec_tbb_internal_TaskTiling_h
