vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkglew/LICENSE.txt"
    VERSION       "2.1.0"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE GLEW
    TARGETS GLEW::GLEW
    STANDARD_INCLUDE_DIRS)

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
