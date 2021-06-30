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
#include "vtkSetGet.h" // For vtkWarningMacro

#include <cstdlib>  // For std::getenv
#include <iostream> // For std::cerr
#include <string>   // For std::string

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
  const std::string backend(type);
  if (backend == "Sequential" && this->SequentialBackend)
    this->ActivatedBackend = BackendType::Sequential;
  else if (backend == "STDThread" && this->STDThreadBackend)
    this->ActivatedBackend = BackendType::STDThread;
  else if (backend == "TBB" && this->TBBBackend)
    this->ActivatedBackend = BackendType::TBB;
  else if (backend == "OpenMP" && this->OpenMPBackend)
    this->ActivatedBackend = BackendType::OpenMP;
  else
  {
    std::cerr << "WARNING: tried to use a non implemented SMPTools backend " << backend << "!\n";
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

} // namespace smp
} // namespace detail
} // namespace vtk
