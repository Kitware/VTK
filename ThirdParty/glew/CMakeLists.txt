vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkglew/LICENSE.txt"
    SPDX_LICENSE_IDENTIFIER
      "BSD-3-Clause AND MIT"
    SPDX_COPYRIGHT_TEXT
      "Copyright (C) 2002-2007, Milan Ikits <milan ikits[]ieee org>."
      "Copyright (C) 2002-2007, Marcelo E. Magallon <mmagallo[]debian org>."
      "Copyright (C) 2002, Lev Povalahev."
      "Copyright (C) 1999-2007  Brian Paul."
      "Copyright (c) 2007 The Khronos Group Inc."
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/glew.git@for/vtk-20240722-2.2.0"
    VERSION
      "2.2.0"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE GLEW
    TARGETS GLEW::GLEW
    STANDARD_INCLUDE_DIRS)

include(vtkOpenGLOptions)
if(VTK_OPENGL_USE_GLES OR ANDROID AND NOT VTK_IOS_BUILD)
  set(VTK_MODULE_vtkglew_GLES3 1)
endif()

include(vtkDetectLibraryType)
vtk_detect_library_shared(vtkglew GLEW::GLEW)

set(VTK_GLEW_SHARED "${vtkglew_is_shared}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_glew.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_glew.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_glew.h")
