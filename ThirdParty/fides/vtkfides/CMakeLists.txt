if (FALSE) # XXX(kitware): VTK handles CMake minimum versions
cmake_minimum_required(VERSION 3.9)
endif ()
project(FIDES LANGUAGES C CXX VERSION 0.1)

list(INSERT CMAKE_MODULE_PATH 0 "${PROJECT_SOURCE_DIR}/cmake")

if (FALSE) # XXX(kitware): not needed for VTK
# Determine Fides version
include(Utilities/Git/Git.cmake)
include(FidesDetermineVersion)

# Load hardcoded version in case this is not a Git repository
file(STRINGS version.txt version_txt)
extract_version_components("${version_txt}" "Fides")
# Get the version from git if we can
determine_version(${PROJECT_SOURCE_DIR} ${GIT_EXECUTABLE} "Fides")

# Add options for performing sanitization
include(cmake/FidesSanitize.cmake)

# Add options for performing code coverage tests
include(cmake/FidesCoverage.cmake)
endif ()

include(FidesModule)

if (FALSE) # XXX(kitware): VTK handles VTK-m
find_package(ADIOS2 REQUIRED)

if (ADIOS2_VERSION VERSION_GREATER_EQUAL "2.8.0")
  set(ADIOS_DEFS FIDES_ADIOS_HAS_RANDOM_ACCESS)
endif()

# need to continue to support v1.9 for awhile
find_package(VTKm REQUIRED)
else ()
vtk_module_find_package(PACKAGE ADIOS2)
if (ADIOS2_VERSION VERSION_GREATER_EQUAL "2.8.0")
  set(ADIOS_DEFS FIDES_ADIOS_HAS_RANDOM_ACCESS)
endif()
endif ()

if (FALSE) # XXX(kitware): VTK handles building shared libs/testing and MPI
if (VTKm_ENABLE_CUDA)
  enable_language(CUDA)
endif()

option(BUILD_SHARED_LIBS "Build fides with shared libraries" ON)

option(FIDES_ENABLE_TESTING "Enable Fides Testing" ON)
option(FIDES_ENABLE_EXAMPLES "Build Fides Examples" ON)

if(ADIOS2_HAVE_MPI)
  set(FIDES_USE_MPI TRUE)
  set(MPI_DEFS FIDES_USE_MPI)
  set(MPI_LIBS MPI::MPI_CXX)
endif()

else ()
set(FIDES_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(FIDES_ENABLE_TESTING OFF)
set(FIDES_ENABLE_EXAMPLES OFF)

if (TARGET VTK::mpi)
  set(FIDES_USE_MPI TRUE)
  set(MPI_DEFS FIDES_USE_MPI)
  set(MPI_LIBS VTK::mpi)
endif ()
endif ()

if (FALSE) # XXX(kitware): VTK handles building shared libs/testing and MPI
option(FIDES_USE_EXTERNAL_RAPIDJSON "Use external RapidJSON" OFF)
mark_as_advanced(FIDES_USE_EXTERNAL_RAPIDJSON)
if (FIDES_USE_EXTERNAL_RAPIDJSON)
  find_package(RapidJSON REQUIRED)
endif()
else ()
set(FIDES_USE_EXTERNAL_RAPIDJSON OFF)
endif ()

if (NOT DEFINED FIDES_INSTALL_INCLUDE_DIR)
  set(FIDES_INSTALL_INCLUDE_DIR "include/fides")
endif()

if (NOT DEFINED FIDES_EXPORT_NAME)
  set(FIDES_EXPORT_NAME "FidesTargets")
endif()

add_subdirectory(fides)

# add third party
add_subdirectory(thirdparty/rapidjson)

if (FIDES_ENABLE_TESTING)
  enable_testing()
  include(CTest)
  add_subdirectory(tests)
endif()

if (FIDES_ENABLE_SANITIZER)
  set(preload_libraries)
  if (FIDES_SANITIZER MATCHES "address")
    find_library(FIDES_ASAN_LIBRARY NAMES libasan.so.5 DOC "ASan library")
    mark_as_advanced(FIDES_ASAN_LIBRARY)

    list(APPEND preload_libraries
      "${FIDES_ASAN_LIBRARY}")
  endif ()
endif()

set(FIDES_COVERAGE_ENABLED OFF)
if (FIDES_ENABLE_COVERAGE)
  set(FIDES_COVERAGE_ENABLED ON)
endif()

if (FALSE) # XXX(kitware): not needed for VTK
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CTestCustom.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake"
  )
endif ()

if (FIDES_ENABLE_EXAMPLES)
  add_subdirectory(examples)
endif()

if (FALSE) # XXX(kitware): VTK handles building shared libs/testing and MPI
option(FIDES_BUILD_DOCS "Build Fides User Guide locally" OFF)
else ()
set(FIDES_BUILD_DOCS OFF)
endif ()
if (FIDES_BUILD_DOCS)
  add_subdirectory(docs)
endif()
