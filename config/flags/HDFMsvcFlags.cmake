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
#  if (CMAKE_C_COMPILER_ID MATCHES "MSVC")
###############################################################################

#-----------------------------------------------------------------------------
# Compiler specific flags
#-----------------------------------------------------------------------------

# MSVC 14.28 enables C5105, but the Windows SDK 10.0.18362.0 triggers it.
if (NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 19.28)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -wd5105")
endif ()

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64" AND ${HDF_CFG_NAME} MATCHES "Debug")
  set (WIN_COMPILE_FLAGS "${WIN_COMPILE_FLAGS} /Gy")
endif ()
