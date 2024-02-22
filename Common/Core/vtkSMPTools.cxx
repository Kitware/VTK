// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSMPTools.h"

#include "vtkSMP.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
const char* vtkSMPTools::GetBackend()
{
  auto& SMPToolsAPI = vtk::detail::smp::vtkSMPToolsAPI::GetInstance();
  return SMPToolsAPI.GetBackend();
}

//------------------------------------------------------------------------------
bool vtkSMPTools::SetBackend(const char* backend)
{
  auto& SMPToolsAPI = vtk::detail::smp::vtkSMPToolsAPI::GetInstance();
  return SMPToolsAPI.SetBackend(backend);
}

//------------------------------------------------------------------------------
void vtkSMPTools::Initialize(int numThreads)
{
  auto& SMPToolsAPI = vtk::detail::smp::vtkSMPToolsAPI::GetInstance();
  return SMPToolsAPI.Initialize(numThreads);
}

//------------------------------------------------------------------------------
int vtkSMPTools::GetEstimatedNumberOfThreads()
{
  auto& SMPToolsAPI = vtk::detail::smp::vtkSMPToolsAPI::GetInstance();
  return SMPToolsAPI.GetEstimatedNumberOfThreads();
}

//------------------------------------------------------------------------------
int vtkSMPTools::GetEstimatedDefaultNumberOfThreads()
{
  auto& SMPToolsAPI = vtk::detail::smp::vtkSMPToolsAPI::GetInstance();
  return SMPToolsAPI.GetEstimatedDefaultNumberOfThreads();
}

//------------------------------------------------------------------------------
void vtkSMPTools::SetNestedParallelism(bool isNested)
{
  auto& SMPToolsAPI = vtk::detail::smp::vtkSMPToolsAPI::GetInstance();
  return SMPToolsAPI.SetNestedParallelism(isNested);
}

//------------------------------------------------------------------------------
bool vtkSMPTools::GetNestedParallelism()
{
  auto& SMPToolsAPI = vtk::detail::smp::vtkSMPToolsAPI::GetInstance();
  return SMPToolsAPI.GetNestedParallelism();
}

//------------------------------------------------------------------------------
bool vtkSMPTools::IsParallelScope()
{
  auto& SMPToolsAPI = vtk::detail::smp::vtkSMPToolsAPI::GetInstance();
  return SMPToolsAPI.IsParallelScope();
}

//------------------------------------------------------------------------------
bool vtkSMPTools::GetSingleThread()
{
  auto& SMPToolsAPI = vtk::detail::smp::vtkSMPToolsAPI::GetInstance();
  return SMPToolsAPI.GetSingleThread();
}
VTK_ABI_NAMESPACE_END
