# A CMake find module for the OpenSlide microscopy file reader library.
#
# http://openslide.org
#
# Once done, this module will define
#   OPENSLIDE_FOUND         - system has OpenSlide
#   OPENSLIDE_INCLUDE_DIRS  - the OpenSlide include directory
#   OPENSLIDE_LIBRARIES     - link to these to use OpenSlide
#   OpenSlide::OpenSlide    - imported target

# Look for the header.
find_path(OPENSLIDE_INCLUDE_DIR
  NAMES
    openslide/openslide.h
  PATHS
    /usr/local/include
    /usr/include
  DOC "OpenSlide include directory")
mark_as_advanced(OPENSLIDE_INCLUDE_DIR)

# Look for the library.
find_library(OPENSLIDE_LIBRARY
  NAMES openslide
  DOC "OpenSlide library")
mark_as_advanced(OPENSLIDE_LIBRARY)

find_package(PackageHandleStandardArgs)
find_package_handle_standard_args(OpenSlide
  REQUIRED_VARS OPENSLIDE_LIBRARY OPENSLIDE_INCLUDE_DIR)

if (OPENSLIDE_FOUND)
  set(OPENSLIDE_LIBRARIES "${OPENSLIDE_LIBRARY}")
  set(OPENSLIDE_INCLUDE_DIRS "${OPENSLIDE_INCLUDE_DIR}")
  if (NOT TARGET OpenSlide::OpenSlide)
    add_library(OpenSlide::OpenSlide UNKNOWN IMPORTED)
    set_target_properties(OpenSlide::OpenSlide PROPERTIES
      IMPORTED_LOCATION "${OPENSLIDE_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${OPENSLIDE_INCLUDE_DIR}")
  endif ()
endif ()
