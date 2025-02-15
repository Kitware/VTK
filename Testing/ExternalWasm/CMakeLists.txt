# This project builds the test directories from all VTK modules as a separate
# project outside the main VTK build tree as if they were an application.
cmake_minimum_required(VERSION 3.8...3.16 FATAL_ERROR)

project(VTKTestExternalWasm)
if(VTK_SOURCE_DIR OR VTK_BINARY_DIR)
  message(FATAL_ERROR "This directory may build only outside VTK!")
endif()

include(CTest)
# Find the top of the main VTK source tree.
get_filename_component(VTK_TOP_DIR "${VTKTestExternalWasm_SOURCE_DIR}/../.." ABSOLUTE)
set(ExternalData_SOURCE_ROOT "${VTK_TOP_DIR}")
set(VTK_SOURCE_DIR "${VTK_TOP_DIR}")

configure_file(
  "${VTK_SOURCE_DIR}/CMake/CTestCustom.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake"
  @ONLY)

add_library(vtkbuild INTERFACE)
add_library(VTKTestExternalWasm::vtkbuild ALIAS vtkbuild)

# Tests and vtkEmscripten use some macros only stored in VTK's source tree.
list(APPEND CMAKE_MODULE_PATH "${VTK_TOP_DIR}/CMake")
# The final test executable needs to be compiled and linked with
# the correct set of emscripten flags depending on whether the architecture is
# wasm32/wasm64. Import the flag selection logic from VTK source tree.
# For VTK_BUILD_TESTING, VTK_TESTING_WASM_ENGINE[,_ARGUMENTS]
include("${VTK_SOURCE_DIR}/CMake/vtkTesting.cmake")
# For VTK_WEBASSEMBLY_64_BIT, VTK_WEBASSEMBLY_THREADS and VTK_WEBASSEMBLY_EXCEPTIONS
include("${VTK_SOURCE_DIR}/CMake/vtkEmscripten.cmake")
# For -sMEMORY64=1, -pthread, etc.
include("${VTK_SOURCE_DIR}/CMake/vtkCompilerPlatformFlags.cmake")
include("${VTK_SOURCE_DIR}/CMake/vtkCompilerWarningFlags.cmake")
# For OpenGL options
include("${VTK_SOURCE_DIR}/CMake/vtkOpenGLOptions.cmake")

# Allow VTK to be discovered because emscripten's toolchain file
# forces cmake to look only inside `CMAKE_FIND_ROOT_PATH`
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE_saved ${CMAKE_FIND_ROOT_PATH_MODE_PACKAGE})
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
# Find the VTK build or install tree. Assume the version matches exactly.
# One should provide VTK_DIR explicitly in our intended use cases.
find_package(VTK REQUIRED NO_MODULE)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ${CMAKE_FIND_ROOT_PATH_MODE_PACKAGE_saved})

# Include VTK's external data settings.
include(vtkExternalData)

# Tests need python interpreter to run VTK_SOURCE_DIR/Testing/WebAssembly/runner.py
find_package(Python3 REQUIRED Interpreter)

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
set(_vtk_build_TEST_DATA_TARGET "VTKData")
set(_vtk_build_TEST_INPUT_DATA_DIRECTORY "${VTK_TOP_DIR}/Testing")
set(_vtk_build_TEST_OUTPUT_DATA_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ExternalData/Testing")
set(_vtk_build_TEST_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Testing/Temporary")
set(_vtk_build_UTILITY_TARGET "VTKTestExternalWasm::vtkbuild")

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
  file(RELATIVE_PATH _vtk_build_module_subdir "${VTK_TOP_DIR}" "${_vtk_build_module_dir}")
  if (EXISTS "${VTK_TOP_DIR}/${_vtk_build_module_subdir}/Testing")
    add_subdirectory(
      "${VTK_TOP_DIR}/${_vtk_build_module_subdir}/Testing"
      "${CMAKE_BINARY_DIR}/${_vtk_build_module_subdir}/Testing")
  endif ()
endforeach ()

# Create target to download data from the VTKData group.  This must come after
# all tests have been added that reference the group, so we put it last.
ExternalData_Add_Target(VTKData)
