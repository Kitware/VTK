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

include(vtkModuleMacros)

#-----------------------------------------------------------------------------
# Do we want examples built?
option(BUILD_EXAMPLES "Build VTK examples." OFF)

set(VTK_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

#-----------------------------------------------------------------------------
# Add the Remote Subdirectory
add_subdirectory(Remote)

#-----------------------------------------------------------------------------
include(vtkCompilerExtras)

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

#-----------------------------------------------------------------------------
# VTK requires special compiler flags on some platforms.
include(vtkDetermineCompilerFlags)

# Tell VTK source files they are being built inside VTK.
add_definitions(-DVTK_IN_VTK)

#-----------------------------------------------------------------------------
# Platform configuration tests.
include(TestBigEndian)
TEST_BIG_ENDIAN(CMAKE_WORDS_BIGENDIAN)

include(FindThreads)
set(VTK_USE_WIN32_THREADS 0)
set(VTK_USE_PTHREADS 0)
# if win32 threads and pthreads are available figure out which
# one the compiler is setup to use.  If you can not figure it
# out default to pthreads.
if(CMAKE_USE_PTHREADS_INIT AND CMAKE_USE_WIN32_THREADS_INIT)
  if(DEFINED VTK_THREAD_MODEL)
    set(output "${VTK_THREAD_MODEL}")
  else()
    execute_process(COMMAND "${CMAKE_C_COMPILER}" -v OUTPUT_VARIABLE output
      ERROR_VARIABLE output RESULT_VARIABLE result TIMEOUT 10)
  endif()
  if(output MATCHES "Thread model: posix")
    set(VTK_THREAD_MODEL "Thread model: posix" CACHE STRING
      "Thread model used by gcc.")
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
  elseif(output MATCHES "Thread model: win32")
    set(VTK_THREAD_MODEL "Thread model: win32" CACHE STRING
      "Thread model used by gcc.")
    set(CMAKE_USE_PTHREADS_INIT 0)
  else()
    set(VTK_THREAD_MODEL "Thread model: posix" CACHE STRING
      "Thread model used by gcc.")
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
  endif()
endif()
mark_as_advanced(VTK_THREAD_MODEL)
if(CMAKE_USE_WIN32_THREADS_INIT)
  set(VTK_USE_WIN32_THREADS 1)
  set(CMAKE_THREAD_LIBS_INIT "")
elseif(CMAKE_USE_PTHREADS_INIT)
  set(VTK_USE_PTHREADS 1)
endif()
set(CMAKE_THREAD_LIBS "${CMAKE_THREAD_LIBS_INIT}" CACHE STRING "Thread library used.")
mark_as_advanced(CMAKE_THREAD_LIBS)
set(VTK_MAX_THREADS "64" CACHE STRING
  "Max number of threads vtkMultiThreader will allocate.")
mark_as_advanced(VTK_MAX_THREADS)

include(CheckTypeSize)

#-----------------------------------------------------------------------------
# Configure files with settings for use by the build.

# Add the option for build the Python wrapping to VTK.
option(VTK_WRAP_PYTHON "Should VTK Python wrapping be built?" OFF)
set(VTK_PYTHON_VERSION 2 CACHE STRING
    "Python version to use: 2, 2.x, 3, 3.x, or empty")

# Add the option for build the Python wrapping to VTK.
option(VTK_WRAP_JAVA "Should VTK Java wrapping be built?" OFF)

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
  find_package(PythonInterp ${VTK_PYTHON_VERSION} QUIET)
  mark_as_advanced(PYTHON_EXECUTABLE)
endif()

if(VTK_WRAP_PYTHON)
  set(VTK_WRAP_PYTHON_EXE vtkWrapPython)
  set(VTK_WRAP_PYTHON_INIT_EXE vtkWrapPythonInit)
  # Force the WrappingPythonCore module to on if wrapping is on
  set(Module_vtkWrappingPythonCore ON CACHE BOOL "Core Python wrapping library"
    FORCE)
else()
  # if wrapping is not on then force WrappingPythonCore module to be off
  set(Module_vtkWrappingPythonCore OFF CACHE BOOL "Core Python wrapping library"
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

option(VTK_WRAP_HIERARCHY "Create wrap hierarchy even if languages are disabled." OFF)
mark_as_advanced(VTK_WRAP_HIERARCHY)
if(VTK_WRAP_PYTHON OR VTK_WRAP_JAVA OR VTK_WRAP_HIERARCHY)
  set(VTK_WRAP_HIERARCHY_EXE vtkWrapHierarchy)
endif()

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
  export(TARGETS ${_vtk_compiletools_targets}
         FILE ${VTK_BINARY_DIR}/VTKCompileToolsConfig.cmake)
  add_dependencies(vtkCompileTools ${_vtk_compiletools_targets})
endif()

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
