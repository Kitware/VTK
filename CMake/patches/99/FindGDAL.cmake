# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindGDAL
--------

Find GDAL.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``GDAL::GDAL``
if GDAL has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``GDAL_FOUND``
  True if GDAL is found.
``GDAL_INCLUDE_DIRS``
  Include directories for GDAL headers.
``GDAL_LIBRARIES``
  Libraries to link to GDAL.
``GDAL_VERSION``
  The version of GDAL found.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``GDAL_LIBRARY``
  The libgdal library file.
``GDAL_INCLUDE_DIR``
  The directory containing ``gdal.h``.

Hints
^^^^^

Set ``GDAL_DIR`` or ``GDAL_ROOT`` in the environment to specify the
GDAL installation prefix.
#]=======================================================================]

# $GDALDIR is an environment variable that would
# correspond to the ./configure --prefix=$GDAL_DIR
# used in building gdal.
#
# Created by Eric Wing. I'm not a gdal user, but OpenSceneGraph uses it
# for osgTerrain so I whipped this module together for completeness.
# I actually don't know the conventions or where files are typically
# placed in distros.
# Any real gdal users are encouraged to correct this (but please don't
# break the OS X framework stuff when doing so which is what usually seems
# to happen).

# This makes the presumption that you are include gdal.h like
#
#include "gdal.h"

find_path(GDAL_INCLUDE_DIR gdal.h
  HINTS
    ${GDAL_ROOT}
    ENV GDAL_DIR
    ENV GDAL_ROOT
  PATH_SUFFIXES
    include/gdal
    include/GDAL
    include
  DOC "Path to the GDAL include directory"
)
mark_as_advanced(GDAL_INCLUDE_DIR)

# GDAL name its library when built with CMake as `gdal${major}${minor}`.
set(_gdal_versions
    3.0 2.4 2.3 2.2 2.1 2.0 1.11 1.10 1.9 1.8 1.7 1.6 1.5 1.4 1.3 1.2)

set(_gdal_libnames)
foreach (_gdal_version IN LISTS _gdal_versions)
    string(REPLACE "." "" _gdal_version "${_gdal_version}")
    list(APPEND _gdal_libnames "gdal${_gdal_version}" "GDAL${_gdal_version}")
endforeach ()

find_library(GDAL_LIBRARY
  NAMES ${_gdal_libnames} gdal gdal_i gdal1.5.0 gdal1.4.0 gdal1.3.2 GDAL
  HINTS
    ${GDAL_ROOT}
    ENV GDAL_DIR
    ENV GDAL_ROOT
  DOC "Path to the GDAL library"
)
mark_as_advanced(GDAL_LIBRARY)

if (EXISTS "${GDAL_INCLUDE_DIR}/gdal_version.h")
    file(STRINGS "${GDAL_INCLUDE_DIR}/gdal_version.h" _gdal_version
        REGEX "GDAL_RELEASE_NAME")
    string(REGEX REPLACE ".*\"\(.*\)\"" "\\1" GDAL_VERSION "${_gdal_version}")
    unset(_gdal_version)
else ()
    set(GDAL_VERSION GDAL_VERSION-NOTFOUND)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GDAL
    VERSION_VAR GDAL_VERSION
    REQUIRED_VARS GDAL_LIBRARY GDAL_INCLUDE_DIR)

if (GDAL_FOUND)
    set(GDAL_LIBRARIES ${GDAL_LIBRARY})
    set(GDAL_INCLUDE_DIRS ${GDAL_INCLUDE_DIR})

    if (NOT TARGET GDAL::GDAL)
        add_library(GDAL::GDAL UNKNOWN IMPORTED)
        set_target_properties(GDAL::GDAL PROPERTIES
            IMPORTED_LOCATION "${GDAL_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${GDAL_INCLUDE_DIR}")
    endif ()
endif ()
