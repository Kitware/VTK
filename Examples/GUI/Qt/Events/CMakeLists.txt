PROJECT(QtEvents)

FIND_PACKAGE(VTK)
IF(NOT VTK_DIR)
  MESSAGE(FATAL_ERROR "Please set VTK_DIR.")
ENDIF(NOT VTK_DIR)
INCLUDE(${VTK_USE_FILE})

# use what QVTK built with
SET(QT_QMAKE_EXECUTABLE ${VTK_QT_QMAKE_EXECUTABLE} CACHE FILEPATH "")
SET(QT_MOC_EXECUTABLE ${VTK_QT_MOC_EXECUTABLE} CACHE FILEPATH "")
SET(QT_UIC_EXECUTABLE ${VTK_QT_UIC_EXECUTABLE} CACHE FILEPATH "")
SET(QT_MAX_VERSION "4.9.9")
FIND_PACKAGE(Qt)

SET (SRCS
  main.cxx
)

# Use the include path and library for Qt that is used by VTK.
INCLUDE_DIRECTORIES(
        ${QT_INCLUDE_DIR}
        ${QT_QTGUI_INCLUDE_DIR}
        ${QT_QTCORE_INCLUDE_DIR}
        ${QT_QT3SUPPORT_INCLUDE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

IF(QT_INST_MAJOR_VERSION GREATER 3)

  # instead of a complete port to Qt4, lets try out uic3 and Qt3Support

  ADD_DEFINITIONS(-DQT_GUI_LIBS -DQT_CORE_LIB -DQT3_SUPPORT)
  STRING(REGEX REPLACE "uic" "uic3" QT_UIC3_EXECUTABLE "${QT_UIC_EXECUTABLE}")
  ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/GUI.h
                     COMMAND ${QT_UIC3_EXECUTABLE}
                     ARGS -o ${CMAKE_CURRENT_BINARY_DIR}/GUI.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/GUI.ui
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/GUI.ui
                     )
  ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/GUI.cxx
                     COMMAND ${QT_UIC3_EXECUTABLE}
                     ARGS -impl ${CMAKE_CURRENT_BINARY_DIR}/GUI.h
                          -o ${CMAKE_CURRENT_BINARY_DIR}/GUI.cxx
                          ${CMAKE_CURRENT_SOURCE_DIR}/GUI.ui
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/GUI.ui 
                             ${CMAKE_CURRENT_BINARY_DIR}/GUI.h
                     )
  GET_DIRECTORY_PROPERTY(moc_includes_tmp INCLUDE_DIRECTORIES)
  SET(moc_includes)
  FOREACH(it ${moc_includes_tmp})
    SET(moc_includes ${moc_includes} "-I${it}")
  ENDFOREACH(it)

  ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/moc_GUI.cxx
                     COMMAND ${QT_MOC_EXECUTABLE}
                     ARGS ${moc_includes} -o ${CMAKE_CURRENT_BINARY_DIR}/moc_GUI.cxx
                          ${CMAKE_CURRENT_BINARY_DIR}/GUI.h
                     DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/GUI.h)
  
  SET(UI_SRCS GUI.cxx moc_GUI.cxx)
ELSE(QT_INST_MAJOR_VERSION GREATER 3)
  QT_WRAP_UI( qtevents UI_HDRS UI_SRCS GUI.ui )
  # help cmake with Qt depends
  SET_SOURCE_FILES_PROPERTIES(main.cxx PROPERTIES
                              OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/GUI.h)
ENDIF(QT_INST_MAJOR_VERSION GREATER 3)


SET_SOURCE_FILES_PROPERTIES(GUI.cxx PROPERTIES
                            OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/GUI.ui.h)

ADD_EXECUTABLE( qtevents MACOSX_BUNDLE ${SRCS} ${UI_SRCS})

# note: if you use Mesa in VTK and get link errors about opengl symbols,
#       those errors are coming from the Qt library needing to link with OpenGL.
#       Either link OpenGL in, configure Qt to dlopen the GL library, 
#       or configure Qt without OpenGL support.

IF(QT_INST_MAJOR_VERSION GREATER 3)
  IF(APPLE)
    SET(QT_LIBS   "-framework Qt3Support -framework QtGui" "-framework QtCore")
    SET_TARGET_PROPERTIES(qtevents PROPERTIES LINK_FLAGS "-F${QT_LIBRARY_DIR}")
  ELSE(APPLE)
    IF(CMAKE_BUILD_TYPE)
      SET(QT_LIBS optimized ${QT_QT3SUPPORT_LIBRARY} debug ${QT_QT3SUPPORT_LIBRARY_DEBUG}
                  optimized ${QT_QTGUI_LIBRARY} debug ${QT_QTGUI_LIBRARY_DEBUG}
                  optimized ${QT_QTCORE_LIBRARY} debug ${QT_QTCORE_LIBRARY_DEBUG} )
    ELSE(CMAKE_BUILD_TYPE)
      IF(CMAKE_CONFIGURATION_TYPES)
        SET(QT_LIBS optimized ${QT_QT3SUPPORT_LIBRARY} debug ${QT_QT3SUPPORT_LIBRARY_DEBUG}
                    optimized ${QT_QTGUI_LIBRARY} debug ${QT_QTGUI_LIBRARY_DEBUG}
                    optimized ${QT_QTCORE_LIBRARY} debug ${QT_QTCORE_LIBRARY_DEBUG} )
      ELSE(CMAKE_CONFIGURATION_TYPES)
        SET(QT_LIBS ${QT_QT3SUPPORT_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTCORE_LIBRARY})
      ENDIF(CMAKE_CONFIGURATION_TYPES)
    ENDIF(CMAKE_BUILD_TYPE)
  ENDIF(APPLE)
ELSE(QT_INST_MAJOR_VERSION GREATER 3)
  SET(QT_LIBS   ${QT_QT_LIBRARY})
ENDIF(QT_INST_MAJOR_VERSION GREATER 3)


TARGET_LINK_LIBRARIES( qtevents
  QVTK
  ${QT_LIBS}
  vtkRendering
  vtkGraphics
  vtkIO
  vtkCommon
)


