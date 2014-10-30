#
# Find the native EXPAT includes and library
#
# This module defines
# EXPAT_INCLUDE_DIR, where to find expat.h, etc.
# EXPAT_LIBRARIES, the libraries to link against to use EXPAT.
# EXPAT_FOUND, If false, do not try to use EXPAT.

# also defined, but not for general use are
# EXPAT_LIBRARY, where to find the EXPAT library.

FIND_PATH(EXPAT_INCLUDE_DIR expat.h
  /usr/local/include
  /usr/include
)

FIND_LIBRARY(EXPAT_LIBRARY expat
  /usr/lib
  /usr/local/lib
)

IF(EXPAT_INCLUDE_DIR)
  IF(EXPAT_LIBRARY)
    SET( EXPAT_FOUND "YES" )
    SET( EXPAT_LIBRARIES ${EXPAT_LIBRARY} )
  ENDIF()
ENDIF()
