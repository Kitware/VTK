PROJECT(StatsView)

IF(NOT VTK_BINARY_DIR)
FIND_PACKAGE(VTK)
IF(NOT VTK_DIR)
  MESSAGE(FATAL_ERROR "Please set VTK_DIR.")
ENDIF(NOT VTK_DIR)
INCLUDE(${VTK_USE_FILE})
ENDIF(NOT VTK_BINARY_DIR)

# use what QVTK built with
SET(QT_MOC_EXECUTABLE ${VTK_QT_MOC_EXECUTABLE} CACHE FILEPATH "")
SET(QT_UIC_EXECUTABLE ${VTK_QT_UIC_EXECUTABLE} CACHE FILEPATH "")
SET(QT_QMAKE_EXECUTABLE ${VTK_QT_QMAKE_EXECUTABLE} CACHE FILEPATH "")
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})


# Use the include path and library for Qt that is used by VTK.
INCLUDE_DIRECTORIES(
  ${QT_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)


# Set your files and resources here  
SET(StatsViewSrcs main.cxx StatsView.cxx)
SET(StatsViewUI StatsView.ui)
SET(StatsViewHeaders StatsView.h)
SET(StatsViewResources Icons/icons.qrc)

# The rest should just work (sure...)
QT4_WRAP_UI(UISrcs ${StatsViewUI})
QT4_WRAP_CPP(MOCSrcs ${StatsViewHeaders} )
QT4_ADD_RESOURCES(ResourceSrcs ${StatsViewResources})

SOURCE_GROUP("Resources" FILES
  ${StatsViewUI}
  ${StatsViewResources}
  ${EXE_ICON}
)

SOURCE_GROUP("Generated" FILES
  ${UISrcs}
  ${MOCSrcs}
  ${ResourceSrcs}
  ${RCS_SOURCES}
)

ADD_DEFINITIONS(-DQT_GUI_LIBS -DQT_CORE_LIB -DQT3_SUPPORT)
SET_SOURCE_FILES_PROPERTIES(${StatsViewSrcs} PROPERTIES
                            OBJECT_DEPENDS "${UISrcs}")

# It's nice to have the ui in the windows project file...just double click on it
# and designer comes up on that ui file :)
IF (${CMAKE_BUILD_TOOL} MATCHES "msdev")
  SET (StatsViewSrcs ${StatsViewSrcs} ${StatsViewUI})
ENDIF (${CMAKE_BUILD_TOOL} MATCHES "msdev")
IF (${CMAKE_BUILD_TOOL} MATCHES "devenv")
  SET (StatsViewSrcs ${StatsViewSrcs} ${StatsViewUI})
ENDIF (${CMAKE_BUILD_TOOL} MATCHES "devenv")

ADD_EXECUTABLE( StatsView MACOSX_BUNDLE ${StatsViewSrcs} ${UISrcs} ${MOCSrcs} ${ResourceSrcs})

TARGET_LINK_LIBRARIES( StatsView
  QVTK
  ${QT_LIBRARIES}
  vtkRendering
  vtkGraphics
  vtkIO
  vtkCommon
  vtkInfovis
  vtkViews
)



