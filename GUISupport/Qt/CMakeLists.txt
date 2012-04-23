
# set up sources to build
SET(QVTKLibSrcs
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
    vtkQtSQLDatabase.cxx
    vtkQtSQLQuery.cxx
    vtkQtTableModelAdapter.cxx
    vtkQtTimePointUtility.cxx
    vtkQtTreeModelAdapter.cxx
    )

SET(QVTKMocHeaders
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

IF(VTK_USE_X AND VTK_USE_TDX)
  SET(QVTKMocHeaders ${QVTKMocHeaders}
      vtkTDxQtUnixDevices.h
    )
  SET(QVTKLibSrcs ${QVTKLibSrcs}
      vtkTDxQtUnixDevices.cxx
    )
ENDIF()

SET(QVTKNonMocHeaders
    QVTKWin32Header.h
    vtkEventQtSlotConnect.h
    QVTKInteractor.h
    )

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

# add additional files if the user will allow Qt's OpenGL support
IF(VTK_USE_QVTK_QTOPENGL)
  SET(QT_USE_QTOPENGL 1)
  SET(QVTKMocHeaders ${QVTKMocHeaders}
    QVTKGraphicsItem.h
    QVTKWidget2.h
    )
  SET(QVTKLibSrcs ${QVTKLibSrcs}
    QVTKGraphicsItem.cxx
    QVTKWidget2.cxx
    )
ENDIF(VTK_USE_QVTK_QTOPENGL)

# add additional files depending on infovis and/or views
IF(VTK_USE_VIEWS)
  SET(QVTKLibSrcs ${QVTKLibSrcs}
    vtkQtAnnotationView.cxx
    vtkQtListView.cxx
    vtkQtRecordView.cxx
    vtkQtTableRepresentation.cxx
    vtkQtTableView.cxx
    vtkQtTreeView.cxx
    vtkQtView.cxx
    )
  SET_SOURCE_FILES_PROPERTIES(vtkQtView.cxx ABSTRACT )
  SET(QVTKMocHeaders ${QVTKMocHeaders}
    vtkQtAnnotationView.h
    vtkQtListView.h
    vtkQtRecordView.h
    vtkQtTableView.h
    vtkQtTreeView.h
    vtkQtView.h
    )
  SET(QVTKNonMocHeaders ${QVTKNonMocHeaders}
    vtkQtTableRepresentation.h
    )

  # Rich-text view requires Qt >= 4.5.0
  # Rich-text depends on Qt Webkit which is not portable on Unix (AIX & HP-UX)
  IF(QT_QTWEBKIT_FOUND)
    OPTION(VTK_QT_USE_WEBKIT "Option to use QT Webkit" ON)
    MARK_AS_ADVANCED(VTK_QT_USE_WEBKIT)
    IF( VTK_QT_USE_WEBKIT )
      QT4_WRAP_UI(UI_FILES vtkQtRichTextView.ui)
      SET(QVTKLibSrcs ${QVTKLibSrcs} ${UI_FILES} vtkQtRichTextView.cxx)
      SET(QVTKMocHeaders ${QVTKMocHeaders} vtkQtRichTextView.h)
      IF(QT_PHONON_FOUND AND APPLE)
        SET(QT_USE_PHONON 1)
      ENDIF(QT_PHONON_FOUND AND APPLE)
      SET(QT_USE_QTWEBKIT 1)
    ENDIF( VTK_QT_USE_WEBKIT )
  ENDIF(QT_QTWEBKIT_FOUND)
ENDIF(VTK_USE_VIEWS)

# Should we build the Qt charts - they are now deprecated.
VTK_DEPENDENT_OPTION(VTK_USE_QTCHARTS "Build Qt based charts - DEPRECATED" OFF
                     "VTK_USE_QT;VTK_USE_RENDERING" OFF)
if(VTK_USE_QTCHARTS)
  if(VTK_USE_VIEWS)
    set(QVTKLibSrcs ${QVTKLibSrcs}
      vtkQtBarChartView.cxx
      vtkQtChartRepresentation.cxx
      vtkQtChartView.cxx
      vtkQtLineChartView.cxx
      vtkQtStackedChartView.cxx
      vtkQtStatisticalBoxChartView.cxx
      )
    set(QVTKMocHeaders ${QVTKMocHeaders}
      vtkQtBarChartView.h
      vtkQtChartView.h
      vtkQtLineChartView.h
      vtkQtStackedChartView.h
      vtkQtStatisticalBoxChartView.h
      )
    set(QVTKNonMocHeaders ${QVTKNonMocHeaders}
      vtkQtChartRepresentation.h
      )
  endif()
endif()

# import Qt4 build settings
SET(QT_USE_QTNETWORK 1)
INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(QVTKLibMocSrcs ${QVTKMocHeaders})

VTK_ADD_LIBRARY(QVTK ${QVTKLibSrcs} ${QVTKLibMocSrcs})
# Add target specific compile flags for ABI setttings
IF(VTK_ABI_CXX_FLAGS)
  SET_PROPERTY(TARGET QVTK APPEND PROPERTY COMPILE_FLAGS "${VTK_ABI_CXX_FLAGS}")
ENDIF(VTK_ABI_CXX_FLAGS)

# Apply user-defined properties to the library target.
IF(VTK_LIBRARY_PROPERTIES)
  SET_TARGET_PROPERTIES(QVTK PROPERTIES ${VTK_LIBRARY_PROPERTIES})
ENDIF(VTK_LIBRARY_PROPERTIES)

TARGET_LINK_LIBRARIES(QVTK
  ${QT_LIBRARIES}
  vtkRendering
  vtkIO
  vtkGraphics
  vtkImaging
  vtkCommon)

# add extra dependencies if necessary
IF(VTK_USE_VIEWS)
  TARGET_LINK_LIBRARIES(QVTK vtkViews)
ENDIF(VTK_USE_VIEWS)

IF(VTK_USE_QVTK_QTOPENGL)
  TARGET_LINK_LIBRARIES(QVTK ${OPENGL_gl_LIBRARY})
ENDIF(VTK_USE_QVTK_QTOPENGL)

IF(VTK_USE_X)
  TARGET_LINK_LIBRARIES(QVTK ${X11_LIBRARIES})
ENDIF()

IF(APPLE)
  IF(VTK_USE_CARBON)
    TARGET_LINK_LIBRARIES( QVTK "-framework Carbon" )
  ENDIF(VTK_USE_CARBON)
ENDIF(APPLE)

if(VTK_USE_QTCHARTS)
  add_subdirectory(Chart)
  target_link_libraries(QVTK vtkQtChart)
endif()

# recurse into testing directory if testing is on
IF(BUILD_TESTING)
  ADD_SUBDIRECTORY(Testing/Cxx)
ENDIF(BUILD_TESTING)

# wrapping for python
if(VTK_WRAP_PYTHON_SIP)

  #### wrap vtkObject derived classes with the VTK python wrapper generator
  # list header files here for wrapping
  set(QVTK_PythonHeaders
    QVTKInteractor.h
    vtkEventQtSlotConnect.h
    vtkQtSQLDatabase.h
    vtkQtSQLQuery.h
    vtkQtTimePointUtility.h
    )
  if(VTK_USE_VIEWS)
    set(QVTK_PythonHeaders ${QVTK_PythonHeaders}
      vtkQtAnnotationView.h
      vtkQtListView.h
      vtkQtRecordView.h
      vtkQtTableRepresentation.h
      vtkQtTableView.h
      vtkQtTreeView.h
      vtkQtView.h
      )
  endif(VTK_USE_VIEWS)

  if(VTK_USE_QTCHARTS)
    set(QVTK_PythonHeaders ${QVTK_PythonHeaders}
      vtkQtBarChartView.h
      vtkQtChartRepresentation.h
      vtkQtChartView.h
      vtkQtLineChartView.h
      vtkQtStackedChartView.h
      vtkQtStatisticalBoxChartView.h
      )
  endif()
  IF(VTK_USE_VIEWS AND VTK_QT_USE_WEBKIT)
    set(QVTK_PythonHeaders ${QVTK_PythonHeaders}
    vtkQtRichTextView.h
    )
  ENDIF(VTK_USE_VIEWS AND VTK_QT_USE_WEBKIT)

  VTK_WRAP_PYTHON3(vtkQtPython QVTK_PythonSrcs "${QVTK_PythonHeaders}")
  VTK_ADD_LIBRARY(vtkQtPythonD ${QVTK_PythonSrcs})
  TARGET_LINK_LIBRARIES(vtkQtPythonD QVTK)
  IF(VTK_USE_VIEWS)
    TARGET_LINK_LIBRARIES(vtkQtPythonD vtkViewsPythonD)
  ENDIF(VTK_USE_VIEWS)
  TARGET_LINK_LIBRARIES(vtkQtPythonD vtkRenderingPythonD vtkPythonCore)

  PYTHON_ADD_MODULE(vtkQtPython vtkQtPythonInit.cxx)
  TARGET_LINK_LIBRARIES(vtkQtPython vtkQtPythonD)

  #### wrap QObject derived classes with SIP
  # list .sip files here for wrapping
  set(SIP_FILES
    QVTKWidget.sip
    QVTKInteractor.sip
    QVTKInteractorAdapter.sip
    vtkQtAbstractModelAdapter.sip
    vtkQtAnnotationLayersModelAdapter.sip
    vtkQtTableModelAdapter.sip
    vtkQtTreeModelAdapter.sip
    )
  if(VTK_USE_QVTK_QTOPENGL)
    set(SIP_FILES
      ${SIP_FILES}
      QVTKWidget2.sip
      QVTKGraphicsItem.sip
      )
  endif(VTK_USE_QVTK_QTOPENGL)

  # auto generate the module .sip file from the list of .sip files above
  set(SIP_MODULE QVTKPython)
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${SIP_MODULE}.sip.in
    "%Module vtk.${SIP_MODULE} 0\n\n")

  set(SIP_OUT)
  foreach(f ${SIP_FILES})
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/${SIP_MODULE}.sip.in "%Include ${f}\n")
    get_filename_component(f1 "${f}" NAME_WE)
    set(SIP_OUT ${SIP_OUT} ${CMAKE_CURRENT_BINARY_DIR}/sip${SIP_MODULE}${f1}.cpp)
  endforeach(f ${SIP_FILES})

  set(MODULE_SIP_FILE ${CMAKE_CURRENT_BINARY_DIR}/${SIP_MODULE}.sip)
  configure_file(${CMAKE_CURRENT_BINARY_DIR}/${SIP_MODULE}.sip.in ${MODULE_SIP_FILE} COPY_ONLY)

  set(SIP_OUT ${SIP_OUT} ${CMAKE_CURRENT_BINARY_DIR}/sip${SIP_MODULE}cmodule.cpp)
  set(SIP_FILES ${SIP_FILES} ${MODULE_SIP_FILE})

  # find .sip files for Qt
  find_path(SIP_PYQT_DIR NAMES QtCore/QtCoremod.sip
    PATHS ${PYTHON_INCLUDE_PATH}/../Lib/site-packages/PyQt4/sip/PyQt4
          ${PYTHON_INCLUDE_PATH}/../sip/PyQt4
          ${PYTHON_INCLUDE_PATH}/../Versions/2.5/share/sip/PyQt4
          ${PYTHON_INCLUDE_PATH}/../share/sip/PyQt4
          /usr/share/sip/PyQt4
          /usr/local/share/sip/PyQt4
    DOC "Root directory containing all PyQt4 sip files."
    )

  # build command args for sip
  if(Q_WS_X11)
    set(SIP_FLAGS ${SIP_FLAGS} -t WS_X11)
  elseif(Q_WS_WIN)
    set(SIP_FLAGS ${SIP_FLAGS} -t WS_WIN)
  elseif(Q_WS_MAC)
    set(SIP_FLAGS ${SIP_FLAGS} -t WS_MACX)
  endif()
  set(SIP_FLAGS ${SIP_FLAGS} -t Qt_${QT_VERSION_MAJOR}_${QT_VERSION_MINOR}_0)
  set(SIP_FLAGS ${SIP_FLAGS}
    -I ${SIP_PYQT_DIR}
    -I ${CMAKE_CURRENT_SOURCE_DIR}
    -I ${VTK_BINARY_DIR}/Rendering
    -I ${VTK_BINARY_DIR}/Filtering
    -I ${VTK_BINARY_DIR}/Common
    )
  IF(VTK_USE_VIEWS)
    set(SIP_FLAGS ${SIP_FLAGS}
      -I ${VTK_BINARY_DIR}/Views
      )
  ENDIF(VTK_USE_VIEWS)

  # call sip on our files
  add_custom_command(
    OUTPUT ${SIP_OUT}
    COMMAND ${SIP_EXECUTABLE}
    ARGS -c "${CMAKE_CURRENT_BINARY_DIR}" ${SIP_FLAGS} ${MODULE_SIP_FILE}
    DEPENDS ${SIP_FILES}
    )

  # create the sip module
  include_directories(${SIP_INCLUDE_DIR})
  include_directories("${PYTHON_INCLUDE_PATH}")
  VTK_ADD_LIBRARY(${SIP_MODULE} MODULE ${SIP_OUT} ${SIP_FILES})
  target_link_libraries(${SIP_MODULE} QVTK vtkRenderingPythonD)
  set_target_properties(${SIP_MODULE} PROPERTIES PREFIX "")
  IF(WIN32 AND NOT CYGWIN)
    SET_TARGET_PROPERTIES(${SIP_MODULE} PROPERTIES SUFFIX ".pyd")
  ENDIF(WIN32 AND NOT CYGWIN)
  get_target_property(lib_loc ${SIP_MODULE} LOCATION)
  add_custom_command(TARGET ${SIP_MODULE} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${lib_loc}" "${VTK_BINARY_DIR}/Wrapping/Python/vtk/"
    )


  SET_TARGET_PROPERTIES(vtkQtPython QVTKPython PROPERTIES SKIP_BUILD_RPATH 1)

  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL(TARGETS vtkQtPythonD
      EXPORT ${VTK_INSTALL_EXPORT_NAME}
      RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeLibraries
      LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development)
    IF(VTK_INSTALL_PYTHON_USING_CMAKE)
      INSTALL(TARGETS vtkQtPython QVTKPython
        EXPORT ${VTK_INSTALL_EXPORT_NAME}
        RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeLibraries
        LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries
        ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development)
    ENDIF(VTK_INSTALL_PYTHON_USING_CMAKE)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)

  IF(BUILD_TESTING)
    ADD_SUBDIRECTORY(Testing/Python)
  ENDIF(BUILD_TESTING)

endif(VTK_WRAP_PYTHON_SIP)


# Configure the VTKConfigQt.cmake support file.
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/VTKConfigQt.cmake.in
               ${VTK_BINARY_DIR}/VTKConfigQt.cmake @ONLY IMMEDIATE)

# build plugin
IF(BUILD_SHARED_LIBS)
  SET ( PluginLibSrcs
        Q4VTKWidgetPlugin.cxx
        )

  SET ( PluginMocHeaders
        Q4VTKWidgetPlugin.h
        )

  ADD_DEFINITIONS(-DQT_PLUGIN)
  INCLUDE_DIRECTORIES(${QT_QTDESIGNER_INCLUDE_DIR})
  QT4_WRAP_CPP ( PluginMocSrcs ${PluginMocHeaders} )

  # add QVTK plugin from sources
  # stand-alone as it doesn't depend on QVTK library
  VTK_ADD_LIBRARY ( QVTKWidgetPlugin
    SHARED
    ${PluginLibSrcs}
    ${PluginMocSrcs}
  )

  SET_TARGET_PROPERTIES(QVTKWidgetPlugin PROPERTIES COMPILE_DEFINITIONS QT_NO_DEBUG)

  # link with Qt libs
  TARGET_LINK_LIBRARIES( QVTKWidgetPlugin
    ${QT_QTGUI_LIBRARY}
    ${QT_QTCORE_LIBRARY}
  )

  # install rules

  # The VTK_INSTALL_QT_PLUGIN_DIR variable sets the location
  # in which the Qt plugin will be installed.  It may or may not contain
  # variable references to CMAKE_INSTALL_PREFIX and VTK_INSTALL_QT_DIR.
  # The default is to install to VTK_INSTALL_QT_DIR under the installation
  # prefix.  The default VTK_INSTALL_QT_DIR will allow the designer plugin
  # path to be set to vtk-install-prefix/plugins to get the plugin.
  IF(NOT VTK_INSTALL_QT_DIR)
    SET(VTK_INSTALL_QT_DIR /plugins/designer)
  ENDIF(NOT VTK_INSTALL_QT_DIR)

  # If no runtime is to be installed then do not install the qt plugin.
  IF(VTK_INSTALL_NO_RUNTIME)
    SET(VTK_INSTALL_NO_QT_PLUGIN 1)
  ENDIF(VTK_INSTALL_NO_RUNTIME)

  IF(NOT VTK_INSTALL_NO_QT_PLUGIN)
    # Set default plugin install directory.
    SET(DOLLAR "$")
    IF(DEFINED VTK_INSTALL_QT_PLUGIN_DIR)
    ELSE(DEFINED VTK_INSTALL_QT_PLUGIN_DIR)
      SET(VTK_INSTALL_QT_PLUGIN_DIR "${DOLLAR}{CMAKE_INSTALL_PREFIX}${DOLLAR}{VTK_INSTALL_QT_DIR}"
        CACHE STRING "Directory in which the VTK Qt plugin is placed during installation.")
      MARK_AS_ADVANCED(VTK_INSTALL_QT_PLUGIN_DIR)
    ENDIF(DEFINED VTK_INSTALL_QT_PLUGIN_DIR)

    # Configure the plugin install script.  This is used instead of
    # INSTALL TARGETS to allow the plugin to be installed outside the
    # main install prefix.  Attach the script as a post-install script.
    CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/PluginInstall.cmake.in
                   ${CMAKE_CURRENT_BINARY_DIR}/PluginInstall.cmake
                   @ONLY IMMEDIATE)
    SET_TARGET_PROPERTIES(QVTK PROPERTIES POST_INSTALL_SCRIPT
      ${CMAKE_CURRENT_BINARY_DIR}/PluginInstall.cmake
      )
  ENDIF(NOT VTK_INSTALL_NO_QT_PLUGIN)
ENDIF(BUILD_SHARED_LIBS)

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${QVTKMocHeaders}
    ${QVTKNonMocHeaders}
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
    COMPONENT Development
    )

  INSTALL(FILES
    "${VTK_BINARY_DIR}/VTKConfigQt.cmake"
    DESTINATION ${VTK_INSTALL_PACKAGE_DIR_CM24}
    COMPONENT Development
    )
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

IF(NOT VTK_INSTALL_NO_LIBRARIES)
  INSTALL(TARGETS QVTK
    EXPORT ${VTK_INSTALL_EXPORT_NAME}
    RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeLibraries
    LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries
    ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.ctest.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.ctest" @ONLY IMMEDIATE)
