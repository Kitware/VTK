// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUProcTable_h
#define vtkWebGPUProcTable_h

#include "vtkRenderingWebGPUModule.h"
#include "webgpu/webgpu.h"

// Opaque handle to the proc table
struct vtkWebGPUProcTableImpl;
typedef struct vtkWebGPUProcTableImpl* vtkWebGPUProcTable;

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * Load a WebGPU implementation at runtime using dlopen.
   * This allows users to swap implementations without recompiling VTK.
   *
   * @param libPath  Path to the WebGPU implementation library (e.g., "libwgpu_dawn.so").
   *                 If NULL, searches standard library paths.
   * @return         Opaque proc table handle on success, NULL on failure.
   *
   * Must be called before any WebGPU functions are invoked.
   */
  vtkWebGPUProcTable vtkWebGPUProcTableLoad(const char* libPath);

  /**
   * Release a proc table and close the loaded library.
   */
  void vtkWebGPUProcTableRelease(vtkWebGPUProcTable table);

  /**
   * Get the global proc table instance.
   * Returns NULL if no proc table has been loaded yet.
   */
  vtkWebGPUProcTable vtkWebGPUProcTableGet(void);

  /**
   * Set the global proc table instance (for internal use).
   */
  void vtkWebGPUProcTableSet(vtkWebGPUProcTable table);

  /**
   * Get a function pointer from the proc table by name.
   * This wraps wgpuGetProcAddress() from the loaded implementation.
   */
  WGPUProc vtkWebGPUProcTableGetProc(vtkWebGPUProcTable table, WGPUStringView procName);

#ifdef __cplusplus
}
#endif

#endif // vtkWebGPUProcTable_h
