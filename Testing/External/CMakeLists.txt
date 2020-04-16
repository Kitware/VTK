# This project builds the test directories from all VTK modules as a separate
# project outside the main VTK build tree as if they were an application.
cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)

project(VTKTestExternal)
if(VTK_SOURCE_DIR OR VTK_BINARY_DIR)
  message(FATAL_ERROR "This directory may build only outside VTK!")
endif()

include(CTest)

# Find the top of the main VTK source tree.
get_filename_component(VTK_TOP_DIR ${VTKTestExternal_SOURCE_DIR}/../.. ABSOLUTE)
set(ExternalData_SOURCE_ROOT ${VTK_TOP_DIR})
set(VTK_SOURCE_DIR "${VTK_TOP_DIR}")

# Find the VTK build or install tree.  Assume the version matches exactly.
# One should provide VTK_DIR explicitly in our intended use cases.
find_package(VTK REQUIRED NO_MODULE)
# Tests use some macros only stored in VTK's source tree.
list(APPEND CMAKE_MODULE_PATH "${VTK_TOP_DIR}/CMake")

# Include VTK's external data settings.
include(vtkExternalData)

# This is a cross-platform project so we cannot use the MS _s API.
if(WIN32 AND CMAKE_C_COMPILER_ID STREQUAL "Intel")
  set(_INTEL_WINDOWS 1)
endif()
if(MSVC OR _INTEL_WINDOWS)
  add_definitions(
    -D_CRT_NONSTDC_NO_DEPRECATE
    -D_CRT_SECURE_NO_DEPRECATE
    -D_SCL_SECURE_NO_DEPRECATE
    )
endif()

# Find the set of modules in the source tree.
vtk_module_find_kits(discovered_kits "${VTK_TOP_DIR}")
vtk_module_find_modules(discovered_modules "${VTK_TOP_DIR}")
vtk_module_scan(
  MODULE_FILES            ${discovered_modules}
  KIT_FILES               ${discovered_kits}
  WANT_BY_DEFAULT         ON
  ENABLE_TESTS            DEFAULT
  HIDE_MODULES_FROM_CACHE ON
  PROVIDES_MODULES        vtk_modules_to_test
  PROVIDES_KITS           vtk_kits)

# Input information for test build files.
option(VTK_USE_LARGE_DATA "Enable tests requiring \"large\" data" OFF)
set(_vtk_build_TEST_DATA_TARGET "VTKData")
set(_vtk_build_TEST_INPUT_DATA_DIRECTORY "${VTK_TOP_DIR}/Testing")
set(_vtk_build_TEST_OUTPUT_DATA_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ExternalData/Testing")
set(_vtk_build_TEST_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Testing/Temporary")

get_property(vtk_test_modules GLOBAL
  PROPERTY  _vtk_module_test_modules)
foreach (_vtk_build_test IN LISTS vtk_test_modules)
  get_property(_vtk_build_test_depends GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_test}_test_depends")
  if (NOT TARGET "${_vtk_build_test}")
    message(STATUS "Skipping ${_vtk_build_test} because it was not built.")
    continue ()
  endif ()

  set(_vtk_build_test_has_depends TRUE)
  set(_vtk_build_test_missing_depends)
  foreach (_vtk_build_test_depend IN LISTS _vtk_build_test_depends)
    if (NOT TARGET "${_vtk_build_test_depend}")
      list(APPEND _vtk_build_test_missing_depends
        "${_vtk_build_test_depend}")
      set(_vtk_build_test_has_depends FALSE)
    endif ()
  endforeach ()
  if (NOT _vtk_build_test_has_depends)
    string(REPLACE ";" "\n  " _vtk_build_test_missing_depends "${_vtk_build_test_missing_depends}")
    message(STATUS "Skipping ${_vtk_build_test} due to missing dependencies:\n  ${_vtk_build_test_missing_depends}")
    continue ()
  endif ()

  get_property(_vtk_build_module_file GLOBAL
    PROPERTY  "_vtk_module_${_vtk_build_test}_file")

  get_filename_component(_vtk_build_module_dir "${_vtk_build_module_file}" DIRECTORY)
  file(RELATIVE_PATH _vtk_build_module_subdir "${CMAKE_SOURCE_DIR}" "${_vtk_build_module_dir}")
  if (EXISTS "${CMAKE_SOURCE_DIR}/${_vtk_build_module_subdir}/Testing")
    add_subdirectory(
      "${CMAKE_SOURCE_DIR}/${_vtk_build_module_subdir}/Testing"
      "${CMAKE_BINARY_DIR}/${_vtk_build_module_subdir}/Testing")
  endif ()
endforeach ()

# Create target to download data from the VTKData group.  This must come after
# all tests have been added that reference the group, so we put it last.
ExternalData_Add_Target(VTKData)
