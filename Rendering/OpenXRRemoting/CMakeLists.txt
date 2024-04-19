vtk_module_find_package(PACKAGE OpenXRRemoting)

set(classes vtkOpenXRRemotingRenderWindow)

set(nowrap_classes
  vtkOpenXRManagerD3DGraphics
  vtkOpenXRManagerRemoteConnection
  )

set(nowrap_headers XrConnectionExtensions.h)

vtk_module_add_module(VTK::RenderingOpenXRRemoting
  CLASSES ${classes}
  NOWRAP_CLASSES ${nowrap_classes}
  NOWRAP_HEADERS ${nowrap_headers}
  )

vtk_module_link(VTK::RenderingOpenXRRemoting PUBLIC OpenXR::Remoting)

vtk_add_test_mangling(VTK::RenderingOpenXRRemoting)
