find_path(GL2PS_INCLUDE_DIR
  NAMES gl2ps.h
  DOC "gl2ps include directories")
mark_as_advanced(GL2PS_INCLUDE_DIR)

find_library(GL2PS_LIBRARY
  NAMES gl2ps
  DOC "gl2ps library")
mark_as_advanced(GL2PS_LIBRARY)

if (GL2PS_INCLUDE_DIR)
  file(STRINGS "${GL2PS_INCLUDE_DIR}/gl2ps.h" _gl2ps_version_lines REGEX "#define[ \t]+GL2PS_(MAJOR|MINOR|PATCH)_VERSION[ \t]+")
  string(REGEX REPLACE ".*GL2PS_MAJOR_VERSION *\([0-9]*\).*" "\\1" _gl2ps_version_major "${_gl2ps_version_lines}")
  string(REGEX REPLACE ".*GL2PS_MINOR_VERSION *\([0-9]*\).*" "\\1" _gl2ps_version_minor "${_gl2ps_version_lines}")
  string(REGEX REPLACE ".*GL2PS_PATCH_VERSION *\([0-9]*\).*" "\\1" _gl2ps_version_patch "${_gl2ps_version_lines}")
  set(GL2PS_VERSION "${_gl2ps_version_major}.${_gl2ps_version_minor}.${_gl2ps_version_patch}")
  unset(_gl2ps_version_major)
  unset(_gl2ps_version_minor)
  unset(_gl2ps_version_patch)
  unset(_gl2ps_version_lines)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GL2PS
  REQUIRED_VARS GL2PS_LIBRARY GL2PS_INCLUDE_DIR
  VERSION_VAR GL2PS_VERSION)

if (GL2PS_FOUND)
  set(GL2PS_INCLUDE_DIRS "${GL2PS_INCLUDE_DIR}")
  set(GL2PS_LIBRARIES "${GL2PS_LIBRARY}")

  if (NOT TARGET GL2PS::GL2PS)
    add_library(GL2PS::GL2PS UNKNOWN IMPORTED)
    set_target_properties(GL2PS::GL2PS PROPERTIES
      IMPORTED_LOCATION "${GL2PS_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${GL2PS_INCLUDE_DIR}")
  endif ()
endif ()
