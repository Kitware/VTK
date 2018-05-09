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
cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

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

message(STATUS "")
colormsg(_HIBLUE_ "Configuring PROJ:")

#################################################################################
#PROJ version information
#################################################################################
include(Proj4Version)
proj_version(MAJOR 4 MINOR 9 PATCH 2)
set(PROJ_API_VERSION "9")
set(PROJ_BUILD_VERSION "10.0.1")

#################################################################################
# Build features and variants
#################################################################################
include(Proj4Config)
include(Proj4Mac)
include(policies)

#################################################################################
# threading configuration
#################################################################################
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package (Threads)

include(CheckIncludeFiles)
include(CheckSymbolExists)
CHECK_SYMBOL_EXISTS(PTHREAD_MUTEX_RECURSIVE pthread.h HAVE_PTHREAD_MUTEX_RECURSIVE_DEFN)
if (HAVE_PTHREAD_MUTEX_RECURSIVE_DEFN)
    add_definitions(-DHAVE_PTHREAD_MUTEX_RECURSIVE=1)
endif()

# Set a default build type for single-configuration cmake generators if
# no build type is set.
if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release)
endif ()

if (FALSE) # XXX(kitware): Debug suffixes are handled by VTK.
if (MSVC OR CMAKE_CONFIGURATION_TYPES)
  # For multi-config systems and for Visual Studio, the debug version of
  # the library has _d appended.
  set (CMAKE_DEBUG_POSTFIX _d)
endif ()
endif ()

#################################################################################
# Installation
#################################################################################
include(Proj4InstallPath)
set(LIBPROJ_BINDIR "${DEFAULT_BINDIR}" CACHE PATH "The directory to install binaries into.")
set(LIBPROJ_LIBDIR "${DEFAULT_LIBDIR}" CACHE PATH "The directory to install libraries into.")
set(LIBPROJ_DATADIR "${DEFAULT_DATADIR}" CACHE PATH "The directory to install data files into.")
set(LIBPROJ_DOCDIR "${DEFAULT_DOCDIR}" CACHE PATH "The directory to install doc files into.")
set(LIBPROJ_INCLUDEDIR "${DEFAULT_INCLUDEDIR}" CACHE PATH "The directory to install includes into.")

mark_as_advanced(LIBPROJ_BINDIR LIBPROJ_LIBDIR LIBPROJ_DATADIR LIBPROJ_DOCDIR LIBPROJ_INCLUDEDIR)

#################################################################################
# Build configured components
#################################################################################
include_directories(${PROJ4_SOURCE_DIR}/src)

message(STATUS "")
add_subdirectory(src)


