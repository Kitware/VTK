cmake_minimum_required(VERSION 3.12...3.21 FATAL_ERROR)
project(ExoticShapes)

include(GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

find_package(VTK COMPONENTS CommonCore RenderingCore RenderingOpenGL2)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

set(_shared_default OFF)
get_target_property(_vtk_libtype VTK::CommonCore TYPE)
if (_vtk_libtype STREQUAL "STATIC_LIBRARY")
  set(_shared_default OFF)
endif ()

option(BUILD_SHARED_LIBS "Build shared or static libraries" "${_shared_default}")
include(CTest)
include(GNUInstallDirs)

# First we scan the modules in our project to find out the dependency graph
# between them.
set(es_install_export ES)
vtk_module_scan(
  # With only 1 module file, this is easier. With more,
  # `vtk_module_find_modules` would be preferred.
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/ExoticShapes/vtk.module"
  # Not building the only module we have is silly.
  REQUEST_MODULES   ES::ExoticShapes
  # Store the list of provided modules from this scan.
  PROVIDES_MODULES  modules
  # Enable the tests for our modules.
  ENABLE_TESTS      ON)

# Build the module we just scanned.
vtk_module_build(
  MODULES ${modules}
  TEST_INPUT_DATA_DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/Testing"
  TEST_OUTPUT_DATA_DIRECTORY  "${CMAKE_CURRENT_BINARY_DIR}/ExternalData/Testing"
  INSTALL_EXPORT    "${es_install_export}")

if (BUILD_TESTING)
  include(CTest)
  ######## Regression Testing ########
  set(ExternalData_BINARY_ROOT ${CMAKE_CURRENT_BINARY_DIR}/ExternalData)

  list(APPEND ExternalData_URL_TEMPLATES
    # Data published by Girder
    "https://data.kitware.com/api/v1/file/hashsum/%(algo)/%(hash)/download"

    # Data published by developers using git-gitlab-push.
    "https://www.vtk.org/files/ExternalData/%(algo)/%(hash)"
  )
  include(ExternalData)
endif ()
