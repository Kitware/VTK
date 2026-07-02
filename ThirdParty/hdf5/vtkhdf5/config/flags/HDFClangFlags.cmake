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
#  if (CMAKE_C_COMPILER_ID MATCHES "[Cc]lang")
###############################################################################

#-----------------------------------------------------------------------------
# Compiler specific flags
#-----------------------------------------------------------------------------
if (${HDF_CFG_NAME} MATCHES "Debug" OR ${HDF_CFG_NAME} MATCHES "Developer")
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Og -ftrapv -fno-common")
endif ()

if (WIN32 AND "x${CMAKE_C_SIMULATE_ID}" STREQUAL "xMSVC")
  set (_CLANG_MSVC_WINDOWS 1)
  if("x${CMAKE_C_COMPILER_FRONTEND_VARIANT}" STREQUAL "xGNU")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker -stack:20000000")
  endif()
endif ()

# Disable deprecation warnings for standard C functions.
# really only needed for newer versions of VS, but should
# not hurt other versions, and this will work into the
# future
if (MSVC OR _CLANG_MSVC_WINDOWS)
  add_definitions (-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
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
  ADD_H5_FLAGS (HDF5_CMAKE_C_FLAGS "${HDF_CONFIG_DIR}/clang-warnings/general")
  ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/clang-warnings/error-general")
  message (VERBOSE "CMAKE_C_FLAGS_GENERAL=${HDF5_CMAKE_C_FLAGS}")
endif ()

#-----------------------------------------------------------------------------
# Option to allow the user to enable developer warnings
# Developer warnings (suggestions from gcc, not code problems)
#-----------------------------------------------------------------------------
if (HDF5_ENABLE_DEV_WARNINGS)
  message (STATUS "....HDF5 developer group warnings are enabled")
  ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/clang-warnings/developer-general")

  # Turn on -Winline warnings now only for non-Debug and
  # non-Developer builds. For at least GNU compilers this
  # flag appears to conflict specifically with the -Og
  # optimization flag and will produce warnings about functions
  # not being considered for inlining
  if (NOT ${HDF_CFG_NAME} MATCHES "Debug" AND NOT ${HDF_CFG_NAME} MATCHES "Developer")
    list (APPEND H5_CFLAGS "-Winline")
  endif ()
else ()
  ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/clang-warnings/no-developer-general")
endif ()
