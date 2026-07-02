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
#  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
###############################################################################
#-----------------------------------------------------------------------------
# Compiler specific flags
#-----------------------------------------------------------------------------

  set (CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
  if (${HDF_CFG_NAME} MATCHES "Debug" OR ${HDF_CFG_NAME} MATCHES "Developer")
    if (NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 5.0)
      set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Og -ftrapv -fno-common")
    endif ()
  else ()
    if (NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 5.0 AND
        NOT CMAKE_C_CLANG_TIDY)
      # `clang-tidy` does not understand -fstdarg-opt
      set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstdarg-opt")
    endif ()
    if (NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 10.0)
      #-----------------------------------------------------------------------------
      # Option to allow the user to enable build extended diagnostics
      #
      # This should NOT be on by default as it can cause process issues.
      #-----------------------------------------------------------------------------
      if (FALSE) # XXX(kitware): Hardcode settings.
      option (HDF5_ENABLE_BUILD_DIAGS "Enable color and URL extended diagnostic messages" OFF)
      mark_as_advanced (HDF5_ENABLE_BUILD_DIAGS)
      else ()
      set(HDF5_ENABLE_BUILD_DIAGS OFF)
      endif ()
      if (HDF5_ENABLE_BUILD_DIAGS)
        message (STATUS "... default color and URL extended diagnostic messages enabled")
      else ()
        message (STATUS "... disable color and URL extended diagnostic messages")
        #set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdiagnostics-urls=never -fno-diagnostics-color")
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
    # Add general CFlags for GCC versions 4.8 and above
    if (CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 4.8)
      ADD_H5_FLAGS (HDF5_CMAKE_C_FLAGS "${HDF_CONFIG_DIR}/gnu-warnings/general")
      ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/error-general")
    endif ()
    # gcc automatically inlines based on the optimization level
    # this is just a failsafe
    list (APPEND H5_CFLAGS "-finline-functions")
  message (VERBOSE "CMAKE_C_FLAGS_GENERAL=${HDF5_CMAKE_C_FLAGS}")
endif ()

#-----------------------------------------------------------------------------
# Option to allow the user to enable developer warnings
# Developer warnings (suggestions from gcc, not code problems)
#-----------------------------------------------------------------------------
if (HDF5_ENABLE_DEV_WARNINGS)
  message (STATUS "....HDF5 developer group warnings are enabled")
  if (CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 4.8)
    ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/developer-general")
  endif ()

  # Turn on -Winline warnings now only for non-Debug and
  # non-Developer builds. For at least GNU compilers this
  # flag appears to conflict specifically with the -Og
  # optimization flag and will produce warnings about functions
  # not being considered for inlining
  if (NOT ${HDF_CFG_NAME} MATCHES "Debug" AND NOT ${HDF_CFG_NAME} MATCHES "Developer")
      list (APPEND H5_CFLAGS "-Winline")
  endif ()
else ()
  if (CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 4.8)
    ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/no-developer-general")
  endif ()
endif ()

  # Technically, variable-length arrays are part of the C99 standard, but
  #   we should approach them a bit cautiously... Only needed for gcc 4.X
  if (CMAKE_C_COMPILER_VERSION VERSION_LESS 5.0 AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 4.8)
    ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/4.8-4.last")
  endif ()

  # Append more extra warning flags that only gcc 4.8+ knows about
  if (NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 4.8)
    ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/4.8")
    if (HDF5_ENABLE_DEV_WARNINGS)
      ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/developer-4.8")
    else ()
      ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/no-developer-4.8")
    endif ()
  endif ()

  # Append more extra warning flags that only gcc 4.9+ knows about
  if (NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 4.9)
    ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/4.9")
  endif ()

  # Append more extra warning flags that only gcc 5.x+ knows about
  if (NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 5.0)
    ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/5")
    ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/error-5")
  endif ()

  # Append more extra warning flags that only gcc 6.x+ knows about
  if (NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 6.0)
    ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/6")
  endif ()

  # Append more extra warning flags that only gcc 7.x+ knows about
  if (NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 7.0)
    ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/7")
    ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/error-7")
    if (HDF5_ENABLE_DEV_WARNINGS)
      ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/developer-7")
    #else ()
    #  ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/no-developer-7")
    endif ()
  endif ()

  # Append more extra warning flags that only gcc 8.x+ knows about
  if (NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 8.0)
    ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/8")
    ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/error-8")
    if (HDF5_ENABLE_DEV_WARNINGS)
      ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/developer-8")
    else ()
      ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/no-developer-8")
    endif ()
  endif ()

  # Append more extra warning flags that only gcc 9.x+ knows about
  if (NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 9.0)
    ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/9")
  endif ()

  # Append more extra warning flags that only gcc 9.3+ knows about
  if (NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 9.3)
    ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/9.3")
  endif ()

  # Append more extra warning flags that only gcc 10.x+ knows about
  if (NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 10.0)
    if (HDF5_ENABLE_DEV_WARNINGS)
      ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/developer-10")
    #else ()
    #  ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/no-developer-10")
    endif ()
  endif ()

  # Append more extra warning flags that only gcc 12.x+ knows about
  # or which should only be enabled for gcc 12.x+
  if (NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 12.0)
    if (HDF5_ENABLE_DEV_WARNINGS)
      ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/developer-12")
    #else ()
    #  ADD_H5_FLAGS (H5_CFLAGS "${HDF_CONFIG_DIR}/gnu-warnings/no-developer-12")
    endif ()
  endif ()

#-----------------------------------------------------------------------------
# This is in here to help some of the GCC based IDES like Eclipse
# and code blocks parse the compiler errors and warnings better.
#-----------------------------------------------------------------------------
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0")

#-----------------------------------------------------------------------------
# This option will force/override the default setting for all configurations
#-----------------------------------------------------------------------------
if (HDF5_ENABLE_SYMBOLS STREQUAL "YES")
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fno-omit-frame-pointer")
elseif (HDF5_ENABLE_SYMBOLS STREQUAL "NO")
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s")
endif ()
