set(classes
  QFilterTreeProxyModel
  QVTKApplication
  QVTKInteractor
  QVTKInteractorAdapter
  QVTKOpenGLNativeWidget
  QVTKOpenGLStereoWidget
  QVTKOpenGLWindow
  QVTKRenderWindowAdapter
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
  QVTKOpenGLWidget.h
  QVTKRenderWidget.h
  QVTKWin32Header.h)

set(private_headers
  QVTKInteractorInternal.h)

find_package(Qt6Widgets QUIET)
set(VTK_QT_VERSION "6" CACHE INTERNAL "Installed Qt version.")
if(NOT Qt6Widgets_FOUND)
  find_package(Qt5Widgets QUIET)
  if(NOT Qt5Widgets_FOUND)
    message(FATAL_ERROR "Could not find a valid Qt installation.")
  endif()
  set(VTK_QT_VERSION 5)
else()
endif()
set(qt_components
  OpenGL
  OpenGLWidgets
  Widgets)

vtk_module_find_package(
  PACKAGE     Qt${VTK_QT_VERSION}
  VERSION     5.9
  COMPONENTS  ${qt_components}
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         Qt${VTK_QT_VERSION}_VERSION)

set(CMAKE_AUTOMOC 1)

vtk_module_add_module(VTK::GUISupportQt
  CLASSES ${classes}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers})

foreach(_qt_comp ${qt_components})
  list(APPEND qt_modules "Qt${VTK_QT_VERSION}::${_qt_comp}")
endforeach()

vtk_module_definitions(VTK::GUISupportQt PRIVATE QT_NO_KEYWORDS)
vtk_module_link(VTK::GUISupportQt PUBLIC ${qt_modules})
