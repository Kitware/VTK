/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPTools.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMPTools.h"

#include "vtkSMP.h"

//------------------------------------------------------------------------------
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
