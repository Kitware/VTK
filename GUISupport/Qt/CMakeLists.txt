include(vtkQt)

# set up sources to build
set(QVTKLibSrcs
  QFilterTreeProxyModel.cxx
  QVTKApplication.cxx
  QVTKInteractorAdapter.cxx
  QVTKInteractor.cxx
  vtkEventQtSlotConnect.cxx
  vtkQtAbstractModelAdapter.cxx
  vtkQtAnnotationLayersModelAdapter.cxx
  vtkQtConnection.cxx
  vtkQtDebugLeaksModel.cxx
  vtkQtDebugLeaksView.cxx
  vtkQtTableModelAdapter.cxx
  vtkQtTreeModelAdapter.cxx
  )

set(QVTKMocHeaders
  QFilterTreeProxyModel.h
  QVTKApplication.h
  QVTKInteractorAdapter.h
  QVTKInteractorInternal.h
  vtkQtAbstractModelAdapter.h
  vtkQtAnnotationLayersModelAdapter.h
  vtkQtConnection.h
  vtkQtDebugLeaksModel.h
  vtkQtDebugLeaksView.h
  vtkQtTableModelAdapter.h
  vtkQtTreeModelAdapter.h
  )

if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  list(APPEND QVTKLibSrcs QVTKOpenGLWidget.cxx)
  list(APPEND QVTKMocHeaders QVTKOpenGLWidget.h)
endif()

set(add_qvtkwidget FALSE)
if(NOT VTK_LEGACY_REMOVE)
  # QVTKWidget and related classes can only be added on Linux when
  # X11 support is enabled since it depends on X11.
  if(WIN32 OR APPLE OR VTK_USE_X)
    list(APPEND QVTKMocHeaders QVTKWidget.h)
    list(APPEND QVTKLibSrcs QVTKPaintEngine.cxx QVTKPaintEngine.h QVTKWidget.cxx)
    set(add_qvtkwidget TRUE)
  endif()
endif()

if(VTK_USE_X AND VTK_USE_TDX)
  list(APPEND QVTKMocHeaders vtkTDxQtUnixDevices.h)
  list(APPEND QVTKLibSrcs vtkTDxQtUnixDevices.cxx)
endif()

if(VTK_USE_X)
  find_package(X11 REQUIRED)
  include_directories(${X11_INCLUDE_DIR})
endif()

set(QVTKNonMocHeaders
  QVTKWin32Header.h
  vtkEventQtSlotConnect.h
  QVTKInteractor.h
  )

set(qt_components Widgets)
if(VTK_USE_X)
  list(APPEND qt_components X11Extras)
endif()
find_package(Qt5 COMPONENTS Widgets ${qt_components} REQUIRED QUIET)


# When this module is loaded by an app, load Qt too.
vtk_module_export_code_find_package(Qt5 COMPONENTS Widgets)
foreach(opt
    QT_RCC_EXECUTABLE
    QT_MOC_EXECUTABLE
    QT_UIC_EXECUTABLE
    QT_QMAKE_EXECUTABLE
    )
  set(VTK_${opt} "${${opt}}")
  list(APPEND vtkGUISupportQt_EXPORT_OPTIONS VTK_${opt})
endforeach()

qt5_wrap_cpp(QVTKLibMocSrcs ${QVTKMocHeaders})

set(${vtk-module}_NO_HeaderTest 1)
vtk_module_library(${vtk-module}
  ${QVTKLibSrcs} ${QVTKLibMocSrcs}
  ${QVTKNonMocHeaders} ${QVTKMocHeaders}
  )

vtk_module_link_libraries(${vtk-module} LINK_PUBLIC Qt5::Widgets)
if(VTK_USE_X)
  vtk_module_link_libraries(${vtk-module} LINK_PRIVATE Qt5::X11Extras ${X11_LIBRARIES})
endif()

# This build plugin logic should just get migrated into a module I think.
cmake_dependent_option(VTK_BUILD_QT_DESIGNER_PLUGIN
  "Build the VTK Qt plugin for the Qt Designer." ON
  "BUILD_SHARED_LIBS;NOT VTK_LEGACY_REMOVE;add_qvtkwidget" OFF)
mark_as_advanced(VTK_BUILD_QT_DESIGNER_PLUGIN)

if(VTK_BUILD_QT_DESIGNER_PLUGIN)
  set(PluginLibSrcs Q4VTKWidgetPlugin.cxx)
  set(PluginMocHeaders Q4VTKWidgetPlugin.h)

  set(qt_components UiPlugin)
  if(Qt5_VERSION VERSION_LESS "5.5.0")
    set(qt_components Designer)
  endif()
  find_package(Qt5 COMPONENTS ${qt_components} REQUIRED QUIET)
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
  vtk_module_link_libraries(QVTKWidgetPlugin LINK_PRIVATE Qt5::UiPlugin)
  if(Qt5_VERSION VERSION_LESS "5.5.0")
    vtk_module_link_libraries(QVTKWidgetPlugin LINK_PRIVATE Qt5::Designer)
  endif()

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
    set_target_properties(${vtk-module} PROPERTIES POST_INSTALL_SCRIPT
      ${CMAKE_CURRENT_BINARY_DIR}/PluginInstall.cmake)
  endif()
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.ctest.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.ctest" @ONLY)
