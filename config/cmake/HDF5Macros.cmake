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
#-------------------------------------------------------------------------------
macro (H5_SET_LIB_OPTIONS libtarget libname libtype libpackage)
  set (LIB_OUT_NAME "${libname}")
  # SOVERSION passed in ARGN when shared
  if (${libtype} MATCHES "SHARED")
    set (PACKAGE_SOVERSION ${HDF5_${libpackage}_PACKAGE_SOVERSION})
    if (WIN32)
      set (LIBHDF_VERSION ${HDF5_PACKAGE_VERSION_MAJOR})
    else ()
      set (LIBHDF_VERSION ${HDF5_${libpackage}_PACKAGE_SOVERSION_MAJOR})
    endif ()
    set_target_properties (${libtarget} PROPERTIES VERSION ${PACKAGE_SOVERSION})
    if (WIN32)
        set (${LIB_OUT_NAME} "${LIB_OUT_NAME}-${LIBHDF_VERSION}")
    else ()
        set_target_properties (${libtarget} PROPERTIES SOVERSION ${LIBHDF_VERSION})
    endif ()
  endif ()
  HDF_SET_LIB_OPTIONS (${libtarget} ${LIB_OUT_NAME} ${libtype})

  #-- Apple Specific install_name for libraries
  if (APPLE)
    option (HDF5_BUILD_WITH_INSTALL_NAME "Build with library install_name set to the installation path" OFF)
    if (HDF5_BUILD_WITH_INSTALL_NAME)
      set_property(TARGET ${libtarget} APPEND PROPERTY
          LINK_FLAGS "-current_version ${HDF5_PACKAGE_VERSION} -compatibility_version ${HDF5_PACKAGE_VERSION}"
      )
      set_target_properties (${libtarget} PROPERTIES
          INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
          BUILD_WITH_INSTALL_RPATH ${HDF5_BUILD_WITH_INSTALL_NAME}
      )
    endif ()
    if (HDF5_BUILD_FRAMEWORKS)
      if (${libtype} MATCHES "SHARED")
        # adapt target to build frameworks instead of dylibs
        set_target_properties(${libtarget} PROPERTIES
            XCODE_ATTRIBUTE_INSTALL_PATH "@rpath"
            FRAMEWORK TRUE
            FRAMEWORK_VERSION ${HDF5_PACKAGE_VERSION_MAJOR}
            MACOSX_FRAMEWORK_IDENTIFIER org.hdfgroup.${libtarget}
            MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${HDF5_PACKAGE_VERSION_MAJOR}
            MACOSX_FRAMEWORK_BUNDLE_VERSION ${HDF5_PACKAGE_VERSION_MAJOR})
      endif ()
    endif ()
  endif ()

endmacro ()
