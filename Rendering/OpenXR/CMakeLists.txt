vtk_module_find_package(PACKAGE OpenXR)

set(classes
  vtkOpenXRRenderWindow
  vtkOpenXRRenderWindowInteractor
  vtkOpenXRManager
  vtkOpenXRCamera
  vtkOpenXRRenderer
  vtkOpenXRInteractorStyle
  vtkOpenXRRay
  vtkOpenXRMenuRepresentation
  vtkOpenXRMenuWidget
  vtkOpenXRHardwarePicker)

# vtk_encode_string(
#   INPUT         "OpenXRDashboard.jpg"
#   HEADER_OUTPUT dashboard_header
#   SOURCE_OUTPUT dashboard_source
#   BINARY)

# set(geometry_sources)
# set(geometry_headers)

# # Generates headers with char arrays that contain the model file contents.
# set(geometry_files
#   data/vtkAvatarHead.vtp
#   data/vtkAvatarLeftForeArm.vtp
#   data/vtkAvatarLeftHand.vtp
#   data/vtkAvatarLeftUpperArm.vtp
#   data/vtkAvatarRightForeArm.vtp
#   data/vtkAvatarRightHand.vtp
#   data/vtkAvatarRightUpperArm.vtp
#   data/vtkAvatarTorso.vtp)
# foreach (geometry_file IN LISTS geometry_files)
#   vtk_encode_string(
#     INPUT         "${geometry_file}"
#     EXPORT_SYMBOL "VTKRENDERINGOPENVR_EXPORT"
#     EXPORT_HEADER "vtkRenderingOpenXRModule.h"
#     HEADER_OUTPUT header
#     SOURCE_OUTPUT source
#     BINARY
#     NUL_TERMINATE)
#   list(APPEND geometry_sources "${source}")
#   list(APPEND geometry_headers "${header}")
# endforeach ()

# vtk_object_factory_declare(
#   BASE      vtkAvatar
#   OVERRIDE  vtkOpenGLAvatar)
# vtk_object_factory_configure(
#   SOURCE_FILE vtk_object_factory_source
#   HEADER_FILE vtk_object_factory_header
#   EXPORT_MACRO "VTKRENDERINGOPENVR_EXPORT")

vtk_module_add_module(VTK::RenderingOpenXR
  CLASSES ${classes}
  )
  # SOURCES "${dashboard_source}" ${vtk_object_factory_source} ${geometry_sources}
  # PRIVATE_HEADERS "${dashboard_header}" ${vtk_object_factory_header} ${geometry_headers})
vtk_module_link(VTK::RenderingOpenXR
  PUBLIC
    OpenXR::OpenXR)

set(openxr_input_files
  vtk_openxr_actions.json
  vtk_openxr_binding_htc_vive_controller.json
  vtk_openxr_binding_khr_simple_controller.json
  vtk_openxr_binding_valve_index_controller.json
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
