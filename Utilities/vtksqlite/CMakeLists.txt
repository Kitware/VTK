# This is the single-file distribution of SQLite version 3.3.16.  You
# can read more about it at www.sqlite.org.  In a nutshell, SQLite is
# a compact implementation of most of SQL92 that can be easily
# embedded within C and C++ applications.  The authors have placed its
# code in the public domain so that it may be used for any purpose.
# You can access SQLite databases through vtkSQLiteDatabase and
# vtkSQLiteQuery.
#


#
# NOTE: To convert the sqlite3 distribution files into these, I
# renamed sqlite3.c and sqlite3.h into vtk_sqlite3.c and vtk_sqlite3.h.
# Then I went into each file and did a string search-and-replace from
# sqlite3 to vtk_sqlite3.h.  Finally, I removed the copy of sqlite3.h 
# from the beginning of vtk_sqlite3.c.  
#

PROJECT(VTKSQLite)

SET( SQLite_SRCS
     vtk_sqlite3.c
)

INCLUDE_DIRECTORIES( ${VTKSQLite_SOURCE_DIR}/Utilities/vtksqlite )

# You really, really want this library to be statically linked on
# Windows.

IF (WIN32)
  VTK_ADD_LIBRARY( vtksqlite STATIC ${SQLite_SRCS} )
ELSE (WIN32)
  INCLUDE(CheckIncludeFiles)
  CHECK_INCLUDE_FILES("pthread.h"        HAVE_PTHREAD_H)
  IF(HAVE_PTHREAD_H AND CMAKE_USE_PTHREADS)
    ADD_DEFINITIONS(-DHAVE_PTHREAD_H)
  ENDIF(HAVE_PTHREAD_H AND CMAKE_USE_PTHREADS)
  VTK_ADD_LIBRARY( vtksqlite ${SQLite_SRCS} )
  IF (UNIX AND HAVE_PTHREAD_H AND CMAKE_USE_PTHREADS)
    TARGET_LINK_LIBRARIES(vtksqlite ${CMAKE_THREAD_LIBS})
  ENDIF (UNIX AND HAVE_PTHREAD_H AND CMAKE_USE_PTHREADS)
ENDIF (WIN32)

IF(VTK_LIBRARY_PROPERTIES)
  SET_TARGET_PROPERTIES(vtksqlite PROPERTIES ${VTK_LIBRARY_PROPERTIES})
ENDIF(VTK_LIBRARY_PROPERTIES)

IF(NOT VTK_INSTALL_NO_LIBRARIES)
  INSTALL(TARGETS vtksqlite
    EXPORT ${VTK_INSTALL_EXPORT_NAME}
    RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeLibraries
    LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries
    ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${VTK_SOURCE_DIR}/Utilities/vtksqlite/vtk_sqlite3.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}/vtksqlite
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
