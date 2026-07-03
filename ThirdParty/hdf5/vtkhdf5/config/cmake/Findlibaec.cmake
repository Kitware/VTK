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
#########################################################################

# CMake module to find the Adaptive Entropy Coding (libaec) library on
# the system, preferring to make use of a CMake configuration file for
# the library if one is found. Derived from the FindCURL.cmake file that
# is included with CMake.
#
# Imported Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines the following `IMPORTED` targets:
#
# ``libaec::aec``
#   The libaec library, if found.
#
# ``libaec::sz``
#   The sz compatibility library, if found.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ``libaec_FOUND``
#   "True" if ``libaec`` is found.
#
# ``libsz_FOUND``
#   "True" if the ``libsz`` compatibility library is found.
#
# ``libaec_INCLUDE_DIRS``
#   The libaec include directories.
#
# ``libsz_INCLUDE_DIRS``
#   The libsz include directories.
#
# ``libaec_LIBRARIES``
#   The libraries to link against when using libaec.
#
# ``libsz_LIBRARIES``
#   The libraries to link against when using libsz.
#
# ``libaec_VERSION``
#   The version of libaec found. May not be set if a version number
#   can't be parsed from libaec.h or a pkg-config file.
#
# ``libsz_VERSION``
#   The version of libsz found. May not be set if a version number
#   can't be parsed from szlib.h or a pkg-config file.
#
# Hints
# ^^^^^
#
# Set ``libaec_ROOT`` to a directory which contains a libaec installation.
# If it is known that the libaec installation contains a libaec-config.cmake
# file, instead set ``libaec_DIR`` to the directory which contains that file.
#

include (FindPackageHandleStandardArgs)

# First check to see if libaec was built with CMake and has a libaec-config.cmake
# file available for use
if (DEFINED libaec_DIR AND NOT libaec_DIR STREQUAL "libaec_DIR-NOTFOUND")
  message (VERBOSE "Looking for libaec CMake configuration file at ${libaec_DIR}")
else ()
  message (VERBOSE "Looking for libaec CMake configuration file")
endif ()
find_package (libaec QUIET NO_MODULE)
mark_as_advanced (libaec_DIR)

# If a libaec-config.cmake file is available for use, prefer that
if (libaec_FOUND)
  find_package_handle_standard_args (libaec HANDLE_COMPONENTS CONFIG_MODE)
  message (VERBOSE "Found existing libaec CMake configuration file at ${libaec_DIR}")

  # Set variables that this module returns
  if (TARGET libaec::aec)
    set (libaec_LIBRARIES "libaec::aec")

    # Determine libaec include directory
    get_target_property (libaec_include_dir_prop libaec::aec INTERFACE_INCLUDE_DIRECTORIES)
    if (libaec_include_dir_prop)
      list (GET libaec_include_dir_prop 0 libaec_INCLUDE_DIRS)
    else ()
      # INTERFACE_INCLUDE_DIRECTORIES property may not be set directly on libaec::aec
      # target, but rather on a library set in its INTERFACE_LINK_LIBRARIES property.
      get_target_property (libaec_link_libs_prop libaec::aec INTERFACE_LINK_LIBRARIES)
      if (libaec_link_libs_prop)
        list (GET libaec_link_libs_prop 0 libaec_link_lib_0)
        get_target_property (libaec_include_dir_prop ${libaec_link_lib_0} INTERFACE_INCLUDE_DIRECTORIES)
        if (libaec_include_dir_prop)
          list (GET libaec_include_dir_prop 0 libaec_INCLUDE_DIRS)
        endif ()
      endif ()
    endif ()
  endif ()

  if (SZIP_FOUND)
    set (libsz_FOUND TRUE)

    if (TARGET libaec::sz)
      set (libsz_LIBRARIES "libaec::sz")
    endif ()

    set (libsz_INCLUDE_DIRS "${SZIP_INCLUDE_DIR}")
    set (libsz_VERSION "${SZIP_VERSION}")
  endif ()

  return ()
elseif (DEFINED libaec_NOT_FOUND_MESSAGE)
  message (VERBOSE "Couldn't load libaec from CMake configuration file: ${libaec_NOT_FOUND_MESSAGE}")
endif ()

# Try to find a libaec pkg-config file
find_package (PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
  pkg_check_modules (PC_LIBAEC QUIET libaec)
endif ()

# Find the libaec.h header file
find_path (libaec_INCLUDE_DIR
  NAMES libaec.h
  HINTS ${libaec_ROOT} ${PC_LIBAEC_INCLUDE_DIRS}
  DOC "Path to the libaec.h header file"
)
mark_as_advanced (libaec_INCLUDE_DIR)

# Find the szlib.h header file
find_path (libsz_INCLUDE_DIR
  NAMES szlib.h
  HINTS ${libaec_ROOT} ${PC_LIBAEC_INCLUDE_DIRS}
  DOC "Path to the szlib.h header file"
)
mark_as_advanced (libsz_INCLUDE_DIR)

# Find the libaec library
find_library (libaec_LIBRARY
  NAMES aec libaec libaec.a
  HINTS ${libaec_ROOT} ${PC_LIBAEC_LIBRARY_DIRS}
  PATH_SUFFIXES lib lib64
  DOC "Path to the libaec library file"
)
mark_as_advanced (libaec_LIBRARY)

# Find the libsz library
find_library (libsz_LIBRARY
  NAMES sz libsz libsz.a
  HINTS ${libaec_ROOT} ${PC_LIBAEC_LIBRARY_DIRS}
  PATH_SUFFIXES lib lib64
  DOC "Path to the libsz library file"
)
mark_as_advanced (libsz_LIBRARY)

# Determine libaec version from libaec.h or from pkg-config file
if (EXISTS "${libaec_INCLUDE_DIR}/libaec.h")
  file (STRINGS "${libaec_INCLUDE_DIR}/libaec.h" libaec_version_str REGEX "^#define[\t ]+AEC_VERSION_STR[\t ]+\".*\"")
  if (libaec_version_str)
    string (REGEX REPLACE "^#define[\t ]+AEC_VERSION_STR[\t ]+\"([^\"]*)\".*" "\\1" libaec_VERSION "${libaec_version_str}")
  endif ()
elseif (PC_LIBAEC_FOUND)
  set (libaec_VERSION ${PC_LIBAEC_VERSION})
endif ()

# Set variables for whether libaec and libsz were found
find_package_handle_standard_args (libaec
  REQUIRED_VARS libaec_LIBRARY libaec_INCLUDE_DIR
  VERSION_VAR libaec_VERSION
  HANDLE_COMPONENTS
)

if (libsz_LIBRARY AND libsz_INCLUDE_DIR)
  set (libsz_FOUND TRUE)
endif ()

# Created imported targets and set remaining variables for module
if (libaec_FOUND)
  if (NOT TARGET libaec::aec)
    add_library(libaec::aec UNKNOWN IMPORTED)

    set_target_properties(libaec::aec PROPERTIES
      IMPORTED_LOCATION "${libaec_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${libaec_INCLUDE_DIR}"
    )
  endif ()

  set (libaec_INCLUDE_DIRS ${libaec_INCLUDE_DIR})
  set (libaec_LIBRARIES ${libaec_LIBRARY})
endif ()

if (libsz_FOUND)
  if (NOT TARGET libaec::sz)
    add_library(libaec::sz UNKNOWN IMPORTED)

    set_target_properties(libaec::sz PROPERTIES
      IMPORTED_LOCATION "${libsz_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${libsz_INCLUDE_DIR}"
    )
  endif ()

  set (libsz_INCLUDE_DIRS ${libsz_INCLUDE_DIR})
  set (libsz_LIBRARIES ${libsz_LIBRARY})
endif ()
