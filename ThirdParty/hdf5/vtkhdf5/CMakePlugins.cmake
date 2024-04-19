#
# Copyright by The HDF Group.
# All rights reserved.
#
# This file is part of HDF5.  The full HDF5 copyright notice, including
# terms governing use, modification, and redistribution, is contained in
# the COPYING file, which can be found at the root of the source code
# distribution tree, or in https://www.hdfgroup.org/licenses.
# If you do not have access to either file, you may request a copy from
# help@hdfgroup.org.
#

include (ExternalProject)
if (FALSE) # XXX(kitware): Hardcode settings.
#option (HDF5_ALLOW_EXTERNAL_SUPPORT "Allow External Library Building (NO GIT TGZ)" "NO")
set (HDF5_ALLOW_EXTERNAL_SUPPORT "NO" CACHE STRING "Allow External Library Building (NO GIT TGZ)")
set_property (CACHE HDF5_ALLOW_EXTERNAL_SUPPORT PROPERTY STRINGS NO GIT TGZ)
else ()
set(HDF5_ALLOW_EXTERNAL_SUPPORT "NO")
endif ()
if (HDF5_ALLOW_EXTERNAL_SUPPORT MATCHES "GIT" OR HDF5_ALLOW_EXTERNAL_SUPPORT MATCHES "TGZ")
  option (PLUGIN_USE_EXTERNAL "Use External Library Building for filter PLUGIN" 1)
  if (HDF5_ALLOW_EXTERNAL_SUPPORT MATCHES "GIT")
    set (PLUGIN_URL ${PLUGIN_GIT_URL} CACHE STRING "Path to PLUGIN git repository")
    set (PLUGIN_BRANCH ${PLUGIN_GIT_BRANCH})
  elseif (HDF5_ALLOW_EXTERNAL_SUPPORT MATCHES "TGZ")
    if (NOT TGZPATH)
      set (TGZPATH ${HDF5_SOURCE_DIR})
    endif ()
    set (PLUGIN_URL ${TGZPATH}/${PLUGIN_TGZ_NAME})
    if (NOT EXISTS "${PLUGIN_URL}")
      set (HDF5_ENABLE_PLUGIN_SUPPORT OFF CACHE BOOL "" FORCE)
      message (STATUS "Filter PLUGIN file ${PLUGIN_URL} not found")
    endif ()
  else ()
    set (PLUGIN_USE_EXTERNAL 0)
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option for PLUGIN support
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_ENABLE_PLUGIN_SUPPORT "Enable PLUGIN Filters" OFF)
else ()
set(HDF5_ENABLE_PLUGIN_SUPPORT OFF)
endif ()
if (HDF5_ENABLE_PLUGIN_SUPPORT)
  if (NOT PLUGIN_USE_EXTERNAL)
    find_package (PLUGIN NAMES ${PLUGIN_PACKAGE_NAME}${HDF_PACKAGE_EXT})
    if (NOT PLUGIN_FOUND)
      find_package (PLUGIN) # Legacy find
    endif ()
  endif ()
  if (NOT PLUGIN_FOUND)
    if (HDF5_ALLOW_EXTERNAL_SUPPORT MATCHES "GIT" OR HDF5_ALLOW_EXTERNAL_SUPPORT MATCHES "TGZ")
      EXTERNAL_PLUGIN_LIBRARY (${HDF5_ALLOW_EXTERNAL_SUPPORT})
      message (STATUS "Filter PLUGIN is built")
    else ()
      message (FATAL_ERROR " PLUGIN is Required for PLUGIN support in HDF5")
    endif ()
  endif ()
  message (STATUS "Filter PLUGIN is ON")
endif ()
