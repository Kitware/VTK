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
  QVTKRenderWidget.h
  QVTKWin32Header.h)

set(private_headers
  QVTKInteractorInternal.h)

set(qt5_components
  Widgets)

vtk_module_find_package(
  PACKAGE     Qt5
  VERSION     5.9
  COMPONENTS  ${qt5_components}
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         Qt5_VERSION)

set(CMAKE_AUTOMOC 1)

vtk_module_add_module(VTK::GUISupportQt
  CLASSES ${classes}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers})

vtk_module_link(VTK::GUISupportQt PUBLIC Qt5::Widgets)
