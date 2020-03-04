cmake_minimum_required(VERSION 3.8)
project(Wrapping)

# We just need the CommonCore and Python modules here.
find_package(VTK COMPONENTS CommonCore Python)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

set(_shared_default ON)
get_target_property(_vtk_libtype VTK::CommonCore TYPE)
if (_vtk_libtype STREQUAL "STATIC_LIBRARY")
  set(_shared_default OFF)
endif ()

option(BUILD_SHARED_LIBS "Build shared or static libraries" "${_shared_default}")
include(CTest)
include(GNUInstallDirs)

# First we scan the modules in our project to find out the dependency graph
# between them.
vtk_module_scan(
  # With only 1 module file, this is easier. With more,
  # `vtk_module_find_modules` would be preferred.
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/module/vtk.module"
  # Not building the only module we have is silly.
  REQUEST_MODULES   Wrapping::Wrappable
  # Store the list of provided modules from this scan.
  PROVIDES_MODULES  modules
  # Enable the tests for our modules.
  ENABLE_TESTS      ON)

vtk_module_python_default_destination(python_destination)

# Build the module we just scanned.
vtk_module_build(MODULES ${modules})

# Wrap it with Python.
vtk_module_wrap_python(
  MODULES         ${modules}
  PYTHON_PACKAGE  "wrapping"
  MODULE_DESTINATION "${python_destination}"
  # Static Python modules are almost never wanted.
  BUILD_STATIC    OFF
  INSTALL_HEADERS OFF)

# Create an `__init__.py` for the wrapped module.
file(WRITE "${CMAKE_BINARY_DIR}/${python_destination}/wrapping/__init__.py" "")
