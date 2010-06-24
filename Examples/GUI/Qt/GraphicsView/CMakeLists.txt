
find_package(Qt4 REQUIRED)
set(QT_USE_QTOPENGL 1)
set(QT_USE_QTWEBKIT 1)
include(${QT_USE_FILE})

find_package(OpenGL)

if(NOT VTK_BINARY_DIR)
  find_package(VTK REQUIRED)
  include(${VTK_USE_FILE})
endif(NOT VTK_BINARY_DIR)

if(NOT VTK_USE_QVTK_QTOPENGL)
  message(WARNING "VTK isn't configured to use QtOpenGL.  GraphicsView example is disabled.")
else(NOT VTK_USE_QVTK_QTOPENGL)

qt4_wrap_cpp(mocs
  OpenGLScene.hpp
  QBoolAnimation.h
  WebView.h
  )

add_executable(qtgraphicsview
  main.cpp
  OpenGLScene.cpp
  TreeRingViewItem.cpp
  GraphLayoutViewItem.cpp
  WebView.cpp
  ${mocs}
  )

target_link_libraries(qtgraphicsview QVTK vtkRendering)
target_link_libraries(qtgraphicsview ${QT_LIBRARIES})

endif(NOT VTK_USE_QVTK_QTOPENGL)
