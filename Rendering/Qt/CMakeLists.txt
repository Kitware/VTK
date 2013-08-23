include(vtkQt)

set(LibSrcs
  vtkQImageToImageSource.cxx
  vtkQtInitialization.cxx
  vtkQtLabelRenderStrategy.cxx
  vtkQtStringToImage.cxx
  vtkQtTreeRingLabelMapper.cxx
  )

if(VTK_QT_VERSION VERSION_GREATER "4")
  find_package(Qt5Widgets REQUIRED QUIET)
  include_directories(${Qt5Widgets_INCLUDE_DIRS})
  add_definitions(${Qt5Widgets_DEFINITIONS})
  set(QT_LIBRARIES ${Qt5Widgets_LIBRARIES})
else()
  # import Qt4 build settings
  find_package(Qt4 REQUIRED QtCore QtGui QUIET)
  include(${QT_USE_FILE})
endif()

vtk_module_library(${vtk-module} ${LibSrcs})
target_link_libraries(${vtk-module} LINK_PRIVATE ${QT_LIBRARIES})
