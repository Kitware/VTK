vtk_module_find_package(PACKAGE Threads)

vtk_module_add_module(VTK::sqlite
  SOURCES sqlite3.c
  HEADERS sqlite3.h
          vtk_sqlite_mangle.h
          "${CMAKE_CURRENT_BINARY_DIR}/vtksqlite_export.h"
  HEADERS_SUBDIR "vtksqlite")

target_compile_definitions(sqlite
  PRIVATE
    SQLITE_THREADSAFE=1)
target_link_libraries(sqlite
  PRIVATE
    ${CMAKE_DL_LIBS}
    Threads::Threads)
target_include_directories(sqlite
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
endif ()

include(GenerateExportHeader)
generate_export_header(sqlite
  EXPORT_MACRO_NAME SQLITE_API
  EXPORT_FILE_NAME  vtksqlite_export.h
  DEPRECATED_MACRO_NAME VTKSQLITE_DEPRECATED)
