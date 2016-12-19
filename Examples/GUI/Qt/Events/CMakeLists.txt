cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

project(QtEvents)

find_package(VTK COMPONENTS
  vtkCommonCore
  vtkFiltersSources
  vtkGUISupportQt
)
include(${VTK_USE_FILE})

if("${VTK_QT_VERSION}" STREQUAL "")
  message(FATAL_ERROR "VTK was not built with Qt")
endif()

set( Srcs main.cxx GUI4.cxx )

set( Hdrs GUI4.h )

set( MOC_Hdrs GUI4.h )

set( UIs GUI4.ui )

# Use the include path and library for Qt that is used by VTK.
include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

if(VTK_QT_VERSION VERSION_GREATER "4")
  find_package(Qt5 COMPONENTS Widgets)
  qt5_wrap_ui(UI_Srcs ${UIs})

  add_executable(qtevents
    MACOSX_BUNDLE  ${Srcs} ${Hdrs} ${UI_Srcs} ${MOC_Hdrs})
  qt5_use_modules(qtevents Core Gui Widgets)
  target_link_libraries(qtevents ${VTK_LIBRARIES})
else()
  find_package(Qt4 REQUIRED)
  include(${QT_USE_FILE})
  # Use what VTK built with
  set(QT_QMAKE_EXECUTABLE ${VTK_QT_QMAKE_EXECUTABLE} CACHE FILEPATH "")
  set(QT_MOC_EXECUTABLE ${VTK_QT_MOC_EXECUTABLE} CACHE FILEPATH "")
  set(QT_UIC_EXECUTABLE ${VTK_QT_UIC_EXECUTABLE} CACHE FILEPATH "")
  qt4_wrap_ui(UI_Srcs ${UIs})

  add_executable(qtevents MACOSX_BUNDLE ${Srcs} ${Hdrs} ${UI_Srcs} ${MOC_Hdrs})
  target_link_libraries(qtevents
    ${QT_LIBRARIES}
    ${VTK_LIBRARIES}
  )
endif()
