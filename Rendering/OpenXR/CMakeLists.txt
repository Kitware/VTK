vtk_module_find_package(PACKAGE OpenXR)

set(classes
  vtkOpenXRRenderWindow
  vtkOpenXRRenderWindowInteractor
  vtkOpenXRCamera
  vtkOpenXRControlsHelper
  vtkOpenXRModel
  vtkOpenXRRenderer
  vtkOpenXRInteractorStyle)

set(nowrap_classes
  vtkOpenXRManager
  vtkOpenXRManagerGraphics
  vtkOpenXRManagerOpenGLGraphics
  vtkOpenXRManagerConnection
  )

# When OpenXR Remoting is enabled, vtkOpenXR.h must define additional types for
# its graphics API and extension set.
if (VTK_MODULE_ENABLE_VTK_RenderingOpenXRRemoting STREQUAL "WANT" OR
  VTK_MODULE_ENABLE_VTK_RenderingOpenXRRemoting STREQUAL "YES" OR
  (VTK_MODULE_ENABLE_VTK_RenderingOpenXRRemoting STREQUAL "DEFAULT" AND VTK_BUILD_ALL_MODULES))
  set(VTK_OPENXR_USE_REMOTING TRUE)
else()
  set(VTK_OPENXR_USE_REMOTING FALSE)
endif()
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkOpenXRConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkOpenXRConfigure.h"
  @ONLY)

set(nowrap_headers
  vtkOpenXR.h
  vtkOpenXRConfigure.h
  vtkOpenXRUtilities.h
  XrExtensions.h
  )

vtk_module_add_module(VTK::RenderingOpenXR
  CLASSES ${classes}
  NOWRAP_CLASSES ${nowrap_classes}
  NOWRAP_HEADERS ${nowrap_headers}
  )

vtk_module_link(VTK::RenderingOpenXR
  PUBLIC
    OpenXR::OpenXR)

set(openxr_input_files
  vtk_openxr_actions.json
  vtk_openxr_binding_htc_vive_controller.json
  vtk_openxr_binding_khr_simple_controller.json
  vtk_openxr_binding_hp_mixed_reality.json
  vtk_openxr_binding_knuckles.json
  vtk_openxr_binding_microsoft_hand_interaction.json
)

foreach(inputfile ${openxr_input_files})
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${inputfile}
    ${CMAKE_BINARY_DIR}/${input_file}
    COPYONLY)
endforeach()

install(
  FILES ${openxr_input_files}
  DESTINATION "${CMAKE_INSTALL_BINDIR}"
  COMPONENT   runtime
)
