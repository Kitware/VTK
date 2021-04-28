set(classes
  QQuickVTKInteractiveWidget
  QQuickVTKInteractorAdapter
  QQuickVTKRenderItem
  QQuickVTKRenderWindow
  )

include(vtkQt)
set(qt_components
  Gui
  OpenGL
  Quick
  Qml
  )

vtk_module_find_package(
  PACKAGE     "Qt${vtk_qt_major_version}"
  VERSION     5.9
  COMPONENTS  ${qt_components}
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         "Qt${vtk_qt_major_version}_VERSION")

set(CMAKE_AUTOMOC 1)

vtk_module_add_module(VTK::GUISupportQtQuick
  CLASSES ${classes}
  )

foreach (_qt_comp IN LISTS qt_components)
  list(APPEND qt_modules
    "Qt${vtk_qt_major_version}::${_qt_comp}")
endforeach ()

vtk_module_definitions(VTK::GUISupportQtQuick PRIVATE QT_NO_KEYWORDS)
vtk_module_link(VTK::GUISupportQtQuick PUBLIC ${qt_modules})

add_subdirectory(qml)
