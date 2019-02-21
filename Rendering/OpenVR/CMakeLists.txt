vtk_module_find_package(PACKAGE OpenVR)

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
  vtkOpenVRPropPicker
  vtkOpenVRRay
  vtkOpenVRRenderWindow
  vtkOpenVRRenderWindowInteractor
  vtkOpenVRRenderer)

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

vtk_module_add_module(VTK::RenderingOpenVR
  CLASSES ${classes}
  SOURCES "${dashboard_source}" ${vtk_object_factory_source} ${geometry_sources}
  PRIVATE_HEADERS "${dashboard_header}" ${vtk_object_factory_header} ${geometry_headers})
vtk_module_link(VTK::RenderingOpenVR
  PUBLIC
    OpenVR::OpenVR)
