#
# Find the ODBC driver manager includes and library.
#
# ODBC is an open standard for connecting to different databases in a
# semi-vendor-independent fashion.  First you install the ODBC driver
# manager.  Then you need a driver for each separate database you want
# to connect to (unless a generic one works).  VTK includes neither
# the driver manager nor the vendor-specific drivers: you have to find
# those yourself.
#
# This module defines
# ODBC_INCLUDE_DIRECTORIES, where to find sql.h
# ODBC_LIBRARIES, the libraries to link against to use ODBC
# ODBC_FOUND.  If false, you cannot build anything that requires MySQL.

# also defined, but not for general use is
# ODBC_LIBRARY, where to find the ODBC driver manager library.

SET( ODBC_FOUND 0 )

FIND_PATH(ODBC_INCLUDE_DIRECTORIES sql.h
  /usr/include
  /usr/include/odbc
  /usr/local/include
  /usr/local/include/odbc
  /usr/local/odbc/include
  "C:/Program Files/ODBC/include"
  "C:/ODBC/include"
  DOC "Specify the directory containing sql.h."
)

FIND_LIBRARY( ODBC_LIBRARY
  NAMES odbc iodbc unixodbc
  PATHS
  /usr/lib
  /usr/lib/odbc
  /usr/local/lib
  /usr/local/lib/odbc
  /usr/local/odbc/lib
  "C:/Program Files/ODBC/lib"
  "C:/ODBC/lib/debug"
  DOC "Specify the ODBC driver manager library here."
)

IF (ODBC_LIBRARY)
  IF (ODBC_INCLUDE_DIRECTORIES)
    SET( ODBC_FOUND 1 )
  ENDIF (ODBC_INCLUDE_DIRECTORIES)
ENDIF (ODBC_LIBRARY)

SET( ODBC_LIBRARIES ${ODBC_LIBRARY} )

MARK_AS_ADVANCED( ODBC_FOUND ODBC_LIBRARY ODBC_EXTRA_LIBRARIES ODBC_INCLUDE_DIRECTORIES )

