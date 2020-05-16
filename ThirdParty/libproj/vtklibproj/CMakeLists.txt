#################################################################################
#
# This file is part of CMake configuration for PROJ4 library (inspired from SOCI
# CMake,  Copyright (C) 2009-2010 Mateusz Loskot <mateusz@loskot.net> )
#
# Copyright (C) 2011 Nicolas David <nicolas.david@ign.fr>
# Distributed under the MIT license
#
#################################################################################
# General settings
#################################################################################
if (FALSE) # XXX(kitware): VTK handles CMake versions.
cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)
endif ()

# proj4 is an ANSI C project
project(PROJ4 C)
set(PROJECT_INTERN_NAME PROJ)

#################################################################################
# PROJ4 CMake modules
#################################################################################
# Path to additional CMake modules
set(CMAKE_MODULE_PATH ${PROJ4_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH ${PROJ4_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

include(Proj4Utilities)

if (FALSE) # XXX(kitware): Hide configure noise.
message(STATUS "")
endif ()
colormsg(_HIBLUE_ "Configuring PROJ:")

#################################################################################
#PROJ version information
#################################################################################
include(Proj4Version)
proj_version(MAJOR 4 MINOR 9 PATCH 3)
set(PROJ_API_VERSION "12")
set(PROJ_BUILD_VERSION "12.0.0")

#################################################################################
# Build features and variants
#################################################################################
include(Proj4SystemInfo)
include(Proj4Config)
if (FALSE) # XXX(kitware): VTK handles CMake versions.
include(Proj4Mac)
include(policies)
endif ()

#################################################################################
# Self-test build config
#################################################################################

if (FALSE) # XXX(kitware): Hardcode settings
option(SELFTEST "Include self-test in build" OFF)
else ()
set(SELFTEST OFF)
endif ()
if(SELFTEST)
    add_definitions(-DPJ_SELFTEST)
endif(SELFTEST)

#################################################################################
# threading configuration
#################################################################################
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package (Threads)

if (Threads_FOUND AND CMAKE_USE_PTHREADS_INIT)
  set (CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}")
  check_c_source_compiles("
#include <pthread.h>
int main(int argc, char* argv[]) {
  (void)PTHREAD_MUTEX_RECURSIVE;
  (void)argv;
  return argc;
}
  " HAVE_PTHREAD_MUTEX_RECURSIVE_DEFN)
  if (HAVE_PTHREAD_MUTEX_RECURSIVE_DEFN)
    add_definitions(-DHAVE_PTHREAD_MUTEX_RECURSIVE=1)
  endif()
endif ()

boost_report_value(PROJ_PLATFORM_NAME)
boost_report_value(PROJ_COMPILER_NAME)

# Set a default build type for single-configuration cmake generators if
# no build type is set.
if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release)
endif ()

if (FALSE) # XXX(kitware): VTK handles this.
if (MSVC OR CMAKE_CONFIGURATION_TYPES)
  # For multi-config systems and for Visual Studio, the debug version of
  # the library has _d appended.
  set (CMAKE_DEBUG_POSTFIX _d)
endif ()
endif ()

if (FALSE) # XXX(kitware): Hardcode settings
option(PROJ4_TESTS "Enable build of collection of PROJ4 tests" ON)
else ()
set(PROJ4_TESTS OFF)
endif ()
boost_report_value(PROJ4_TESTS)
if(PROJ4_TESTS)
    include(CTest)
    enable_testing()
endif(PROJ4_TESTS)
include(Proj4Test)

# Put the libaries and binaries that get built into directories at the
# top of the build tree rather than in hard-to-find leaf
# directories. This simplifies manual testing and the use of the build
# tree rather than installed Boost libraries.
if (FALSE) # XXX(kitware): VTK handles this.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif ()

#################################################################################
# Installation
#################################################################################
if (FALSE) # XXX(kitware): VTK handles installation.
include(Proj4InstallPath)
set(BINDIR "${DEFAULT_BINDIR}" CACHE PATH "The directory to install binaries into.")
set(LIBDIR "${DEFAULT_LIBDIR}" CACHE PATH "The directory to install libraries into.")
set(DATADIR "${DEFAULT_DATADIR}" CACHE PATH "The directory to install data files into.")
set(DOCDIR "${DEFAULT_DOCDIR}" CACHE PATH "The directory to install doc files into.")
set(INCLUDEDIR "${DEFAULT_INCLUDEDIR}" CACHE PATH "The directory to install includes into.")
endif ()

#################################################################################
# Build configured components
#################################################################################
include_directories(${PROJ4_SOURCE_DIR}/src)

if (FALSE) # XXX(kitware): Hide configure noise.
message(STATUS "")
endif ()
if (FALSE) # XXX(kitware): Hide unnecessary directories.
add_subdirectory(nad)
endif ()
add_subdirectory(src)
if (FALSE) # XXX(kitware): Hide unnecessary directories.
add_subdirectory(man)
add_subdirectory(cmake)
endif ()

