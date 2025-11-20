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
macro (EXTERNAL_ZLIB_LIBRARY compress_type)
  if (HDF5_USE_ZLIB_NG)
    set (zlib_folder "ZLIBNG")
  else ()
    set (zlib_folder "ZLIB")
  endif ()
  if (${compress_type} MATCHES "GIT")
    FetchContent_Declare (HDF5_ZLIB
        GIT_REPOSITORY ${ZLIB_URL}
        GIT_TAG ${ZLIB_BRANCH}
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy
            ${HDF_RESOURCES_DIR}/${zlib_folder}/CMakeLists.txt
            <SOURCE_DIR>/CMakeLists.txt
    )
  elseif (${compress_type} MATCHES "TGZ")
    #message (VERBOSE "Filter ZLIB file ${ZLIB_URL}")
    FetchContent_Declare (HDF5_ZLIB
        URL ${ZLIB_URL}
        URL_HASH ""
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy
            ${HDF_RESOURCES_DIR}/${zlib_folder}/CMakeLists.txt
            <SOURCE_DIR>/CMakeLists.txt
    )
  endif ()

  FetchContent_MakeAvailable(HDF5_ZLIB)

  add_library(${HDF_PACKAGE_NAMESPACE}zlib-static ALIAS zlib-static)
  set (H5_ZLIB_STATIC_LIBRARY "${HDF_PACKAGE_NAMESPACE}zlib-static")
  set (H5_ZLIB_LIBRARIES ${H5_ZLIB_STATIC_LIBRARY})
  if (HDF5_USE_ZLIB_NG)
    set (H5_ZLIB_HEADER "zlib-ng.h")
  else ()
    set (H5_ZLIB_HEADER "zlib.h")
  endif ()

  set (H5_ZLIB_INCLUDE_DIR_GEN "${hdf5_zlib_BINARY_DIR}")
  set (H5_ZLIB_INCLUDE_DIR "${hdf5_zlib_SOURCE_DIR}")
  set (H5_ZLIB_FOUND 1)
  set (H5_ZLIB_INCLUDE_DIRS ${H5_ZLIB_INCLUDE_DIR_GEN} ${H5_ZLIB_INCLUDE_DIR})
endmacro ()

#-------------------------------------------------------------------------------
macro (EXTERNAL_SZIP_LIBRARY compress_type encoding)
  # Only libaec library is usable
  if (${compress_type} MATCHES "GIT")
    FetchContent_Declare (SZIP
        GIT_REPOSITORY ${SZIP_URL}
        GIT_TAG ${SZIP_BRANCH}
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy
            ${HDF_RESOURCES_DIR}/LIBAEC/CMakeLists.txt
            <SOURCE_DIR>/CMakeLists.txt
    )
  elseif (${compress_type} MATCHES "TGZ")
    #message (VERBOSE "Filter SZIP file ${SZIP_URL}")
    FetchContent_Declare (SZIP
        URL ${SZIP_URL}
        URL_HASH ""
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy
            ${HDF_RESOURCES_DIR}/LIBAEC/CMakeLists.txt
            <SOURCE_DIR>/CMakeLists.txt
    )
  endif ()
  FetchContent_MakeAvailable(SZIP)

  add_library (${HDF_PACKAGE_NAMESPACE}szaec-static ALIAS szaec-static)
  add_library (${HDF_PACKAGE_NAMESPACE}aec-static ALIAS aec-static)
  set (H5_SZIP_STATIC_LIBRARY "${HDF_PACKAGE_NAMESPACE}szaec-static;${HDF_PACKAGE_NAMESPACE}aec-static")
  set (H5_SZIP_LIBRARIES ${H5_SZIP_STATIC_LIBRARY})

  set (H5_SZIP_INCLUDE_DIR_GEN "${szip_BINARY_DIR}")
  set (H5_SZIP_INCLUDE_DIR "${szip_SOURCE_DIR}/include")
  set (H5_SZIP_FOUND 1)
  set (H5_SZIP_INCLUDE_DIRS ${H5_SZIP_INCLUDE_DIR_GEN} ${H5_SZIP_INCLUDE_DIR})
endmacro ()
