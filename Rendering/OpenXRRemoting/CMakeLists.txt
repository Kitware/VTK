vtk_module_find_package(PACKAGE OpenXRRemoting)

set(classes vtkOpenXRRemotingRenderWindow)

set(nowrap_classes
  vtkOpenXRManagerD3DGraphics
  vtkOpenXRManagerRemoteConnection
  )

vtk_module_add_module(VTK::RenderingOpenXRRemoting
  CLASSES ${classes}
  NOWRAP_CLASSES ${nowrap_classes}
  )

# When this module is enabled, the ExtensionDispatchTable struct must define
# additional functions for remoting (see Rendering/OpenXR/XrExtensions.h). 
# Tell the VTK::RenderingOpenXR module to add the required definitions:
target_compile_definitions(RenderingOpenXR PUBLIC VTK_OPENXR_USE_REMOTING=1)
vtk_module_link(VTK::RenderingOpenXR PUBLIC OpenXR::Remoting)
