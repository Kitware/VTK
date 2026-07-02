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
#  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
###############################################################################
#-----------------------------------------------------------------------------
# Compiler specific flags
#-----------------------------------------------------------------------------

  set (CMAKE_CXX_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_CXX_FLAGS}")
  if (${HDF_CFG_NAME} MATCHES "Debug" OR ${HDF_CFG_NAME} MATCHES "Developer")
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og -ftrapv -fno-common")
    endif ()
  else ()
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstdarg-opt")
    endif ()
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0)
      if (HDF5_ENABLE_BUILD_DIAGS)
        message (STATUS "... default color and URL extended diagnostic messages enabled")
      else ()
        message (STATUS "... disable color and URL extended diagnostic messages")
        #set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-urls=never -fno-diagnostics-color")
      endif ()
    endif ()
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
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 4.8)
      # add the general CXX flags for g++ compiler versions 4.8 and above.
      ADD_H5_FLAGS (HDF5_CMAKE_CXX_FLAGS "${HDF_CONFIG_DIR}/gnu-warnings/cxx-general")
      ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/cxx-error-general")
    endif ()
  #message (VERBOSE "CMAKE_CXX_FLAGS_GENERAL=${HDF5_CMAKE_CXX_FLAGS}")
endif ()

#-----------------------------------------------------------------------------
# Option to allow the user to enable developer warnings
# Developer warnings (suggestions from gcc, not code problems)
#-----------------------------------------------------------------------------
if (HDF5_ENABLE_DEV_WARNINGS)
  message (STATUS "....HDF5 CXX developer group warnings are enabled")
    # Use the C warnings as CXX warnings are the same
    ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/developer-general")
else ()
    # Use the C warnings as CXX warnings are the same
    ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/no-developer-general")
endif ()

  # Technically, variable-length arrays are part of the C99 standard, but
  #   we should approach them a bit cautiously... Only needed for gcc 4.X
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0 AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 4.8)
    # Use the C warnings as CXX warnings are the same
    ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/4.8-4.last")
  endif ()

  # Append more extra warning flags that only gcc 4.8+ knows about
  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
    ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/cxx-4.8")
    if (HDF5_ENABLE_DEV_WARNINGS)
      # Use the C warnings as CXX warnings are the same
      ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/cxx-developer-4.8")
    else ()
      ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/no-cxx-developer-4.8")
    endif ()
  endif ()

  # Append more extra warning flags that only gcc 4.9+ knows about
  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
    # Use the C warnings as CXX warnings are the same
    ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/cxx-4.9")
  endif ()

  # Append more extra warning flags that only gcc 5.1+ knows about
  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
    ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/cxx-5")
    ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/cxx-error-5")
  endif ()

  # Append more extra warning flags that only gcc 6.x+ knows about
  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
    # Use the C warnings as CXX warnings are the same
    ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/6")
  endif ()

  # Append more extra warning flags that only gcc 7.x+ knows about
  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
    # Use the C warnings as CXX warnings are the same
    ADD_H5_FLAGS (H5_CXxFLAGS2 "${HDF_CONFIG_DIR}/gnu-warnings/7")
    if (HDF5_ENABLE_DEV_WARNINGS)
    # Use the C warnings as CXX warnings are the same
      ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/developer-7")
    #else ()
    #  ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/no-developer-7")
    endif ()
  endif ()

  # Append more extra warning flags that only gcc 8.x+ knows about
  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
    # Use the C warnings as CXX warnings are the same
    ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/8")
    ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/error-8")
    if (HDF5_ENABLE_DEV_WARNINGS)
      # Use the C warnings as CXX warnings are the same
      ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/developer-8")
    else ()
      ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/no-developer-8")
    endif ()
  endif ()

  # Append more extra warning flags that only gcc 9.x+ knows about
  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    # Use the C warnings as CXX warnings are the same
    ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/cxx-9")
  endif ()

  # Append more extra warning flags that only gcc 9.3+ knows about
  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.3)
    # do not use C warnings, gnu-warnings 9.3, no cxx warnings
    # ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/9.3")
  endif ()

  # Append more extra warning flags that only gcc 10.x+ knows about
  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0)
    if (HDF5_ENABLE_DEV_WARNINGS)
      # Use the C warnings as CXX warnings are the same
      ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/developer-10")
    #else ()
    #  ADD_H5_FLAGS (H5_CXXFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/no-developer-10")
    endif ()
  endif ()

#-----------------------------------------------------------------------------
# Option to allow the user to enable all warnings
#-----------------------------------------------------------------------------
if (HDF5_ENABLE_ALL_WARNINGS)
  message (STATUS "....All CXX Warnings are enabled")
  if (MSVC)
    if (HDF5_ENABLE_DEV_WARNINGS)
      string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
      list (APPEND HDF5_CMAKE_CXX_FLAGS "/Wall" "/wd4668")
    else ()
      string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
      list (APPEND HDF5_CMAKE_CXX_FLAGS "/W3" "/wd4100" "/wd4706" "/wd4127")
    endif ()
  else ()
    list (APPEND HDF5_CMAKE_CXX_FLAGS ${H5_CXXFLAGS})
  endif ()
endif ()

#-----------------------------------------------------------------------------
# This is in here to help some of the GCC based IDES like Eclipse
# and code blocks parse the compiler errors and warnings better.
#-----------------------------------------------------------------------------
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0")

#-----------------------------------------------------------------------------
# This option will force/override the default setting for all configurations
#-----------------------------------------------------------------------------
if (HDF5_ENABLE_SYMBOLS STREQUAL "YES")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
elseif (HDF5_ENABLE_SYMBOLS STREQUAL "NO")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s")
endif ()
