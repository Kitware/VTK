vtk_module_find_package(PACKAGE OpenXR VERSION 1.0.31)

set(classes
  vtkOpenXRRenderWindow
  vtkOpenXRRenderWindowInteractor
  vtkOpenXRCamera
  vtkOpenXRControlsHelper
  vtkOpenXRModel
  vtkOpenXRRenderer
  vtkOpenXRInteractorStyle
  vtkOpenXRSceneObserver
  vtkOpenXRSceneComponent
  )

set(nowrap_classes
  vtkOpenXRManager
  vtkOpenXRManagerGraphics
  vtkOpenXRManagerOpenGLGraphics
  vtkOpenXRManagerConnection
  )

set(nowrap_headers
  vtkOpenXR.h
  vtkOpenXRPlatform.h
  vtkOpenXRUtilities.h
  XrExtensions.h
  XrGraphicsExtensions.h
  )

vtk_module_add_module(VTK::RenderingOpenXR
  CLASSES ${classes}
  NOWRAP_CLASSES ${nowrap_classes}
  NOWRAP_HEADERS ${nowrap_headers}
  )

vtk_module_link(VTK::RenderingOpenXR
  PUBLIC
    OpenXR::openxr_loader
    OpenXR::headers)

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
    ${CMAKE_CURRENT_BINARY_DIR}/${inputfile}
    COPYONLY)
endforeach()

install(
  FILES ${openxr_input_files}
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/xr_actions/"
  COMPONENT   runtime
)
vtk_add_test_mangling(VTK::RenderingOpenXR)
