set(classes
  vtkQImageToImageSource
  vtkQtInitialization
  vtkQtLabelRenderStrategy
  vtkQtStringToImage
  vtkQtTreeRingLabelMapper
  )

vtk_module_add_module(VTK::RenderingQt
  CLASSES ${classes})

vtk_module_find_package(
  PACKAGE     Qt5
  COMPONENTS  Widgets
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         Qt5_VERSION)
vtk_module_link(VTK::RenderingQt PUBLIC Qt5::Widgets)
