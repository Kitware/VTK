set(LibSrcs
  vtkQImageToImageSource.cxx
  vtkQtInitialization.cxx
  vtkQtLabelRenderStrategy.cxx
  vtkQtStringToImage.cxx
  vtkQtTreeRingLabelMapper.cxx
  )

# import Qt4 build settings
find_package(Qt4 REQUIRED QtCore QtGui QUIET)
include(${QT_USE_FILE})

vtk_module_library(${vtk-module} ${LibSrcs})
target_link_libraries(${vtk-module} ${QT_LIBRARIES})
