// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUInternalsCallbacks_h
#define vtkWebGPUInternalsCallbacks_h

#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtk_wgpu.h"

VTK_ABI_NAMESPACE_BEGIN

/**
 * Utilitary class for various WebGPU callbacks methods
 */
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUInternalsCallbacks
{
public:
  /**
   * Callback called when the WGPU device is lost
   */
  static void DeviceLostCallback(
    WGPUDeviceLostReason reason, char const* message, void* userdata = nullptr);

  /**
   * Callback called when an error occured in the manipulation of WGPU
   */
  static void UncapturedErrorCallback(
    WGPUErrorType type, char const* message, void* userdata = nullptr);

  /*
   * Logs a VTK error based on the WGPU error type and message given
   */
  static void PrintWGPUError(WGPUErrorType type, const char* message, void* userdata = nullptr);
};

VTK_ABI_NAMESPACE_END

#endif
