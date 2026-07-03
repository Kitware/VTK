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
#  if (CMAKE_Fortran_COMPILER_ID MATCHES "NVHPC" )
###############################################################################

message (VERBOSE "Warnings Configuration: default Fortran: ${CMAKE_Fortran_FLAGS}")

#-----------------------------------------------------------------------------
# HDF5 library compile options - to be made available to all targets
#-----------------------------------------------------------------------------
if (CMAKE_Fortran_COMPILER_ID STREQUAL "NVHPC")
  if (NOT ${HDF_CFG_NAME} MATCHES "Debug" AND NOT ${HDF_CFG_NAME} MATCHES "Developer")
    set (CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Mnoframe")
    if (NOT ${HDF_CFG_NAME} MATCHES "RelWithDebInfo")
      set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -s")
    endif ()
  else ()
    set (CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Mbounds -Mchkptr -Mdclchk -g")
  endif ()
endif ()

if (NOT MSVC AND NOT MINGW)
  # General flags
  message (VERBOSE "HDF5_CMAKE_Fortran_FLAGS=${HDF5_CMAKE_Fortran_FLAGS}")
endif ()

