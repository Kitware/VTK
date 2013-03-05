# This project builds the test directories from all VTK modules as a separate
# project outside the main VTK build tree as if they were an application.
cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
project(VTKTestExternal)
if(VTK_SOURCE_DIR OR VTK_BINARY_DIR)
  message(FATAL_ERROR "This directory may build only outside VTK!")
endif()

include(CTest)

# Find the top of the main VTK source tree.
get_filename_component(VTK_TOP_DIR ${VTKTestExternal_SOURCE_DIR}/../.. ABSOLUTE)

# Load module infrastructure macros.
list(APPEND CMAKE_MODULE_PATH ${VTK_TOP_DIR}/CMake)
include(vtkModuleMacros)
include(vtkTestingMacros)

# Find the VTK build or install tree.  Assume the version matches exactly.
# One should provide VTK_DIR explicitly in our intended use cases.
find_package(VTK REQUIRED NO_MODULE)

# Use VTK's flags.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${VTK_REQUIRED_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VTK_REQUIRED_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${VTK_REQUIRED_EXE_LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${VTK_REQUIRED_SHARED_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${VTK_REQUIRED_MODULE_LINKER_FLAGS}")

# This is a cross-platform project so we cannot use the MS _s API.
if(WIN32 AND "${CMAKE_C_COMPILER_ID}" MATCHES "^(Intel)$")
  set(_INTEL_WINDOWS 1)
endif()
if(MSVC OR _INTEL_WINDOWS)
  add_definitions(
    -D_CRT_NONSTDC_NO_DEPRECATE
    -D_CRT_SECURE_NO_DEPRECATE
    -D_SCL_SECURE_NO_DEPRECATE
    )
endif()

# Glob the set of modules in the source tree including test modules.
vtk_module_glob("${VTK_TOP_DIR}" "${VTKTestExternal_BINARY_DIR}" Cxx)

# Input information for test build files.
find_path(VTK_DATA_ROOT VTKData.readme ${VTK_TOP_DIR}/../VTKData)
find_path(VTK_LARGE_DATA_ROOT VTKLargeData.readme ${VTK_TOP_DIR}/../VTKLargeData)
set(VTK_TEST_OUTPUT_DIR "${VTKTestExternal_BINARY_DIR}/Testing/Temporary")

# If vtkParallelMPI is found then look for MPI and bring in
# vtkTestingMPISupport.
if(vtkParallelMPI_LOADED)
  find_package(MPI REQUIRED)
  include(vtkTestingMPISupport)
endif()

# Add the test directory for each enabled module that has tests
# whose dependencies are also enabled.
foreach(mod ${VTK_MODULES_ALL})
  if(${mod}_TESTS_FOR AND ${${mod}_TESTS_FOR}_LOADED)
    set(missing "")
    foreach(dep IN LISTS ${mod}_DEPENDS)
      if(NOT ${dep}_LOADED)
        set(missing "${missing}  ${dep}\n")
      endif()
    endforeach()
    if(NOT missing)
      set(vtk-module ${${mod}_TESTS_FOR})
      add_subdirectory(${${mod}_SOURCE_DIR} ${${mod}_BINARY_DIR})
    else()
      message(STATUS "Skipping ${mod} due to missing dependencies:\n${missing}")
    endif()
  endif()
endforeach()
