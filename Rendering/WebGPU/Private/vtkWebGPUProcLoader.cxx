// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUProcLoader.h"
#include "Private/vtkWebGPUProcDispatch.h"
#include "vtkDynamicLoader.h"
#include "vtkWebGPUProcTable.h"

#include <cstdlib> // for std::getenv
#include <sstream>

static vtkWebGPUProcLoader* g_Instance = nullptr;

//------------------------------------------------------------------------------
vtkWebGPUProcLoader::vtkWebGPUProcLoader()
  : Loaded(false)
{
}

//------------------------------------------------------------------------------
vtkWebGPUProcLoader::~vtkWebGPUProcLoader()
{
  // Release the proc table if we own it
  if (this->Loaded)
  {
    vtkWebGPUProcTable table = vtkWebGPUProcTableGet();
    if (table)
    {
      vtkWebGPUProcTableRelease(table);
    }
  }
}

//------------------------------------------------------------------------------
bool vtkWebGPUProcLoader::Load(const std::string& libPath)
{
  if (this->Loaded)
  {
    return true; // Already loaded
  }

  // Load the proc table
  const char* path = libPath.empty() ? nullptr : libPath.c_str();
  vtkWebGPUProcTable table = vtkWebGPUProcTableLoad(path);

  if (!table)
  {
    std::ostringstream ss;
    ss << "Failed to load WebGPU library";
    if (!libPath.empty())
    {
      ss << " " << libPath;
    }
    if (const char* reason = vtkDynamicLoader::LastError())
    {
      ss << ": " << reason;
    }
    this->Error = ss.str();
    return false;
  }

#if !defined(__EMSCRIPTEN__)
  // Fill the dispatch table before anything can call through it.
  auto resolveOne = [](WGPUStringView name) -> WGPUProc
  { return vtkWebGPUProcTableGetProc(vtkWebGPUProcTableGet(), name); };
  if (const char* missing = vtkWebGPUProcDispatchResolve(resolveOne))
  {
    std::ostringstream ss;
    ss << "The WebGPU library";
    if (!libPath.empty())
    {
      ss << " at " << libPath;
    }
    ss << " does not provide " << missing;
    this->Error = ss.str();
    return false;
  }
#endif

  this->Implementation = vtkWebGPUProcLoader::DetectImplementation();

  this->Loaded = true;
  return true;
}

//------------------------------------------------------------------------------
vtkWebGPUProcLoader::ImplementationType vtkWebGPUProcLoader::DetectImplementation()
{
#if defined(__EMSCRIPTEN__)
  // --use-port=emdawnwebgpu links Dawn into the module.
  return vtkWebGPUProcLoader::Dawn;
#else
  // wgpuGenerateReport comes from wgpu.h, the extension header wgpu-native ships
  // alongside webgpu.h. Dawn does not declare or export it.
  const WGPUStringView marker{ "wgpuGenerateReport", WGPU_STRLEN };
  if (vtkWebGPUProcTableGetProc(vtkWebGPUProcTableGet(), marker) != nullptr)
  {
    return vtkWebGPUProcLoader::WgpuNative;
  }
  return vtkWebGPUProcLoader::Dawn;
#endif
}

//------------------------------------------------------------------------------
bool vtkWebGPUProcLoader::IsLoaded() const
{
  return this->Loaded;
}

//------------------------------------------------------------------------------
vtkWebGPUProcLoader* vtkWebGPUProcLoader::GetInstance()
{
  if (!g_Instance)
  {
    g_Instance = new vtkWebGPUProcLoader();
    // VTK_WEBGPU_LIBRARY names the implementation to load. Without it the loader
    // tries the names implementations are known by, then the one this build was
    // configured against.
    const char* configured = std::getenv("VTK_WEBGPU_LIBRARY");
    bool loaded = g_Instance->Load(configured ? configured : "");
#if defined(VTK_WEBGPU_CONFIGURED_LIBRARY)
    if (!loaded && !configured)
    {
      loaded = g_Instance->Load(VTK_WEBGPU_CONFIGURED_LIBRARY);
    }
#endif
    if (!loaded)
    {
      delete g_Instance;
      g_Instance = nullptr;
    }
  }
  return g_Instance;
}
