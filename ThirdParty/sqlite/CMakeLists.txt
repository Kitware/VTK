vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "public-domain"
    SPDX_LICENSE_IDENTIFIER
      "blessing"
    SPDX_COPYRIGHT_TEXT
      "NONE"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/sqlite.git@for/vtk-20210924-3.36.0"
    VERSION
      "3.36.0"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE SQLite3
    TARGETS SQLite::SQLite3
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_sqlite.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_sqlite.h")

if (VTK_MODULE_USE_EXTERNAL_vtksqlite)
  if (vtk_sqlite_build_binary)
    find_program(VTK_SQLITE_EXECUTABLE
      NAMES sqlite3
      DOC "sqlite binary")
    if (NOT VTK_SQLITE_EXECUTABLE)
      message(FATAL_ERROR
        "Failed to find the `sqlite3` binary; required to build other "
        "components.")
    endif ()

    add_executable(VTK::sqlitebin IMPORTED GLOBAL)
    set_target_properties(VTK::sqlitebin
      PROPERTIES
        IMPORTED_LOCATION "${VTK_SQLITE_EXECUTABLE}")
  endif ()
endif ()

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_sqlite.h")
