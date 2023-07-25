// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Microsoft
// SPDX-License-Identifier: BSD-3-Clause AND Apache-2.0
/**
 * @file   XrGraphicsExtensions.h
 *
 * @brief  Load OpenXR extensions for the defined graphics backend.
 *
 * Provides the GraphicsExtensionDispatchTable struct to load platform-specific
 * extensions at runtime for the current XrInstance.
 * XR_USE_GRAPHICS_API_D3D11 and/or XR_USE_GRAPHICS_API_OPENGL must be defined
 * prior to including this header to enable the expected graphics backend.
 *
 * File adapted from:
 * https://github.com/microsoft/MixedReality-HolographicRemoting-Samples/blob/f6b55479646bda3bffea58bb3e9c9d9c5e0ab177/remote_openxr/desktop/XrUtility/XrExtensions.h
 *
 * @sa
 * vtkOpenXrPlatform.h XrExtensions.h XrConnectionExtensions.h
 */

#ifndef XrGraphicsExtensions_h
#define XrGraphicsExtensions_h

#ifdef XR_USE_PLATFORM_WIN32
#define FOR_EACH_WIN32_EXTENSION_FUNCTION(_) _(xrConvertWin32PerformanceCounterToTimeKHR)
#else
#define FOR_EACH_WIN32_EXTENSION_FUNCTION(_)
#endif

#ifdef XR_USE_GRAPHICS_API_D3D11
#define FOR_EACH_D3D11_EXTENSION_FUNCTION(_) _(xrGetD3D11GraphicsRequirementsKHR)
#else
#define FOR_EACH_D3D11_EXTENSION_FUNCTION(_)
#endif

#ifdef XR_USE_GRAPHICS_API_OPENGL
#define FOR_EACH_OPENGL_EXTENSION_FUNCTION(_) _(xrGetOpenGLGraphicsRequirementsKHR)
#else
#define FOR_EACH_OPENGL_EXTENSION_FUNCTION(_)
#endif

#define FOR_EACH_EXTENSION_FUNCTION(_)                                                             \
  FOR_EACH_WIN32_EXTENSION_FUNCTION(_)                                                             \
  FOR_EACH_OPENGL_EXTENSION_FUNCTION(_)                                                            \
  FOR_EACH_D3D11_EXTENSION_FUNCTION(_)

#define GET_INSTANCE_PROC_ADDRESS(name)                                                            \
  (void)xrGetInstanceProcAddr(                                                                     \
    instance, #name, reinterpret_cast<PFN_xrVoidFunction*>(const_cast<PFN_##name*>(&name)));
#define DEFINE_PROC_MEMBER(name) PFN_##name name{ nullptr };

namespace xr
{
VTK_ABI_NAMESPACE_BEGIN
struct GraphicsExtensionDispatchTable
{
  FOR_EACH_EXTENSION_FUNCTION(DEFINE_PROC_MEMBER);

  GraphicsExtensionDispatchTable() = default;
  void PopulateDispatchTable(XrInstance instance)
  {
    FOR_EACH_EXTENSION_FUNCTION(GET_INSTANCE_PROC_ADDRESS);
  }
};
VTK_ABI_NAMESPACE_END
} // namespace xr

#undef DEFINE_PROC_MEMBER
#undef GET_INSTANCE_PROC_ADDRESS
#undef FOR_EACH_EXTENSION_FUNCTION

#endif
