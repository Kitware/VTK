vtk_add_library(vtksqlite
  sqlite3.c
  sqlite3.h
  vtk_sqlite_mangle.h
  "${CMAKE_CURRENT_BINARY_DIR}/vtksqlite_export.h")
if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES
    sqlite3.h
    vtk_sqlite_mangle.h
    "${CMAKE_CURRENT_BINARY_DIR}/vtksqlite_export.h"
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}/vtksqlite"
    COMPONENT Development)
endif()

target_compile_definitions(vtksqlite
  PRIVATE
    SQLITE_THREADSAFE=1)
target_link_libraries(vtksqlite
  PRIVATE
    ${CMAKE_THREAD_LIBS})
target_include_directories(vtksqlite
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")

include(GenerateExportHeader)
generate_export_header(vtksqlite
  EXPORT_MACRO_NAME SQLITE_API
  EXPORT_FILE_NAME  vtksqlite_export.h)
