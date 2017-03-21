#-----------------------------------------------------------------------------
#
# VTKConfig.cmake - VTK CMake configuration file for external projects.
#
# This file is configured by VTK and used by the UseVTK.cmake module
# to load VTK's settings for an external project.
@VTK_CONFIG_CODE@

set (__vtk_install_tree @VTK_CONFIG_INSTALLED@)

if (CMAKE_VERSION VERSION_LESS "3.3")
    message(FATAL_ERROR "VTK now requires CMake 3.3 or newer")
  endif()

#-----------------------------------------------------------------------------
# Minimum compiler version check: GCC >= 4.6
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.6)
  message(FATAL_ERROR "GCC 4.6 or later is required.")
endif ()

#-----------------------------------------------------------------------------
# Minimum compiler version check: LLVM Clang >= 3.0
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.0)
  message(FATAL_ERROR "LLVM Clang 3.0 or later is required.")
endif ()

#-----------------------------------------------------------------------------
# Minimum compiler version check: Apple Clang >= 3.0 (Xcode 4.2)
if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.0)
  message(FATAL_ERROR "Apple Clang 3.0 or later is required.")
endif ()

#-----------------------------------------------------------------------------
# Minimum compiler version check: Microsoft C/C++ >= 17.0 (aka VS 2012 aka VS 11.0)
if ("x${CMAKE_CXX_COMPILER_ID}" STREQUAL "xMSVC" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 17.0)
  message(FATAL_ERROR "Microsoft Visual Studio 2012 or later is required.")
endif ()

#-----------------------------------------------------------------------------
# Minimum compiler version check: Intel C++ (ICC) >= 14
if (CMAKE_CXX_COMPILER_ID STREQUAL "Intel" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 14.0)
  message(FATAL_ERROR "Intel C++ (ICC) 14.0 or later is required.")
endif ()

# The C and C++ flags added by VTK to the cmake-configured flags.
SET(VTK_REQUIRED_C_FLAGS "@VTK_REQUIRED_C_FLAGS@")
SET(VTK_REQUIRED_CXX_FLAGS "@VTK_REQUIRED_CXX_FLAGS@")
SET(VTK_REQUIRED_EXE_LINKER_FLAGS "@VTK_REQUIRED_EXE_LINKER_FLAGS@")
SET(VTK_REQUIRED_SHARED_LINKER_FLAGS "@VTK_REQUIRED_SHARED_LINKER_FLAGS@")
SET(VTK_REQUIRED_MODULE_LINKER_FLAGS "@VTK_REQUIRED_MODULE_LINKER_FLAGS@")

# The VTK version number
SET(VTK_MAJOR_VERSION "@VTK_MAJOR_VERSION@")
SET(VTK_MINOR_VERSION "@VTK_MINOR_VERSION@")
SET(VTK_BUILD_VERSION "@VTK_BUILD_VERSION@")

# The location of the UseVTK.cmake file.
SET(VTK_CMAKE_DIR "@VTK_CONFIG_CMAKE_DIR@")
SET(VTK_USE_FILE "${VTK_CMAKE_DIR}/UseVTK.cmake")

# The rendering backend VTK was configured to use.
set(VTK_RENDERING_BACKEND "@VTK_RENDERING_BACKEND@")

if (__vtk_install_tree)
  if (WIN32)
    set (VTK_RUNTIME_DIRS "@CMAKE_INSTALL_PREFIX@/@VTK_INSTALL_RUNTIME_DIR@")
  else ()
    set (VTK_RUNTIME_DIRS "@CMAKE_INSTALL_PREFIX@/@VTK_INSTALL_LIBRARY_DIR@")
  endif ()
else()
  if (WIN32)
    set (VTK_RUNTIME_DIRS "@CMAKE_BINARY_DIR@/bin")
  else ()
    set (VTK_RUNTIME_DIRS "@CMAKE_BINARY_DIR@/lib")
  endif ()
endif()

#-----------------------------------------------------------------------------
# Load requested modules.

# List of available VTK modules.
set(VTK_MODULES_ENABLED)

# Determine list of available VTK-modules by scanning the VTK_MODULES_DIR.
file(GLOB config_files RELATIVE "${VTK_MODULES_DIR}" "${VTK_MODULES_DIR}/*.cmake")
foreach (_file ${config_files})
  if (NOT "${_file}" MATCHES "[^\\-]+-[a-zA-Z]+\\.cmake")
    string(REGEX REPLACE "\\.cmake$" "" _module "${_file}")
    list(APPEND VTK_MODULES_ENABLED "${_module}")
  endif()
endforeach()

# Import VTK targets.
set(VTK_CONFIG_TARGETS_FILE "@VTK_CONFIG_TARGETS_FILE@")
if(NOT TARGET @VTK_COMMON_TARGET@)
  include("${VTK_CONFIG_TARGETS_FILE}")
endif()

# Load module interface macros.
include("@VTK_CONFIG_MODULE_API_FILE@")

# Compute set of requested modules.
if(VTK_FIND_COMPONENTS)
  # Specific modules requested by find_package(VTK).
  set(VTK_MODULES_REQUESTED "${VTK_FIND_COMPONENTS}")
else()
  # No specific modules requested.  Use all of them.
  set(VTK_MODULES_REQUESTED "${VTK_MODULES_ENABLED}")
endif()

# Load requested modules and their dependencies into variables:
#  VTK_DEFINITIONS     = Preprocessor definitions
#  VTK_LIBRARIES       = Libraries to link
#  VTK_INCLUDE_DIRS    = Header file search path
#  VTK_LIBRARY_DIRS    = Library search path (for outside dependencies)
vtk_module_config(VTK ${VTK_MODULES_REQUESTED})

#-----------------------------------------------------------------------------

# VTK global configuration options.
SET(VTK_BUILD_SHARED_LIBS "@BUILD_SHARED_LIBS@")
SET(VTK_LEGACY_REMOVE "@VTK_LEGACY_REMOVE@")
SET(VTK_LEGACY_SILENT "@VTK_LEGACY_SILENT@")
SET(VTK_WRAP_PYTHON "@VTK_WRAP_PYTHON@")
SET(VTK_WRAP_TCL "@VTK_WRAP_TCL@")
SET(VTK_WRAP_JAVA "@VTK_WRAP_JAVA@")
SET(VTK_QT_VERSION "@VTK_QT_VERSION@")
SET(VTK_ENABLE_KITS "@VTK_ENABLE_KITS@")

# Do not add options or information here that is specific to a
# particular module.  Instead set <module>_EXPORT_OPTIONS and/or
# <module>_EXPORT_CODE_BUILD and <module>_EXPORT_CODE_INSTALL
# at the top of the module CMakeLists.txt file.
