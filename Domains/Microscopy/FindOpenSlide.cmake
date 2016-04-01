# A CMake find module for the OpenSlide microscopy file reader library.
#
# http://openslide.org
#
# Once done, this module will define
#   OPENSLIDE_FOUND         - system has OpenSlide
#   OPENSLIDE_INCLUDE_DIRS  - the OpenSlide include directory
#   OPENSLIDE_LIBRARIES     - link to these to use OpenSlide

# Look for the header.
find_path( OPENSLIDE_INCLUDE_DIR NAMES openslide.h PATHS
    /usr/local/include/openslide
    /usr/include/openslide)

mark_as_advanced( OPENSLIDE_INCLUDE_DIR )

# Look for the library.
find_library( OPENSLIDE_LIBRARY NAMES openslide )
mark_as_advanced( OPENSLIDE_LIBRARY )

# Handle the QUIETLY and REQUIRED arguments and set OPENSLIDE_FOUND true if all
# the listed variables are TRUE.

find_package( PackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( OPENSLIDE DEFAULT_MSG OPENSLIDE_LIBRARY OPENSLIDE_INCLUDE_DIR )

if( OPENSLIDE_FOUND )
  set( OPENSLIDE_LIBRARIES ${OPENSLIDE_LIBRARY} )
  set( OPENSLIDE_INCLUDE_DIRS ${OPENSLIDE_INCLUDE_DIR} )
endif()
