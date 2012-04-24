find_package(Qt4 REQUIRED QtCore QtGui QtWebKit QUIET)

# Rich-text view requires Qt >= 4.5.0
# Rich-text depends on Qt Webkit which is not portable on Unix (AIX & HP-UX)
QT4_WRAP_UI(UI_FILES vtkQtRichTextView.ui)
set(LibSrcs ${QVTKLibSrcs} ${UI_FILES} vtkQtRichTextView.cxx)
set(MocHeaders ${QVTKMocHeaders} vtkQtRichTextView.h)
if(QT_PHONON_FOUND AND APPLE)
  set(QT_USE_PHONON 1)
endif()
set(QT_USE_QTWEBKIT 1)

# import Qt4 build settings
set(QT_USE_QTNETWORK 1)
include(${QT_USE_FILE})

qt4_wrap_cpp(LibMocSrcs ${MocHeaders})

set(${vtk-module}_NO_HeaderTest 1)
vtk_module_library(${vtk-module} ${LibSrcs} ${LibMocSrcs})

target_link_libraries(${vtk-module} ${QT_LIBRARIES})
