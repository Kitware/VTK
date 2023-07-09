set(classes
  vtkZSpaceCamera
  vtkZSpaceHardwarePicker
  vtkZSpaceInteractorStyle
  vtkZSpaceRayActor
  vtkZSpaceRenderer
  vtkZSpaceRenderWindowInteractor
  vtkZSpaceSDKManager)

set(private_classes)

set(ZSPACE_USE_COMPAT_SDK "ON" CACHE BOOL
  "Set to \"ON\" to use the newer version of the zSpace SDK
  (\"Core Compatibility SDK\") instead of the legacy one
  (\"Core SDK\")")
mark_as_advanced(ZSPACE_USE_COMPAT_SDK)

if(ZSPACE_USE_COMPAT_SDK)
  # No need to search for specific package in that case.
  # Headers are included in the zSpaceCompat subfolder,
  # and there is no static library to link at compile time.
  list(APPEND private_classes
    vtkZSpaceCoreCompatibilitySDKManager)
else()
  list(APPEND private_classes
    vtkZSpaceCoreSDKManager)
  vtk_module_find_package(PACKAGE zSpace)
endif()

set_property(SOURCE vtkZSpaceSDKManager.cxx APPEND
  PROPERTY
    COMPILE_DEFINITIONS "VTK_ZSPACE_USE_COMPAT_SDK=$<BOOL:${ZSPACE_USE_COMPAT_SDK}>")

vtk_module_add_module(VTK::RenderingZSpace
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes})

if(ZSPACE_USE_COMPAT_SDK)
  vtk_module_include(VTK::RenderingZSpace
    PRIVATE
    # XXX(cmake-3.26): Use `BUILD_LOCAL_INTERFACE` instead.
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty>")
else()
  vtk_module_link(VTK::RenderingZSpace
    PUBLIC zSpace::zSpace)
endif()
