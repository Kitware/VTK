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
#ifndef viskores_exec_TaskBase_h
#define viskores_exec_TaskBase_h

#include <viskores/Types.h>

#include <viskores/cont/internal/Hints.h>

#include <viskores/exec/internal/ErrorMessageBuffer.h>

namespace viskores
{
namespace exec
{

/// Base class for all classes that are used to marshal data from the invocation
/// parameters to the user worklets when invoked in the execution environment.
class TaskBase
{
};
}
} // namespace viskores::exec

#endif //viskores_exec_TaskBase_h
