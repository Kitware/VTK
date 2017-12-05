cmake_minimum_required(VERSION 3.3 FATAL_ERROR)
foreach(p
    CMP0071 # 3.10: Let AUTOMOC and AUTOUIC process GENERATED files
    )
  if(POLICY ${p})
    cmake_policy(SET ${p} NEW)
  endif()
endforeach()

project( GraphicsView )

find_package(OpenGL)

find_package(VTK COMPONENTS
  vtkCommonCore
  vtkGUISupportQt
  vtkGUISupportQtOpenGL
  vtkIOInfovis
  vtkViewsInfovis
)
include(${VTK_USE_FILE})

if("${VTK_QT_VERSION}" STREQUAL "")
  message(FATAL_ERROR "VTK was not built with Qt")
endif()

set( Srcs
  main.cpp
  OpenGLScene.cpp
  TreeRingViewItem.cpp
  GraphLayoutViewItem.cpp
  WebView.cpp
)

set( Hdrs
  GraphicsView.hpp
  OpenGLScene.hpp
  QBoolAnimation.h
  TreeRingViewItem.h
  GraphLayoutViewItem.h
  WebView.h
)

set( MOC_Hdrs
  OpenGLScene.hpp
  QBoolAnimation.h
  WebView.h
)

set( QRCs
  GraphicsView.qrc
)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS WebKitWidgets REQUIRED QUIET)

qt5_add_resources(QRC_Srcs ${QRCs} )

add_executable(qtgraphicsview MACOSX_BUNDLE
  ${Srcs} ${Hdrs} ${MOC_Hdrs} ${QRC_Srcs})
qt5_use_modules(qtgraphicsview Core Gui Widgets
                WebKit WebKitWidgets OpenGL OpenGLExtensions)
target_link_libraries(qtgraphicsview ${VTK_LIBRARIES})
