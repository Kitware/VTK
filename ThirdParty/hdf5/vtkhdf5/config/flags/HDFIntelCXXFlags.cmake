#
# Copyright by The HDF Group.
# All rights reserved.
#
# This file is part of HDF5.  The full HDF5 copyright notice, including
# terms governing use, modification, and redistribution, is contained in
# the LICENSE file, which can be found at the root of the source code
# distribution tree, or in https://www.hdfgroup.org/licenses.
# If you do not have access to either file, you may request a copy from
# help@hdfgroup.org.
#

###############################################################################
# This file included from HDFCompilerFlags.cmake with
#  if (CMAKE_CXX_COMPILER_ID MATCHES "Intel")
###############################################################################

#-----------------------------------------------------------------------------
# Compiler specific flags
#-----------------------------------------------------------------------------
if (WIN32)
  set (_INTEL_WINDOWS 1)
endif ()

#-----------------------------------------------------------------------------
# HDF5 library compile options - to be made available to all targets
#-----------------------------------------------------------------------------
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
  # General flags
  #
  # Note that some of the flags listed here really should be developer
  # flags (listed in a separate variable, below) but we put them here
  # because they are not raised by the current code and we'd like to
  # know if they do start showing up.
  #
  # NOTE: Don't add -Wpadded here since we can't/won't fix the (many)
  # warnings that are emitted. If you need it, add it at configure time.
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    if (_INTEL_WINDOWS)
      ADD_H5_FLAGS (HDF5_CMAKE_CXX_FLAGS "${HDF_CONFIG_DIR}/intel-warnings/classic/win-general")
    else ()
      ADD_H5_FLAGS (HDF5_CMAKE_CXX_FLAGS "${HDF_CONFIG_DIR}/intel-warnings/classic/general")
    endif()
    if (NOT _INTEL_WINDOWS)
      if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 15.0)
        ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/intel-warnings/classic/15")
      endif ()
      if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 18.0)
        ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/intel-warnings/classic/18")
      endif ()
    endif ()
  elseif (CMAKE_CXX_COMPILER_ID MATCHES "IntelLLVM")
    if (_INTEL_WINDOWS)
      ADD_H5_FLAGS (HDF5_CMAKE_CXX_FLAGS "${HDF_CONFIG_DIR}/intel-warnings/oneapi/win-general")
    else ()
      ADD_H5_FLAGS (HDF5_CMAKE_CXX_FLAGS "${HDF_CONFIG_DIR}/intel-warnings/oneapi/general")
    endif ()
  endif ()
  #message (VERBOSE "CMAKE_CXX_FLAGS_GENERAL=${HDF5_CMAKE_CXX_FLAGS}")
endif ()

#-----------------------------------------------------------------------------
# Option to allow the user to enable developer warnings
# Developer warnings (suggestions from gcc, not code problems)
#-----------------------------------------------------------------------------
if (HDF5_ENABLE_DEV_WARNINGS)
  message (STATUS "....HDF5 CXX developer group warnings are enabled")
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    if (_INTEL_WINDOWS)
      ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/intel-warnings/classic/win-developer-general")
    else ()
      ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/intel-warnings/classic/developer-general")
    endif ()
  elseif (CMAKE_CXX_COMPILER_ID MATCHES "IntelLLVM")
    if (_INTEL_WINDOWS)
      ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/intel-warnings/oneapi/win-developer-general")
    else ()
      ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/intel-warnings/oneapi/developer-general")
    endif ()
  endif ()
endif ()

#-----------------------------------------------------------------------------
# This option will force/override the default setting for all configurations
#-----------------------------------------------------------------------------
if (NOT _INTEL_WINDOWS)
  if (HDF5_ENABLE_SYMBOLS STREQUAL "YES")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
  elseif (HDF5_ENABLE_SYMBOLS STREQUAL "NO")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-s")
  endif ()
endif ()
