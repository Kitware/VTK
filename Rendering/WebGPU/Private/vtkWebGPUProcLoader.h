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
 * This is a singleton: construction and loading are private so that
 * GetInstance() owns the one and only instance.
 *
 * @code{.cpp}
 * if (auto* loader = vtkWebGPUProcLoader::GetInstance())
 * {
 *   WGPUInstance instance = wgpuCreateInstance(nullptr);
 *   // ...
 * }
 * @endcode
 *
 * @note The proc table is process wide. vtkWebGPUProcTableLoad() hands back the
 * existing global table without taking a reference, so a second instance of
 * this class would release a table still in use by the first when it is
 * destroyed. Keeping the constructor private makes that unrepresentable.
 */
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUProcLoader
{
public:
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
   *
   * The library to load is taken from the `VTK_WEBGPU_LIBRARY` environment
   * variable when it is set; otherwise the names implementations are commonly
   * installed under are tried in turn.
   */
  static vtkWebGPUProcLoader* GetInstance();

  vtkWebGPUProcLoader(const vtkWebGPUProcLoader&) = delete;
  void operator=(const vtkWebGPUProcLoader&) = delete;

private:
  vtkWebGPUProcLoader();
  ~vtkWebGPUProcLoader();

  /**
   * Load a WebGPU implementation library.
   * If libPath is empty, tries standard library names.
   * Returns true on success, false on failure.
   */
  bool Load(const std::string& libPath = "");

  std::string Error;
  bool Loaded;
};

#endif // vtkWebGPUProcLoader_h
