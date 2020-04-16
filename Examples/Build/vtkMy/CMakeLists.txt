cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)

#
# Set the project name.  The version number is optional.
#

project(VTKMY VERSION 1.0.0)

#
# Find VTK, get all required components
#

find_package(VTK
  COMPONENTS
    CommonCore
    CommonExecutionModel
  OPTIONAL_COMPONENTS
    RenderingOpenGL2
    Python)

list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#
# Check if VTK is shared or static, and default to the same.
#

get_target_property(_vtk_lib_type VTK::CommonCore TYPE)
if(_vtk_lib_type STREQUAL SHARED_LIBRARY)
  set(_default ON)
else()
  set(_default OFF)
endif()
option(BUILD_SHARED_LIBS "Build shared libraries." ${_default})

#
# Check the system for the proper install locations for libraries,
# headers, documentation, etcetera.  This will set the variables
# CMAKE_INSTALL_LIBDIR/BINDIR/INCLUDEDIR/DOCDIR/DATAROOTDIR, etc.,
# which are used by the vtk_module macros.
#

include(GNUInstallDirs)

#
# Set the directories where executables and libraries will be put.
# The "archive" directory tells cmake where to put static libraries,
# so we set it exactly the same as the "library" directory.
#

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})

#
# Set the directory where cmake configuration files are installed.
#
set(my_config_name vtkmy)
if(WIN32)
  set(my_config_dir cmake/${my_config_name})
else()
  set(my_config_dir ${CMAKE_INSTALL_LIBDIR}/cmake/${my_config_name})
endif()

#
# Search for "vtk.module" files in all subdirectories.
#

vtk_module_find_modules(vtk_module_files ${CMAKE_CURRENT_SOURCE_DIR})

#
# Scan the module files.
#

vtk_module_scan(
  MODULE_FILES        ${vtk_module_files}
  REQUEST_MODULES     VTKMY::Common VTKMY::Imaging VTKMY::Unsorted
  PROVIDES_MODULES    my_modules
  ENABLE_TESTS        "${BUILD_TESTING}")

#
# Set up the module build.
#

vtk_module_build(
  MODULES             ${my_modules}
  INSTALL_EXPORT      VTKMY
  CMAKE_DESTINATION   "${my_config_dir}"
  VERSION             "${VTKMY_VERSION}"
  SOVERSION           "1")

#
# Perform the wrapping
#

if(VTK_WRAP_PYTHON)
  vtk_module_wrap_python(
    MODULES             ${my_modules}
    INSTALL_EXPORT      VTKMY
    PYTHON_PACKAGE      "vtkmy"
    CMAKE_DESTINATION   "${my_config_dir}"
    BUILD_STATIC        OFF)
endif()

#
# Build examples too ?
#

option(BUILD_EXAMPLES "Build examples." ON)
if(BUILD_EXAMPLES)
  add_subdirectory(Examples)
endif()

#
# Utilities folder creates doxygen documentation
#
add_subdirectory(Utilities)
