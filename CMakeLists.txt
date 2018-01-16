cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
foreach(policy
    CMP0083 # CMake 3.14
    )
  if(POLICY ${policy})
    cmake_policy(SET ${policy} NEW)
  endif()
endforeach()

if(CMAKE_SYSTEM_NAME STREQUAL "Android" AND CMAKE_VERSION VERSION_LESS 3.7)
  message(FATAL_ERROR "CMake 3.7 or above is required to build for Android")
endif()

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

#-----------------------------------------------------------------------------
# Do we want examples built?
option(BUILD_EXAMPLES "Build VTK examples." OFF)

set(VTK_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

#-----------------------------------------------------------------------------
# Add the Remote Subdirectory
#add_subdirectory(Remote)

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

# Kits bundle multiple modules together into a single library, this
# is used to dramatically reduce the number of generated libraries.
option(VTK_ENABLE_KITS "Build VTK using kits instead of modules." OFF)
mark_as_advanced(VTK_ENABLE_KITS)

option(VTK_USE_MPI "Support MPI" OFF)

set(VTK_BUILD_TESTING "DEFAULT"
  CACHE STRING "Build all modules by default")
set_property(CACHE VTK_BUILD_TESTING
  PROPERTY
    STRINGS "ON;OFF;DEFAULT")

include(vtkEncodeString)

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
if (VTK_USE_MPI)
  set(VTK_GROUP_ENABLE_MPI ON)
else ()
  # Reject `VTK::mpi`, but allow those which happen to be part of the group to
  # be built.
  set(VTK_GROUP_ENABLE_MPI DONT_WANT)
  list(APPEND vtk_rejected_modules
    VTK::mpi)
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

include(vtkModule)
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
  "${CMAKE_CURRENT_SOURCE_DIR}/Rendering"
  "${CMAKE_CURRENT_SOURCE_DIR}/Testing"
  "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty"
  "${CMAKE_CURRENT_SOURCE_DIR}/Utilities"
  "${CMAKE_CURRENT_SOURCE_DIR}/Views"
  "${CMAKE_CURRENT_SOURCE_DIR}/Web"
  "${CMAKE_CURRENT_SOURCE_DIR}/Wrapping")
vtk_module_find_modules(vtk_module_files ${vtk_source_directories})
vtk_module_find_kits(vtk_kit_files ${vtk_source_directories})
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
  ENABLE_TESTS        "${VTK_BUILD_TESTING}")

if (vtk_required_modules OR vtk_unrecognized_modules)
  message(FATAL_ERROR
    "The following modules were requested or required, but not found: "
    "${vtk_required_modules};${vtk_unrecognized_modules}.")
endif ()

list(FIND vtk_modules "VTK::Python" vtk_python_idx)
if (NOT vtk_python_idx EQUAL "-1")
  include(vtkModuleWrapPython)
  vtk_module_python_default_destination(VTK_PYTHON_SITE_PACKAGES_SUFFIX)
endif ()

option(VTK_ENABLE_KITS "Enable kits compilation" OFF)
mark_as_advanced(VTK_ENABLE_KITS)

option(VTK_USE_EXTERNAL "Use external copies of third party libraries by default" OFF)
mark_as_advanced(VTK_USE_EXTERNAL)

vtk_module_build(
  MODULES             ${vtk_modules}
  KITS                ${vtk_kits}
  INSTALL_EXPORT      VTK
  HEADERS_DESTINATION "include/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}"
  CMAKE_DESTINATION   "lib/cmake/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}"
  LIBRARY_NAME_SUFFIX "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}"
  VERSION             "${VTK_VERSION}"
  SOVERSION           "1"
  TEST_DATA_TARGET    VTKData
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

ExternalData_Add_Target(VTKData)

if (VTK_WRAP_PYTHON)
  vtk_module_wrap_python(
    MODULES         ${vtk_modules}
    INSTALL_EXPORT  VTK
    PYTHON_PACKAGE  "vtkmodules"
    MODULE_DESTINATION  "${VTK_PYTHON_SITE_PACKAGES_SUFFIX}"
    CMAKE_DESTINATION   "lib/cmake/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}"
    WRAPPED_MODULES vtk_python_wrapped_modules
    TARGET          VTK::vtkpythonmodules)

  add_subdirectory(Wrapping/Python)
endif ()

if (VTK_WRAP_JAVA)
  add_subdirectory(Wrapping/Java)
endif ()

set(vtk_cmake_build_dir
  "${CMAKE_CURRENT_BINARY_DIR}/lib/cmake/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")

if (TARGET "VTK::vtkm")
  set(vtk_has_vtkm ON)
else ()
  set(vtk_has_vtkm OFF)
endif ()

configure_file(
  "${vtk_cmake_dir}/vtk-config.cmake.in"
  "${vtk_cmake_build_dir}/vtk-config.cmake"
  @ONLY)
include(CMakePackageConfigHelpers)
write_basic_package_version_file("${vtk_cmake_build_dir}/vtk-config-version.cmake"
  VERSION "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}"
  COMPATIBILITY SameMinorVersion)

set(vtk_cmake_module_files
  FindADIOS1.cmake
  Finddouble-conversion.cmake
  FindEigen3.cmake
  FindEXPAT.cmake
  FindFFMPEG.cmake
  FindFontConfig.cmake
  FindFreetype.cmake
  FindGL2PS.cmake
  FindGLEW.cmake
  FindJsonCpp.cmake
  FindLibHaru.cmake
  FindLibPROJ.cmake
  FindLibXml2.cmake
  FindLZ4.cmake
  FindLZMA.cmake
  Findmpi4py.cmake
  FindMySQL.cmake
  FindNetCDF.cmake
  FindODBC.cmake
  FindOGG.cmake
  FindOpenMP.cmake
  FindOpenSlide.cmake
  FindOpenVR.cmake
  FindOptiX.cmake
  FindOSMesa.cmake
  FindPostgreSQL.cmake
  FindPythonModules.cmake
  FindTBB.cmake
  FindTHEORA.cmake

  vtkCMakeBackports.cmake
  vtkEncodeString.cmake
  vtkExternalData.cmake
  vtkHashSource.cmake
  vtkModule.cmake
  vtkModuleGraphviz.cmake
  vtkModuleJson.cmake
  vtkModuleTesting.cmake
  vtkModuleWrapJava.cmake
  vtkModuleWrapPython.cmake
  vtkObjectFactory.cmake
  vtkObjectFactory.cxx.in
  vtkObjectFactory.h.in
  vtkTestingRenderingDriver.cmake
  vtkTopologicalSort.cmake)
set(vtk_cmake_patch_files
  patches/3.7/FindPNG.cmake
  patches/3.7/FindTIFF.cmake
  patches/3.7/exportheader.cmake.in
  patches/3.7/GenerateExportHeader.cmake
  patches/3.10/FindMPI/fortranparam_mpi.f90.in
  patches/3.10/FindMPI/libver_mpi.c
  patches/3.10/FindMPI/libver_mpi.f90.in
  patches/3.10/FindMPI/mpiver.f90.in
  patches/3.10/FindMPI/test_mpi.c
  patches/3.10/FindMPI/test_mpi.f90.in
  patches/3.10/FindMPI.cmake
  patches/3.13/FindZLIB.cmake
  patches/3.14/FindGDAL.cmake
  patches/3.14/FindJPEG.cmake
  patches/3.14/FindOpenGL.cmake
  patches/3.14/FindSQLite3.cmake
  patches/3.14/FindX11.cmake)

set(vtk_cmake_files_to_install)
foreach (vtk_cmake_module_file IN LISTS vtk_cmake_module_files vtk_cmake_patch_files)
  configure_file(
    "${vtk_cmake_dir}/${vtk_cmake_module_file}"
    "${vtk_cmake_build_dir}/${vtk_cmake_module_file}"
    COPYONLY)
  list(APPEND vtk_cmake_files_to_install
    "${vtk_cmake_module_file}")
endforeach ()

foreach (vtk_cmake_file IN LISTS vtk_cmake_files_to_install)
  get_filename_component(subdir "${vtk_cmake_file}" DIRECTORY)
  install(
    FILES       "${vtk_cmake_dir}/${vtk_cmake_file}"
    DESTINATION "lib/cmake/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}/${subdir}"
    COMPONENT   "development")
endforeach ()

install(
  FILES       "${vtk_cmake_build_dir}/vtk-config.cmake"
              "${vtk_cmake_build_dir}/vtk-config-version.cmake"
  DESTINATION "lib/cmake/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}"
  COMPONENT   "development")

vtk_module_export_find_packages(
  CMAKE_DESTINATION "lib/cmake/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}"
  FILE_NAME         "VTK-vtk-module-find-packages.cmake"
  MODULES           ${vtk_modules})

if(BUILD_TESTING)
  add_subdirectory(Testing/Install)
endif()

if(BUILD_EXAMPLES)
  add_subdirectory(Examples)
endif()

# The doxygen documentation needs to be aware of all modules.
option(VTK_BUILD_DOCUMENTATION "Build the VTK documentation" OFF)
if(VTK_BUILD_DOCUMENTATION)
  add_subdirectory(Utilities/Doxygen)
endif()

# If python wrapping and testing is enabled then add driver scripts to run
# tests.  Note: Many pythong tests used to be automatically converted from TCL
# scripts. Hence the name vtkTclTest2Py
if(BUILD_TESTING AND VTK_WRAP_PYTHON)
  add_subdirectory(Utilities/vtkTclTest2Py)
endif()

# TODO: The VTKCompileTools package.
# TODO: HeaderTest exclusions for memcheck.

#-----------------------------------------------------------------------------
# Install the license file
install(FILES ${VTK_SOURCE_DIR}/Copyright.txt DESTINATION ${VTK_INSTALL_DOC_DIR})

#-----------------------------------------------------------------------------
# Do this at the end so the all variables it uses are setup.
include(vtkBuildPath)
