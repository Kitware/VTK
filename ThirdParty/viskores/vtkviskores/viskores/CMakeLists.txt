##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project (Viskores)

# We only allow c++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# When using C++14 support make sure you use the standard C++ extensions rather
# than compiler-specific versions of the extensions (to preserve portability).
set(CMAKE_CXX_EXTENSIONS OFF)

# Update module path
set(Viskores_CMAKE_MODULE_PATH ${Viskores_SOURCE_DIR}/CMake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${Viskores_CMAKE_MODULE_PATH})

# While disabled system-wide, Viskores uses UNITY builds in some modules
set(CMAKE_UNITY_BUILD OFF)

# By default effectively disable unity builds
if (NOT DEFINED CMAKE_UNITY_BUILD_BATCH_SIZE)
  set(CMAKE_UNITY_BUILD_BATCH_SIZE 1)
endif()

# Determine Viskores version
include(Utilities/Git/Git.cmake)
include(ViskoresDetermineVersion)

# Load hardcoded version in case this is not a Git repository
file(STRINGS version.txt version_txt)
extract_version_components("${version_txt}" "Viskores")
# Get the version from git if we can
determine_version(${Viskores_SOURCE_DIR} ${GIT_EXECUTABLE} "Viskores")

if (NOT DEFINED Viskores_INSTALL_INCLUDE_DIR)
  set(Viskores_INSTALL_INCLUDE_DIR "include/viskores-${Viskores_VERSION_MAJOR}.${Viskores_VERSION_MINOR}")
endif()
if (NOT DEFINED Viskores_INSTALL_CONFIG_DIR)
  set(Viskores_INSTALL_CONFIG_DIR "lib/cmake/viskores-${Viskores_VERSION_MAJOR}.${Viskores_VERSION_MINOR}")
endif()
if (NOT DEFINED Viskores_INSTALL_LIB_DIR)
  set(Viskores_INSTALL_LIB_DIR "lib")
endif()
if (NOT DEFINED Viskores_INSTALL_BIN_DIR)
  set(Viskores_INSTALL_BIN_DIR "bin")
endif()
if (NOT DEFINED Viskores_INSTALL_SHARE_DIR)
  set(Viskores_INSTALL_SHARE_DIR "share/viskores-${Viskores_VERSION_MAJOR}.${Viskores_VERSION_MINOR}")
endif()
if (NOT DEFINED Viskores_INSTALL_CMAKE_MODULE_DIR)
  set(Viskores_INSTALL_CMAKE_MODULE_DIR "${Viskores_INSTALL_SHARE_DIR}/cmake")
endif()
if (NOT DEFINED Viskores_BUILD_CMAKE_BASE_DIR)
  set(Viskores_BUILD_CMAKE_BASE_DIR "${Viskores_BINARY_DIR}")
endif()
if(NOT DEFINED Viskores_EXECUTABLE_OUTPUT_PATH)
  ## Set the directory where the binaries will be stored
  set(Viskores_EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
endif()
if(NOT DEFINED Viskores_LIBRARY_OUTPUT_PATH)
  ## Set the directory where the libraries will be stored
  set(Viskores_LIBRARY_OUTPUT_PATH  ${PROJECT_BINARY_DIR}/lib)
endif()
if (NOT DEFINED Viskores_EXPORT_NAME)
  set(Viskores_EXPORT_NAME "ViskoresTargets")
endif()

set(Viskores_BINARY_INCLUDE_DIR "${Viskores_BINARY_DIR}/include")

#-----------------------------------------------------------------------------
# viskores_option(variable doc [initial])
#   Provides an option if it is not already defined.
# This can be replaced when CMake 3.13 is our cmake_minimum_required
macro (viskores_option variable)
  if (NOT DEFINED "${variable}")
    option("${variable}" ${ARGN})
  endif ()
endmacro ()

# Configurable Options
viskores_option(Viskores_ENABLE_CUDA "Enable Cuda support" OFF)
viskores_option(Viskores_ENABLE_KOKKOS "Enable Kokkos support" OFF)
viskores_option(Viskores_ENABLE_OPENMP "Enable OpenMP support" OFF)
viskores_option(Viskores_ENABLE_TBB "Enable TBB support" OFF)
viskores_option(Viskores_ENABLE_RENDERING "Enable rendering library" ON)
viskores_option(Viskores_ENABLE_BENCHMARKS "Enable Viskores Benchmarking" OFF)
viskores_option(Viskores_ENABLE_MPI "Enable MPI support" OFF)
viskores_option(Viskores_ENABLE_DOCUMENTATION "Build Doxygen documentation" OFF)
viskores_option(Viskores_ENABLE_EXAMPLES "Build examples" OFF)
viskores_option(Viskores_ENABLE_TUTORIALS "Build tutorials" OFF)
if (NOT DEFINED Viskores_ENABLE_TESTING)
    if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
      viskores_option(Viskores_ENABLE_TESTING "Enable Viskores Testing" ON)
    else()
      viskores_option(Viskores_ENABLE_TESTING "Enable Viskores Testing" OFF)
    endif()
endif()

# By default: (Viskores_ENABLE_TESTING OR Viskores_ENABLE_BENCHMARKS) -> Viskores_ENABLE_TESTING_LIBRARY
include(CMakeDependentOption)
cmake_dependent_option(Viskores_ENABLE_TESTING_LIBRARY "Enable Viskores Testing Library"
  OFF "NOT Viskores_ENABLE_TESTING;NOT Viskores_ENABLE_BENCHMARKS" ON)
mark_as_advanced(Viskores_ENABLE_TESTING_LIBRARY)

# The ANARI interop library uses a bit of code in viskores_rendering, so this option
# currently requires viskores_rendering to be built. Eventually this dependency
# should go away as viskores_anari doesn't require applications to use anything from
# viskores_rendering directly.
cmake_dependent_option(Viskores_ENABLE_ANARI "Enable ANARI interop support"
  OFF "Viskores_ENABLE_RENDERING" OFF)

# If you are building Doxygen, you also have the option to build the User's Guide with Sphinx
cmake_dependent_option(Viskores_ENABLE_USERS_GUIDE
  "Build User's Guide with Sphinx"
  ON
  "Viskores_ENABLE_DOCUMENTATION"
  OFF
  )
# If you are building the User's Guide, you may only need the XML output from Doxygen, which
# is much faster than generating the HTML. Thus, give the option to turn on/off the HTML output.
cmake_dependent_option(Viskores_Doxygen_HTML_output
  "Build HTML output"
  ON
  "Viskores_ENABLE_USERS_GUIDE"
  ON
  )
mark_as_advanced(Viskores_Doxygen_HTML_output)

# We may want to make finer controls on whether libraries/modules get built.
# VTK uses the concept of groups for its modules
viskores_option(Viskores_BUILD_ALL_LIBRARIES
  "Build all libraries by default. (Can be overridden for each library.)"
  ON
  )
mark_as_advanced(Viskores_BUILD_ALL_LIBRARIES)

viskores_option(Viskores_USE_DOUBLE_PRECISION "Use double precision for floating point calculations" OFF)
viskores_option(Viskores_USE_64BIT_IDS "Use 64-bit indices." ON)

viskores_option(Viskores_ENABLE_HDF5_IO "Enable HDF5 support" OFF)
if (Viskores_ENABLE_HDF5_IO)
  find_package(HDF5 REQUIRED COMPONENTS HL)
endif()

# If the linked HDF5 is parallel, we need to add the MPI include directory
# for its dependencies
viskores_option(Viskores_HDF5_IS_PARALLEL "HDF5 library is parallel" OFF)
mark_as_advanced(Viskores_HDF5_IS_PARALLEL)

# Viskores will turn on logging by default, but will set the default
# logging level to WARN.  This option should not be visible by default
# in the GUI, as ERROR and WARN level logging should not interfere
# with the performance of viskores
viskores_option(Viskores_ENABLE_LOGGING "Enable Viskores Logging" ON)

# When Viskores is embedded into larger projects they may desire to turn off
# Viskores internal assert checks when in debug mode to improve debug runtime
# performance.
viskores_option(Viskores_NO_ASSERT "Disable assertions in debugging builds." OFF)

# The CUDA compiler (as of CUDA 11) takes a surprising long time to compile
# kernels with assert in them. By default we turn off asserts when compiling
# for CUDA devices.
viskores_option(Viskores_NO_ASSERT_CUDA "Disable assertions for CUDA devices." ON)

# The HIP compiler (as of ROCm 3.7) takes a surprising long time to compile
# kernels with assert in them they generate `printf` calls which are very
# slow ( cause massive register spillage). By default we turn off asserts when
# compiling for HIP devices.
viskores_option(Viskores_NO_ASSERT_HIP "Disable assertions for HIP devices." ON)

# When Viskores is embedded into larger projects that wish to make end user
# applications they want to only install libraries and don't want CMake/headers
# installed.
viskores_option(Viskores_INSTALL_ONLY_LIBRARIES "install only viskores libraries and no headers" OFF)

# Install examples projects
cmake_dependent_option(Viskores_INSTALL_EXAMPLES "Install examples" OFF "NOT Viskores_ENABLE_EXAMPLES" ON)

# Viskores is setup by default not to export symbols unless explicitly stated.
# We prefer to only export symbols of a small set of user facing classes,
# rather than exporting all symbols. This flag is added so that consumers
# which require static builds can force all symbols on, which is something
# VTK does.
viskores_option(Viskores_HIDE_PRIVATE_SYMBOLS "Hide symbols from libraries." ON)

viskores_option(BUILD_SHARED_LIBS "Build Viskores with shared libraries" ON)
set(Viskores_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

# This flag can be used to prevent Viskores from exporting its warning flags in its
# build interface. This is useful when building Viskores as a thirdparty library
# and the warnings are too strict for the parent project.
viskores_option(Viskores_ENABLE_DEVELOPER_FLAGS "Enable compiler flags that are useful while developing Viskores" ON)

# By default Viskores would install its README.md and LICENSE.md.
# Some application might need not to install those, hence this option.
viskores_option(Viskores_NO_INSTALL_README_LICENSE "disable the installation of README and LICENSE files" OFF)

# In Python wheels, having SONAME suffixes just ends up duplicating files.
# Allow VTK to turn off these symlinks for its wheel distribution.
viskores_option(Viskores_SKIP_LIBRARY_VERSIONS "Skip versioning Viskores libraries" OFF)

# During development, running unit tests with the default values can be too lengthy.
# Allow for the developer to skip the majority of the default values and control them
# through ctest's command-line. Doesn't affect CI unless enabled.
viskores_option(Viskores_OVERRIDE_CTEST_TIMEOUT "Disable default ctest timeout" OFF)

# Viskores_ENABLE_GPU_MPI makes Viskores to use DIY routines that enables GPU aware
# MPI. By default, this option is disabled. Also, this option is hidden unless
# Viskores_ENABLE_MPI=ON.
cmake_dependent_option(Viskores_ENABLE_GPU_MPI "Enable GPU AWARE MPI support" OFF "Viskores_ENABLE_MPI" OFF)

mark_as_advanced(
  Viskores_ENABLE_LOGGING
  Viskores_NO_ASSERT
  Viskores_NO_ASSERT_CUDA
  Viskores_NO_ASSERT_HIP
  Viskores_INSTALL_ONLY_LIBRARIES
  Viskores_HIDE_PRIVATE_SYMBOLS
  Viskores_ENABLE_DEVELOPER_FLAGS
  Viskores_NO_INSTALL_README_LICENSE
  Viskores_SKIP_LIBRARY_VERSIONS
  Viskores_OVERRIDE_CTEST_TIMEOUT
  )

#-----------------------------------------------------------------------------
# Force building of modules where specified by user-facing options.
include(ViskoresModules)
viskores_module_force_group(Core VALUE "YES" REASON "Core modules always built")
viskores_module_force_group(Rendering ENABLE_OPTION Viskores_ENABLE_RENDERING)
viskores_module_force_group(Logging ENABLE_OPTION Viskores_ENABLE_LOGGING)
viskores_module_force_group(Testing
  ENABLE_OPTION Viskores_ENABLE_TESTING_LIBRARY
  ENABLE_VALUE "WANT"
  DISABLE_VALUE "DONT_WANT"
  )
viskores_module_force_group(Benchmarking ENABLE_OPTION Viskores_ENABLE_BENCHMARKS)
viskores_module_force_group(ANARI ENABLE_OPTION Viskores_ENABLE_ANARI)

# The tutorial requires several common filters. This logic might need to
# become more complicated (or less compliated if we decide to always
# compile these).
if(Viskores_ENABLE_TUTORIALS)
  viskores_module_force_group(FiltersCommon VALUE "YES" REASON "Tutorial needs common filters.")
endif()

#-----------------------------------------------------------------------------

# Setup default build types
include(ViskoresBuildType)

# Include the viskores wrappers
include(ViskoresWrappers)

# By default: Set Viskores_ENABLE_KOKKOS_THRUST to ON if Viskores_ENABLE_KOKKOS is ON, otherwise
# disable it (or if the user explicitly turns this option OFF)
cmake_dependent_option(
  Viskores_ENABLE_KOKKOS_THRUST
  "Enable Kokkos thrust support (only valid with CUDA and HIP)"
  ON
  "Viskores_ENABLE_KOKKOS;Kokkos_ENABLE_CUDA OR Kokkos_ENABLE_HIP; NOT Kokkos_ENABLE_HIP AND CMAKE_VERSION VERSION_LESS 3.24"
  OFF
)

# CUDA already provides thrust
if (Viskores_ENABLE_KOKKOS_THRUST AND TARGET viskores_kokkos_hip)
  if (CMAKE_VERSION VERSION_LESS 3.24)
    message(FATAL_ERROR "Viskores_ENABLE_KOKKOS_THRUST=ON with HIP needs CMAKE >= 3.24")
  endif()
  # This policy is needed for LINK_ONLY to work in LINK_LIBRARIES.
  cmake_policy(SET CMP0131 NEW)
  find_package(rocthrust REQUIRED CONFIG)
endif()

# Create viskores_compiler_flags library. This is an interface library that
# holds all the C++ compiler flags that are needed for consumers and
# when building Viskores.
include(ViskoresCompilerFlags)

if (VISKORES_EXAMPLE_CONTOURTREE_ENABLE_DEBUG_PRINT)
  add_compile_definitions(DEBUG_PRINT)
endif()
#-----------------------------------------------------------------------------
# We need to check and see if the git submodule has been correctly cloned.
if (Viskores_ENABLE_TESTING)
  file(STRINGS "${Viskores_SOURCE_DIR}/data/data/sentinel-data" sentinel_data LIMIT_COUNT 1)
  if (NOT sentinel_data STREQUAL "-- DO NOT MODIFY THIS LINE --")
    message(WARNING
      "Testing is enabled, but the data is not available. Use git submodule in order "
      "to obtain the testing data.")
    set(Viskores_ENABLE_TESTING off)
  endif()
endif()

# We include the wrappers unconditionally as Viskores expects the function to
# always exist (and early terminate when testing is disabled).
include(testing/ViskoresTestWrappers)
if (Viskores_ENABLE_TESTING)
  enable_testing()
  # Only include CTest if it has not been included by a superproject. The
  # variable DEFAULT_CTEST_CONFIGURATION_TYPE is a non-cached variable set by
  # CTest.cmake, so we'll use that to determine if it's already included.
  if(NOT DEFINED DEFAULT_CTEST_CONFIGURATION_TYPE)
    include(CTest)
    # Mark this as advanced to avoid confusion, since we actually rely on
    # Viskores_ENABLE_TESTING.
    mark_as_advanced(BUILD_TESTING)
  endif()


  configure_file(CTestCustom.cmake.in
    ${Viskores_BINARY_DIR}/CTestCustom.cmake @ONLY)

  #-----------------------------------------------------------------------------
  # Find the Python interpreter, which we will use during the build process
  find_package(Python QUIET COMPONENTS Interpreter)

  #-----------------------------------------------------------------------------
  # Find Pyexpander in case somebody wants to update the auto generated
  # faux variadic template code
  find_package(Pyexpander QUIET)

  # Setup compiler flags for dynamic analysis if needed
  include(testing/ViskoresCompilerDynamicAnalysisFlags)

endif()

#-----------------------------------------------------------------------------
# Check basic type sizes.
include(CheckTypeSize)

check_type_size(long Viskores_SIZE_LONG BUILTIN_TYPES_ONLY)
check_type_size("long long" Viskores_SIZE_LONG_LONG BUILTIN_TYPES_ONLY)

#-----------------------------------------------------------------------------
# Ready files for find_package
include(CMakePackageConfigHelpers)

configure_package_config_file(
  ${Viskores_SOURCE_DIR}/CMake/ViskoresConfig.cmake.in
  ${Viskores_BUILD_CMAKE_BASE_DIR}/${Viskores_INSTALL_CONFIG_DIR}/ViskoresConfig.cmake
  INSTALL_DESTINATION ${Viskores_INSTALL_CONFIG_DIR}
  PATH_VARS
    Viskores_INSTALL_INCLUDE_DIR
    Viskores_INSTALL_CONFIG_DIR
    Viskores_INSTALL_LIB_DIR
    Viskores_INSTALL_BIN_DIR
    Viskores_INSTALL_CMAKE_MODULE_DIR
  )

write_basic_package_version_file(
  ${Viskores_BUILD_CMAKE_BASE_DIR}/${Viskores_INSTALL_CONFIG_DIR}/ViskoresConfigVersion.cmake
  VERSION ${Viskores_VERSION}
  COMPATIBILITY SameMajorVersion )

#-----------------------------------------------------------------------------
# Add subdirectories
add_subdirectory(viskoresstd)

#-----------------------------------------------------------------------------
# Process modules
viskores_modules_scan(
  SCAN_DIRECTORIES viskores benchmarking docs
  PROVIDED_MODULES all_modules
  )
viskores_modules_build(
  PROVIDED_MODULES ${all_modules}
  WANT_BY_DEFAULT ${Viskores_BUILD_ALL_LIBRARIES}
  WANT_BY_DEFAULT_REASON "Viskores_BUILD_ALL_LIBRARIES is `${Viskores_BUILD_ALL_LIBRARIES}`"
  )

#-----------------------------------------------------------------------------
# Build documentation
if (Viskores_ENABLE_DOCUMENTATION)
  include(ViskoresBuildDocumentation)
endif()

#-----------------------------------------------------------------------------
# Create makefile/package files for projects not using CMake

set(lib_args)
viskores_module_get_list(module_list)
list(REVERSE module_list)
foreach(module IN LISTS module_list)
  get_target_property(type ${module} TYPE)
  if (NOT type MATCHES "LIBRARY" OR type STREQUAL "INTERFACE_LIBRARY")
    continue()
  endif()
  get_target_property(library ${module} OUTPUT_NAME)
  if (NOT library)
    continue()
  endif()
  set(lib_args "${lib_args} \\
    -l${library}")
endforeach()
if (TARGET viskoresdiympi)
  set(lib_args "${lib_args} \\
    -lviskoresdiympi")
endif()
if (TARGET viskoresdiympi_nompi)
  set(lib_args "${lib_args} \\
    -lviskoresdiympi_nompi")
endif()

configure_file(${Viskores_SOURCE_DIR}/config/viskores_config.mk.in
  ${Viskores_BINARY_DIR}/config/viskores_config.mk @ONLY)
install(FILES ${Viskores_BINARY_DIR}/config/viskores_config.mk
  DESTINATION ${Viskores_INSTALL_SHARE_DIR}
  )
configure_file(${Viskores_SOURCE_DIR}/config/viskores.pc.in
  ${Viskores_BINARY_DIR}/config/viskores.pc @ONLY)
install(FILES ${Viskores_BINARY_DIR}/config/viskores.pc
  DESTINATION ${Viskores_INSTALL_SHARE_DIR}
  )

include(ViskoresInstallCMakePackage)

# Install the readme and license files.
if (NOT Viskores_NO_INSTALL_README_LICENSE)
install(FILES ${Viskores_SOURCE_DIR}/README.md
  DESTINATION ${Viskores_INSTALL_SHARE_DIR}
  RENAME ViskoresREADME.md
  )
install(FILES ${Viskores_SOURCE_DIR}/LICENSE.txt
  DESTINATION ${Viskores_INSTALL_SHARE_DIR}
  RENAME ViskoresLICENSE.txt
  )
endif()

if(NOT Viskores_INSTALL_ONLY_LIBRARIES)
  install(
    FILES
      ${Viskores_BUILD_CMAKE_BASE_DIR}/${Viskores_INSTALL_CONFIG_DIR}/ViskoresConfig.cmake
      ${Viskores_BUILD_CMAKE_BASE_DIR}/${Viskores_INSTALL_CONFIG_DIR}/ViskoresConfigVersion.cmake
    DESTINATION ${Viskores_INSTALL_CONFIG_DIR}
    )

  # Install helper configure files.
  # XXX(kitware): removed by VTK

  # XXX(kitware): removed by VTK


  # Install support files.
  install(
    FILES
      ${Viskores_SOURCE_DIR}/CMake/ViskoresCPUVectorization.cmake
      ${Viskores_SOURCE_DIR}/CMake/ViskoresDetectCUDAVersion.cu
      ${Viskores_SOURCE_DIR}/CMake/ViskoresDeviceAdapters.cmake
      ${Viskores_SOURCE_DIR}/CMake/ViskoresDIYUtils.cmake
      ${Viskores_SOURCE_DIR}/CMake/ViskoresExportHeaderTemplate.h.in
      ${Viskores_SOURCE_DIR}/CMake/ViskoresRenderingContexts.cmake
      ${Viskores_SOURCE_DIR}/CMake/ViskoresWrappers.cmake
    DESTINATION ${Viskores_INSTALL_CMAKE_MODULE_DIR}
    )

  # Create and install exports for external projects
  export(EXPORT ${Viskores_EXPORT_NAME}
    NAMESPACE viskores::
    FILE ${Viskores_BUILD_CMAKE_BASE_DIR}/${Viskores_INSTALL_CONFIG_DIR}/ViskoresTargets.cmake
    )
  install(EXPORT ${Viskores_EXPORT_NAME}
    NAMESPACE viskores::
    DESTINATION ${Viskores_INSTALL_CONFIG_DIR}
    FILE ViskoresTargets.cmake
    )
endif()

viskores_option(Viskores_ENABLE_CPACK "Enable CPack packaging of Viskores" ON)
if (Viskores_ENABLE_CPACK)
  # Enable CPack packaging
  set(CPACK_PACKAGE_DESCRIPTION_FILE ${Viskores_SOURCE_DIR}/README.md)
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The Viskores Toolkit")
  set(CPACK_PACKAGE_NAME "Viskores")
  set(CPACK_PACKAGE_VERSION_MAJOR ${Viskores_VERSION_MAJOR})
  set(CPACK_PACKAGE_VERSION_MINOR ${Viskores_VERSION_MINOR})
  set(CPACK_PACKAGE_VERSION_PATCH ${Viskores_VERSION_PATCH})
  set(CPACK_PACKAGE_FILE_NAME "Viskores-${Viskores_VERSION}")
  set(CPACK_RESOURCE_FILE_LICENSE ${Viskores_SOURCE_DIR}/LICENSE.txt)
  set(CPACK_RESOURCE_FILE_README ${Viskores_SOURCE_DIR}/README.md)
  include(CPack)
endif ()

#-----------------------------------------------------------------------------
if (Viskores_ENABLE_TESTING)

  #-----------------------------------------------------------------------------
  # Add "meta" tests that check the state of the repository
  # SystemInformation prints out information about the current configuration
  # LicenseStatement checks that the copyright statement is in all source files
  # SourceInBuild checks that all source files are listed in the build
  # SourceInInstall checks that all source files are installed in the build
  add_test(NAME SystemInformation
    COMMAND ${CMAKE_COMMAND} "-DViskores_SOURCE_DIR=${Viskores_SOURCE_DIR}" "-DViskores_BINARY_DIR=${Viskores_BINARY_DIR}" -P "${Viskores_SOURCE_DIR}/CMake/testing/ViskoresSystemInformation.cmake"
    )
  if(NOT WIN32)
    # Test takes too long on windows
    add_test(NAME LicenseStatement
      COMMAND ${CMAKE_COMMAND} "-DViskores_SOURCE_DIR=${Viskores_SOURCE_DIR}" -P "${Viskores_SOURCE_DIR}/CMake/ViskoresCheckLicense.cmake"
      )
    # increase timeout since on some machines LicenseStatement test takes a long time.
    set_tests_properties(LicenseStatement PROPERTIES TIMEOUT 300)
  endif()

  # Setup the infrastructure to allow Viskores to run tests against a temporary
  # installed version of Viskores.
  include(testing/ViskoresTestInstall)
  viskores_test_install()
else ()
  set(CTEST_USE_LAUNCHERS off)
endif()

#-----------------------------------------------------------------------------
# Build examples
add_subdirectory(examples)

if (Viskores_INSTALL_EXAMPLES)
  include(GNUInstallDirs)
  install(DIRECTORY examples DESTINATION ${CMAKE_INSTALL_DOCDIR} REGEX examples/CMakeLists.txt EXCLUDE)
endif()

#-----------------------------------------------------------------------------
# Tutorial examples
if(Viskores_ENABLE_TUTORIALS)
  add_subdirectory(tutorial)
endif()
