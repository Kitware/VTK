vtk_module_find_package(PACKAGE Threads)

vtk_module_add_module(VTK::sqlite
  SOURCES sqlite3.c
  HEADERS sqlite3.h
          vtk_sqlite_mangle.h
          "${CMAKE_CURRENT_BINARY_DIR}/vtksqlite_export.h"
  HEADERS_SUBDIR "vtksqlite")

vtk_module_definitions(VTK::sqlite
  PRIVATE
    SQLITE_THREADSAFE=1)
vtk_module_link(VTK::sqlite
  PRIVATE
    ${CMAKE_DL_LIBS}
    Threads::Threads)
vtk_module_include(VTK::sqlite
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")

if (vtk_sqlite_build_binary)
  vtk_module_add_executable(sqlitebin
    NO_INSTALL
    shell.c)
  add_executable(VTK::sqlitebin ALIAS sqlitebin)
  target_link_libraries(sqlitebin
    PRIVATE
      VTK::sqlite)
    if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
      # allows sqlitebin to access host file system for generating proj.db etc.
      # permit memory growth to generate arbitrary sized files.
      # set EXIT_RUNTIME=1 so that stdout is flushed on exit.
      target_link_options(sqlitebin
        PRIVATE
          "-sALLOW_MEMORY_GROWTH=1"
          "-sNODERAWFS=1"
          "-sEXIT_RUNTIME=1")
    endif ()
endif ()

include(GenerateExportHeader)
generate_export_header(vtksqlite
  BASE_NAME         SQLITE
  EXPORT_MACRO_NAME SQLITE_API
  EXPORT_FILE_NAME  vtksqlite_export.h
  DEPRECATED_MACRO_NAME VTKSQLITE_DEPRECATED)
