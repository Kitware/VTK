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
set (CMAKE_C_STANDARD 11)
set (CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_C_EXTENSIONS ON)

set (CMAKE_C_FLAGS "${CMAKE_C11_STANDARD_COMPILE_OPTION} ${CMAKE_C_FLAGS}")
set (CMAKE_C_FLAGS "${CMAKE_C_SANITIZER_FLAGS} ${CMAKE_C_FLAGS}")
#message (VERBOSE "Warnings Configuration: C default: ${CMAKE_C_FLAGS}")
#-----------------------------------------------------------------------------
# Compiler specific flags
#-----------------------------------------------------------------------------
# Disable deprecation warnings for standard C functions.
# really only needed for newer versions of VS, but should
# not hurt other versions, and this will work into the
# future
if (MSVC)
  add_definitions (-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stack:10000000")
endif ()


#-----------------------------------------------------------------------------
# Option to allow the user to disable compiler warnings
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_DISABLE_COMPILER_WARNINGS "Disable compiler warnings" OFF)
else ()
set(HDF5_DISABLE_COMPILER_WARNINGS OFF)
endif ()
if (HDF5_DISABLE_COMPILER_WARNINGS)
  #message (STATUS "....Compiler warnings are suppressed")
  # MSVC uses /w to suppress warnings.  It also complains if another
  # warning level is given, so remove it.
  if (MSVC)
    set (HDF5_WARNINGS_BLOCKED 1)
    string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W0")
  endif ()
  if (WIN32)
    add_definitions (-D_CRT_SECURE_NO_WARNINGS)
  endif ()
  # Borland uses -w- to suppress warnings.
  if (BORLAND)
    set (HDF5_WARNINGS_BLOCKED 1)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w-")
  endif ()

  # Most compilers use -w to suppress warnings.
  if (NOT HDF5_WARNINGS_BLOCKED)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
  endif ()
endif ()

if (CMAKE_C_COMPILER_ID MATCHES "NVHPC" )
  include (${HDF_CONFIG_DIR}/flags/HDFNvhpcFlags.cmake)
endif ()

if (CMAKE_C_COMPILER_ID MATCHES "GNU")
  include (${HDF_CONFIG_DIR}/flags/HDFGnuFlags.cmake)
endif ()

if (CMAKE_C_COMPILER_ID MATCHES "Intel")
  include (${HDF_CONFIG_DIR}/flags/HDFIntelFlags.cmake)
endif ()

if (CMAKE_C_COMPILER_ID MATCHES "MSVC")
  include (${HDF_CONFIG_DIR}/flags/HDFMsvcFlags.cmake)
endif ()

#because this will match other compilers with clang in the name this should be checked last
if (CMAKE_C_COMPILER_ID MATCHES "[Cc]lang")
  include (${HDF_CONFIG_DIR}/flags/HDFClangFlags.cmake)
endif ()

#-----------------------------------------------------------------------------
# HDF5 library compile options - to be made available to all targets
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Option to allow the user to interpret certain warnings as errors
#
# This should NOT be on by default as it can cause a lot of conflicts with
# new operating systems and compiler versions. Header files that are out of
# our control (MPI, HDFS, etc.) can also raise warnings.
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_ENABLE_WARNINGS_AS_ERRORS "Interpret some warnings as errors" OFF)
if (HDF5_ENABLE_WARNINGS_AS_ERRORS)
  #message (STATUS "...some warnings will be interpreted as errors")
endif ()
else ()
set(HDF5_ENABLE_WARNINGS_AS_ERRORS OFF)
endif ()
if (${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
  list (APPEND HDF5_CMAKE_C_FLAGS "-erroff=%none -DBSD_COMP")
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
  if (CMAKE_C_COMPILER_ID STREQUAL "PGI")
    list (APPEND HDF5_CMAKE_C_FLAGS "-Minform=inform")
  endif ()
  #message (VERBOSE "CMAKE_C_FLAGS_GENERAL=${HDF5_CMAKE_C_FLAGS}")
endif ()

#-----------------------------------------------------------------------------
# Option to allow the user to enable developer warnings
# Developer warnings (suggestions from gcc, not code problems)
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_ENABLE_DEV_WARNINGS "Enable HDF5 developer group warnings" OFF)
if (${HDF_CFG_NAME} MATCHES "Developer")
  # Developer build modes should always have these types of warnings enabled
  set (HDF5_ENABLE_DEV_WARNINGS ON CACHE BOOL "Enable HDF5 developer group warnings" FORCE)
endif ()
else ()
set(HDF5_ENABLE_DEV_WARNINGS OFF)
endif ()

#-----------------------------------------------------------------------------
# Option to allow the user to enable debug output
# from various HDF5 modules
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_ENABLE_DEBUG_APIS "Turn on extra debug output in all packages" OFF)
if (HDF5_ENABLE_DEBUG_APIS)
  # Add standard debug definitions to any existing ones
  list (APPEND HDF5_DEBUG_APIS
      H5AC_DEBUG
      H5CX_DEBUG
      H5D_DEBUG
      H5D_CHUNK_DEBUG
      H5F_DEBUG
      H5MM_DEBUG
      H5O_DEBUG
      H5T_DEBUG
      H5Z_DEBUG
  )
endif ()
else ()
  set(HDF5_ENABLE_DEBUG_APIS OFF)
endif ()

#-----------------------------------------------------------------------------
# Option to allow the user to enable all warnings
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_ENABLE_ALL_WARNINGS "Enable all warnings" ON)
if (HDF5_ENABLE_ALL_WARNINGS)
  #message (STATUS "....All Warnings are enabled")
  if (MSVC)
    if (HDF5_ENABLE_DEV_WARNINGS)
      string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
      list (APPEND HDF5_CMAKE_C_FLAGS "/Wall" "/wd4668")
    else ()
      string (REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
      list (APPEND HDF5_CMAKE_C_FLAGS "/W3" "/wd4100" "/wd4706" "/wd4127")
    endif ()
  else ()
    list (APPEND HDF5_CMAKE_C_FLAGS ${H5_CFLAGS})
  endif ()
endif ()
mark_as_advanced (HDF5_ENABLE_ASSERTS)
else ()
set(HDF5_ENABLE_ALL_WARNINGS OFF)
endif ()

#-----------------------------------------------------------------------------
# By default, CMake adds NDEBUG to CMAKE_${lang}_FLAGS for Release build types
# This option will force/override the default setting for all configurations
#-----------------------------------------------------------------------------
#option (HDF5_ENABLE_ASSERTS "Determines whether NDEBUG is defined to control assertions." OFF)
if (FALSE) # XXX(kitware): Hardcode settings.
set (HDF5_ENABLE_ASSERTS "OFF" CACHE STRING "Determines whether NDEBUG is defined to control assertions (OFF NO YES)")
set_property (CACHE HDF5_ENABLE_ASSERTS PROPERTY STRINGS OFF NO YES)
else ()
set(HDF5_ENABLE_ASSERTS OFF)
endif ()
if (HDF5_ENABLE_ASSERTS MATCHES "YES")
  add_compile_options ("-UNDEBUG")
elseif (HDF5_ENABLE_ASSERTS MATCHES "NO")
  add_compile_options ("-DNDEBUG")
endif ()

#-----------------------------------------------------------------------------
# This option will force/override the default setting for all configurations
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
#option (HDF5_ENABLE_SYMBOLS "Add debug symbols to the library independent of the build mode and optimization level." OFF)
set (HDF5_ENABLE_SYMBOLS "OFF" CACHE STRING "Add debug symbols to the library independent of the build mode and optimization level (OFF NO YES)")
set_property (CACHE HDF5_ENABLE_SYMBOLS PROPERTY STRINGS OFF NO YES)
mark_as_advanced (HDF5_ENABLE_SYMBOLS)
else ()
set(HDF5_ENABLE_SYMBOLS OFF)
endif ()

#-----------------------------------------------------------------------------
# This option will force/override the default setting for all configurations
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_ENABLE_PROFILING "Enable profiling flags independently from the build mode." OFF)
mark_as_advanced (HDF5_ENABLE_PROFILING)
else ()
set(HDF5_ENABLE_PROFILING OFF)
endif ()
if (HDF5_ENABLE_PROFILING)
  list (APPEND HDF5_CMAKE_C_FLAGS "${PROFILE_CFLAGS}")
endif ()

#-----------------------------------------------------------------------------
# This option will force/override the default setting for all configurations
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
mark_as_advanced (HDF5_ENABLE_OPTIMIZATION)
else ()
set(HDF5_ENABLE_OPTIMIZATION OFF)
endif ()
option (HDF5_ENABLE_OPTIMIZATION "Enable optimization flags/settings independently from the build mode" OFF)
if (HDF5_ENABLE_OPTIMIZATION)
  list (APPEND HDF5_CMAKE_C_FLAGS "${OPTIMIZE_CFLAGS}")
endif ()

#-----------------------------------------------------------------------------
# The build mode flags are not added to CMAKE_C_FLAGS, so create a separate
# variable for them so they can be written out to libhdf5.settings and
# H5build_settings.c
#-----------------------------------------------------------------------------
if ("${HDF_CFG_NAME}" STREQUAL     "Debug")
  set (HDF5_BUILD_MODE_C_FLAGS     "${CMAKE_C_FLAGS_DEBUG}")
elseif ("${HDF_CFG_NAME}" STREQUAL "Developer")
  set (HDF5_BUILD_MODE_C_FLAGS     "${CMAKE_C_FLAGS_DEVELOPER}")
elseif ("${HDF_CFG_NAME}" STREQUAL "Release")
  set (HDF5_BUILD_MODE_C_FLAGS     "${CMAKE_C_FLAGS_RELEASE}")
elseif ("${HDF_CFG_NAME}" STREQUAL "MinSizeRel")
  set (HDF5_BUILD_MODE_C_FLAGS     "${CMAKE_C_FLAGS_MINSIZEREL}")
elseif ("${HDF_CFG_NAME}" STREQUAL "RelWithDebInfo")
  set (HDF5_BUILD_MODE_C_FLAGS     "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
else ()
  set (HDF5_BUILD_MODE_C_FLAGS     "")
endif ()

