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
#  if (CMAKE_CXX_COMPILER_ID MATCHES "NVHPC" )
###############################################################################

#-----------------------------------------------------------------------------
# Compiler specific flags
#-----------------------------------------------------------------------------
if (NOT DEFINED CMAKE_CXX${CMAKE_CXX_STANDARD}_STANDARD_COMPILE_OPTION)
  if (NOT CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD EQUAL 11)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C11_STANDARD_COMPILE_OPTION}")
  endif ()
endif ()
if (NOT ${HDF_CFG_NAME} MATCHES "Debug" AND NOT ${HDF_CFG_NAME} MATCHES "Developer")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Minform=warn")
  if (NOT ${HDF_CFG_NAME} MATCHES "RelWithDebInfo")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s")
  endif ()
else ()
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Mbounds -gopt -g")
endif ()
