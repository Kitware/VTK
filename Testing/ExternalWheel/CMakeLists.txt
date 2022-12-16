# This project builds the test directories from all VTK modules as a separate
# project outside the main VTK build tree as if they were an application.
cmake_minimum_required(VERSION 3.8...3.16 FATAL_ERROR)

project(VTKTestExternalWheel NONE)
if(VTK_SOURCE_DIR OR VTK_BINARY_DIR)
  message(FATAL_ERROR "This directory may build only outside VTK!")
endif()

include(CTest)

# Find the top of the main VTK source tree.
get_filename_component(VTK_TOP_DIR "${VTKTestExternalWheel_SOURCE_DIR}/../.." ABSOLUTE)
set(ExternalData_SOURCE_ROOT "${VTK_TOP_DIR}")
set(VTK_SOURCE_DIR "${VTK_TOP_DIR}")

# Use VTK macros to find what we need.
list(APPEND CMAKE_MODULE_PATH "${VTK_TOP_DIR}/CMake")

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(_vtk_testing_python_exe "$<TARGET_FILE:Python3::Interpreter>")
set(_vtk_testing_python_exe_config "${Python3_EXECUTABLE}")

# Include VTK's module system.
include(vtkModule)

# Include VTK's external data settings.
include(vtkExternalData)

# Find the set of modules in the source tree.
vtk_module_find_kits(discovered_kits "${VTK_TOP_DIR}")
vtk_module_find_modules(discovered_modules "${VTK_TOP_DIR}")
set(filtered_modules)
foreach (discovered_module IN LISTS discovered_modules)
  if (NOT discovered_module MATCHES "/Examples/")
    list(APPEND filtered_modules "${discovered_module}")
  endif ()
endforeach ()
vtk_module_scan(
  MODULE_FILES            ${filtered_modules}
  KIT_FILES               ${discovered_kits}
  WANT_BY_DEFAULT         ON
  ENABLE_TESTS            DEFAULT
  HIDE_MODULES_FROM_CACHE ON
  PROVIDES_MODULES        vtk_modules_to_test
  PROVIDES_KITS           vtk_kits)

function (check_module_importable result module)
  execute_process(
    COMMAND "${_vtk_testing_python_exe_config}" -c "import ${module}"
    OUTPUT_VARIABLE _
    ERROR_VARIABLE _
    RESULT_VARIABLE res)
  if (NOT res)
    set("${result}" 1 PARENT_SCOPE)
  else ()
    set("${result}" 0 PARENT_SCOPE)
  endif ()
endfunction ()

# Populate module metadata enough to stitch things together.
foreach (vtk_module_to_test IN LISTS vtk_modules_to_test)
  get_property(module_exclude_wrap GLOBAL PROPERTY "_vtk_module_${vtk_module_to_test}_exclude_wrap")
  if (NOT module_exclude_wrap)
    get_property(module_library_name GLOBAL PROPERTY "_vtk_module_${vtk_module_to_test}_library_name")
    check_module_importable(is_importable "vtkmodules.${module_library_name}")
    if (NOT is_importable)
      continue ()
    endif ()
  endif ()

  get_property(target_name GLOBAL PROPERTY "_vtk_module_${vtk_module_to_test}_target_name")
  get_property(namespace GLOBAL PROPERTY "_vtk_module_${vtk_module_to_test}_namespace")
  add_library("${target_name}" INTERFACE)
  add_library("${namespace}::${target_name}" ALIAS "${target_name}")
  if (NOT module_exclude_wrap)
    _vtk_module_set_module_property("${target_name}"
      PROPERTY  "library_name"
      VALUE     "${module_library_name}")
  endif ()
endforeach ()

# Input information for test build files.
option(VTK_USE_LARGE_DATA "Enable tests requiring \"large\" data" OFF)
set(_vtk_build_TEST_DATA_TARGET "VTKData")
set(_vtk_build_TEST_INPUT_DATA_DIRECTORY "${VTK_TOP_DIR}/Testing")
set(_vtk_build_TEST_OUTPUT_DATA_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ExternalData/Testing")
set(_vtk_build_TEST_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Testing/Temporary")

set(VTK_WRAP_PYTHON 1)
set(vtk_testing_cxx_disabled 1)

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
  if (EXISTS "${VTK_TOP_DIR}/${_vtk_build_module_subdir}/Testing" AND
      # Skip modules without Python testing.
      EXISTS "${VTK_TOP_DIR}/${_vtk_build_module_subdir}/Testing/Python")
    add_subdirectory(
      "${VTK_TOP_DIR}/${_vtk_build_module_subdir}/Testing"
      "${CMAKE_BINARY_DIR}/${_vtk_build_module_subdir}/Testing")
  endif ()
endforeach ()

# Create target to download data from the VTKData group.  This must come after
# all tests have been added that reference the group, so we put it last.
ExternalData_Add_Target(VTKData)
