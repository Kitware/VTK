option(VTK_ENABLE_VR_COLLABORATION "Enable VR collaboration support. Requires libzmq library" OFF)
mark_as_advanced(VTK_ENABLE_VR_COLLABORATION)

set(classes
  vtkOpenGLAvatar
  vtkVRCamera
  vtkVRControlsHelper
  vtkVRFollower
  vtkVRHardwarePicker
  vtkVRHMDCamera
  vtkVRInteractorStyle
  vtkVRMenuRepresentation
  vtkVRMenuWidget
  vtkVRModel
  vtkVRPanelRepresentation
  vtkVRPanelWidget
  vtkVRRay
  vtkVRRenderer
  vtkVRRenderWindow
  vtkVRRenderWindowInteractor)

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
    EXPORT_SYMBOL "VTKRENDERINGVR_EXPORT"
    EXPORT_HEADER "vtkRenderingVRModule.h"
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
  EXPORT_MACRO "VTKRENDERINGVR_EXPORT")

if (VTK_ENABLE_VR_COLLABORATION)
  vtk_module_find_package(PACKAGE ZeroMQ)
  list(APPEND classes vtkVRCollaborationClient)
endif()

vtk_module_add_module(VTK::RenderingVR
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source} ${geometry_sources}
  PRIVATE_HEADERS ${vtk_object_factory_header} ${geometry_headers}
  )

if (VTK_ENABLE_VR_COLLABORATION)
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

  vtk_module_link(VTK::RenderingVR PRIVATE "${zmq_target}")
endif ()
