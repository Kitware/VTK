cmake_minimum_required(VERSION 3.12...3.21 FATAL_ERROR)

project(VTK)

if (POLICY CMP0127)
  cmake_policy(SET CMP0127 NEW) # cmake_dependent_option changes
endif ()

if (POLICY CMP0154)
  cmake_policy(SET CMP0154 NEW) # optimize custom commands in PRIVATE filesets
endif ()

set(vtk_cmake_dir "${VTK_SOURCE_DIR}/CMake")
list(INSERT CMAKE_MODULE_PATH 0 "${vtk_cmake_dir}")

if (APPLE)
  include(vtkApple)
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  include(vtkEmscripten)
  if (CMAKE_GENERATOR STREQUAL "Ninja" AND CMAKE_HOST_WIN32)
    # emcc.bat receives too many characters, so use response files instead.
    set(CMAKE_NINJA_FORCE_RESPONSE_FILE 1 CACHE INTERNAL "")
  endif ()
endif ()

# must be before the following iOS / Android
find_package(Git QUIET)
include(VTKDetermineVersion)
include(vtkVersion)
set(VTK_VERSION
    "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}")
determine_version("${CMAKE_CURRENT_SOURCE_DIR}" "${GIT_EXECUTABLE}" "VTK")

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

include(vtkCMakeBackports)

option(VTK_WHEEL_BUILD "Build in such a way that Python wheels are supported" OFF)
mark_as_advanced(VTK_WHEEL_BUILD)
if (VTK_WHEEL_BUILD)
  include(vtkWheelPreparation)
endif ()

include(CMakeDependentOption)
# (vtk/vtk#19315) Enable by default after WebGPU support is completed.
option(VTK_ENABLE_WEBGPU "Enable the WebGPU experimental rendering backend." OFF)

include(GNUInstallDirs)

add_library(vtkbuild INTERFACE)
add_library(VTK::vtkbuild ALIAS vtkbuild)

# Some flags are really bad for caching. Replace them with better variants.
if (vtk_replace_uncacheable_flags)
  get_property(is_multiconfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if (is_multiconfig)
    string(TOUPPER "${CMAKE_CONFIGURATION_TYPES}" build_types)
  else ()
    string(TOUPPER "${CMAKE_BUILD_TYPE}" build_types)
  endif ()
  foreach (lang IN ITEMS C CXX Fortran)
    foreach (prefix IN ITEMS - /)
      foreach (flag IN ITEMS Zi ZI)
        string(REPLACE "${prefix}${flag}" "-Z7" "CMAKE_${lang}_FLAGS" "${CMAKE_${lang}_FLAGS}")

        foreach (build_type IN LISTS build_types)
          string(REPLACE "${prefix}${flag}" "-Z7" "CMAKE_${lang}_FLAGS_${build_type}" "${CMAKE_${lang}_FLAGS_${build_type}}")
        endforeach ()
      endforeach ()
    endforeach ()
  endforeach ()
endif ()

include(vtkCompilerChecks)
include(vtkCompilerPlatformFlags)
include(vtkCompilerExtraFlags)
include(vtkCompilerWarningFlags)
include(vtkInitializeBuildType)
include(vtkSupportMacros)
include(vtkDownload)
include(vtkSanitize)
include(vtkTesting)
include(vtkMobileDevices)
include(vtkWrapSettings)
include(vtkCrossCompiling)
include(vtkObjectFactory)
include(vtkOpenGLOptions)

option(VTK_RELOCATABLE_INSTALL "Do not embed hard-coded paths into the install" ON)
mark_as_advanced(VTK_RELOCATABLE_INSTALL)
cmake_dependent_option(VTK_UNIFIED_INSTALL_TREE "Assume that the install tree contains all of VTK's dependencies" OFF
  VTK_RELOCATABLE_INSTALL OFF)
mark_as_advanced(VTK_UNIFIED_INSTALL_TREE)

set(CMAKE_INSTALL_LICENSEDIR ""
  CACHE STRING "License files (DATAROOTDIR/licenses/${CMAKE_PROJECT_NAME}")
mark_as_advanced(CMAKE_INSTALL_LICENSEDIR)
if (NOT CMAKE_INSTALL_LICENSEDIR)
  set(CMAKE_INSTALL_LICENSEDIR
    "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${CMAKE_PROJECT_NAME}")
endif ()

set(CMAKE_INSTALL_QMLDIR ""
  CACHE PATH "QML files (LIBDIR/qml)")
mark_as_advanced(CMAKE_INSTALL_QMLDIR)
if (NOT CMAKE_INSTALL_QMLDIR)
  set(CMAKE_INSTALL_QMLDIR
    "${CMAKE_INSTALL_LIBDIR}/qml")
endif ()

set(VTK_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(CMAKE_CXX_VISIBILITY_PRESET "hidden")
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

#-----------------------------------------------------------------------------
# Add the Remote Subdirectory
if (VTK_ENABLE_REMOTE_MODULES)
  add_subdirectory(Remote)
endif ()

#-----------------------------------------------------------------------------
# Does VTK require support for 64 bit file systems
include(CheckCXXSourceRuns)
file(READ "${vtk_cmake_dir}/vtkRequireLargeFilesSupport.cxx"
  VTK_REQUIRE_LARGE_FILE_SUPPORT_FILE)
check_cxx_source_runs("${VTK_REQUIRE_LARGE_FILE_SUPPORT_FILE}"
  VTK_REQUIRE_LARGE_FILE_SUPPORT)

#-----------------------------------------------------------------------------
# Provide compatibility options.
option(VTK_LEGACY_REMOVE "Remove all legacy code completely." OFF)
option(VTK_LEGACY_SILENT "Silence all legacy code messages." OFF)
mark_as_advanced(VTK_LEGACY_REMOVE VTK_LEGACY_SILENT)

include(CheckTypeSize)

option(VTK_USE_MPI "Support MPI" OFF)

option(VTK_SERIAL_TESTS_USE_MPIEXEC "Used on HPC to run serial tests on compute nodes" OFF)
mark_as_advanced(VTK_SERIAL_TESTS_USE_MPIEXEC)

#-----------------------------------------------------------------------------
# Add an option to enable memkind
if (UNIX AND NOT APPLE)
  option(VTK_USE_MEMKIND "Build support for extended memory" OFF)
endif()

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
# Add an option to enable/disable KOKKOS support.
cmake_dependent_option(VTK_USE_KOKKOS "Support KOKKOS backend accelerator" OFF
  "CMAKE_VERSION VERSION_GREATER_EQUAL 3.21;NOT VTK_USE_CUDA" OFF)
mark_as_advanced(VTK_USE_KOKKOS)
if(VTK_USE_KOKKOS)
  set(VTK_KOKKOS_BACKEND:STRING "SERIAL" CACHE STRING "'HIP', 'SYCL', or 'CUDA'")
endif()
# If the Kokkos backend is set, it should imply that Kokkos is to used
if(VTK_KOKKOS_BACKEND)
  set(VTK_USE_KOKKOS ON)
endif()

if(VTK_USE_HIP)
  message(WARNING "VTK_USE_HIP will be removed in future releases, please use VTK_USE_KOKKOS and VTK_KOKKOS_BACKEND")
  set(VTK_USE_KOKKOS ON)
  set(VTK_KOKKOS_BACKEND "HIP")
endif()

if(VTK_USE_KOKKOS)
  if(VTK_USE_CUDA)
    message(FATAL_ERROR "VTK_USE_KOKKOS and VTK_USE_CUDA are mutually exclusive, enable only one.")
  endif()
  # This needs to be a top level flag as enable_language needs
  # to be called from CMake entry point for variables to have
  # the correct scope.
  if (VTK_KOKKOS_BACKEND STREQUAL "HIP")
    enable_language(HIP)
  elseif (VTK_KOKKOS_BACKEND STREQUAL "CUDA")
    enable_language(CUDA)
  elseif (VTK_KOKKOS_BACKEND STREQUAL "SYCL")
  elseif (VTK_KOKKOS_BACKEND STREQUAL "SERIAL")
  else ()
    message(FATAL_ERROR "VTK_KOKKOS_BACKEND not recognized")
  endif()
endif()

#-----------------------------------------------------------------------------
# Add an option to enable/disable logging support.
option(VTK_ENABLE_LOGGING "Enable logging support." ON)
mark_as_advanced(VTK_ENABLE_LOGGING)

set(VTK_LOGGING_TIME_PRECISION "3"
  CACHE STRING "Precision of loguru scope timers. 3=ms, 6=us, 9=ns")
mark_as_advanced(VTK_LOGGING_TIME_PRECISION)
set(known_logging_precisions 3 6 9)
set_property(CACHE VTK_LOGGING_TIME_PRECISION
  PROPERTY
    STRINGS ${known_logging_precisions})
# Emulate `cmake_dependent_option` for non-bool values.
if (VTK_ENABLE_LOGGING)
  set_property(CACHE VTK_LOGGING_TIME_PRECISION PROPERTY TYPE STRING)
else ()
  set_property(CACHE VTK_LOGGING_TIME_PRECISION PROPERTY TYPE INTERNAL)
  # Use a default value.
  set(VTK_LOGGING_TIME_PRECISION 3)
endif ()
if (NOT VTK_LOGGING_TIME_PRECISION IN_LIST known_logging_precisions)
  string(REPLACE ";" ", " known_logging_precisions_list "${known_logging_precisions}")
  message(FATAL_ERROR
    "`VTK_LOGGING_TIME_PRECISION` must be one of "
    "${known_logging_precisions_list}; given '${VTK_LOGGING_TIME_PRECISION}'")
endif ()

#-----------------------------------------------------------------------------
# Add an option to enable/disable catalyst2 support.
option(VTK_ENABLE_CATALYST "Build catalyst-dependent modules, including the VTK catalyst implementation." OFF)
mark_as_advanced(VTK_ENABLE_CATALYST)

#-----------------------------------------------------------------------------
# Add an option to enable/disable SPDX generation
# Remove the "Experimental" once https://gitlab.kitware.com/vtk/vtk/-/issues/18999 is addressed
option(VTK_GENERATE_SPDX "Generate SPDX file for each module." OFF)
mark_as_advanced(VTK_GENERATE_SPDX)

include(vtkEncodeString)

# Set up our directory structure for output libraries and binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${VTK_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${VTK_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${VTK_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

set(vtk_requested_modules)
set(vtk_rejected_modules)
if (VTK_WRAP_PYTHON)
  list(APPEND vtk_requested_modules
    VTK::WrappingPythonCore)
  set("_vtk_module_reason_VTK::WrappingPythonCore"
    "via `VTK_WRAP_PYTHON`")
  if (NOT VTK_WHEEL_BUILD)
    list(APPEND vtk_requested_modules
      VTK::PythonInterpreter)
    set("_vtk_module_reason_VTK::PythonInterpreter"
      "via `VTK_WRAP_PYTHON AND NOT VTK_WHEEL_BUILD`")
  endif ()
else ()
  list(APPEND vtk_rejected_modules
    VTK::Python)
  set("_vtk_module_reason_VTK::Python"
    "via `VTK_WRAP_PYTHON`")
endif ()
if (VTK_WRAP_JAVA)
  list(APPEND vtk_requested_modules
    VTK::Java)
else ()
  list(APPEND vtk_rejected_modules
    VTK::Java)
endif ()
set("_vtk_module_reason_VTK::Java"
  "via `VTK_WRAP_JAVA`")
if (VTK_WRAP_SERIALIZATION)
  list(APPEND vtk_requested_modules
    VTK::SerializationManager)
  if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    list(APPEND vtk_requested_modules
      VTK::WebAssembly)
  else ()
    list(APPEND vtk_rejected_modules
      VTK::WebAssembly)
  endif ()
  set("_vtk_module_reason_VTK::WebAssembly"
  "via `VTK_WRAP_SERIALIZATION` AND `CMAKE_SYSTEM_NAME` STREQUAL Emscripten")
else ()
  list(APPEND vtk_rejected_modules
    VTK::SerializationManager)
endif ()
set("_vtk_module_reason_VTK::SerializationManager"
  "via `VTK_WRAP_SERIALIZATION`")
# For vtkTkRenderWidget (only useful with Python, since WRAP_TCL is gone)
if (VTK_USE_TK)
  if (VTK_GROUP_ENABLE_Tk STREQUAL "DEFAULT")
    set(VTK_GROUP_ENABLE_Tk "WANT")
  elseif (VTK_GROUP_ENABLE_Tk STREQUAL "NO")
    message(FATAL_ERROR "VTK_USE_TK=ON is not compatible with VTK_GROUP_ENABLE_Tk=NO")
  endif ()
endif ()
if (VTK_USE_MPI)
  if (VTK_GROUP_ENABLE_MPI STREQUAL "DEFAULT")
    set(VTK_GROUP_ENABLE_MPI "WANT")
  elseif (VTK_GROUP_ENABLE_MPI STREQUAL "NO")
    message(FATAL_ERROR "VTK_USE_MPI=ON is not compatible with VTK_GROUP_ENABLE_MPI=NO")
  endif ()
  list(APPEND vtk_requested_modules
    VTK::ParallelMPI)
  set("_vtk_module_reason_VTK::ParallelMPI"
    "via `VTK_USE_MPI`")
else ()
  # Reject `VTK::mpi`, but allow those which happen to be part of the group to
  # be built.
  set(_vtk_module_group_default_MPI "DONT_WANT")
  list(APPEND vtk_rejected_modules
    VTK::mpi)
  set("_vtk_module_reason_VTK::mpi"
    "via `VTK_USE_MPI`")
endif ()
if (VTK_ENABLE_LOGGING)
  list(APPEND vtk_requested_modules
    VTK::loguru)
else ()
  list(APPEND vtk_rejected_modules
    VTK::loguru)
endif ()
set("_vtk_module_reason_VTK::loguru"
  "via `VTK_ENABLE_LOGGING`")
if (VTK_ENABLE_CATALYST)
  list(APPEND vtk_requested_modules
    VTK::catalyst)
  # This is kept because it may be how users are enabling the module today.
  list(APPEND vtk_requested_modules
    VTK::IOCatalystConduit)
else ()
  list(APPEND vtk_rejected_modules
    VTK::catalyst)
endif ()
set("_vtk_module_reason_VTK::catalyst"
  "via `VTK_ENABLE_CATALYST`")
if (VTK_ENABLE_WEBGPU)
  list(APPEND vtk_requested_modules
    VTK::RenderingWebGPU)
else ()
  list(APPEND vtk_rejected_modules
    VTK::RenderingWebGPU)
endif ()
set("_vtk_module_reason_VTK::RenderingWebGPU"
  "via `VTK_ENABLE_WEBGPU`")

if (VTK_BUILD_TESTING)
  list(APPEND vtk_requested_modules
    # Required for just about all VTK tests.
    VTK::TestingCore)
  set("_vtk_module_reason_VTK::TestingCore"
    "via `VTK_BUILD_TESTING`")
endif ()

list(APPEND vtk_requested_modules
  # Required for just about everything.
  VTK::CommonCore
  VTK::CommonDataModel
)
set("_vtk_module_reason_VTK::CommonCore"
  "because all VTK builds require VTK::CommonCore")
set("_vtk_module_reason_VTK::CommonDataModel"
  "because all VTK builds require VTK::CommonDataModel")

if (VTK_ENABLE_WRAPPING)
  list(APPEND vtk_requested_modules
    # Required for wrapping with hierarchy files.
    VTK::WrappingTools)
  set("_vtk_module_reason_VTK::WrappingTools"
    "via `VTK_ENABLE_WRAPPING`")
endif ()

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
set(_vtk_module_reason_WANT_BY_DEFAULT
  "via `VTK_BUILD_ALL_MODULES`")

option(VTK_TARGET_SPECIFIC_COMPONENTS "Install files in target-specific components" OFF)
mark_as_advanced(VTK_TARGET_SPECIFIC_COMPONENTS)

# Turn on these groups by default.
set(_vtk_module_group_default_StandAlone "WANT")
set(_vtk_module_group_default_Rendering "WANT")

if (VTK_FORBID_DOWNLOADS AND VTK_BUILD_TESTING)
  message(STATUS
    "Module testing will fail due to `VTK_FORBID_DOWNLOADS` without a local "
    "copy of the testing data.")
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
  "${CMAKE_CURRENT_SOURCE_DIR}/Rendering"
  "${CMAKE_CURRENT_SOURCE_DIR}/Serialization"
  "${CMAKE_CURRENT_SOURCE_DIR}/Testing"
  "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty"
  "${CMAKE_CURRENT_SOURCE_DIR}/Utilities"
  "${CMAKE_CURRENT_SOURCE_DIR}/Views"
  "${CMAKE_CURRENT_SOURCE_DIR}/Web"
  "${CMAKE_CURRENT_SOURCE_DIR}/Wrapping")

set(remote_default ON)
if (VTK_WHEEL_BUILD)
  set(remote_default OFF)
endif ()
option(VTK_ENABLE_REMOTE_MODULES "Enable remote modules" ON)
mark_as_advanced(VTK_ENABLE_REMOTE_MODULES)
if (VTK_ENABLE_REMOTE_MODULES)
  list(APPEND vtk_source_directories
    "${CMAKE_CURRENT_SOURCE_DIR}/Remote")
endif ()

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
  ENABLE_TESTS        "${VTK_BUILD_TESTING}")

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

# Handle the requirement of `sqlitebin` for `proj`.
set(vtk_sqlite_build_binary 0)
if ("VTK::libproj" IN_LIST vtk_modules)
  set(vtk_sqlite_build_binary 1)
endif ()

cmake_dependent_option(VTK_ENABLE_KITS "Enable kits compilation" OFF
  # Static builds don't make sense with kits. Ignore the flag if shared
  # libraries aren't being built.
  "VTK_BUILD_SHARED_LIBS" OFF)
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
install(TARGETS vtkbuild EXPORT "${vtk_install_export}")

if (NOT DEFINED vtk_hierarchy_destination_args)
  set(vtk_hierarchy_destination_args
    HIERARCHY_DESTINATION
    "${CMAKE_INSTALL_LIBDIR}/vtk${vtk_version_suffix}/hierarchy/VTK")
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

set(vtk_library_version_info
  VERSION             "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}"
  SOVERSION           "1")
if (VTK_WHEEL_BUILD)
  set(vtk_library_version_info)
endif ()

vtk_module_build(
  MODULES             ${vtk_modules}
  KITS                ${vtk_kits}
  INSTALL_EXPORT      "${vtk_install_export}"
  ${vtk_target_package}
  HEADERS_DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/vtk${vtk_version_suffix}"
  ${vtk_hierarchy_destination_args}
  CMAKE_DESTINATION   "${vtk_cmake_destination}"
  LICENSE_DESTINATION "${CMAKE_INSTALL_LICENSEDIR}"
  LIBRARY_NAME_SUFFIX "${VTK_CUSTOM_LIBRARY_SUFFIX}"
  ${vtk_library_version_info}
  TEST_DATA_TARGET    VTKData
  INSTALL_HEADERS     "${VTK_INSTALL_SDK}"
  USE_FILE_SETS       ON
  BUILD_WITH_KITS     "${VTK_ENABLE_KITS}"
  USE_EXTERNAL        "${VTK_USE_EXTERNAL}"
  ENABLE_WRAPPING     "${VTK_ENABLE_WRAPPING}"
  UTILITY_TARGET      "VTK::vtkbuild"
  TARGET_SPECIFIC_COMPONENTS  "${VTK_TARGET_SPECIFIC_COMPONENTS}"
  TEST_INPUT_DATA_DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/Testing"
  TEST_OUTPUT_DATA_DIRECTORY  "${CMAKE_CURRENT_BINARY_DIR}/ExternalData/Testing"
  GENERATE_SPDX           ${VTK_GENERATE_SPDX}
  SPDX_DOCUMENT_NAMESPACE "https://vtk.org/spdx"
  SPDX_DOWNLOAD_LOCATION  "https://gitlab.kitware.com/vtk/vtk/-/tree/master"
  ENABLE_SERIALIZATION    "${VTK_WRAP_SERIALIZATION}")

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
  get_property(vtk_web_python_modules GLOBAL
    PROPERTY  vtk_web_python_modules)
  if (vtk_web_python_modules)
    list(REMOVE_DUPLICATES vtk_web_python_modules)
  endif ()
  string(REPLACE ";" "\n" vtk_web_python_modules "${vtk_web_python_modules}")
  file(WRITE "${CMAKE_BINARY_DIR}/requirements_web.txt"
    "${vtk_web_python_modules}\n")
  get_property(vtk_soabi GLOBAL
    PROPERTY  _vtk_python_soabi)
  cmake_dependent_option(VTK_WINDOWS_PYTHON_DEBUGGABLE "Append `_d` to Python module names" OFF
    "WIN32;VTK_WRAP_PYTHON" OFF)
  mark_as_advanced(VTK_WINDOWS_PYTHON_DEBUGGABLE)
  cmake_dependent_option(VTK_WINDOWS_PYTHON_DEBUGGABLE_REPLACE_SUFFIX "Replace any other debug suffix for Python module names" OFF
    "VTK_WINDOWS_PYTHON_DEBUGGABLE" OFF)
  mark_as_advanced(VTK_WINDOWS_PYTHON_DEBUGGABLE_REPLACE_SUFFIX)
  cmake_dependent_option(VTK_BUILD_PYI_FILES "Build `.pyi` files for VTK's Python modules" OFF
    "VTK_WRAP_PYTHON" OFF)
  mark_as_advanced(VTK_BUILD_PYI_FILES)

  if (APPLE AND VTK_WHEEL_BUILD)
    list(APPEND CMAKE_INSTALL_RPATH
      # Wheels place loaded libraries under a `.dylibs` subdirectory. Add this
      # to the rpath list.
      "@loader_path/.dylibs")
  endif ()

  set(vtk_python_interpreter_args)
  if (NOT VTK_WHEEL_BUILD)
    list(APPEND vtk_python_interpreter_args
      INTERPRETER "$<TARGET_FILE:VTK::vtkpython>")
  elseif (NOT vtk_python_interpreter_can_import_vtk)
    if (VTK_PYTHON_VERSION STREQUAL "3" AND
        Python3_VERSION VERSION_LESS "3.8" AND
        WIN32)
      if ("$ENV{CI}" STREQUAL "")
        message(AUTHOR_WARNING
          "Disabling `.pyi` compilation as building it in the wheel does not "
          "work. Please update to Python 3.8 for `.pyi` support in Windows "
          "wheels. If the interpreter is known to be able to import "
          "`vtkmodules`, " "please set "
          "`vtk_python_interpreter_can_import_vtk` to `ON`.")
      endif ()
      list(APPEND vtk_python_interpreter_args
        INTERPRETER "DISABLE")
    endif ()
  endif ()

  vtk_module_wrap_python(
    MODULES         ${vtk_modules}
    INSTALL_EXPORT  VTKPython
    PYTHON_PACKAGE  "vtkmodules"
    LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    MODULE_DESTINATION  "${VTK_PYTHON_SITE_PACKAGES_SUFFIX}"
    CMAKE_DESTINATION   "${vtk_cmake_destination}"
    INSTALL_HEADERS     "${VTK_INSTALL_SDK}"
    HEADERS_DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/vtk${vtk_version_suffix}/vtkpythonmodules"
    BUILD_PYI_FILES     "${VTK_BUILD_PYI_FILES}"
    SOABI               "${vtk_soabi}"
    USE_DEBUG_SUFFIX    "${VTK_WINDOWS_PYTHON_DEBUGGABLE}"
    REPLACE_DEBUG_SUFFIX "${VTK_WINDOWS_PYTHON_DEBUGGABLE_REPLACE_SUFFIX}"
    UTILITY_TARGET      "VTK::vtkbuild"
    WRAPPED_MODULES vtk_python_wrapped_modules
    TARGET_SPECIFIC_COMPONENTS "${VTK_TARGET_SPECIFIC_COMPONENTS}"
    TARGET          VTK::vtkpythonmodules)

  if (APPLE AND VTK_WHEEL_BUILD)
    list(REMOVE_ITEM CMAKE_INSTALL_RPATH
      "@loader_path/.dylibs")
  endif ()

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
  enable_language(Java)

  add_subdirectory(Wrapping/Java)
endif ()

if (VTK_BUILD_TESTING)
  # Create target to download data from the VTKData group.  This must come after
  # all tests have been added that reference the group, so we put it last.
  ExternalData_Add_Target(VTKData)

  if (VTK_DATA_EXCLUDE_FROM_ALL)
    set_property(TARGET VTKData PROPERTY EXCLUDE_FROM_ALL 1)
    if (NOT VTK_DATA_EXCLUDE_FROM_ALL_NO_WARNING)
      message(WARNING
        "VTK_DATA_EXCLUDE_FROM_ALL is ON so test data (needed because "
        "VTK_BUILD_TESTING is ON) may not be available without manually "
        "building the 'VTKData' target.")
    endif ()
  endif ()

  if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    # EMSDK makes python available after the environment is initialized with emsdk_env.{sh, csh, bat, ps1}
    find_package(Python3 COMPONENTS Interpreter)
  endif ()
endif ()

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

if (VTK_BUILD_TESTING)
  add_subdirectory(Testing/Install)
endif ()

# The doxygen documentation needs to be aware of all modules.
option(VTK_BUILD_DOCUMENTATION "Build the Doxygen VTK documentation" OFF)
if (VTK_BUILD_DOCUMENTATION)
  add_subdirectory(Utilities/Doxygen)
endif ()

option(VTK_BUILD_SPHINX_DOCUMENTATION "Build the Sphinx documentation for VTK" OFF)
if (VTK_BUILD_SPHINX_DOCUMENTATION)
  add_subdirectory(Utilities/Sphinx)
endif ()

# Adds a test to verify consistent use of VTK_MARSHAL(AUTO|MANUAL) macros.
# This is a pure python test, it doesn't need any compiled VTK modules.
if (VTK_BUILD_TESTING)
  add_subdirectory(Utilities/Marshalling)
endif ()

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
  DESTINATION "${CMAKE_INSTALL_LICENSEDIR}"
  COMPONENT   "license")

# TODO: HeaderTest exclusions for memcheck.

if (VTK_WHEEL_BUILD)
  include(vtkWheelFinalization)
endif ()

# Some generated code has some warnings that we need to exclude for now.
configure_file(
  "${VTK_SOURCE_DIR}/.clang-tidy"
  "${VTK_BINARY_DIR}/.clang-tidy"
  COPYONLY)

#-----------------------------------------------------------------------------
# Do this at the end so the all variables it uses are setup.
include(vtkBuildPath)
