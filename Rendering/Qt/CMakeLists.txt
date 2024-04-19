set(classes
  vtkQImageToImageSource
  vtkQtInitialization
  vtkQtLabelRenderStrategy
  vtkQtStringToImage
  vtkQtTreeRingLabelMapper
  )

vtk_module_add_module(VTK::RenderingQt
  CLASSES ${classes})

include(vtkQt)
vtk_module_find_package(
  PACKAGE     "Qt${vtk_qt_major_version}"
  COMPONENTS  Widgets
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         "Qt${vtk_qt_major_version}_VERSION")
vtk_module_link(VTK::RenderingQt PUBLIC "Qt${vtk_qt_major_version}::Widgets")
vtk_add_test_mangling(VTK::RenderingQt)
