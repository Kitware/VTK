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
ENABLE_LANGUAGE (CXX)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED TRUE)

set (CMAKE_CXX_EXTENSIONS OFF)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_SANITIZER_FLAGS} ${CMAKE_CXX_FLAGS}")
#message (VERBOSE "Warnings Configuration: CXX default: ${CMAKE_CXX_FLAGS}")
#-----------------------------------------------------------------------------
# Compiler specific flags
#-----------------------------------------------------------------------------
# MSVC 14.28 enables C5105, but the Windows SDK 10.0.18362.0 triggers it.
if ((_CLANG_MSVC_WINDOWS OR CMAKE_CXX_COMPILER_ID STREQUAL "MSVC") AND CMAKE_CXX_COMPILER_LOADED)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
  if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.28)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd5105")
  endif ()
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL SunPro AND CMAKE_CXX_COMPILER_LOADED)
  if (NOT DEFINED CMAKE_CXX${CMAKE_CXX_STANDARD}_STANDARD_COMPILE_OPTION)
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.13)
      if (NOT CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD EQUAL 98)
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++03")
      endif ()
    else ()
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -library=stlport4")
    endif ()
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to allow the user to disable compiler warnings
#-----------------------------------------------------------------------------
if (CMAKE_CXX_COMPILER_LOADED)
  if (HDF5_DISABLE_COMPILER_WARNINGS)
    #message (STATUS "....Compiler warnings are suppressed")
    # MSVC uses /w to suppress warnings.  It also complains if another
    # warning level is given, so remove it.
    if (MSVC)
      set (HDF5_WARNINGS_BLOCKED 1)
      string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0")
    endif ()
    if (WIN32)
      add_definitions (-D_CRT_SECURE_NO_WARNINGS)
    endif ()

    # Most compilers use -w to suppress warnings.
    if (NOT HDF5_WARNINGS_BLOCKED)
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
    endif ()
  endif ()

  if (CMAKE_CXX_COMPILER_ID MATCHES "NVHPC" )
    include (${HDF_CONFIG_DIR}/flags/HDFNvhpcCXXFlags.cmake)
  endif ()

  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    include (${HDF_CONFIG_DIR}/flags/HDFGnuCXXFlags.cmake)
  endif ()

  if (CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    include (${HDF_CONFIG_DIR}/flags/HDFIntelCXXFlags.cmake)
  endif ()

  if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    include (${HDF_CONFIG_DIR}/flags/HDFMsvcCXXFlags.cmake)
  endif ()

  #because this will match other compilers with clang in the name this should be checked last
  if (CMAKE_CXX_COMPILER_ID MATCHES "[Cc]lang")
    include (${HDF_CONFIG_DIR}/flags/HDFClangCXXFlags.cmake)
  endif ()

  #-----------------------------------------------------------------------------
  # HDF5 library compile options - to be made available to all targets
  #-----------------------------------------------------------------------------

  if (${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
    list (APPEND HDF5_CMAKE_CXX_FLAGS "-erroff=%none -DBSD_COMP")
  else ()
    # General flags
    #
    # Note that some of the flags listed here really should be developer
    # flags (listed in a separate variable, below) but we put them here
    # because they are not raised by the current code and we'd like to
    # know if they do start showing up.
    #
    # NOTE: Don't add -Wpadded here since we can't/won't fix the (many)
    # warnings that are emitted. If you need it, add it at configure time.
    if (CMAKE_CXX_COMPILER_ID STREQUAL "PGI")
      list (APPEND HDF5_CMAKE_CXX_FLAGS "-Minform=inform")
    endif ()
    #message (VERBOSE "CMAKE_CXX_FLAGS_GENERAL=${HDF5_CMAKE_CXX_FLAGS}")
  endif ()

  #-----------------------------------------------------------------------------
  # Option to allow the user to enable all warnings
  #-----------------------------------------------------------------------------
  if (HDF5_ENABLE_ALL_WARNINGS)
    #message (STATUS "....All Warnings are enabled")
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
  # This option will force/override the default setting for all configurations
  #-----------------------------------------------------------------------------
  if (HDF5_ENABLE_PROFILING)
    list (APPEND HDF5_CMAKE_CXX_FLAGS "${PROFILE_CXXFLAGS}")
  endif ()

  #-----------------------------------------------------------------------------
  # This option will force/override the default setting for all configurations
  #-----------------------------------------------------------------------------
  if (HDF5_ENABLE_OPTIMIZATION)
    list (APPEND HDF5_CMAKE_CXX_FLAGS "${OPTIMIZE_CXXFLAGS}")
  endif ()

  #-----------------------------------------------------------------------------
  # Option to override the compiler for h5c++
  #-----------------------------------------------------------------------------
  if (HDF5_ENABLE_PARALLEL AND MPI_CXX_FOUND)
    set (_HDF5_H5CC_CXX_COMPILER ${MPI_CXX_COMPILER})
  else ()
    set (_HDF5_H5CC_CXX_COMPILER ${CMAKE_CXX_COMPILER})
  endif ()
  set (HDF5_H5CC_CXX_COMPILER ${_HDF5_H5CC_CXX_COMPILER} CACHE STRING "C++ compiler to use in h5c++")
endif ()

#-----------------------------------------------------------------------------
# The build mode flags are not added to CMAKE_CXX_FLAGS, so create a separate
# variable for them so they can be written out to libhdf5.settings and
# H5build_settings.c
#-----------------------------------------------------------------------------
if ("${HDF_CFG_NAME}" STREQUAL     "Debug")
  set (HDF5_BUILD_MODE_CXX_FLAGS   "${CMAKE_CXX_FLAGS_DEBUG}")
elseif ("${HDF_CFG_NAME}" STREQUAL "Developer")
  set (HDF5_BUILD_MODE_CXX_FLAGS   "${CMAKE_CXX_FLAGS_DEVELOPER}")
elseif ("${HDF_CFG_NAME}" STREQUAL "Release")
  set (HDF5_BUILD_MODE_CXX_FLAGS   "${CMAKE_CXX_FLAGS_RELEASE}")
elseif ("${HDF_CFG_NAME}" STREQUAL "MinSizeRel")
  set (HDF5_BUILD_MODE_CXX_FLAGS   "${CMAKE_CXX_FLAGS_MINSIZEREL}")
elseif ("${HDF_CFG_NAME}" STREQUAL "RelWithDebInfo")
  set (HDF5_BUILD_MODE_CXX_FLAGS   "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
else ()
  set (HDF5_BUILD_MODE_CXX_FLAGS   "")
endif ()
