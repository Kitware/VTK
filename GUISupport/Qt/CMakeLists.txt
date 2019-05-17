set(classes
  QFilterTreeProxyModel
  QVTKApplication
  QVTKInteractor
  QVTKInteractorAdapter
  QVTKOpenGLNativeWidget
  QVTKOpenGLWidget
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

set(add_qvtkwidget FALSE)
if (NOT VTK_LEGACY_REMOVE)
  # QVTKWidget and related classes can only be added on Linux when
  # X11 support is enabled since it depends on X11.
  if(WIN32 OR APPLE OR VTK_USE_X)
    list(APPEND classes
      QVTKPaintEngine
      QVTKWidget)
    set(add_qvtkwidget TRUE)
  endif ()
endif ()

if (VTK_USE_X AND VTK_USE_TDX)
  list(APPEND classes
    vtkTDxQtUnixDevices)
endif ()

set(headers
  QVTKWin32Header.h)

set(private_headers
  QVTKInteractorInternal.h)

set(qt5_components
  Widgets)

if (VTK_USE_X AND add_qvtkwidget)
  # X11Extras is only needed for QVTKWidget, which is a deprecated class.
  list(APPEND qt5_components
    X11Extras)
endif ()

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
if (VTK_USE_X AND add_qvtkwidget)
  # X11Extras is only needed for QVTKWidget, which is a deprecated class.
  vtk_module_link(VTK::GUISupportQt PRIVATE Qt5::X11Extras)
endif ()

# This build plugin logic should just get migrated into a module I think.
cmake_dependent_option(VTK_BUILD_QT_DESIGNER_PLUGIN
  "Build the VTK Qt plugin for the Qt Designer." ON
  "BUILD_SHARED_LIBS;NOT VTK_LEGACY_REMOVE;add_qvtkwidget" OFF)
mark_as_advanced(VTK_BUILD_QT_DESIGNER_PLUGIN)

if (VTK_BUILD_QT_DESIGNER_PLUGIN)
  set(plugin_srcs
    Q4VTKWidgetPlugin.cxx
    Q4VTKWidgetPlugin.h)

  # The VTK_INSTALL_QT_PLUGIN_DIR variable sets the location
  # in which the Qt plugin will be installed.
  if(NOT VTK_INSTALL_QT_PLUGIN_DIR)
    set(VTK_INSTALL_QT_PLUGIN_DIR qt5/plugins/designer)
  endif()

  set(module_destination "${CMAKE_INSTALL_LIBDIR}")
  if (WIN32)
    set(module_destination "${CMAKE_INSTALL_BINDIR}")
  endif ()

  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${module_destination}/${VTK_INSTALL_QT_PLUGIN_DIR}")

  find_package(Qt5 COMPONENTS UiPlugin REQUIRED)

  # add QVTK plugin from sources
  # stand-alone as it doesn't depend on QVTK library
  add_library(QVTKWidgetPlugin MODULE
    ${plugin_srcs})
  target_compile_definitions(QVTKWidgetPlugin
    PRIVATE
      QT_NO_DEBUG)
  target_link_libraries(QVTKWidgetPlugin
    PRIVATE
      Qt5::UiPlugin)
  if (WIN32)
    set_target_properties(QVTKWidgetPlugin
      PROPERTIES
        DEBUG_POSTFIX "d")
  endif ()

  # install rules

  # If no runtime is to be installed then do not install the qt plugin.
  if(NOT VTK_INSTALL_NO_RUNTIME)
    install(
      TARGETS QVTKWidgetPlugin
      LIBRARY DESTINATION "${module_destination}/${VTK_INSTALL_QT_PLUGIN_DIR}")
  endif ()
endif ()
