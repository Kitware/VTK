if(NOT VTK_LEGACY_REMOVE)
  include(vtkQt)

  # Rich-text view requires Qt >= 4.5.0
  # Rich-text depends on Qt Webkit which is not portable on Unix (AIX & HP-UX)
  set(LibSrcs ${QVTKLibSrcs} vtkQtRichTextView.cxx)
  set(MocHeaders ${QVTKMocHeaders} vtkQtRichTextView.h)
  find_package(Qt5 COMPONENTS WebKitWidgets QUIET)
  if(Qt5WebKitWidgets_FOUND)
    qt5_wrap_ui(UI_FILES vtkQtRichTextView.ui)
    qt5_wrap_cpp(LibMocSrcs ${MocHeaders})

    # When this module is loaded by an app, load Qt too.
    vtk_module_export_code_find_package(Qt5 COMPONENTS WebKitWidgets)

    set(${vtk-module}_NO_HeaderTest 1)
    vtk_module_library(${vtk-module} ${LibSrcs} ${UI_FILES} ${LibMocSrcs})
    vtk_module_link_libraries(${vtk-module} LINK_PRIVATE Qt5::WebKitWidgets)
  else()
    message(STATUS "Qt5WebKitWidgets not found. vtkQtRichTextView is disabled.")
  endif()
endif() # NOT VTK_LEGACY_REMOVE
