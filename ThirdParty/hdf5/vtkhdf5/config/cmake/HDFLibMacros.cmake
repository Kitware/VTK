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
# HDFLibMacros.cmake - Macros for configuring external ZLIB and SZIP libraries for HDF5
#
# This file provides macros to automate the configuration, fetching, and build
# of external ZLIB and SZIP (libaec) libraries for HDF5, supporting both GIT and TGZ sources.
# It sets up include directories, library variables, and handles patching of CMakeLists.txt
# for custom build requirements. These macros are used to ensure consistent integration
# of compression libraries in HDF5 builds.

#-------------------------------------------------------------------------------
macro (EXTERNAL_ZLIB_LIBRARY compress_type)
  # Select the correct folder for ZLIB or ZLIB-NG
  if (HDF5_USE_ZLIB_NG)
    set (zlib_folder "ZLIBNG")
  else ()
    set (zlib_folder "ZLIB")
  endif ()
  # Handle GIT or TGZ source for ZLIB
  if (${compress_type} MATCHES "GIT")
    # Use a different CMakeLists for 'develop' branch
    if (${ZLIB_BRANCH} MATCHES "develop")
      set (ZLIB_FILE "devCMakeLists")
    else ()
      set (ZLIB_FILE "CMakeLists")
    endif ()
    message (STATUS "Filter ZLIB file ${ZLIB_URL}")
    # Fetch ZLIB from GIT and patch CMakeLists.txt
    FetchContent_Declare (HDF5_ZLIB
        GIT_REPOSITORY ${ZLIB_URL}
        GIT_TAG ${ZLIB_BRANCH}
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy
            ${HDF_RESOURCES_DIR}/${zlib_folder}/${ZLIB_FILE}.txt
            <SOURCE_DIR>/CMakeLists.txt
    )
  elseif (${compress_type} MATCHES "TGZ")
    # message (VERBOSE "Filter ZLIB file ${ZLIB_URL}")
    # Fetch ZLIB from TGZ and patch CMakeLists.txt
    FetchContent_Declare (HDF5_ZLIB
        URL ${ZLIB_URL}
        URL_HASH ""
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy
            ${HDF_RESOURCES_DIR}/${zlib_folder}/CMakeLists.txt
            <SOURCE_DIR>/CMakeLists.txt
    )
  endif ()

  # Make ZLIB available for the build
  FetchContent_MakeAvailable(HDF5_ZLIB)

  # Optionally add namespace alias for static zlib
  if (HDF_PACKAGE_NAMESPACE)
    add_library(${HDF_PACKAGE_NAMESPACE}zlib-static ALIAS zlib-static)
  endif ()
  set (H5_ZLIB_STATIC_LIBRARY "${HDF_PACKAGE_NAMESPACE}zlib-static")
  set (H5_ZLIB_LIBRARIES ${H5_ZLIB_STATIC_LIBRARY})
  # Set the correct header for zlib-ng compatibility
  if (HDF5_USE_ZLIB_NG)
    if (ZLIB_COMPAT)
      set (H5_ZLIB_HEADER "zlib.h")
    else ()
      set (H5_ZLIB_HEADER "zlib-ng.h")
    endif ()
  else ()
    set (H5_ZLIB_HEADER "zlib.h")
  endif ()

  # Set include directories for generated and source headers
  set (H5_ZLIB_INCLUDE_DIR_GEN "${hdf5_zlib_BINARY_DIR}")
  set (H5_ZLIB_INCLUDE_DIR "${hdf5_zlib_SOURCE_DIR}")
  set (H5_ZLIB_FOUND 1)
  set (H5_ZLIB_INCLUDE_DIRS ${H5_ZLIB_INCLUDE_DIR_GEN} ${H5_ZLIB_INCLUDE_DIR})
endmacro ()

#-------------------------------------------------------------------------------
macro (EXTERNAL_SZIP_LIBRARY compress_type encoding)
  # Only libaec library is usable for SZIP
  if (${compress_type} MATCHES "GIT")
    # Fetch libaec from GIT and patch CMakeLists.txt
    FetchContent_Declare (SZIP
        GIT_REPOSITORY ${SZIP_URL}
        GIT_TAG ${SZIP_BRANCH}
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy
            ${HDF_RESOURCES_DIR}/LIBAEC/CMakeLists.txt
            <SOURCE_DIR>/CMakeLists.txt
    )
  elseif (${compress_type} MATCHES "TGZ")
    #message (VERBOSE "Filter SZIP file ${SZIP_URL}")
    # Fetch libaec from TGZ and patch CMakeLists.txt
    FetchContent_Declare (SZIP
        URL ${SZIP_URL}
        URL_HASH ""
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy
            ${HDF_RESOURCES_DIR}/LIBAEC/CMakeLists.txt
            <SOURCE_DIR>/CMakeLists.txt
    )
  endif ()
  # Make SZIP (libaec) available for the build
  FetchContent_MakeAvailable(SZIP)

  # Optionally add namespace aliases for static szaec and aec
  if (HDF_PACKAGE_NAMESPACE)
    add_library (${HDF_PACKAGE_NAMESPACE}szaec-static ALIAS szaec-static)
    add_library (${HDF_PACKAGE_NAMESPACE}aec-static ALIAS aec-static)
  endif ()
  set (H5_SZIP_STATIC_LIBRARY "${HDF_PACKAGE_NAMESPACE}szaec-static;${HDF_PACKAGE_NAMESPACE}aec-static")
  set (H5_SZIP_LIBRARIES ${H5_SZIP_STATIC_LIBRARY})

  # Set include directories for generated and source headers
  set (H5_SZIP_INCLUDE_DIR_GEN "${szip_BINARY_DIR}")
  set (H5_SZIP_INCLUDE_DIR "${szip_SOURCE_DIR}/include")
  set (H5_SZIP_FOUND 1)
  set (H5_SZIP_INCLUDE_DIRS ${H5_SZIP_INCLUDE_DIR_GEN} ${H5_SZIP_INCLUDE_DIR})
endmacro ()
