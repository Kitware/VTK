project(vtkLocalExample)
cmake_minimum_required(VERSION 2.8)

# Find and load VTK settings.
if(NOT VTK_BINARY_DIR)
  find_package(VTK 6.0 REQUIRED NO_MODULE)
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
