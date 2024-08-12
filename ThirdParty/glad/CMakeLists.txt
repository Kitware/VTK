vtk_module_third_party_internal(
  LICENSE_FILES           "vtkglad/LICENSE"
  SPDX_LICENSE_IDENTIFIER "GLAD"
  SPDX_COPYRIGHT_TEXT     "Copyright (c) 2013-2022 David Herberth, Copyright (c) 2013-2020 The Khronos Group Inc., "
  SPDX_DOWNLOAD_LOCATION  "git+https://github.com/Dav1dde/glad/tree/glad2"
  VERSION       "2.0.6"
  STANDARD_INCLUDE_DIRS)

include(vtkOpenGLOptions)
if(VTK_OPENGL_USE_GLES OR ANDROID AND NOT VTK_IOS_BUILD)
  set(VTK_MODULE_vtkglad_GLES3 1)
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_glad.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_glad.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_glad.h")
