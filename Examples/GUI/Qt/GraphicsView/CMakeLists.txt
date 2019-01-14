cmake_minimum_required(VERSION 3.3...3.12 FATAL_ERROR)
project(GraphicsView)

find_package(VTK
  COMPONENTS
    CommonCore
    GUISupportQt
    GUISupportQtOpenGL
    IOInfovis
    InfovisCore
    ViewsInfovis)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

find_package(Qt5 COMPONENTS WebKitWidgets)
if (NOT TARGET Qt5::WebKitWidgets)
  message("Skipping example: ${Qt5_NOT_FOUND_MESSAGE}")
  return ()
endif ()

set(Srcs
  GraphLayoutViewItem.cpp
  OpenGLScene.cpp
  TreeRingViewItem.cpp
  WebView.cpp
  main.cpp)

set(Hdrs
  GraphLayoutViewItem.h
  GraphicsView.hpp
  OpenGLScene.hpp
  QBoolAnimation.h
  TreeRingViewItem.h
  WebView.h)

set(QRCs
  GraphicsView.qrc)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(qtgraphicsview MACOSX_BUNDLE
  ${Srcs} ${Hdrs} ${QRCs})
target_link_libraries(qtgraphicsview
  PRIVATE
    ${VTK_LIBRARIES}
    Qt5::WebKitWidgets)
vtk_module_autoinit(
  TARGETS qtgraphicsview
  MODULES ${VTK_LIBRARIES})
