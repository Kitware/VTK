cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)

project(vtkLocalExample)

# Find and load VTK settings.
if(NOT VTK_BINARY_DIR)
  find_package(VTK REQUIRED vtkCommonCore)
  include(${VTK_USE_FILE})
endif()

include(${VTK_CMAKE_DIR}/vtkExternalModuleMacros.cmake)

vtk_module(vtkLocalExample
  DEPENDS
  vtkCommonCore
)

include_directories(SYSTEM
  ${VTK_INCLUDE_DIRS}
)

# Source files.
set(vtkLOCAL_SRCS
  vtkLocalExample.cxx
)

# Build and link library.
vtk_module_library(vtkLocalExample ${vtkLOCAL_SRCS})
