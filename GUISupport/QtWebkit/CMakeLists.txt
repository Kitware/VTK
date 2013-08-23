include(vtkQt)

# Rich-text view requires Qt >= 4.5.0
# Rich-text depends on Qt Webkit which is not portable on Unix (AIX & HP-UX)

set(LibSrcs ${QVTKLibSrcs} vtkQtRichTextView.cxx)
set(MocHeaders ${QVTKMocHeaders} vtkQtRichTextView.h)

if(VTK_QT_VERSION VERSION_GREATER "4")
  find_package(Qt5WebKitWidgets REQUIRED QUIET)
  include_directories(${Qt5WebKitWidgets_INCLUDE_DIRS})
  add_definitions(${Qt5WebKitWidgets_DEFINITIONS})

  qt5_wrap_ui(UI_FILES vtkQtRichTextView.ui)
  qt5_wrap_cpp(LibMocSrcs ${MocHeaders})

  set(QT_LIBRARIES ${Qt5WebKitWidgets_LIBRARIES})
else()
  find_package(Qt4 REQUIRED QtCore QtGui QtWebKit QUIET)

  # import Qt4 build settings
  if(QT_PHONON_FOUND AND APPLE)
    set(QT_USE_PHONON 1)
  endif()
  set(QT_USE_QTWEBKIT 1)
  set(QT_USE_QTNETWORK 1)
  include(${QT_USE_FILE})

  qt4_wrap_ui(UI_FILES vtkQtRichTextView.ui)
  qt4_wrap_cpp(LibMocSrcs ${MocHeaders})
endif()

set(${vtk-module}_NO_HeaderTest 1)
vtk_module_library(${vtk-module} ${LibSrcs} ${UI_FILES} ${LibMocSrcs})

target_link_libraries(${vtk-module} LINK_PRIVATE ${QT_LIBRARIES})
