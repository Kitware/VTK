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
SET(DESIRED_QT_VERSION ${VTK_DESIRED_QT_VERSION} CACHE FILEPATH "")
FIND_PACKAGE(Qt)
IF(QT_USE_FILE)
  SET(QT_USE_QT3SUPPORT TRUE)
  INCLUDE(${QT_USE_FILE})
ELSE(QT_USE_FILE)
  SET(QT_LIBRARIES   ${QT_QT_LIBRARY})
ENDIF(QT_USE_FILE)

SET (SRCS
  main.cxx
)

# Use the include path and library for Qt that is used by VTK.
INCLUDE_DIRECTORIES(
        ${QT_INCLUDE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

IF(DESIRED_QT_VERSION MATCHES 4)

  # instead of a complete port to Qt4, lets try out uic3 and Qt3Support

  # cmake 2.4.0 didn't have support for uic3, help out
  IF(NOT QT_UIC3_EXECUTABLE)
    GET_FILENAME_COMPONENT(QTPATH "${QT_UIC_EXECUTABLE}" PATH)
    FIND_PROGRAM(QT_UIC3_EXECUTABLE
                 NAMES uic3
                 PATHS ${QTPATH}
                 NO_DEFAULT_PATH
                 )
  ENDIF(NOT QT_UIC3_EXECUTABLE)


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
ELSE(DESIRED_QT_VERSION MATCHES 4)
  QT_WRAP_UI( qtevents UI_HDRS UI_SRCS GUI.ui )
  # help cmake with Qt depends
  SET_SOURCE_FILES_PROPERTIES(main.cxx PROPERTIES
                              OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/GUI.h)
ENDIF(DESIRED_QT_VERSION MATCHES 4)


SET_SOURCE_FILES_PROPERTIES(GUI.cxx PROPERTIES
                            OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/GUI.ui.h)

ADD_EXECUTABLE( qtevents MACOSX_BUNDLE ${SRCS} ${UI_SRCS})

TARGET_LINK_LIBRARIES( qtevents
  QVTK
  ${QT_LIBRARIES}
  vtkRendering
  vtkGraphics
  vtkIO
  vtkCommon
)


