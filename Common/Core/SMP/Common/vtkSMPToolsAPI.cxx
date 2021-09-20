/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPToolsAPI.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "SMP/Common/vtkSMPToolsAPI.h"
#include "vtkSMP.h"    // For SMP preprocessor information
#include "vtkSetGet.h" // For vtkWarningMacro

#include <algorithm> // For std::toupper
#include <cstdlib>   // For std::getenv
#include <iostream>  // For std::cerr
#include <string>    // For std::string

namespace vtk
{
namespace detail
{
namespace smp
{

//------------------------------------------------------------------------------
vtkSMPToolsAPI::vtkSMPToolsAPI()
{
  // XXX(c++14): use std::make_unique
#if VTK_SMP_ENABLE_SEQUENTIAL
  this->SequentialBackend = std::unique_ptr<vtkSMPToolsImpl<BackendType::Sequential>>(
    new vtkSMPToolsImpl<BackendType::Sequential>());
#endif
#if VTK_SMP_ENABLE_STDTHREAD
  this->STDThreadBackend = std::unique_ptr<vtkSMPToolsImpl<BackendType::STDThread>>(
    new vtkSMPToolsImpl<BackendType::STDThread>());
#endif
#if VTK_SMP_ENABLE_TBB
  this->TBBBackend =
    std::unique_ptr<vtkSMPToolsImpl<BackendType::TBB>>(new vtkSMPToolsImpl<BackendType::TBB>());
#endif
#if VTK_SMP_ENABLE_OPENMP
  this->OpenMPBackend = std::unique_ptr<vtkSMPToolsImpl<BackendType::OpenMP>>(
    new vtkSMPToolsImpl<BackendType::OpenMP>());
#endif

  // Set backend from env if setted
  const char* vtkSMPBackendInUse = std::getenv("VTK_SMP_BACKEND_IN_USE");
  if (vtkSMPBackendInUse)
  {
    this->SetBackend(vtkSMPBackendInUse);
  }

  // Set max thread number from env
  this->RefreshNumberOfThread();
}

//------------------------------------------------------------------------------
vtkSMPToolsAPI& vtkSMPToolsAPI::GetInstance()
{
  static vtkSMPToolsAPI instance;
  return instance;
}

//------------------------------------------------------------------------------
BackendType vtkSMPToolsAPI::GetBackendType()
{
  return this->ActivatedBackend;
}

//------------------------------------------------------------------------------
const char* vtkSMPToolsAPI::GetBackend()
{
  switch (this->ActivatedBackend)
  {
    case BackendType::Sequential:
      return "Sequential";
    case BackendType::STDThread:
      return "STDThread";
    case BackendType::TBB:
      return "TBB";
    case BackendType::OpenMP:
      return "OpenMP";
  }
  return nullptr;
}

//------------------------------------------------------------------------------
bool vtkSMPToolsAPI::SetBackend(const char* type)
{
  std::string backend(type);
  std::transform(backend.cbegin(), backend.cend(), backend.begin(), ::toupper);
  if (backend == "SEQUENTIAL" && this->SequentialBackend)
    this->ActivatedBackend = BackendType::Sequential;
  else if (backend == "STDTHREAD" && this->STDThreadBackend)
    this->ActivatedBackend = BackendType::STDThread;
  else if (backend == "TBB" && this->TBBBackend)
    this->ActivatedBackend = BackendType::TBB;
  else if (backend == "OPENMP" && this->OpenMPBackend)
    this->ActivatedBackend = BackendType::OpenMP;
  else
  {
    std::cerr << "WARNING: tried to use a non implemented SMPTools backend \"" << type << "\"!\n";
    std::cerr << "The available backends are:" << (this->SequentialBackend ? " \"Sequential\"" : "")
              << (this->STDThreadBackend ? " \"STDThread\"" : "")
              << (this->TBBBackend ? " \"TBB\"" : "") << (this->OpenMPBackend ? " \"OpenMP\"" : "")
              << "\n";
    std::cerr << "Using " << this->GetBackend() << " instead." << std::endl;
    return false;
  }
  this->RefreshNumberOfThread();
  return true;
}

//------------------------------------------------------------------------------
void vtkSMPToolsAPI::Initialize(int numThreads)
{
  this->DesiredNumberOfThread = numThreads;
  this->RefreshNumberOfThread();
}

//------------------------------------------------------------------------------
void vtkSMPToolsAPI::RefreshNumberOfThread()
{
  const int numThreads = this->DesiredNumberOfThread;
  switch (this->ActivatedBackend)
  {
    case BackendType::Sequential:
      this->SequentialBackend->Initialize(numThreads);
      break;
    case BackendType::STDThread:
      this->STDThreadBackend->Initialize(numThreads);
      break;
    case BackendType::TBB:
      this->TBBBackend->Initialize(numThreads);
      break;
    case BackendType::OpenMP:
      this->OpenMPBackend->Initialize(numThreads);
      break;
  }
}

//------------------------------------------------------------------------------
int vtkSMPToolsAPI::GetEstimatedNumberOfThreads()
{
  switch (this->ActivatedBackend)
  {
    case BackendType::Sequential:
      return this->SequentialBackend->GetEstimatedNumberOfThreads();
    case BackendType::STDThread:
      return this->STDThreadBackend->GetEstimatedNumberOfThreads();
    case BackendType::TBB:
      return this->TBBBackend->GetEstimatedNumberOfThreads();
    case BackendType::OpenMP:
      return this->OpenMPBackend->GetEstimatedNumberOfThreads();
  }
  return 0;
}

//------------------------------------------------------------------------------
void vtkSMPToolsAPI::SetNestedParallelism(bool isNested)
{
  switch (this->ActivatedBackend)
  {
    case BackendType::Sequential:
      this->SequentialBackend->SetNestedParallelism(isNested);
      break;
    case BackendType::STDThread:
      this->STDThreadBackend->SetNestedParallelism(isNested);
      break;
    case BackendType::TBB:
      this->TBBBackend->SetNestedParallelism(isNested);
      break;
    case BackendType::OpenMP:
      this->OpenMPBackend->SetNestedParallelism(isNested);
      break;
  }
}

//------------------------------------------------------------------------------
bool vtkSMPToolsAPI::GetNestedParallelism()
{
  switch (this->ActivatedBackend)
  {
    case BackendType::Sequential:
      return this->SequentialBackend->GetNestedParallelism();
    case BackendType::STDThread:
      return this->STDThreadBackend->GetNestedParallelism();
    case BackendType::TBB:
      return this->TBBBackend->GetNestedParallelism();
    case BackendType::OpenMP:
      return this->OpenMPBackend->GetNestedParallelism();
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkSMPToolsAPI::IsParallelScope()
{
  switch (this->ActivatedBackend)
  {
    case BackendType::Sequential:
      return this->SequentialBackend->IsParallelScope();
    case BackendType::STDThread:
      return this->STDThreadBackend->IsParallelScope();
    case BackendType::TBB:
      return this->TBBBackend->IsParallelScope();
    case BackendType::OpenMP:
      return this->OpenMPBackend->IsParallelScope();
  }
  return false;
}

} // namespace smp
} // namespace detail
} // namespace vtk
