vtk_module_find_package(PACKAGE OpenVR)

set(classes
  vtkOpenVRCamera
  vtkOpenVRControlsHelper
  vtkOpenVRDefaultOverlay
  vtkOpenVRInteractorStyle
  vtkOpenVRModel
  vtkOpenVROverlay
  vtkOpenVRRenderer
  vtkOpenVRRenderWindow
  vtkOpenVRRenderWindowInteractor)

vtk_encode_string(
  INPUT         "OpenVRDashboard.jpg"
  EXPORT_HEADER "vtkRenderingOpenVRModule.h"
  EXPORT_SYMBOL "VTKRENDERINGOPENVR_NO_EXPORT"
  HEADER_OUTPUT dashboard_header
  SOURCE_OUTPUT dashboard_source
  BINARY)

vtk_module_add_module(VTK::RenderingOpenVR
  CLASSES ${classes}
  SOURCES "${dashboard_source}" ${vtk_object_factory_source} ${geometry_sources}
  PRIVATE_HEADERS "${dashboard_header}" ${vtk_object_factory_header} ${geometry_headers})
vtk_module_link(VTK::RenderingOpenVR
  PUBLIC
    OpenVR::OpenVR)

set(openvr_input_files
  vtk_openvr_actions.json
  vtk_openvr_binding_hpmotioncontroller.json
  vtk_openvr_binding_knuckles.json
  vtk_openvr_binding_oculus_touch.json
  vtk_openvr_binding_vive_controller.json
)

foreach(inputfile IN LISTS openvr_input_files)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${inputfile}"
    "${CMAKE_CURRENT_BINARY_DIR}/${input_file}"
    COPYONLY)
endforeach()

install(
  FILES ${openvr_input_files}
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/vr_actions/"
  COMPONENT   runtime
)
vtk_add_test_mangling(VTK::RenderingOpenVR)
