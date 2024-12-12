vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkpng/LICENSE"
    SPDX_LICENSE_IDENTIFIER
      "Libpng"
    SPDX_COPYRIGHT_TEXT
      "Copyright (c) 1995-2024 The PNG Reference Library Authors."
      "Copyright (c) 2018-2024 Cosmin Truta."
      "Copyright (c) 2000-2002, 2004, 2006-2018 Glenn Randers-Pehrson."
      "Copyright (c) 1996-1997 Andreas Dilger."
      "Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc."
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/png.git@for/vtk-20240416-1.6.44"
    VERSION
      "1.6.44"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE PNG
    TARGETS PNG::PNG
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_png.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_png.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_png.h")
