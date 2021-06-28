vtk_module_find_package(PACKAGE OpenVR)

option(VTK_ENABLE_OPENVR_COLLABORATION "Enable OpenVR collaboration support. Requires libzmq library" OFF)
mark_as_advanced(VTK_ENABLE_OPENVR_COLLABORATION)

set(classes
  vtkOpenGLAvatar
  vtkOpenVRCamera
  vtkOpenVRControlsHelper
  vtkOpenVRDefaultOverlay
  vtkOpenVRFollower
  vtkOpenVRHardwarePicker
  vtkOpenVRInteractorStyle
  vtkOpenVRMenuRepresentation
  vtkOpenVRMenuWidget
  vtkOpenVRModel
  vtkOpenVROverlay
  vtkOpenVRPanelRepresentation
  vtkOpenVRPanelWidget
  vtkOpenVRRay
  vtkOpenVRRenderer
  vtkOpenVRRenderWindow
  vtkOpenVRRenderWindowInteractor)

vtk_encode_string(
  INPUT         "OpenVRDashboard.jpg"
  HEADER_OUTPUT dashboard_header
  SOURCE_OUTPUT dashboard_source
  BINARY)

set(geometry_sources)
set(geometry_headers)

# Generates headers with char arrays that contain the model file contents.
set(geometry_files
  data/vtkAvatarHead.vtp
  data/vtkAvatarLeftForeArm.vtp
  data/vtkAvatarLeftHand.vtp
  data/vtkAvatarLeftUpperArm.vtp
  data/vtkAvatarRightForeArm.vtp
  data/vtkAvatarRightHand.vtp
  data/vtkAvatarRightUpperArm.vtp
  data/vtkAvatarTorso.vtp)
foreach (geometry_file IN LISTS geometry_files)
  vtk_encode_string(
    INPUT         "${geometry_file}"
    EXPORT_SYMBOL "VTKRENDERINGOPENVR_EXPORT"
    EXPORT_HEADER "vtkRenderingOpenVRModule.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source
    BINARY
    NUL_TERMINATE)
  list(APPEND geometry_sources "${source}")
  list(APPEND geometry_headers "${header}")
endforeach ()

vtk_object_factory_declare(
  BASE      vtkAvatar
  OVERRIDE  vtkOpenGLAvatar)
vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGOPENVR_EXPORT")

if (VTK_ENABLE_OPENVR_COLLABORATION)
  vtk_module_find_package(PACKAGE ZeroMQ)
  list(APPEND classes vtkOpenVRCollaborationClient)
endif()

vtk_module_add_module(VTK::RenderingOpenVR
  CLASSES ${classes}
  SOURCES "${dashboard_source}" ${vtk_object_factory_source} ${geometry_sources}
  PRIVATE_HEADERS "${dashboard_header}" ${vtk_object_factory_header} ${geometry_headers})
vtk_module_link(VTK::RenderingOpenVR
  PUBLIC
    OpenVR::OpenVR)

if (VTK_ENABLE_OPENVR_COLLABORATION)
  # try for matching libzmq to our shared/static build type
  if (BUILD_SHARED_LIBS AND TARGET libzmq)
    set(zmq_target libzmq)
  elseif (NOT BUILD_SHARED_LIBS AND TARGET libzmq-static)
    set(zmq_target libzmq-static)
  else ()
    # can't find matching, so use whatever we can find
    if (TARGET libzmq-static)
      set(zmq_target libzmq-static)
    else ()
      set(zmq_target libzmq)
    endif ()
  endif ()

  vtk_module_link(VTK::RenderingOpenVR PRIVATE "${zmq_target}")
endif ()

set(openvr_input_files
  vtk_openvr_actions.json
  vtk_openvr_binding_hpmotioncontroller.json
  vtk_openvr_binding_oculus_touch.json
  vtk_openvr_binding_vive_controller.json
)

foreach(inputfile IN LISTS openvr_input_files)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${inputfile}"
    "${CMAKE_BINARY_DIR}/${input_file}"
    COPYONLY)
endforeach()

install(
  FILES ${openvr_input_files}
  DESTINATION "${CMAKE_INSTALL_BINDIR}"
  COMPONENT   runtime
)
