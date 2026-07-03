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
#  if (CMAKE_C_COMPILER_ID MATCHES "NVHPC" )
###############################################################################

#-----------------------------------------------------------------------------
# Compiler specific flags
#-----------------------------------------------------------------------------
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Minform=warn")
if (NOT ${HDF_CFG_NAME} MATCHES "Debug" AND NOT ${HDF_CFG_NAME} MATCHES "Developer")
  if (NOT ${HDF_CFG_NAME} MATCHES "RelWithDebInfo")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s")
  endif ()
else ()
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Mbounds -gopt")
endif ()
