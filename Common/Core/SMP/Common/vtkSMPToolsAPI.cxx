// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkSMPToolsAPI::vtkSMPToolsAPI()
{
#if VTK_SMP_ENABLE_SEQUENTIAL
  this->SequentialBackend = std::make_unique<vtkSMPToolsImpl<BackendType::Sequential>>();
#endif
#if VTK_SMP_ENABLE_STDTHREAD
  this->STDThreadBackend = std::make_unique<vtkSMPToolsImpl<BackendType::STDThread>>();
#endif
#if VTK_SMP_ENABLE_TBB
  this->TBBBackend = std::make_unique<vtkSMPToolsImpl<BackendType::TBB>>();
#endif
#if VTK_SMP_ENABLE_OPENMP
  this->OpenMPBackend = std::make_unique<vtkSMPToolsImpl<BackendType::OpenMP>>();
#endif

  // Set backend from env if set
  const char* vtkSMPBackendInUse = std::getenv("VTK_SMP_BACKEND_IN_USE");
  if (vtkSMPBackendInUse)
  {
    this->SetBackend(vtkSMPBackendInUse);
  }

  // Set max thread number from env
  this->RefreshNumberOfThread();
}

//------------------------------------------------------------------------------
// Must NOT be initialized. Default initialization to zero is necessary.
vtkSMPToolsAPI* vtkSMPToolsAPIInstanceAsPointer;

//------------------------------------------------------------------------------
vtkSMPToolsAPI& vtkSMPToolsAPI::GetInstance()
{
  return *vtkSMPToolsAPIInstanceAsPointer;
}

//------------------------------------------------------------------------------
void vtkSMPToolsAPI::ClassInitialize()
{
  if (!vtkSMPToolsAPIInstanceAsPointer)
  {
    vtkSMPToolsAPIInstanceAsPointer = new vtkSMPToolsAPI;
  }
}

//------------------------------------------------------------------------------
void vtkSMPToolsAPI::ClassFinalize()
{
  delete vtkSMPToolsAPIInstanceAsPointer;
  vtkSMPToolsAPIInstanceAsPointer = nullptr;
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
  {
    this->ActivatedBackend = BackendType::Sequential;
  }
  else if (backend == "STDTHREAD" && this->STDThreadBackend)
  {
    this->ActivatedBackend = BackendType::STDThread;
  }
  else if (backend == "TBB" && this->TBBBackend)
  {
    this->ActivatedBackend = BackendType::TBB;
  }
  else if (backend == "OPENMP" && this->OpenMPBackend)
  {
    this->ActivatedBackend = BackendType::OpenMP;
  }
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
int vtkSMPToolsAPI::GetEstimatedDefaultNumberOfThreads()
{
  switch (this->ActivatedBackend)
  {
    case BackendType::Sequential:
      return this->SequentialBackend->GetEstimatedDefaultNumberOfThreads();
    case BackendType::STDThread:
      return this->STDThreadBackend->GetEstimatedDefaultNumberOfThreads();
    case BackendType::TBB:
      return this->TBBBackend->GetEstimatedDefaultNumberOfThreads();
    case BackendType::OpenMP:
      return this->OpenMPBackend->GetEstimatedDefaultNumberOfThreads();
  }
  return 0;
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

//------------------------------------------------------------------------------
bool vtkSMPToolsAPI::GetSingleThread()
{
  // Currently, this will work as expected for one parallel area and or nested
  // parallel areas. If there are two or more parallel areas that are not nested,
  // this function will not work properly.
  switch (this->ActivatedBackend)
  {
    case BackendType::Sequential:
      return this->SequentialBackend->GetSingleThread();
    case BackendType::STDThread:
      return this->STDThreadBackend->GetSingleThread();
    case BackendType::TBB:
      return this->TBBBackend->GetSingleThread();
    case BackendType::OpenMP:
      return this->OpenMPBackend->GetSingleThread();
    default:
      return false;
  }
}

//------------------------------------------------------------------------------
// Must NOT be initialized. Default initialization to zero is necessary.
unsigned int vtkSMPToolsAPIInitializeCount;

//------------------------------------------------------------------------------
vtkSMPToolsAPIInitialize::vtkSMPToolsAPIInitialize()
{
  if (++vtkSMPToolsAPIInitializeCount == 1)
  {
    vtkSMPToolsAPI::ClassInitialize();
  }
}

//------------------------------------------------------------------------------
vtkSMPToolsAPIInitialize::~vtkSMPToolsAPIInitialize()
{
  if (--vtkSMPToolsAPIInitializeCount == 0)
  {
    vtkSMPToolsAPI::ClassFinalize();
  }
}

VTK_ABI_NAMESPACE_END
} // namespace smp
} // namespace detail
} // namespace vtk
