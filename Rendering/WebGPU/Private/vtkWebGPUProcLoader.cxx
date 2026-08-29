// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUProcLoader.h"
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
      ss << ": " << libPath;
    }
    this->Error = ss.str();
    return false;
  }

  this->Loaded = true;
  return true;
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
    // VTK_WEBGPU_LIBRARY names the implementation to load. Without it the
    // loader falls back to the names implementations are known by, which is
    // what an installed Dawn or wgpu-native answers to.
    const char* configured = std::getenv("VTK_WEBGPU_LIBRARY");
    if (!g_Instance->Load(configured ? configured : ""))
    {
      delete g_Instance;
      g_Instance = nullptr;
    }
  }
  return g_Instance;
}
