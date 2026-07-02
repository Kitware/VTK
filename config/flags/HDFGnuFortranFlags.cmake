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
# This file included from HDFCompilerFortranFlags.cmake with
#  if (CMAKE_Fortran_COMPILER_ID MATCHES "GNU" )
###############################################################################

message (VERBOSE "Warnings Configuration: default Fortran: ${CMAKE_Fortran_FLAGS}")

#-----------------------------------------------------------------------------
# HDF5 library compile options - to be made available to all targets
#-----------------------------------------------------------------------------
if (NOT CMAKE_Fortran_COMPILER_VERSION VERSION_LESS 10.0)
  if (HDF5_ENABLE_BUILD_DIAGS)
    message (STATUS "... default color and URL extended diagnostic messages enabled")
  else ()
    message (STATUS "... disable color and URL extended diagnostic messages")
    #set (CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -fdiagnostics-urls=never -fno-diagnostics-color")
  endif ()
endif ()

if (NOT MSVC AND NOT MINGW)
  # General flags
  ADD_H5_FLAGS (HDF5_CMAKE_Fortran_FLAGS "${HDF_CONFIG_DIR}/gnu-warnings/gfort-general")
  if (HDF5_ENABLE_DEV_WARNINGS)
    ADD_H5_FLAGS (HDF5_CMAKE_Fortran_FLAGS "${HDF_CONFIG_DIR}/gnu-warnings/gfort-developer-general")
  else ()
    ADD_H5_FLAGS (HDF5_CMAKE_Fortran_FLAGS "${HDF_CONFIG_DIR}/gnu-warnings/gfort-no-developer-general")
  endif ()
  list (APPEND HDF5_CMAKE_Fortran_FLAGS "-ffree-form" "-fimplicit-none")
  if (CMAKE_Fortran_COMPILER_VERSION VERSION_LESS 8.0 AND NOT CMAKE_Fortran_COMPILER_VERSION VERSION_LESS 4.6)
    list (APPEND HDF5_CMAKE_Fortran_FLAGS "-std=f2008ts")
  else ()
    list (APPEND HDF5_CMAKE_Fortran_FLAGS "-std=f2008")
  endif ()
  message (VERBOSE "HDF5_CMAKE_Fortran_FLAGS=${HDF5_CMAKE_Fortran_FLAGS}")

  # Append more extra warning flags that only gcc 4.8+ knows about
  if (NOT CMAKE_Fortran_COMPILER_VERSION VERSION_LESS 4.8)
    ADD_H5_FLAGS (HDF5_CMAKE_Fortran_FLAGS "${HDF_CONFIG_DIR}/gnu-warnings/gfort-4.8")
    if (HDF5_ENABLE_DEV_WARNINGS)
      ADD_H5_FLAGS (HDF5_CMAKE_Fortran_FLAGS "${HDF_CONFIG_DIR}/gnu-warnings/gfort-developer-4.8")
    else ()
      ADD_H5_FLAGS (HDF5_CMAKE_Fortran_FLAGS "${HDF_CONFIG_DIR}/gnu-warnings/gfort-no-developer-4.8")
    endif ()
  endif ()

  # Append more extra warning flags that only gcc 4.9+ knows about
  #if (NOT CMAKE_Fortran_COMPILER_VERSION VERSION_LESS 4.9)
  #  ADD_H5_FLAGS (HDF5_CMAKE_Fortran_FLAGS "${HDF_CONFIG_DIR}/gnu-warnings/gfort-4.9")
  #endif ()

  # Append more extra warning flags that only gcc 5.x+ knows about
  if (NOT CMAKE_Fortran_COMPILER_VERSION VERSION_LESS 5.0)
    if (HDF5_ENABLE_DEV_WARNINGS)
      ADD_H5_FLAGS (HDF5_CMAKE_Fortran_FLAGS "${HDF_CONFIG_DIR}/gnu-warnings/gfort-developer-5")
    endif ()
  endif ()

  # Append more extra warning flags that only gcc 6.x+ knows about
  if (NOT CMAKE_Fortran_COMPILER_VERSION VERSION_LESS 6.0)
    ADD_H5_FLAGS (HDF5_CMAKE_Fortran_FLAGS "${HDF_CONFIG_DIR}/gnu-warnings/gfort-6")
  endif ()

  # Append more extra warning flags that only gcc 7.x+ knows about
  #if (NOT CMAKE_Fortran_COMPILER_VERSION VERSION_LESS 7.0)
  #  ADD_H5_FLAGS (HDF5_CMAKE_Fortran_FLAGS "${HDF_CONFIG_DIR}/gnu-warnings/gfort-7")
  #endif ()

  # Append more extra warning flags that only gcc 8.x+ knows about
  if (NOT CMAKE_Fortran_COMPILER_VERSION VERSION_LESS 8.0)
    ADD_H5_FLAGS (HDF5_CMAKE_Fortran_FLAGS "${HDF_CONFIG_DIR}/gnu-warnings/gfort-8")
  endif ()

  # Append more extra warning flags that only gcc 9.x+ knows about
  #if (NOT CMAKE_Fortran_COMPILER_VERSION VERSION_LESS 9.0)
  #  ADD_H5_FLAGS (HDF5_CMAKE_Fortran_FLAGS "${HDF_CONFIG_DIR}/gnu-warnings/gfort-9")
  #endif ()
endif ()

