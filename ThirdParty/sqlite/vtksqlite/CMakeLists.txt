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
ELSE ()
  INCLUDE(CheckIncludeFiles)
  CHECK_INCLUDE_FILES("pthread.h"        HAVE_PTHREAD_H)
  IF(HAVE_PTHREAD_H AND VTK_USE_PTHREADS)
    ADD_DEFINITIONS(-DHAVE_PTHREAD_H)
    ADD_DEFINITIONS(-DVTK_SQLITE_THREADSAFE=1)
  ELSE()
    ADD_DEFINITIONS(-DVTK_SQLITE_THREADSAFE=0)
  ENDIF()
  VTK_ADD_LIBRARY( vtksqlite ${SQLite_SRCS} )
  IF (UNIX AND HAVE_PTHREAD_H AND VTK_USE_PTHREADS)
    TARGET_LINK_LIBRARIES(vtksqlite ${CMAKE_THREAD_LIBS})
  ENDIF()
ENDIF ()

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/vtk_sqlite3.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}/vtksqlite
    COMPONENT Development)
ENDIF()
