cmake_minimum_required(VERSION 2.8.8)
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW) # CMake 3.0
endif()
if(POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW) # CMake 3.1
endif()
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
