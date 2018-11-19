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

list(APPEND ${vtk-module}_SYSTEM_INCLUDE_DIRS ${OPENVR_INCLUDE_DIRS})

vtk_module_library(${vtk-module} ${Module_SRCS})
vtk_module_link_libraries(${vtk-module} LINK_PRIVATE ${OPENVR_LIBRARY})

include(vtkOpenGL)
vtk_opengl_link(${vtk-module})

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/OpenVRDashboard.jpg"
  "${CMAKE_BINARY_DIR}/OpenVRDashboard.jpg"
  COPYONLY
  )
