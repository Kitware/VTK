option(VTK_ENABLE_VR_COLLABORATION "Enable VR collaboration support. Requires libzmq library" OFF)
mark_as_advanced(VTK_ENABLE_VR_COLLABORATION)

set(classes
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
  vtkVRRenderer
  vtkVRRenderWindow
  vtkVRRenderWindowInteractor)

if (VTK_ENABLE_VR_COLLABORATION)
  vtk_module_find_package(PRIVATE_IF_SHARED
    PACKAGE ZeroMQ)
  list(APPEND classes vtkVRCollaborationClient)
endif()

vtk_module_add_module(VTK::RenderingVR
  CLASSES ${classes}
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

  vtk_module_link(VTK::RenderingVR NO_KIT_EXPORT_IF_SHARED PRIVATE "${zmq_target}")
endif ()
vtk_add_test_mangling(VTK::RenderingVR)
