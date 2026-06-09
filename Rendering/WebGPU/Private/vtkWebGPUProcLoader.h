// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUProcLoader_h
#define vtkWebGPUProcLoader_h

#include "vtkRenderingWebGPUModule.h"
#include <string>

/**
 * vtkWebGPUProcLoader provides C++ integration for runtime WebGPU library loading.
 * It ensures the proc table is initialized and provides access to WebGPU functions.
 *
 * Usage:
 *   vtkWebGPUProcLoader loader;
 *   if (loader.Load("libwgpu_dawn.so")) {
 *     WGPUInstance instance = wgpuCreateInstance(nullptr);
 *     ...
 *   }
 */
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUProcLoader
{
public:
  vtkWebGPUProcLoader();
  ~vtkWebGPUProcLoader();

  /**
   * Load a WebGPU implementation library.
   * If libPath is empty, tries standard library names.
   * Returns true on success, false on failure.
   */
  bool Load(const std::string& libPath = "");

  /**
   * Check if a library has been successfully loaded.
   */
  bool IsLoaded() const;

  /**
   * Get the last error message if Load() failed.
   */
  const std::string& GetError() const { return this->Error; }

  /**
   * Get the singleton instance (loads on first access).
   * Returns nullptr if load fails.
   */
  static vtkWebGPUProcLoader* GetInstance();

private:
  std::string Error;
  bool Loaded;
};

#endif // vtkWebGPUProcLoader_h
