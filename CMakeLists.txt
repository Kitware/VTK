cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

if(CMAKE_SYSTEM_NAME STREQUAL "Android" AND CMAKE_VERSION VERSION_LESS 3.7)
  message(FATAL_ERROR "CMake 3.7 or above is required to build for Android")
endif()

project(VTK)

set(VTK_CMAKE_DIR "${VTK_SOURCE_DIR}/CMake")
set(CMAKE_MODULE_PATH ${VTK_CMAKE_DIR} ${CMAKE_MODULE_PATH})

if (APPLE)
  include(vtkApple)
endif ()

include(vtkCompilerChecks)
include(vtkCompilerPlatformFlags)
include(vtkCompilerExtraFlags)
include(vtkInitializeBuildType)
include(vtkVersion)
set(VTK_VERSION
    "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}")
include(vtkSupportMacros)
include(vtkDownload)
include(vtkTesting)
include(vtkDirectories)
include(vtkMobileDevices)
include(vtkCrossCompiling)
include(vtkThirdParty)
include(vtkThreads)
include(vtkWrapSettings)

# Setup compiler flags for dynamic analysis
# Should be included after vtkTesting
include(vtkCompilerDynamicAnalysisFlags)

include(vtkModuleMacros)

#-----------------------------------------------------------------------------
# Do we want examples built?
option(BUILD_EXAMPLES "Build VTK examples." OFF)

set(VTK_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

#-----------------------------------------------------------------------------
# Add the Remote Subdirectory
add_subdirectory(Remote)

#-----------------------------------------------------------------------------
# Does VTK require support for 64 bit file systems
include(CheckCXXSourceRuns)
file(READ "${VTK_CMAKE_DIR}/vtkRequireLargeFilesSupport.cxx"
  VTK_REQUIRE_LARGE_FILE_SUPPORT_FILE)
check_cxx_source_runs("${VTK_REQUIRE_LARGE_FILE_SUPPORT_FILE}"
  CMAKE_REQUIRE_LARGE_FILE_SUPPORT "Support for 64 bit file systems")
set(VTK_REQUIRE_LARGE_FILE_SUPPORT ${CMAKE_REQUIRE_LARGE_FILE_SUPPORT})

#-----------------------------------------------------------------------------
# Provide compatibility options.
option(VTK_LEGACY_REMOVE "Remove all legacy code completely." OFF)
option(VTK_LEGACY_SILENT "Silence all legacy code messages." OFF)
mark_as_advanced(VTK_LEGACY_REMOVE VTK_LEGACY_SILENT)

# Tell VTK source files they are being built inside VTK.
add_definitions(-DVTK_IN_VTK)

include(CheckTypeSize)

# Kits bundle multiple modules together into a single library, this
# is used to dramatically reduce the number of generated libraries.
option(VTK_ENABLE_KITS "Build VTK using kits instead of modules." OFF)
mark_as_advanced(VTK_ENABLE_KITS)

include(vtkEncodeString)

#----------------------------------------------------------------------
# Load the module DAG, assess all modules etc.
include(vtkModuleTop)

# Now build the Python wrapping if necessary.
if(VTK_WRAP_PYTHON)
  add_subdirectory(Wrapping/Python)
endif()

if(BUILD_TESTING)
  add_subdirectory(Testing/Install)
endif()

if(BUILD_EXAMPLES)
  add_subdirectory(Examples)
endif()

# The doxygen documentation needs to be aware of all modules.
option(BUILD_DOCUMENTATION "Build the VTK documentation" OFF)
if(BUILD_DOCUMENTATION)
  add_subdirectory(Utilities/Doxygen)
endif()

# If python wrapping and testing is enabled then add driver scripts to run
# tests.  Note: Many pythong tests used to be automatically converted from TCL
# scripts. Hence the name vtkTclTest2Py
if(BUILD_TESTING AND VTK_WRAP_PYTHON)
  add_subdirectory(Utilities/vtkTclTest2Py)
endif()

# Configure the CTestCustom.cmake file now that everything is done.
set(memcheck_excluded_tests "  # These tests do not run any VTK code\n")
foreach(vtk-module ${VTK_MODULES_ALL})
  set(memcheck_excluded_tests
    "${memcheck_excluded_tests}  ${vtk-module}-HeaderTest\n")
endforeach()
configure_file("${VTK_CMAKE_DIR}/CTestCustom.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake" @ONLY)

#-----------------------------------------------------------------------------
# Export all targets at once from the build tree in their final configuration.
get_property(_vtk_targets GLOBAL PROPERTY VTK_TARGETS)
get_property(_vtk_compiletools_targets GLOBAL PROPERTY VTK_COMPILETOOLS_TARGETS)
set (_vtk_all_targets ${_vtk_targets} ${_vtk_compiletools_targets})
if (_vtk_all_targets)
  list(REMOVE_DUPLICATES _vtk_all_targets)
  export(TARGETS ${_vtk_all_targets} FILE ${VTK_BINARY_DIR}/VTKTargets.cmake)
endif()
# Add a virtual target that can be used to build all compile tools.
add_custom_target(vtkCompileTools)
if (_vtk_compiletools_targets)
  list(REMOVE_DUPLICATES _vtk_compiletools_targets)
  add_dependencies(vtkCompileTools ${_vtk_compiletools_targets})
endif()
export(TARGETS ${_vtk_compiletools_targets}
  FILE ${VTK_BINARY_DIR}/VTKCompileToolsConfig.cmake)

unset(_vtk_targets)
unset(_vtk_compiletools_targets)
unset(_vtk_all_targets)

# Create target to download data from the VTKData group.  This must come after
# all tests have been added that reference the group, so we put it last.
ExternalData_Add_Target(VTKData)
if(VTK_DATA_EXCLUDE_FROM_ALL)
  set_property(TARGET VTKData PROPERTY EXCLUDE_FROM_ALL 1)
  if(BUILD_TESTING AND NOT VTK_DATA_EXCLUDE_FROM_ALL_NO_WARNING)
    message(WARNING "VTK_DATA_EXCLUDE_FROM_ALL is ON so test data "
      "(needed because BUILD_TESTING is ON) may not be available "
      "without manually building the 'VTKData' target.")
  endif()
endif()

#-----------------------------------------------------------------------------
# Install the license file
install(FILES ${VTK_SOURCE_DIR}/Copyright.txt DESTINATION ${VTK_INSTALL_DOC_DIR})

#-----------------------------------------------------------------------------
# Do this at the end so the all variables it uses are setup.
include(vtkBuildPath)
