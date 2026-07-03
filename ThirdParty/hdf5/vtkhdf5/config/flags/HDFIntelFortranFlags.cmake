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
#  if (CMAKE_Fortran_COMPILER_ID MATCHES "Intel" )
###############################################################################

message (VERBOSE "Warnings Configuration: default Fortran: ${CMAKE_Fortran_FLAGS}")

#-----------------------------------------------------------------------------
# Option to allow the user to disable compiler warnings
#-----------------------------------------------------------------------------
if (HDF5_DISABLE_COMPILER_WARNINGS)
  # MSVC uses /w to suppress warnings.  It also complains if another
  # warning level is given, so remove it.
  if (MSVC)
    if (CMAKE_Fortran_COMPILER_ID STREQUAL "Intel")
      set (CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} /warn:none")
    elseif (CMAKE_Fortran_COMPILER_ID MATCHES "IntelLLVM")
      set (CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} /warn:none")
    endif ()
  endif ()
endif ()

#-----------------------------------------------------------------------------
# HDF5 library compile options - to be made available to all targets
#-----------------------------------------------------------------------------
if (NOT MSVC AND NOT MINGW)
  # General flags
  if (CMAKE_Fortran_COMPILER_ID STREQUAL "Intel")
    if (_INTEL_WINDOWS)
      ADD_H5_FLAGS (HDF5_CMAKE_Fortran_FLAGS "${HDF_CONFIG_DIR}/intel-warnings/classic/win-ifort-general")
    else ()
      ADD_H5_FLAGS (HDF5_CMAKE_Fortran_FLAGS "${HDF_CONFIG_DIR}/intel-warnings/classic/ifort-general")
    endif()
    list (APPEND HDF5_CMAKE_Fortran_FLAGS "-free")
  elseif (CMAKE_Fortran_COMPILER_ID MATCHES "IntelLLVM")
    if (_INTEL_WINDOWS)
      ADD_H5_FLAGS (HDF5_CMAKE_Fortran_FLAGS "${HDF_CONFIG_DIR}/intel-warnings/oneapi/win-ifort-general")
    else ()
      ADD_H5_FLAGS (HDF5_CMAKE_Fortran_FLAGS "${HDF_CONFIG_DIR}/intel-warnings/oneapi/ifort-general")
    endif()
    list (APPEND HDF5_CMAKE_Fortran_FLAGS "-free")
  endif ()
  message (VERBOSE "HDF5_CMAKE_Fortran_FLAGS=${HDF5_CMAKE_Fortran_FLAGS}")
else ()
  if (CMAKE_Fortran_COMPILER_ID STREQUAL "Intel")
    ADD_H5_FLAGS (HDF5_CMAKE_Fortran_FLAGS "${HDF_CONFIG_DIR}/intel-warnings/classic/win-ifort-general")
    list (APPEND HDF5_CMAKE_Fortran_FLAGS "/stand:f03" "/free")
  elseif (CMAKE_Fortran_COMPILER_ID MATCHES "IntelLLVM")
    ADD_H5_FLAGS (HDF5_CMAKE_Fortran_FLAGS "${HDF_CONFIG_DIR}/intel-warnings/oneapi/win-ifort-general")
    list (APPEND HDF5_CMAKE_Fortran_FLAGS "/stand:f03" "/free")
  endif ()
endif ()

