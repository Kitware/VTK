include(vtkQt)

set(LibSrcs
  vtkQImageToImageSource.cxx
  vtkQtInitialization.cxx
  vtkQtLabelRenderStrategy.cxx
  vtkQtStringToImage.cxx
  vtkQtTreeRingLabelMapper.cxx
  )

find_package(Qt5 COMPONENTS Widgets REQUIRED QUIET)
include_directories(${Qt5Widgets_INCLUDE_DIRS})
add_definitions(${Qt5Widgets_DEFINITIONS})
set(QT_LIBRARIES ${Qt5Widgets_LIBRARIES})

# When this module is loaded by an app, load Qt too.
vtk_module_export_code_find_package(Qt5 COMPONENTS Widgets)

vtk_module_library(${vtk-module} ${LibSrcs})
vtk_module_link_libraries(${vtk-module} LINK_PRIVATE ${QT_LIBRARIES})
