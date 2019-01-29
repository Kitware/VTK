set(classes
  QFilterTreeProxyModel
  QVTKApplication
  QVTKInteractor
  QVTKInteractorAdapter
  QVTKOpenGLNativeWidget
  QVTKOpenGLWidget
  QVTKOpenGLWindow
  vtkEventQtSlotConnect
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

if (VTK_USE_X)
  list(APPEND qt5_components
    X11Extras)
endif ()

vtk_module_find_package(
  PACKAGE     Qt5
  COMPONENTS  ${qt5_components}
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         Qt5_VERSION)

set(CMAKE_AUTOMOC 1)

vtk_module_add_module(VTK::GUISupportQt
  CLASSES ${classes}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers})

vtk_module_link(VTK::GUISupportQt PUBLIC Qt5::Widgets)
if (VTK_USE_X)
  vtk_module_link(VTK::GUISupportQt PRIVATE Qt5::X11Extras)
endif ()

# This build plugin logic should just get migrated into a module I think.
cmake_dependent_option(VTK_BUILD_QT_DESIGNER_PLUGIN
  "Build the VTK Qt plugin for the Qt Designer." ON
  "BUILD_SHARED_LIBS;NOT VTK_LEGACY_REMOVE;add_qvtkwidget" OFF)
mark_as_advanced(VTK_BUILD_QT_DESIGNER_PLUGIN)

if (VTK_BUILD_QT_DESIGNER_PLUGIN)
  set(PluginLibSrcs Q4VTKWidgetPlugin.cxx)

  set(PluginMocHeaders Q4VTKWidgetPlugin.h)

  set(qt_component UiPlugin)
  if (Qt5_VERSION VERSION_LESS "5.5.0")
    set(qt_component Designer)
  endif ()
  find_package(Qt5 COMPONENTS ${qt_component} REQUIRED)
  qt5_wrap_cpp(PluginMocSrcs ${PluginMocHeaders} TARGET QVTKWidgetPlugin)

  # add QVTK plugin from sources
  # stand-alone as it doesn't depend on QVTK library
  add_library(QVTKWidgetPlugin
    SHARED
    ${PluginLibSrcs}
    ${PluginMocSrcs}
    )

  set_target_properties(QVTKWidgetPlugin PROPERTIES COMPILE_DEFINITIONS QT_NO_DEBUG)

  # link with Qt libs
  target_link_libraries(QVTKWidgetPlugin Qt5::${qt_component})

  # install rules

  # The VTK_INSTALL_QT_PLUGIN_DIR variable sets the location
  # in which the Qt plugin will be installed.  It may or may not contain
  # variable references to CMAKE_INSTALL_PREFIX and VTK_INSTALL_QT_DIR.
  # The default is to install to VTK_INSTALL_QT_DIR under the installation
  # prefix.  The default VTK_INSTALL_QT_DIR will allow the designer plugin
  # path to be set to vtk-install-prefix/plugins to get the plugin.
  if(NOT VTK_INSTALL_QT_DIR)
    set(VTK_INSTALL_QT_DIR plugins/designer)
  endif()

  # If no runtime is to be installed then do not install the qt plugin.
  if(VTK_INSTALL_NO_RUNTIME)
    set(VTK_INSTALL_NO_QT_PLUGIN 1)
  endif()

  if(NOT VTK_INSTALL_NO_QT_PLUGIN)
    # Set default plugin install directory.
    set(DOLLAR "$")
    if(DEFINED VTK_INSTALL_QT_PLUGIN_DIR)
    else()
      set(VTK_INSTALL_QT_PLUGIN_DIR
        "${DOLLAR}{CMAKE_INSTALL_PREFIX}/${DOLLAR}{VTK_INSTALL_QT_DIR}"
        CACHE STRING "Directory in which the VTK Qt plugin is placed during installation.")
      mark_as_advanced(VTK_INSTALL_QT_PLUGIN_DIR)
    endif()

    # Configure the plugin install script.  This is used instead of
    # INSTALL TARGETS to allow the plugin to be installed outside the
    # main install prefix.  Attach the script as a post-install script.
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/PluginInstall.cmake.in
      ${CMAKE_CURRENT_BINARY_DIR}/PluginInstall.cmake
      @ONLY)
    set_target_properties(GUISupportQt PROPERTIES POST_INSTALL_SCRIPT
      ${CMAKE_CURRENT_BINARY_DIR}/PluginInstall.cmake)
  endif()
endif ()
