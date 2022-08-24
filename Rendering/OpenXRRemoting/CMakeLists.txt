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

vtk_module_link(VTK::RenderingOpenXR PUBLIC OpenXR::Remoting)
