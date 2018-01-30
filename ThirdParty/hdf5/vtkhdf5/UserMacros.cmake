#
# Copyright by The HDF Group.
# All rights reserved.
#
# This file is part of HDF5.  The full HDF5 copyright notice, including
# terms governing use, modification, and redistribution, is contained in
# the COPYING file, which can be found at the root of the source code
# distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.
# If you do not have access to either file, you may request a copy from
# help@hdfgroup.org.
#
########################################################
#  Include file for user options
########################################################

#-----------------------------------------------------------------------------
#------------------- E X A M P L E   B E G I N--------------------------------
#-----------------------------------------------------------------------------
# Option to Build with User Defined Values
#-----------------------------------------------------------------------------
macro (MACRO_USER_DEFINED_LIBS)
  set (USER_DEFINED_VALUE "FALSE")
endmacro ()

#-------------------------------------------------------------------------------
if (FALSE) # XXX(kitware): hardcode settings
option (BUILD_USER_DEFINED_LIBS "Build With User Defined Values" OFF)
else ()
set(BUILD_USER_DEFINED_LIBS OFF)
endif ()
if (BUILD_USER_DEFINED_LIBS)
  MACRO_USER_DEFINED_LIBS ()
endif ()
#-----------------------------------------------------------------------------
#------------------- E X A M P L E   E N D -----------------------------------
#-----------------------------------------------------------------------------
