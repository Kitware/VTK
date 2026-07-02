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
#  if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
###############################################################################

#-----------------------------------------------------------------------------
# Compiler specific flags
#-----------------------------------------------------------------------------

# MSVC 14.28 enables C5105, but the Windows SDK 10.0.18362.0 triggers it.
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.28)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd5105")
  endif ()
