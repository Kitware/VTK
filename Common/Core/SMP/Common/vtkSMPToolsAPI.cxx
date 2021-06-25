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
  // Set backend from env if existing
  const char* vtkSMPBackendInUse = std::getenv("VTK_SMP_BACKEND_IN_USE");
  if (vtkSMPBackendInUse)
  {
    this->SetBackend(vtkSMPBackendInUse);
  }
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
void vtkSMPToolsAPI::SetBackend(const char* type)
{
  const std::string backend(type);
  if (backend == "Sequential")
    this->ActivatedBackend = BackendType::Sequential;
  else if (backend == "STDThread")
    this->ActivatedBackend = BackendType::STDThread;
  else if (backend == "TBB")
    this->ActivatedBackend = BackendType::TBB;
  else if (backend == "OpenMP")
    this->ActivatedBackend = BackendType::OpenMP;
  else
  {
    std::cerr << "WARNING: tried to use a non implemented SMPTools backend " << backend << "!\n";
    std::cerr.flush();
  }
}

//------------------------------------------------------------------------------
void vtkSMPToolsAPI::Initialize(int numThreads)
{
  switch (this->ActivatedBackend)
  {
    case BackendType::Sequential:
      this->SequentialBackend.Initialize(numThreads);
      break;
    case BackendType::STDThread:
      this->STDThreadBackend.Initialize(numThreads);
      break;
    case BackendType::TBB:
      this->TBBBackend.Initialize(numThreads);
      break;
    case BackendType::OpenMP:
      this->OpenMPBackend.Initialize(numThreads);
      break;
  }
}

//------------------------------------------------------------------------------
int vtkSMPToolsAPI::GetEstimatedNumberOfThreads()
{
  switch (this->ActivatedBackend)
  {
    case BackendType::Sequential:
      return this->SequentialBackend.GetEstimatedNumberOfThreads();
    case BackendType::STDThread:
      return this->STDThreadBackend.GetEstimatedNumberOfThreads();
    case BackendType::TBB:
      return this->TBBBackend.GetEstimatedNumberOfThreads();
    case BackendType::OpenMP:
      return this->OpenMPBackend.GetEstimatedNumberOfThreads();
  }
  return 0;
}

} // namespace smp
} // namespace detail
} // namespace vtk
