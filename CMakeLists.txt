cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
foreach(policy
    CMP0083 # CMake 3.14
    CMP0071 # CMake 3.10
    )
  if(POLICY ${policy})
    cmake_policy(SET ${policy} NEW)
  endif()
endforeach()

if (POLICY CMP0063)
  cmake_policy(SET CMP0063 NEW)
endif ()
if (POLICY CMP0065)
  cmake_policy(SET CMP0065 NEW)
endif ()

project(VTK)

set(vtk_cmake_dir "${VTK_SOURCE_DIR}/CMake")
list(INSERT CMAKE_MODULE_PATH 0 "${vtk_cmake_dir}")

if (APPLE)
  include(vtkApple)
endif ()

# must be before the following iOS / Android
include(vtkVersion)
set(VTK_VERSION
    "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}")

option(VTK_IOS_BUILD "Build vtk.framework for iOS" OFF)
mark_as_advanced(VTK_IOS_BUILD)
if (VTK_IOS_BUILD)
  include(vtkiOS)
  return()
endif()

option(VTK_ANDROID_BUILD "Build VTK for Android" OFF)
mark_as_advanced(VTK_ANDROID_BUILD)
if (VTK_ANDROID_BUILD)
  include(vtkAndroid)
  return()
endif()

include(vtkCompilerChecks)
include(vtkCompilerPlatformFlags)
include(vtkCompilerExtraFlags)
include(vtkInitializeBuildType)
include(vtkSupportMacros)
include(vtkDownload)
include(vtkTesting)
include(vtkDirectories)
include(vtkMobileDevices)
include(vtkCrossCompiling)
include(vtkWrapSettings)
include(vtkObjectFactory)
include(vtkCMakeBackports)

# Setup compiler flags for dynamic analysis
# Should be included after vtkTesting
include(vtkCompilerDynamicAnalysisFlags)

set(VTK_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(CMAKE_CXX_VISIBILITY_PRESET "hidden")
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

#-----------------------------------------------------------------------------
# Add the Remote Subdirectory
add_subdirectory(Remote)

#-----------------------------------------------------------------------------
# Does VTK require support for 64 bit file systems
include(CheckCXXSourceRuns)
file(READ "${vtk_cmake_dir}/vtkRequireLargeFilesSupport.cxx"
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

option(VTK_USE_MPI "Support MPI" OFF)

set(VTK_BUILD_TESTING "WANT"
  CACHE STRING "Build module testing directories")
set_property(CACHE VTK_BUILD_TESTING
  PROPERTY
    STRINGS "ON;OFF;WANT")
set(BUILD_TESTING OFF)
if (VTK_BUILD_TESTING)
  set(BUILD_TESTING ON)
endif ()

#-----------------------------------------------------------------------------
# Add an option to enable/disable components that have CUDA.
option(VTK_USE_CUDA "Support CUDA compilation" OFF)
if(VTK_USE_CUDA)
  # This needs to be a top level flag as enable_language needs
  # to be called from CMake entry point for variables to have
  # the correct scope.
  enable_language(CUDA)
endif()

#-----------------------------------------------------------------------------
# Add an option to enable/disable logging support.
option(VTK_ENABLE_LOGGING "Enable logging support." ON)
mark_as_advanced(VTK_ENABLE_LOGGING)

include(vtkEncodeString)

if (VTK_WHEEL_BUILD)
  include(vtkWheelPreparation)
endif ()

# Set up our directory structure for output libraries and binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${VTK_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${VTK_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${VTK_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

set(vtk_requested_modules)
set(vtk_rejected_modules)
if (VTK_WRAP_PYTHON)
  list(APPEND vtk_requested_modules
    VTK::WrappingPythonCore
    VTK::WrappingTools
    VTK::PythonInterpreter)
else ()
  list(APPEND vtk_rejected_modules
    VTK::Python)
endif ()
if (VTK_WRAP_JAVA)
  list(APPEND vtk_requested_modules
    VTK::Java
    VTK::WrappingTools)
else ()
  list(APPEND vtk_rejected_modules
    VTK::Java)
endif ()
# For vtkTkRenderWidget (only useful with Python, since WRAP_TCL is gone)
if (VTK_USE_TK)
  set(VTK_GROUP_ENABLE_Tk YES)
endif ()
if (VTK_USE_MPI)
  set(VTK_GROUP_ENABLE_MPI YES)
else ()
  # Reject `VTK::mpi`, but allow those which happen to be part of the group to
  # be built.
  set(_vtk_module_group_default_MPI "DONT_WANT")
  list(APPEND vtk_rejected_modules
    VTK::mpi)
endif ()
if (VTK_ENABLE_LOGGING)
  list(APPEND vtk_requested_modules
    VTK::loguru)
endif ()

if (VTK_BUILD_TESTING)
  list(APPEND vtk_requested_modules
    # Required for just about all VTK tests.
    VTK::TestingCore)
endif ()

list(APPEND vtk_requested_modules
  # Required for just about everything.
  VTK::CommonCore
  VTK::CommonDataModel
  # Required for wrapping with hierarchy files.
  VTK::WrappingTools)

if (vtk_requested_modules)
  list(REMOVE_DUPLICATES vtk_requested_modules)
endif ()
if (vtk_rejected_modules)
  list(REMOVE_DUPLICATES vtk_rejected_modules)
endif ()

set_property(GLOBAL
  PROPERTY
    "_vtk_module_autoinit_include" "\"vtkAutoInit.h\"")

option(VTK_BUILD_ALL_MODULES "Build all modules by default" OFF)
mark_as_advanced(VTK_BUILD_ALL_MODULES)

# Turn on these groups by default.
set(_vtk_module_group_default_StandAlone "WANT")
set(_vtk_module_group_default_Rendering "WANT")

set(vtk_enable_tests "${VTK_BUILD_TESTING}")
if (VTK_FORBID_DOWNLOADS)
  message(STATUS "Module testing disabled due to `VTK_FORBID_DOWNLOADS` being set.")
  set(vtk_enable_tests "OFF")
endif ()

include(vtkModule)
include(vtkModuleDebugging)
set(vtk_source_directories
  "${CMAKE_CURRENT_SOURCE_DIR}/Accelerators"
  "${CMAKE_CURRENT_SOURCE_DIR}/Charts"
  "${CMAKE_CURRENT_SOURCE_DIR}/Common"
  "${CMAKE_CURRENT_SOURCE_DIR}/Domains"
  "${CMAKE_CURRENT_SOURCE_DIR}/Filters"
  "${CMAKE_CURRENT_SOURCE_DIR}/Geovis"
  "${CMAKE_CURRENT_SOURCE_DIR}/GUISupport"
  "${CMAKE_CURRENT_SOURCE_DIR}/Imaging"
  "${CMAKE_CURRENT_SOURCE_DIR}/Infovis"
  "${CMAKE_CURRENT_SOURCE_DIR}/Interaction"
  "${CMAKE_CURRENT_SOURCE_DIR}/IO"
  "${CMAKE_CURRENT_SOURCE_DIR}/Parallel"
  "${CMAKE_CURRENT_SOURCE_DIR}/Remote"
  "${CMAKE_CURRENT_SOURCE_DIR}/Rendering"
  "${CMAKE_CURRENT_SOURCE_DIR}/Testing"
  "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty"
  "${CMAKE_CURRENT_SOURCE_DIR}/Utilities"
  "${CMAKE_CURRENT_SOURCE_DIR}/Views"
  "${CMAKE_CURRENT_SOURCE_DIR}/Web"
  "${CMAKE_CURRENT_SOURCE_DIR}/Wrapping")
vtk_module_find_modules(vtk_module_files ${vtk_source_directories})
vtk_module_find_kits(vtk_kit_files ${vtk_source_directories})

# If we're building only the compile tools, limit what is done.
option(VTK_BUILD_COMPILE_TOOLS_ONLY "Build only VTK's compile tools" OFF)
mark_as_advanced(VTK_BUILD_COMPILE_TOOLS_ONLY)
if (VTK_BUILD_COMPILE_TOOLS_ONLY)
  set(vtk_requested_modules
    VTKCompileTools::WrappingTools)
  set(vtk_rejected_modules)
  set(vtk_module_files
    "${CMAKE_CURRENT_SOURCE_DIR}/Wrapping/Tools/vtkcompiletools.module")
  set(vtk_kit_files)
endif ()

cmake_dependent_option(VTK_INSTALL_SDK "Install SDK components" ON
  "NOT VTK_BUILD_COMPILE_TOOLS_ONLY" ON)
mark_as_advanced(VTK_INSTALL_SDK)

vtk_module_scan(
  MODULE_FILES        ${vtk_module_files}
  KIT_FILES           ${vtk_kit_files}
  REQUEST_MODULES     ${vtk_requested_modules}
  REJECT_MODULES      ${vtk_rejected_modules}
  PROVIDES_MODULES    vtk_modules
  PROVIDES_KITS       vtk_kits
  REQUIRES_MODULES    vtk_required_modules
  UNRECOGNIZED_MODULES vtk_unrecognized_modules
  WANT_BY_DEFAULT     "${VTK_BUILD_ALL_MODULES}"
  ENABLE_TESTS        "${vtk_enable_tests}")

if (vtk_required_modules OR vtk_unrecognized_modules)
  message(FATAL_ERROR
    "The following modules were requested or required, but not found: "
    "${vtk_required_modules};${vtk_unrecognized_modules}.")
endif ()

if ("VTK::Python" IN_LIST vtk_modules)
  include(vtkModuleWrapPython)
  if (NOT DEFINED VTK_PYTHON_SITE_PACKAGES_SUFFIX)
    vtk_module_python_default_destination(VTK_PYTHON_SITE_PACKAGES_SUFFIX)
  endif ()
endif ()

option(VTK_ENABLE_KITS "Enable kits compilation" OFF)
mark_as_advanced(VTK_ENABLE_KITS)

option(VTK_USE_EXTERNAL "Use external copies of third party libraries by default" OFF)
mark_as_advanced(VTK_USE_EXTERNAL)

option(VTK_VERSIONED_INSTALL "Install artifacts with versioned names." ON)
mark_as_advanced(VTK_VERSIONED_INSTALL)
set(vtk_version_suffix "")
if (VTK_VERSIONED_INSTALL)
  set(vtk_version_suffix "-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")
endif ()

set(vtk_install_export VTK)
if (NOT DEFINED vtk_cmake_destination)
  set(vtk_cmake_destination
    "${CMAKE_INSTALL_LIBDIR}/cmake/vtk${vtk_version_suffix}")
  set(vtk_target_package)
  if (VTK_BUILD_COMPILE_TOOLS_ONLY)
    set(vtk_install_export VTKCompileTools)
    set(vtk_cmake_destination
      "${CMAKE_INSTALL_LIBDIR}/cmake/vtkcompiletools${vtk_version_suffix}")
    set(vtk_target_package PACKAGE "${vtk_install_export}")
  endif ()
endif ()

set(VTK_CUSTOM_LIBRARY_SUFFIX "<DEFAULT>"
  CACHE STRING "Custom library file name suffix (defaults to the version number)")
mark_as_advanced(VTK_CUSTOM_LIBRARY_SUFFIX)
if (VTK_CUSTOM_LIBRARY_SUFFIX STREQUAL "<DEFAULT>")
  if (VTK_VERSIONED_INSTALL)
    set(VTK_CUSTOM_LIBRARY_SUFFIX "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")
  else ()
    set(VTK_CUSTOM_LIBRARY_SUFFIX "")
  endif ()
endif ()

vtk_module_build(
  MODULES             ${vtk_modules}
  KITS                ${vtk_kits}
  INSTALL_EXPORT      "${vtk_install_export}"
  ${vtk_target_package}
  HEADERS_DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/vtk${vtk_version_suffix}"
  ${vtk_hierarchy_destination_args}
  CMAKE_DESTINATION   "${vtk_cmake_destination}"
  LIBRARY_NAME_SUFFIX "${VTK_CUSTOM_LIBRARY_SUFFIX}"
  VERSION             "${VTK_VERSION}"
  SOVERSION           "1"
  TEST_DATA_TARGET    VTKData
  INSTALL_HEADERS     "${VTK_INSTALL_SDK}"
  BUILD_WITH_KITS     "${VTK_ENABLE_KITS}"
  USE_EXTERNAL        "${VTK_USE_EXTERNAL}"
  TEST_INPUT_DATA_DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/Testing"
  TEST_OUTPUT_DATA_DIRECTORY  "${CMAKE_CURRENT_BINARY_DIR}/ExternalData/Testing")
include(vtkModuleJson)
vtk_module_json(
  MODULES ${vtk_modules}
  OUTPUT  "modules.json")
include(vtkModuleGraphviz)
vtk_module_graphviz(
  MODULES ${vtk_modules}
  KIT_CLUSTERS ON
  PRIVATE_DEPENDENCIES OFF
  OUTPUT  "modules.dot")

if (VTK_WRAP_PYTHON)
  get_property(vtk_required_python_modules GLOBAL
    PROPERTY  vtk_required_python_modules)
  if (vtk_required_python_modules)
    list(REMOVE_DUPLICATES vtk_required_python_modules)
  endif ()
  string(REPLACE ";" "\n" vtk_required_python_modules "${vtk_required_python_modules}")
  file(WRITE "${CMAKE_BINARY_DIR}/requirements.txt"
    "${vtk_required_python_modules}\n")
  vtk_module_wrap_python(
    MODULES         ${vtk_modules}
    INSTALL_EXPORT  VTKPython
    PYTHON_PACKAGE  "vtkmodules"
    LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    MODULE_DESTINATION  "${VTK_PYTHON_SITE_PACKAGES_SUFFIX}"
    CMAKE_DESTINATION   "${vtk_cmake_destination}"
    INSTALL_HEADERS     "${VTK_INSTALL_SDK}"
    SOABI               "${vtk_soabi}"
    WRAPPED_MODULES vtk_python_wrapped_modules
    TARGET          VTK::vtkpythonmodules)

  add_subdirectory(Wrapping/Python)

  export(
    EXPORT    VTKPython
    NAMESPACE VTK::
    FILE      "${CMAKE_BINARY_DIR}/${vtk_cmake_destination}/VTKPython-targets.cmake")
  install(
    EXPORT      VTKPython
    NAMESPACE   VTK::
    FILE        VTKPython-targets.cmake
    DESTINATION "${vtk_cmake_destination}"
    COMPONENT   "development")
endif ()

if (VTK_WRAP_JAVA)
  add_subdirectory(Wrapping/Java)
endif ()

ExternalData_Add_Target(VTKData)

# Create target to download data from the VTKData group.  This must come after
# all tests have been added that reference the group, so we put it last.
if(VTK_DATA_EXCLUDE_FROM_ALL)
  set_property(TARGET VTKData PROPERTY EXCLUDE_FROM_ALL 1)
  if(BUILD_TESTING AND NOT VTK_DATA_EXCLUDE_FROM_ALL_NO_WARNING)
    message(WARNING "VTK_DATA_EXCLUDE_FROM_ALL is ON so test data "
      "(needed because BUILD_TESTING is ON) may not be available "
      "without manually building the 'VTKData' target.")
  endif()
endif()

if (VTK_INSTALL_SDK)
  set(vtk_cmake_build_dir
    "${CMAKE_CURRENT_BINARY_DIR}/${vtk_cmake_destination}")
  if (NOT VTK_BUILD_COMPILE_TOOLS_ONLY)
    include(vtkInstallCMakePackage)
  else ()
    include(vtkInstallCMakePackageCompileTools)
  endif ()
endif ()

cmake_dependent_option(VTK_ENABLE_CDASH_THIRD_PARTY_WARNINGS
  "Suppress warnings from third-party libraries from showing on CDash" OFF
  "VTK_BUILD_TESTING" OFF)
mark_as_advanced(VTK_ENABLE_CDASH_THIRD_PARTY_WARNINGS)

configure_file(
  "${vtk_cmake_dir}/CTestCustom.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake"
  @ONLY)

option(VTK_BUILD_EXAMPLES "Build VTK examples." OFF)
if (VTK_BUILD_EXAMPLES)
  add_subdirectory(Examples)
endif ()

if(BUILD_TESTING)
  add_subdirectory(Testing/Install)
endif()

# The doxygen documentation needs to be aware of all modules.
option(VTK_BUILD_DOCUMENTATION "Build the VTK documentation" OFF)
if(VTK_BUILD_DOCUMENTATION)
  add_subdirectory(Utilities/Doxygen)
endif()

# If python wrapping and testing is enabled then add driver scripts to run
# tests.  Note: Many python tests used to be automatically converted from TCL
# scripts. Hence the name vtkTclTest2Py
if(BUILD_TESTING AND VTK_WRAP_PYTHON)
  add_subdirectory(Utilities/vtkTclTest2Py)
endif()

if (NOT TARGET uninstall)
  add_custom_target(uninstall
    COMMAND
      "${CMAKE_COMMAND}"
        "-DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}"
        -P "${CMAKE_CURRENT_LIST_DIR}/CMake/vtkUninstall.cmake"
    USES_TERMINAL
    COMMENT "Uninstalling VTK")
endif ()

install(
  FILES       "${CMAKE_CURRENT_LIST_DIR}/Copyright.txt"
  DESTINATION "${CMAKE_INSTALL_DOCDIR}"
  COMPONENT   "license")

# TODO: HeaderTest exclusions for memcheck.

if (VTK_WHEEL_BUILD)
  include(vtkWheelFinalization)
endif ()

#-----------------------------------------------------------------------------
# Do this at the end so the all variables it uses are setup.
include(vtkBuildPath)
