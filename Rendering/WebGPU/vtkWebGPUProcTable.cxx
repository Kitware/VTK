// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUProcTable.h"

#include "vtkDynamicLoader.h"

#if defined(_WIN32)
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global proc table instance
static vtkWebGPUProcTable g_vtkWebGPUProcTable;

// Impl struct holds the loaded library handle. Entry points are resolved one by
// one from that handle rather than through wgpuGetProcAddress: the function is a
// Dawn extension that wgpu-native exports as a stub which panics when called,
// and that panic crosses an extern "C" boundary and aborts the process.
struct vtkWebGPUProcTableImpl
{
  vtkLibHandle libHandle; // Handle from vtkDynamicLoader
};

#if !defined(__EMSCRIPTEN__)
//------------------------------------------------------------------------------
// Open a library given only its file name, letting the platform search for it.
//
// vtkDynamicLoader::OpenLibrary cannot be used here on Windows: it routes the
// name through Encoding::ToWindowsExtendedPath, which calls GetFullPathNameW and
// so resolves a bare name against the current working directory instead of
// searching PATH. LoadLibraryW applies the standard DLL search order, which is
// what a runtime-loaded implementation needs. If the library is already in the
// process it returns the same module and takes a reference, so the handle is
// still ours to close.
static vtkLibHandle vtkWebGPUProcTableOpenByName(const char* name, int openFlags)
{
#if defined(_WIN32)
  (void)openFlags;
  const int wideLength = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0);
  if (wideLength <= 0)
  {
    return NULL;
  }
  wchar_t* wideName = (wchar_t*)malloc((size_t)wideLength * sizeof(wchar_t));
  if (!wideName)
  {
    return NULL;
  }
  MultiByteToWideChar(CP_UTF8, 0, name, -1, wideName, wideLength);
  vtkLibHandle handle = LoadLibraryW(wideName);
  free(wideName);
  return handle;
#else
  return vtkDynamicLoader::OpenLibrary(name, openFlags);
#endif
}
#endif // !__EMSCRIPTEN__

//------------------------------------------------------------------------------
vtkWebGPUProcTable vtkWebGPUProcTableLoad(const char* libPath)
{
  if (g_vtkWebGPUProcTable != NULL)
  {
    // Already loaded
    return g_vtkWebGPUProcTable;
  }

#if defined(__EMSCRIPTEN__)
  // Emscripten links the implementation into the module through
  // --use-port=emdawnwebgpu, so there is no shared library to open and the wgpu*
  // entry points are already resolved. Hand back a table that owns no handle;
  // nothing resolves symbols through the table on this platform.
  (void)libPath;
  vtkWebGPUProcTableImpl* wasmTable =
    (vtkWebGPUProcTableImpl*)malloc(sizeof(vtkWebGPUProcTableImpl));
  if (!wasmTable)
  {
    return NULL;
  }
  wasmTable->libHandle = NULL;
  g_vtkWebGPUProcTable = wasmTable;
  return wasmTable;
#else

  // RTLDGlobal keeps the implementation's symbols globally visible, so code that
  // still calls the wgpu* entry points directly resolves against the library we
  // just loaded. vtkDynamicLoader maps this onto dlopen() or LoadLibrary() as
  // appropriate for the platform.
  //
  // Windows accepts no flag other than SearchBesideLibrary and rejects the call
  // outright otherwise, without recording an error. Global symbol visibility is
  // not a Windows concept in the first place, so ask for nothing there.
#if defined(_WIN32)
  const int openFlags = 0;
#else
  const int openFlags = vtksys::DynamicLoader::RTLDGlobal;
#endif

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
      handle = vtkWebGPUProcTableOpenByName(candidates[i], openFlags);
    }
  }

  if (!handle)
  {
    // The caller probes several candidates; it reports once when all have failed.
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

  g_vtkWebGPUProcTable = table;
  return table;
#endif
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
  if (!impl->libHandle || !procName.data)
  {
    return NULL;
  }

  // A string view carries an explicit length and is not required to be NUL
  // terminated, so copy it before handing it to the loader.
  size_t length = procName.length == WGPU_STRLEN ? strlen(procName.data) : procName.length;
  char* name = (char*)malloc(length + 1);
  if (!name)
  {
    return NULL;
  }
  memcpy(name, procName.data, length);
  name[length] = '\0';

  WGPUProc proc = (WGPUProc)vtkDynamicLoader::GetSymbolAddress(impl->libHandle, name);
  free(name);
  return proc;
}
