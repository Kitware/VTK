// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUProcTable.h"

#include "vtkDynamicLoader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global proc table instance
static vtkWebGPUProcTable g_vtkWebGPUProcTable;

// Impl struct holds the loaded library handle and bootstrap function pointer
struct vtkWebGPUProcTableImpl
{
  vtkLibHandle libHandle;                    // Handle from vtkDynamicLoader
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

  // RTLDGlobal keeps the implementation's symbols globally visible, so code that
  // still calls the wgpu* entry points directly resolves against the library we
  // just loaded. vtkDynamicLoader maps this onto dlopen() or LoadLibrary() as
  // appropriate for the platform.
  const int openFlags = vtksys::DynamicLoader::RTLDGlobal;

  vtkLibHandle handle = NULL;
  if (libPath)
  {
    handle = vtkDynamicLoader::OpenLibrary(libPath, openFlags);
  }
  else
  {
    // No explicit path, so try the names each implementation is known by. The
    // list spans every platform on purpose; names that cannot exist on the
    // current one simply fail to open. vtkDynamicLoader::LibExtension() is not
    // used here because it reports the module suffix, which is ".so" even on
    // macOS where these are ".dylib".
    const char* candidates[] = { "libwgpu_dawn.so", "libwebgpu_dawn.so", "libwgpu_dawn.so.0",
      "libwebgpu_dawn.so.0", "libwgpu_dawn.dylib", "libwebgpu_dawn.dylib", "wgpu_dawn.dll",
      "webgpu_dawn.dll", NULL };
    for (int i = 0; candidates[i] != NULL && !handle; ++i)
    {
      handle = vtkDynamicLoader::OpenLibrary(candidates[i], openFlags);
    }
  }

  if (!handle)
  {
    fprintf(stderr, "vtkWebGPUProcTable: failed to load WebGPU library: %s\n",
      vtkDynamicLoader::LastError());
    return NULL;
  }

  // Get the wgpuGetProcAddress bootstrap function
  WGPUProcGetProcAddress getProcAddress =
    (WGPUProcGetProcAddress)vtkDynamicLoader::GetSymbolAddress(handle, "wgpuGetProcAddress");
  if (!getProcAddress)
  {
    fprintf(stderr, "vtkWebGPUProcTable: wgpuGetProcAddress not found: %s\n",
      vtkDynamicLoader::LastError());
    vtkDynamicLoader::CloseLibrary(handle);
    return NULL;
  }

  // Create proc table instance
  vtkWebGPUProcTableImpl* table = (vtkWebGPUProcTableImpl*)malloc(sizeof(vtkWebGPUProcTableImpl));
  if (!table)
  {
    vtkDynamicLoader::CloseLibrary(handle);
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
    vtkDynamicLoader::CloseLibrary(impl->libHandle);
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
