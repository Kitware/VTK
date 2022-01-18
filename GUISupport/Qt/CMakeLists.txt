set(classes
  QFilterTreeProxyModel
  QVTKApplication
  QVTKInteractor
  QVTKInteractorAdapter
  QVTKOpenGLNativeWidget
  QVTKOpenGLStereoWidget
  QVTKOpenGLWindow
  QVTKRenderWindowAdapter
  QVTKTableModelAdapter
  vtkEventQtSlotConnect
  vtkQWidgetRepresentation
  vtkQWidgetTexture
  vtkQWidgetWidget
  vtkQtAbstractModelAdapter
  vtkQtAnnotationLayersModelAdapter
  vtkQtConnection
  vtkQtDebugLeaksModel
  vtkQtDebugLeaksView
  vtkQtTableModelAdapter
  vtkQtTreeModelAdapter)

if (VTK_USE_X AND VTK_USE_TDX)
  list(APPEND classes
    vtkTDxQtUnixDevices)
endif ()

set(headers
  QVTKRenderWidget.h
  QVTKWin32Header.h)

set(private_headers
  QVTKInteractorInternal.h)

include(vtkQt)
set(qt_components
  OpenGL
  Widgets)
if(vtk_qt_major_version VERSION_GREATER_EQUAL 6)
  list(APPEND qt_components OpenGLWidgets)
endif()

vtk_module_find_package(
  PACKAGE     "Qt${vtk_qt_major_version}"
  VERSION     5.9
  COMPONENTS  ${qt_components}
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         "Qt${vtk_qt_major_version}_VERSION")

set(CMAKE_AUTOMOC 1)

vtk_module_add_module(VTK::GUISupportQt
  CLASSES ${classes}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers})

foreach(_qt_comp IN LISTS qt_components)
  list(APPEND qt_modules "Qt${vtk_qt_major_version}::${_qt_comp}")
endforeach()

vtk_module_definitions(VTK::GUISupportQt PRIVATE QT_NO_KEYWORDS)
vtk_module_link(VTK::GUISupportQt PUBLIC ${qt_modules})
