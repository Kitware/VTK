find_path(LZMA_INCLUDE_DIR
  NAMES lzma.h
  DOC "lzma include directory")
find_library(LZMA_LIBRARY
  NAMES lzma liblzma
  DOC "lzma library")

if (LZMA_INCLUDE_DIR)
  file(STRINGS "${LZMA_INCLUDE_DIR}/lzma/version.h" _lzma_version_lines
    REGEX "#define[ \t]+LZMA_VERSION_(MAJOR|MINOR|PATCH)")
  string(REGEX REPLACE ".*LZMA_VERSION_MAJOR *\([0-9]*\).*" "\\1" _lzma_version_major "${_lzma_version_lines}")
  string(REGEX REPLACE ".*LZMA_VERSION_MINOR *\([0-9]*\).*" "\\1" _lzma_version_minor "${_lzma_version_lines}")
  string(REGEX REPLACE ".*LZMA_VERSION_PATCH *\([0-9]*\).*" "\\1" _lzma_version_patch "${_lzma_version_lines}")
  set(LZMA_VERSION "${_lzma_version_major}.${_lzma_version_minor}.${_lzma_version_patch}")
  unset(_lzma_version_major)
  unset(_lzma_version_minor)
  unset(_lzma_version_patch)
  unset(_lzma_version_lines)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LZMA
  REQUIRED_VARS LZMA_LIBRARY LZMA_INCLUDE_DIR
  VERSION_VAR LZMA_VERSION)

if (LZMA_FOUND)
  set(LZMA_LIBRARIES "${LZMA_LIBRARY}")
  set(LZMA_INCLUDE_DIRS "${LZMA_INCLUDE_DIR}")

  if (NOT TARGET LZMA::LZMA)
    include(vtkDetectLibraryType)
    vtk_detect_library_type(lzma_library_type
      PATH "${LZMA_LIBRARY}")
    add_library(LZMA::LZMA "${lzma_library_type}" IMPORTED)
    unset(lzma_library_type)
    set_target_properties(LZMA::LZMA PROPERTIES
      IMPORTED_LOCATION "${LZMA_LIBRARY}"
      IMPORTED_IMPLIB "${LZMA_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${LZMA_INCLUDE_DIR}")
  endif ()
endif ()
