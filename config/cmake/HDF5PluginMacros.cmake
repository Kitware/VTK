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
#-------------------------------------------------------------------------------
# Plugins must be built SHARED
#-------------------------------------------------------------------------------
macro (EXTERNAL_PLUGIN_LIBRARY compress_type)
  if (${compress_type} MATCHES "GIT")
    FetchContent_Declare (PLUGIN
        GIT_REPOSITORY ${PLUGIN_URL}
        GIT_TAG ${PLUGIN_BRANCH}
    )
  elseif (${compress_type} MATCHES "TGZ")
    FetchContent_Declare (PLUGIN
        URL ${PLUGIN_URL}
        URL_HASH ""
    )
  endif ()
  include (${HDF_RESOURCES_DIR}/HDF5PluginCache.cmake)
  set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
  FetchContent_MakeAvailable(PLUGIN)
  set (PLUGIN_BINARY_DIR "${plugin_BINARY_DIR}")
  set (PLUGIN_SOURCE_DIR "${plugin_SOURCE_DIR}")
  set (PLUGIN_LIBRARY "PLUGIN")
  set (PLUGIN_FOUND 1)
endmacro ()

#-------------------------------------------------------------------------------
macro (FILTER_OPTION plname)
  string(TOLOWER ${plname} PLUGIN_NAME)
  option (ENABLE_${plname} "Enable Library Building for ${plname} plugin" ON)
  if (ENABLE_${plname})
    option (HDF_${plname}_USE_EXTERNAL "Use External Library Building for ${PLUGIN_NAME} plugin else search" OFF)
    mark_as_advanced (HDF_${plname}_USE_EXTERNAL)
    if (H5PL_ALLOW_EXTERNAL_SUPPORT MATCHES "GIT" OR H5PL_ALLOW_EXTERNAL_SUPPORT MATCHES "TGZ")
      set (HDF_${plname}_USE_EXTERNAL ON CACHE BOOL "Use External Library Building for ${PLUGIN_NAME} plugin" FORCE)
      if (H5PL_ALLOW_EXTERNAL_SUPPORT MATCHES "GIT")
        set (HDF_${plname}_URL ${HDF_${plname}_GIT_URL})
        set (HDF_${plname}_BRANCH ${HDF_${plname}_GIT_BRANCH})
      elseif (H5PL_ALLOW_EXTERNAL_SUPPORT MATCHES "TGZ")
        if (NOT H5PL_COMP_TGZPATH)
          set (H5PL_COMP_TGZPATH ${H5PL_SOURCE_DIR}/libs)
        endif ()
        set (HDF_${plname}_URL ${H5PL_COMP_TGZPATH}/${HDF_${plname}_TGZ_NAME})
      endif ()
    endif ()
    add_subdirectory (${plname})
    set_global_variable (H5PL_LIBRARIES_TO_EXPORT "${H5PL_LIBRARIES_TO_EXPORT};${H5${plname}_LIBRARIES_TO_EXPORT}")
  endif ()
endmacro ()

#-------------------------------------------------------------------------------
macro (PACKAGE_PLUGIN_LIBRARY compress_type)
  if (${compress_type} MATCHES "GIT" OR ${compress_type} MATCHES "TGZ")
    #message (VERBOSE "Filter PLUGIN is to be packaged")
  endif ()
endmacro ()
