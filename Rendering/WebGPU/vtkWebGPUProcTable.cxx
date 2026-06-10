// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUProcTable.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global proc table instance
static vtkWebGPUProcTable g_vtkWebGPUProcTable = NULL;

// Impl struct holds the loaded library handle and bootstrap function pointer
struct vtkWebGPUProcTableImpl
{
  void* libHandle;                           // Handle from dlopen()
  WGPUProcGetProcAddress getProcAddressFunc; // Bootstrap function
};

//------------------------------------------------------------------------------
vtkWebGPUProcTable vtkWebGPUProcTableLoad(const char* libPath)
{
  if (g_vtkWebGPUProcTable != NULL)
  {
    // Already loaded
    return g_vtkWebGPUProcTable;
  }

  // Determine which library to load
  const char* lib = libPath ? libPath : "libwgpu_dawn.so";

  // Try to load the WebGPU implementation library
  void* handle = dlopen(lib, RTLD_LAZY | RTLD_GLOBAL);
  if (!handle)
  {
    // Try alternative library names (including webgpu_dawn naming variant)
    const char* alternatives[] = { "libwebgpu_dawn.so", "libwgpu_dawn.so.0", "libwebgpu_dawn.so.0",
      "libwgpu_dawn.dylib", "libwebgpu_dawn.dylib", "wgpu_dawn.dll", "webgpu_dawn.dll", NULL };

    for (int i = 0; alternatives[i] != NULL; ++i)
    {
      if (libPath == NULL) // Only try alternatives if no explicit path given
      {
        handle = dlopen(alternatives[i], RTLD_LAZY | RTLD_GLOBAL);
        if (handle)
          break;
      }
    }

    if (!handle)
    {
      fprintf(stderr, "vtkWebGPUProcTable: failed to load WebGPU library: %s\n", dlerror());
      return NULL;
    }
  }

  // Get the wgpuGetProcAddress bootstrap function
  WGPUProcGetProcAddress getProcAddress =
    (WGPUProcGetProcAddress)dlsym(handle, "wgpuGetProcAddress");
  if (!getProcAddress)
  {
    fprintf(stderr, "vtkWebGPUProcTable: wgpuGetProcAddress not found: %s\n", dlerror());
    dlclose(handle);
    return NULL;
  }

  // Create proc table instance
  vtkWebGPUProcTableImpl* table = (vtkWebGPUProcTableImpl*)malloc(sizeof(vtkWebGPUProcTableImpl));
  if (!table)
  {
    dlclose(handle);
    return NULL;
  }

  table->libHandle = handle;
  table->getProcAddressFunc = getProcAddress;

  g_vtkWebGPUProcTable = table;
  return table;
}

//------------------------------------------------------------------------------
void vtkWebGPUProcTableRelease(vtkWebGPUProcTable table)
{
  if (!table)
    return;

  vtkWebGPUProcTableImpl* impl = (vtkWebGPUProcTableImpl*)table;

  if (impl->libHandle)
  {
    dlclose(impl->libHandle);
  }

  free(impl);

  if (g_vtkWebGPUProcTable == table)
  {
    g_vtkWebGPUProcTable = NULL;
  }
}

//------------------------------------------------------------------------------
vtkWebGPUProcTable vtkWebGPUProcTableGet(void)
{
  return g_vtkWebGPUProcTable;
}

//------------------------------------------------------------------------------
void vtkWebGPUProcTableSet(vtkWebGPUProcTable table)
{
  g_vtkWebGPUProcTable = table;
}

//------------------------------------------------------------------------------
WGPUProc vtkWebGPUProcTableGetProc(vtkWebGPUProcTable table, WGPUStringView procName)
{
  if (!table)
  {
    table = g_vtkWebGPUProcTable;
  }

  if (!table)
  {
    return NULL;
  }

  vtkWebGPUProcTableImpl* impl = (vtkWebGPUProcTableImpl*)table;
  return impl->getProcAddressFunc(procName);
}
