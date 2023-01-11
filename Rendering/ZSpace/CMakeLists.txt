set(classes
  vtkZSpaceCamera
  vtkZSpaceHardwarePicker
  vtkZSpaceInteractorStyle
  vtkZSpaceRayActor
  vtkZSpaceRenderer
  vtkZSpaceRenderWindowInteractor
  vtkZSpaceSDKManager)

set(ZSPACE_USE_COMPAT_SDK "ON" CACHE BOOL 
  "Set to \"ON\" to use the newer version of the zSpace SDK 
  (\"Core Compatibility SDK\") instead of the legacy one 
  (\"Core SDK\")")
mark_as_advanced(ZSPACE_USE_COMPAT_SDK)

if(ZSPACE_USE_COMPAT_SDK)
  list(APPEND classes vtkZSpaceCoreCompatibilitySDKManager)
  vtk_module_find_package(PACKAGE zSpaceCompat)
else()
  list(APPEND classes vtkZSpaceCoreSDKManager)
  vtk_module_find_package(PACKAGE zSpace)
endif()

vtk_module_add_module(VTK::RenderingZSpace
  CLASSES ${classes})

if(ZSPACE_USE_COMPAT_SDK)
  # Just provide the headers as there are no library
  # link during compile time
  vtk_module_link(VTK::RenderingZSpace
    PUBLIC zSpaceHeaders)
else()
  vtk_module_link(VTK::RenderingZSpace
    PUBLIC zSpace::zSpace)
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkZSpaceSDKVersion.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkZSpaceSDKVersion.h")
