// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "SMP/Common/vtkSMPToolsImpl.h"
#include "SMP/Sequential/vtkSMPToolsImpl.txx"

namespace vtk
{
namespace detail
{
namespace smp
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
template <>
void vtkSMPToolsImpl<BackendType::Sequential>::Initialize(int)
{
}

//------------------------------------------------------------------------------
template <>
int vtkSMPToolsImpl<BackendType::Sequential>::GetEstimatedNumberOfThreads()
{
  return 1;
}

//------------------------------------------------------------------------------
template <>
bool vtkSMPToolsImpl<BackendType::Sequential>::GetSingleThread()
{
  return true;
}

VTK_ABI_NAMESPACE_END
} // namespace smp
} // namespace detail
} // namespace vtk
