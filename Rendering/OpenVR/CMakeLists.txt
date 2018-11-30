if(NOT VTK_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.3...3.12 FATAL_ERROR)
  project(vtkRenderingOpenVR)
  find_package(VTK REQUIRED)
endif()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${VTK_CMAKE_DIR}")
if(NOT VTK_SOURCE_DIR)
  include(vtkExternalModuleMacros)
endif()

find_package(OpenVR REQUIRED)

set(Module_SRCS
  vtkOpenVRCamera.cxx
  vtkOpenVRControlsHelper.cxx
  vtkOpenVRDefaultOverlay.cxx
  vtkOpenVRFollower.cxx
  vtkOpenVRHardwarePicker.cxx
  vtkOpenVRInteractorStyle.cxx
  vtkOpenVRMenuRepresentation.cxx
  vtkOpenVRMenuWidget.cxx
  vtkOpenVRModel.cxx
  vtkOpenVROverlay.cxx
  vtkOpenVRPanelRepresentation.cxx
  vtkOpenVRPanelWidget.cxx
  vtkOpenVRPropPicker.cxx
  vtkOpenVRRenderer.cxx
  vtkOpenVRRenderWindow.cxx
  vtkOpenVRRenderWindowInteractor.cxx
  )

vtk_encode_string(
  INPUT         "OpenVRDashboard.jpg"
  HEADER_OUTPUT header
  SOURCE_OUTPUT source
  BINARY)
list(APPEND Module_SRCS ${source})

# Avatar's dependency on vtkIOXML keeps it out of OpenGL2 module
list(APPEND Module_SRCS
  vtkOpenGLAvatar.cxx
  )

# Generates headers with char arrays that contain the model file contents.
set(geometry_files
  data/vtkAvatarHead.vtp
  data/vtkAvatarLeftHand.vtp
  data/vtkAvatarRightHand.vtp
  data/vtkAvatarTorso.vtp
  data/vtkAvatarLeftForeArm.vtp
  data/vtkAvatarRightForeArm.vtp
  )
unset(geometry_h_files)
foreach(file ${geometry_files})
  vtk_encode_string(
    INPUT         "${file}"
    EXPORT_SYMBOL "VTKRENDERINGOPENVR_EXPORT"
    EXPORT_HEADER "vtkRenderingOpenVRModule.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source
    BINARY
    NUL_TERMINATE)
  list(APPEND Module_SRCS ${source})
endforeach()

vtk_add_override(vtkAvatar vtkOpenGLAvatar)
vtk_object_factory_configure("${vtk_module_overrides}")

list(APPEND Module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)

list(APPEND ${vtk-module}_SYSTEM_INCLUDE_DIRS ${OPENVR_INCLUDE_DIRS})

vtk_module_library(${vtk-module} ${Module_SRCS})
vtk_module_link_libraries(${vtk-module} LINK_PRIVATE ${OPENVR_LIBRARY})

include(vtkOpenGL)
vtk_opengl_link(${vtk-module})
