#pragma once

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
struct GraphicsExtensionDispatchTable
{
  FOR_EACH_EXTENSION_FUNCTION(DEFINE_PROC_MEMBER);

  GraphicsExtensionDispatchTable() = default;
  void PopulateDispatchTable(XrInstance instance)
  {
    FOR_EACH_EXTENSION_FUNCTION(GET_INSTANCE_PROC_ADDRESS);
  }
};
} // namespace xr

#undef DEFINE_PROC_MEMBER
#undef GET_INSTANCE_PROC_ADDRESS
#undef FOR_EACH_EXTENSION_FUNCTION
