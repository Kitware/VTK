include(vtkQt)

# set up sources to build
set(QVTKLibSrcs
  vtkEventQtSlotConnect.cxx
  vtkQtConnection.cxx
  QVTKApplication.cxx
  QVTKInteractor.cxx
  QVTKInteractorAdapter.cxx
  QVTKWidget.cxx
  QFilterTreeProxyModel.cxx
  QVTKPaintEngine.cxx
  QVTKPaintEngine.h
  vtkQtAbstractModelAdapter.cxx
  vtkQtAnnotationLayersModelAdapter.cxx
  vtkQtDebugLeaksModel.cxx
  vtkQtDebugLeaksView.cxx
  vtkQtTableModelAdapter.cxx
  vtkQtTreeModelAdapter.cxx
  )

set(QVTKMocHeaders
  QVTKApplication.h
  QVTKInteractorAdapter.h
  QVTKInteractorInternal.h
  QVTKWidget.h
  vtkQtConnection.h
  QFilterTreeProxyModel.h
  vtkQtAbstractModelAdapter.h
  vtkQtAnnotationLayersModelAdapter.h
  vtkQtDebugLeaksModel.h
  vtkQtDebugLeaksView.h
  vtkQtTableModelAdapter.h
  vtkQtTreeModelAdapter.h
  )

if(VTK_USE_X AND VTK_USE_TDX)
  set(QVTKMocHeaders ${QVTKMocHeaders}
    vtkTDxQtUnixDevices.h
    )
  set(QVTKLibSrcs ${QVTKLibSrcs}
    vtkTDxQtUnixDevices.cxx
    )
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

if(VTK_QT_VERSION VERSION_GREATER "4")
  set(qt_component_x11extras)
  if(VTK_USE_X)
    set(qt_component_x11extras X11Extras)
  endif()

  find_package(Qt5 COMPONENTS Widgets ${qt_component_x11extras} REQUIRED QUIET)
  include_directories(${Qt5Widgets_INCLUDE_DIRS})
  add_definitions(${Qt5Widgets_DEFINITIONS})

  if(qt_component_x11extras)
    include_directories(${Qt5X11Extras_INCLUDE_DIRS})
    add_definitions(${Qt5X11Extras_DEFINITIONS})
  endif()

  qt5_wrap_cpp(QVTKLibMocSrcs ${QVTKMocHeaders})

  set(QT_LIBRARIES ${Qt5Widgets_LIBRARIES})
  set(QT_LIBRARIES_PRIVATE ${Qt5X11Extras_LIBRARIES})

  # When this module is loaded by an app, load Qt too.
  vtk_module_export_code_find_package(Qt5 COMPONENTS Widgets)
else()
  # import Qt4 build settings
  set(QT_USE_QTNETWORK 1)
  find_package(Qt4 REQUIRED QUIET)
  mark_as_advanced(QT_QMAKE_EXECUTABLE)
  include(${QT_USE_FILE})

  qt4_wrap_cpp(QVTKLibMocSrcs ${QVTKMocHeaders})
  set(QT_LIBRARIES_PRIVATE)
endif()

foreach(opt
    QT_RCC_EXECUTABLE
    QT_MOC_EXECUTABLE
    QT_UIC_EXECUTABLE
    QT_QMAKE_EXECUTABLE
    )
  set(VTK_${opt} "${${opt}}")
  list(APPEND vtkGUISupportQt_EXPORT_OPTIONS VTK_${opt})
endforeach()

set(${vtk-module}_NO_HeaderTest 1)
vtk_module_library(${vtk-module}
  ${QVTKLibSrcs} ${QVTKLibMocSrcs}
  ${QVTKNonMocHeaders} ${QVTKMocHeaders}
  )

vtk_module_link_libraries(${vtk-module} LINK_PUBLIC ${QT_LIBRARIES})
if(QT_LIBRARIES_PRIVATE)
  vtk_module_link_libraries(${vtk-module} LINK_PRIVATE ${QT_LIBRARIES_PRIVATE})
endif()

if(VTK_USE_X)
  vtk_module_link_libraries(${vtk-module} LINK_PRIVATE ${X11_LIBRARIES})
endif()

# This build plugin logic should just get migrated into a module I think.
cmake_dependent_option(VTK_BUILD_QT_DESIGNER_PLUGIN
  "Build the VTK Qt plugin for the Qt Designer." ON
  BUILD_SHARED_LIBS OFF)
mark_as_advanced(VTK_BUILD_QT_DESIGNER_PLUGIN)

if(BUILD_SHARED_LIBS AND VTK_BUILD_QT_DESIGNER_PLUGIN)
  set(PluginLibSrcs Q4VTKWidgetPlugin.cxx)

  set(PluginMocHeaders Q4VTKWidgetPlugin.h)

  if(VTK_QT_VERSION VERSION_GREATER "4")
    set(qt_component UiPlugin)
    if (Qt5_VERSION VERSION_LESS "5.5.0")
      set(qt_component Designer)
    endif ()
    find_package(Qt5 COMPONENTS ${qt_component} REQUIRED QUIET)
    add_definitions(${Qt5${qt_component}_DEFINITIONS})
    include_directories(${Qt5${qt_component}_INCLUDE_DIRS})
    qt5_wrap_cpp(PluginMocSrcs ${PluginMocHeaders} TARGET QVTKWidgetPlugin)
    set(QT_LIBRARIES Qt5::${qt_component})
  else()
    add_definitions(-DQT_PLUGIN)
    include_directories(${QT_QTDESIGNER_INCLUDE_DIR})
    qt4_wrap_cpp(PluginMocSrcs ${PluginMocHeaders})
    set(QT_LIBRARIES ${QT_QTGUI_LIBRARY} ${QT_QTCORE_LIBRARY})
  endif()

  # add QVTK plugin from sources
  # stand-alone as it doesn't depend on QVTK library
  add_library(QVTKWidgetPlugin
    SHARED
    ${PluginLibSrcs}
    ${PluginMocSrcs}
    )

  set_target_properties(QVTKWidgetPlugin PROPERTIES COMPILE_DEFINITIONS QT_NO_DEBUG)

  # link with Qt libs
  vtk_module_link_libraries(QVTKWidgetPlugin ${QT_LIBRARIES})

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
