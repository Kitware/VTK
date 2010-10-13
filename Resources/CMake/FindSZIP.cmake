
# - Find SZIP library
# - Derived from the FindTiff.cmake that is included with cmake
# Find the native SZIP includes and library
# This module defines
#  SZIP_INCLUDE_DIRS, where to find tiff.h, etc.
#  SZIP_LIBRARIES, libraries to link against to use SZIP.
#  SZIP_FOUND, If false, do not try to use SZIP.
#    also defined, but not for general use are
#  SZIP_LIBRARY, where to find the SZIP library.
#  SZIP_LIBRARY_DEBUG - Debug version of tiff library
#  SZIP_LIBRARY_RELEASE - Release Version of tiff library

# MESSAGE (STATUS "Finding Szip library and headers..." )

############################################
#
# Check the existence of the libraries.
#
############################################
# This macro was taken directly from the FindQt4.cmake file that is included
# with the CMake distribution. This is NOT my work. All work was done by the
# original authors of the FindQt4.cmake file. Only minor modifications were
# made to remove references to Qt and make this file more generally applicable
#########################################################################

MACRO (SZIP_ADJUST_LIB_VARS basename)
  IF (${basename}_INCLUDE_DIR)

    # if only the release version was found, set the debug variable also to the release version
    IF (${basename}_LIBRARY_RELEASE AND NOT ${basename}_LIBRARY_DEBUG)
      SET (${basename}_LIBRARY_DEBUG ${${basename}_LIBRARY_RELEASE})
      SET (${basename}_LIBRARY       ${${basename}_LIBRARY_RELEASE})
      SET (${basename}_LIBRARIES     ${${basename}_LIBRARY_RELEASE})
    ENDIF (${basename}_LIBRARY_RELEASE AND NOT ${basename}_LIBRARY_DEBUG)

    # if only the debug version was found, set the release variable also to the debug version
    IF (${basename}_LIBRARY_DEBUG AND NOT ${basename}_LIBRARY_RELEASE)
      SET (${basename}_LIBRARY_RELEASE ${${basename}_LIBRARY_DEBUG})
      SET (${basename}_LIBRARY         ${${basename}_LIBRARY_DEBUG})
      SET (${basename}_LIBRARIES       ${${basename}_LIBRARY_DEBUG})
    ENDIF (${basename}_LIBRARY_DEBUG AND NOT ${basename}_LIBRARY_RELEASE)
    IF (${basename}_LIBRARY_DEBUG AND ${basename}_LIBRARY_RELEASE)
      # if the generator supports configuration types then set
      # optimized and debug libraries, or if the CMAKE_BUILD_TYPE has a value
      IF (CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
	SET (${basename}_LIBRARY       optimized ${${basename}_LIBRARY_RELEASE} debug ${${basename}_LIBRARY_DEBUG})
      ELSE(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
	# if there are no configuration types and CMAKE_BUILD_TYPE has no value
	# then just use the release libraries
	SET (${basename}_LIBRARY       ${${basename}_LIBRARY_RELEASE} )
      ENDIF (CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
      SET (${basename}_LIBRARIES       optimized ${${basename}_LIBRARY_RELEASE} debug ${${basename}_LIBRARY_DEBUG})
    ENDIF (${basename}_LIBRARY_DEBUG AND ${basename}_LIBRARY_RELEASE)

    SET (${basename}_LIBRARY ${${basename}_LIBRARY} CACHE FILEPATH "The ${basename} library")

    IF (${basename}_LIBRARY)
      SET (${basename}_FOUND 1)
    ENDIF (${basename}_LIBRARY)

  ENDIF (${basename}_INCLUDE_DIR )

  # Make variables changeble to the advanced user
  MARK_AS_ADVANCED (${basename}_LIBRARY ${basename}_LIBRARY_RELEASE ${basename}_LIBRARY_DEBUG ${basename}_INCLUDE_DIR )
ENDMACRO (SZIP_ADJUST_LIB_VARS)


# Look for the header file.
SET (SZIP_INCLUDE_SEARCH_DIRS
    $ENV{SZIP_INSTALL}/include
    $ENV{SZIP_INSTALL}/include/szip
    /usr/include
    /usr/include/szip
)

SET (SZIP_LIB_SEARCH_DIRS
    $ENV{SZIP_INSTALL}/lib
    /usr/lib
)

SET (SZIP_BIN_SEARCH_DIRS
    $ENV{SZIP_INSTALL}/bin
    /usr/bin
)

FIND_PATH (SZIP_INCLUDE_DIR
    NAMES szlib.h
    PATHS ${SZIP_INCLUDE_SEARCH_DIRS}
    NO_DEFAULT_PATH
)

IF (WIN32 AND NOT MINGW)
    SET (SZIP_SEARCH_DEBUG_NAMES "sz_d;libsz_d")
    SET (SZIP_SEARCH_RELEASE_NAMES "sz;libsz")
ELSE (WIN32 AND NOT MINGW)
    SET (SZIP_SEARCH_DEBUG_NAMES "sz_d")
    SET (SZIP_SEARCH_RELEASE_NAMES "sz")
ENDIF (WIN32 AND NOT MINGW)

# Look for the library.
FIND_LIBRARY (SZIP_LIBRARY_DEBUG
    NAMES ${SZIP_SEARCH_DEBUG_NAMES}
    PATHS ${SZIP_LIB_SEARCH_DIRS}
    NO_DEFAULT_PATH
)

FIND_LIBRARY (SZIP_LIBRARY_RELEASE
    NAMES ${SZIP_SEARCH_RELEASE_NAMES}
    PATHS ${SZIP_LIB_SEARCH_DIRS}
    NO_DEFAULT_PATH
)

SZIP_ADJUST_LIB_VARS (SZIP)

IF (SZIP_INCLUDE_DIR AND SZIP_LIBRARY)
  SET (SZIP_FOUND 1)
  SET (SZIP_LIBRARIES ${SZIP_LIBRARY})
  SET (SZIP_INCLUDE_DIRS ${SZIP_INCLUDE_DIR})
  IF (SZIP_LIBRARY_DEBUG)
    GET_FILENAME_COMPONENT (SZIP_LIBRARY_PATH ${SZIP_LIBRARY_DEBUG} PATH)
    SET (SZIP_LIB_DIR  ${SZIP_LIBRARY_PATH})
  ELSEIF (SZIP_LIBRARY_RELEASE)
    GET_FILENAME_COMPONENT (SZIP_LIBRARY_PATH ${SZIP_LIBRARY_RELEASE} PATH)
    SET (SZIP_LIB_DIR  ${SZIP_LIBRARY_PATH})
  ENDIF (SZIP_LIBRARY_DEBUG)

ELSE (SZIP_INCLUDE_DIR AND SZIP_LIBRARY)
  SET (SZIP_FOUND 0)
  SET (SZIP_LIBRARIES)
  SET (SZIP_INCLUDE_DIRS)
ENDIF (SZIP_INCLUDE_DIR AND SZIP_LIBRARY)

# Report the results.
IF (NOT SZIP_FOUND)
  SET (SZIP_DIR_MESSAGE
      "SZip was not found. Make sure SZIP_LIBRARY and SZIP_INCLUDE_DIR are set or set the SZIP_INSTALL environment variable."
  )
  IF (NOT SZIP_FIND_QUIETLY)
    MESSAGE (STATUS "${SZIP_DIR_MESSAGE}")
  ELSE (NOT SZIP_FIND_QUIETLY)
    IF (SZIP_FIND_REQUIRED)
      MESSAGE (FATAL_ERROR "SZip was NOT found and is Required by this project")
    ENDIF (SZIP_FIND_REQUIRED)
  ENDIF (NOT SZIP_FIND_QUIETLY)
ENDIF (NOT SZIP_FOUND)

IF (SZIP_FOUND)
  INCLUDE (CheckSymbolExists)
  #############################################
  # Find out if SZIP was build using dll's
  #############################################
  # Save required variable
  SET (CMAKE_REQUIRED_INCLUDES_SAVE ${CMAKE_REQUIRED_INCLUDES})
  SET (CMAKE_REQUIRED_FLAGS_SAVE    ${CMAKE_REQUIRED_FLAGS})
  # Add SZIP_INCLUDE_DIR to CMAKE_REQUIRED_INCLUDES
  SET (CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES};${SZIP_INCLUDE_DIRS}")

  CHECK_SYMBOL_EXISTS (SZIP_BUILT_AS_DYNAMIC_LIB "SZconfig.h" HAVE_SZIP_DLL)

  IF (HAVE_SZIP_DLL STREQUAL "TRUE")
    SET (HAVE_SZIP_DLL "1")
  ENDIF (HAVE_SZIP_DLL STREQUAL "TRUE")

  # Restore CMAKE_REQUIRED_INCLUDES and CMAKE_REQUIRED_FLAGS variables
  SET (CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES_SAVE})
  SET (CMAKE_REQUIRED_FLAGS    ${CMAKE_REQUIRED_FLAGS_SAVE})
  #
  #############################################
ENDIF (SZIP_FOUND)

IF (FIND_SZIP_DEBUG)
  MESSAGE (STATUS "SZIP_INCLUDE_DIR: ${SZIP_INCLUDE_DIR}")
  MESSAGE (STATUS "SZIP_INCLUDE_DIRS: ${SZIP_INCLUDE_DIRS}")
  MESSAGE (STATUS "SZIP_LIBRARY_DEBUG: ${SZIP_LIBRARY_DEBUG}")
  MESSAGE (STATUS "SZIP_LIBRARY_RELEASE: ${SZIP_LIBRARY_RELEASE}")
  MESSAGE (STATUS "HAVE_SZIP_DLL: ${HAVE_SZIP_DLL}")
  MESSAGE (STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
ENDIF (FIND_SZIP_DEBUG)
