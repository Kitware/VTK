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
option (PLUGIN_USE_EXTERNAL "Use External Library Building for filter PLUGIN else search" OFF)
option (PLUGIN_USE_LOCALCONTENT "Use local file for PLUGIN FetchContent" OFF)

if (NOT PLUGIN_USE_LOCALCONTENT)
  set (PLUGIN_URL ${PLUGIN_TGZ_ORIGPATH}/${PLUGIN_TGZ_NAME})
else ()
  if (NOT H5PL_TGZPATH)
    set (H5PL_TGZPATH ${TGZPATH})
  endif ()
  set (PLUGIN_URL ${H5PL_TGZPATH}/${PLUGIN_TGZ_NAME})
endif ()
#message (VERBOSE "Filter PLUGIN file is ${PLUGIN_URL}")

include (ExternalProject)
if (FALSE) # XXX(kitware): Hardcode settings.
#option (HDF5_ALLOW_EXTERNAL_SUPPORT "Allow External Library Building (NO GIT TGZ)" "NO")
set (HDF5_ALLOW_EXTERNAL_SUPPORT "NO" CACHE STRING "Allow External Library Building (NO GIT TGZ)")
set_property (CACHE HDF5_ALLOW_EXTERNAL_SUPPORT PROPERTY STRINGS NO GIT TGZ)
else ()
set(HDF5_ALLOW_EXTERNAL_SUPPORT "NO")
endif ()
if (HDF5_ALLOW_EXTERNAL_SUPPORT MATCHES "GIT" OR HDF5_ALLOW_EXTERNAL_SUPPORT MATCHES "TGZ")
  set (PLUGIN_USE_EXTERNAL ON CACHE BOOL "Use External Library Building for PLUGIN else search" FORCE)
  if (HDF5_ALLOW_EXTERNAL_SUPPORT MATCHES "GIT")
    set (PLUGIN_URL ${PLUGIN_GIT_URL} CACHE STRING "Path to PLUGIN git repository")
    set (PLUGIN_BRANCH ${PLUGIN_GIT_BRANCH})
  elseif (HDF5_ALLOW_EXTERNAL_SUPPORT MATCHES "TGZ")
    if (NOT H5PL_TGZPATH)
      set (H5PL_TGZPATH ${TGZPATH})
    endif ()
    if (PLUGIN_USE_LOCALCONTENT)
      if (NOT EXISTS "${PLUGIN_URL}")
        set (HDF5_ENABLE_PLUGIN_SUPPORT OFF CACHE BOOL "" FORCE)
        #message (VERBOSE "Filter PLUGIN file ${PLUGIN_URL} not found")
      endif ()
    endif ()
  else ()
    set (PLUGIN_USE_EXTERNAL OFF CACHE BOOL "Use External Library Building for PLUGIN else search")
    #message (VERBOSE "Filter PLUGIN not built")
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
  else ()
    if (HDF5_ALLOW_EXTERNAL_SUPPORT MATCHES "GIT" OR HDF5_ALLOW_EXTERNAL_SUPPORT MATCHES "TGZ")
      EXTERNAL_PLUGIN_LIBRARY (${HDF5_ALLOW_EXTERNAL_SUPPORT})
      message (STATUS "Filter PLUGIN is built")
    endif ()
  endif ()
  if (PLUGIN_FOUND)
    message (STATUS "Filter PLUGIN is ON")
  else ()
    set (HDF5_ENABLE_PLUGIN_SUPPORT OFF CACHE BOOL "" FORCE)
    message (FATAL_ERROR " PLUGIN support in HDF5 was enabled but not found")
  endif ()
endif ()
