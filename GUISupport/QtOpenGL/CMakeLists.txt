set(classes
  QVTKGraphicsItem)

if (NOT VTK_LEGACY_REMOVE)
  list(APPEND classes
    QVTKWidget2)
endif ()

vtk_module_find_package(
  PACKAGE     Qt5
  COMPONENTS  OpenGL
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         Qt5_VERSION)
set(CMAKE_AUTOMOC 1)
vtk_module_add_module(VTK::GUISupportQtOpenGL
  CLASSES ${classes})
vtk_module_link(VTK::GUISupportQtOpenGL PUBLIC Qt5::OpenGL)
vtk_module_definitions(VTK::GUISupportQtOpenGL PRIVATE VTK_OPENGL2)
