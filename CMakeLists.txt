cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

project(VTK)

set(VTK_CMAKE_DIR "${VTK_SOURCE_DIR}/CMake")
set(CMAKE_MODULE_PATH ${VTK_CMAKE_DIR} ${CMAKE_MODULE_PATH})

include(vtkModuleMacros)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

# Test output directory.
set(VTK_TEST_OUTPUT_DIR "${VTK_BINARY_DIR}/Testing/Temporary")

# Set up our directory structure for output libraries and binaries
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${VTK_BINARY_DIR}/bin")
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  if(UNIX)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${VTK_BINARY_DIR}/lib")
  else()
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${VTK_BINARY_DIR}/bin")
  endif()
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${VTK_BINARY_DIR}/lib")
endif()
mark_as_advanced(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  CMAKE_LIBRARY_OUTPUT_DIRECTORY
  CMAKE_ARCHIVE_OUTPUT_DIRECTORY
  )

# Choose static or shared libraries.
option(BUILD_SHARED_LIBS "Build VTK with shared libraries." ON)
set(VTK_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

#-----------------------------------------------------------------------------
include(vtkVersion)
set(VTK_VERSION
    "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}")

# VTK installation structure
if(NOT VTK_INSTALL_RUNTIME_DIR)
  set(VTK_INSTALL_RUNTIME_DIR bin)
endif()
if(NOT VTK_INSTALL_LIBRARY_DIR)
  set(VTK_INSTALL_LIBRARY_DIR lib)
endif()
if(NOT VTK_INSTALL_ARCHIVE_DIR)
  set(VTK_INSTALL_ARCHIVE_DIR lib)
endif()
if(NOT VTK_INSTALL_INCLUDE_DIR)
  set(VTK_INSTALL_INCLUDE_DIR include/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION})
endif()
if(NOT VTK_INSTALL_DATA_DIR)
  set(VTK_INSTALL_DATA_DIR share/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION})
endif()
if(NOT VTK_INSTALL_DOC_DIR)
  set(VTK_INSTALL_DOC_DIR share/doc/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION})
endif()
if(NOT VTK_INSTALL_PACKAGE_DIR)
  set(VTK_INSTALL_PACKAGE_DIR "lib/cmake/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")
endif()
if(NOT VTK_INSTALL_DOXYGEN_DIR)
  set(VTK_INSTALL_DOXYGEN_DIR ${VTK_INSTALL_DOC_DIR}/doxygen)
endif()
if(NOT VTK_INSTALL_TCL_DIR)
  # tclsh searches <prefix>/lib/tcltk and its subdirectories for pkgIndex.tcl files
  set(VTK_INSTALL_TCL_DIR lib/tcltk/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION})
endif()
if(NOT VTK_INSTALL_EXPORT_NAME)
  set(VTK_INSTALL_EXPORT_NAME VTKTargets)
endif()
if(NOT VTK_MODULES_DIR)
  set(VTK_MODULES_DIR "${VTK_BINARY_DIR}/${VTK_INSTALL_PACKAGE_DIR}/Modules")
endif()

if (CMAKE_CROSSCOMPILING AND NOT COMPILE_TOOLS_IMPORTED)
  # if CMAKE_CROSSCOMPILING is true, we need to import build-tools targets.
  find_package(VTKCompileTools REQUIRED)
  set (COMPILE_TOOLS_IMPORTED TRUE)
endif()

# FIXME: These will be removed, used by vtkzlib etc
set(VTK_INSTALL_BIN_DIR_CM24 ${VTK_INSTALL_RUNTIME_DIR})
set(VTK_INSTALL_LIB_DIR_CM24 ${VTK_INSTALL_LIBRARY_DIR})
set(VTK_INSTALL_PACKAGE_DIR_CM24 ${VTK_INSTALL_PACKAGE_DIR})
set(VTK_INSTALL_INCLUDE_DIR_CM24 ${VTK_INSTALL_INCLUDE_DIR})
set(VTK_INSTALL_DOXYGEN_DIR_CM24 ${VTK_INSTALL_DATA_DIR}/doxygen)

#-----------------------------------------------------------------------------
# The third party macros are still used in one or two third party builds.
include(vtkThirdParty)

#-----------------------------------------------------------------------------
include(vtkCompilerExtras)
include(vtkBuildPath)

#-----------------------------------------------------------------------------
include(CTest)

#-----------------------------------------------------------------------------
if(APPLE)
  mark_as_advanced(
    CMAKE_OSX_ARCHITECTURES
    CMAKE_OSX_DEPLOYMENT_TARGET
    CMAKE_OSX_SYSROOT
    )
endif()

#-----------------------------------------------------------------------------
# Does VTK require support for 64 bit file systems
INCLUDE(CheckCXXSourceRuns)
FILE(READ "${VTK_CMAKE_DIR}/vtkRequireLargeFilesSupport.cxx"
  VTK_REQUIRE_LARGE_FILE_SUPPORT_FILE)
CHECK_CXX_SOURCE_RUNS("${VTK_REQUIRE_LARGE_FILE_SUPPORT_FILE}"
  CMAKE_REQUIRE_LARGE_FILE_SUPPORT "Support for 64 bit file systems")
SET(VTK_REQUIRE_LARGE_FILE_SUPPORT ${CMAKE_REQUIRE_LARGE_FILE_SUPPORT})

#-----------------------------------------------------------------------------
# Does the const_reverse_iterator have the comparison operators? Before GCC
# 4.1 they were not present.
include(CheckCXXSourceCompiles)
set(VTK_CONST_REVERSE_ITERATOR_COMPARISON_FILE
"#include <vector>
int main()
{
  std::vector<int> test;
  std::vector<int>::const_reverse_iterator it = test.rbegin();
  it != test.rend();
  return 0;
}")
check_cxx_source_compiles("${VTK_CONST_REVERSE_ITERATOR_COMPARISON_FILE}"
  VTK_CONST_REVERSE_ITERATOR_COMPARISON)

#-----------------------------------------------------------------------------
# Provide compatibility options.
option(VTK_LEGACY_REMOVE "Remove all legacy code completely." ON)
option(VTK_LEGACY_SILENT "Silence all legacy code messages." OFF)
mark_as_advanced(VTK_LEGACY_REMOVE VTK_LEGACY_SILENT)

#-----------------------------------------------------------------------------
# VTK requires special compiler flags on some platforms.
include(vtkDetermineCompilerFlags)

# Tell VTK source files they are being built inside VTK.
add_definitions(-DVTK_IN_VTK)

#-----------------------------------------------------------------------------
# Configure the default VTK_DATA_ROOT for the location of VTKData.  To get
# the VTKData repository from git, issue the following command:
#
# git clone git://vtk.org/VTKData.git
#
# To see the web hosted repository, visit http://vtk.org/VTKData.git
FIND_PATH(VTK_DATA_ROOT VTKData.readme
  ${VTK_SOURCE_DIR}/VTKData
  ${VTK_SOURCE_DIR}/../VTKData
  ${VTK_SOURCE_DIR}/../../VTKData
  $ENV{VTK_DATA_ROOT}
  DOC "The repository for data used for testing.  To obtain from git: \"git clone git://vtk.org/VTKData.git\""
  )
mark_as_advanced(VTK_DATA_ROOT)

#-----------------------------------------------------------------------------
# Configure the default VTK_LARGE_DATA_ROOT for the location of VTKLargeData.
# To get the VTKLargeData repository from git, issue the following command:
#
# git clone git://vtk.org/VTKLargeData.git
#
# To see the web hosted repository, visit http://vtk.org/VTKLargeData.git
FIND_PATH(VTK_LARGE_DATA_ROOT VTKLargeData.readme
  ${VTK_SOURCE_DIR}/VTKLargeData
  ${VTK_SOURCE_DIR}/../VTKLargeData
  ${VTK_SOURCE_DIR}/../../VTKLargeData
  $ENV{VTK_LARGE_DATA_ROOT}
  DOC "The repository for large data used for testing.  To obtain from git: \"git clone git://vtk.org/VTKLargeData.git\""
  )
mark_as_advanced(VTK_LARGE_DATA_ROOT)

#-----------------------------------------------------------------------------
# Platform configuration tests.
include(TestBigEndian)
TEST_BIG_ENDIAN(CMAKE_WORDS_BIGENDIAN)

include(FindThreads)
set(VTK_USE_WIN32_THREADS 0)
set(VTK_USE_PTHREADS 0)
set(VTK_HP_PTHREADS 0)
set(VTK_USE_SPROC 0)
if(CMAKE_USE_WIN32_THREADS_INIT)
  set(VTK_USE_WIN32_THREADS 1)
  set(CMAKE_THREAD_LIBS_INIT "")
elseif(CMAKE_USE_PTHREADS_INIT)
  set(VTK_USE_PTHREADS 1)
  if(CMAKE_HP_PTHREADS_INIT)
    set(VTK_HP_PTHREADS 1)
  endif()
elseif(CMAKE_USE_SPROC_INIT)
  set(VTK_USE_SPROC 1)
endif()
set(CMAKE_THREAD_LIBS "${CMAKE_THREAD_LIBS_INIT}" CACHE STRING "Thread library used.")
mark_as_advanced(CMAKE_THREAD_LIBS)
set(VTK_MAX_THREADS "64" CACHE STRING
  "Max number of threads vktMultiThreader will allocate.")
mark_as_advanced(VTK_MAX_THREADS)

include(TestForANSIStreamHeaders)
include(TestForSTDNamespace)
include(TestForANSIForScope)
include(CheckTypeSize)

# Simulate old CMakeBackwardCompatibilityCXX test.
include(TestForSSTREAM)

# Tests for various integer, bool and float types
include(vtkTestTypes)

# Socket tests etc.
#INCLUDE(Parallel/VTKParallelCMakeTests.cmake)

# Check for full template specialization support by compiler.
include(vtkTestFullSpecialization)

# Check for explicit template instantiation support by compiler.
include(vtkTestExplicitInstantiation)

# Test for atomics and other compiler intrinsics
include(vtkTestBuiltins)

#-----------------------------------------------------------------------------

# Check the severity of EOF bugs in the streams library.
# this must be after the test for the long types
INCLUDE(vtkTestStreamsLibrary)

#-----------------------------------------------------------------------------
# Dispatch the build into the proper subdirectories.
SET(VTK_HAS_EXODUS 1)

#-----------------------------------------------------------------------------
# Provide a few configuration options.
OPTION(BUILD_EXAMPLES "Build VTK examples." OFF)

#-----------------------------------------------------------------------------
# Configure files with settings for use by the build.

# Include the sockets test.
# FIXME: The test should be moved and rewritten.
include(VTKParallelCMakeTests)

# Add the option for build the Python wrapping to VTK.
option(VTK_WRAP_PYTHON "Should VTK Python wrapping be built?" OFF)

# Add the option for build the Python wrapping to VTK.
option(VTK_WRAP_JAVA "Should VTK Java wrapping be built?" OFF)

# FIXME: This variable should not be necessary once we are done
set(VTK_IGNORE_BTX ON CACHE INTERNAL "VTK modular always ignores BTX")

# Force reset of hints file location in cache if it was moved
if(VTK_WRAP_HINTS)
  if(NOT EXISTS ${VTK_WRAP_HINTS})
    set(VTK_WRAP_HINTS "VTK_WRAP_HINTS-NOTFOUND")
  endif()
endif()

find_file(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping/Tools
  NO_CMAKE_FIND_ROOT_PATH)
mark_as_advanced(VTK_WRAP_HINTS)

if(BUILD_TESTING OR VTK_WRAP_PYTHON)
  # Need PYTHON_EXECUTABLE for HeaderTesting or python wrapping
  find_package(PythonInterp QUIET)
  mark_as_advanced(PYTHON_EXECUTABLE)
endif()

if(VTK_WRAP_PYTHON)
  set(VTK_WRAP_PYTHON_EXE vtkWrapPython)
  set(VTK_WRAP_PYTHON_INIT_EXE vtkWrapPythonInit)
  set(VTK_PYTHON_EXE vtkpython)
  # Force the WrappingPython module to on if wrapping is on
  set(Module_vtkWrappingPython ON CACHE BOOL "Core Python wrapping library"
    FORCE)
else()
  # if wrapping is not on then force WrappingPython module to be off
  set(Module_vtkWrappingPython OFF CACHE BOOL "Core Python wrapping library"
    FORCE)
endif()

if(VTK_WRAP_JAVA)
  set(VTK_WRAP_JAVA3_INIT_DIR "${VTK_SOURCE_DIR}/Wrapping/Java")
  # Wrapping executables.
  set(VTK_WRAP_JAVA_EXE  vtkWrapJava)
  set(VTK_PARSE_JAVA_EXE vtkParseJava)
  set(Module_vtkWrappingJava ON CACHE BOOL "Core Java wrapping library"
    FORCE)

  # Java package location.
  set(VTK_JAVA_JAR ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/vtk.jar)
  set(VTK_JAVA_HOME ${VTK_BINARY_DIR}/java/vtk)
  file(MAKE_DIRECTORY ${VTK_JAVA_HOME})
else()
  set(Module_vtkWrappingJava OFF CACHE BOOL "Core Java wrapping library"
    FORCE)
endif()

# Now for Tcl wrapping.
option(VTK_WRAP_TCL "Should VTK Tcl wrapping be built?" OFF)
if(VTK_WRAP_TCL)
  set(VTK_WRAP_TCL_EXE vtkWrapTcl)
  set(VTK_WRAP_TCL_INIT_EXE vtkWrapTclInit)
  set(VTK_TCL_EXE vtk)
  # Tcl package location.
  set(VTK_TCL_HOME ${VTK_BINARY_DIR}/Wrapping/Tcl)
  # Force the WrappingTcl module to on if wrapping is on
  set(Module_vtkWrappingTcl ON CACHE BOOL "Core Tcl wrapping library"
    FORCE)
else()
  # if wrapping is not on then force WrappingTcl module to be off
  set(Module_vtkWrappingTcl OFF CACHE BOOL "Core Tcl wrapping library"
    FORCE)
endif()

if(VTK_WRAP_PYTHON OR VTK_WRAP_TCL OR VTK_WRAP_JAVA)
  set(VTK_WRAP_HIERARCHY_EXE vtkWrapHierarchy)
endif()

# This is not normally necessary, but could be desirable in some circumstances.
option(VTK_MAKE_INSTANTIATORS "Should all modules build instantiators" OFF)
mark_as_advanced(VTK_MAKE_INSTANTIATORS)

#----------------------------------------------------------------------
# Find the GhostScript executable for GL2PS tests.
find_program(VTK_GHOSTSCRIPT_EXECUTABLE gs gswin32c gsos2)
mark_as_advanced(VTK_GHOSTSCRIPT_EXECUTABLE)

#----------------------------------------------------------------------
# Load the module DAG.
include(vtkModuleTop)

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
  export(TARGETS ${_vtk_compiletools_targets}
         FILE ${VTK_BINARY_DIR}/VTKCompileToolsConfig.cmake)
  add_dependencies(vtkCompileTools ${_vtk_compiletools_targets})
endif()
unset(_vtk_targets)
unset(_vtk_compiletools_targets)
unset(_vtk_all_targets)
