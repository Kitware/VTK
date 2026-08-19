// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @file vtkWebGPUImplExtensions.h
 * @brief Implementation-specific WebGPU structs that upstream webgpu-headers does not declare.
 *
 * VTK compiles against the upstream webgpu-headers C API, which by design only
 * covers the standard surface. Implementations extend it through `nextInChain`
 * using `WGPUSType` values from blocks upstream reserves for them (Emscripten
 * gets 0x0004xxxx, Dawn 0x0005xxxx). The few extensions VTK uses are declared
 * here rather than by pulling in an implementation's own copy of `webgpu.h`,
 * which would redefine the whole standard API.
 *
 * Each declaration must match the implementation's layout exactly; both are
 * plain `WGPUChainedStruct` extensions and have been stable across releases.
 *
 * This is a private implementation header; it is not installed.
 */

#ifndef vtkWebGPUImplExtensions_h
#define vtkWebGPUImplExtensions_h

#include "vtk_wgpu.h" // for the WebGPU C API

#ifdef __EMSCRIPTEN__
// Selects the HTML canvas a surface renders into. Provided by emdawnwebgpu.
#define VTK_WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector ((WGPUSType)0x00040000)

typedef struct VTKWGPUEmscriptenSurfaceSourceCanvasHTMLSelector
{
  WGPUChainedStruct chain;
  WGPUStringView selector;
} VTKWGPUEmscriptenSurfaceSourceCanvasHTMLSelector;

#define VTK_WGPU_EMSCRIPTEN_SURFACE_SOURCE_CANVAS_HTML_SELECTOR_INIT                               \
  VTKWGPUEmscriptenSurfaceSourceCanvasHTMLSelector                                                 \
  {                                                                                                \
    /*.chain=*/{ /*.next=*/nullptr,                                                                \
      /*.sType=*/VTK_WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector },                        \
      /*.selector=*/WGPUStringView                                                                 \
    {                                                                                              \
      nullptr, WGPU_STRLEN                                                                         \
    }                                                                                              \
  }
#endif // __EMSCRIPTEN__

#if VTK_USE_DAWN_WEBGPU
// Reports the adapter's power preference. Chained onto WGPUAdapterInfo by Dawn.
#define VTK_WGPUSType_DawnAdapterPropertiesPowerPreference ((WGPUSType)0x00050008)

typedef struct VTKWGPUDawnAdapterPropertiesPowerPreference
{
  WGPUChainedStruct chain;
  WGPUPowerPreference powerPreference;
} VTKWGPUDawnAdapterPropertiesPowerPreference;

#define VTK_WGPU_DAWN_ADAPTER_PROPERTIES_POWER_PREFERENCE_INIT                                     \
  VTKWGPUDawnAdapterPropertiesPowerPreference                                                      \
  {                                                                                                \
    /*.chain=*/{ /*.next=*/nullptr,                                                                \
      /*.sType=*/VTK_WGPUSType_DawnAdapterPropertiesPowerPreference },                             \
      /*.powerPreference=*/WGPUPowerPreference_Undefined                                           \
  }
#endif // VTK_USE_DAWN_WEBGPU

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUImplExtensions.h
