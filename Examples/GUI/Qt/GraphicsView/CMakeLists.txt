if(NOT VTK_USE_QT5)
  find_package(Qt4 REQUIRED)
  set(QT_USE_QTOPENGL 1)
  set(QT_USE_QTWEBKIT 1)
  include(${QT_USE_FILE})
else()
  find_package(Qt5WebKitWidgets REQUIRED QUIET)
  include_directories(${Qt5WebKitWidgets_INCLUDE_DIRS})
  add_definitions(${Qt5WebKitWidgets_DEFINITIONS})
  set(QT_LIBRARIES ${Qt5WebKitWidgets_LIBRARIES})
endif()

find_package(OpenGL)

if(NOT VTK_BINARY_DIR)
  find_package(VTK REQUIRED)
  include(${VTK_USE_FILE})
endif()

if(NOT QT_QTWEBKIT_FOUND OR QT_VERSION_MINOR LESS 6)
  message(STATUS "VTK isn't configured to use QtOpenGL, QtWebKit wasn't found, or Qt 4.6 wasn't found.  GraphicsView example is disabled.")
else()

  qt4_add_resources(qrcfiles GraphicsView.qrc)

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
    ${qrcfiles}
  )

  target_link_libraries(qtgraphicsview ${QT_LIBRARIES} ${VTK_LIBRARIES})

endif()
